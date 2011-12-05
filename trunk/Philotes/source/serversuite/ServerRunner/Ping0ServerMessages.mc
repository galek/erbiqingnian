;//----------------------------------------------------------------------------
;// Ping0ServerMessages.mc
;// 
;// Modified     : $DateTime: 2008/05/08 14:55:04 $
;// by           : $Author: adeshpande $
;//
;// (C)Copyright 2006, Ping0 LLC. All rights reserved.
;//----------------------------------------------------------------------------
;#pragma once


;// Header section
;
MessageIdTypedef=DWORD

LanguageNames = (English = 0x0409:Messages_ENU)


;/*
; *
; * 
; *  Categories: Keep in sync with Eventlog.h
; *
; *
; *
; *
; */
MessageId = 1
SymbolicName = MSG_CATEGORY_SERVER_RUNNER
Language = English
Server Runner 
.

MessageId = 2
SymbolicName = MSG_CATEGORY_USER_SERVER
Language = English
User Server 
.
MessageId = 3
SymbolicName = MSG_CATEGORY_GAME_SERVER
Language = English
Game Server 
.

MessageId = 4
SymbolicName = MSG_CATEGORY_CHAT_SERVER
Language = English
Chat Server 
.

MessageId = 5
SymbolicName = MSG_CATEGORY_LOGIN_SERVER
Language = English
Login Server 
.

MessageId = 6
SymbolicName = MSG_CATEGORY_PATCH_SERVER
Language = English
Patch Server 
.

MessageId = 7
SymbolicName = MSG_CATEGORY_BILLING_SERVER
Language = English
Billing Proxy 
.

;/*
; *
; *  
; *  Event Definitions
; *  
; *  For consistency, use the MessageId above as the 100's digit of the message
; *  for a particular server. e.g., Server Runner messages should be 101, 102,
; *  etc.
; *  
; */


MessageId = 101
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_SERVER_FAILED_TO_START
Language = English
Fatal error. A Server of type %1 failed to start. 
.

MessageId = 102
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_EXCEPTION_WHILE_EXECUTING_SERVER
Language = English
Fatal error. An exception was thrown while processing a Server of type %1. 
.

MessageId = 103
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_LOST_ALL_DATABASE_CONNECTIONS
Language = English
Fatal error. All connections to the database have been lost. 
.

MessageId = 104
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_ERROR_READING_CONFIG
Language = English
Fatal error. Error while reading startup config file. Filename: %1.
.

MessageId = 105
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_DB_REQUEST_QUEUE_TOO_LONG
Language = English
Fatal error. The database request queue has exceeded the maximum allowed length.
.

MessageId = 106
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_PERF_SHARED_MEMORY_EXISTS
Language = English
Fatal error. Stop remote perf counter collections first.
.

MessageId = 301
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_GAME_SERVER_PAKFILE_ERROR
Language = English
Fatal error. Game server has failed to load the pakfiles from disk.
.

MessageId = 601
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_PATCH_SERVER_PAKFILE_ERROR
Language = English
Fatal error. Patch server has failed to load the pakfiles from disk.
.

MessageId = 701
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_ACCOUNTS_DB_ERROR
Language = English
The billing proxy accounts database error: %1.
.

MessageId = 702
Severity = Error
Facility =  Application
SymbolicName = MSG_ID_LOST_CONNECTION_TO_BILLING_SERVER
Language = English
The billing proxy lost its connection to the billing server.
.
