//-------------------------------------------------------------------------------------------------
// CrashHandler.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
#ifndef _CRASHHANDLER_H_
#define _CRASHHANDLER_H_

#define IDIGNOREALWAYS (IDIGNORE + 1)

// Custom exception codes
//
#define EXCEPTION_CODE_OUT_OF_MEMORY		0xE0000001  // See the RaiseException docs for how to define these
#define EXCEPTION_CODE_MEMORY_CORRUPTION	0xE0000002

// Client crash callback
typedef BOOL (*LPGETLOGFILE) (void * lpvState);

BOOL CrashHandlerInit(
	LPCTSTR szModulename,
	LPGETLOGFILE lpfn = NULL);           // Client crash callback

BOOL CrashHandlerDispose();

void GenerateErrorReport(
	PEXCEPTION_POINTERS pExInfo,		// Exception pointers (see MSDN)
	LPCTSTR szSummary = NULL,
	int nSummaryBufferLength = INVALID_ID);      

int GenerateAssertReport(
	LPCTSTR szSummary,
	int nSummaryBufferLength);

int GenerateUserBugReport(
	LPCTSTR szSummary, 
	int nSummaryBufferLength,
	const OS_PATH_CHAR* szScreenshotFile);


LONG GetCrashFile(PEXCEPTION_POINTERS pExInfo, 
                  OS_PATH_CHAR * szFile, 
                  int nStrLen);

ULONG AddBugReport(
	const char *szShortTitle,
	const char *szDescription,
	const char *szEmail,
	const OS_PATH_CHAR *szScreenshotFile);

BOOL CrashHandlerActive();

#endif