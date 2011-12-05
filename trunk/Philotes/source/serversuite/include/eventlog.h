//----------------------------------------------------------------------------
// eventlog.h
// 
// Modified     : $DateTime: 2007/07/11 16:51:05 $
// by           : $Author: pmason $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

// If you add a category, remember to :
// 1. change the CategoryCount in the reg file
// 2. update the installer .wxs file appropriately.
// 3. update the .mc file.

enum EventCategory { // one for each server type
   EventCategoryInvalid,
   EventCategoryServerRunner, 
   EventCategoryUserServer, 
   EventCategoryGameServer, 
   EventCategoryChatServer,
   EventCategoryLoginServer,
   EventCategoryPatchServer,
   EventCategoryBillingProxy,
   EventCategoryMax
};

__checkReturn BOOL InitEventLog(void);
BOOL CleanupEventLog(void);

// dwEventID comes from EventMsgs.h 
//
// insertString is an optional string that should be inserted in the message.
//              This should only be specified if the message text contains a 
//              placeholder (%1)
//
void LogErrorInEventLog(      EventCategory category,
                              DWORD         dwEventID,
__in_opt                      LPCWSTR       insertString,
__in_opt __bcount(dwDataSize) BYTE         *eventData,
                              DWORD         dwDataSize);

void LogWarningInEventLog(    EventCategory category,
                              DWORD         dwEventID,
__in_opt                      LPCWSTR       insertString,
__in_opt __bcount(dwDataSize) BYTE         *eventData,
                              DWORD         dwDataSize);

void LogInfoInEventLog(       EventCategory category,
                              DWORD         dwEventID,
__in_opt                      LPCWSTR       insertString,
__in_opt __bcount(dwDataSize) BYTE         *eventData,
                              DWORD         dwDataSize);

