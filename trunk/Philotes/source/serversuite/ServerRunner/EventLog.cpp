//----------------------------------------------------------------------------
// eventlog.cpp
// 
// Modified     : $DateTime: 2007/01/24 00:35:14 $
// by           : $Author: jlind $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "Ping0ServerMessages.h"
#include "Eventlog.cpp.tmh"

static HANDLE g_hEventLog;

static const WCHAR * EVENT_SOURCE_NAME = L"WatchdogClient";

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitEventLog(void) 
{

    BOOL bRet = TRUE;

#if _DEBUG
#pragma warning(push,3)
    CBRA(MSG_CATEGORY_SERVER_RUNNER == EventCategoryServerRunner);
    CBRA(MSG_CATEGORY_USER_SERVER == EventCategoryUserServer);
    CBRA(MSG_CATEGORY_GAME_SERVER == EventCategoryGameServer);
    CBRA(MSG_CATEGORY_CHAT_SERVER == EventCategoryChatServer);
    CBRA(MSG_CATEGORY_LOGIN_SERVER == EventCategoryLoginServer);
    CBRA(MSG_CATEGORY_PATCH_SERVER == EventCategoryPatchServer);
#pragma warning(pop)
#endif /*DEBUG*/


    g_hEventLog = RegisterEventSourceW(NULL,EVENT_SOURCE_NAME);

    if(g_hEventLog == NULL) 
    {
        TraceError("Could not register event source %ls\n, error : %d\n",
                   EVENT_SOURCE_NAME,GetLastError());

        CBRA(g_hEventLog != NULL);
    }

Error:
    return bRet;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CleanupEventLog(void) 
{
    BOOL bRet = TRUE;

    CBRA(g_hEventLog != NULL);

    DeregisterEventSource(g_hEventLog);

Error:
    return bRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogToEventLog(WORD wType, 
                      WORD wCategory,
                      DWORD dwEventID,
                      LPCWSTR insertString,
                      __in_opt __bcount(dataSize) BYTE *pbData,
                      DWORD dataSize) {
    BOOL bRet = TRUE;

    CBRA(g_hEventLog != NULL);

    if(!ReportEventW(g_hEventLog,
                    wType,
                    wCategory,
                    dwEventID,
                    NULL,
                    1,
                    dataSize,
                    &insertString,
                    (void*)pbData)
                   )  
    {
        DWORD error = GetLastError();
        TraceError("ReportEvent failure in %!FUNC! :%d\n",
                   error);
        CBRA(error == 0);
    }
Error:
    if(!bRet) 
    {
        TraceError("Eventlog: failure in %!FUNC!\n");
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LogErrorInEventLog(      EventCategory category,
                              DWORD         dwEventID,
                              LPCWSTR       insertString,
                              BYTE         *eventData,
                              DWORD         dwDataSize) 
{

    ASSERT_RETURN((unsigned)category < EventCategoryMax);

    sLogToEventLog(EVENTLOG_ERROR_TYPE,
                   (WORD)category,
                   dwEventID,
                   insertString,
                   eventData,
                   dwDataSize);
}

void LogWarningInEventLog(    EventCategory category,
                              DWORD         dwEventID,
                              LPCWSTR       insertString,
                              BYTE         *eventData,
                              DWORD         dwDataSize)
{
    ASSERT_RETURN((unsigned)category < EventCategoryMax);

    sLogToEventLog(EVENTLOG_WARNING_TYPE,
                   (WORD)category,
                   dwEventID,
                   insertString,
                   eventData,
                   dwDataSize);
}

void LogInfoInEventLog(       EventCategory category,
                              DWORD         dwEventID,
                              LPCWSTR       insertString,
                              BYTE         *eventData,
                              DWORD         dwDataSize)
{
    ASSERT_RETURN((unsigned)category < EventCategoryMax);

    sLogToEventLog(EVENTLOG_INFORMATION_TYPE,
                   (WORD)category,
                   dwEventID,
                   insertString,
                   eventData,
                   dwDataSize);
}
