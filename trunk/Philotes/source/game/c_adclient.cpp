//-------------------------------------------------------------------------------------------------
// Ad Client
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "c_adclient.h"
#include "sku.h"
#include "excel.h"

#if defined(AD_CLIENT_ENABLED) && !defined(TUGBOAT)

#pragma message("Compiling Ad Client")

//#define TEST_AD_CLIENT
#define TEST_ZONE_NAME	"TestZone"
#define TEST_IE_NAME	"Test1"

#include "level.h"
#include "e_settings.h"
#include "uix_priv.h"
#include "gameoptions.h"

#include "MassiveClientCore.h"
#include "MassiveAdObjectSubscriber.h"
#include "massive_publickey.h"
#include "massive_publickey_de.h"
#include "massive_publickey_eu.h"
#include "massive_publickey_na.h"
#include "massive_publickey_kr.h"
#include "massive_publickey_kr_en.h"

using namespace MassiveAdClient3;

struct AD_CLIENT_AD_MEDIA
{
	int nId;
	AD_CLIENT_AD_MEDIA * pNext;

	const char * pMediaData;
	unsigned int nDataLength;
	unsigned int nMediaType;
	unsigned int nMediaID;
};
static CHash<AD_CLIENT_AD_MEDIA> sgtAdMedia;

struct AD_CLIENT_AD_INSTANCE
{
	int nId;
	AD_CLIENT_AD_INSTANCE * pNext;

	class HellgateAdObjectSubscriber * pAdObjectSubscriber;
	unsigned int nAdMediaId;
	AD_DOWNLOADED_CALLBACK pfnDownloadCallback;
	AD_CALCULATE_IMPRESSION_CALLBACK pfnImpressionCallback;
	int nLevelIndex;
	BOOL bNonReporting;
};

static int sgnAdInstanceIdNext = 0;
static CHash<AD_CLIENT_AD_INSTANCE> sgtAdInstances;

class HellgateAdObjectSubscriber : public CMassiveAdObjectSubscriber
{
public:
	HellgateAdObjectSubscriber(const char *szName) : CMassiveAdObjectSubscriber(szName) {}
	virtual massiveBool MediaDownload(
		const massiveU32 uAssetID)
	{
		AD_CLIENT_AD_MEDIA * pAdMedia = HashGet(sgtAdMedia, uAssetID);
		if(pAdMedia)
		{
			if(VerifyData(uAssetID, pAdMedia->nDataLength, pAdMedia->pMediaData))
			{
				return FALSE;
			}
			else
			{
				// Data failed to verify!  Let's destroy our internal copy and redownload it
				FREE(NULL, pAdMedia->pMediaData);
				HashRemove(sgtAdMedia, uAssetID);
			}
		}
		return TRUE;
	}

	virtual massiveBool MediaDownloadComplete(
		const char *MediaData, 
		const massiveU32 uDataLength, 
		const massiveU32 uMediaType, 
		const massiveU32 uAssetID)
	{
		AD_CLIENT_AD_MEDIA * pAdMedia = HashGet(sgtAdMedia, uAssetID);
		if(!pAdMedia)
		{
			pAdMedia = HashAdd(sgtAdMedia, uAssetID);
			pAdMedia->pMediaData = (const char *)MALLOC(NULL, uDataLength);
			MemoryCopy((void*)pAdMedia->pMediaData, uDataLength, MediaData, uDataLength);
			pAdMedia->nDataLength = uDataLength;
			pAdMedia->nMediaType = uMediaType;
			pAdMedia->nMediaID = uAssetID;
		}

		AD_CLIENT_AD_INSTANCE * pAdInstance = HashGet(sgtAdInstances, nAdInstanceId);
		ASSERT_RETFALSE(pAdInstance);

		if(pAdInstance->pfnDownloadCallback)
		{
			return pAdInstance->pfnDownloadCallback(nAdInstanceId, pAdMedia->pMediaData, pAdMedia->nDataLength, pAdInstance->bNonReporting);
		}
		return FALSE;
	}

	virtual void Tick()
	{
		AD_CLIENT_AD_INSTANCE * pAdInstance = HashGet(sgtAdInstances, nAdInstanceId);
		ASSERT_RETURN(pAdInstance);
		ASSERT_RETURN( pAdInstance->pfnImpressionCallback );

		AD_CLIENT_IMPRESSION tImpression;
		if(UIComponentGetVisibleByEnum(UICOMP_SKILLMAP))
		{
			tImpression.bInView = FALSE;
			tImpression.fAngle = 0.0f;
			tImpression.nSize = 0;
		}
		else
		{
			pAdInstance->pfnImpressionCallback( nAdInstanceId, tImpression );
		}
		c_AdClientReportImpression( nAdInstanceId, tImpression );
	}

	int nAdInstanceId;
};


//-------------------------------------------------------------------------------------------------
static CMassiveClientCore * sgpAdClientCore = NULL;;
static int gnAdClientCurrentLevelIndex = INVALID_ID;

// We need SKUs from Massive
static char sgpszSKUVersion[] = "1.0";

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void *AdClientMalloc(size_t sz)
{
	/*
	void * ptr = MALLOC(NULL, (unsigned int)sz);
	trace("Allocating %p\n", ptr);
	return ptr;
	// */
	return MALLOC(NULL, (unsigned int)sz);
}

void AdClientFree(void * ptr)
{
	//trace("Freeing %p\n", ptr);
	FREE(NULL, ptr);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct AD_CLIENT_PUBLIC_KEY_MAP
{
	const char * pszKeyName;
	char * pszSKUName;
	const unsigned char * pPublicKey;
};

static AD_CLIENT_PUBLIC_KEY_MAP sgpPublicKeyMap[] =
{
	{ "Disabled",	"Disabled",							NULL					},
	{ "Namco",		"Namco_Hellgate_London_pc",			MASSIVE_PublicKey		},
	{ "EU",			"ea_hellgate_london_pc_eu",			MassivePublicKeyEU		},
	{ "German",		"ea_hellgate_london_pc_de",			MassivePublicKeyDE		},
	{ "NorthAmerica","ea_hellgate_london_pc_na",		MassivePublicKeyNA		},
	{ "Korea",		"hanbit_hellgate_london_pc_kr",		MassivePublicKeyKR		},
	{ "EnglishKR",	"hanbit_hellgate_london_pc_kr_en",	MassivePublicKeyKREN	},
};

static int sgnPublicKeyMapSize = arrsize(sgpPublicKeyMap);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int c_sAdClientMapPublicKeyStringToKeyIndex(
	const char * pszKeyString)
{
	ASSERT_RETINVALID(pszKeyString);
	ASSERT_RETINVALID(sgnPublicKeyMapSize > 0);
	for(int i=0; i<sgnPublicKeyMapSize; i++)
	{
		if(PStrICmp(sgpPublicKeyMap[i].pszKeyName, pszKeyString) == 0)
		{
			return i;
		}
	}
	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientFixupSKUs()
{
	int nCount = ExcelGetNumRows(NULL, DATATABLE_SKU);
	for (int ii = 0; ii < nCount; ++ii)
	{
		SKU_DATA * sku_data = (SKU_DATA *)ExcelGetData(NULL, DATATABLE_SKU, ii);
		if(!sku_data)
		{
			continue;
		}

		sku_data->nAdPublicKey = c_sAdClientMapPublicKeyStringToKeyIndex(sku_data->szAdPublicKey);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientInit()
{
	if(AppIsHammer() || ! AppIsHellgate() || AppIsDemo() || AppIsBeta())
	{
		return;
	}
	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	if(tOptions.bDisableAdUpdates)
	{
		return;
	}

	int nAdPublicKeyIndex = 0;
	const SKU_DATA * pSKUData = SKUGetData(SKUGetCurrent());
	if(pSKUData && pSKUData->nAdPublicKey >= 0)
	{
		nAdPublicKeyIndex = pSKUData->nAdPublicKey;
	}
	if(sgpPublicKeyMap[nAdPublicKeyIndex].pPublicKey == NULL)
	{
		return;
	}

	ASSERT_RETURN(!sgpAdClientCore);

	MAD_MASSIVE_INIT_STRUCT tMassiveInitStruct;
	structclear(tMassiveInitStruct);
	tMassiveInitStruct.szSku = sgpPublicKeyMap[nAdPublicKeyIndex].pszSKUName;
	tMassiveInitStruct.szSkuVersion = sgpszSKUVersion;
	tMassiveInitStruct.szPublicKey = (unsigned char*)sgpPublicKeyMap[nAdPublicKeyIndex].pPublicKey;
	tMassiveInitStruct.u_libraryConfigFlags = MAD_DEFAULT_CONFIG_FLAGS;
#if ISVERSION(DEVELOPMENT)
	tMassiveInitStruct.eLogLevelFile = LOG_LEVEL_DEBUG;
#else
	tMassiveInitStruct.eLogLevelFile = LOG_LEVEL_NONE;
#endif
	tMassiveInitStruct.eLogLevelDebugger = LOG_LEVEL_NONE;
	tMassiveInitStruct.u_impressionFlushTime = 210000;
	tMassiveInitStruct.szThirdPartyID = "";
	tMassiveInitStruct.szThirdPartyService = "";

	CMassiveClientCore::SetCustomMemoryFunctions(AdClientMalloc, AdClientFree);
	sgpAdClientCore = CMassiveClientCore::Initialize(&tMassiveInitStruct);

	HashInit(sgtAdInstances, NULL, 8);
	HashInit(sgtAdMedia, NULL, 32);

	gnAdClientCurrentLevelIndex = INVALID_ID;

	e_AdClientInit();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sFreeAdInstance( AD_CLIENT_AD_INSTANCE * pInstance )
{
	ASSERT_RETURN( pInstance );
	V( e_AdInstanceFreeAd( pInstance->nId ) );
	FREE_DELETE(NULL, pInstance->pAdObjectSubscriber, HellgateAdObjectSubscriber);
	HashRemove(sgtAdInstances, pInstance->nId);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sFreeAllAdInstances( int nLevelIndex = INVALID_ID )
{
	for(AD_CLIENT_AD_INSTANCE * pInstance = HashGetFirst(sgtAdInstances); pInstance; )
	{
		AD_CLIENT_AD_INSTANCE * pNext = HashGetNext(sgtAdInstances, pInstance);
		if ( nLevelIndex == INVALID_ID || pInstance->nLevelIndex == nLevelIndex )
		{
			sFreeAdInstance( pInstance );
		}
		pInstance = pNext;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientShutdown()
{
	if(AppIsHammer() || ! AppIsHellgate())
	{
		return;
	}

	if ( gnAdClientCurrentLevelIndex != INVALID_ID )
		c_AdClientExitLevel( NULL, gnAdClientCurrentLevelIndex );

	for(AD_CLIENT_AD_MEDIA * pMedia = HashGetFirst(sgtAdMedia); pMedia; )
	{
		AD_CLIENT_AD_MEDIA * pNext = HashGetNext(sgtAdMedia, pMedia);
		FREE(NULL, pMedia->pMediaData);
		HashRemove(sgtAdMedia, pMedia->nId);
		pMedia = pNext;
	}

	if(sgpAdClientCore)
	{
		CMassiveClientCore::Shutdown(true, 1000); // Wait for one second
		sgpAdClientCore = NULL;
	}

	sFreeAllAdInstances();

	sgtAdMedia.Free();
	sgtAdInstances.Free();

	gnAdClientCurrentLevelIndex = INVALID_ID;

	e_AdClientDestroy();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientEnterLevel(
	GAME * pGame,
	int nLevelIndex)
{
	if(sgpAdClientCore)
	{
		ASSERT( gnAdClientCurrentLevelIndex == INVALID_ID );

		const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(nLevelIndex);
		ASSERT_RETURN(pLevelDef);

#ifdef TEST_AD_CLIENT
		massiveBool bSuccess = sgpAdClientCore->EnterZone(TEST_ZONE_NAME);
#else
		massiveBool bSuccess = sgpAdClientCore->EnterZone(pLevelDef->pszName);
#endif

		REF(bSuccess);
		//ASSERTX_RETURN(bSuccess, "Massive Client Core failed to enter zone");

		gnAdClientCurrentLevelIndex = nLevelIndex;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientExitLevel(
	 GAME * pGame,
	 int nLevelIndex)
{
	if(sgpAdClientCore)
	{
		ASSERT( gnAdClientCurrentLevelIndex == nLevelIndex );

		const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(nLevelIndex);
		ASSERT_RETURN(pLevelDef);

		// remove all of the ads from this level index
		sFreeAllAdInstances( nLevelIndex );

#ifdef TEST_AD_CLIENT
		massiveBool bSuccess = sgpAdClientCore->ExitZone(TEST_ZONE_NAME);
#else
		massiveBool bSuccess = sgpAdClientCore->ExitZone(pLevelDef->pszName);
#endif
		REF(bSuccess);
		//ASSERTX_RETURN(bSuccess, "Massive Client Core failed to exit zone");

		gnAdClientCurrentLevelIndex = INVALID_ID;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientUpdate()
{
	if(sgpAdClientCore)
	{
		massiveBool bSuccess = sgpAdClientCore->Tick();
		REF(bSuccess);
		//ASSERTX_RETURN(bSuccess, "Massive Client Core failed to update");
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_AdClientAddAd(
	const char * pszName,
	AD_DOWNLOADED_CALLBACK pfnDownloadCallback,
	AD_CALCULATE_IMPRESSION_CALLBACK pfnImpressionCallback)
{
	if(sgpAdClientCore)
	{
		int nId = sgnAdInstanceIdNext++;
		ASSERT_RETINVALID(nId >= 0);
		AD_CLIENT_AD_INSTANCE * pAdInstance = HashAdd(sgtAdInstances, nId);
		ASSERT_RETINVALID(pAdInstance);

		pAdInstance->pAdObjectSubscriber = (HellgateAdObjectSubscriber*)MALLOC(NULL, sizeof(HellgateAdObjectSubscriber));
		ASSERT_RETINVALID(pAdInstance->pAdObjectSubscriber);

#ifdef TEST_AD_CLIENT
		new (pAdInstance->pAdObjectSubscriber) HellgateAdObjectSubscriber(TEST_IE_NAME);
#else
		new (pAdInstance->pAdObjectSubscriber) HellgateAdObjectSubscriber(pszName);
#endif

		pAdInstance->nLevelIndex = gnAdClientCurrentLevelIndex;
		pAdInstance->pfnDownloadCallback = pfnDownloadCallback;
		pAdInstance->pfnImpressionCallback = pfnImpressionCallback;
		pAdInstance->pAdObjectSubscriber->nAdInstanceId = nId;
		pAdInstance->bNonReporting = FALSE;
		return nId;
	}
	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AdClientReportImpression(int nAdInstance, AD_CLIENT_IMPRESSION & tAdClientImpression)
{
	AD_CLIENT_AD_INSTANCE * pAdInstance = HashGet(sgtAdInstances, nAdInstance);
	ASSERT_RETURN(pAdInstance);
	ASSERT_RETURN(pAdInstance->pAdObjectSubscriber);

	MAD_Impression tImpression;
	structclear(tImpression);
	tImpression.reportAngle = ANGLE_DOTPRODUCT;
	tImpression.reportSize  = SIZE_LINEAR;
	tImpression.bInView     = tAdClientImpression.bInView ? true : false;
	tImpression.uSize       = tAdClientImpression.nSize;
	tImpression.fAngle      = tAdClientImpression.fAngle;

	int nWindowWidth, nWindowHeight;
	e_GetWindowSize(nWindowWidth, nWindowHeight);
	tImpression.siScreenWidth  = (short)nWindowWidth;
	tImpression.siScreenHeight = (short)nWindowHeight;

	// CML 2007.09.05 - Massive requested we not call SetImpression if the ad is non-reporting.
	if ( ! pAdInstance->bNonReporting )
		pAdInstance->pAdObjectSubscriber->SetImpression(&tImpression);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_AdClientIsDisabled()
{
	return (sgpAdClientCore == NULL);
}

#else

void c_AdClientInit() {}
void c_AdClientShutdown() {}

void c_AdClientEnterLevel(struct GAME * pGame, int nLevelIndex) {}
void c_AdClientExitLevel(struct GAME * pGame, int nLevelIndex) {}

void c_AdClientUpdate() {}

int c_AdClientAddAd(const char * pszName, AD_DOWNLOADED_CALLBACK pfnDownloadCallback, AD_CALCULATE_IMPRESSION_CALLBACK pfnImpressionCallback) { return INVALID_ID; }
void c_AdClientReportImpression(int nAdInstance, AD_CLIENT_IMPRESSION & tAdClientImpression) {}

void c_AdClientFixupSKUs()
{
	int nCount = ExcelGetNumRows(NULL, DATATABLE_SKU);
	for (int ii = 0; ii < nCount; ++ii)
	{
		SKU_DATA * sku_data = (SKU_DATA *)ExcelGetData(NULL, DATATABLE_SKU, ii);
		if(!sku_data)
		{
			continue;
		}

		sku_data->nAdPublicKey = INVALID_ID;
	}
}

BOOL c_AdClientIsDisabled() { return TRUE; }

#endif // AD_CLIENT_ENABLED
