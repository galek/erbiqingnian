//----------------------------------------------------------------------------
// Ping0ServerMessages.mc
// 
// Modified     : $DateTime: 2008/05/08 14:55:04 $
// by           : $Author: adeshpande $
//
// (C)Copyright 2006, Ping0 LLC. All rights reserved.
//----------------------------------------------------------------------------
#pragma once
// Header section

/*
 *
 * 
 *  Categories: Keep in sync with Eventlog.h
 *
 *
 *
 *
 */
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_CATEGORY_SERVER_RUNNER
//
// MessageText:
//
//  Server Runner 
//
#define MSG_CATEGORY_SERVER_RUNNER       ((DWORD)0x00000001L)

//
// MessageId: MSG_CATEGORY_USER_SERVER
//
// MessageText:
//
//  User Server 
//
#define MSG_CATEGORY_USER_SERVER         ((DWORD)0x00000002L)

//
// MessageId: MSG_CATEGORY_GAME_SERVER
//
// MessageText:
//
//  Game Server 
//
#define MSG_CATEGORY_GAME_SERVER         ((DWORD)0x00000003L)

//
// MessageId: MSG_CATEGORY_CHAT_SERVER
//
// MessageText:
//
//  Chat Server 
//
#define MSG_CATEGORY_CHAT_SERVER         ((DWORD)0x00000004L)

//
// MessageId: MSG_CATEGORY_LOGIN_SERVER
//
// MessageText:
//
//  Login Server 
//
#define MSG_CATEGORY_LOGIN_SERVER        ((DWORD)0x00000005L)

//
// MessageId: MSG_CATEGORY_PATCH_SERVER
//
// MessageText:
//
//  Patch Server 
//
#define MSG_CATEGORY_PATCH_SERVER        ((DWORD)0x00000006L)

//
// MessageId: MSG_CATEGORY_BILLING_SERVER
//
// MessageText:
//
//  Billing Proxy 
//
#define MSG_CATEGORY_BILLING_SERVER      ((DWORD)0x00000007L)

/*
 *
 *  
 *  Event Definitions
 *  
 *  For consistency, use the MessageId above as the 100's digit of the message
 *  for a particular server. e.g., Server Runner messages should be 101, 102,
 *  etc.
 *  
 */
//
// MessageId: MSG_ID_SERVER_FAILED_TO_START
//
// MessageText:
//
//  Fatal error. A Server of type %1 failed to start. 
//
#define MSG_ID_SERVER_FAILED_TO_START    ((DWORD)0xC0000065L)

//
// MessageId: MSG_ID_EXCEPTION_WHILE_EXECUTING_SERVER
//
// MessageText:
//
//  Fatal error. An exception was thrown while processing a Server of type %1. 
//
#define MSG_ID_EXCEPTION_WHILE_EXECUTING_SERVER ((DWORD)0xC0000066L)

//
// MessageId: MSG_ID_LOST_ALL_DATABASE_CONNECTIONS
//
// MessageText:
//
//  Fatal error. All connections to the database have been lost. 
//
#define MSG_ID_LOST_ALL_DATABASE_CONNECTIONS ((DWORD)0xC0000067L)

//
// MessageId: MSG_ID_ERROR_READING_CONFIG
//
// MessageText:
//
//  Fatal error. Error while reading startup config file. Filename: %1.
//
#define MSG_ID_ERROR_READING_CONFIG      ((DWORD)0xC0000068L)

//
// MessageId: MSG_ID_DB_REQUEST_QUEUE_TOO_LONG
//
// MessageText:
//
//  Fatal error. The database request queue has exceeded the maximum allowed length.
//
#define MSG_ID_DB_REQUEST_QUEUE_TOO_LONG ((DWORD)0xC0000069L)

//
// MessageId: MSG_ID_PERF_SHARED_MEMORY_EXISTS
//
// MessageText:
//
//  Fatal error. Stop remote perf counter collections first.
//
#define MSG_ID_PERF_SHARED_MEMORY_EXISTS ((DWORD)0xC000006AL)

//
// MessageId: MSG_ID_GAME_SERVER_PAKFILE_ERROR
//
// MessageText:
//
//  Fatal error. Game server has failed to load the pakfiles from disk.
//
#define MSG_ID_GAME_SERVER_PAKFILE_ERROR ((DWORD)0xC000012DL)

//
// MessageId: MSG_ID_PATCH_SERVER_PAKFILE_ERROR
//
// MessageText:
//
//  Fatal error. Patch server has failed to load the pakfiles from disk.
//
#define MSG_ID_PATCH_SERVER_PAKFILE_ERROR ((DWORD)0xC0000259L)

//
// MessageId: MSG_ID_ACCOUNTS_DB_ERROR
//
// MessageText:
//
//  The billing proxy accounts database error: %1.
//
#define MSG_ID_ACCOUNTS_DB_ERROR         ((DWORD)0xC00002BDL)

//
// MessageId: MSG_ID_LOST_CONNECTION_TO_BILLING_SERVER
//
// MessageText:
//
//  The billing proxy lost its connection to the billing server.
//
#define MSG_ID_LOST_CONNECTION_TO_BILLING_SERVER ((DWORD)0xC00002BEL)

