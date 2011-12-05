// (C)Copyright 2007, Flagship Studios. All rights reserved.

#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#define SYSINFO_USE_UNICODE_ENCODING 1

#ifdef SYSINFO_USE_UNICODE_ENCODING
typedef wchar_t SYSINFO_CHAR;
#define SYSINFO_TEXT(s) L##s
#else
typedef char SYSINFO_CHAR;
#define SYSINFO_TEXT(s) s
#endif

#include "textlog.h"
typedef TextLog<SYSINFO_CHAR> SysInfoLog;

void SysInfo_Init(void);
void SysInfo_Deinit(void);

void SysInfo_ReportAll(SysInfoLog * pLog);
void SysInfo_ReportAllToDebugOutput(void);

typedef void (SYSINFO_REPORTFUNC)(SysInfoLog * pLog, void * pContext);
void SysInfo_RegisterReportCallback(SYSINFO_REPORTFUNC * pCallback, void * pContext);

const int SYSINFO_VERSION_STRING_SIZE = 32;
void SysInfo_VersionToString(SYSINFO_CHAR * pStrOut, unsigned nBufSize, ULONGLONG nVersion);

#endif
