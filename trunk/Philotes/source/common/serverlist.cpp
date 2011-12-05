//----------------------------------------------------------------------------
// FILE: serverlist.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "serverlist.h"
#include "stringreplacement.h"
#include "markup.h"
#include "markupUnicode.h"
#include "region.h"
#include "prime.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
T_SERVERLIST g_Serverlist;
T_SERVERLIST g_ServerlistOverride;

#define XML_EXTENSION L".xml"
#define UNICODE_EXTENSION L".uni"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct WEB_AREA_VARIABLE
{
	STRING_REPLACEMENT eToken;
	STRING_REPLACEMENT eReplacement;
};
static const WEB_AREA_VARIABLE sgtWebAreaStagingLookup[] = 
{
	{ SR_HTTP_ACCOUNTS,			SR_HTTP_ACCOUNTS_STAGING },
	{ SR_HTTPS_ACCOUNTS,		SR_HTTPS_ACCOUNTS_STAGING },
	{ SR_HTTP_WWW,				SR_HTTP_WWW_STAGING },
	{ SR_HTTPS_WWW,				SR_HTTPS_WWW_STAGING },
	{ SR_HTTP_FORUMS,			SR_HTTP_FORUMS_STAGING },
	{ SR_HTTPS_FORUMS,			SR_HTTPS_FORUMS_STAGING },
};
static const int sgnNumAreaStagingLookup = arrsize( sgtWebAreaStagingLookup );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ServerListURLMakeStaging(
	WCHAR *puszBuffer,
	int nBufferLen)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLen, "Invalid buffer length" );

	// scan through the table of web variable replacements
	for (int i = 0; i < sgnNumAreaStagingLookup; ++i)
	{
		const WEB_AREA_VARIABLE *pLookup = &sgtWebAreaStagingLookup[ i ];
		ASSERTX_CONTINUE( pLookup, "Expected lookup" );
		
		// do replacements
		if (pLookup->eToken != SR_INVALID && pLookup->eReplacement != SR_INVALID)
		{
		
			// do not do this replacement if the replacement token exists in
			// the string, yeah this isn't very clean :(
			const WCHAR *puszReplacement = StringReplacementTokensGet( pLookup->eReplacement );
			ASSERTX_CONTINUE( puszReplacement, "Expected replacement" );			
			if (PStrStrI( puszBuffer, puszReplacement ) == NULL)
			{
			
				// get the token to search for and its replacement
				const WCHAR *puszToken = StringReplacementTokensGet( pLookup->eToken );
				ASSERTX_CONTINUE( puszToken, "Expected token" );
				
				// do the replacement
				PStrReplaceToken( puszBuffer, nBufferLen, puszToken, puszReplacement );

			}
						
		}
				
	}
	
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
			case RU_LAUNCHER_PATCH_DATA_SERVER_01:
			case RU_LAUNCHER_PATCH_DATA_SERVER_02:
			case RU_LAUNCHER_PATCH_DATA_SERVER_03:
			case RU_LAUNCHER_PATCH_DATA_SERVER_04:
			case RU_LAUNCHER_PATCH_DATA_SERVER_05:
			case RU_LAUNCHER_PATCH_DATA_SERVER_06:
			case RU_LAUNCHER_PATCH_DATA_SERVER_07:
			case RU_LAUNCHER_PATCH_DATA_SERVER_08:
			{
				return RU_LAUNCHER_PATCH_DATA_SERVER_STAGING;
			}
			
		}

	}
	
	return eRegionURL;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *ServerListURLGetTag(
	REGION_URL eRegionURL,
	BOOL bUseStaging /*= FALSE*/)
{

	eRegionURL = sGetRegionURLToUse( eRegionURL, bUseStaging );

	ASSERT_RETNULL(eRegionURL >= 0);
	ASSERT_RETNULL(eRegionURL < RU_NUM_URLS);

	return s_szServerListURLTagNames[eRegionURL];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sServerListLoad(
	T_SERVERLIST &ServerList, 
	LPCWSTR szFileName)
{
	char szData[MAX_XML_STRING_LENGTH];

	#ifdef UNICODE_SERVERLIST
		CUnicodeMarkup ServerListXml;
		LPCWSTR wszData = NULL;
		#define SERVERLIST_STRING(x) L##x
	#else
		CMarkup ServerListXml;
		LPCSTR wszData = NULL;
		#define SERVERLIST_STRING(x) x
	#endif

	#define SERVERLIST_REQUIRED_PARM(arg)																	\
		ASSERT_RETFALSE(ServerListXml.FindChildElem(SERVERLIST_STRING(#arg)) && ServerListXml.IntoElem());	\
		wszData = ServerListXml.GetData();																	\
		PStrCvt(pServer->arg, wszData, arrsize(pServer->arg));												\
		ASSERT_RETFALSE(ServerListXml.OutOfElem());
	
	if (!ServerListXml.Load(szFileName))
	{
		return FALSE;
	}

	if (!ServerListXml.FindElem(SERVERLIST_STRING("SERVERLIST_DEFINITION")))
	{
		ASSERT_RETFALSE(ServerListXml.FindElem(SERVERLIST_STRING("SERVERLISTOVERRIDE_DEFINITION")));
	}

	if (ServerListXml.FindChildElem(SERVERLIST_STRING("pRegionViewsCount")))
	{
		ASSERT_RETFALSE(ServerListXml.IntoElem());
		wszData = ServerListXml.GetData();
		ServerList.nRegionViewCount = PStrToInt(wszData);
		ASSERT_RETFALSE(ServerListXml.OutOfElem());

		ServerList.pRegionViews = (T_REGION_VIEW*)MALLOCZ(NULL, ServerList.nRegionViewCount * sizeof(T_REGION_VIEW));
		ASSERT_RETFALSE(ServerList.pRegionViews);

		int i = 0;
		while (ServerListXml.FindChildElem(SERVERLIST_STRING("REGION_VIEW_DEFINITION")))
		{
			ASSERT_BREAK(i < ServerList.nRegionViewCount);

			T_REGION_VIEW *pServer = &ServerList.pRegionViews[i];

			ASSERT_RETFALSE(ServerListXml.IntoElem());

				ASSERT_RETFALSE(ServerListXml.FindChildElem(SERVERLIST_STRING("nRegion")) && ServerListXml.IntoElem());
				wszData = ServerListXml.GetData();
				PStrCvt(szData, wszData, MAX_XML_STRING_LENGTH);
				pServer->eRegion = RegionGetByName(AppGameGet(), szData);
				ASSERT_RETFALSE(ServerListXml.OutOfElem());

				ASSERT_RETFALSE(ServerListXml.FindChildElem(SERVERLIST_STRING("pnViewRegionsCount")));
				ASSERT_RETFALSE(ServerListXml.IntoElem());
				wszData = ServerListXml.GetData();
				pServer->nViewRegionCount = PStrToInt(wszData);
				ASSERT_RETFALSE(ServerListXml.OutOfElem());

				pServer->peViewRegions = (WORLD_REGION *)MALLOCZ(NULL, pServer->nViewRegionCount * sizeof(WORLD_REGION));
				ASSERT_RETFALSE(pServer->peViewRegions);

				int j = 0;
				while (ServerListXml.FindChildElem(SERVERLIST_STRING("pnViewRegions")))
				{
					ASSERT(j < pServer->nViewRegionCount);

					ASSERT_RETFALSE(ServerListXml.IntoElem());
					wszData = ServerListXml.GetData();
					PStrCvt(szData, wszData, MAX_XML_STRING_LENGTH);
					pServer->peViewRegions[j] = RegionGetByName(AppGameGet(), szData);
					ASSERT_RETFALSE(ServerListXml.OutOfElem());

					++j;
				}
				ASSERT(j == pServer->nViewRegionCount);

				while (ServerListXml.FindChildElem())
				{
					ASSERT_RETFALSE(ServerListXml.IntoElem());
					for (int k=0; k < RU_NUM_URLS; ++k)
					{
						const char *pszTagName = ServerListURLGetTag((REGION_URL)k);
						const char *pszXMLTagName = NULL;
						#ifdef UNICODE_SERVERLIST
							const int MAX_TAG_NAME = 256;
							char szXMLTagName[ MAX_TAG_NAME ];
							const WCHAR *puszXMLTagName = ServerListXml.GetTagName();
							PStrCvt(szXMLTagName, puszXMLTagName, MAX_TAG_NAME );
							pszXMLTagName = szXMLTagName;
						#else
							pszXMLTagName = ServerListXml.GetTagName();
						#endif

						if (!PStrCmp(pszXMLTagName, pszTagName))
						{
							PStrCvt(pServer->szURLs[k], ServerListXml.GetData(), arrsize(pServer->szURLs[k]));
						}
					}
					ASSERT_RETFALSE(ServerListXml.OutOfElem());
				}

			ASSERT_RETFALSE(ServerListXml.OutOfElem());
			++i;
		}
		ASSERT(i == ServerList.nRegionViewCount);
	}
	else
	{
		ServerList.nRegionViewCount = 1;
		ServerList.pRegionViews = (T_REGION_VIEW*)MALLOCZ(NULL, ServerList.nRegionViewCount * sizeof(T_REGION_VIEW));
		ASSERT_RETFALSE(ServerList.pRegionViews);

		T_REGION_VIEW *pServer = &ServerList.pRegionViews[0];
		pServer->nViewRegionCount = 1;

		pServer->peViewRegions = (WORLD_REGION *)MALLOCZ(NULL, pServer->nViewRegionCount * sizeof(WORLD_REGION));
		ASSERT_RETFALSE(pServer->peViewRegions);
	}

	ServerListXml.ResetPos();

	if (ServerListXml.FindChildElem(SERVERLIST_STRING("pServerDefinitionsCount")))
	{
		ASSERT_RETFALSE(ServerListXml.IntoElem());
		wszData = ServerListXml.GetData();
		ServerList.nServerDefinitionCount = PStrToInt(wszData);
		ASSERT_RETFALSE(ServerListXml.OutOfElem());

		ServerList.pServerDefinitions = (T_SERVER*)MALLOCZ(NULL, ServerList.nServerDefinitionCount * sizeof(T_SERVER));
		ASSERT_RETFALSE(ServerList.pServerDefinitions);

		int i = 0;
		while (ServerListXml.FindChildElem(SERVERLIST_STRING("SERVER_DEFINITION")))
		{
			ASSERT_BREAK(i < ServerList.nServerDefinitionCount);

			T_SERVER *pServer = &ServerList.pServerDefinitions[i];

			ASSERT_RETFALSE(ServerListXml.IntoElem());

				if (ServerListXml.FindChildElem(SERVERLIST_STRING("nRegion")))
				{
					ASSERT_RETFALSE(ServerListXml.IntoElem());
					wszData = ServerListXml.GetData();
					PStrCvt(szData, wszData, MAX_XML_STRING_LENGTH);
					pServer->eRegion = RegionGetByName(AppGameGet(), szData);
					ASSERT_RETFALSE(ServerListXml.OutOfElem());
				}
				else
				{
					pServer->eRegion = WORLD_REGION_NORTH_AMERICA;
				}

				ASSERT_RETFALSE(ServerListXml.FindChildElem(SERVERLIST_STRING("szCommonName")) && ServerListXml.IntoElem());
				wszData = ServerListXml.GetData();
				PStrCvt(pServer->wszCommonName, wszData, arrsize(pServer->wszCommonName));
				ASSERT_RETFALSE(ServerListXml.OutOfElem());

				SERVERLIST_REQUIRED_PARM(szOperationalName);
				SERVERLIST_REQUIRED_PARM(szIP);

				if (ServerListXml.FindChildElem(SERVERLIST_STRING("bAllowRandomSelect")))
				{
					ASSERT_RETFALSE(ServerListXml.IntoElem());
					wszData = ServerListXml.GetData();
					pServer->bAllowRandomSelect = (BOOL)PStrToInt(wszData);
					ASSERT_RETFALSE(ServerListXml.OutOfElem());
				}
				else
				{
					pServer->bAllowRandomSelect = TRUE;
				}

				while (ServerListXml.FindChildElem())
				{
					ASSERT_RETFALSE(ServerListXml.IntoElem());
					for (int k=0; k < RU_NUM_URLS; ++k)
					{
						const char *pszTagName = ServerListURLGetTag((REGION_URL)k);
						const char *pszXMLTagName = NULL;
						#ifdef UNICODE_SERVERLIST
							const int MAX_TAG_NAME = 256;
							char szXMLTagName[ MAX_TAG_NAME ];
							const WCHAR *puszXMLTagName = ServerListXml.GetTagName();
							PStrCvt(szXMLTagName, puszXMLTagName, MAX_TAG_NAME );
							pszXMLTagName = szXMLTagName;
						#else
							pszXMLTagName = ServerListXml.GetTagName();
						#endif
					
						if (!PStrCmp(pszXMLTagName, pszTagName))
						{
							PStrCvt(pServer->szURLs[k], ServerListXml.GetData(), arrsize(pServer->szURLs[k]));
						}
					}
					ASSERT_RETFALSE(ServerListXml.OutOfElem());
				}

			ASSERT_RETFALSE(ServerListXml.OutOfElem());
			++i;
		}
		ASSERT(i == ServerList.nServerDefinitionCount);
	}
	else
	{
		ServerList.nServerDefinitionCount = 0;
		ServerList.pServerDefinitions = NULL;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ServerListLoad(
	const WCHAR *puszServerListFilename,
	const WCHAR *puszServerListOverrideFilename /*= NULL*/)
{
	ASSERTX_RETFALSE( puszServerListFilename, "Expected filename for serverlist" );
	const int MAX_FILENAME = 1024;
	WCHAR uszFilename[ MAX_FILENAME ] = { 0 };
	WCHAR uszFilenameOverride[ MAX_FILENAME ] = { 0 };

	// copy filename to local
	PStrCopy( uszFilename, puszServerListFilename, MAX_FILENAME );
	
	// copy override filename to local if present
	if (puszServerListOverrideFilename)
	{
		PStrCopy( uszFilenameOverride, puszServerListOverrideFilename, MAX_FILENAME );
	}
	
	// load the serverlist
	ASSERTV_RETFALSE( sServerListLoad( g_Serverlist, uszFilename ), "Unable to load serverlist '%s'", uszFilename);
	
	// load the override (if present)
	if (uszFilenameOverride[ 0 ])
	{
		if (sServerListLoad( g_ServerlistOverride, uszFilenameOverride ) == FALSE)
		{
			trace( "INFO: Override serverlist not loaded" );
		}
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ServerListLoad(
	GAME *pGame)
{
	REF( pGame );
		
	const WCHAR *puszServerListFilename = AppIsHellgate() ? SERVER_LIST_HELLGATE : SERVER_LIST_MYTHOS;;
	const WCHAR *puszServerListOverrideFilename = AppIsHellgate() ? SERVER_LIST_OVERRIDE_HELLGATE : SERVER_LIST_OVERRIDE_MYTHOS;;
	return ServerListLoad(puszServerListFilename, puszServerListOverrideFilename);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sServerListFree(
	T_SERVERLIST &ServerList)
{
	if (ServerList.pRegionViews)
	{
		for (int i=0; i<ServerList.nRegionViewCount; ++i)
		{
			if (ServerList.pRegionViews[i].peViewRegions)
			{
				FREE(NULL, ServerList.pRegionViews[i].peViewRegions);
				ServerList.pRegionViews[i].peViewRegions = NULL;
			}
		}

		FREE(NULL, ServerList.pRegionViews);
		ServerList.pRegionViews = NULL;
	}

	if (ServerList.pServerDefinitions)
	{
		FREE(NULL, ServerList.pServerDefinitions);
		ServerList.pServerDefinitions = NULL;
	}
}

void ServerListFree(
	void)
{
	sServerListFree(g_Serverlist);
	sServerListFree(g_ServerlistOverride);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
T_SERVERLIST * ServerListGet(
	void)
{
	return &g_Serverlist;
}

T_SERVERLIST * ServerListOverrideGet(
	void)
{
	return &g_ServerlistOverride;
}
