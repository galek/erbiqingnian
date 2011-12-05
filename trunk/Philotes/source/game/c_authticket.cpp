//----------------------------------------------------------------------------
// c_authticket.cpp
//
// contains routines to talk to authentication servers to get an auth ticket.
//
// Last modified : $Author: cschillinger $
//            at : $DateTime: 2008/03/28 18:00:30 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#pragma intrinsic(memcpy,memset)
#include <atlenc.h>
#include <wincrypt.h>
#include <cryptoapi.h>
#include <atlenc.h>
#include <ws2tcpip.h>
#include <wininet.h>

#include <nethttp.h>
#include <version.h>
#include <ServerSuite/include/ehm.h>
#include <servers.h>
#include "c_authticket.h"
#include "appcommon.h"
#include "globalindex.h"
#include "dataversion.h"
#include "fileversion.h"
#include "prime.h"

int gnAuthServerPort = 0;   //0 for SSL

//
// These functions are NOT THREAD SAFE !! 
//
#if !ISVERSION(SERVER_VERSION)

static const char* AUTHTICKET_HELLGATE_URL		 = "/authenticate/authenticate_server.aspx?product=hellgate";
static const char* AUTHTICKET_HELLGATE_URL_LOCAL = "/authentication/authenticate_server.aspx?product=hellgate";

static const char* AUTHTICKET_TUGBOAT_URL		= "/authenticate/authenticate_server.aspx?product=mythos";
static const char* AUTHTICKET_TUGBOAT_URL_LOCAL = "/authentication/authenticate_server.aspx?product=mythos";

static const char* AUTH_CONTENT_TYPE   = "Content-Type: application/x-www-form-urlencoded\r\n";


// add server types for login here
static const char *POST_FORMAT_STRING="version=%d.%d.%d.%d&username=%s&hash=%s&servers=%d,%d,%d,%d&data_version=%d.%d.%d.%d&mac=%s";

static void sAuthTicketReadCallback(INET_CONTEXT *,
                                    HTTP_CONNECTION*,
                                    void *,
                                    DWORD ,
                                    BYTE *,
                                    DWORD );

//----------------------------------------------------------------------------
// Get one of the following for each server type
//----------------------------------------------------------------------------
struct AuthTicketResponse {
	DWORD	dwFullTicketSize;
	DWORD	dwHeaderSize;
    DWORD     dwSizeOfEncryptedTicket;
    DWORD     dwSizeOfSessionKey;
    DWORD     dwSizeOfSessionSeed;
    DWORD     dwAddressLen;
    DWORD     dwServerType;
    ULONGLONG accountId;

	ENCRYPTION_TYPE eEncryptionTypeClientToServer; //Sent as DWORD
	ENCRYPTION_TYPE eEncryptionTypeServerToClient; //Sent as DWORD

    BYTE      *ticket;
    BYTE      *sessionKey;
    BYTE      *sessionSeed;
    BYTE      *UserServerAddress;
};

static const DWORD MAX_TICKET_OR_KEY_SIZE=512;

static INET_CONTEXT *	spInetContext;

static const unsigned int MAX_AUTHTICKET_SERVER_TYPE = TOTAL_SERVER_TYPE_COUNT + 20;/*Buffer room for future added servers*/

//----------------------------------------------------------------------------
// OVERARCHING CONTEXT STRUCT
//----------------------------------------------------------------------------
struct AuthTicketContext
{
	AuthTicketState     ExternalState;
	HTTP_CONNECTION    *pTicketConnection;
	AuthTicketResponse *ticketResponses[MAX_AUTHTICKET_SERVER_TYPE];

	MEMORYPOOL *	    m_pPool;
	volatile LONG	    m_refcount;
    WCHAR               failureReason[MAX_ERROR_STRING];
	DWORD				dwFailureCode;
	
	AuthTicketContext::AuthTicketContext( void )
		:	ExternalState( AUTH_TICKET_STATE_INVALID ),
			pTicketConnection( NULL ),
			m_pPool( NULL ),
			m_refcount( 1 ),
			dwFailureCode( AUTH_TICKET_OK )
	{ 
		failureReason[ 0 ] = 0;
		for (int i = 0; i < MAX_AUTHTICKET_SERVER_TYPE; ++i)
		{
			ticketResponses[ i ] = NULL;
		}
	}
	
};

//----------------------------------------------------------------------------
// Global state for single client auth
//----------------------------------------------------------------------------
static AuthTicketContext sTicketContext;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
AuthTicketContext * AuthTicketContextNew(MEMORYPOOL * pPool)
{
	 AuthTicketContext *toRet = 
		 (AuthTicketContext *) MALLOCZ(pPool, sizeof(AuthTicketContext));
	 toRet->ExternalState = AUTH_TICKET_STATE_INVALID;
	 toRet->m_pPool = pPool;
	 toRet->m_refcount = 1;
#ifndef _BOT
	 const WCHAR *puszAuthFailedLoginError = GlobalStringGet(GS_ERROR_AUTH_FAILED_LOGIN);
     PStrPrintf(toRet->failureReason,ARRAYSIZE(toRet->failureReason),puszAuthFailedLoginError);
#endif
	toRet->dwFailureCode = AUTH_TICKET_OK;
	 return toRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthTicketContextAddRef(AuthTicketContext * pContext)
{
	ASSERT_RETURN(pContext);
	InterlockedIncrement(&pContext->m_refcount);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthTicketContextRelease(AuthTicketContext * pContext)
{
	ASSERT_RETURN(pContext);
	if(InterlockedDecrement(&pContext->m_refcount) == 0)
	{
		ASSERT_RETURN(pContext != &sTicketContext);
		FREE(pContext->m_pPool, pContext);
	}
}
//----------------------------------------------------------------------------
// returns a NULL-terminated string with a hash of the username and password.
//----------------------------------------------------------------------------
char *AuthTicketGetPasswordHash64(MEMORYPOOL *pool,
                                  char *szUserName,
                                  char *szPassword)
{
    BOOL bRet = TRUE;
    DWORD passwordLen = (DWORD)strlen(szPassword);
    DWORD userNameLen = (DWORD)strlen(szUserName);
    DWORD dwHashLen = 0;
    DWORD dwOut = 0;
    BYTE *pHashedCreds = NULL;
    char *szHashedCreds64 = NULL;
    char *szNamePass = NULL;

	ASSERT_RETFALSE(!_strlwr_s(szUserName,userNameLen+1));

    szNamePass = (char*)MALLOCZ(pool,passwordLen + userNameLen +1 );
    
    CBRA(szNamePass);

    CBRA(PStrPrintf(szNamePass,passwordLen + userNameLen + 1,"%s%s",szUserName,szPassword));

    CBRA(CryptoHashBuffer(CALG_SHA1,
                          (BYTE*)szNamePass,
                          passwordLen + userNameLen,
                          &pHashedCreds,
                          &dwHashLen));

    SecureZeroMemory(szNamePass,passwordLen + userNameLen);

    dwOut = Base64EncodeGetRequiredLength(dwHashLen, ATL_BASE64_FLAG_NOCRLF);

    CBRA(dwOut);

    dwOut++; // one for NULL terminator

    szHashedCreds64 = (char*)MALLOCZ(pool,dwOut);

    CBRA(szHashedCreds64);

    CBRA(Base64Encode(pHashedCreds,
                      dwHashLen,
                      szHashedCreds64,
                      (int*)&dwOut,
                      ATL_BASE64_FLAG_NOCRLF));

Error:
    if(pHashedCreds)
    {
        CryptoFreeHashBuffer(pHashedCreds);
    }
    if(szPassword)
    {
        SecureZeroMemory(szPassword,passwordLen);
    }

    if(szNamePass)
    {
        SecureZeroMemory(szNamePass,passwordLen +userNameLen );
        FREE(pool,szNamePass);
    }
    if(!bRet)
    {
        if(szHashedCreds64)
        {
            SecureZeroMemory(szHashedCreds64,dwHashLen);
            FREE(pool,szHashedCreds64);
            szHashedCreds64 = NULL;
        }
        
    }
    return szHashedCreds64;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char *sGetAuthURL(
	void)
{
	BOOL bUseLocalAuth = AppTestDebugFlag( ADF_AUTHENTICATE_LOCAL );
	
	if (AppIsHellgate())
	{
		if (bUseLocalAuth)
		{
			return AUTHTICKET_HELLGATE_URL_LOCAL;
		}
		return AUTHTICKET_HELLGATE_URL;
	}
	else
	{
		if (bUseLocalAuth)
		{
			return AUTHTICKET_TUGBOAT_URL_LOCAL;
		}
		return AUTHTICKET_TUGBOAT_URL;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AuthTicketInit(MEMORYPOOL *pool,
                    char *szTicketServer,
                    char *szUserName,
                    char *szPassword,
					AuthTicketContext *pTicketContext)
{
    BOOL bRet = TRUE;
    http_client_config config;
    char *szPostString = NULL;
    DWORD dwPostLen = 0;
    char *szHashedCreds64 = NULL;
	FILE_VERSION tDataVersion;
	
    if(spInetContext == NULL)
    {
        spInetContext = AppCommonGetHttp();
    }
	if(pTicketContext == NULL)
    {
        pTicketContext = &sTicketContext;
    }

    structclear(config);

    ASSERT_RETFALSE(spInetContext);
    ASSERT_RETFALSE(szTicketServer);
    ASSERT_RETFALSE(szUserName);
    ASSERT_RETFALSE(szPassword);


    dwPostLen = dwPostLen + (DWORD)strlen(szUserName) + 
                    (DWORD)strlen(POST_FORMAT_STRING)+64; //some padding for server ids.

    szHashedCreds64 = AuthTicketGetPasswordHash64(NULL,szUserName,szPassword);

    CBRA(szHashedCreds64);

    dwPostLen += (DWORD)strlen(szHashedCreds64);

    szPostString = (char*)MALLOCZ(NULL,dwPostLen);

    CBRA(szPostString);

	// get the version of the data we have
	DataVersionGet( tDataVersion );
#if ISVERSION(DEVELOPMENT)
	if (!AppTestDebugFlag(ADF_FORCE_DATA_VERSION_CHECK))
	{
		tDataVersion.dwVerTitle = AppIsHellgate() ? 1 : 2;	//	hellgate is 1, tugboat is 2
		tDataVersion.dwVerML = 9999;
		tDataVersion.dwVerRC = 9999;
		tDataVersion.dwVerPUB = 9999;
	}
#endif

	char szMacAddress[1024];
	if (!NetGetClientMacAddress(szTicketServer, szMacAddress, sizeof(szMacAddress)))
	{
		szMacAddress[0] = 0;
	}

    CBRA(PStrPrintf(szPostString,
                dwPostLen,
                POST_FORMAT_STRING,
                TITLE_VERSION,
                PRODUCTION_BRANCH_VERSION,
                RC_BRANCH_VERSION,
                MAIN_LINE_VERSION,
                szUserName,
                szHashedCreds64,
                GAME_SERVER,
                CHAT_SERVER,
                PATCH_SERVER, 
                BILLING_PROXY,
				tDataVersion.dwVerTitle,
                tDataVersion.dwVerPUB,
                tDataVersion.dwVerRC,
                tDataVersion.dwVerML,
				szMacAddress));

    //
    // Internet stuff

    config.serverAddr = szTicketServer;
    config.pCallbackContext = pTicketContext;
    config.serverPort = (WORD)gnAuthServerPort; //0 for SSL
    config.fp_readCallback = sAuthTicketReadCallback;

	AuthTicketContextAddRef(pTicketContext);

    pTicketContext->pTicketConnection = NetHttpCreateClient(spInetContext,config);

    CBRA(pTicketContext->pTicketConnection);

	// get the authentication server
	const char *pszAuthURL = sGetAuthURL();

	// send auth
	CBRA(NetHttpSendRequest(pTicketContext->pTicketConnection,
							pszAuthURL,
							NET_HTTP_VERB_POST,
							0,
							AUTH_CONTENT_TYPE,
							(BYTE*)szPostString,
							(DWORD)strlen(szPostString)));
	
    pTicketContext->ExternalState = AUTH_TICKET_STATE_IN_PROGRESS;
Error:
    if(!bRet)
    {
        pTicketContext->ExternalState = AUTH_TICKET_STATE_FAILED;

        if(pTicketContext->pTicketConnection)
        {
            NetHttpCloseClient(pTicketContext->pTicketConnection);
            pTicketContext->pTicketConnection = NULL;
        }
    }
    if(szHashedCreds64)
    {
        SecureZeroMemory(szHashedCreds64,strlen(szHashedCreds64));
        FREE(NULL,szHashedCreds64);
    }
    if(szPostString)
    {
        SecureZeroMemory(szPostString,dwPostLen);
        FREE(NULL,szPostString);
    }

    return bRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthTicketFree(AuthTicketContext *pTicketContext)
{
	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    if(pTicketContext->pTicketConnection)
    {
        NetHttpCloseClient(pTicketContext->pTicketConnection);
        pTicketContext->pTicketConnection = NULL;
    }
    for(int i =0 ; i < MAX_AUTHTICKET_SERVER_TYPE; i++)
    {
        if(pTicketContext->ticketResponses[i] != NULL)
        {
            FREE(NULL,pTicketContext->ticketResponses[i]);
            pTicketContext->ticketResponses[i] = NULL;
        }
    }
    spInetContext = NULL;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCopyResponseData(__in __notnull AuthTicketResponse *pResponse, 
                       __in __bcount(dwLen) BYTE *pBuffer, 
                       DWORD dwLen)
{
    pResponse->ticket = (BYTE*)(pResponse + 1);
    memcpy(pResponse->ticket, pBuffer,pResponse->dwSizeOfEncryptedTicket);
    pBuffer += pResponse->dwSizeOfEncryptedTicket;

    pResponse->sessionKey = pResponse->ticket + pResponse->dwSizeOfEncryptedTicket;
    memcpy(pResponse->sessionKey,pBuffer, pResponse->dwSizeOfSessionKey);
    pBuffer += pResponse->dwSizeOfSessionKey;

    pResponse->sessionSeed = pResponse->sessionKey + pResponse->dwSizeOfSessionKey;
    memcpy( pResponse->sessionSeed, pBuffer, pResponse->dwSizeOfSessionSeed);
    pBuffer += pResponse->dwSizeOfSessionSeed;

    pResponse->UserServerAddress = pResponse->sessionSeed + pResponse->dwSizeOfSessionSeed;
    memcpy( pResponse->UserServerAddress, pBuffer, pResponse->dwAddressLen);
    *(pResponse->UserServerAddress + pResponse->dwAddressLen) = 0; //NULL terminate it.
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT_VERSION)
void sPrintResponseData(__in __notnull AuthTicketResponse *pResp)
{
    TraceDebugOnly("Response from server for Server type %d\n",pResp->dwServerType);
    TraceDebugOnly("\tAccount Id 0x%I64x\n",pResp->accountId);
    TraceDebugOnly("\tUserServer addr %s\n",pResp->UserServerAddress);
    TraceDebugOnly("\tTicketSize %d\n",pResp->dwSizeOfEncryptedTicket);
    TraceDebugOnly("\tKeySize %d\n",pResp->dwSizeOfSessionKey);
    TraceDebugOnly("\tSeedSize %d\n",pResp->dwSizeOfSessionSeed);
}
#else
#define sPrintResponseData(pResp)
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetDWORDFromBuffer(BYTE *pBuffer, DWORD*dwOut,
                                DWORD dwMaxValuePlusOne)
{
    memcpy(dwOut,pBuffer,sizeof(*dwOut));
    *dwOut = (unsigned)ntohl(*dwOut);

    return (*dwOut < dwMaxValuePlusOne);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sAuthTicketFindFailureReason(DWORD errorCode,AuthTicketContext *pTicketContext)
{
    HMODULE hModule = NULL;
	if(pTicketContext == NULL) 
    {
        pTicketContext = &sTicketContext;
    }
    pTicketContext->failureReason[0] = UNICODE_NULL;
    if(errorCode > 599 || errorCode < 100) 
    {
        if(errorCode > INTERNET_ERROR_BASE && errorCode < INTERNET_ERROR_LAST) 
        {
            hModule = GetModuleHandleW(L"Wininet.dll");
        }
		WCHAR *puszAuthCertExpiredError = NULL;
		if(errorCode == ERROR_INTERNET_SEC_CERT_DATE_INVALID)
		{
			// SSL Certificate Expired - Tell user to check his system time
			puszAuthCertExpiredError = (WCHAR *)GlobalStringGet(GS_ERROR_AUTH_EXPIRED_CERTIFICATE_12037);
			if(puszAuthCertExpiredError)
			{
            PStrPrintf(pTicketContext->failureReason,
                       ARRAYSIZE(pTicketContext->failureReason),
                       puszAuthCertExpiredError);
			}
		}
        if(!puszAuthCertExpiredError && !FormatMessageW(
                          hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM,
                          hModule,
                          errorCode,
                          MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                          pTicketContext->failureReason,
                          ARRAYSIZE(pTicketContext->failureReason),
                          NULL))
        {
			const WCHAR *puszAuthUnknownError = GlobalStringGet(GS_ERROR_AUTH_UNKNOWN_WITH_PARAM);
            PStrPrintf(pTicketContext->failureReason,
                       ARRAYSIZE(pTicketContext->failureReason),
                       puszAuthUnknownError, errorCode);
        }
    }
    else 
    {
       switch(errorCode / 100)
       {
           case 5:
			   if(errorCode == 503)
			   {
				     // Servers Offline
					 const WCHAR *puszAuthServerOfflineError = GlobalStringGet(GS_ERROR_AUTH_OFFLINE_503);
					 PStrPrintf(pTicketContext->failureReason,
						       ARRAYSIZE(pTicketContext->failureReason),
                        	   puszAuthServerOfflineError);
			   } else {
					 const WCHAR *puszAuthInternalError = GlobalStringGet(GS_ERROR_AUTH_INTERNAL_500);
					 PStrPrintf(pTicketContext->failureReason,
						       ARRAYSIZE(pTicketContext->failureReason),
                        	   puszAuthInternalError);
			   }
               break;
           case 4:
               if(errorCode == 402)
               {
				   // Missing or Invalid Product Key
				   const WCHAR *puszAuthInvalidProductKeyError = GlobalStringGet(GS_ERROR_AUTH_INVALID_PRODUCT_KEY_402);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInvalidProductKeyError);
               }
			   else if(errorCode == 403)
			   {
				   // Banned By GeoIP
				   const WCHAR *puszAuthInvalidAddressError = GlobalStringGet(GS_ERROR_USER_SERVER_AUTH_1401);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInvalidAddressError);
			   }
               else if(errorCode == 404)
               {
				   // Invalid Auth Server URL
				   const WCHAR *puszAuthInvalidAddressError = GlobalStringGet(GS_ERROR_AUTH_INVALID_ADDRESS_404);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInvalidAddressError);
               }
			   else if(errorCode == 405)
			   {
				   // Banned User Account 
				   const WCHAR *puszAuthBannedError = GlobalStringGet(GS_ERROR_AUTH_BANNED_405);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthBannedError);
			   }
			   else if(errorCode == 406)
			   {
				   // Insufficient Client Version
				   const WCHAR *puszAuthInvalidVersionError = GlobalStringGet(GS_ERROR_AUTH_INVALID_VERSION_406);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInvalidVersionError);
			   }
			   else if(errorCode == 407)
			   {
				   // Inactive Account
				   const WCHAR *puszAuthInactiveAccountError = GlobalStringGet(GS_ERROR_AUTH_NOT_ACTIVE_407);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInactiveAccountError);
			   }
			   else if(errorCode == 409)
			   {
				   // Underage user attempting connection to age-restricted shard
				   const WCHAR *puszAuthUnderageError = GlobalStringGet(GS_ERROR_AUTH_UNDERAGE_409);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthUnderageError);
			   }
			   else if(errorCode == 410)
			   {
				   // Expired grace-period user (key check enabled on server and no ACCT_MODIFIER_GRACE_PERIOD_USER badge)
				   const WCHAR *puszAuthExpiredGracePeriodError = GlobalStringGet(GS_ERROR_AUTH_EXPIRED_GRACE_PERIOD_USER_410);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthExpiredGracePeriodError);
			   }
			   else if (errorCode == AUTH_TICKET_ERROR_411_OLD_DATA_VERSION)
			   {
				   // Insufficient Client Data Version
				   GLOBAL_STRING eString = GS_ERROR_AUTH_INVALID_DATA_VERSION_411;
				   if (AppAppliesMinDataPatchesInLauncher())
				   {
						eString = GS_ERROR_AUTH_INVALID_DATA_VERSION_411_RUN_LAUNCHER;
				   }
				   const WCHAR *puszAuthMessage = GlobalStringGet(eString);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthMessage);			   
			   }
               else
               {
				   // Invalid account name or password
				   const WCHAR *puszAuthInvalidPasswordError = GlobalStringGet(GS_ERROR_AUTH_INVALID_PASSWORD_401);
                   PStrPrintf(pTicketContext->failureReason,
                              ARRAYSIZE(pTicketContext->failureReason),
                              puszAuthInvalidPasswordError);
               }
               break;
           default:
			    // Unknown Error
				const WCHAR *puszAuthUnknownError = GlobalStringGet(GS_ERROR_AUTH_UNKNOWN_WITH_PARAM);
                PStrPrintf(pTicketContext->failureReason,
                          ARRAYSIZE(pTicketContext->failureReason),
                          puszAuthUnknownError,errorCode);
               break;
       }
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sParseResponse(__in __bcount(dwLen) char *pBuffer,
						   DWORD dwLen,
						   AuthTicketContext *pTicketContext = NULL)
{
    BOOL bRet = TRUE;

    AuthTicketResponse *pResp = NULL;
    DWORD dwCount = 0;
    DWORD dwRespTotal = 0;
    DWORD dwTS = 0, dwKS = 0, dwKSS=0,dwAL=0,serverType = 0;
	DWORD dwEncryptionTypeCS = 0, dwEncryptionTypeSC = 0;
    ULONGLONG actid;
	
	DWORD dwFullTicketSize;
	DWORD dwHeaderSize;

    DWORD dwBytesLeft = dwLen;
    BYTE *pCur = (BYTE*)pBuffer;

	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    if(!sGetDWORDFromBuffer(pCur,&dwCount,MAX_AUTHTICKET_SERVER_TYPE))
	{
		trace("unable to parse response from auth server.");
		bRet = FALSE;
		goto Error;
	}
    pCur += sizeof(dwCount);
    dwBytesLeft -= sizeof(dwCount);

	DWORD nTicketsParsed = 0;

	BOOL bGotPatchServerTicket = FALSE;
	BYTE *pStartOfCurrentTicket = pCur;
    while(dwBytesLeft)
    {
		ASSERTX_BREAK(nTicketsParsed < dwCount, "Error: more authtickets than specified");
        CBRA(dwBytesLeft < dwLen);

        CBRA(dwBytesLeft > sizeof(AuthTicketResponse));

		// get fullticketsize
		CBRA(sGetDWORDFromBuffer(pCur,&dwFullTicketSize,MAX_TICKET_OR_KEY_SIZE));
		pCur += sizeof(dwFullTicketSize);
		dwBytesLeft -= sizeof(dwFullTicketSize);

		// get ticketheadersize
		CBRA(sGetDWORDFromBuffer(pCur,&dwHeaderSize,MAX_TICKET_OR_KEY_SIZE));
		pCur += sizeof(dwHeaderSize);
		dwBytesLeft -= sizeof(dwHeaderSize);

        // get encryptedticketsize
        CBRA(sGetDWORDFromBuffer(pCur,&dwTS,MAX_TICKET_OR_KEY_SIZE));
        pCur += sizeof(dwTS);
        dwBytesLeft -= sizeof(dwTS);

        // get key size
        CBRA(sGetDWORDFromBuffer(pCur,&dwKS,MAX_TICKET_OR_KEY_SIZE));
        pCur += sizeof(dwKS);
        dwBytesLeft -= sizeof(dwKS);

        // get key Seed size
        CBRA(sGetDWORDFromBuffer(pCur,&dwKSS,MAX_TICKET_OR_KEY_SIZE));
        pCur += sizeof(dwKSS);
        dwBytesLeft -= sizeof(dwKSS);

        // get address length
        CBRA(sGetDWORDFromBuffer(pCur,&dwAL,INET6_ADDRSTRLEN));
        pCur += sizeof(dwAL);
        dwBytesLeft -= sizeof(dwAL);

        // get server type
        CBRA(sGetDWORDFromBuffer(pCur,&serverType,MAX_AUTHTICKET_SERVER_TYPE));
        CBRA(pTicketContext->ticketResponses[serverType] == NULL);
        pCur += sizeof(serverType);
        dwBytesLeft -= sizeof(serverType);
        
        // did we get a patch server ticket
        if (serverType == PATCH_SERVER)
        {
			bGotPatchServerTicket = TRUE;
        }

        memcpy(&actid,pCur,sizeof(ULONGLONG));
        pCur += sizeof(actid);
        dwBytesLeft -= sizeof(actid);

		CBRA(sGetDWORDFromBuffer(pCur,&dwEncryptionTypeCS,ENCRYPTION_TYPE_LAST));
		pCur += sizeof(dwEncryptionTypeCS);
		dwBytesLeft -= sizeof(dwEncryptionTypeCS);

		CBRA(sGetDWORDFromBuffer(pCur,&dwEncryptionTypeSC,ENCRYPTION_TYPE_LAST));
		pCur += sizeof(dwEncryptionTypeSC);
		dwBytesLeft -= sizeof(dwEncryptionTypeSC);
		
        // total size is size of header + all these field above.
        dwRespTotal = dwTS + dwKS +dwKSS + dwAL ;

        if(dwRespTotal > dwBytesLeft)
        {
            TraceError("AuthTicketResponse: expecting %d, only left with %d\n",
                    dwRespTotal, dwBytesLeft);
            bRet = FALSE;
            break;
        }
        pResp = (AuthTicketResponse*)MALLOCZ(NULL,sizeof(*pResp) + dwRespTotal +1 );
        CBRA(pResp);

		pResp->dwFullTicketSize = dwFullTicketSize;
		pResp->dwHeaderSize = dwHeaderSize;

        pResp->dwSizeOfEncryptedTicket = dwTS;
        pResp->dwSizeOfSessionKey = dwKS;
        pResp->dwSizeOfSessionSeed = dwKSS;
        pResp->dwAddressLen = dwAL;
        pResp->dwServerType = serverType;
        pResp->accountId =  actid;

		pResp->eEncryptionTypeClientToServer = ENCRYPTION_TYPE(dwEncryptionTypeCS);
		pResp->eEncryptionTypeServerToClient = ENCRYPTION_TYPE(dwEncryptionTypeSC);

		//Validate that we are in the correct part of the buffer according to received sizes.
		ASSERTV_DO(pCur == pStartOfCurrentTicket + dwHeaderSize,
			"Informational: size of known header structure %d does not match sent size %d.  New version?",
			pCur - pStartOfCurrentTicket, dwHeaderSize)
		{
			dwBytesLeft -= (dwHeaderSize - (pCur - pStartOfCurrentTicket) );
			pCur = pStartOfCurrentTicket + dwHeaderSize;
		}

        sCopyResponseData(pResp,pCur,dwBytesLeft);

        pCur += (dwTS + dwKS + +dwKSS + dwAL);
        dwBytesLeft -= (dwTS + dwKS + +dwKSS + dwAL);

        sPrintResponseData(pResp);

		 pTicketContext->ticketResponses[serverType] = pResp;

		 ASSERTV_DO(pCur == pStartOfCurrentTicket + dwFullTicketSize,
			 "WTF: size of known full ticket %d does not match sent size %d.  New version?",
			 pCur - pStartOfCurrentTicket, dwFullTicketSize)
		 {
			 dwBytesLeft -= (dwFullTicketSize - (pCur - pStartOfCurrentTicket) );
			 pCur = pStartOfCurrentTicket + dwFullTicketSize;
		 }

		 pStartOfCurrentTicket = pCur;
		 nTicketsParsed++;
    }

	// this is a little ugly ... what we've done for the "minimum data version" check
	// is to change the auth server to only return a ticket for the patch server 
	// if this client does not have the required data to play.  That way even clients
	// that have old data like this can get patched any new .exe and files required
	// to get to the main menu and we can always add new logic to clients.  So the way
	// we're detecting that situation is if we got exactly 1 ticket back, and it was
	// a ticket for the patch server
	if (bGotPatchServerTicket == TRUE && dwCount == 1)
	{
		AppSetFlag( AF_PATCH_ONLY, TRUE );
	}
	
Error:
    return bRet;
}
//----------------------------------------------------------------------------
// This function is called after the server has sent us data
//----------------------------------------------------------------------------
static void sContinueTicketAuth(char *buffer, DWORD bufLen,
								AuthTicketContext *pTicketContext = NULL)
{
    BOOL bRet = TRUE;
    DWORD dwRequiredLen = Base64DecodeGetRequiredLength(bufLen);

    char *decodedBuffer = (char*)MALLOCZ(NULL,dwRequiredLen);

	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    CBRA(decodedBuffer);

    CBRA(Base64Decode(buffer,bufLen,(BYTE*)decodedBuffer,(int*)&dwRequiredLen));

	if(!sParseResponse(decodedBuffer,dwRequiredLen, pTicketContext))
	{
		trace("unable to parse response from authentication server.");
		bRet = FALSE;
		goto Error;
	}
    //
    pTicketContext->ExternalState = AUTH_TICKET_STATE_SUCCEEDED;

Error:
    if(!bRet)
    {
        pTicketContext->ExternalState = AUTH_TICKET_STATE_FAILED;
    }
    if(decodedBuffer)
    {
        SecureZeroMemory(decodedBuffer,dwRequiredLen);
        FREE(NULL,decodedBuffer);
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WSABUF newCreds;
static DWORD  dwCurrentOffset = 0;
static void sAuthTicketReadCallback(INET_CONTEXT *pIc,
                                    HTTP_CONNECTION*pConn,
                                    void * pContext,
                                    DWORD dwError,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
	AuthTicketContext *pTicketContext = (AuthTicketContext *) pContext;
	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    BOOL bRet = TRUE;
    if(dwError != AUTH_TICKET_OK )
    {
        TraceError("sAuthTicketReadCallback: Error %d, buffersize %d\n",dwError,dwBufSize);
        pTicketContext->ExternalState = AUTH_TICKET_STATE_FAILED;
        pTicketContext->dwFailureCode = dwError;
        sAuthTicketFindFailureReason(dwError,pTicketContext);
        return;
    }

    if(newCreds.buf == NULL) 
    {
        CBR(dwBufSize);

        newCreds.buf = (char*)MALLOCZ(NULL,dwBufSize << 2); // 4 times this buffer

        CBRA(newCreds.buf);

        newCreds.len = (dwBufSize << 2);

        memcpy(newCreds.buf,pBuffer,dwBufSize);

        dwCurrentOffset = dwBufSize;
    }
    else
    {
        if(newCreds.len - dwCurrentOffset < dwBufSize)
        {
            TraceDebugOnly("Expanding newcreds buffer from %d to %d bytes\n",
                            newCreds.len, newCreds.len + (dwBufSize << 2));

            char *save = newCreds.buf;
            newCreds.buf = (char*)REALLOCZ(NULL,newCreds.buf,newCreds.len + (dwBufSize << 2));

            if(!newCreds.buf)
            {
                FREE(NULL,save);
            }

            CBRA(newCreds.buf);

            memcpy(newCreds.buf + dwCurrentOffset,pBuffer,dwBufSize);

            dwCurrentOffset += dwBufSize;
        }

        if(dwBufSize == 0)  // no more data
        {
            CBRA(dwCurrentOffset);

            sContinueTicketAuth(newCreds.buf,dwCurrentOffset, pTicketContext);

            FREE(NULL,newCreds.buf);
            newCreds.buf = NULL;

			AuthTicketContextRelease(pTicketContext); //releasing nethttp addref, should still have refcount 1 usually.
        }
    }
Error:
    if(!bRet)
    {
        if(newCreds.buf)
        {
            FREE(NULL,newCreds.buf);
            newCreds.buf = NULL;
        }
		//NOTE: currently this code doesn't ever get hit.
		//If it ever did get hit, we could potentially be
		//changing the state of a ticket released above.
        pTicketContext->ExternalState = AUTH_TICKET_STATE_FAILED;
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
AuthTicketState AuthTicketGetState(AuthTicketContext *pTicketContext)
{
	if(pTicketContext == NULL) pTicketContext = &sTicketContext;
    return pTicketContext->ExternalState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthTicketGetFailureReason(AUTH_FAILURE *pAuthFailure, AuthTicketContext *pTicketContext /*= NULL*/)
{
	ASSERTX_RETURN( pAuthFailure, "Expected auth failure struct" );
	
    if(pTicketContext == NULL)
    {
        pTicketContext = &sTicketContext;
    }
    if(PStrLen(pTicketContext->failureReason) == 0)
    {
        PStrPrintf(pTicketContext->failureReason,
                   ARRAYSIZE(pTicketContext->failureReason),
                   L"Unknown Error from Server. Please check your password and try again. ");
    }
    
    // copy to auth failure return param
    PStrCopy( pAuthFailure->uszFailureReason, pTicketContext->failureReason, MAX_ERROR_STRING );
    pAuthFailure->dwFailureCode = pTicketContext->dwFailureCode;
        
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AuthTicketGetAuthCredentials(int serverType,
                                  BYTE *pCredBuffer,
                                  DWORD *pCBSize,
								  AuthTicketContext *pTicketContext)
{
    BOOL bRet = TRUE;

	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    if(pTicketContext->ticketResponses[serverType] == NULL )
    {
        TraceDebugOnly("Attempted to get creds for server type %d, but none found\n",
                        serverType);
        return FALSE;
    }
    if(pTicketContext->ticketResponses[serverType]->dwSizeOfEncryptedTicket > *pCBSize)
    {
        TraceError("Buffer for credentials was too small\n");
        CBRA(FALSE);
    }
    memcpy(pCredBuffer,pTicketContext->ticketResponses[serverType]->ticket,pTicketContext->ticketResponses[serverType]->dwSizeOfEncryptedTicket);
    *pCBSize = pTicketContext->ticketResponses[serverType]->dwSizeOfEncryptedTicket ;
Error:
    if(!bRet)
    {
        *pCBSize = 0;
    }
    return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthTicketGetUserServer(int server, char *szBuffer,DWORD bufSize,unsigned short *port,
							 AuthTicketContext *pTicketContext)
{
    char *ptr;

	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

    if(!pTicketContext->ticketResponses[server])
    {
        TraceDebugOnly("Attempted to get userserver for server type %d, but none found\n",
                        server);
        return;
    }

    ptr =  (char*)PStrRChr((char*)pTicketContext->ticketResponses[server]->UserServerAddress,':');
    *port = (SHORT)PStrToInt(ptr + 1);
    *ptr = 0;
    PStrCopy(szBuffer,(char*)pTicketContext->ticketResponses[server]->UserServerAddress,bufSize);
    *ptr = ':';
}

//----------------------------------------------------------------------------
// theoretically, there are two encryption keys to get: client to server
// and server to client.  We just use one half of the keyBuf for one,
// and one half for the other.
//----------------------------------------------------------------------------
BOOL AuthTicketGetEncryptionKey(int server, BYTE * bKeyBuf,  DWORD bufSize, DWORD &nKeyLength,
								AuthTicketContext *pTicketContext)
{
	ASSERT_RETFALSE(bKeyBuf);

	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

	if(!pTicketContext->ticketResponses[server])
	{
		TraceDebugOnly("Attempted to get encryption key for server type %d, but none found\n",
			server);
		return FALSE;
	}

	nKeyLength = pTicketContext->ticketResponses[server]->dwSizeOfSessionKey;
	ASSERTX_RETFALSE(nKeyLength <= bufSize, "Buffer smaller than encryption key length.\n");
	MemoryCopy(bKeyBuf, bufSize, pTicketContext->ticketResponses[server]->sessionKey, nKeyLength);

	return TRUE;
}

//----------------------------------------------------------------------------
BOOL AuthTicketGetEncryptionType(int server, 
								ENCRYPTION_TYPE &eEncryptionTypeClientToServer,
								ENCRYPTION_TYPE &eEncryptionTypeServerToClient,
								AuthTicketContext *pTicketContext)
{
	if(pTicketContext == NULL) pTicketContext = &sTicketContext;

	if(!pTicketContext->ticketResponses[server])
	{
		TraceDebugOnly("Attempted to get encryption type for server type %d, but none found\n",
			server);
		return FALSE;
	}

	eEncryptionTypeClientToServer = 
		pTicketContext->ticketResponses[server]->eEncryptionTypeClientToServer;
	eEncryptionTypeServerToClient =
		pTicketContext->ticketResponses[server]->eEncryptionTypeServerToClient;

	return TRUE;
}

#endif // !ISVERSION(SERVER_VERSION)

