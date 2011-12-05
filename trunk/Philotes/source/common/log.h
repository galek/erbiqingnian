//----------------------------------------------------------------------------
// log.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _LOG_H_
#define _LOG_H_


#if !ISVERSION(SERVER_VERSION)

#include "ospathchar.h"


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	LOG_FILE,
	ERROR_LOG,
	SEND_LOG,
	RECV_LOG,
	PERF_LOG,
	INVENTORY_LOG,
	COMBAT_LOG,
	AI_LOG,
	TIMER_LOG,
	PERSONAL_LOG,  // just a convenient place to log stuff for yourself, please don't check in active personal logging
	NETWORK_LOG,
	EXCEL_LOG,
	TREASURE_LOG,
	SOUND_LOG,
	PERFORCE_LOG,
	NODIRECT_LOG,
	DOWNLOAD_LOG,
	DEFINITION_LOG,
	DATAWARNING_LOG,
	MEM_TRACE_LOG,
	SKU_LOG,
	NUM_LOGS,
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void LogInit(
	void);

void LogCheckEnabled(
	void);

void LogFree(
	void);

void LogFlush(unsigned int log = 0xFFFFFFFF);

void LogText(
	unsigned int log,
	const char * format,
	...);

void LogTextV(
	unsigned int log,
	const char * format,
	va_list & args);

void LogText1(
	unsigned int log,
	const char * str);

void LogMessage(
	const char * format,
	...);

void LogMessageV(
	const char * format,
	va_list & args);

void LogMessage1(
	const char * str);

void LogMessage(
	unsigned int log,
	const char * format,
	...);

void LogMessageV(
	unsigned int log,
	const char * format,
	va_list & args);

void LogMessage1(
	unsigned int log,
	const char * str);

void LogMessageAndWarn( 
	unsigned int log,
	const char * format,
	...);

void LogMessageAndWarnV( 
	unsigned int log,
	const char * format,
	va_list & args);

void LogMessageAndWarn1( 
	unsigned int log,
	const char *str);

BOOL LogGetLogging(
	unsigned int log);

void LogSetLogging(
	unsigned int log,
	BOOL value);

BOOL LogGetTrace(
	unsigned int log);

void LogSetTrace(
	unsigned int log,
	BOOL value);

BOOL LogGetConsole(
	unsigned int log);

void LogSetConsole(
	unsigned int log,
	BOOL value,
	DWORD color = 0xffffffff);

const OS_PATH_CHAR * LogGetFilename(
	unsigned int log);

const OS_PATH_CHAR * LogGetRootPath();

#define TraceError(fmt,...)				LogMessage(ERROR_LOG,fmt,__VA_ARGS__)
#define TracePerf(fmt,...)				LogMessage(PERF_LOG,fmt,__VA_ARGS__)
#define TraceDebugOnly(fmt,...)			LogMessage(fmt,__VA_ARGS__)
#define TraceVerbose(flag,fmt,...)		LogMessage(PERSONAL_LOG,fmt,__VA_ARGS__)
#define TraceExtraVerbose(flag,fmt,...)	REF(__VA_ARGS__)//LogMessage(PERSONAL_LOG,fmt,__VA_ARGS__) //Extraverbose traces are designed to be too spammy to leave turned on regularly...

#if !ISVERSION(RETAIL_VERSION)
#define TracePersonal(fmt, ...)			LogMessage(PERSONAL_LOG, fmt, __VA_ARGS__)
#define TraceCombat(fmt, ...)			LogMessage(COMBAT_LOG, fmt, __VA_ARGS__)
#define TraceInventory(fmt, ...)		LogMessage(INVENTORY_LOG, fmt, __VA_ARGS__)
#define TraceTreasure(fmt, ...)			LogMessage(TREASURE_LOG, fmt, __VA_ARGS__)
#define TraceAI(fmt, ...)				LogMessage(AI_LOG, fmt, __VA_ARGS__)
#define TraceGameInfo(fmt, ...)			LogMessage(fmt, __VA_ARGS__)
#define TraceNetDebug(fmt,  ...)		LogMessage(NETWORK_LOG, fmt, __VA_ARGS__)
#define TraceNetSend(fmt,  ...)			LogMessage(SEND_LOG, fmt, __VA_ARGS__)
#define TraceNetRecv(fmt,  ...)			LogMessage(RECV_LOG, fmt, __VA_ARGS__)
#define TraceExcel(fmt, ...)			LogMessage(EXCEL_LOG, fmt, __VA_ARGS__)
#else
#define TracePersonal(fmt, ...)			
#define TraceCombat(fmt, ...)			
#define TraceInventory(fmt, ...)
#define TraceTreasure(fmt, ...)			
#define TraceAI(fmt, ...)				
#define TraceGameInfo(fmt, ...)
#define TraceNetDebug(fmt,  ...)			
#define TraceNetSend(fmt,  ...)			
#define TraceNetRecv(fmt,  ...)			
#define TraceExcel(fmt, ...)	
#endif

#define TraceInfo(flag,fmt,...)			LogMessage(fmt,__VA_ARGS__)
#define TraceWarn(flag,fmt,...)			LogMessage(ERROR_LOG,fmt,__VA_ARGS__)
#define TraceNoDirect(fmt,...)			LogMessage(NODIRECT_LOG,fmt,__VA_ARGS__)

#define TraceDownload(fmt, ...)			LogMessage(DOWNLOAD_LOG,fmt,__VA_ARGS__)
#define TraceNetWarn(fmt, ...)			LogMessage(NETWORK_LOG,fmt,__VA_ARGS__)

#else

#include <svrdebugtrace.h>
#define LogInit()
#define LogFree()
#define LogFlush(...)
#define LogCheckEnabled()
#define LogGetLogging(a)      FALSE
#define LogSetLogging(a,b)
#define LogMessage(...)
#define LogSetTrace(a,b)
#define LogGetTrace
#define LogMessageV
#define EXCEL_LOG WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_MISC_LOG
#define ERROR_LOG WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_ERROR
#define PERF_LOG  WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_MISC_LOG
#define TREASURE_LOG WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_MISC_LOG
#define LOG_FILE WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_MISC_LOG
#define TraceGameInfo(a, ...)

#endif //!SERVER_VERSION
#endif // _LOG_H_