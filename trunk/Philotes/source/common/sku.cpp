//----------------------------------------------------------------------------
// FILE: sku.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "excel.h"
#include "filepaths.h"
#include "language.h"
#include "prime.h"
#include "region.h"
#include "sku.h"
#include "stringtable.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
#define SKU_FILE_MAGIC		'sKuM'
#define SKU_FILE_VERSION	(1)
#define SKU_FILENAME		"sku.sku"

//----------------------------------------------------------------------------
struct SKU_GLOBALS
{
	char szSKUFilePath[ MAX_PATH ];
	int nSKUCurrent;
};
static SKU_GLOBALS sgtSKUGlobals;  // One per app
static char sgszSKUToValidate[ DEFAULT_INDEX_SIZE ] = { 0 };

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSKUGlobalsInit(
	SKU_GLOBALS *pGlobals)
{
	ASSERTX_RETFALSE( pGlobals, "Expected SKU globals" );
	
	// set path to the file containing the SKU setting
	PStrPrintf( pGlobals->szSKUFilePath, MAX_PATH, "%s%s", FILE_PATH_DATA, SKU_FILENAME );
	
	// init SKU
	pGlobals->nSKUCurrent = INVALID_LINK;
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SKU_GLOBALS *sSKUGlobalsGet(
	void)
{
	return &sgtSKUGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUExcelPostProcessAll(
	EXCEL_TABLE *pTable)
{
	ASSERTX_RETFALSE( pTable, "Expected table" );

	for (unsigned int nSKU = 0; nSKU < ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_SKU ); ++nSKU)
	{
		SKU_DATA *pSKUData = (SKU_DATA *)SKUGetData( nSKU );
		if (pSKUData)
		{
			if (pSKUData->szMovielistIntro[ 0 ] != 0)
			{
				pSKUData->nMovielistIntro = ExcelGetLineByStringIndex( EXCEL_CONTEXT(), DATATABLE_MOVIELISTS, pSKUData->szMovielistIntro );
			}
		}
	}
	
	return TRUE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SKU_DATA *SKUGetData(
	int nSKU)
{
	return (const SKU_DATA *)ExcelGetData( NULL, DATATABLE_SKU, nSKU );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SKUGetByName( 
	GAME *pGame,
	const char *pszSKU)
{
	ASSERTX_RETINVALID( pszSKU, "Expected SKU string" );
	
	int nNumRows = ExcelGetNumRows( pGame, DATATABLE_SKU );
	for (int i = 0; i < nNumRows; ++i)
	{
		const SKU_DATA *pSKUData = SKUGetData( i );
		if (pSKUData && PStrICmp( pSKUData->szName, pszSKU ) == 0)
		{
			return i;
		}
		
	}
	return INVALID_LINK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUUsesBackgroundTextureLanguage(
	int nSKU,
	LANGUAGE eLanguageBackgroundTexure)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected sku" );

	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETFALSE( pSKUData, "Expected sku data" );

	// go through languages in this SKU	
	APP_GAME eAppGame = AppGameGet();
	for (int i = 0; i < SKU_MAX_LANGUAGES; ++i)
	{
		LANGUAGE eLanguage = pSKUData->eLanguages[ i ];
		if (eLanguage != LANGUAGE_INVALID)
		{
			const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
			if (pLanguageData)
			{
				if (pLanguageData->eLanguageBackgroundTextures == eLanguageBackgroundTexure)
				{
					return TRUE;
				}
			}
		}
	}
		
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUWriteToPak( 
	int nSKU)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Invalid SKU" );
	const SKU_GLOBALS *pGlobals = sSKUGlobalsGet();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );	
	
	// get the SKU code
	DWORD dwCodeSKU = ExcelGetCode( NULL, DATATABLE_SKU, nSKU );
	
	// add to buffer
	WRITE_BUF_DYNAMIC tBuffer;
	ASSERT_RETFALSE( tBuffer.PushInt( (DWORD)SKU_FILE_MAGIC ) );
	ASSERT_RETFALSE( tBuffer.PushInt( (DWORD)SKU_FILE_VERSION ) );
	ASSERT_RETFALSE( tBuffer.PushInt( dwCodeSKU ) );
	
	// save file
	UINT64 iGenTime = 0;
	ASSERTX_RETFALSE( tBuffer.Save( pGlobals->szSKUFilePath, NULL, &iGenTime ), "Error saving SKU file" );
	
	// add to pak
	DECLARE_LOAD_SPEC( tLoadSpec, pGlobals->szSKUFilePath );
	tLoadSpec.buffer = (void *)tBuffer.GetBuf();
	tLoadSpec.size = tBuffer.GetPos();
	tLoadSpec.gentime = iGenTime;
	tLoadSpec.pakEnum = PAK_LOCALIZED;
	if (PakFileAddFile( tLoadSpec ) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ReadSKUFromFileBuffer(
	BYTE *pFileData,
	DWORD dwSize)
{
	int nSKU = INVALID_LINK;
	
	if (pFileData)
	{
	
		// setup buffer to read
		BYTE_BUF tByteBuffer( pFileData, dwSize );

		// read all data
		DWORD dwMagic;
		DWORD dwVersion;
		DWORD dwSKUCode;
		ASSERT_RETFALSE( tByteBuffer.PopBuf( &dwMagic, sizeof( DWORD ) ) );
		ASSERT_RETFALSE( tByteBuffer.PopBuf( &dwVersion, sizeof( DWORD ) ) );
		ASSERT_RETFALSE( tByteBuffer.PopBuf( &dwSKUCode, sizeof( DWORD ) ) );

		// validate
		if (dwMagic != SKU_FILE_MAGIC)
		{
			ASSERTX( 0, "Expected SKU file magic" );
		}
		else
		{
			
			// get SKU
			nSKU = ExcelGetLineByCode( NULL, DATATABLE_SKU, dwSKUCode );
			ASSERTX( nSKU != INVALID_ID, "Invalid SKU read from file" );
			
		}

	}
	
	return nSKU;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSKUReadFromPak(
	void)
{
	const SKU_GLOBALS *pGlobals = sSKUGlobalsGet();
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );
	int nSKU = INVALID_LINK;

	if (AppCommonUsePakFile())
	{
	
		// load from pak file
		DECLARE_LOAD_SPEC( tLoadSpec, pGlobals->szSKUFilePath );
		tLoadSpec.flags |= PAKFILE_LOAD_FORCE_FROM_PAK;
		tLoadSpec.pakEnum = PAK_LOCALIZED;
		BYTE *pFileData = (BYTE *)PakFileLoadNow( tLoadSpec );
		if (pFileData)
		{
		
			// must be from pak
			if (tLoadSpec.frompak == FALSE)
			{
				ASSERTX( 0, "Did not load sku file from pak" );
			}
			else
			{
			
				// read from file buffer
				nSKU = ReadSKUFromFileBuffer( pFileData, tLoadSpec.bytesread );
							
			}		
			
		}

		// cleanup file data
		if (pFileData)
		{
			FREE( NULL, pFileData );
			pFileData = NULL;
		}

	}
			
	return nSKU;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SKUGetDefault(
	void)
{
	
	// scan the datatable and look for what we've marked as default
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_SKU );
	for (int i = 0; i < nNumRows; ++i)
	{
		const SKU_DATA *pSKUData = SKUGetData( i );
		#if ISVERSION(SERVER_VERSION)
		{
			// server version will always look for the SKU we've designated as the server SKU
			if (pSKUData->bServer)
			{
				return i;
			}
		}
		#else
		{
			#if ISVERSION( DEVELOPMENT )
			{
				// during development, our fallback is the development SKU
				if (pSKUData->bDeveloper)
				{
					return i;
				}
			}
			#else
			{			
				// as a fallback, clients that have edited away all the places we have chosen to
				// store the SKU by hacking etc, we will set them to the default (unknown) sku
				if (pSKUData->bDefault)
				{
					return i;
				}
			}
			#endif
			
		}
		#endif
		
	}
	
	return INVALID_LINK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSKUSetCurrent(
	int nSKU)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU" );
	SKU_GLOBALS *pGlobals = sSKUGlobalsGet();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );

	// the server should only ever set the SKU to the server SKU
	#if ISVERSION(SERVER_VERSION)	
		const SKU_DATA *pSKUDataServer = SKUGetData( nSKU );
		ASSERTV( pSKUDataServer->bServer, "Server not allowed to use the SKU '%d' (%s), it must use the server SKU", nSKU, pSKUDataServer->szName );
		nSKU = SKUGetDefault();
	#endif 
		
	// save in global
	pGlobals->nSKUCurrent = nSKU;
	
	// init app flags based on the SKU options
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETFALSE( pSKUData, "Expected SKU data" );
	AppSetCensorFlag( AF_CENSOR_LOCKED, pSKUData->bCensorLocked, TRUE );
	AppSetCensorFlag( AF_CENSOR_PARTICLES, pSKUData->bCensorParticles, TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_BONE_SHRINKING, pSKUData->bCensorBoneShrinking, TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_HUMANS, pSKUData->bCensorMonsterClassReplacementsNoHumans, TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_GORE, pSKUData->bCensorMonsterClassReplacementsNoGore,   TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_PVP, pSKUData->bCensorPvP, TRUE );

	return TRUE;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUInitializeForApp(
	void)
{
	SKU_GLOBALS *pGlobals = sSKUGlobalsGet();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );
	
	// init sku globals
	if (sSKUGlobalsInit( pGlobals ) == FALSE)
	{
		return FALSE;
	}
	
	// If we're force loading all language files under -fillloadxml, than we shouldn't load the sku.sku file
	if (AppCommonGetForceLoadAllLanguages()) 
	{
		return TRUE;
	}

	// forget hammer
	if (AppIsHammer())
	{
		return TRUE;
	}
	
	// read SKU from pakfile
	int nSKU = sSKUReadFromPak();
	
	// if no SKU in pak file, use the default one
	if (nSKU == INVALID_LINK)
	{
	
		// search for the SKU in the stand alone file on the hard drive
		DWORD dwSize = 0;
		BYTE *pFileData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE( NULL, pGlobals->szSKUFilePath, &dwSize ));
		if (pFileData)
		{
			nSKU = ReadSKUFromFileBuffer( pFileData, dwSize );
			FREE( NULL, pFileData );
		}
		
		// if still no SKU, get the default SKU
		if (nSKU == INVALID_LINK)
		{
			nSKU = SKUGetDefault();
		}
		
	}

	// set the current SKU
	return sSKUSetCurrent( nSKU );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUOverride(
	int nSKU)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU" );
	return sSKUSetCurrent( nSKU );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUContainsLanguage(
	int nSKU,
	LANGUAGE eLangauge,
	BOOL bCheckDevelopmentLanguage)
{
	if (AppCommonGetForceLoadAllLanguages()) 
	{
		return TRUE;
	}

	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU" );
	ASSERTX_RETFALSE( eLangauge != LANGUAGE_INVALID, "Expected langauge" );
	
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	if (pSKUData)
	{
		for (int i = 0; i < SKU_MAX_LANGUAGES; ++i)
		{
			if (pSKUData->eLanguages[ i ] == eLangauge)
			{
				return TRUE;
			}
		}
	}

	// if this is the development version, and this is the development language, it is included
	if (bCheckDevelopmentLanguage)
	{
		#if ISVERSION(DEVELOPMENT)
		{
			LANGUAGE eLanguageDev = LanguageGetDevLanguage();
			if (eLanguageDev != LANGUAGE_INVALID && eLanguageDev == eLangauge)
			{
				return TRUE;
			}
		}
		#endif
	}
				
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUContainsAudioLanguage(
	int nSKU,
	LANGUAGE eLanguageAudio)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected sku link" );
	ASSERTX_RETFALSE( eLanguageAudio != LANGUAGE_INVALID, "Expected language link" );
	
	// get sku data
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETFALSE( pSKUData, "Expected sku data" );
	
	// go through each language
	APP_GAME eAppGame = AppGameGet();
	for (int i = 0; i < SKU_MAX_LANGUAGES; ++i)
	{
		LANGUAGE eLanguage = pSKUData->eLanguages[ i ];
		if (eLanguage != LANGUAGE_INVALID)
		{
			// check for audio language
			if (LanguageGetAudioLanguage( eAppGame, eLanguage ) == eLanguageAudio)
			{
				return TRUE;
			}
			
		}
		
	}

	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SKUGetCurrent()
{
	SKU_GLOBALS *pGlobals = sSKUGlobalsGet();
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );
	
	return pGlobals->nSKUCurrent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SKUGetIntroMovieList(
	int nSKU)
{	
	ASSERTX_RETINVALID( nSKU != INVALID_LINK, "Expected SKU link" );

	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	
	if (pSKUData)
	{
		return pSKUData->nMovielistIntro;	
	}
	
	return INVALID_LINK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUContainsRegion(
	int nSKU,
	WORLD_REGION eRegion)
{
	if (AppCommonGetForceLoadAllLanguages()) {
		return TRUE;
	}

	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU link" );
	ASSERTX_RETFALSE( eRegion != WORLD_REGION_INVALID, "Expected region link" );
	
	// get SKU data
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETFALSE( pSKUData, "Expected SKU data" );
	
	// search each region
	for (int i = 0; i < SKU_MAX_REGIONS; ++i)
	{
		WORLD_REGION eRegionOther = pSKUData->eRegions[ i ];
		if (eRegion == eRegionOther)
		{
			return TRUE;
		}
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORLD_REGION SKUGetSingleRegion(
	int nSKU)
{
	ASSERTX_RETVAL( nSKU != INVALID_LINK, WORLD_REGION_INVALID, "Expected SKU link" );
	
	// get sku data
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETVAL( pSKUData, WORLD_REGION_INVALID, "Expected SKU data" );
	
	// check the regions, if there is only one, we can use it
	WORLD_REGION eRegion = WORLD_REGION_INVALID;
	for (int i = 0; i < SKU_MAX_REGIONS; ++i)
	{
		WORLD_REGION eReginInSKU = pSKUData->eRegions[ i ];
		if (eReginInSKU != WORLD_REGION_INVALID)
		{
			
			// if we've already found a region, this SKU cannot represent a single region, sorry
			if (eRegion != WORLD_REGION_INVALID)
			{
				return WORLD_REGION_INVALID;
			}
			
			// save this region
			eRegion = eReginInSKU;
						
		}
		
	}
	
	// return the single region that is used for this SKU
	return eRegion;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUAllowsHQMovies(
	int nSKU)
{
	ASSERTX_RETTRUE( nSKU != INVALID_LINK, "Expected SKU link" );

	// get sku data
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETTRUE( pSKUData, "Expected SKU data" );

	return !pSKUData->bLowQualityMoviesOnly;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SKUCensorsMovies(
	int nSKU)
{
	ASSERTX_RETTRUE( nSKU != INVALID_LINK, "Expected SKU link" );

	// get sku data
	const SKU_DATA *pSKUData = SKUGetData( nSKU );
	ASSERTX_RETTRUE( pSKUData, "Expected SKU data" );

	return pSKUData->bCensorMovies;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SKUSetSKUToValidate(
	const char *pszSKUToValidate)
{
	ASSERTX_RETURN( pszSKUToValidate, "Expected sku to validate" );
		
	// copy to globals
	PStrCopy( sgszSKUToValidate, pszSKUToValidate, arrsize( sgszSKUToValidate ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SKUGetSKUToValidate(
	void)
{
	int nSKUToValidate = INVALID_LINK;
	if (sgszSKUToValidate[ 0 ] != 0)
	{
		nSKUToValidate = SKUGetByName( NULL, sgszSKUToValidate );	
	}
	return nSKUToValidate;		
}
