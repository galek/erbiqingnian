// ---------------------------------------------------------------------------
// FILE:	nethttp.cpp
// DESC:	WinHttp based HTTP/HTTTPS methods
//
// Modified : $DateTime: 2008/01/21 14:44:28 $
// by       : $Author: dxis $
//
// Copyright 2006, Flagship Studios
// ---------------------------------------------------------------------------

// CHB 2006.09.07 - Fix tapi.h(43) : fatal error C1017: invalid integer constant expression
#ifdef WIN32
#undef WIN32
#define WIN32 1
#endif

#pragma warning(push,3)
#include <tapi3.h>
#define _INTSAFE_H_INCLUDED_
#include <msputils.h>
#pragma warning(pop)
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <ServerSuite/include/ehm.h>
#include "nethttp.h"
#pragma intrinsic(memcpy,memset)
#include "appcommon.h"


#if !ISVERSION(SERVER_VERSION) // DO NOT USE IN SERVER !!

#define IGNORE_INVALID_CA_FOR_TESTING /*TODO REMOVE THIS IN RETAIL!*/


static const DWORD DEFAULT_RESPONSE_BUFFER_SIZE = 8<<10; //8kb response buffer
enum WinInetOpType {

    NET_INET_OP_INTERNET_OPEN,

    NET_INET_OP_INTERNET_CONNECT,

    NET_INET_OP_OPEN_REQUEST
};

struct BASE_INET_CONTEXT : public OVERLAPPED {
    WinInetOpType op;
};
struct INET_CONTEXT : BASE_INET_CONTEXT {

    MEMORYPOOL      *pPool;

    HINTERNET        hInternet;

    LIST_ENTRY       ConnectionListHead;

    CCriticalSection CSConnectionList;
};

struct HTTP_CONNECTION : BASE_INET_CONTEXT {

    LIST_ENTRY       listEntry;

    HINTERNET        hConnection;

    HINTERNET        hRequest;

    BOOL             bIsSecure;

    WSABUF           postBuffer;

    INTERNET_BUFFERS responseBuffers;

    DWORD            dwResponseBufferSize;

    FP_HTTP_READ_CALLBACK  fp_readCallback;
    void                  *pApplicationContext;

    INET_CONTEXT    *pParent;

    char            *serverAddr;
    short            serverPort;
    
    long             refCount;

    void AddRef(void)
    {
        InterlockedIncrement(&refCount);
    }
    void Free(void)
    {
        if(InterlockedDecrement(&refCount) == 0)
        {
			if(hRequest)
			{
				InternetCloseHandle(hRequest);
			}
			if(hConnection)
			{
				InternetCloseHandle(hConnection);
			}
			if(postBuffer.buf)
			{
				FREE(pParent->pPool,postBuffer.buf);
			}
            FREE(pParent->pPool, this);
        }
    }
};
struct HTTP_SEND_REQUEST_CTX : public OVERLAPPED {
    HTTP_CONNECTION    *pConnection;
    char               *szObject;
    char               *szHeaders;
    NET_HTTP_VERB_TYPE verbType;
    DWORD              dwFlags;
};

static char *g_sNetVerbStrings[] = {
    "GET",
    "POST",
    NULL
};
static char *g_szAcceptTypes[] = {
    "*/*",
    NULL
};

static const ULONG_PTR INET_CONNECT_KEY = 0xdeadbeef;
static const ULONG_PTR INET_HTTP_SENDREQ_KEY = 0xfeedface;

static void sNetHttpSendRequest(HTTP_CONNECTION *pHttpConnection,
                                HINTERNET hRequest);
                            
static DWORD WINAPI sNetHttpAsyncWorker(void *);

static void sNetHttpCloseClient(HTTP_CONNECTION *pConnection, BOOL bRemove);

HANDLE shCompletionPort;
HANDLE shWorkerThread;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
INET_CONTEXT *NetHttpInit(MEMORYPOOL *pool,char *szAgent) 
{
    BOOL bRet = TRUE;
    INET_CONTEXT* pHttp =  NULL;

    shCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                              NULL,
                                              NULL,
                                              0);
    if(shCompletionPort == NULL)
    {
        TraceError("could not create inet completion port %d\n",GetLastError());
        return NULL;
    }
    shWorkerThread = CreateThread(NULL,
                                  0,
                                  sNetHttpAsyncWorker,
                                  NULL,
                                  0,
                                  NULL);
    CBRA(shWorkerThread != NULL);

    pHttp = (INET_CONTEXT*)MALLOCZ(pool,sizeof(INET_CONTEXT));

    CBRA(pHttp != NULL);

    pHttp->pPool = pool;
    pHttp->op = NET_INET_OP_INTERNET_OPEN;

    pHttp->CSConnectionList.Init();

    InitializeListHead(&pHttp->ConnectionListHead);


    pHttp->hInternet = InternetOpen(szAgent ? 
                                        szAgent : NET_HTTP_DEFAULT_USER_AGENT, 
                                    INTERNET_OPEN_TYPE_PRECONFIG,
                                    NULL,
                                    NULL,
                                    0);

    if(pHttp->hInternet == NULL)
    {
        TraceError("InternetOpen failed : %d\n",GetLastError());
        CBRA(pHttp->hInternet != NULL);
    }
Error:
    if(!bRet)
    {
        if(pHttp)
        {
            NetHttpFree(pHttp);
            pHttp = NULL;
        }

    }
    return pHttp;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void NetHttpFree(INET_CONTEXT *pIC)
{
    ASSERT_RETURN(pIC);

    if(pIC->hInternet)
    {
        InternetCloseHandle(pIC->hInternet);
    }
    {
        CSAutoLock lock(&pIC->CSConnectionList);

        LIST_ENTRY *pEntry;
        HTTP_CONNECTION *pConn;

        while(!IsListEmpty(&pIC->ConnectionListHead))
        {
            pEntry = RemoveHeadList(&pIC->ConnectionListHead);
            pConn = CONTAINING_RECORD(pEntry,HTTP_CONNECTION,listEntry);
            
            sNetHttpCloseClient(pConn,FALSE);

        }

    }
    if(shWorkerThread)
    {
        PostQueuedCompletionStatus(shCompletionPort,
                                    0,
                                    0,
                                    NULL);
        WaitForSingleObject(shWorkerThread,30000); //30s.
        CloseHandle(shWorkerThread);
    }
    if(shCompletionPort)
    {
        CloseHandle(shCompletionPort);
    }
    pIC->CSConnectionList.Free();
    FREE(pIC->pPool,pIC);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
HTTP_CONNECTION* NetHttpCreateClient(INET_CONTEXT *pIC,
                                     http_client_config & config)
{

    BOOL bRet = TRUE;
    HTTP_CONNECTION *pConnection = NULL;
    DWORD dwRespSize = config.dwResponseSizeHint ? config.dwResponseSizeHint:
                                    DEFAULT_RESPONSE_BUFFER_SIZE;

    unsigned int serverLen;
    CBRA(pIC);

    CBRA(config.serverAddr);
    CBRA(config.fp_readCallback);

    // use HTTPS by default
    if(config.serverPort == 0)
    {
        config.serverPort = INTERNET_DEFAULT_HTTPS_PORT;
    }

    serverLen = (unsigned int)strlen(config.serverAddr) + 1;
    pConnection = (HTTP_CONNECTION*)MALLOCZ(pIC->pPool,
                                    sizeof(HTTP_CONNECTION) + dwRespSize +
                                    serverLen );

    CBRA(pConnection);

    pConnection->AddRef();

    pConnection->op = NET_INET_OP_INTERNET_CONNECT;
    pConnection->pParent = pIC;
    // Set up response buffer
    pConnection->responseBuffers.dwStructSize = sizeof(INTERNET_BUFFERS);
    pConnection->responseBuffers.lpvBuffer = (void*)(pConnection + 1);
    pConnection->responseBuffers.dwBufferLength = dwRespSize;
    pConnection->dwResponseBufferSize = dwRespSize;
    pConnection->fp_readCallback = config.fp_readCallback;
    pConnection->pApplicationContext = config.pCallbackContext;
    pConnection->serverPort = config.serverPort;

    pConnection->serverAddr = (char*)(pConnection->responseBuffers.lpvBuffer)
                                    + pConnection->responseBuffers.dwBufferLength;

    strcpy_s(pConnection->serverAddr,serverLen,config.serverAddr);
    // copy the server addr

    if(config.serverPort == INTERNET_DEFAULT_HTTPS_PORT)
    {
        pConnection->bIsSecure = TRUE;
    }
    {
        CSAutoLock lock(&pIC->CSConnectionList);

        InsertTailList(&pIC->ConnectionListHead,&pConnection->listEntry);
    }

	pConnection->AddRef();
    if(!PostQueuedCompletionStatus(shCompletionPort,
                                   sizeof(HTTP_CONNECTION),
                                   INET_CONNECT_KEY,
                                   (OVERLAPPED*)pConnection))
	{
		pConnection->Free();
		CBRA(FALSE);
	}
Error:
    if(!bRet)
    {
        if(pConnection)
        {
            CSAutoLock lock(&pIC->CSConnectionList);

            RemoveEntryList(&pConnection->listEntry);

            pConnection->Free();

            pConnection = NULL;
        }
    }
    return pConnection;
                                   
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void NetHttpCloseClient(HTTP_CONNECTION *pConnection)
{
    return sNetHttpCloseClient(pConnection,TRUE);
}
void sNetHttpCloseClient(HTTP_CONNECTION *pConnection, BOOL bRemove)
{
    ASSERT_RETURN(pConnection);

    if(bRemove){
        CSAutoLock lock(&pConnection->pParent->CSConnectionList);

        RemoveEntryList(&pConnection->listEntry);
    }

    pConnection->Free();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL NetHttpSendRequest(HTTP_CONNECTION *pConn,
                        const char *szObject,
                        NET_HTTP_VERB_TYPE verbType,
                        DWORD dwInetFlags,
                        const char *szHeaders,
                        BYTE *pPostBody,
                        DWORD dwPostLength
                        )
{
    BOOL bRet = TRUE;
    DWORD objLen = (DWORD)strlen(szObject)+ 1;
    DWORD hdrLen = (DWORD)strlen(szHeaders) + 1;
    HTTP_SEND_REQUEST_CTX *pRequestCtx = NULL;

    CBRA(pConn);
    CBRA(verbType < NET_HTTP_VERB_TYPE_INVALID);

    pRequestCtx = (HTTP_SEND_REQUEST_CTX*)MALLOCZ(pConn->pParent->pPool,
                                                  sizeof(HTTP_SEND_REQUEST_CTX)
                                                  + objLen + hdrLen);

    CBRA(pRequestCtx);

    strcpy_s((char*)(pRequestCtx + 1),objLen,szObject);
    strcpy_s((char*)(pRequestCtx + 1)+ objLen,hdrLen,szHeaders);

    pRequestCtx->pConnection = pConn;
    pRequestCtx->szObject = (char*)(pRequestCtx + 1);
    pRequestCtx->szHeaders = pRequestCtx->szObject + objLen;
    pRequestCtx->verbType = verbType;

    if(dwPostLength != 0)
    {
        pConn->postBuffer.buf = (char*)MALLOCZ(pConn->pParent->pPool,dwPostLength);
        ASSERT_RETFALSE(pConn->postBuffer.buf);

        pConn->postBuffer.len = dwPostLength;

        memcpy(pConn->postBuffer.buf,pPostBody,dwPostLength);
    }

    if(pConn->bIsSecure)
    {
        dwInetFlags |= INTERNET_FLAG_SECURE;
    }
    dwInetFlags |= (INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_UI);

    pRequestCtx->dwFlags = dwInetFlags;

    pConn->op = NET_INET_OP_OPEN_REQUEST;

    pRequestCtx->pConnection->AddRef();

    if(!(PostQueuedCompletionStatus(shCompletionPort,
                                    0,
                                    INET_HTTP_SENDREQ_KEY,
                                    pRequestCtx)))
    {
        pRequestCtx->pConnection->Free();
        CBRA(FALSE);
    }

Error:
    if(!bRet)
    {
       if(pRequestCtx && pConn)
       {
            FREE(pConn->pParent->pPool,pRequestCtx); 
       }
       if(pConn)
       {
           if(pConn->postBuffer.buf)
           {
                FREE(pConn->pParent->pPool,pConn->postBuffer.buf);
           }
       }
    }
    return bRet;
}
// ---------------------------------------------------------------------------
//
// Begin Static functions :
// These are called from the async worker.
// ---------------------------------------------------------------------------
static BOOL sNetHttpSendRequest(HTTP_CONNECTION *pHttpConnection,HINTERNET hRequest,
                                char *szHeaders)
{
    BOOL bRet = TRUE;
#if !ISVERSION(RETAIL_VERSION) || defined(_BOT) || defined(_PROFILE)
again:
#endif

	if(!InternetSetOption(hRequest,
						  INTERNET_OPTION_IGNORE_OFFLINE,
						  NULL,
						  0))
	{
		TraceError("Option to ignore offline mode failed. Please un-check Internet Explorer \"Work Offline\" menu item\n");
	}

    if(!HttpSendRequest(hRequest,
                        szHeaders,
                        szHeaders?-1:0,
                        pHttpConnection->postBuffer.buf,
                         pHttpConnection->postBuffer.len))
    {
       if(GetLastError() != ERROR_IO_PENDING)
       {
           DWORD err = GetLastError();
#if !ISVERSION(RETAIL_VERSION) || defined(_BOT) || defined(_PROFILE)
        if(err == ERROR_INTERNET_INVALID_CA){
            DWORD dwFlags;
            DWORD dwBufLen = sizeof(dwFlags);
            InternetQueryOption(hRequest,
                                INTERNET_OPTION_SECURITY_FLAGS,
                                (void*)&dwFlags,
                                &dwBufLen);
            dwFlags |= (SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                        SECURITY_FLAG_IGNORE_REVOCATION | //vista
                        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID) ;
            InternetSetOption(hRequest,INTERNET_OPTION_SECURITY_FLAGS,
                              &dwFlags,sizeof(dwFlags));
            goto again;
        }
#endif
           TraceError("HttpSendRequest failed. Error :%d \n",err); 

           if(pHttpConnection->fp_readCallback) 
			   pHttpConnection->fp_readCallback(pHttpConnection->pParent,
                                            pHttpConnection,
                                            pHttpConnection->pApplicationContext,
                                            err,
                                            NULL,
                                            0);
           bRet = FALSE;
       }
    }
    return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static BOOL sNetHttpPostRead(__in __notnull HTTP_CONNECTION *pHttpConn)
{
    BOOL bRet = TRUE;
    DWORD dwErr = NO_ERROR;
    DWORD dwBytes = 0;
    BYTE *pBuffer = NULL;

    do {
        pHttpConn->responseBuffers.dwBufferLength = pHttpConn->dwResponseBufferSize;

        if(!InternetReadFileEx(pHttpConn->hRequest,
                               &pHttpConn->responseBuffers,
                               0,
                               NULL))
        {
            dwErr = GetLastError() ;
            if(dwErr != ERROR_IO_PENDING)
            {
                TraceError("InternetReadFileEx failed: %d\n",dwErr);
                CBRA(dwErr != ERROR_IO_PENDING);
            }
        }
        else
        {
            char szStatus[4]={0};
            DWORD dwErrLen = sizeof(szStatus);
            pBuffer = (BYTE*)pHttpConn->responseBuffers.lpvBuffer;
            dwBytes = pHttpConn->responseBuffers.dwBufferLength;

            TraceExtraVerbose(0,"InternetReadFile read %d bytes\n",dwBytes);

            CBRA(HttpQueryInfo(pHttpConn->hRequest,
                               HTTP_QUERY_STATUS_CODE,
                               &szStatus,
                               &dwErrLen,NULL));
                            

            pHttpConn->fp_readCallback(pHttpConn->pParent,
                                       pHttpConn,
                                       pHttpConn->pApplicationContext,
                                       atoi(szStatus),
                                       pBuffer,
                                       dwBytes);
        }
    }while(pHttpConn->responseBuffers.dwBufferLength != 0);
Error:
    if(!bRet)
    {
        pBuffer = NULL;
        dwBytes = 0;
        pHttpConn->fp_readCallback(pHttpConn->pParent,
                                   pHttpConn,
                                   pHttpConn->pApplicationContext,
                                   dwErr,
                                   pBuffer,
                                   dwBytes);
    }
    return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL sNetHttpDoSend(HTTP_SEND_REQUEST_CTX *pRequestCtx)
{
    BOOL bRet = TRUE;
    HTTP_CONNECTION *pConn = pRequestCtx->pConnection;

    pConn->hRequest = HttpOpenRequestA(pConn->hConnection,
                                      g_sNetVerbStrings[pRequestCtx->verbType],
                                      pRequestCtx->szObject,
                                      NULL, //1.1
                                      NULL, //no referrer,
                                      (LPCSTR*)g_szAcceptTypes,
                                      pRequestCtx->dwFlags,
                                      NULL);

    if(pConn->hRequest == NULL)
    {
        DWORD err = GetLastError();
        TraceError("Could not open http request for %s, error %d\n",
                                        pRequestCtx->szObject,
                                        err);

       pConn->fp_readCallback(pConn->pParent,
                              pConn,
                              pConn->pApplicationContext,
                              err,
                              NULL,
                              0);
        CBRA(pConn->hRequest != NULL);
    }
    else
    {
        TraceNetDebug("HttpOpenRequestA returned handle 0x%08x\n",
                                                            pConn->hRequest);
        bRet = sNetHttpSendRequest(pConn,pConn->hRequest,pRequestCtx->szHeaders);
    }
Error:
    FREE(pConn->pParent->pPool,pRequestCtx);
    return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL sNetHttpDoConnect(HTTP_CONNECTION *pConnection)
{
    BOOL bRet = TRUE;
    // Open the connection
    pConnection->hConnection = InternetConnect(pConnection->pParent->hInternet,
                                       pConnection->serverAddr,
                                       pConnection->serverPort,
                                       NULL,
                                       NULL,
                                       INTERNET_SERVICE_HTTP,
                                       0,
                                       NULL);
    if(pConnection->hConnection== NULL)
    {

        DWORD err = GetLastError();
        TraceError("Could not connect to %s, error %d\n",
                    pConnection->serverAddr,err);

       pConnection->fp_readCallback(pConnection->pParent,
                                     pConnection,
                                     pConnection->pApplicationContext,
                                     err,
                                     NULL,
                                     0);
        CBRA(pConnection->hConnection != NULL);

    }

Error:
    if(!bRet)
    {
        TraceError("sNetHttpDoConnect failed\n");
    }
    return bRet;
    
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static DWORD WINAPI sNetHttpAsyncWorker(void *ignore)
{
    UNREFERENCED_PARAMETER(ignore);
    BOOL bRet = TRUE;
    ULONG_PTR ptrKey = NULL;
    OVERLAPPED *pOverlap = NULL;
    HTTP_CONNECTION *pConnection = NULL;
    DWORD bytesTransferred;

    while(bRet)
    {
        bRet = GetQueuedCompletionStatus(shCompletionPort,
                                         &bytesTransferred,
                                         &ptrKey,
                                         &pOverlap,
                                         INFINITE);

        if(!bRet)
        {
            TraceError("GetQueuedCompletion Status failed %d\n",GetLastError());
            break;
        }
        if(bytesTransferred == 0 && ptrKey == NULL && pOverlap == NULL)
        {
            TraceGameInfo("Internet Async worker quitting\n");
            break;
        }

        ASSERT_BREAK(pOverlap);

        if(ptrKey == INET_CONNECT_KEY)
        {
            pConnection = (HTTP_CONNECTION*)pOverlap;

            if(!sNetHttpDoConnect(pConnection)) 
            {
                TraceError("DoConnect failed\n");
            }
            pConnection->Free();
        }
        else if(ptrKey == INET_HTTP_SENDREQ_KEY)
        {
            HTTP_SEND_REQUEST_CTX *ctx = (HTTP_SEND_REQUEST_CTX*)pOverlap;
            pConnection = ctx->pConnection;
            if(!sNetHttpDoSend(ctx))
            {
                TraceError("DoSend failed\n");
                continue;
            }
            sNetHttpPostRead(pConnection);
            pConnection->Free();

        }
    }
    return bRet;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static DWORD WINAPI HttpDownloadThread(
	LPVOID param)    
{
	Sleep(10000);

	HTTP_DOWNLOAD * download = (HTTP_DOWNLOAD *)param;
	AUTOFREE autofree(NULL, download);

	HINTERNET hSession = NULL;
	HINTERNET hURL = NULL;

	hSession = InternetOpen("Ping0 Download Client", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hSession)
	{
		goto _error;
	}

	hURL = InternetOpenUrl(hSession, download->url, NULL, 0, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hURL)
	{
		goto _error;
	}

	download->buffer = NULL;
	download->len = 0;
	char buffer[4096];
	DWORD bytesread;

	while (InternetReadFile(hURL, buffer, arrsize(buffer), &bytesread))
	{
		if (!bytesread)
		{
			break;
		}
		ASSERT_BREAK(bytesread <= arrsize(buffer));
		download->buffer = (BYTE *)REALLOC(download->pool, download->buffer, download->len + bytesread + 1);
		memcpy_s(download->buffer + download->len, bytesread, buffer, bytesread);
		download->len += bytesread;
	}
	if (download->buffer != NULL)
	{
		download->buffer[download->len] = 0;
		download->len++;
	}	

	if (download->fpCallback)
	{
		download->fpCallback(download);
	}

_error:
	if (hURL)
	{
		InternetCloseHandle(hURL);
	}
	if (hSession)
	{
		InternetCloseHandle(hSession);
	}

	return 1;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HttpDownload(
	MEMORYPOOL * pool,
	const char * url,
	FN_HTTP_DOWNLOAD_CALLBACK *	fpCallback)
{
	ASSERT_RETURN(url);

#if !ISVERSION(SERVER_VERSION)
	HTTP_DOWNLOAD * download = (HTTP_DOWNLOAD *)MALLOCZ(NULL, sizeof(HTTP_DOWNLOAD));
	ASSERT_RETURN(PStrLen(url) < arrsize(download->url));
	PStrCopy(download->url, url, arrsize(download->url));
	download->pool = pool;
	download->fpCallback = fpCallback;

	DWORD dwThreadId;
	HANDLE hThread = (HANDLE)CreateThread(NULL,	0, HttpDownloadThread, (void *)download, 0, &dwThreadId);
	if (hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hThread);
	}
	else
	{
		FREE(NULL, download);
	}
#else
	REF(pool);
	REF(fpCallback);
#endif
}


#endif //!SERVER_VERSION                            
