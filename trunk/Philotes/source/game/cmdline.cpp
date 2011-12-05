//----------------------------------------------------------------------------
// prime.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "act.h"
#include "prime.h"
#include "appcommon.h"
#include "definition_common.h"
#include "definition_priv.h"
#include "c_authticket.h"
#include "c_sound_util.h"
#include "dataversion.h"
#include "fileversion.h"
#include "language.h"
#include "player.h"
#include "cmdline.h"
#include "markup.h"
#include "config.h"		// CHB 2006.12.22
#include "e_main.h"
#include "fillpak.h"
#include "sku.h"
#include "e_initdb.h"	// CHB 2007.05.22
#include "stringtable.h"
#include "c_credits.h"
#include "statssave.h"
#include "global_themes.h"
#include "namefilter.h"
#include "accountbadges.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void (*FN_APP_COMMAND_CALLBACK)(const TCHAR *);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct APP_COMMAND_LINE
{
	TCHAR *						cmd;
	unsigned int				strlen;
	FN_APP_COMMAND_CALLBACK		callback;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsWhitespace(
	TCHAR c)
{
	return (c == _T(' '));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkipWhitespace(
	const TCHAR *& str)
{
	while (sIsWhitespace(*str))
	{
		*str++;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCmdLineParseInteger(
	const TCHAR *& str,
	int & num,
	BOOL bAllowNegative,
	unsigned int nMaxDigits)
{
	BOOL isnegative = FALSE;
	if (bAllowNegative)
	{
		if (*str == _T('-'))
		{
			str++;
			isnegative = TRUE;
		}
	}
	num = 0;
	for (unsigned int ii = 0; ii < nMaxDigits; ii++)
	{
		if (*str < _T('0') || *str > _T('9'))
		{
			if (ii == 0)
			{
				return FALSE;
			}
			break;
		}
		num = num * 10 + (*str - _T('0'));
		str++;
	}
	if (isnegative)
	{
		num = -num;
	}
	if (*str >= _T('0') && *str <= _T('9'))		// shouldn't end on a digit
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCmdLineParseIPString(
	const TCHAR *& str,
	char * dest,
	unsigned int desten,
	const char * defaultip)
{
	if (defaultip)
	{
		PStrCopy(dest, defaultip, MAX_NET_ADDRESS_LEN);
	}
	else
	{
		dest[0] = 0;
	}

	BOOL bDefaultIP = FALSE;
	int digits[4];
	for (unsigned int ii = 0; ii < 4; ++ii)
	{
		if (!sCmdLineParseInteger(str, digits[ii], FALSE, 3))
		{
			return FALSE;
		}
		if (digits[ii] < 0 || digits[ii] > 255)
		{
			return FALSE;
		}
		if (ii < 3)
		{
			if (*str != _T('.'))
			{
				if(!*str && ii == 0)
				{
					bDefaultIP = TRUE;
					break;
				}
				return FALSE;
			}
			str++;
		}
	}
	
	if(bDefaultIP)
	{
		digits[3] = digits[0];
		digits[0] = 192;
		digits[1] = 168;
		digits[2] = 1;
	}

	PStrPrintf(dest, MAX_NET_ADDRESS_LEN, "%d.%d.%d.%d", digits[0], digits[1], digits[2], digits[3]);
	return TRUE;
}


//----------------------------------------------------------------------------
// returns TRUE if it ended in a comma, FALSE otherwise
//----------------------------------------------------------------------------
static BOOL sCmdLineParseCommaDelimetedString(
	const TCHAR *& str,
	TCHAR * token,
	unsigned int maxlen)
{
	BOOL retval = FALSE;
	BOOL bWithinQuotes = FALSE;
	unsigned int ii = 0;
	token[ii] = 0;

	while (*str && ii < maxlen - 1)
	{
		if ( *str == _T('"') )
		{
			bWithinQuotes = ! bWithinQuotes;
			str++;
		}
		if ( ! bWithinQuotes )
		{
			if (*str == _T(',') || *str == _T('\n'))
			{
				retval = TRUE;
				str++;
				break;
			}

			if (sIsWhitespace(*str))
			{
				break;
			}
		}
		token[ii++] = *str++;
	}
	token[ii] = 0;

	ASSERTV( ! bWithinQuotes, "You forgot to end your quotes for the token: %s!", token );

	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineComment(
	const TCHAR *)
{
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || defined(_PROFILE)
static void sCmdLineClosedClient(
	const TCHAR * str)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-cc not supported in standalone server." ) );
	CLT_VERSION_ONLY(
		gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
		sCmdLineParseIPString(str, gApp.szServerIP, MAX_NET_ADDRESS_LEN, DEFAULT_CLOSED_SERVER_IP) );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineUserName(
	const TCHAR * str)
{
#if !ISVERSION(SERVER_VERSION)
    const char *end = PStrChr(str,' ');
    gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
    if(end)
    {
        PStrCopy(gApp.userName,str,min(sizeof(gApp.userName),(DWORD)(end - str+1)));
    }
    else
    {
        PStrCopy(gApp.userName,str,sizeof(gApp.userName));
    }
#else
    ASSERT(FALSE);
    UNREFERENCED_PARAMETER(str);
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLinePassword(
	const TCHAR * str)
{
#if !ISVERSION(SERVER_VERSION)
	const char *end = PStrChr(str,' ');
    gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
    if(end)
    {
        PStrCopy(gApp.passWord,str,min(sizeof(gApp.passWord),(DWORD)(end - str+1)));
    }
    else
    {
        PStrCopy(gApp.passWord,str,sizeof(gApp.passWord));
    }
#else
    ASSERT(FALSE);
    UNREFERENCED_PARAMETER(str);
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineMyIP(
	const TCHAR * str)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-myip not supported in standalone server." ) );
	CLT_VERSION_ONLY( sCmdLineParseIPString(str, gApp.szLocalIP, MAX_NET_ADDRESS_LEN, DEFAULT_MY_IP) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineColin(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_COLIN_BIT, TRUE);	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineJabberocize(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_JABBERWOCIZE, TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTextLength(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_TEXT_LENGTH, TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTextLabels(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_TEXT_LABELS, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineNoUIClipping(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_UI_NO_CLIPPING, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTextPointSize(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_TEXT_POINT_SIZE, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTextDeveloper(
	const TCHAR * str)
{
	AppSetDebugFlag(ADF_TEXT_DEVELOPER, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineClosedServer(
	const TCHAR *)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-cs not supported in standalone server." ) );
	CLT_VERSION_ONLY( gApp.eStartupAppType = APP_TYPE_CLOSED_SERVER );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineOpenClient(
	const TCHAR * str)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-oc not supported in standalone server." ) );
	CLT_VERSION_ONLY(
		gApp.eStartupAppType = APP_TYPE_OPEN_CLIENT;
		sCmdLineParseIPString(str, gApp.szServerIP, MAX_NET_ADDRESS_LEN, DEFAULT_OPEN_SERVER_IP) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineOpenServer(
	const TCHAR * str)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-os not supported in standalone server." ) );
	CLT_VERSION_ONLY(
		gApp.eStartupAppType = APP_TYPE_OPEN_SERVER;
		char myIp[ MAX_NET_ADDRESS_LEN ];
		NetGetServerIpToUse( myIp, MAX_NET_ADDRESS_LEN, 1 );
		sCmdLineParseIPString(str, gApp.szServerIP, MAX_NET_ADDRESS_LEN, myIp ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineGenPak(
	const TCHAR *)
{
	if (AppIsHammer())
	{
		gtAppCommon.m_bUpdatePakFile = TRUE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSetupFillPak(
	void)
{
	if ( !AppTestDebugFlag( ADF_FILL_PAK_FORCE ) )
	{
#if !ISVERSION(OPTIMIZED_VERSION)
		ASSERTX_RETFALSE( 0, "Fillpak Requires an Optimized Build!  Use -forcepak for development purposes" );
#endif

// AE 2008.02.11: Now that generated assets are being checked in, we don't need to enforce being built on an x64 machine.
//#if !defined(_WIN64)
//		ASSERTX_RETFALSE( 0, "Fillpak Requires an x64 Binary Executable! Use -forcepak for development purposes.");
//#endif
	}

#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETFALSE( !e_DX10IsEnabled(), "Fillpak Requires a DX9 Build!" );
#endif

	gtAppCommon.m_bDisableSound = TRUE;
	gtAppCommon.m_bUpdatePakFile = TRUE;
	gtAppCommon.m_bFillPakFile = TRUE;
	DefinitionForceNoAsync();
	AsyncFileSetSynchronousRead(TRUE);
	c_SoundSetForceBlockingLoad();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineMinPak(
	const TCHAR *str)
{
#if ISVERSION(SERVER_VERSION)
    UNREFERENCED_PARAMETER(str);
#else
	if ( sSetupFillPak() )
	{
		gtAppCommon.m_bGenMinPak = TRUE;
		gtAppCommon.m_GenMinPakStartTick = GetTickCount();
		AppSetDebugFlag(ADF_FILL_PAK_MIN_BIT, TRUE);
		TracePersonal("FILLPAK - sCmdLineMinPak()");
	}
#endif //!SERVER_VERSION
}

#if ISVERSION(DEVELOPMENT)
BOOL sSetupFillPak();
static void sCmdLineRunFillPakClient(
									 const TCHAR * str)
{
#if ISVERSION(SERVER_VERSION)
	BOOL bAssert = FALSE;
	ASSERTX_RETURN(bAssert, "-fpc not supported in standalone server.");
#else
	gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
	sCmdLineParseIPString(str, gApp.szServerIP, MAX_NET_ADDRESS_LEN, DEFAULT_CLOSED_SERVER_IP);

	if (sSetupFillPak()) 
	{
		AppSetDebugFlag(ADF_FILL_PAK_BIT, TRUE);
		AppSetDebugFlag(ADF_FILL_SOUND_PAK_BIT, TRUE);	
		AppSetDebugFlag(ADF_FILL_PAK_CLIENT_BIT, TRUE);
		AppSetDebugFlag(ADF_DELAY_EXCEL_LOAD_POST_PROCESS, TRUE);
		AppSetDebugFlag(ADF_DONT_ADD_TO_PAKFILE, TRUE);
		FillPakLocalizedSetup( NULL );
	}

#endif
}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineSkipMenu(
	const TCHAR *)
{
	SVR_VERSION_ONLY( BOOL assert = FALSE; ASSERTX_RETURN( assert, "-skipmenu not supported in standalone server." ) );
	CLT_VERSION_ONLY( gApp.m_bSkipMenu = TRUE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineLoadDirectlyFromSave(
	const TCHAR* str)
{
#if !ISVERSION(SERVER_VERSION)
	//Call the Skip Menu cmd
	sCmdLineSkipMenu(str);
	char filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, "%s", str);
	char *pFilename = &filename[1];
	filename[strlen(filename)-1] = 0;
	WIN32_FIND_DATA data;
	FindFileHandle handle = FindFirstFile( pFilename, &data );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		char szTemp[MAX_CHARACTER_NAME];
		// grab my all
		PStrRemoveExtension(szTemp, MAX_CHARACTER_NAME, data.cFileName);
		PStrCvt( gApp.m_wszSavedFile, szTemp, MAX_CHARACTER_NAME);

	}
#else
    UNREFERENCED_PARAMETER(str);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineFillPak(
	const TCHAR* str )
{
	if ( sSetupFillPak() )
	{
		AppSetDebugFlag(ADF_FILL_PAK_BIT, TRUE);
		AppSetDebugFlag(ADF_FILL_SOUND_PAK_BIT, TRUE);	
		FillPakLocalizedSetup( NULL );
	}

	// Assign the local system a pak file generation id
	// that will be used when writing out pak files during the
	// fill pak operation.  This number should start at 0
	//
	sCmdLineParseInteger(str, gtAppCommon.m_nDistributedPakFileGenerationIndex, FALSE, 1);

	// This number is used to specify the max number of build servers used in
	// the pak file generation process
	//
	sCmdLineParseInteger(str, gtAppCommon.m_nDistributedPakFileGenerationMax, FALSE, 1);
	min(gtAppCommon.m_nDistributedPakFileGenerationMax, 1);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineForcePak(
	const TCHAR *str)
{
#if ISVERSION( SERVER_VERSION )
	UNREFERENCED_PARAMETER( str );
#else
	AppSetDebugFlag( ADF_FILL_PAK_FORCE, TRUE );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineFillPakMost(
	const TCHAR *)
{
	if( sSetupFillPak() )
	{
		gtAppCommon.m_bGenMinPak = TRUE;
	    gtAppCommon.m_GenMinPakStartTick = GetTickCount();
		AppSetDebugFlag(ADF_FILL_PAK_MIN_BIT, TRUE);
		AppSetDebugFlag(ADF_FILL_PAK_BIT, TRUE);
		AppSetDebugFlag(ADF_FILL_SOUND_PAK_BIT, TRUE);	
		FillPakLocalizedSetup( NULL );		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineFillSoundPak(
	const TCHAR* str )
{
	if ( sSetupFillPak() )
	{
		AppSetDebugFlag(ADF_FILL_SOUND_PAK_BIT, TRUE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineFillPakSKU(
	const TCHAR *pszString)
{
	TCHAR pszSKU[ 128 ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, pszSKU, 128 );
	
	// setup the fillpak for this sku
	if (pszSKU[ 0 ] != 0)
	{
		if ( sSetupFillPak() )
		{
			FillPakLocalizedSetup( pszSKU );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineLanguage(
	const TCHAR *pszString)
{
	TCHAR pszLanguage[ 128 ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, pszLanguage, 128 );
	
	// set current language
	if (pszLanguage[ 0 ] != 0)
	{
		LanguageSetOverride( pszLanguage );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineOverrideSKU(
	const TCHAR *pszString)
{
	TCHAR pszSKU[ 128 ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, pszSKU, 128 );

	// set current language
	if (pszSKU[ 0 ] != 0)
	{
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
		PStrCopy(gApp.m_szSKUOverride, pszSKU, DEFAULT_INDEX_SIZE);
#endif
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineSKUValidate(
	const TCHAR *pszString)
{
	TCHAR szSKU[ 128 ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, szSKU, 128 );

	// set current language
	if (szSKU[ 0 ] != 0)
	{
		SKUSetSKUToValidate( szSKU );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineAdvertDump(
	const TCHAR* str )
{
	if ( sSetupFillPak() )
		AppSetDebugFlag(ADF_FILL_PAK_ADVERT_DUMP_BIT, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineTracePak(
	const TCHAR* str )
{
#if ISVERSION(DEVELOPMENT)
	PakFileRequestTrace();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#include "filepaths.h"
static void sCmdLineUseServerPakFileDir(
	const TCHAR *)
{
	int strLen = PStrLen(FILE_PATH_PAK_FILES);
	int checkLen = PStrLen("ServerPakFiles\\");

	if (strLen > checkLen &&
		PStrICmp(&FILE_PATH_PAK_FILES[strLen - checkLen], "ServerPakFiles\\") == 0)
		return;

	TCHAR newPath[MAX_PATH];
	PStrPrintf(newPath, MAX_PATH, "%s\\%s", FILE_PATH_PAK_FILES, "ServerPakFiles\\");
	PStrCopy(FILE_PATH_PAK_FILES, MAX_PATH, newPath, MAX_PATH);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineRegenExcel(
	const TCHAR *)
{
	gtAppCommon.m_bRegenExcel = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTDefinition(
	const TCHAR *)
{
	testDefinition();
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineNoDirectLoad(
	const TCHAR *)
{
	gtAppCommon.m_bDirectLoad = FALSE;
	gtAppCommon.m_bUpdatePakFile = FALSE;
	gtAppCommon.m_bRegenExcel = FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineDirectLoad(
	const TCHAR *)
{
	gtAppCommon.m_bDirectLoad = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineOneParty(
	const TCHAR *)
{
	gtAppCommon.m_bOneParty = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineOneInstance(
	const TCHAR *)
{
	gtAppCommon.m_bOneInstance = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineSPInstance(
	const TCHAR *)
{
	AppSetDebugFlag(ADF_SP_USE_INSTANCE, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineMultiplayerOnly(
	const TCHAR *)
{
	gtAppCommon.m_bMultiplayerOnly = TRUE;
	gtAppCommon.m_bSingleplayerOnly = FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && ISVERSION(DEVELOPMENT)
static void sCmdLineSingleplayerOnly(
	const TCHAR *)
{
	gtAppCommon.m_bSingleplayerOnly = TRUE;
	gtAppCommon.m_bMultiplayerOnly = FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineMonsterScaling(
	const TCHAR * str)
{
	int monscaling;
	if (!sCmdLineParseInteger(str, monscaling, FALSE, 2))
	{
		return;
	}
	if (monscaling <= 0)
	{
		return;
	}
	gtAppCommon.m_nMonsterScalingOverride = monscaling;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || defined(_PROFILE)
static void sCmdLineAllowPatching(
	const TCHAR* str)
{
	UNREFERENCED_PARAMETER(str);
	CLT_VERSION_ONLY(AppSetAllowPatching() );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineAllowFillLoadXML(
	const TCHAR* str)
{
	UNREFERENCED_PARAMETER(str);
#if !ISVERSION(SERVER_VERSION)
	gtAppCommon.m_GenMinPakStartTick = GetTickCount();
	gtAppCommon.m_bForceLoadAllLanguages = TRUE;
	AppSetAllowFillLoadXML(TRUE);
#endif
}


static void sCmdLineDeleteTempFiles(
	const TCHAR* str)
{
	UNREFERENCED_PARAMETER(str);
#if !ISVERSION(SERVER_VERSION)
	PStrCopy(gApp.m_fnameDelete, str, DEFAULT_FILE_WITH_PATH_SIZE);
	gApp.m_bHasDelete = TRUE;

	CMarkup markup;
	if (markup.Load(str) == FALSE) {
		return;
	}

	TCHAR tmp_fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	while (markup.FindElem("delete")) {
		if (markup.GetData(tmp_fname, DEFAULT_FILE_WITH_PATH_SIZE)) {
			DeleteFile(tmp_fname);
		}
	}
	DeleteFile(str);
#endif
}


//----------------------------------------------------------------------------
// SYNTAX -dbADDRESS,SERVER,USER,PASSWORD,DATABASE
// example -db192.168.1.33,SQLEXPRESS,flagship,flagship,hellgate
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineDatabase(
	const TCHAR * str)
{
	if (!sCmdLineParseCommaDelimetedString(str, gtAppCommon.m_szDatabaseAddress, MAX_PATH))
	{
		trace("COMMAND LINE PARSE ERROR: Invalid Database Specification\n");
		return;
	}
	if (!sCmdLineParseCommaDelimetedString(str, gtAppCommon.m_szDatabaseServer, MAX_PATH))
	{
		trace("COMMAND LINE PARSE ERROR: Invalid Database Specification\n");
		return;
	}
	if (!sCmdLineParseCommaDelimetedString(str, gtAppCommon.m_szDatabaseUser, MAX_PATH))
	{
		trace("COMMAND LINE PARSE ERROR: Invalid Database Specification\n");
		return;
	}
	if (!sCmdLineParseCommaDelimetedString(str, gtAppCommon.m_szDatabasePassword, MAX_PATH))
	{
		trace("COMMAND LINE PARSE ERROR: Invalid Database Specification\n");
		return;
	}
	sCmdLineParseCommaDelimetedString(str, gtAppCommon.m_szDatabaseDb, MAX_PATH);
	gtAppCommon.m_bDatabase = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineDisableSound(
	const TCHAR * str)
{
	gtAppCommon.m_bDisableSound = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineTugboat(
	const TCHAR * str)
{
	ASSERT(AppGameGet() == APP_GAME_TUGBOAT);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineRelaunchMCE(
	const TCHAR* str)
{
	gtAppCommon.m_bRelaunchMCE = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineDemoMode(
	const TCHAR* str)
{
#if ! ISVERSION(CLIENT_ONLY_VERSION)

	TCHAR demo[128] = {0};
	PStrCopy( demo, str, 128 );
	char* end = PStrChr(demo,' ');
	if ( ! end )
		end = PStrChr(demo,'\0');
	if ( ! end )
		return;
	if ( end == demo )
		return;
	*end = NULL;
	char* start = PStrChr(demo,':');
	if ( ! start )
		return;
	start++;

	PStrCopy( gtAppCommon.m_szDemoLevel, start, DEFAULT_INDEX_SIZE );
	CLT_VERSION_ONLY( gApp.m_bSkipMenu = TRUE );

#endif // CLIENT_ONLY_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineWindowed(
	const TCHAR* str)
{
	gtAppCommon.m_bForceWindowed = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdNoMinSpecCheck(
	const TCHAR* str)
{
	gtAppCommon.m_bNoMinSpecCheck = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineNoKeyHooks(
	const TCHAR* str)
{
	gtAppCommon.m_bDisableKeyHooks = TRUE;
#if ! ISVERSION(SERVER_VERSION)	
	void ReleaseKeyboardHook();
	ReleaseKeyboardHook();
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineSoundField(
	const TCHAR* str)
{
#if ISVERSION(DEVELOPMENT)
	AppSetDebugFlag( ADF_START_SOUNDFIELD_BIT, TRUE );
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLinePakDevBuild(
	const TCHAR* str)
{
#if ISVERSION(DEVELOPMENT)
	StringTableSetFillPakDevLanguage( TRUE );
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLinePakDevLanguage(
	const TCHAR* str)
{
#if ISVERSION(DEVELOPMENT)
	StringTableSetFillPakDevLanguage( TRUE );
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineManualCredits(
	const TCHAR* str)
{
#if ISVERSION(DEVELOPMENT)
	gbAutoAdvancePages = FALSE;
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineDevelopmentPakSettings(
	const TCHAR*)
{
	gtAppCommon.m_bDirectLoad = TRUE;
	gtAppCommon.m_bUpdatePakFile = TRUE;
	gtAppCommon.m_bUsePakFile = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineNoCharacterSaving(const TCHAR*)
{
	AppSetFlag(AF_NO_PLAYER_SAVING, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineAllowFileVersionUpdate(const TCHAR*)
{
	AppSetDebugFlag( ADF_ALLOW_FILE_VERSION_UPDATE, TRUE );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineCheatLevels(const TCHAR*)
{
	gtAppCommon.m_dwGlobalFlagsToSet |= GLOBAL_FLAG_CHEAT_LEVELS;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineNoPopups(const TCHAR*)
{
	gtAppCommon.m_dwGlobalFlagsToSet |= GLOBAL_FLAG_NO_POPUPS;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineGenerateAssets(const TCHAR*)
{
	gtAppCommon.m_dwGlobalFlagsToSet |= GLOBAL_FLAG_GENERATE_ASSETS_IN_GAME;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLinePromptForCheckout(const TCHAR*)
{
	gtAppCommon.m_dwGlobalFlagsToSet |= GLOBAL_FLAG_PROMPT_FOR_CHECKOUT;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineConvertAssets(const TCHAR* pszString)
{
	TCHAR pszAsset[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	
	BOOL bContinue = TRUE;
	while ( bContinue )
	{
		bContinue = sCmdLineParseCommaDelimetedString( pszString, 
			pszAsset, DEFAULT_FILE_WITH_PATH_SIZE );
		
		if ( pszAsset[ 0 ] != 0 )
		{
			FillPakAddFilter( pszAsset );
			AppSetDebugFlag( ADF_FILL_PAK_USES_CONVERT_FILTERS, TRUE );
			AppCommonSetMaxExcelVersion();
			AppCommonSetExcelPackageAll();
		}
	};

	// Let's assume -forcepak -pak was sent
	sCmdLineForcePak( "" );
	sCmdLineFillPak( "" );
}

static void sCmdLineConvertAssetsUsingList(const TCHAR* pszString)
{
	TCHAR pszFile[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";

	sCmdLineParseCommaDelimetedString( pszString, pszFile, DEFAULT_FILE_WITH_PATH_SIZE );

	if ( pszFile[ 0 ] != 0 )
	{
		HANDLE hFile = FileOpen( pszFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, 0 );
		ASSERT( hFile != INVALID_FILE );

		if ( hFile != INVALID_FILE )
		{
			UINT64 nSize = FileGetSize( hFile );
			ASSERT( nSize > 0 && nSize <= UINT_MAX );
			if ( nSize > 0 && nSize <= UINT_MAX )
			{				
				char* pszAssets = MALLOCZ_TYPE( char, NULL, (MEMORY_SIZE)(nSize + 1) );
				ASSERT( FileRead( hFile, pszAssets, (DWORD)nSize ) == (DWORD)nSize );				
				sCmdLineConvertAssets( pszAssets );
				FREE( NULL, pszAssets );
			}

			FileClose( hFile );
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineForceUpdate(
	const TCHAR *pszString)
{
#if !ISVERSION(SERVER_VERSION)
	AppSetDebugFlag( ADF_FORCE_ASSET_CONVERSION_UPDATE, TRUE );
#endif	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || defined(_PROFILE)
static void sCmdLineAllowCheats(const TCHAR*)
{
	gtAppCommon.m_dwGameFlagsToSet |= GLOBAL_GAMEFLAG_ALLOW_CHEATS;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineExcelPackage(
	const TCHAR* pszString)
{
	TCHAR pszPackage[ 128 ];
	sCmdLineParseCommaDelimetedString( pszString, pszPackage, 128 );
	gtAppCommon.m_dwExcelManifestPackage |= ExcelGetVersionPackageByString( pszPackage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineExcelVersionMajor(
	const TCHAR* pszString)
{
	TCHAR pszPackage[ 128 ];
	sCmdLineParseCommaDelimetedString( pszString, pszPackage, 128 );
	gtAppCommon.m_wExcelVersionMajor = (WORD) PStrToInt( pszPackage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineExcelVersionMinor(
	const TCHAR* pszString)
{
	TCHAR pszPackage[ 128 ];
	sCmdLineParseCommaDelimetedString( pszString, pszPackage, 128 );
	gtAppCommon.m_wExcelVersionMinor = (WORD) PStrToInt( pszPackage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineLastAct(
	const TCHAR* pszString)
{

	const int MAX_STRING = 128;
	TCHAR uszAct[ MAX_STRING ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, uszAct, MAX_STRING );
	
	// set last act
	if (uszAct[ 0 ] != 0)
	{
		char szAct[ MAX_STRING ];
		PStrCvt( szAct, uszAct, MAX_STRING );
		ActSetLastAct( szAct );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineProcessWhenInactive(const TCHAR* pszString)
{
	AppSetFlag(AF_PROCESS_WHEN_INACTIVE_BIT, true);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static
void sCmdLineDumpInitDB(const TCHAR * /*pszString*/)
{
#if ! ISVERSION(SERVER_VERSION)
	e_InitDB_SetDumpToFile(true);
#endif // SERVER_VERSION
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static
void sCmdLineIgnoreInitDB(const TCHAR * /*pszString*/)
{
#if ! ISVERSION(SERVER_VERSION)
	e_InitDB_SetIgnoreDB(true);
#endif // SERVER_VERSION
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineUseDBUnitLoadMultiplier(const TCHAR * /*pszString*/)
{
	s_SetDBUnitLoadMultiply(TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineUseIncrementalDBSave(const TCHAR * /*pszString*/)
{
	s_SetIncrementalSave(TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineUseIncrementalDBSaveLog(const TCHAR * /*pszString*/)
{
	s_DBUnitLogEnable( NULL, TRUE, TRUE );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineAllowKoreanCharacters(const TCHAR * /*pszString*/)
{
	NameFilterAllowKoreanCharacters(TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineAllowChineseCharacters(const TCHAR * /*pszString*/)
{
	NameFilterAllowChineseCharacters(TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "dbunit.h"
static
void sCmdLineAllowJapaneseCharacters(const TCHAR * /*pszString*/)
{
	NameFilterAllowJapaneseCharacters(TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.07.11
#if ISVERSION(DEVELOPMENT)
static void sCmdLineLoadAllLODs(const TCHAR *)
{
	AppSetDebugFlag(ADF_LOAD_ALL_LODS, true);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineSetDifficulty(const TCHAR* pszString)
{
	AppSetDebugFlag(ADF_DIFFICULTY_OVERRIDE, true);
	if(strcmp(pszString, "Normal") == 0)
		AppSetDebugFlag(ADF_NORMAL, true);
	else if(strcmp(pszString, "Nightmare") == 0)
		AppSetDebugFlag(ADF_NIGHTMARE, true);
	else if(strcmp(pszString, "Hell") == 0)
		AppSetDebugFlag(ADF_HELL, true);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineForceFixedSeeds(const TCHAR *)
{
	AppSetDebugFlag(ADF_FIXED_SEEDS, true);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineDumpSkillIcons(const TCHAR *)
{
	AppSetDebugFlag(ADF_DUMP_SKILL_ICONS, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineDumpSkillDesc(const TCHAR *)
{

	AppSetDebugFlag(ADF_DUMP_SKILL_DESCS, true);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineStatXferLog(const TCHAR *)
{
	gbStatXferLog = TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineAllBadges(const TCHAR *)
{
	gbAllBadges = TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.07.11
#if ISVERSION(DEVELOPMENT)
static void sCmdLineNoMovies(const TCHAR *)
{
	AppSetDebugFlag(ADF_NOMOVIES, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCmdLineEnableUnitMetrics(const TCHAR *)
{
	AppSetFlag(AF_UNITMETRICS, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
static void sCmdLineGlobalTheme(
	const TCHAR * pszString)
{
	const int MAX_STRING = 128;
	TCHAR uszTheme[ MAX_STRING ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, uszTheme, MAX_STRING );

	// force a global theme
	if (uszTheme[ 0 ] != 0)
	{
		char szTheme[ MAX_STRING ];
		PStrCvt( szTheme, uszTheme, MAX_STRING );
		if ( szTheme[ 0 ] == '!' )
		{
			GlobalThemeForce( &szTheme[ 1 ], FALSE );
		} else {
			GlobalThemeForce( szTheme, TRUE );
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
static void sCmdLineExpBonus(
	const char * pszString)
{
	const int MAX_STRING = 128;
	char pszBonus[MAX_STRING] = {0};
	sCmdLineParseCommaDelimetedString( pszString, pszBonus, MAX_STRING );
	gGameSettings.m_nExpBonus = PStrToInt(pszBonus);

}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
static void sCmdLineTestCenterSubscriber(
   const char* pszString )
{
	AppSetFlag(AF_TEST_CENTER_SUBSCRIBER, TRUE);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
static void sCmdLineRespec(
   const char* pszString )
{
	AppSetFlag(AF_RESPEC, TRUE);
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineAuthServerPort(
	const TCHAR *pszString)
{
	const int MAX_STRING = 128;
	TCHAR uszPort[ MAX_STRING ] = { 0 };
	sCmdLineParseCommaDelimetedString( pszString, uszPort, MAX_STRING );
	
	int nPort = -1;
	if (PStrToInt( uszPort, nPort ))
	{
		if (nPort >= 0)
		{
			gnAuthServerPort = nPort;
		}
	}
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineAuthServerLocal(
	const TCHAR *pszString)
{
	AppSetDebugFlag( ADF_AUTHENTICATE_LOCAL, TRUE );
	gnAuthServerPort = 2555;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sCmdLineCheckDataVersion(
	const TCHAR *pszString)
{
	AppSetDebugFlag( ADF_FORCE_DATA_VERSION_CHECK, TRUE );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)
static void sCmdLineDataVersion(
	const TCHAR *pszString)
{

	// skip the first space so the command line can look like "-dataversion 1.0.0.0"
	if (pszString[ 0 ] == L' ')
	{
		pszString++;
		
		const int MAX_STRING = 128;
		TCHAR uszDataVersion[ MAX_STRING ] = { 0 };
		sCmdLineParseCommaDelimetedString( pszString, uszDataVersion, MAX_STRING );
		
		// convert file version string to struct
		FILE_VERSION tDataVersion;
		FileVersionStringToVersion( uszDataVersion, tDataVersion );
		DataVersionSetOverride( tDataVersion );

	}
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CMDLINE(cmd, func)		{ _T(cmd), 0, func }

static APP_COMMAND_LINE gCmdLine[] =
{	// cmd						callback
	CMDLINE(";",				sCmdLineComment),

#if !ISVERSION(CLIENT_ONLY_VERSION) && ISVERSION(DEVELOPMENT)
	CMDLINE("singleplayer",		sCmdLineSingleplayerOnly),
#endif

#if ISVERSION(DEVELOPMENT) || defined(_PROFILE)
	CMDLINE("cc",				sCmdLineClosedClient),
	CMDLINE("allowcheats",		sCmdLineAllowCheats),
	CMDLINE("patching",			sCmdLineAllowPatching),	
#endif

#if ISVERSION(DEVELOPMENT)

	CMDLINE("multiplayer",		sCmdLineMultiplayerOnly),
	CMDLINE("client",			sCmdLineClosedClient),
	CMDLINE("un",		        sCmdLineUserName),
	CMDLINE("pw",		        sCmdLinePassword),
	CMDLINE("server",			sCmdLineClosedServer),
	CMDLINE("cs",				sCmdLineClosedServer),
	CMDLINE("openclient",		sCmdLineOpenClient),
	CMDLINE("oc",				sCmdLineOpenClient),
	CMDLINE("openserver",		sCmdLineOpenServer),
	CMDLINE("os",				sCmdLineOpenServer),
	CMDLINE("regentxt",			sCmdLineRegenExcel),
	CMDLINE("tdefinition",		sCmdLineTDefinition),
	CMDLINE("nodirect",			sCmdLineNoDirectLoad),
	CMDLINE("myip",				sCmdLineMyIP),
	CMDLINE("d",				sCmdLineSkipMenu),
	CMDLINE("oneparty",			sCmdLineOneParty),
	CMDLINE("oneinstance",		sCmdLineOneInstance),	
	CMDLINE("spinstance",		sCmdLineSPInstance),
	CMDLINE("monscaling",		sCmdLineMonsterScaling),
	CMDLINE("tugboat",			sCmdLineTugboat),
	CMDLINE("devpak",			sCmdLineDevelopmentPakSettings),
	CMDLINE("cheatlevels",		sCmdLineCheatLevels),
	CMDLINE("colin",			sCmdLineColin),
	CMDLINE("jabber",			sCmdLineJabberocize),
	CMDLINE("textlength",		sCmdLineTextLength),
	CMDLINE("textlabels",		sCmdLineTextLabels),	
	CMDLINE("nouiclipping",		sCmdLineNoUIClipping),		
	CMDLINE("textpointsize",	sCmdLineTextPointSize),	
	CMDLINE("textdeveloper",	sCmdLineTextDeveloper),	
	CMDLINE("english",			sCmdLineTextDeveloper),	
	CMDLINE("db",				sCmdLineDatabase),
	CMDLINE("nopopups",		    sCmdLineNoPopups),
	CMDLINE("forcepak",			sCmdLineForcePak),
	CMDLINE("tracepak",			sCmdLineTracePak),
	CMDLINE("useserverpak",		sCmdLineUseServerPakFileDir),
	CMDLINE("genassets",		sCmdLineGenerateAssets),
	CMDLINE("checkout",			sCmdLinePromptForCheckout),
	CMDLINE("fpc",				sCmdLineRunFillPakClient),
	CMDLINE("inactive",			sCmdLineProcessWhenInactive),
	CMDLINE("fileversionupdate",sCmdLineAllowFileVersionUpdate),
	CMDLINE("dumpinitdb",		sCmdLineDumpInitDB),				// CHB 2007.05.22
	CMDLINE("ignoreinitdb",		sCmdLineIgnoreInitDB),				// CHB 2007.05.23
	CMDLINE("loadalllods",		sCmdLineLoadAllLODs),				// CHB 2007.07.11
	CMDLINE("difficulty",		sCmdLineSetDifficulty),
	CMDLINE("nomovies",			sCmdLineNoMovies),
	CMDLINE("skipmenu",			sCmdLineSkipMenu),
	CMDLINE("language_",		sCmdLineLanguage),
	CMDLINE("sku_",				sCmdLineOverrideSKU),
	CMDLINE("skuvalidate_",		sCmdLineSKUValidate),
	CMDLINE("soundfield",		sCmdLineSoundField),		
	CMDLINE("pakdevbuild",		sCmdLinePakDevBuild),			
	CMDLINE("pakdevlanguage",	sCmdLinePakDevLanguage),		
	CMDLINE("manualcredits",	sCmdLineManualCredits),			
	CMDLINE("fixedseeds",		sCmdLineForceFixedSeeds),
	CMDLINE("statxferlog",		sCmdLineStatXferLog),				
	CMDLINE("allbadges",		sCmdLineAllBadges),				
	CMDLINE("authserverport",	sCmdLineAuthServerPort),
	CMDLINE("authserverlocal",	sCmdLineAuthServerLocal),
	CMDLINE("checkdataversion",	sCmdLineCheckDataVersion),
	CMDLINE("dataversion",		sCmdLineDataVersion),
	CMDLINE("convert_",			sCmdLineConvertAssets),
	CMDLINE("convertlist_",		sCmdLineConvertAssetsUsingList),
	CMDLINE("forceupdate",		sCmdLineForceUpdate),
#endif	


#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
	CMDLINE("globaltheme",		sCmdLineGlobalTheme),
	CMDLINE("usetestcentersubscriber",	sCmdLineTestCenterSubscriber),
#endif

#if ISVERSION(SERVER_VERSION)
	CMDLINE("incrementalsave",			sCmdLineUseIncrementalDBSave),
	CMDLINE("incrementalsavelog",		sCmdLineUseIncrementalDBSaveLog),
	CMDLINE("dbloadmultiply",			sCmdLineUseDBUnitLoadMultiplier),
	CMDLINE("allowkoreanchar",			sCmdLineAllowKoreanCharacters),
	CMDLINE("allowchinesechar",			sCmdLineAllowChineseCharacters),
	CMDLINE("allowjapanesechar",		sCmdLineAllowJapaneseCharacters),
	CMDLINE("expbonus",					sCmdLineExpBonus),
	CMDLINE("respec",					sCmdLineRespec),
#endif

	CMDLINE("nosave",			sCmdLineNoCharacterSaving),
	CMDLINE("package",			sCmdLineExcelPackage),
	CMDLINE("version_major",	sCmdLineExcelVersionMajor),
	CMDLINE("version_minor",	sCmdLineExcelVersionMinor),
	CMDLINE("lastact_",			sCmdLineLastAct),
	CMDLINE("load",				sCmdLineLoadDirectlyFromSave),
	CMDLINE("disablesound",		sCmdLineDisableSound),
	CMDLINE("genpak",			sCmdLineGenPak),
	CMDLINE("minpak",			sCmdLineMinPak),
	CMDLINE("pak",				sCmdLineFillPak),
	CMDLINE("mostpak",			sCmdLineFillPakMost),
	CMDLINE("paksound",			sCmdLineFillSoundPak),	
	CMDLINE("paksku_",			sCmdLineFillPakSKU),
	CMDLINE("advert",			sCmdLineAdvertDump),
	CMDLINE("direct",			sCmdLineDirectLoad),
	CMDLINE("fillloadxml",		sCmdLineAllowFillLoadXML),
	CMDLINE("relaunchmce",		sCmdLineRelaunchMCE),
	CMDLINE("windowed",			sCmdLineWindowed),
	CMDLINE("nokeyhooks",		sCmdLineNoKeyHooks),		
	CMDLINE("unitmetrics",		sCmdLineEnableUnitMetrics),
	CMDLINE("delete",			sCmdLineDeleteTempFiles),
	CMDLINE("nominreq",			sCmdNoMinSpecCheck),
	CMDLINE("rundemo",			sCmdLineDemoMode),
	CMDLINE("dumpskillicons",	sCmdLineDumpSkillIcons),
	CMDLINE("dumpskilldescs",	sCmdLineDumpSkillDesc),
};

static const unsigned int nCmdLine = arrsize(gCmdLine);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int CommandLineFindCommand(
	const TCHAR *& curr)
{
	const APP_COMMAND_LINE * cmd = gCmdLine;
	for (unsigned int ii = 0; ii < nCmdLine; ++ii, ++cmd)
	{
		if (!cmd->cmd)
		{
			break;
		}
		if (PStrICmp(cmd->cmd, curr, cmd->strlen) == 0)
		{
			curr += cmd->strlen;
			return ii;
		}
	}
	return nCmdLine;
}


//----------------------------------------------------------------------------
// parse and execute a single command
//----------------------------------------------------------------------------
static BOOL CommandLineParseCommand(
	const TCHAR *& curr)
{
	if (!sSkipWhitespace(curr))
	{
		return FALSE;
	}
	// skip leading '-'
	if (*curr == _T('-'))
	{
		*curr++;
	}
	if (*curr == NULL)
	{
		return FALSE;
	}
	unsigned int cmd = CommandLineFindCommand(curr);
	if (cmd < nCmdLine)
	{
		gCmdLine[cmd].callback(curr);
	}
	
	// skip to next whitespace
	while (*curr && *curr != _T(' '))
	{
		curr++;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// command line compare function for qsort
// longer strlen is considered less
//----------------------------------------------------------------------------
static int CommandLineSortCompare(
	const void * a,
	const void * b)
{
	APP_COMMAND_LINE * A = (APP_COMMAND_LINE *)a;
	APP_COMMAND_LINE * B = (APP_COMMAND_LINE *)b;
	if (A->strlen == 0)
	{
		return (B->strlen == 0 ? 0 : 1);
	}
	if (B->strlen == 0)
	{
		return -1;
	}
	if (A->strlen > B->strlen)
	{
		return -1;
	}
	if (A->strlen < B->strlen)
	{
		return 1;
	}
	return PStrCmp(A->cmd, B->cmd, A->strlen);
}


//----------------------------------------------------------------------------
// sort command line commands in gCmdLine, conver to lowercase, and compute 
// lengths
//----------------------------------------------------------------------------
static void CommandLineInit(
	void)
{
	for (int ii = 0; ii < nCmdLine; ++ii)
	{
		gCmdLine[ii].strlen = (gCmdLine[ii].cmd ? PStrLen(gCmdLine[ii].cmd) : 0);
		PStrLower(gCmdLine[ii].cmd, gCmdLine[ii].strlen + 1);
	}
	qsort(gCmdLine, nCmdLine, sizeof(APP_COMMAND_LINE), CommandLineSortCompare);
}


//----------------------------------------------------------------------------
// parse command line settings
//----------------------------------------------------------------------------
void AppParseCommandLine(
	const char * cmdline)
{
	ASSERT_RETURN(cmdline);

	CommandLineInit();

	const char * curr = cmdline;

	while (*curr)
	{
		if (!CommandLineParseCommand(curr))
		{
			break;
		}
	}
}
