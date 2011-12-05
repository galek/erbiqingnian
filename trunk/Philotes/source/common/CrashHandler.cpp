// ---------------------------------------------------------------------------
// FILE:	CrashHandler.cpp
// DESC:	bug reporter
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// INCLUDE
// ---------------------------------------------------------------------------

#include "appcommon.h"
#include "CrashHandler.h"
#include "dbghelp.h"
#include "resource.h"
#include "filepaths.h"
#include "globalindex.h"

#if ISVERSION(SERVER_VERSION)
#include "CrashHandler.cpp.tmh"
#define sGetMiniDumpType() MiniDumpWithFullMemory
#else //!SERVER_VERSION
#if !ISVERSION(RETAIL_VERSION) || _PROFILE
#include "mysql.h" 
#endif //!_RETAIL
#include "sysinfo.h"	// CHB 2007.01.09
#include "nethttp.h"
#include "c_authticket.h"
#include <zlib.h>
#include <atlenc.h>
#include "config.h"		
#include "prime.h"
#include "version.h"

// CHB 2007.05.21 - These are needed for retail build.
#include "filetextlog.h"	// CHB 2007.05.21
typedef FileTextLogW SysInfoLog_File;

// CHB 2007.08.01 - Needed for MiniDumpWriteDump()
#pragma comment(lib, "dbghelp")

using namespace FSCommon;


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define MAX_DESCRIPT_LENGTH				8192
#define MAX_SHORTTITLE_LENGTH			1024

#if !ISVERSION(RETAIL_VERSION) || _PROFILE
#define ERROR_HANDLER_INSTRUCTIONS		"The application has encountered an error.  However, the situation is under control.  Do not be alarmed.  Would you like to report this error?  If so, please describe what you were doing when the error occured."
#define BUG_REPORT_INSTRUCTIONS			"So you wanna report a bug, huh?  You think you're a big man, huh?  Well go ahead.  Do your worst!"
#define DEFAULT_CRASH_DESCRIPTION		"Auto generated crash report"


#define HELLGATE_BUGZILLA_USER			"dumpor"
#define HELLGATE_BUGZILLA_PASSWORD		"shippy!"
#define HELLGATE_BUGZILLA_HOST			"fsshome.com"	//"10.1.50.24"	//
#define HELLGATE_BUGZILLA_DATABASE		"bugs"
#define HELLGATE_QA_HANDLER_LOGIN		"Queue@flagshipstudios.com"
#define HELLGATE_DEFAULT_LOGIN			"defaultbugzilla@flagshipstudios.com"
#define HELLGATE_BUG_FILE_LOC			"\\\\venus\\public\\bugdump\\"
#define HELLGATE_BUG_FILE_WEBPATH		"\\\\venus\\public\\bugdump\\"			// a symlink in the bugzilla directory on the web server

#define TUGBOAT_BUGZILLA_USER			"bugs"
#define TUGBOAT_BUGZILLA_PASSWORD		"pushinme"
#define TUGBOAT_BUGZILLA_HOST			"saturn.fsshome.com"
#define TUGBOAT_BUGZILLA_DATABASE		"bugs"
#define TUGBOAT_QA_HANDLER_LOGIN		"tbaldree@flagshipstudios.com"
#define TUGBOAT_DEFAULT_LOGIN			"defaultbugzilla@flagshipstudios.com"
#define TUGBOAT_BUG_FILE_LOC			"\\\\192.168.1.220\\public\\tugdump\\"
#define TUGBOAT_BUG_FILE_WEBPATH		"\\\\192.168.1.220\\public\\tugdump\\"	// a symlink in the bugzilla directory on the web server



// ---------------------------------------------------------------------------
// ENUMS
// ---------------------------------------------------------------------------
enum REPORT_TYPE
{
	RT_UNHANDLED_ERROR,
	RT_ASSERT,
	RT_USER_REPORT,
};


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
struct CRASH_HANDLER_LOCALS
{
	REPORT_TYPE m_eReportType;
	BOOL m_bInitialized;
	LPGETLOGFILE m_lpfnGetLogFile;
	LPTOP_LEVEL_EXCEPTION_FILTER  m_oldFilter;      // previous exception filter
	char szModulename[256];
	MYSQL * pSQLData;
	char szEmail[256];
	ULONG nBugzillaUserID;
	char szShortTitle[MAX_SHORTTITLE_LENGTH];
	char szDescription[MAX_DESCRIPT_LENGTH];
};


// ---------------------------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------------------------
CRASH_HANDLER_LOCALS g_Locals;

static const char * BUGZILLA_USER = NULL;
static const char * BUGZILLA_PASSWORD = NULL;
static const char * BUGZILLA_HOST = NULL;
static const char * BUGZILLA_DATABASE = NULL;
static const char * QA_HANDLER_LOGIN = NULL;
static const char * DEFAULT_LOGIN = NULL;
static const OS_PATH_CHAR * BUG_FILE_LOC = NULL;
static const char * BUG_FILE_WEBPATH = NULL;

#endif //!SERVER_VERSION

#if USE_MEMORY_MANAGER
#define CRASH_ALLOCATOR IMemoryManager::GetInstance().GetSystemAllocator()
#else
#define CRASH_ALLOCATOR NULL
#endif

static BOOL sbDialogActive = FALSE;

static const WCHAR* hellgateRegKey = L"Software\\Flagship Studios\\Hellgate London";
static const WCHAR* tugboatRegKey = L"Software\\Flagship Studios\\Mythos";
// ---------------------------------------------------------------------------
// FORWARD DECLARATIONS
// ---------------------------------------------------------------------------
void SendError(PEXCEPTION_POINTERS pExInfo);
// Crash-dump upload features
static BOOL sCommonUploadCrashDump(__in __notnull const char *szDumpServer,
                                   __in_opt const OS_PATH_CHAR *szDumpFile,
                                   __in __notnull const char *szUserName,
                                   __in_opt const char *szTitle,
                                   __in_opt const char *szDescription,
                                   __in_opt const OS_PATH_CHAR *szScreenShotFile);

ULONG GetBugzillaUserID(LPCTSTR szEmail);
BOOL DisconnectFromBugzillaDB();

// ---------------------------------------------------------------------------
// FUNCTIONS
// ---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CrashHandlerActive
//
// Returns
//	TRUE if the crash dialog is up, FALSE if not.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CrashHandlerActive()
{
	return sbDialogActive;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CustomUnhandledExceptionFilter
//
// Parameters
//	pExInfo : A pointer to the exception information
//
// Returns
//	Should return EXCEPTION_CONTINUE_SEARCH if the exception occurs 
//	during a debug session so the debugger has a change to handle the
//	exception.  It will also cause the windows popup to come up asking 
//  the user if they want to terminate or debug the app.
//
//	Should return EXCEPTION_EXECUTE_HANDLER otherwise, which
//	returns control to the exception filter's exception block, which
//	will probably terminate the app.
//
// Remarks:
//	The default unhandled exception filter is what calls this method.
//	If a debugger is attached, this method won't be called.
//
//	Any further exceptions generated in this function will just terminate
//	the process with no popup.
//
// Notes:
//  The documentation says that GetExceptionInformation cannot be called
//	from within a filter function, only a filter expression.  GenerateErrorReport
//	is calling GetExceptionInformation, when it probably should be taking it
//	from the argument to this function.
//
//	In retail, if sUploadDumpToServer fails for some reason, there will be
//	no popup and the app will just terminate.
//
/////////////////////////////////////////////////////////////////////////////
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{

#if USE_MEMORY_MANAGER	
	if(pExInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CODE_OUT_OF_MEMORY)
	{
		IMemoryManager::GetInstance().MemTrace("Out Of Memory", NULL, false);	
	}
#endif

#if ISVERSION(RETAIL_VERSION) && !_PROFILE // Retail builds

	OS_PATH_CHAR crashDump[MAX_PATH];

	// Generate a crash dump
	//
	if(GetCrashFile(pExInfo, crashDump, MAX_PATH) != EXCEPTION_CONTINUE_SEARCH)
	{
		// Attempt to upload it to the server
		//
#if USE_MEMORY_MANAGER		

		if(pExInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CODE_OUT_OF_MEMORY)
		{
		
			OS_PATH_CHAR memTracePath[MAX_PATH];
			const OS_PATH_CHAR* logFilePath = FilePath_GetSystemPath(FILE_PATH_LOG);

			time_t curtime;
			time(&curtime);
			struct tm curtimeex;
			localtime_s(&curtimeex, &curtime);	
			PStrPrintf(memTracePath, MAX_PATH, OS_PATH_TEXT("%sMEMTRACE%04d%02d%02d_0000000001.xml"), logFilePath, (int)curtimeex.tm_year + 1900, (int)curtimeex.tm_mon + 1, (int)curtimeex.tm_mday);
		
			sCommonUploadCrashDump(AppGetAuthenticationIp(), crashDump, "SomePoorUser", "crash dump", "out of memory", memTracePath);
		}
		else if(pExInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CODE_MEMORY_CORRUPTION)
		{
			sCommonUploadCrashDump(AppGetAuthenticationIp(), crashDump, "SomePoorUser", "crash dump", "memory corruption detected", NULL);
		}
		else
#endif		
		{
			sCommonUploadCrashDump(AppGetAuthenticationIp(), crashDump, "SomePoorUser", "crash dump", "A Crash dump", NULL);
		}
	}	
	
#else // Non-Retail builds
		
	GenerateErrorReport(pExInfo); 	
	
#endif 


#if ISVERSION(RETAIL_VERSION) && !_PROFILE // Retail builds
	// Returning EXCEPTION_CONTINUE_SEARCH will cause the windows popup to ask
	// the user if they want to terminate the application.
	//
	// TODO - For Windows Vista, there are some new APIs to pass along the error to WER without creating another dialog...
	return EXCEPTION_CONTINUE_SEARCH;
#else // Non-Retail builds
	// Returning EXCEPTION_EXECUTE_HANDLER in this case will cause the app to
	// immediately terminate, which is okay here since the code above already
	// made a popup
	//
	return EXCEPTION_EXECUTE_HANDLER;
#endif 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(RETAIL_VERSION) || _PROFILE
void HandleInternalError(
	const char *format,
	...)
{
	va_list args;
	va_start(args, format);

	ErrorDialogV( format, args );	// CHB 2007.08.01 - String audit: development
	LogMessageV(ERROR_LOG, format, args);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(RETAIL_VERSION) || _PROFILE
void DisplayLastWinError()
{
	LPVOID lpMsgBuf;
	if (!FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL ))
	{
		// Handle the error.
		return;
	}

	// Display the string.
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST );	// CHB 2007.08.01 - String audit: development

	TraceError("WinError: %s", lpMsgBuf);

	// Free the buffer.
	LocalFree( lpMsgBuf );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 0	// CHB 2007.08.01 - unused
void TraceStack(
	char * szTrace,
	int	nLen)
{
	AfxDumpStack(AFX_STACK_DUMP_TARGET_BOTH);

	if (OpenClipboard(AppCommonGetHWnd())) 
    { 
		HANDLE hData = GetClipboardData(CF_TEXT); 
		if (hData == NULL)
		{
			DisplayLastWinError();
		}
		else
		{
			char *szClipboard = (char *)GlobalLock(hData); 

			if (szClipboard)
			{
				const char * szStartPoint = NULL;
				// if this was an assert, we don't care much for the error-handling calls after
				if ((szStartPoint = PStrStr(szClipboard, "GenerateAssertReport")) != NULL)
				{
					// find the next line
					szStartPoint = PStrStr(szStartPoint, "\n");
					if (szStartPoint)
					{
						szStartPoint++;	// next char
						PStrCopy(szTrace, szStartPoint, nLen);
					}
				}
				else if ((szStartPoint = PStrStr(szClipboard, "CustomUnhandledExceptionFilter")) != NULL)
				{
					// find the next line
					szStartPoint = PStrStr(szStartPoint, "\n");
					if (szStartPoint)
					{
						szStartPoint++;	// next char
						PStrCopy(szTrace, szStartPoint, nLen);
					}
				}
				else
				{
					// just copy it all
					PStrCopy(szTrace, szClipboard, nLen);
				}
			}
	
			GlobalUnlock(hData); 
		}

        CloseClipboard(); 
    } 
}
#endif

#if !ISVERSION(RETAIL_VERSION) || _PROFILE
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INT_PTR CALLBACK CrashDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	UNREFERENCED_PARAMETER(lParam);
	GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal(TRUE);
    if(pDefinition)
    {
        if(	pDefinition->dwFlags & GLOBAL_FLAG_NO_POPUPS 
		 && ! AppTestDebugFlag( ADF_FORCE_ASSERTS_ON ) )
        {
            ExitProcess((DWORD)-1);
        }
    }
	switch (message) 
	{ 
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK: 
		case IDABORT:
		case IDRETRY:
		case IDIGNORE:
		case IDIGNOREALWAYS:
			if (!GetDlgItemText(hwndDlg, IDC_DESCRIPTION, g_Locals.szDescription, MAX_DESCRIPT_LENGTH)) 
			{
				*g_Locals.szDescription = 0; 
			}
			if (!GetDlgItemText(hwndDlg, IDC_TITLE, g_Locals.szShortTitle, MAX_SHORTTITLE_LENGTH)) 
			{
				PStrCopy(g_Locals.szShortTitle, DEFAULT_CRASH_DESCRIPTION, MAX_SHORTTITLE_LENGTH);
			}
			if (!GetDlgItemText(hwndDlg, IDC_LOGIN, g_Locals.szEmail, 256)) 
			{
				*g_Locals.szEmail = 0; 
				if (IsDlgButtonChecked(hwndDlg, IDC_CHK_SEND) == BST_CHECKED)
				{
					MessageBox( hwndDlg, "Please enter a valid bugzilla login email address before sending to Bugzilla.", "Enter Email", MB_ICONEXCLAMATION | MB_OK ) ;	// CHB 2007.08.01 - String audit: development
					break;
				}
			}
			else
			{
				g_Locals.nBugzillaUserID = 0;
				if (IsDlgButtonChecked(hwndDlg, IDC_CHK_SEND) == BST_CHECKED ||
					LOWORD(wParam) == IDOK)											// this is the "send" button
				{
					g_Locals.nBugzillaUserID = GetBugzillaUserID(g_Locals.szEmail);
					DisconnectFromBugzillaDB();

					if (g_Locals.nBugzillaUserID == 0)
					{
						MessageBox( hwndDlg, "Please enter a valid bugzilla login email address before sending to Bugzilla.", "Enter Email", MB_ICONEXCLAMATION | MB_OK ) ;	// CHB 2007.08.01 - String audit: development
						break;
					}
					else			
					{
						::WritePrivateProfileString("Flagship Error Handler", "Bugzilla Login", g_Locals.szEmail, "CrashHandler.ini");
					}
				}
			}
			// Fall through. 

		case IDCANCEL: 
			if ((LOWORD(wParam) == IDABORT ||
				 LOWORD(wParam) == IDRETRY ||
				 LOWORD(wParam) == IDIGNORE ||
				 LOWORD(wParam) == IDIGNOREALWAYS) &&
			    IsDlgButtonChecked(hwndDlg, IDC_CHK_SEND) == BST_CHECKED)
			{
				TraceError("Sending error with NO exception pointers");
				SendError(NULL);
			}

			TraceError("Ending dialog with %u as the result.", wParam);
			EndDialog(hwndDlg, wParam); 

			if (LOWORD(wParam) == IDRETRY)
			{
				//Uninstall the crash handler before we break, so it'll go to the default system crash handler.
				CrashHandlerDispose();
			}
			return TRUE; 
		}
		break;
	case WM_INITDIALOG:
		{
			if (g_Locals.szShortTitle && g_Locals.szShortTitle[0])
			{
				SetDlgItemText(hwndDlg, IDC_TITLE, g_Locals.szShortTitle);
			}
			::GetPrivateProfileString("Flagship Error Handler", "Bugzilla Login", "", g_Locals.szEmail, 256, "CrashHandler.ini");
			SetDlgItemText(hwndDlg, IDC_LOGIN, g_Locals.szEmail); 

			SetDlgItemText(hwndDlg, IDC_INSTRUCTIONS, g_Locals.m_eReportType == RT_USER_REPORT ? BUG_REPORT_INSTRUCTIONS : ERROR_HANDLER_INSTRUCTIONS);

			HWND hwndControl = NULL;
			hwndControl = GetDlgItem(hwndDlg, IDOK);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType == RT_ASSERT ? SW_HIDE : SW_SHOW);
			hwndControl = GetDlgItem(hwndDlg, IDCANCEL);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType == RT_ASSERT ? SW_HIDE : SW_SHOW);
			
			hwndControl = GetDlgItem(hwndDlg, IDC_CHK_SEND);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType != RT_ASSERT ? SW_HIDE : SW_SHOW);
			hwndControl = GetDlgItem(hwndDlg, IDABORT);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType != RT_ASSERT ? SW_HIDE : SW_SHOW);
			hwndControl = GetDlgItem(hwndDlg, IDRETRY);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType != RT_ASSERT ? SW_HIDE : SW_SHOW);
			hwndControl = GetDlgItem(hwndDlg, IDIGNORE);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType != RT_ASSERT ? SW_HIDE : SW_SHOW);
			hwndControl = GetDlgItem(hwndDlg, IDIGNOREALWAYS);
			ASSERT(hwndControl);
			ShowWindow(hwndControl, g_Locals.m_eReportType != RT_ASSERT ? SW_HIDE : SW_SHOW);

			SetWindowPos(hwndDlg, 
				HWND_TOPMOST, 
				0, 0, 0, 0,          // ignores move and size arguments 
				SWP_NOSIZE | SWP_NOMOVE); 

			//Set the starting keyboard focus
			SetFocus(GetDlgItem(hwndDlg, (g_Locals.m_eReportType == RT_ASSERT ? IDIGNORE : IDC_TITLE)));
			return FALSE;
		}
		break;

	case WM_ACTIVATE:
		{
			if (!c_GetToolMode())
			{
				BOOL bActive = ( LOWORD(wParam) != WA_INACTIVE );
				int nCount = 0;
				do		// make sure it's turned ... the right way
				{
					nCount = ::ShowCursor(bActive);
				} while ((bActive && nCount < 0) || (!bActive && nCount >= 0));

				trace("Crash dialog cursor show count (active=%d)  %d\n", bActive, nCount);
				return TRUE;
			}
		}

	case WM_INPUT:
		// don't let the prime wndproc handle this
		return TRUE;

	} 
	return FALSE; 
} 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectToBugzillaDB()
{
	if ((g_Locals.pSQLData = mysql_init((MYSQL*)0)) != NULL)
	{
		if (mysql_real_connect( g_Locals.pSQLData, BUGZILLA_HOST, BUGZILLA_USER, BUGZILLA_PASSWORD, BUGZILLA_DATABASE, MYSQL_PORT, NULL, 0 ) )
		{
			if ( mysql_select_db( g_Locals.pSQLData, BUGZILLA_DATABASE ) < 0 ) 
			{
				HandleInternalError( "Can't select the %s database !\n", BUGZILLA_DATABASE );
				DisconnectFromBugzillaDB();
				return FALSE;
			}
		}
		else 
		{
			const char * szErr = mysql_error(g_Locals.pSQLData);
			HandleInternalError( "Failed to connect to Bugzilla database: Error: %s\n", szErr);
			DisconnectFromBugzillaDB();
			return FALSE;
		}
	}
	else 
	{
		HandleInternalError( "Can't initialize the MySQL interface for Bugzilla." ) ;
		DisconnectFromBugzillaDB();
		return FALSE;
	}

	TraceError("Connected to Bugzilla database %s host %s with database user %s", BUGZILLA_DATABASE, BUGZILLA_HOST, BUGZILLA_USER);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DisconnectFromBugzillaDB()
{
	if (g_Locals.pSQLData)
		mysql_close( g_Locals.pSQLData );

	g_Locals.pSQLData = NULL;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG GetBugzillaUserID(LPCTSTR szEmail)
{
	ULONG nID = 0;

	if (!ConnectToBugzillaDB())
		return 0;

	char szQuery[512];
	PStrPrintf(szQuery, 512, "SELECT userid FROM profiles WHERE login_name = '%s'", szEmail);
	if (mysql_query(g_Locals.pSQLData, szQuery) != 0)
	{
		HandleInternalError( "Failed to select profile from Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
		DisconnectFromBugzillaDB();
		return 0;
	}
	MYSQL_RES *pResults = mysql_use_result(g_Locals.pSQLData);
	if (pResults)
	{
		MYSQL_ROW row;
		ASSERT(mysql_num_fields(pResults) > 0);
		while ((row = mysql_fetch_row(pResults)) != NULL)
		{
			nID = atol(row[0]);
		}
	}
	else
	{
		HandleInternalError( "Failed to select profile from Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
		DisconnectFromBugzillaDB();
		return 0;
	}

	TraceError("Located Bugzilla userid for user %s", szEmail);

	return nID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG InsertBugzillaRecord(void)
{
	if (!ConnectToBugzillaDB())
		return 0;

	char szQuery[MAX_DESCRIPT_LENGTH * 2];
	char szDateTime[128];

	time_t curtime;
	time(&curtime);
	struct tm curtimeex;
	localtime_s(&curtimeex, &curtime);
	PStrPrintf(szDateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", 
		(int)curtimeex.tm_year + 1900, 
		(int)curtimeex.tm_mon + 1, 
		(int)curtimeex.tm_mday, 
		(int)curtimeex.tm_hour, 
		(int)curtimeex.tm_min, 
		(int)curtimeex.tm_sec);

	ULONG nReporter = g_Locals.nBugzillaUserID;
	if (nReporter == 0)	//couldn't find the name entered.  We need a valid id for descriptions to show, so find the default profile
	{
		nReporter = GetBugzillaUserID(DEFAULT_LOGIN);
	}

	int nAssignTo = GetBugzillaUserID(QA_HANDLER_LOGIN);
	
	// init product id for hellgate
	int nProductID = 1;		// Prime
	int nComponentID = 4;	// General
	
	// override for other projects
	if (AppIsTugboat())
	{
		nProductID = 2;		// Tugboat
		nComponentID = 3;	// General
	}

	//PStrReplaceChars(g_Locals.szShortTitle, '\'', '\"');
	//TODO: Add real (not hardcoded) bug severity and priority fields
	char szAdjustedTitle[MAX_SHORTTITLE_LENGTH * 2];
	mysql_real_escape_string(g_Locals.pSQLData, szAdjustedTitle, g_Locals.szShortTitle, PStrLen(g_Locals.szShortTitle));
	PStrPrintf(szQuery, MAX_DESCRIPT_LENGTH * 2,
		"INSERT into bugs (assigned_to, creation_ts, short_desc, rep_platform, reporter, product_id, version, component_id, bug_severity, bug_status) values (%d, '%s', '%s', 'PC', %lu, %d, %d, %d, 'normal','UNCONFIRMED')",
		nAssignTo,
		szDateTime,
		szAdjustedTitle,
		nReporter,
		nProductID,
		BUILD_VERSION,
		nComponentID);

	if (mysql_query(g_Locals.pSQLData, szQuery) != 0)
	{
		HandleInternalError( "Failed to add record to Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
		DisconnectFromBugzillaDB();
		return 0;
	}

	TraceError("New Bugzilla record inserted");

	ULONG nBugID = (ULONG)mysql_insert_id(g_Locals.pSQLData);
	if (nBugID)
	{
		TraceError("New Bugzilla record ID = %lu", nBugID);		// CHB 2007.01.09

		//char szStackTrace[MAX_DESCRIPT_LENGTH / 2];
		//TraceStack(szStackTrace, arrsize(szStackTrace));

		char szBuf[MAX_DESCRIPT_LENGTH+25];
		PStrPrintf(szBuf, MAX_DESCRIPT_LENGTH+25, "Build Version [%d.%d.%d.%d]%c%c%s", 
                                                   TITLE_VERSION,
                                                   PRODUCTION_BRANCH_VERSION,
                                                   RC_BRANCH_VERSION,
                                                   MAIN_LINE_VERSION, 13, 10, g_Locals.szDescription);

		char szAdjustedDescript[MAX_DESCRIPT_LENGTH * 2];
		mysql_real_escape_string(g_Locals.pSQLData, szAdjustedDescript, szBuf, PStrLen(szBuf));
		PStrPrintf(szQuery, MAX_DESCRIPT_LENGTH * 2,
			"INSERT into longdescs (bug_id, who, bug_when, thetext) values (%lu, %d, '%s', '%s')",
			nBugID,
			nReporter,
			szDateTime,
			szAdjustedDescript);

		if (mysql_query(g_Locals.pSQLData, szQuery) != 0)
		{
			HandleInternalError( "Failed to add long description to Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
			DisconnectFromBugzillaDB();
			return 0;
		}

		TraceError("Inserted new Bugzilla description record");
	}
	else
	{
		HandleInternalError("Failed to locate new Bugzilla record ID");
	}

	DisconnectFromBugzillaDB();
	return nBugID;
}

#endif !ISVERSION(RETAIL_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CrashHandlerInit(
	LPCTSTR szModulename,
	LPGETLOGFILE lpfn /*= NULL*/)           // Client crash callback
{
#if !ISVERSION(RETAIL_VERSION)
	if (g_Locals.m_bInitialized)
		return TRUE;

	if (AppIsHellgate())
	{
		BUGZILLA_USER =			HELLGATE_BUGZILLA_USER;
		BUGZILLA_PASSWORD =		HELLGATE_BUGZILLA_PASSWORD;
		BUGZILLA_HOST =			HELLGATE_BUGZILLA_HOST;
		BUGZILLA_DATABASE =		HELLGATE_BUGZILLA_DATABASE;
		QA_HANDLER_LOGIN =		HELLGATE_QA_HANDLER_LOGIN;
		DEFAULT_LOGIN =			HELLGATE_DEFAULT_LOGIN;
		BUG_FILE_LOC =			OS_PATH_TEXT(HELLGATE_BUG_FILE_LOC);
		BUG_FILE_WEBPATH =		HELLGATE_BUG_FILE_WEBPATH;
	}
	else if (AppIsTugboat())
	{
		BUGZILLA_USER =			TUGBOAT_BUGZILLA_USER;
		BUGZILLA_PASSWORD =		TUGBOAT_BUGZILLA_PASSWORD;
		BUGZILLA_HOST =			TUGBOAT_BUGZILLA_HOST;
		BUGZILLA_DATABASE =		TUGBOAT_BUGZILLA_DATABASE;
		QA_HANDLER_LOGIN =		TUGBOAT_QA_HANDLER_LOGIN;
		DEFAULT_LOGIN =			TUGBOAT_DEFAULT_LOGIN;
		BUG_FILE_LOC =			OS_PATH_TEXT(TUGBOAT_BUG_FILE_LOC);
		BUG_FILE_WEBPATH =		TUGBOAT_BUG_FILE_WEBPATH;
	}

	// save user supplied callback
	if (lpfn)
	{
		g_Locals.m_lpfnGetLogFile = lpfn;
	}

	if (szModulename)
	{
		PStrCopy(g_Locals.szModulename, szModulename, 256);
	}

    // do not invoke crash handler on the build server.
    if(PStrStr(GetCommandLine(),"-nopopups") == NULL)
    {
        // add this filter in the exception callback chain
        g_Locals.m_oldFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
    }
#else
    UNREFERENCED_PARAMETER(lpfn);
    UNREFERENCED_PARAMETER(szModulename);
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
#endif // !ISVERSION(RETAIL_VERSION)

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CrashHandlerDispose()
{
#if !ISVERSION(RETAIL_VERSION)
	// reset exception callback
	if (g_Locals.m_oldFilter)
		SetUnhandledExceptionFilter(g_Locals.m_oldFilter);
#endif // !ISVERSION(RETAIL_VERSION)

	return TRUE;
}


#if !ISVERSION(RETAIL_VERSION) || _PROFILE
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AttachFileToBug(ULONG nBugID, const OS_PATH_CHAR* szFilepath, LPCTSTR szMIMEtype)
{
#define INSERT_QUERY		"INSERT INTO attachments(bug_id, description, mimetype, filename, creation_ts, thedata) VALUES(%lu, '%s', '%s', '%s', '%s', %s)"
#define INSERT_QUERY2		"INSERT INTO attachments(bug_id, description, mimetype, filename, creation_ts)			VALUES(%lu, '%s', '%s', '%s', '%s')"
#define INSERT_BLOB_QUERY	"INSERT INTO attach_data(id, thedata) VALUES(%lu, ?)"

	MYSQL_BIND bind[1];

	if (!FileExists(szFilepath))
	{
		HandleInternalError("Could not find file \"%s\"", szFilepath);
		return FALSE;
	}

	DWORD n;
	char * pFileData = (char *) FileLoad(MEMORY_FUNC_FILELINE(CRASH_ALLOCATOR, szFilepath, &n));
	ULONG nFileSize = (ULONG)n;
	if (!pFileData)
	{
		HandleInternalError("Error reading file \"%s\"", szFilepath);
		return FALSE;
	}

	if (!ConnectToBugzillaDB())
		goto FAIL;

	OS_PATH_CHAR szFilename[MAX_PATH];
	PStrRemoveEntirePath(szFilename, MAX_PATH, szFilepath);
	char szQuery[512];
	char szDateTime[128];

	time_t curtime;
	time(&curtime);
	struct tm curtimeex;
	localtime_s(&curtimeex, &curtime);
	PStrPrintf(szDateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", 
		(int)curtimeex.tm_year + 1900, 
		(int)curtimeex.tm_mon + 1, 
		(int)curtimeex.tm_mday, 
		(int)curtimeex.tm_hour, 
		(int)curtimeex.tm_min, 
		(int)curtimeex.tm_sec);

	unsigned long uVersion = mysql_get_server_version(g_Locals.pSQLData);
	if (uVersion >= 50000)
	{
		PStrPrintf(szQuery, 512, INSERT_QUERY2, nBugID, szFilename, szMIMEtype, szFilename, szDateTime);

		if (mysql_query(g_Locals.pSQLData, szQuery) != 0)
		{
			HandleInternalError( "Failed to add attachment record to Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
			DisconnectFromBugzillaDB();
			return 0;
		}

		TraceError("New Bugzilla attachment record inserted");

		ULONG nAttachID = (ULONG)mysql_insert_id(g_Locals.pSQLData);
		if (nAttachID)
		{
			TraceError("New Bugzilla attachment record ID = %lu", nAttachID);		// CHB 2007.01.09

			MYSQL_STMT * pStatement = mysql_stmt_init(g_Locals.pSQLData);
			if (!pStatement)
			{
				HandleInternalError("mysql_stmt_init(), out of memory");
				goto FAIL;
			}
			
			PStrPrintf(szQuery, 512, INSERT_BLOB_QUERY, nAttachID);

			if (mysql_stmt_prepare(pStatement, szQuery, PStrLen(szQuery)))
			{
				HandleInternalError("mysql_stmt_prepare(), INSERT failed - %s\n%s", szQuery, mysql_stmt_error(pStatement));
				goto FAIL;
			}
			memset(bind, 0, sizeof(bind));
			bind[0].buffer_type= MYSQL_TYPE_LONG_BLOB;
			bind[0].length= &nFileSize;
			bind[0].is_null= 0;

			/* Bind the buffers */
			if (mysql_stmt_bind_param(pStatement, bind))
			{
				HandleInternalError("param bind failed - %s", mysql_stmt_error(pStatement));
				goto FAIL;
			}

			/* Supply data in chunks to server */
			if (mysql_stmt_send_long_data(pStatement, 0, pFileData, nFileSize))
			{
				HandleInternalError("send_long_data failed - %s", mysql_stmt_error(pStatement));
				goto FAIL;
			}

			/* Now, execute the query */
			if (mysql_stmt_execute(pStatement))
			{
				HandleInternalError("mysql_stmt_execute failed - %s", mysql_stmt_error(pStatement));
				goto FAIL;
			}

			TraceError("Successfully added attachment to Bugzilla (ver >= 50000)");
		}
		else
		{
			HandleInternalError("Failed to locate new Bugzilla attachment record ID");
		}

	}
	else if (uVersion >= 40100)
	{
		MYSQL_STMT * pStatement = mysql_stmt_init(g_Locals.pSQLData);
		if (!pStatement)
		{
			HandleInternalError("mysql_stmt_init(), out of memory");
			goto FAIL;
		}
		
		PStrPrintf(szQuery, 512, INSERT_QUERY, nBugID, szFilename, szMIMEtype, szFilename, szDateTime, "?");

		if (mysql_stmt_prepare(pStatement, szQuery, PStrLen(szQuery)))
		{
			HandleInternalError("mysql_stmt_prepare(), INSERT failed - %s\n%s", szQuery, mysql_stmt_error(pStatement));
			goto FAIL;
		}
		memset(bind, 0, sizeof(bind));
		bind[0].buffer_type= MYSQL_TYPE_LONG_BLOB;
		bind[0].length= &nFileSize;
		bind[0].is_null= 0;

		/* Bind the buffers */
		if (mysql_stmt_bind_param(pStatement, bind))
		{
			HandleInternalError("param bind failed - %s", mysql_stmt_error(pStatement));
			goto FAIL;
		}

		/* Supply data in chunks to server */
		if (!mysql_stmt_send_long_data(pStatement, 0, pFileData, nFileSize))
		{
			HandleInternalError("send_long_data failed - %s", mysql_stmt_error(pStatement));
			goto FAIL;
		}

		/* Now, execute the query */
		if (mysql_stmt_execute(pStatement))
		{
			HandleInternalError("mysql_stmt_execute failed - %s", mysql_stmt_error(pStatement));
			goto FAIL;
		}

		TraceError("Successfully added attachment to Bugzilla (ver >= 40100)");
	}
	else
	{
		PStrPrintf(szQuery, 512, INSERT_QUERY, nBugID, szFilename, szMIMEtype, szFilename, szDateTime, "'%s'");

		char * szBinData = (char *)MALLOCZ(CRASH_ALLOCATOR, nFileSize * 2 + 1);
        mysql_real_escape_string(g_Locals.pSQLData, szBinData, pFileData, nFileSize);

		ULONG nBigQuerySize = PStrLen(szQuery) + PStrLen(szBinData);
		ULONG nMaxSize = 16 * 1024 * 1024;	//max_allowed_packet;	// the param doesn't seem to be returning the right value right now, so I'm going to hard code it for now

		if (nMaxSize < nBigQuerySize)
		{
			//HandleInternalError("max_allowed_packet is too small to send attachment [%s].  Try increasing its value.", szFilepath);
			char szMsg[512];
			PStrPrintf(szMsg, 512, "Attachment [%s] is too large to add to the database.", szFilepath);
			MessageBox(AppCommonGetHWnd(), szMsg, "Too Big", MB_OK);	// CHB 2007.08.01 - String audit: development
			FREE(CRASH_ALLOCATOR, szBinData);
			goto FAIL;
		}

		char * szBigQuery = (char *)MALLOCZ(CRASH_ALLOCATOR, nBigQuerySize + 1);
		PStrPrintf(szBigQuery, nBigQuerySize, szQuery, szBinData);
		
		int nRetVal = mysql_query(g_Locals.pSQLData, szBigQuery);

		FREE(CRASH_ALLOCATOR, szBinData);
		FREE(CRASH_ALLOCATOR, szBigQuery);

		if (nRetVal != 0)
		{
			HandleInternalError( "Failed to add record to Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
			goto FAIL;
		}

		TraceError("Successfully added attachment to Bugzilla (ver < 40100)");
	}

	DisconnectFromBugzillaDB();
	FREE( CRASH_ALLOCATOR, pFileData );
	return TRUE;

FAIL:
	DisconnectFromBugzillaDB();
	FREE( CRASH_ALLOCATOR, pFileData );
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendFileToBugDump(ULONG nBugID, const OS_PATH_CHAR * szFilepath)
{
#define UPPDATE_QUERY	"UPDATE bugs SET bug_file_loc = '%s' WHERE bug_id = %d"

	OS_PATH_CHAR szDirName[MAX_PATH];
	PStrPrintf(szDirName, _countof(szDirName), OS_PATH_TEXT("%s%d\\"), BUG_FILE_LOC, nBugID);
	OS_PATH_CHAR szDirCheckName[MAX_PATH];
	PStrPrintf(szDirCheckName, _countof(szDirCheckName), OS_PATH_TEXT("%s%d\\."), BUG_FILE_LOC, nBugID);
	char szWebDirName[MAX_PATH];
	PStrPrintf(szWebDirName, MAX_PATH, "%s%d\\", BUG_FILE_WEBPATH, nBugID);

	//See if the directory already exists
	OS_PATH_FUNC(WIN32_FIND_DATA) tFileFindData;
	FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)(szDirCheckName, &tFileFindData);
	if (handle == INVALID_HANDLE_VALUE)
	{
		if (!OS_PATH_FUNC(CreateDirectory)(szDirName, NULL))
		{
			TraceError("!! FAILED CreateDirectory(\"%s\")", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szDirName) );	
			DisplayLastWinError();
			return FALSE;
		}
	}

	BOOL retval = FALSE;
	ONCE
	{
		if (!ConnectToBugzillaDB())
		{
			break;
		}

		char szQuery[512];
		char szAdjustedPath[MAX_PATH * 2];
		mysql_real_escape_string(g_Locals.pSQLData, szAdjustedPath, szWebDirName, PStrLen(szWebDirName));

		PStrPrintf(szQuery, 512, UPPDATE_QUERY, szAdjustedPath, nBugID);

		if (mysql_query(g_Locals.pSQLData, szQuery) != 0)
		{
			HandleInternalError( "Failed to update record in Bugzilla database: Error: %s\n", mysql_error(g_Locals.pSQLData));
		}
		TraceError("Successfully updated Bugzilla record with bugdump location.");

		OS_PATH_CHAR szFilename[MAX_PATH];
		PStrRemoveEntirePath(szFilename, MAX_PATH, szFilepath);
		OS_PATH_CHAR szDestFilepath[MAX_PATH];
		PStrPrintf(szDestFilepath, MAX_PATH, OS_PATH_TEXT("%s%s"), szDirName, szFilename);

		if (!OS_PATH_FUNC(CopyFile)(szFilepath, szDestFilepath, FALSE))
		{
			TraceError("!! FAILED CopyFile(\"%s\", \"%s\")", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilepath), OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szDestFilepath));	
			DisplayLastWinError();
			DisconnectFromBugzillaDB();
			break;
		}
		TraceError("CopyFile(\"%s\", \"%s\")", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilepath), OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szDestFilepath));	// CHB 2007.01.09
		TraceError("Successfully copied dump file to bugdump location.");

		DisconnectFromBugzillaDB();

		retval = TRUE;
	}
	return retval;
}
#endif // !ISVERSION(RETAIL_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static MINIDUMP_TYPE sGetMiniDumpType(void)
{
#if !ISVERSION(RETAIL_VERSION)
    MINIDUMP_TYPE defaultType = MiniDumpWithIndirectlyReferencedMemory;
#else
    MINIDUMP_TYPE defaultType = MiniDumpNormal;
#endif
    DWORD dwValueType = 0;
    DWORD dwData,dwDataLen;
    HKEY hKey;

    if(RegOpenKeyExW(HKEY_CURRENT_USER,
                     AppIsTugboat()?tugboatRegKey:hellgateRegKey, 
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS)
    {
        return defaultType;
    }

    if(RegQueryValueExW(hKey,
                        L"MiniDumpType",
                        NULL,
                        &dwValueType,
                        (BYTE*)&dwData,
                        &dwDataLen) == ERROR_SUCCESS)
    {
            if(dwValueType == REG_DWORD)
            {
                defaultType = (MINIDUMP_TYPE)dwData;
            }
    }
    return defaultType;
}

#endif //!SERVER_VERSION
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LONG GetCrashFile(PEXCEPTION_POINTERS pExInfo, OS_PATH_CHAR * szFile, int nStrLen)
{
	const OS_PATH_CHAR * pCrashPath = FilePath_GetSystemPath(FILE_PATH_CRASH);
	if (!FileDirectoryExists(pCrashPath))
    {
		if(!FileCreateDirectory(pCrashPath))
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

	// Create the dump file name
	time_t curtime;
	time(&curtime);
	struct tm curtimeex;
	localtime_s(&curtimeex, &curtime);
	PStrPrintf(szFile, nStrLen, OS_PATH_TEXT("%sminidump_%04d%02d%02d_%02d%02d.dmp"), pCrashPath,
		(int)curtimeex.tm_year + 1900, (int)curtimeex.tm_mon + 1, (int)curtimeex.tm_mday, (int)curtimeex.tm_hour, (int)curtimeex.tm_min);

	// Create the file
	HANDLE hFile = FileOpen(
		szFile,
		GENERIC_WRITE,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL);

	//
	// Write the minidump to the file
	//
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ThreadId = GetCurrentThreadId();
		eInfo.ExceptionPointers = pExInfo;
		eInfo.ClientPointers = FALSE;
        MINIDUMP_TYPE minidumpType = sGetMiniDumpType();


		if(MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			minidumpType, //MiniDumpNormal,
			pExInfo ? &eInfo : NULL,
			NULL,
			NULL))
        {
            TraceError("Successfully opened crash file and wrote mini-dump.");
        }
	}
    else
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

	// Close file
	CloseHandle(hFile);


	return EXCEPTION_EXECUTE_HANDLER;
}

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MakeDump(OS_PATH_CHAR * szFile, int nStrLen)
{
	__try
	{
		//This is a planned error to generate a minidump.
		// Go up the stack for a bit, and you'll see where the actual assert happened.
		int *pPointerThatFails = 0;
		*pPointerThatFails = 0;
	}
	__except( GetCrashFile( GetExceptionInformation(), szFile, nStrLen ) )
	{
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(RETAIL_VERSION) || _PROFILE
void SendError(PEXCEPTION_POINTERS pExInfo)
{
	OS_PATH_CHAR szBuffer[MAX_PATH] = OS_PATH_TEXT("");

	//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_WAIT ) );

	if (pExInfo)
	{
		GetCrashFile(pExInfo, szBuffer, MAX_PATH);
	}
	else
	{
		MakeDump(szBuffer, MAX_PATH);
	}

	ULONG nBugID = InsertBugzillaRecord();
	if (nBugID)
	{
		if (szBuffer && szBuffer[0])
		{
			//AttachFileToBug(nBugID, szBuffer, "application/octet-stream");
			if (!SendFileToBugDump(nBugID, szBuffer))
			{
				// might want to put another message here.  Not doing it yet.
				//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
				return;
			}
		}

		// CHB 2007.01.09
		{
			// Create the file name.
			OS_PATH_CHAR szSILFile[MAX_PATH];
			time_t curtime;
			time(&curtime);
			struct tm curtimeex;
			localtime_s(&curtimeex, &curtime);
			PStrPrintf(szSILFile, _countof(szSILFile), OS_PATH_TEXT("%s%s_%04d%02d%02d_%02d%02d.txt"), FilePath_GetSystemPath(FILE_PATH_CRASH), OS_PATH_TEXT("sil0"), curtimeex.tm_year + 1900, curtimeex.tm_mon + 1, curtimeex.tm_mday, curtimeex.tm_hour, curtimeex.tm_min);

			//
			SysInfoLog_File fl;
			if (fl.Open(szSILFile))
			{
				SysInfo_ReportAll(&fl);
				fl.Close();
				if (!SendFileToBugDump(nBugID, szSILFile))
				{
					//
				}
			}
		}

		// Attach the log files		
		for (int i = 0; i < NUM_LOGS; i++)
		{
			if(i != MEM_TRACE_LOG)
			{
				LogFlush(i);
				const OS_PATH_CHAR * pszLogFilename = LogGetFilename(i);
				if ( ! pszLogFilename )
					continue;

				OS_PATH_CHAR szFilename[MAX_PATH];
				PStrPrintf(szFilename, MAX_PATH, OS_PATH_TEXT("%s"), pszLogFilename);
				if (FileExists(szFilename))
				{
					//AttachFileToBug(nBugID, LogGetFilename(i), "text/plain");
					if (!SendFileToBugDump(nBugID, szFilename))
					{
						// might want to put another message here.  Not doing it yet.
						//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
						return;
					}
				}
			}
		}
		
		// Attach memory trace files
		//		
		const OS_PATH_CHAR* logFilePath = FilePath_GetSystemPath(FILE_PATH_LOG);
		unsigned int resultsCount = 0;
		FILE_ITERATE_RESULT* results = NULL;
		FilesIterateInDirectory(logFilePath, L"*.xml", false, results, resultsCount);
		results = (FILE_ITERATE_RESULT* )MALLOC(CRASH_ALLOCATOR, resultsCount * sizeof(FILE_ITERATE_RESULT));
		FilesIterateInDirectory(logFilePath, L"*.xml", false, results, resultsCount);
		
		for (unsigned int i = 0; i < resultsCount; i++)
		{		
			OS_PATH_CHAR szFilename[MAX_PATH];
			PStrPrintf(szFilename, MAX_PATH, OS_PATH_TEXT("%S"), results[i].szFilenameRelativepath);		
			if (FileExists(szFilename))
			{		
				if (!SendFileToBugDump(nBugID, szFilename))
				{
					// might want to put another message here.  Not doing it yet.
					//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
					return;
				}
			}
		}
		FREE(CRASH_ALLOCATOR, results);		
		
		// write the .exe and .pdb files
#if 1	// CHB 2007.01.09 - For faster testing, set this to 0.
		if (OS_PATH_FUNC(GetModuleFileName)(NULL, szBuffer, MAX_PATH))
		{
			ASSERT(FileExists(szBuffer));
			SendFileToBugDump(nBugID, szBuffer);
			OS_PATH_CHAR szPDBFile[MAX_PATH];
			PStrReplaceExtension(szPDBFile, MAX_PATH, szBuffer, OS_PATH_TEXT("pdb"));
			if (FileExists(szPDBFile))
			{
				if (!SendFileToBugDump(nBugID, szPDBFile))
				{
					// might want to put another message here.  Not doing it yet.
					//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
					return;
				}
			}
		}
#endif
	}
	//SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GenerateErrorReport
//
// Parameters
//	pExInfo : A pointer to the exception info passed from the exception handler
//
// Remarks
//	This function shows the crash popup and allows the user to send an error
//	report
//
/////////////////////////////////////////////////////////////////////////////
void GenerateErrorReport(PEXCEPTION_POINTERS pExInfo, LPCTSTR szSummary /*= NULL*/,
						 int nSummaryBufferLength /*= INVALID_ID*/)
{
	// Display the error dialog to the user
	if (szSummary && nSummaryBufferLength > 0)
	{
		PStrCopy(g_Locals.szShortTitle, szSummary, MAX_SHORTTITLE_LENGTH);
	}
	else if(pExInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CODE_OUT_OF_MEMORY)
	{
		PStrCvt(g_Locals.szShortTitle, GlobalStringGetEx(GS_MSG_ERROR_NO_MEMORY_TEXT, 0), MAX_SHORTTITLE_LENGTH);	
	}
	else
	{
		*g_Locals.szShortTitle = 0; 
	}

	// Display the error dialog to the user
	g_Locals.m_eReportType = RT_UNHANDLED_ERROR;
	HINSTANCE hInstance = AppCommonGetHInstance();
	HWND hwndApp = AppCommonGetHWnd();

	if (hwndApp == NULL)
	{
		TraceError("Errhandler has no main HWND");
		PStrCopy(g_Locals.szShortTitle, "Exception on app exit", MAX_SHORTTITLE_LENGTH);
	}

	sbDialogActive = TRUE;
	int nResult = (int)DialogBox(hInstance, MAKEINTRESOURCE(IDD_CRASH_DIALOG), hwndApp, CrashDialogProc);
	sbDialogActive = FALSE;
	if (nResult == IDOK)
	{
		// Send the error
		TraceError("Sending error with exception pointers");
		 SendError(pExInfo);
	}
	else if (nResult == IDCANCEL)
	{
	}
	else
	{
		TraceError("Error Dialog unhandled result");
		DisplayLastWinError();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GenerateAssertReport(LPCTSTR szSummary,
						 int nSummaryBufferLength)
{
	// Display the error dialog to the user
	if (szSummary && nSummaryBufferLength > 0)
	{
		PStrCopy(g_Locals.szShortTitle, szSummary, MAX_SHORTTITLE_LENGTH);
		// convert LF to CRLF for windows edit dialog
		PStrCvtToCRLF( g_Locals.szShortTitle, MAX_SHORTTITLE_LENGTH );
	}
	else
	{
		*g_Locals.szShortTitle = 0; 
	}

	// Display the error dialog to the user
	g_Locals.m_eReportType = RT_ASSERT;
	HINSTANCE hInstance = AppCommonGetHInstance();
	sbDialogActive = TRUE;
	int nResult = (int)DialogBox(hInstance, MAKEINTRESOURCE(IDD_CRASH_DIALOG), AppCommonGetHWnd(), CrashDialogProc);
	sbDialogActive = FALSE;
	if (nResult == IDABORT || nResult == IDRETRY || nResult == IDIGNORE || nResult == IDIGNOREALWAYS)
	{
		// Dialog handles most of it.  Return the result
		return nResult;
	}
	if(nResult == -1 && (GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND) ||
		(GetLastError() == ERROR_RESOURCE_DATA_NOT_FOUND))
	{
		return IDRETRY;

	}
	else 
	{

		DisplayLastWinError();
	}
	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GenerateUserBugReport(LPCTSTR szSummary, int nSummaryBufferLength, const OS_PATH_CHAR* szScreenshotFile)
{
	// Display the error dialog to the user
	if (szSummary && nSummaryBufferLength > 0)
	{
		PStrCopy(g_Locals.szShortTitle, szSummary, nSummaryBufferLength);
	}
	else
	{
		*g_Locals.szShortTitle = 0; 
	}

	g_Locals.m_eReportType = RT_USER_REPORT;
	HINSTANCE hInstance = AppCommonGetHInstance();
	sbDialogActive = TRUE;
	int nResult = (int)DialogBox(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDD_CRASH_DIALOG), AppCommonGetHWnd(), (DLGPROC)CrashDialogProc);
	sbDialogActive = FALSE;

	if (nResult == IDOK)
	{
		// Send the error
		ULONG nBugID = InsertBugzillaRecord();
		if (nBugID && szScreenshotFile)
		{
			AttachFileToBug(nBugID, szScreenshotFile, "image/jpeg");
		}
		return nBugID;
	}
	else if (nResult == IDCANCEL)
	{
		return 0;
	}
	else
	{
		DisplayLastWinError();
	}

	return 0;
}
#endif //  !ISVERSION(RETAIL_VERSION)
//
/*
 * DO NOT ASSSERT in these function. Or Log. These are called during the minidump process, 
 * so we may be out of memory.
 */


static char *sCommonGetBase64StringFromFile(const OS_PATH_CHAR *szFile,
                                            DWORD *fileSize,
                                            BOOL bEncode,
                                            BOOL bCompress)
{
    BOOL bRet = TRUE;
    char *szUnCompressed = NULL, *szCompressed = NULL,*szRet = NULL;
    DWORD dwSize = 0;
    DWORD dwCompressedSize = 0;
    DWORD bread=0;
    DWORD dw64Len= 0;

    HANDLE hFile = OS_PATH_FUNC(CreateFile)(szFile,
                                            GENERIC_READ,
                                            FILE_SHARE_READ,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);

    ASSERT_GOTO(hFile != INVALID_HANDLE_VALUE,Error);

    dwSize = GetFileSize(hFile,NULL);

    ASSERT_GOTO(dwSize != INVALID_FILE_SIZE,Error);

    szUnCompressed = (char*)MALLOCZ(CRASH_ALLOCATOR,dwSize);

    ASSERT_GOTO(ReadFile(hFile,szUnCompressed,dwSize,&bread,NULL),Error);

    ASSERT_GOTO(bread == dwSize,Error);

    if(bCompress)
    {
        dwCompressedSize = compressBound(dwSize);

        szCompressed = (char*)MALLOCZ(CRASH_ALLOCATOR,dwCompressedSize);

        ASSERT_GOTO(compress((Bytef*)szCompressed,&dwCompressedSize,(Bytef*)szUnCompressed,dwSize) == Z_OK,Error);

        trace("Minidump of size %d compressed to %d\n",dwSize,dwCompressedSize);

        FREE(CRASH_ALLOCATOR,szUnCompressed);
        szUnCompressed = NULL;
    }
    else
    {
        dwCompressedSize = dwSize;
        szCompressed = szUnCompressed;
        szUnCompressed = NULL;
    }

    if(bEncode)
    {
        dw64Len = Base64EncodeGetRequiredLength(dwCompressedSize, ATL_BASE64_FLAG_NOCRLF);

        ASSERT_GOTO(dw64Len,Error);

        szRet = (char*)MALLOCZ(CRASH_ALLOCATOR,dw64Len +1);

        ASSERT_GOTO(szRet,Error);

        ASSERT_GOTO(Base64Encode((BYTE*)szCompressed,
            dwCompressedSize,
            (LPSTR)szRet,
            (int*)&dw64Len,
            ATL_BASE64_FLAG_NOCRLF),Error);

        FREE(CRASH_ALLOCATOR,szCompressed);
        szCompressed = NULL;
    }
    else
    {
        dw64Len = dwCompressedSize;
        szRet = szCompressed;
        szCompressed = NULL;
    }
    *fileSize = dw64Len;
Error:
    CloseHandle(hFile);

    if(!bRet)
    {
        if(szRet)
        {
            FREE(CRASH_ALLOCATOR,szRet);
            szRet = NULL;
        }
        if(szUnCompressed)
        {
            FREE(CRASH_ALLOCATOR,szUnCompressed);
        }
        if(szCompressed)
        {
            FREE(CRASH_ALLOCATOR,szCompressed);
        }
    }
    return szRet;
}
static void sCommonFreeFileString(char *sz)
{
    FREE(CRASH_ALLOCATOR,sz);
}
static char* sCommonGetSysInfoString(__in __out DWORD *pdwSize)
{
    OS_PATH_CHAR pathBuf[MAX_PATH+1];
    OS_PATH_CHAR fileNameBuf[MAX_PATH+1];
    DWORD dwRet = 0;
    char *strRet = NULL;

    dwRet = OS_PATH_FUNC(GetTempPath)(ARRAYSIZE(pathBuf),pathBuf);

    if(dwRet == 0 || dwRet > ARRAYSIZE(pathBuf))
    {
        return NULL;
    }
    if(!OS_PATH_FUNC(GetTempFileName)(pathBuf,OS_PATH_TEXT("GFX"),0,fileNameBuf)) 
    {
        return NULL;
    }
    {
        SysInfoLog_File fl;
        if (fl.Open(fileNameBuf))
        {
            SysInfo_ReportAll(&fl);
            fl.Close();
        }
        strRet = sCommonGetBase64StringFromFile(fileNameBuf,pdwSize,TRUE,FALSE);
        OS_PATH_FUNC(DeleteFile)(fileNameBuf);
    }
    return strRet;

}
static void sCommonFreeSysInfoString(char *str)
{
    sCommonFreeFileString(str);
}
// XML upload spec. All data is string (or base-64-encoded string).
#define szUploadXMLFmtString   "<crashdump> "\
                                        "<username>%s</username>" \
                                        "<passhash>%s</passhash>" \
                                        "<dumpfile>%s</dumpfile>"\
                                        "<title>%s</title>"\
                                        "<description>%s</description>"\
                                        "<screenshot>%s</screenshot>"\
                                        "<sysinfo>%s</sysinfo>"\
                                        "</crashdump>"

#define FORMAT_UPLOAD_STRING(buffer,bufferLen,un,pw,df,title,desc,ss,si) \
                                                            PStrPrintf((buffer), \
                                                                       (bufferLen), \
                                                                       szUploadXMLFmtString,\
                                                                       (un),\
                                                                       (pw),\
                                                                       (df),\
                                                                       (title),\
                                                                       (desc),\
                                                                       (ss),(si))
                            
static const char* HELLGATE_UPLOAD_URL = "/upload_dump/upload_dump.aspx?product=hellgate";
static const char* TUGBOAT_UPLOAD_URL = "/upload_dump/upload_dump.aspx?product=tugboat";
static const char* UPLOAD_CONTENT_TYPE   = "Content-Type: application/octet-stream\r\n";

static void sUploadCallback(INET_CONTEXT *pIC,
                                    HTTP_CONNECTION*pConn,
                                    void *context,
                                    DWORD dwError,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
    UNREFERENCED_PARAMETER(pIC);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(dwBufSize);
    if(dwError != 200)
    {
        //HandleInternalError("upload dump: failed : Error code %d\n",dwError);
    }
    if(pConn)
    {
        NetHttpCloseClient(pConn);
    }
    if(context)
    {
        SetEvent((HANDLE)context);
    }
}
#ifndef CBR
#define CBR(a) ASSERT_GOTO((a),Error)
#endif //CBR
BOOL sCommonUploadCrashDump(const char *szDumpServer,
                            const OS_PATH_CHAR *szDumpFile,
                            const char *szUserName,
                            const char *szTitle,
                            const char *szDescription,
                            const OS_PATH_CHAR *szScreenShotFile)
{
    BOOL bRet = TRUE;
    http_client_config config = {0};
    DWORD dwPostLen = 0,dwDumpSize = 0;
    char *szDumpString = NULL;
    char *szScreenShotString = NULL;
    char *szPostString = NULL;
    char *szSysInfoString = NULL;
    HANDLE hEvent = NULL;

    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);


    config.serverAddr = (char*)szDumpServer;
    config.fp_readCallback = sUploadCallback;
    config.pCallbackContext = hEvent;

    HTTP_CONNECTION *pConn = NetHttpCreateClient(AppCommonGetHttp(),config);

    CBR(hEvent);
    CBR(pConn);
    CBR(szDumpServer);
    CBR(szDescription);
    CBR(szUserName);

    dwPostLen =  PStrLen(szUserName) +  PStrLen(szDescription) +
                 PStrLen(szTitle);

    dwPostLen += (sizeof(szUploadXMLFmtString)-1);
                    

    if(szDumpFile)
    {
        szDumpString = sCommonGetBase64StringFromFile(szDumpFile,&dwDumpSize,TRUE,TRUE);

        CBR(szDumpString);

        dwPostLen += dwDumpSize+1;
    }
    else
    {
        dwPostLen += sizeof("none");
    }
    if(szScreenShotFile)
    {
        dwDumpSize = 0;
        // encode in base64, but don't try to compress
        szScreenShotString = sCommonGetBase64StringFromFile(szScreenShotFile,&dwDumpSize,TRUE,FALSE);

        CBR(szScreenShotString);

        dwPostLen += dwDumpSize+1;
    }
    else
    {
        dwPostLen += sizeof("none");
    }
    dwDumpSize = 0;
    szSysInfoString = sCommonGetSysInfoString(&dwDumpSize);
    if(szSysInfoString)
    {
        dwPostLen += dwDumpSize;
    }
    else
    {
        dwPostLen += sizeof("none");
    }
    
	szPostString = (char*)MALLOCZ(CRASH_ALLOCATOR, dwPostLen);    

    CBR(szPostString);

    FORMAT_UPLOAD_STRING(szPostString, dwPostLen,szUserName,"",
                         szDumpString?szDumpString:"",
                         szTitle?szTitle:"No Title",
                         szDescription?szDescription:"none",
                         szScreenShotString?szScreenShotString:"",
                         szSysInfoString?szSysInfoString:"");

    if(AppIsHellgate()) 
    {
        CBR(NetHttpSendRequest(pConn,
                                HELLGATE_UPLOAD_URL,
                                NET_HTTP_VERB_POST,
                                0,
                                UPLOAD_CONTENT_TYPE,
                                (BYTE*)szPostString,
                                (DWORD)PStrLen(szPostString)));
    }
    else if (AppIsTugboat())
    {
        CBR(NetHttpSendRequest(pConn,
                                TUGBOAT_UPLOAD_URL,
                                NET_HTTP_VERB_POST,
                                0,
                                UPLOAD_CONTENT_TYPE,
                                (BYTE*)szPostString,
                                (DWORD)PStrLen(szPostString)));
    }
                                                    
    WaitForSingleObject(hEvent,2*1000); //wait 1 secs.

    TraceError("Successfully uploaded mini-dump.");

Error:
    if(!bRet)
    {
        if(pConn)
        {
            NetHttpCloseClient(pConn);
        }
        trace("upload dump failed\n");
    }
    if(szPostString)
    {
        SecureZeroMemory(szPostString,dwPostLen);
        FREE(CRASH_ALLOCATOR,szPostString);
    }
    if(szDumpString)
    {
        sCommonFreeFileString(szDumpString);
    }
    if(szScreenShotString)
    {
        sCommonFreeFileString(szScreenShotString);
    }
    if(szSysInfoString)
    {
        sCommonFreeSysInfoString(szSysInfoString);
    }
    if(hEvent)
    {
        CloseHandle(hEvent);
    }
    return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG AddBugReport(
	const char *szShortTitle,
	const char *szDescription,
	const char *szEmail,
	const OS_PATH_CHAR* szScreenshotFile)
{
#if !ISVERSION(RETAIL_VERSION)
	PStrCopy(g_Locals.szShortTitle, szShortTitle, MAX_SHORTTITLE_LENGTH);
	PStrCopy(g_Locals.szDescription, szDescription, MAX_DESCRIPT_LENGTH);
	PStrCopy(g_Locals.szEmail, szEmail, arrsize(g_Locals.szEmail));

	g_Locals.nBugzillaUserID = GetBugzillaUserID(g_Locals.szEmail);
	DisconnectFromBugzillaDB();

	if (g_Locals.nBugzillaUserID == 0)
	{
		return (ULONG)-1;
	}

	// Send the error
	ULONG nBugID = InsertBugzillaRecord();
	if (nBugID && szScreenshotFile)
	{
		AttachFileToBug(nBugID, szScreenshotFile, "image/jpeg");
	}
	return nBugID;
#else
#if ISVERSION(CLIENT_ONLY_VERSION)
    // treating BOOL as int here. ugly.
    return sCommonUploadCrashDump(AppGetAuthenticationIp(),
                           NULL,
                           szEmail,
                           szShortTitle,
                           szDescription,
                           szScreenshotFile);
#else //!CLIENT_ONLY
	UNREFERENCED_PARAMETER(szScreenshotFile);
	UNREFERENCED_PARAMETER(szEmail);
	UNREFERENCED_PARAMETER(szDescription);
	UNREFERENCED_PARAMETER(szShortTitle);
    return TRUE;
#endif //!CLIENT_ONLY

#endif //RETAIL_VERSION
}

#endif // !ISVERSION(SERVER_VERSION)
