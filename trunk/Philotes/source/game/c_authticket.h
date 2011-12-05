//----------------------------------------------------------------------------
// c_authticket.h
// 
// Modified by: $Author: pmason $
// at         : $DateTime: 2007/12/07 14:56:19 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum AuthTicketState
{
	AUTH_TICKET_STATE_INVALID,
    AUTH_TICKET_STATE_IN_PROGRESS,
    AUTH_TICKET_STATE_FAILED,
    AUTH_TICKET_STATE_SUCCEEDED
};

//----------------------------------------------------------------------------
enum AUTH_TICKET_CONSTANTS
{
	MAX_UPLOAD_NAME = 32,
	MAX_ERROR_STRING = 256,
};

//----------------------------------------------------------------------------
enum AUTH_TICKET_CODE
{
	AUTH_TICKET_OK = 200,
	AUTH_TICKET_ERROR_411_OLD_DATA_VERSION = 411,
};

//----------------------------------------------------------------------------
struct AUTH_FAILURE
{
	DWORD dwFailureCode;
	WCHAR uszFailureReason[ MAX_ERROR_STRING ];
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

struct AuthTicketContext * AuthTicketContextNew(MEMORYPOOL * pPool);
void AuthTicketContextRelease(AuthTicketContext * pContext);
void AuthTicketContextAddRef(AuthTicketContext * pContext);

BOOL AuthTicketInit(MEMORYPOOL *pool,
                    __in __notnull char *szTicketServer,
                    __in __out __notnull char *szUserName, // these will be zeroed on return
                    __in __out __notnull char *szPassword,
					struct AuthTicketContext *pTicketContext = NULL);

void AuthTicketFree(AuthTicketContext *pTicketContext = NULL);

AuthTicketState AuthTicketGetState(AuthTicketContext *pTicketContext = NULL);
void AuthTicketGetFailureReason(AUTH_FAILURE *pAuthFailure, AuthTicketContext *pTicketContext = NULL);

BOOL AuthTicketGetAuthCredentials(int serverType,
                                  __in __out __notnull BYTE *pCredBuffer,
                                  __in __out __notnull DWORD *pCBSize,
								  AuthTicketContext *pTicketContext = NULL);

void AuthTicketGetUserServer(int server, 
                             __in __out __notnull __bcount(bufSize)char *szBuffer,
                             DWORD bufSize,
                             __in __out __notnull unsigned short *port,
							 AuthTicketContext *pTicketContext = NULL);

BOOL AuthTicketGetEncryptionKey(int server,
								__in __out __notnull BYTE * bKeyBuf,
								DWORD bufSize,
								DWORD &nKeyLength,
								AuthTicketContext *pTicketContext = NULL);

BOOL AuthTicketGetEncryptionType(int server, 
								ENCRYPTION_TYPE &eEncryptionTypeClientToServer,
								ENCRYPTION_TYPE &eEncryptionTypeServerToClient,
								AuthTicketContext *pTicketContext = NULL);

char *AuthTicketGetPasswordHash64(MEMORYPOOL *pool,
                                  __in __notnull char *szUserName,
                                  __in __notnull char *szPassword);
                                  
//----------------------------------------------------------------------------
// Extern
//----------------------------------------------------------------------------
extern int gnAuthServerPort;
