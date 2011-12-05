// ---------------------------------------------------------------------------
// FILE:		nethttp.h
//
// Modified : $DateTime: 2007/09/14 17:35:38 $
// By       : $Author: phu $
//  Copyright 2006, Flagship Studios
// ---------------------------------------------------------------------------
#pragma once

static const char *NET_HTTP_DEFAULT_USER_AGENT = "Ping0 Http Client";

struct INET_CONTEXT;
struct HTTP_CONNECTION;

// Read callback. dwBufSize is 0 when all the response has been read.
typedef void (*FP_HTTP_READ_CALLBACK)(INET_CONTEXT *pInet,
                                      HTTP_CONNECTION *pConn,
                                      void  *pContext,
                                      DWORD dwHttpStatus, // can be win32 error OR Http status code
                                      BYTE *pBuffer,
                                      DWORD dwBufSize);

struct http_client_config
{
    // Address of the server. e.g., "www.myserver.com" 
    char                             *serverAddr;
    // if serverPort is 0, it defaults to INTERNET_DEFAULT_HTTPS_PORT(443)
    WORD                              serverPort; 
    FP_HTTP_READ_CALLBACK             fp_readCallback;
    void                             *pCallbackContext;
    //
    // If expected response size is larger than 8k, this is a good field to fill out.
    DWORD                             dwResponseSizeHint;
};

enum NET_HTTP_VERB_TYPE 
{
    NET_HTTP_VERB_GET,
    NET_HTTP_VERB_POST,
    NET_HTTP_VERB_TYPE_INVALID
};

// Call once to initialize HTTP
INET_CONTEXT *NetHttpInit(MEMORYPOOL *pool,char *szAgent);

// Call once to free the INET_CONTEXT from NetHttpInit.
void NetHttpFree(__in __notnull INET_CONTEXT *pIC);

// For each server you wish to connect to, call this once.
HTTP_CONNECTION* NetHttpCreateClient(__in __notnull INET_CONTEXT *pIC,
                                     http_client_config & config);

// Close when done with a server
void NetHttpCloseClient(__in __notnull HTTP_CONNECTION*);

BOOL NetHttpSendRequest(__in __notnull HTTP_CONNECTION *pConn,
                        __in __notnull const char *szObject,
                        NET_HTTP_VERB_TYPE verbType,
                        DWORD dwInetFlags,
                        __in  const char *lpszHeaders, // NULL TERMINATED string !!
                        __in __bcount(dwPostLength) BYTE *pPostBody,
                        DWORD dwPostLength
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void FN_HTTP_DOWNLOAD_CALLBACK(struct HTTP_DOWNLOAD * download);


struct HTTP_DOWNLOAD
{
	MEMORYPOOL *				pool;
	char						url[4096];
	BYTE *						buffer;
	unsigned int				len;
	FN_HTTP_DOWNLOAD_CALLBACK *	fpCallback;
};


void HttpDownload(
	MEMORYPOOL * pool,
	const char * url,
	FN_HTTP_DOWNLOAD_CALLBACK *	fpCallback);
