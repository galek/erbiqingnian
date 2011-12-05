//----------------------------------------------------------------------------
// runnerdebug.cpp
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "pstring.h"
#include "eventlog.h"
#include "CrashHandler.h"
#include "Ping0ServerMessages.h"
#include "runnerdebug.cpp.tmh"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define EXEPTION_TYPE(e)	case e: return #e; break;

CCriticalSection assertLock(TRUE);

//----------------------------------------------------------------------------
// DEBUG METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LONG RunnerTakeDump(EXCEPTION_POINTERS*pExPtrs)
{
    OS_PATH_CHAR szDumpFile[MAX_PATH];
	LONG toRet = EXCEPTION_CONTINUE_SEARCH;

    TraceError("LogProcessor:ServerCrash Writing crash dump\n");

    if( GetCrashFile(pExPtrs,szDumpFile,MAX_PATH) == EXCEPTION_CONTINUE_SEARCH)
    {
        TraceError("Could not create dump file\n");
        // OS will at least report that we crashed
        toRet = EXCEPTION_CONTINUE_SEARCH; 
    }
    else
    {
		TraceError("Crash dump logged in %ls\n",szDumpFile);
        toRet = EXCEPTION_EXECUTE_HANDLER; //stop exception processing
    }

	if(DesktopServerIsEnabled())
		toRet = EXCEPTION_CONTINUE_SEARCH;

	RunnerCloseLogging();
	FreeDesktopServerTracing();
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char * GetExceptionCodeString(
	DWORD exception)
{
	switch(exception)
	{
		EXEPTION_TYPE( EXCEPTION_ACCESS_VIOLATION )
		EXEPTION_TYPE( EXCEPTION_DATATYPE_MISALIGNMENT )
		EXEPTION_TYPE( EXCEPTION_BREAKPOINT )
		EXEPTION_TYPE( EXCEPTION_SINGLE_STEP )
		EXEPTION_TYPE( EXCEPTION_ARRAY_BOUNDS_EXCEEDED )
		EXEPTION_TYPE( EXCEPTION_FLT_DENORMAL_OPERAND )
		EXEPTION_TYPE( EXCEPTION_FLT_DIVIDE_BY_ZERO )
		EXEPTION_TYPE( EXCEPTION_FLT_INEXACT_RESULT )
		EXEPTION_TYPE( EXCEPTION_FLT_INVALID_OPERATION )
		EXEPTION_TYPE( EXCEPTION_FLT_OVERFLOW )
		EXEPTION_TYPE( EXCEPTION_FLT_STACK_CHECK )
		EXEPTION_TYPE( EXCEPTION_FLT_UNDERFLOW )
		EXEPTION_TYPE( EXCEPTION_INT_DIVIDE_BY_ZERO )
		EXEPTION_TYPE( EXCEPTION_INT_OVERFLOW )
		EXEPTION_TYPE( EXCEPTION_PRIV_INSTRUCTION )
		EXEPTION_TYPE( EXCEPTION_IN_PAGE_ERROR )
		EXEPTION_TYPE( EXCEPTION_ILLEGAL_INSTRUCTION )
		EXEPTION_TYPE( EXCEPTION_NONCONTINUABLE_EXCEPTION )
		EXEPTION_TYPE( EXCEPTION_STACK_OVERFLOW )
		EXEPTION_TYPE( EXCEPTION_INVALID_DISPOSITION )
		EXEPTION_TYPE( EXCEPTION_GUARD_PAGE )
		EXEPTION_TYPE( EXCEPTION_INVALID_HANDLE )
		case 0xE0000000:
			return "GAME_SERVER_ERROR";
		default:
			return "== UNKNOWN ==";
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RunnerExceptFilter(
	EXCEPTION_POINTERS*pExceptionPointers,
	SVRTYPE svrType,
	SVRINSTANCE svrInstance,
	const char * description )
{
	const char * serverName = ServerGetName(svrType);

	//	log to trace
	TraceError(
		"!! An exception was thrown. !!\n\tException Code: %#x\n\tException Type: %s\n"
        "\tat addresss %p\n\tDescription: \"%s\"\n"
        "\tExecuting Server Type: %s\n\tExecuting Server Instance: %hu",
		pExceptionPointers->ExceptionRecord->ExceptionCode,
		GetExceptionCodeString(pExceptionPointers->ExceptionRecord->ExceptionCode),
        pExceptionPointers->ExceptionRecord->ExceptionAddress,
		description,
		serverName,
		svrInstance);

	//	log to event log
	WCHAR wcServerName[256];
	PStrCvt(wcServerName, serverName, 256);
	LogErrorInEventLog(
		EventCategoryServerRunner,
		MSG_ID_EXCEPTION_WHILE_EXECUTING_SERVER,
		wcServerName,
		NULL,
		0);

	return EXCEPTION_CONTINUE_SEARCH;
}


//----------------------------------------------------------------------------
// desktop server methods
//----------------------------------------------------------------------------

static BOOL		g_isDesktopServerMode = FALSE;
static BOOL		g_isQuickStartDesktopServer = FALSE;
static char		g_desktopTraceFilePath[MAX_PATH];
static HANDLE	g_desktopTraceFileHandle = INVALID_HANDLE_VALUE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DesktopServerIsEnabled(
	void )
{
	BOOL toRet = g_isDesktopServerMode;
#if !ISVERSION(DEVELOPMENT)
	toRet = FALSE;	//	never desktop mode in release builds
#endif
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetDesktopServerMode(
	BOOL toSet )
{
	g_isDesktopServerMode = toSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuickStartDesktopServerIsEnabled(
	void )
{
#if ISVERSION(DEVELOPMENT)
	return (DesktopServerIsEnabled() && g_isQuickStartDesktopServer);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetQuickStartDesktopServerMode(
	BOOL toSet )
{
	g_isQuickStartDesktopServer = toSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitDesktopServerTracing(
	char * filename )
{
	if(!DesktopServerIsEnabled())
		return TRUE;

	ASSERT_RETFALSE(filename && filename[0]);

	PStrCopy(g_desktopTraceFilePath, filename, MAX_PATH);

	g_desktopTraceFileHandle = CreateFile(
		filename,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		0,
		NULL);
	ASSERTX_RETFALSE(
		g_desktopTraceFileHandle != INVALID_HANDLE_VALUE,
		"Unable to create desktop server mode trace file.");

	SetFilePointer(g_desktopTraceFileHandle, 0, NULL, FILE_END);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FreeDesktopServerTracing(
	void )
{
	if(g_desktopTraceFileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_desktopTraceFileHandle);
		g_desktopTraceFileHandle = INVALID_HANDLE_VALUE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DoDesktopServerAssert(
	char * szErrorMessage )
{
	if(!DesktopServerIsEnabled())
		return IDRETRY;

	DesktopServerTrace("%s", szErrorMessage);

	if(gtAppCommon.m_bSilentAssert)
		return IDIGNORE;

    {
        CSAutoLock lock(&assertLock);
        return MessageBox(NULL, szErrorMessage, "ERROR: Abort to exit, Retry to debug, Ignore to continue execution.", MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION );
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void DesktopServerTrace(
	const char * format,
	... )
{
	if(!DesktopServerIsEnabled())
		return;

	char message[4096];
	va_list args;
	va_start(args,format);
	PStrVprintf(message, 4096, format,args);

	//	to the logs
	TraceError(
		"DESKTOP TRACE: %s\n",
		message);
	
	//	to the console
	printf("%s\n", message);

	//	to the trace file
	DWORD numWritten = 0;
	if(g_desktopTraceFileHandle != INVALID_HANDLE_VALUE)
	{
		WriteFile(g_desktopTraceFileHandle, message, PStrLen(message, 4096), &numWritten, NULL);
		WriteFile(g_desktopTraceFileHandle, "\n", 1, &numWritten, NULL);
	}
}
#endif
