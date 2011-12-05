//----------------------------------------------------------------------------
// debug.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "definition_common.h"
#include "CrashHandler.h"
#include "winos.h"
#include "p_windows.h"
#include "c_input.h"

#include "config.h"
#if ISVERSION(SERVER_VERSION)
#include "debug.cpp.tmh"
#endif // ISVERSION(SERVER_VERSION)

#include "globalindex.h"	// CHB 2007.08.01


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
volatile long g_AssertCounter = 0;

static PFN_DEBUG_STRING_CHAR    pfnDebugString    = NULL;
static PFN_DEBUG_STRING_WCHAR   pfnDebugStringW   = NULL;
static PFN_CONSOLE_STRING_CHAR  pfnConsoleString  = NULL;
static PFN_CONSOLE_STRING_WCHAR pfnConsoleStringW = NULL;
static int  sgnDebugValue = 0;
static int  sgnDebugKeysRefCount = 0;
int KEYTRACE_CLASS::snGlobalKey = VK_CAPITAL;

static const int MAX_IGNORE_ASSERT_FILES = 30;
static const int MAX_IGNORE_ASSERT_LINES = 10;
static int sgnIgnoreFiles = 0;
static char sgszIgnoreFiles[MAX_IGNORE_ASSERT_FILES][MAX_PATH] = {0};
static int  sgpnIgnoreLines[MAX_IGNORE_ASSERT_FILES][MAX_IGNORE_ASSERT_LINES] = {0};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_TRACES
void trace1(
	const char * str)
{
	PDebugTrace(str);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_TRACES
void vtrace(
	const char * format,
	va_list & args)
{
	char str[4096];
	PStrVprintf(str, 4096, format, args);
	PDebugTrace(str);
}
#endif


//----------------------------------------------------------------------------
// printf to output window of .net
//----------------------------------------------------------------------------
#if DEBUG_USE_TRACES
void trace(
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	vtrace(format, args);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sErrorDialog(
	const char * str,
	UINT uType = DEFAULT_ERRORDLG_MB_TYPE)
{
#if !ISVERSION(SERVER_VERSION)
	WCHAR wbuf[4096];
	PStrCvt(wbuf, str, 4096);

	TraceError("ERROR: %s", str);
	DebugString(OP_ERROR, L"ERROR: %s", wbuf);

    if( PStrStr(GetCommandLine(),"-nopopups") != NULL
	 && ! AppTestDebugFlag( ADF_FORCE_ASSERTS_ON )) 
    {
        return IDIGNORE;
    }

	return MessageBox(AppCommonGetHWnd(), str, "Error", uType);	// CHB 2007.08.01 - String audit: USED IN RELEASE, but appears to be diagnostics intented for developer, and is controlled by a run-time mechanism
#else
	UNREFERENCED_PARAMETER(uType);
	TraceError("\nERROR ** %s\n", str);
	return IDRETRY;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ErrorDialogCustom(
	UINT uType,
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	return ErrorDialogV(format, args, uType);	// CHB 2007.08.01 - String audit: USED IN RELEASE
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
int ErrorDialogEx(
	const char * format,
	...)
{
	if (AppCommonGetSilentAssert())
	{
		return 0;
	}

	va_list args;
	va_start(args, format);

	char buf[4096];
	PStrVprintf(buf, 4096, format, args);

	int nResult = sErrorDialog(buf);	// CHB 2007.08.01 - String audit: development
	switch (nResult)
	{
	case IDABORT:
		_exit(0);
		break;
	case IDRETRY:
		return 1;
	case IDIGNORE:
	default:
		break;
	}
	return 0;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ErrorDialogV(	// CHB 2007.08.01 - String audit: USED IN RELEASE
	const char * format,
	va_list & args,
	UINT uType)
{
	if (AppCommonGetSilentAssert())
	{
		return 0;
	}

	char buf[4096];
	PStrVprintf(buf, 4096, format, args);

	int nResult = sErrorDialog(buf, uType);	// CHB 2007.08.01 - String audit: USED IN RELEASE; see ErrorDialogCustom()
	switch (nResult)
	{
	case IDABORT:
		_exit(0);
		break;
	case IDRETRY:
		DEBUG_BREAK();
		break;
	case IDCANCEL:
		return -1;
	case IDNO:
		return -2;
	case IDOK:
	case IDIGNORE:
	case IDYES:
	default:
		break;
	}

	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddToAlwaysIgnoreList( 
	const char *szFile, 
	int nLine )
{
	// add the assert to the list of asserts we'll automatically ignore from now on.
	int nFileIdx = -1;
	for (int iFile = 0; iFile < MAX_IGNORE_ASSERT_FILES && iFile < sgnIgnoreFiles; iFile++)
	{
		if (PStrICmp(sgszIgnoreFiles[iFile], szFile) == 0)
		{
			nFileIdx = iFile;
			break;
		}
	}

	if (nFileIdx == -1 && sgnIgnoreFiles < MAX_IGNORE_ASSERT_FILES)
	{
		nFileIdx = sgnIgnoreFiles++;
		PStrCopy(sgszIgnoreFiles[nFileIdx], szFile, MAX_PATH);
	}

	if (nFileIdx > -1)
	{
		for (int iLine = 0; iLine < MAX_IGNORE_ASSERT_LINES; iLine++ )
		{
			if (sgpnIgnoreLines[nFileIdx][iLine] == 0 ||
				sgpnIgnoreLines[nFileIdx][iLine] == nLine)
			{
				sgpnIgnoreLines[nFileIdx][iLine] = nLine;
				TraceError( "Ignoring all further asserts from FILE:%s,  LINE:%d", szFile, nLine);
				LogFlush();

				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsInAlwaysIgnoreList( 
	const char *szFile, 
	int nLine )
{
	if (!szFile || !szFile[0])
		return FALSE;

	// add the assert to the list of asserts we'll automatically ignore from now on.
	int nFileIdx = -1;
	for (int iFile = 0; iFile < MAX_IGNORE_ASSERT_FILES && iFile < sgnIgnoreFiles; iFile++)
	{
		if (PStrICmp(sgszIgnoreFiles[iFile], szFile) == 0)
		{
			nFileIdx = iFile;
			break;
		}
	}

	if (nFileIdx > -1)
	{
		for (int iLine = 0; iLine < MAX_IGNORE_ASSERT_LINES; iLine++ )
		{
			if (sgpnIgnoreLines[nFileIdx][iLine] == nLine)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
int DoAssertV(
	const char * szExpression,
	const char * szFile,
	int nLine,
	const char * szFunction,
	const char * szMessageFmt,
	va_list & args )
{
	if (sIsInAlwaysIgnoreList(szFile, nLine))
	{
		return 0;
	}

	char buf[4096];
	PStrVprintf(buf, 4096, szMessageFmt, args);

#if ISVERSION(SERVER_VERSION)
	TraceError(
		"*** ASSERTION *** cond: (%s), func: %s, expl: %s, file: %s, line: %d",
		szExpression, szFunction, buf, szFile, nLine );
#else
	TraceError( "\n*** ASSERTION ***");
	TraceError( "expression: %s\n", szExpression);
	if (buf[0])
	{
		TraceError( "explanation: %s", buf);
	}
	TraceError( "FILE:%s,  LINE:%d - FUNCTION: %s", szFile, nLine, szFunction);
#endif
	LogFlush();

#if !ISVERSION(SERVER_VERSION)
	if (AppCommonGetSilentAssert() && ! AppTestDebugFlag( ADF_FORCE_ASSERTS_ON ) )
#else if ISVERSION(DEVELOPMENT)
	BOOL DesktopServerIsEnabled(void);
	if(AppCommonGetSilentAssert() && !DesktopServerIsEnabled())
#endif
	{
		return 0;
	}

	if(	PStrStr( GetCommandLine() ,"-nopopups") != NULL
	 && ! AppTestDebugFlag( ADF_FORCE_ASSERTS_ON ) ) 
	{
		return 0;
	}

	char szErrorMessage[4096];

	if (szExpression != NULL)
	{
		PStrPrintf(szErrorMessage, 4096, "ASSERT: %s - %s\n[%s ln: %d - %s]", buf, szExpression, szFile, nLine, szFunction);
	}
	else
	{
		PStrPrintf(szErrorMessage, 4096, "ASSERT: %s\n[%s ln: %d - %s]", buf, szFile, nLine, szFunction);
	}

	int nResult = IDRETRY;
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	long counter = InterlockedIncrement(&g_AssertCounter);
	if (counter > 1)
	{
		InterlockedDecrement(&g_AssertCounter);
		return 0;
	}
	nResult = GenerateAssertReport(szErrorMessage, arrsize(szErrorMessage));
	InterlockedDecrement(&g_AssertCounter);


	if (nResult == IDIGNOREALWAYS && szFile)
	{
		sAddToAlwaysIgnoreList( szFile, nLine );
	}
#elif ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	int DoDesktopServerAssert(char *);
	nResult = DoDesktopServerAssert(szErrorMessage);
#endif

	switch (nResult) 
	{
	case IDABORT:
		_exit(0);
	case IDRETRY:
		return 1;
	case IDIGNORE:
	default:
		return 0;
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
int DoAssertEx(
	const char * szExpression,
	const char * szFile,
	int nLine,
	const char * szFunction,
	const char * szMessageFmt,
	... )
{
	va_list args;
	va_start(args, szMessageFmt);

	return DoAssertV( szExpression, szFile, nLine, szFunction, szMessageFmt, args );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
int DoAssert(
	const char * szExpression,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction)
{
	return DoAssertEx( szExpression, szFile, nLine, szFunction, "%s", szMessage );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
int DoNameAssert(
	const char * expression,
	const char * username,
	const char * filename,
	int linenumber,
	const char * function)
{
	char szUsername[512];
	DWORD dwSize = 512;
	if (GetUserName(szUsername, &dwSize))
	{
		if (PStrICmp(szUsername, username) == 0)
		{
			return DoAssert(expression, "", filename, linenumber, function);
		}
	}

	return 0;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if DEBUG_USE_ASSERTS
void DoUnignorableAssert(
	const char * szExpression,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction )
{
	TraceError( "\n*** ASSERTION ***");
	TraceError( "expression: %s\n", szExpression);
	if (szMessage[0])
	{
		TraceError( "explanation: %s", szMessage);
	}
	TraceError( "FILE:%s,  LINE:%d - FUNCTION: %s", szFile, nLine, szFunction);
	LogFlush();

#if ISVERSION(DEBUG_VERSION)
	DebugBreak();
#else
	Sleep(INFINITE);
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sDoHalt(
	const char * szExpression,
	const WCHAR * pText,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction)
{
	char szErrorMessageA[4096];
	WCHAR szErrorMessageW[4096];
	WCHAR szTextW[4096];

	if (pText == 0)
	{
		PStrPrintf(szTextW, _countof(szTextW), L"%S", szMessage);
		pText = szTextW;
	}

	if (szExpression != NULL)
	{
		PStrPrintf(szErrorMessageA, _countof(szErrorMessageA), "%s\n\nExpression: %s\n\nFILE: %s,  LINE: %d - FUNCTION: %s", szMessage, szExpression, szFile, nLine, szFunction);
		PStrPrintf(szErrorMessageW, _countof(szErrorMessageW), L"%s\n\nExpression: %S\n\nFILE: %S,  LINE: %d - FUNCTION: %S", pText, szExpression, szFile, nLine, szFunction);
	}
	else
	{
		PStrPrintf(szErrorMessageA, _countof(szErrorMessageA), "%s\n\nFILE: %s,  LINE: %d - FUNCTION: %s", szMessage, szFile, nLine, szFunction);
		PStrPrintf(szErrorMessageW, _countof(szErrorMessageW), L"%s\n\nFILE: %S,  LINE: %d - FUNCTION: %S", pText, szFile, nLine, szFunction);
	}

	DebugString(OP_ERROR, L"HALT: %S", szErrorMessageA);

	TraceError("*** ERROR ***");
	TraceError("expression: %s", szExpression);
	if (szMessage[0])
	{
		TraceError("explanation: %s", szMessage);
	}
	TraceError("FILE:%s,  LINE:%d - FUNCTION: %s", szFile, nLine, szFunction);
	LogFlush();

	ErrorDialog( szErrorMessageA );

#if !ISVERSION(SERVER_VERSION)
	long counter = InterlockedIncrement(&g_AssertCounter);
	if (counter > 1)
	{
		InterlockedDecrement(&g_AssertCounter);
		_exit(0);
	}
	const WCHAR * pCaption = AppCommonGetGlobalString(GS_MSG_UNRECOVERABLE_ERROR, L"Unrecoverable Error");
	::MessageBoxW(AppCommonGetHWnd(), szErrorMessageW, pCaption, MB_OK);	// CHB 2007.08.01 - String audit: USED IN RELEASE
	InterlockedDecrement(&g_AssertCounter);
#endif
	_exit(0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DoHalt(
	const char * szExpression,
	unsigned nStringId,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction)
{
	sDoHalt(szExpression, AppCommonGetGlobalString(nStringId, 0), szMessage, szFile, nLine, szFunction);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DoHalt(
	const char * szExpression,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction)
{
	sDoHalt(szExpression, 0, szMessage, szFile, nLine, szFunction);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DoWarn(
	const char * szExpression,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction)
{
#if !ISVERSION(SERVER_VERSION)
	char szErrorMessage[4096];

	if (szExpression != NULL)
	{
		PStrPrintf(szErrorMessage, 4096, "%s\n\nExpression: %s\n\nFILE: %s,  LINE: %d - FUNCTION: %s", szMessage, szExpression, szFile, nLine, szFunction);
	}
	else
	{
		PStrPrintf(szErrorMessage, 4096, "%s\n\nFILE: %s,  LINE: %d - FUNCTION: %s", szMessage, szFile, nLine, szFunction);
	}

	DebugString(OP_ERROR, L"WARN: %S", szErrorMessage);

	("*** WARNING ***");
	LogMessage("expression: %s", szExpression);
	if (szMessage[0])
	{
		LogMessage("explanation: %s", szMessage);
	}
	LogMessage("FILE:%s,  LINE:%d - FUNCTION: %s", szFile, nLine, szFunction);
	LogMessage(szErrorMessage);
	LogFlush();
	
	if (AppCommonGetSilentAssert())
	{
		return;
	}

	long counter = InterlockedIncrement(&g_AssertCounter);
	if (counter > 1)
	{
		InterlockedDecrement(&g_AssertCounter);
		return;
	}
    if( PStrStr(GetCommandLine(),"-nopopups") != NULL
	 && ! AppTestDebugFlag( ADF_FORCE_ASSERTS_ON ) ) 
    {
        return;
    }
#if ISVERSION(DEVELOPMENT)
	int nResult = MessageBox(AppCommonGetHWnd(), szErrorMessage, "Warning", DEFAULT_ERRORDLG_MB_TYPE);	// CHB 2007.08.01 - String audit: USED IN RELEASE, but appears to be diagnostics intented for developer, and is controlled by a run-time mechanism
#else
	int nResult = MessageBox(AppCommonGetHWnd(), szErrorMessage, "Warning", MB_OK);	// CHB 2007.08.01 - String audit: USED IN RELEASE, but appears to be diagnostics intented for developer, and is controlled by a run-time mechanism
#endif // ISVERSION(DEVELOPMENT)
	InterlockedDecrement(&g_AssertCounter);

	switch (nResult) 
	{
	case IDABORT:
		_exit(0);
	case IDRETRY:
		DEBUG_BREAK();
	case IDIGNORE:
	case IDOK:
	default:
		break;
	}

#else
	TraceError("*** WARNING *** %s\n\nExpression %s at FILE %s, LINE %d, FUNCTION %s\n",
				szMessage,
				szExpression,
				szFile,
				nLine,
				szFunction);
				
#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ShowDataWarningV(
	const char * szFormat, 
	va_list & args, 
	DWORD dwWarningType)
{
#if ! ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	if (ShowDataWarnings(dwWarningType))
	{
		ErrorDialogV(szFormat, args);	// CHB 2007.08.01 - String audit: USED IN RELEASE, but appears to be diagnostics intented for developer, and is controlled by a run-time mechanism
	} 
	else
	{
		DebugStringV(OP_HIGH, szFormat, args);
	}
	LogMessageV(DATAWARNING_LOG, szFormat, args);
#else
	REF( szFormat );
	REF( args );
	REF( dwWarningType );
#endif // !SERVER && DEVELOPMENT
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ShowDataWarnings(
	DWORD dwWarningType)
{
	const GLOBAL_DEFINITION * global = DefinitionGetGlobal();
	if (!global)
	{
		return TRUE;
	}
	if (dwWarningType & WARNING_TYPE_BACKGROUND)
	{
		return (global->dwFlags & GLOBAL_FLAG_BACKGROUND_WARNINGS) != 0;
	}
	if (dwWarningType & WARNING_TYPE_STRINGS)
	{
		return (global->dwFlags & GLOBAL_FLAG_STRING_WARNINGS) != 0;
	}
	return (global->dwFlags & GLOBAL_FLAG_DATA_WARNINGS) != 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ShowDataWarning(
	DWORD dwWarningType,
	const char * szFormat, 
	...)
{
	va_list args;
	va_start(args, szFormat);
	ShowDataWarningV(szFormat, args, dwWarningType);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ShowDataWarning(
	const char * szFormat,
	...)
{
	va_list args;
	va_start(args, szFormat);
	ShowDataWarningV(szFormat, args, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DataFileCheck(
	const TCHAR* filename,
	BOOL bNoDialog /*= FALSE*/ )
{
	BOOL bResult = FALSE;

	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if (   c_GetToolMode() 
		|| AppCommonIsFillpakInConvertMode()
		|| ( pGlobals && TEST_MASK( pGlobals->dwFlags, GLOBAL_FLAG_PROMPT_FOR_CHECKOUT ) ) )
	{
		if (	( AppCommonIsAnyFillpak() && ! AppCommonIsFillpakInConvertMode() )
			 || bNoDialog
			 || AppTestDebugFlag( ADF_SILENT_CHECKOUT )
			 || ErrorDialogCustom( PERFORCE_ERRORDLG_MB_TYPE, 
					"File needs update, but it's read-only! "
					"Would you like to attempt to check it out from Perforce?\n\n%s", 
					filename ) == 0 )
		{
			FileCheckOut( filename );
			bResult = ! FileIsReadOnly( filename );
			ASSERTV( bResult, "Perforce checkout failed for:\n\n%s", filename );
		}
	}
	else
	{
		ShowDataWarning( 
			"File needs update, but it's read-only!  It probably needs to be checked out of Perforce.\n\n%s", 
			filename );
	}

	return bResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetDebugStringCallbackChar(
	PFN_DEBUG_STRING_CHAR pfnCallback)
{
	pfnDebugString = pfnCallback;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetDebugStringCallbackWideChar(
	PFN_DEBUG_STRING_WCHAR pfnCallback)
{
	pfnDebugStringW = pfnCallback;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetConsoleStringCallbackChar(
	PFN_CONSOLE_STRING_CHAR pfnCallback)
{
	pfnConsoleString = pfnCallback;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetConsoleStringCallbackWideChar(
	PFN_CONSOLE_STRING_WCHAR pfnCallback)
{
	pfnConsoleStringW = pfnCallback;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugString(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

	DebugStringV(ePriority, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugStringV(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	va_list & args)
{
	if (pfnDebugStringW)
	{
		pfnDebugStringW(ePriority, format, args);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugString(
	OUTPUT_PRIORITY ePriority,
	const char *format,
	...)
{
	va_list args;
	va_start(args, format);

	DebugStringV(ePriority, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugStringV(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	va_list & args)
{
	if (pfnDebugString)
	{
		pfnDebugString(ePriority, format, args);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsolePrintString(
	DWORD dwColor,
	const char *str)
{
	if (pfnConsoleString)
	{
		pfnConsoleString(dwColor, str);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsolePrintString(
	DWORD dwColor,
	const WCHAR *str)
{
	if (pfnConsoleStringW)
	{
		pfnConsoleStringW(dwColor, str);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * GetErrorString(
	DWORD error,
	char * str,
	int len)
{
	ASSERT_RETNULL(len > 8);

	*str = 0;
	int retval = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)str, len, NULL);
	if (!retval)
	{
		PIntToStr(str, len, error);
		PStrCat(str, "\n", len);
	}
	return str;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugKeyTrace(
	const char * format,
	...)
{
	if (IsKeyDown(KEYTRACE_CLASS::GetKey()))
	{
		va_list args;
		va_start(args, format);

		vtrace(format, args);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
HANDLE sghDebugThreadEnd = INVALID_HANDLE_VALUE;
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
struct DEBUG_TIME_LIMITER
{
	CCritSectLite						m_CS;
	DWORD								m_dwSetTime;
	DWORD								m_dwThreshold;
	BOOL								m_bDebugThreshold;
	HANDLE								m_hTimer;

	DEBUG_TIME_LIMITER(
		void)
	{
		m_CS.Init();
		m_dwSetTime = 0;
		m_dwThreshold = 0;
		m_bDebugThreshold = FALSE;
		m_hTimer = INVALID_HANDLE_VALUE;
	}

	~DEBUG_TIME_LIMITER(
		void)
	{
		if (m_hTimer != INVALID_HANDLE_VALUE)
		{
			CancelWaitableTimer(m_hTimer);
			CloseHandle(m_hTimer);
			m_hTimer = INVALID_HANDLE_VALUE;
		}
		m_CS.Free();
	}
};

DEBUG_TIME_LIMITER						g_DebugTimeLimiter;
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugSetTimeLimiter(
	void)
{
	if (!g_DebugTimeLimiter.m_bDebugThreshold || g_DebugTimeLimiter.m_hTimer == INVALID_HANDLE_VALUE)
	{
		return;
	}
	g_DebugTimeLimiter.m_dwSetTime = GetTickCount();

	LARGE_INTEGER time;
	time.QuadPart = -(int)g_DebugTimeLimiter.m_dwThreshold * 10000;	// 100 nanosecond intervals, negative number for relative time
	SetWaitableTimer(g_DebugTimeLimiter.m_hTimer, &time, 0, NULL, NULL, FALSE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugClearTimeLimiter(
	void)
{
	if (g_DebugTimeLimiter.m_dwSetTime != 0)
	{
		CancelWaitableTimer(g_DebugTimeLimiter.m_hTimer);
		g_DebugTimeLimiter.m_dwSetTime = 0;
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
DWORD WINAPI DebugThresholdThread(
	LPVOID)
{
	for (;;)
	{
		if (WaitForSingleObject(g_DebugTimeLimiter.m_hTimer, INFINITE) != WAIT_OBJECT_0)
		{
			break;
		}
		if (!g_DebugTimeLimiter.m_bDebugThreshold)
		{
			break;
		}
		DWORD curtick = GetTickCount();
		DWORD elapsed = curtick - g_DebugTimeLimiter.m_dwSetTime;
		trace("Debug Threshold Hit: Elapsed Time = %d msecs\n", elapsed);
		DebugBreak();
	}
	return 0;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
DWORD WINAPI DebugThread(
	LPVOID)
{
	for (;;)
	{
		static volatile long DEBUG_BREAKER_INTERVAL	= 50;
		static long DEBUG_BREAKER_MAX_THRESHOLD = 5 * MSECS_PER_SEC;

		if (WaitForSingleObject(sghDebugThreadEnd, DEBUG_BREAKER_INTERVAL) == WAIT_OBJECT_0)
		{
			break;
		}

		if (IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT) && IsKeyDown(VK_UP))		// ctrl-shift-up arrow
		{
			if(IsDebuggerAttached())
			{
				DebugBreak();
			}
		}
	}

	return 0;
}
#endif

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugInit(
	LPSTR lpCmdLine)
{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	if (PStrStrI(lpCmdLine, "-framelimiter") != NULL)
	{
		g_DebugTimeLimiter.m_bDebugThreshold = TRUE;
		g_DebugTimeLimiter.m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

		LPSTR sz = PStrStrI(lpCmdLine, "-framelimiter");
		if (sz)
		{
			sz += PStrLen("-framelimiter");
			g_DebugTimeLimiter.m_dwThreshold = PStrToInt(sz);
		}
		if (g_DebugTimeLimiter.m_dwThreshold <= 10)
		{
			g_DebugTimeLimiter.m_dwThreshold = 200;
		}

		DWORD dwThreadId;
		HANDLE thread = CreateThread(NULL, 0, DebugThresholdThread, NULL, 0, &dwThreadId);
		CloseHandle(thread);
	}

	{
		sghDebugThreadEnd = CreateEvent(NULL, FALSE, FALSE, NULL);

		DWORD dwThreadId;
		HANDLE thread = CreateThread(NULL, 0, DebugThread, NULL, 0, &dwThreadId);
		CloseHandle(thread);
	}
#else
	REF(lpCmdLine);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugFree(
	void)
{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	if (g_DebugTimeLimiter.m_bDebugThreshold && g_DebugTimeLimiter.m_hTimer != INVALID_HANDLE_VALUE)
	{
		// kill the waitable timer
		g_DebugTimeLimiter.m_bDebugThreshold = FALSE;

		LARGE_INTEGER time;
		time.QuadPart = -1;
		SetWaitableTimer(g_DebugTimeLimiter.m_hTimer, &time, 0, NULL, NULL, FALSE);
	}

	SetEvent(sghDebugThreadEnd);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugKeyHandlerStart(
	int nInitValue)
{
	sgnDebugKeysRefCount++;
	sgnDebugValue = nInitValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugKeyHandlerEnd(
	void)
{
	sgnDebugKeysRefCount--;
	if (sgnDebugKeysRefCount < 0)
	{
		sgnDebugKeysRefCount = 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DebugKeysAreActive(
	int)
{
	return (sgnDebugKeysRefCount > 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugKeyUp(
	void)
{
	sgnDebugValue++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugKeyDown(
	void)
{
	sgnDebugValue--;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DebugKeyGetValue(
	void)
{
	return sgnDebugValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DebugKeySetValue(
	int nValue)
{
	sgnDebugValue = nValue;
}

