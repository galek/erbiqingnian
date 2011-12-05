//----------------------------------------------------------------------------
// log.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------


#if !ISVERSION(SERVER_VERSION)
#include "filepaths.h"
#include "definition_common.h"
#include "appcommon.h"
#include "config.h"
#include "version.h"

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
using namespace FSCommon;
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define LOG_LINE_MAX_LEN		4096						// maximum length for a single line in the log file
#define DEFAULT_LOG_BUFSIZE		8192						// default buffer size
#define DEFAULT_CONSOLE_COLOR	0xffffffff					// default color to output text to console
#define LOGFILE_EXPIRE_TIME		(10000000i64 * 60i64 * 60i64 * 24i64 * 7i64)	// keep logs for 7 days
#define LOG_ASYNC_PRIORITY		ASYNC_PRIORITY_3

enum
{
	LOG_MASK_BUFFER =			MAKE_MASK(0),				// buffer writes
	LOG_MASK_ALWAYS_LOG =		MAKE_MASK(1),				// ignore GLOBAL_FLAG_FULL_LOGGING flag
	LOG_MASK_WRITE_LOG =		MAKE_MASK(2),				// write to log file
	LOG_MASK_WRITE_TRACE =		MAKE_MASK(3),				// write to trace window  (DEVELOPMENT only)
	LOG_MASK_WRITE_CONSOLE =	MAKE_MASK(4),				// write to console
	LOG_MASK_NO_FLUFF =			MAKE_MASK(5),				// only write text passed to the log functions, no timestamp, header, etc.
	LOG_MASK_PER_FLUSH =		MAKE_MASK(6),				// specifies that a new log file will be created each time a flush occurs.
};


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// we keep different log files for different types of logging
//----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4324)								// disable warning for padding added to end of structure declared with __declspec(align)
__declspec(align(64)) struct LOG_DESC
{
	long						m_Initialized;				// has this file ever been initialized
	long						m_WriteLog;
	long						m_WriteTrace;
	long						m_WriteConsole;
	long						m_NoFluff;	
	long						m_PerFlush;
	CCriticalSection			m_cs;						// critical section
	DWORD						m_flags;
	unsigned int				m_LogId;					// log enum (ERROR_LOG, SEND_LOG, etc)
	char						m_strFilePrefix[32];		// file name prefix i.e. "ERR" if log file will be named "ERR20060630.log"
	char						m_strFileExtension[32];		// file extension i.e. "log" if log file will be named "ERR20060630.log"
	SYSTEMTIME					m_Time;						// time used for file name
	FILETIME					m_FileTime;
	ASYNC_FILE *				m_File;						// async file pointer
	char *						m_Buffer;					// buffer for log messages
	unsigned int				m_BufferSize;				// size of buffer
	unsigned int				m_BufferCur;				// cursor into buffer
	UINT64						m_FileOffset;				// offset into file

	DWORD						m_ConsoleColor;				// color for console messages
};
#pragma warning(pop)


//----------------------------------------------------------------------------
// LOG system
//----------------------------------------------------------------------------
struct LOG_SYSTEM
{
	LOG_DESC					m_Logs[NUM_LOGS];
	OS_PATH_CHAR				m_strRootPath[MAX_PATH];
	BOOL						m_bFullLogging;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
LOG_SYSTEM g_Log;


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLogGetGlobalFlag(
	void)
{
	const GLOBAL_DEFINITION * globalDef = DefinitionGetGlobal();
	if (globalDef)
	{
		return TEST_MASK(globalDef->dwFlags, GLOBAL_FLAG_FULL_LOGGING);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UINT64 sGetFileTimeAsInt64(
	FILETIME & filetime)
{
	ULARGE_INTEGER time64;
	MemoryCopy(&time64, sizeof(ULARGE_INTEGER), &filetime, sizeof(ULARGE_INTEGER));
	return (UINT64)time64.QuadPart;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogGetDay(
	LOG_DESC & log)
{
	GetLocalTime(&log.m_Time);
	SystemTimeToFileTime(&log.m_Time, &log.m_FileTime);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogGetFileName(
	LOG_DESC & log,
	OS_PATH_CHAR * filename,
	unsigned int len,
	const char * prefix,
	const char * extension)
{
	OS_PATH_CHAR convertedPrefix[_countof(g_Log.m_Logs[0].m_strFilePrefix)];
	PStrCvt(convertedPrefix, prefix, _countof(convertedPrefix));

	OS_PATH_CHAR convertedExtension[_countof(g_Log.m_Logs[0].m_strFileExtension)];
	PStrCvt(convertedExtension, extension, _countof(convertedExtension));	
	
	if(log.m_PerFlush)
	{
		PStrPrintf(filename, len, OS_PATH_TEXT("%s%s%04d%02d%02d_%010d.%s"), g_Log.m_strRootPath, convertedPrefix, log.m_Time.wYear, log.m_Time.wMonth, log.m_Time.wDay, log.m_PerFlush, convertedExtension);
	}
	else
	{
		PStrPrintf(filename, len, OS_PATH_TEXT("%s%s%04d%02d%02d.%s"), g_Log.m_strRootPath, convertedPrefix, log.m_Time.wYear, log.m_Time.wMonth, log.m_Time.wDay, convertedExtension);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogWelcome(
	unsigned int idLog)
{
	ASSERT_RETURN(idLog < NUM_LOGS);
	LOG_DESC & log = g_Log.m_Logs[idLog];

	ASSERT_RETURN(log.m_File);
	LogMessage(idLog, "%s Log Initialized -- Build Version [%I64x]", log.m_strFilePrefix, VERSION_AS_ULONGLONG);
}


//----------------------------------------------------------------------------
// only gets called on app init
//----------------------------------------------------------------------------
static void sLogInitStatic(
	unsigned int idLog,
	const char * filePrefix,
	const char * fileExtension,
	DWORD flags,
	unsigned int bufsize,
	DWORD consoleColor)
{
	ASSERT_RETURN(idLog < NUM_LOGS);
	ASSERT_RETURN(filePrefix && filePrefix[0]);

	LOG_DESC & log = g_Log.m_Logs[idLog];
	structclear(log);

	log.m_cs.Init();

	log.m_LogId = idLog;
	log.m_flags = flags;
	if (TEST_MASK(flags, LOG_MASK_ALWAYS_LOG))
	{
		SET_MASK(flags, LOG_MASK_WRITE_LOG);
		log.m_WriteLog = TRUE;
	}
	else if (g_Log.m_bFullLogging && TEST_MASK(flags, LOG_MASK_WRITE_LOG))
	{
		log.m_WriteLog = TRUE;
	}
	if (TEST_MASK(flags, LOG_MASK_WRITE_TRACE))
	{
		log.m_WriteTrace = TRUE;
	}
	if (TEST_MASK(flags, LOG_MASK_WRITE_CONSOLE))
	{
		log.m_WriteConsole = TRUE;
	}
	if (TEST_MASK(flags, LOG_MASK_NO_FLUFF))
	{
		log.m_NoFluff = TRUE;
	}
	if (TEST_MASK(flags, LOG_MASK_PER_FLUSH))
	{
		log.m_PerFlush = TRUE;
	}		

	log.m_ConsoleColor = consoleColor;

	if (TEST_MASK(flags, LOG_MASK_BUFFER))
	{
		log.m_BufferSize = bufsize > LOG_LINE_MAX_LEN ? bufsize : DEFAULT_LOG_BUFSIZE;
	}

	PStrCopy(log.m_strFilePrefix, filePrefix, arrsize(log.m_strFilePrefix));
	PStrCopy(log.m_strFileExtension, fileExtension, arrsize(log.m_strFileExtension));	
}


//----------------------------------------------------------------------------
// gets called on app init & when log day change occurs
//----------------------------------------------------------------------------
static void sLogInitDynamic(
	unsigned int idLog)
{
	ASSERT_RETURN(idLog < NUM_LOGS);

	LOG_DESC & log = g_Log.m_Logs[idLog];

	CSAutoLock lock(&log.m_cs);

	sLogGetDay(log);

	if (log.m_BufferSize)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		log.m_Buffer = (char *)MALLOC(g_StaticAllocator, log.m_BufferSize);
#else
		log.m_Buffer = (char *)MALLOC(NULL, log.m_BufferSize);
#endif			
	}

	OS_PATH_CHAR filename[MAX_PATH];
	sLogGetFileName(log, filename, arrsize(filename), log.m_strFilePrefix, log.m_strFileExtension);

	DWORD flags = FF_WRITE | FF_SHARE_READ | FF_SHARE_WRITE | FF_SEQUENTIAL_SCAN;
	if(g_Log.m_Logs[idLog].m_PerFlush)
	{
		flags |= FF_CREATE_ALWAYS;
	}
	else
	{
		flags |= FF_OPEN_ALWAYS;
	}


	log.m_File = AsyncFileOpen(filename, flags);
	ASSERT_RETURN(log.m_File != NULL);

	log.m_FileOffset = AsyncFileGetSize(log.m_File);

	log.m_Initialized = TRUE;

	if(log.m_NoFluff == FALSE)
	{
		sLogWelcome(idLog);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogInit(
	unsigned int idLog,
	const char * filePrefix,
	const char * fileExtension,
	DWORD flags,
	unsigned int bufsize,
	DWORD consoleColor)
{
	sLogInitStatic(idLog, filePrefix, fileExtension, flags, bufsize, consoleColor);
	sLogInitDynamic(idLog);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogInitAll(
	void)
{
	#define LOG_DECL(id, prefix, extension, flags, bufsize, color)		sLogInit(id, prefix, extension, flags, bufsize, color)

	LOG_DECL(LOG_FILE,			"LOG", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(ERROR_LOG,			"ERR", "log", LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 0, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(NETWORK_LOG,		"NET", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(SEND_LOG,			"SEND", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(RECV_LOG,			"RECV", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(PERF_LOG,			"PERF", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(COMBAT_LOG,		"COMBAT", "log", LOG_MASK_BUFFER, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(INVENTORY_LOG,		"INV", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_TRACE | LOG_MASK_ALWAYS_LOG, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(AI_LOG,			"AI", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_LOG, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(TIMER_LOG,			"TIMER", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_LOG, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(PERSONAL_LOG,		"PERSONAL", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(EXCEL_LOG,			"EXCEL", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(TREASURE_LOG,		"TREASURE", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(SOUND_LOG,			"SOUND", "log", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 8192, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(NODIRECT_LOG,		"NODIRECT", "log", LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 0, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(DOWNLOAD_LOG,		"DOWNLOAD", "log", LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 0, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(DEFINITION_LOG,	"DEFINITION", "log", LOG_MASK_BUFFER | LOG_MASK_WRITE_LOG | LOG_MASK_ALWAYS_LOG, 16384, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(DATAWARNING_LOG,	"DATAWARNING", "log", LOG_MASK_WRITE_LOG | LOG_MASK_ALWAYS_LOG, 0, DEFAULT_CONSOLE_COLOR);
	LOG_DECL(MEM_TRACE_LOG,		"MEMTRACE", "xml", LOG_MASK_BUFFER | LOG_MASK_ALWAYS_LOG | LOG_MASK_NO_FLUFF | LOG_MASK_PER_FLUSH, 0, DEFAULT_CONSOLE_COLOR);	
	LOG_DECL(SKU_LOG,			"SKU", "log", LOG_MASK_ALWAYS_LOG | LOG_MASK_WRITE_TRACE, 0, DEFAULT_CONSOLE_COLOR);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogInitCreateDirectory(
	void)
{
	const OS_PATH_CHAR * pLogPath = FilePath_GetSystemPath(FILE_PATH_LOG);
	if ( pLogPath[0] && pLogPath[1] == L':')
	{
		PStrCopy(g_Log.m_strRootPath, pLogPath, arrsize(g_Log.m_strRootPath));
	}
	else
	{
		int len;
		const OS_PATH_CHAR * strRoot = AppCommonGetRootDirectory(&len);
		PStrCopy(g_Log.m_strRootPath, arrsize(g_Log.m_strRootPath), strRoot, len);
		PStrForceBackslash(g_Log.m_strRootPath, arrsize(g_Log.m_strRootPath));
		PStrCat(g_Log.m_strRootPath, pLogPath, arrsize(g_Log.m_strRootPath));
	}

	FileCreateDirectory(g_Log.m_strRootPath);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogDeleteOld(
	unsigned int id)
{
	if (g_Log.m_Logs[id].m_strFilePrefix[0] == 0)
	{
		return;
	}
	
	OS_PATH_CHAR szPrefix[_countof(g_Log.m_Logs[id].m_strFilePrefix)];
	PStrCvt(szPrefix, g_Log.m_Logs[id].m_strFilePrefix, _countof(szPrefix));

	OS_PATH_CHAR szExtension[_countof(g_Log.m_Logs[id].m_strFileExtension)];
	PStrCvt(szExtension, g_Log.m_Logs[id].m_strFileExtension, _countof(szExtension));	
	
	OS_PATH_CHAR filespec[MAX_PATH];
	PStrPrintf(filespec, arrsize(filespec), OS_PATH_TEXT("%s%s*.%s"), g_Log.m_strRootPath, szPrefix, szExtension);

	UINT64 deletetime = sGetFileTimeAsInt64(g_Log.m_Logs[id].m_FileTime) - LOGFILE_EXPIRE_TIME;

	OS_PATH_FUNC(WIN32_FIND_DATA) findData;
	FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)(filespec, &findData);
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (findData.cFileName[0] && findData.cFileName[0] != '.' && findData.cFileName[1] != '.')
			{
				UINT64 filetime = sGetFileTimeAsInt64(findData.ftLastWriteTime);
				if (filetime < deletetime || g_Log.m_Logs[id].m_PerFlush)
				{
					OS_PATH_CHAR filename[MAX_PATH];
					ASSERT_BREAK(PStrPrintf(filename, arrsize(filename), OS_PATH_TEXT("%s%s"), g_Log.m_strRootPath, findData.cFileName) > 0);
					FileDelete(filename);
				}
			}
		} while (OS_PATH_FUNC(FindNextFile)(handle, &findData));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogDeleteAllOld(
	void)
{
	for (int ii = 0; ii < NUM_LOGS; ++ii)
	{
		sLogDeleteOld(ii);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogInit(
	void)
{
	structclear(g_Log);

	sLogInitCreateDirectory();
	sLogInitAll();
	sLogDeleteAllOld();
}


//----------------------------------------------------------------------------
// only do this after all non-main logging threads have closed
//----------------------------------------------------------------------------
void LogFree(
	void)
{
	for (unsigned int ii = 0; ii < NUM_LOGS; ++ii)
	{
		if (!g_Log.m_Logs[ii].m_Initialized)
		{
			continue;
		}
		g_Log.m_Logs[ii].m_Initialized = FALSE;
		if (g_Log.m_Logs[ii].m_File)
		{
			AsyncFileClose(g_Log.m_Logs[ii].m_File);
		}
		if (g_Log.m_Logs[ii].m_Buffer)
		{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			FREE(g_StaticAllocator, g_Log.m_Logs[ii].m_Buffer);
#else
			FREE(NULL, g_Log.m_Logs[ii].m_Buffer);
#endif				
		}
		g_Log.m_Logs[ii].m_cs.Free();
		memclear(g_Log.m_Logs + ii, sizeof(LOG_FILE));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogCheckEnabled(
	void)
{
	g_Log.m_bFullLogging = sLogGetGlobalFlag();

	for (int ii = 0; ii < NUM_LOGS; ++ii)
	{
		if (g_Log.m_Logs[ii].m_Initialized)
		{
			if (!g_Log.m_bFullLogging && !TEST_MASK(g_Log.m_Logs[ii].m_flags, LOG_MASK_ALWAYS_LOG))
			{
				g_Log.m_Logs[ii].m_WriteLog = FALSE;
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogChangeDay(
	unsigned int log)
{
	ASSERT_RETURN(log < NUM_LOGS);

	ASYNC_FILE * oldfile = NULL;
	void * oldbuffer = NULL;
	unsigned int oldbuflen = 0;
	UINT64 oldfileoffset = 0;

	{
		CSAutoLock lock(&g_Log.m_Logs[log].m_cs);

		// exit if log has already been changed to new day

		// record old log info for write outside of lock
		if (g_Log.m_Logs[log].m_File)
		{
			oldfile = g_Log.m_Logs[log].m_File;
			g_Log.m_Logs[log].m_File = NULL;
			oldbuffer = g_Log.m_Logs[log].m_Buffer;
			g_Log.m_Logs[log].m_Buffer = NULL;
			oldbuflen = g_Log.m_Logs[log].m_BufferCur;
			g_Log.m_Logs[log].m_BufferCur = 0;
			oldfileoffset = g_Log.m_Logs[log].m_FileOffset;
			g_Log.m_Logs[log].m_FileOffset += oldbuflen;
		}

		// create new log file
	}

	// write last of data into old log & close it
	if (oldfile)
	{
		if (oldbuffer && oldbuflen > 0)
		{
			ASYNC_DATA asyncData(oldfile, oldbuffer, oldfileoffset, oldbuflen, ADF_FREEBUFFER);
			AsyncFileWrite(asyncData, LOG_ASYNC_PRIORITY, 0);	
		}
		AsyncFileClose(oldfile);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogFlush(unsigned int log)
{
	if(log < NUM_LOGS && g_Log.m_Logs[log].m_Buffer && g_Log.m_Logs[log].m_File && g_Log.m_Logs[log].m_BufferCur)
	{
		CSAutoLock lock(&g_Log.m_Logs[log].m_cs);
			
		unsigned int buflen = g_Log.m_Logs[log].m_BufferCur;

#if USE_MEMORY_MANAGER
		void * buffer = MALLOC(IMemoryManager::GetInstance().GetSystemAllocator(), buflen);
#else
		void * buffer = MALLOC(NULL, buflen);
#endif					
		
		MemoryCopy(buffer, buflen, g_Log.m_Logs[log].m_Buffer, buflen);
		UINT64 fileoffset = g_Log.m_Logs[log].m_FileOffset;
		g_Log.m_Logs[log].m_FileOffset += buflen;
		g_Log.m_Logs[log].m_BufferCur = 0;
			
		ASYNC_DATA asyncData(g_Log.m_Logs[log].m_File, buffer, fileoffset, buflen, ADF_FREEBUFFER);

#if USE_MEMORY_MANAGER
		asyncData.m_Pool = IMemoryManager::GetInstance().GetSystemAllocator();
#endif					
		
		AsyncFileWrite(asyncData, LOG_ASYNC_PRIORITY, 0);
		
		// Open a new file is "per flush" is specified for the log
		//
		if(g_Log.m_Logs[log].m_PerFlush)
		{
			++g_Log.m_Logs[log].m_PerFlush;
			
			// Close the old file
			//
			AsyncFileClose(g_Log.m_Logs[log].m_File);			
			
			OS_PATH_CHAR filename[MAX_PATH];
			sLogGetFileName(g_Log.m_Logs[log], filename, arrsize(filename), g_Log.m_Logs[log].m_strFilePrefix, g_Log.m_Logs[log].m_strFileExtension);

			DWORD flags = FF_WRITE | FF_SHARE_READ | FF_SHARE_WRITE | FF_SEQUENTIAL_SCAN | FF_CREATE_ALWAYS;
			g_Log.m_Logs[log].m_File = AsyncFileOpen(filename, flags);
			ASSERT_RETURN(g_Log.m_Logs[log].m_File != NULL);

			g_Log.m_Logs[log].m_FileOffset = 0;

			if(g_Log.m_Logs[log].m_NoFluff == FALSE)
			{
				sLogWelcome(log);
			}						
		}				
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogSetLogging(
	unsigned int log,
	BOOL value)
{
	ASSERT_RETURN(log < NUM_LOGS);
	g_Log.m_Logs[log].m_WriteLog = value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LogGetLogging(
	unsigned int log)
{
	ASSERT_RETFALSE(log < NUM_LOGS);
	return g_Log.m_Logs[log].m_WriteLog;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogSetTrace(
	unsigned int log,
	BOOL value)
{
	ASSERT_RETURN(log < NUM_LOGS);
	g_Log.m_Logs[log].m_WriteTrace = value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LogGetTrace(
	unsigned int log)
{
	ASSERT_RETFALSE(log < NUM_LOGS);
	return g_Log.m_Logs[log].m_WriteTrace;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogSetConsole(
	unsigned int log,
	BOOL value,
	DWORD color)
{
	ASSERT_RETURN(log < NUM_LOGS);
	g_Log.m_Logs[log].m_WriteConsole = value;
	g_Log.m_Logs[log].m_ConsoleColor = color;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LogGetConsole(
	unsigned int log)
{
	ASSERT_RETFALSE(log < NUM_LOGS);
	return g_Log.m_Logs[log].m_WriteConsole;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLogSetup(
	unsigned int log)
{
	if (log >= NUM_LOGS)
	{
		return FALSE;
	}
	if (!g_Log.m_Logs[log].m_Initialized)
	{
		return FALSE;
	}

	SYSTEMTIME time;
	GetLocalTime(&time);
	if (time.wDay != g_Log.m_Logs[log].m_Time.wDay)
	{
		DBG_VERSION_ONLY(OutputDebugString("$"));
		return TRUE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// doesn't write time, doesn't add newline
//----------------------------------------------------------------------------
static void sLogText(
	unsigned int log,
	const char * str,
	unsigned int len)
{
	ASSERT_RETURN(log < NUM_LOGS);
	ASSERT_RETURN(len > 0 && len < LOG_LINE_MAX_LEN);

	if (!sLogSetup(log))
	{
		trace1(str);
		return;
	}

	if (g_Log.m_Logs[log].m_WriteLog)
	{
		void * buffer = 0;
		unsigned int buflen = 0;
		UINT64 fileoffset = 0;

		ONCE
		{
			CSAutoLock lock(&g_Log.m_Logs[log].m_cs);

			if (!g_Log.m_Logs[log].m_File)
			{
				break;
			}
			if (g_Log.m_Logs[log].m_Buffer)
			{
				if (g_Log.m_Logs[log].m_BufferCur + len > g_Log.m_Logs[log].m_BufferSize)
				{
					buflen = g_Log.m_Logs[log].m_BufferCur;

#if USE_MEMORY_MANAGER
					buffer = MALLOC(IMemoryManager::GetInstance().GetSystemAllocator(), buflen);
#else
					buffer = MALLOC(NULL, buflen);
#endif					
					
					MemoryCopy(buffer, buflen, g_Log.m_Logs[log].m_Buffer, buflen);
					fileoffset = g_Log.m_Logs[log].m_FileOffset;
					g_Log.m_Logs[log].m_FileOffset += buflen;
					g_Log.m_Logs[log].m_BufferCur = 0;
				}
				MemoryCopy(g_Log.m_Logs[log].m_Buffer + g_Log.m_Logs[log].m_BufferCur, g_Log.m_Logs[log].m_BufferSize - g_Log.m_Logs[log].m_BufferCur, str, len);
				g_Log.m_Logs[log].m_BufferCur += len;
			}
			else
			{
				buflen = len;

#if USE_MEMORY_MANAGER
				buffer = MALLOC(IMemoryManager::GetInstance().GetSystemAllocator(), buflen);
#else
				buffer = MALLOC(NULL, buflen);
#endif					
				
				MemoryCopy(buffer, buflen, str, buflen);
				fileoffset = g_Log.m_Logs[log].m_FileOffset;
				g_Log.m_Logs[log].m_FileOffset += buflen;
			}
		}

		if (buffer && buflen > 0)
		{
			ASYNC_DATA asyncData(g_Log.m_Logs[log].m_File, buffer, fileoffset, buflen, ADF_FREEBUFFER);

#if USE_MEMORY_MANAGER
			asyncData.m_Pool = IMemoryManager::GetInstance().GetSystemAllocator();
#endif					
			
			AsyncFileWrite(asyncData, LOG_ASYNC_PRIORITY, 0);
		}
	}

	if (g_Log.m_Logs[log].m_WriteTrace)
	{
		trace1(str);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogTextV(
	unsigned int log,
	const char * format,
	va_list & args)
{
	char buf[LOG_LINE_MAX_LEN];

	int len = PStrVprintf(buf, arrsize(buf), format, args);
	if (len > 0)
	{
		sLogText(log, buf, len);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogText(
	unsigned int log,
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	LogTextV(log, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogText1(
	unsigned int log,
	const char * str)
{
	unsigned int len = PStrLen(str, LOG_LINE_MAX_LEN);
	sLogText(log, str, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogMessage(
	unsigned int log,
	const char * str)
{
	if(g_Log.m_Logs[log].m_Initialized)
	{
		if(g_Log.m_Logs[log].m_NoFluff == FALSE)
		{
			SYSTEMTIME time;
			GetLocalTime(&time);

			char buf[LOG_LINE_MAX_LEN];
			
			PStrPrintf(buf, arrsize(buf), "%02d:%02d:%02d.%03d ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
			PStrCopy(buf + 13, str, arrsize(buf) - 13);
			PStrCat(buf, "\n", arrsize(buf));
			buf[arrsize(buf) - 2] = '\n';
			buf[arrsize(buf) - 1] = 0;

			sLogText(log, buf, PStrLen(buf, arrsize(buf)));
		}
		else
		{
			unsigned int len = PStrLen(str, LOG_LINE_MAX_LEN);
			sLogText(log, str, len);		
		}	
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessageV(
	const char * format,
	va_list & args)
{
	char buf[LOG_LINE_MAX_LEN];

	PStrVprintf(buf, arrsize(buf) - 14, format, args);
	sLogMessage(LOG_FILE, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessage(
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	LogMessageV(format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessage1(
	const char * str)
{
	sLogMessage(LOG_FILE, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessageV(
	unsigned int log,
	const char * format,
	va_list & args)
{
	char buf[LOG_LINE_MAX_LEN];

	PStrVprintf(buf, LOG_LINE_MAX_LEN - 14, format, args);
	sLogMessage(log, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessage(
	unsigned int log,
	const char *format,
	...)
{
	va_list args;
	va_start(args, format);

	LogMessageV(log, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessage1(
	unsigned int log,
	const char *str)
{
	sLogMessage(log, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessageAndWarnV(
	unsigned int log,
	const char * format,
	va_list & args)
{
	char buf[LOG_LINE_MAX_LEN];

	PStrVprintf(buf, LOG_LINE_MAX_LEN - 14, format, args);
	WARNX(0, buf);
	sLogMessage(log, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessageAndWarn(
	unsigned int log,
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	LogMessageAndWarnV(log, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogMessageAndWarn1(
	unsigned int log,
	const char * str)
{
	WARNX(0, str);
	sLogMessage(log, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const OS_PATH_CHAR * LogGetFilename(
	unsigned int log)
{
	ASSERT_RETNULL(log < NUM_LOGS);

	if ( ! g_Log.m_Logs[log].m_File )
		return NULL;

	return AsyncFileGetFilename(g_Log.m_Logs[log].m_File);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const OS_PATH_CHAR * LogGetRootPath(
	void)
{
	return g_Log.m_strRootPath;
}


#endif// !ISVERSION(SERVER_VERSION)
