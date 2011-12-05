//----------------------------------------------------------------------------
// patchclient.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "prime.h"


#if !ISVERSION(SERVER_VERSION)
#include "c_connmgr.h"
#include "mailslot.h"
#include "pakfile.h"
#include "performance.h"
#include "compression.h"
#include "asyncfile.h"
#include "patchclientmsg.h"
#include "patchclient.h"
#include "list.h"
#include "cryptoapi.h"
#include "c_authticket.h"
#include "filepaths.h"
#include "sku.h"
#include "language.h"
#include "globalindex.h"
#include "stringreplacement.h"
#include "TokenBucket.h"

using namespace COMMON_SHA1;

// Defines
#define MAX_DOWNLOADS 8
#define ENCRYPTION_KEY_SIZE 32
#define ACK_RECEIVED_BYTE_THRESHOLD 1 << 15 // 32KB
#define HASHLIST_INDEX MAX_DOWNLOADS //use the extra index for tracking the hash list file

// Enums
enum PATCH_CLIENT_STATE
{
	PATCH_CLIENT_DISCONNECTED,
	PATCH_CLIENT_WAITING_FOR_ACK,
	PATCH_CLIENT_RECEIVING_EXE_LIST,
	PATCH_CLIENT_RECEIVING_EXE_LIST_FILE,
	PATCH_CLIENT_RECEIVING_DATA_LIST,
	PATCH_CLIENT_CONNECTED,
	PATCH_CLIENT_RESTART,
	PATCH_CLIENT_ERROR
};


// Struct for download requests during exe patching
struct PATCH_CLIENT_EXE_REQUEST
{
	LPTSTR		fpath;
	LPTSTR		fname;
	TCHAR		tmp_fname[DEFAULT_FILE_WITH_PATH_SIZE];
	BOOL		bInPak;
	PAK_ENUM	pakType;
	UINT8		sha_hash[SHA1HashSize];
	UINT32		gentime_low;
	UINT32		gentime_high;
	BOOL		bCompress;

	PATCH_CLIENT_EXE_REQUEST() : fpath(NULL), fname(NULL), bInPak(FALSE), pakType(PAK_INVALID) {}

	PATCH_CLIENT_EXE_REQUEST& operator= (const PATCH_CLIENT_EXE_REQUEST& other)
	{
		this->fpath = other.fpath;
		this->fname = other.fname;
		this->bInPak = other.bInPak;
		this->pakType = other.pakType;
		this->gentime_high = other.gentime_high;
		this->gentime_low = other.gentime_low;
		MemoryCopy(this->sha_hash, SHA1HashSize, other.sha_hash, SHA1HashSize);
		PStrCopy(tmp_fname, other.tmp_fname, DEFAULT_FILE_WITH_PATH_SIZE);
		this->bCompress = other.bCompress;
		return *this;
	}
};

// Struct for regular download requests
struct PATCH_CLIENT_REQUEST
{
	ASYNC_DATA*						m_asyncData;
	FILE_INDEX_NODE*				m_pFINode;
	HANDLE							m_hEvent;
	DWORD							m_iTemp;
	CCriticalSection				m_csLock;

	// only non-null if it points to strings on the stack
	LPCTSTR							m_strFilePath;
	LPCTSTR							m_strFileName;

	// Only used for exe patching
	PATCH_CLIENT_EXE_REQUEST		m_exeRequestInfo;

	PATCH_CLIENT_REQUEST(BOOL bInitCS = TRUE) : m_asyncData(NULL), m_pFINode(NULL), m_hEvent(NULL), m_csLock(bInitCS), m_strFilePath(NULL), m_strFileName(NULL) {}

	PATCH_CLIENT_REQUEST& operator= (const PATCH_CLIENT_REQUEST& request) {
		this->m_asyncData = request.m_asyncData;
		this->m_hEvent = request.m_hEvent;
		this->m_pFINode = request.m_pFINode;
		this->m_iTemp = request.m_iTemp;
		this->m_exeRequestInfo = request.m_exeRequestInfo;
		this->m_strFileName = request.m_strFileName;
		this->m_strFilePath = request.m_strFilePath;
		return *this;
	}
};

static VOID sPatchClientFinishRequest(
	ASYNC_DATA* pAsyncData,
	UINT32 fileidx);

// Struct for the patch client context
struct PATCH_CLIENT
{
	PATCH_CLIENT_REQUEST			m_CurrentRequests[MAX_DOWNLOADS];
	PList<PATCH_CLIENT_REQUEST>		m_PendingRequests[NUM_ASYNC_PRIORITIES+1];
	PList<PATCH_CLIENT_EXE_REQUEST> m_exeRequests;
	PList<PATCH_CLIENT_EXE_REQUEST> m_exeFinishedRequests;
	CCriticalSection				m_csRequests;
	CCriticalSection				m_csInit;
	BOOL							m_bInitialized;
	MEMORYPOOL*						m_pPool;
	PATCH_CLIENT_STATE				m_iState;
	WCHAR							m_strProgress[2 * DEFAULT_FILE_WITH_PATH_SIZE];
	void						(*	m_waitFn)(void);
	DWORD							m_waitInterval;
	UINT32							m_iFilesNeeded;
	UINT32							m_iCurrentFilesCount;
	UINT32							m_iProgress;
	UINT32							m_LastAckedBytesArray[MAX_DOWNLOADS+1]; //extra index for hashlist patching
	TokenBucketEx					m_AckTokenBucket;
	struct {
		UINT32 sizeCompressed;
		UINT32 sizeOriginal;
		UINT32 sizeCurrent;
		void* pBufferCompressed;
		void* pBufferOriginal;
		BOOL bFile;
	} m_listBufferInfo;

	PATCH_CLIENT() : 
		m_bInitialized(FALSE), 
		m_pPool(NULL), 
		m_csRequests(TRUE), 
		m_csInit(TRUE),
		m_AckTokenBucket(PATCH_CLIENT_ACKS_PER_SECOND, PATCH_CLIENT_MAX_ACK_TOKEN_COUNT)
	{
		UINT32 i;

		m_exeRequests.Init();
		m_exeFinishedRequests.Init();
		m_pPool = NULL;
		m_strProgress[0] = L'\0';
		m_iProgress = 0;
		m_waitFn = NULL;
		m_waitInterval = 0;
		m_iFilesNeeded = 0;
		m_iCurrentFilesCount = 0;

		for (i = 0; i < arrsize(m_LastAckedBytesArray); ++i)
			m_LastAckedBytesArray[i] = 0;
		for (i = 0; i <= NUM_ASYNC_PRIORITIES; i++) {
			m_PendingRequests[i].Init();
		}
		Init();
	}
	~PATCH_CLIENT() {
		Free();
	}

	void Init() {
		CSAutoLock auto1(&m_csInit);
		if (m_bInitialized) {
			return;
		}
		m_iState = PATCH_CLIENT_DISCONNECTED;
		m_listBufferInfo.sizeCompressed = 0;
		m_listBufferInfo.sizeOriginal = 0;
		m_listBufferInfo.sizeCurrent = 0;
		m_listBufferInfo.pBufferCompressed = NULL;
		m_listBufferInfo.pBufferOriginal = NULL;
		m_bInitialized = TRUE;
	}

	void Free() {
		UINT32 i;

		CSAutoLock auto1(&m_csInit);
		if (m_bInitialized == FALSE) {
			return;
		}

		m_csRequests.Enter();

		PList<PATCH_CLIENT_REQUEST>::USER_NODE* pNode = NULL;
		for (i = 0; i < NUM_ASYNC_PRIORITIES; i++) {
			while ((pNode = m_PendingRequests[i].GetNext(pNode)) != NULL) {
				PATCH_CLIENT_REQUEST* pRequest = &pNode->Value;
				ASSERT_CONTINUE(pRequest != NULL);
				if (pRequest->m_exeRequestInfo.fname != NULL) {
					FREE(m_pPool, pRequest->m_exeRequestInfo.fname);
				}
				if (pRequest->m_exeRequestInfo.fpath != NULL) {
					FREE(m_pPool, pRequest->m_exeRequestInfo.fpath);
				}
				if (pRequest->m_hEvent == NULL) {
					FREE(m_pPool, pRequest->m_asyncData);
				}
			}
		}

		PList<PATCH_CLIENT_EXE_REQUEST>::USER_NODE* pNodeExe = NULL;
		while ((pNodeExe = m_exeRequests.GetNext(pNodeExe)) != NULL) {
			PATCH_CLIENT_EXE_REQUEST* pRequest = &pNodeExe->Value;
			if (pRequest->fname != NULL) {
				FREE(m_pPool, pRequest->fname);
			}
			if (pRequest->fpath != NULL) {
				FREE(m_pPool, pRequest->fpath);
			}
		}
		while ((pNodeExe = m_exeFinishedRequests.GetNext(pNodeExe)) != NULL) {
			PATCH_CLIENT_EXE_REQUEST* pRequest = &pNodeExe->Value;
			if (pRequest->fname != NULL) {
				FREE(m_pPool, pRequest->fname);
			}
			if (pRequest->fpath != NULL) {
				FREE(m_pPool, pRequest->fpath);
			}
		}

		m_csRequests.Leave();

		for (UINT32 i = 0; i < MAX_DOWNLOADS; i++) {
			sPatchClientFinishRequest(m_CurrentRequests[i].m_asyncData, i);
		}

		if (m_listBufferInfo.pBufferCompressed) {
			FREE(m_pPool, m_listBufferInfo.pBufferCompressed);
			m_listBufferInfo.pBufferCompressed = NULL;
		}
		if (m_listBufferInfo.pBufferOriginal) {
			FREE(m_pPool, m_listBufferInfo.pBufferOriginal);
			m_listBufferInfo.pBufferOriginal = NULL;
		}
		m_exeRequests.Destroy(m_pPool);
		m_exeFinishedRequests.Destroy(m_pPool);
		for (i = 0; i <= NUM_ASYNC_PRIORITIES; i++) {
			m_PendingRequests[i].Destroy(m_pPool);
		}
		m_iState = PATCH_CLIENT_DISCONNECTED;
		m_bInitialized = FALSE;
	}
};


// Globals
extern APP gApp;
PATCH_CLIENT gPatchClient;
static CCriticalSection sLockProgress(TRUE);
static CCriticalSection sLockInit(TRUE);
static LONG iErrorStatus = 0;

/*void PatchClientFree()
{
	gPatchClient.Free();
}*/

// Waits for a specific message from the server. Can wait indefinitely or fail after a timeout.
BOOL PatchClientWaitForMessage(
	PATCH_SERVER_RESPONSE_COMMANDS cmd,
	MSG_STRUCT* pMSG)
{
	MAILSLOT* mailslot = AppGetPatchMailSlot();
	ASSERT_RETFALSE(mailslot != NULL);

	MSG_BUF *buf, *head, *tail, *next;
	UINT32 count = MailSlotGetMessages(mailslot, head, tail);

	if (count == 0) {
		return FALSE;
	}

	buf = head;
	while (buf != NULL) {
		next = buf->next;
		if (buf->msg[0] == cmd) {
			MailSlotPushbackMessages(mailslot, next, tail);
			break;
		} else {
			MailSlotPushbackMessages(mailslot, buf, buf);
			buf = next;
		}
	}

	if (buf == NULL) {
		return FALSE;
	}

	BOOL result = TRUE;
	if (pMSG != NULL) {
		result = NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, buf->msg, buf->size, pMSG);
	}
	MailSlotRecycleMessages(mailslot, buf, buf);

	ASSERT_RETFALSE(result);
	return TRUE;
}

//----------------------------------------------------------------------------
// Track how many chunks we receive for a given file.
// Has timers for each index so we're not repeatedly re-sending
// on every single chunk.  Always send a completion.
//
// Funny name.  It updates the sending of download status updates. :)
//----------------------------------------------------------------------------
static BOOL sPatchClientUpdateSendStatusUpdateForFile(
	BYTE idx,
	DWORD receivedBytes,
	BOOL bFileDone, 
	BOOL bFullMsg)
{
	CHANNEL* pChannel = AppGetPatchChannel();
	ASSERT_RETFALSE(pChannel != NULL);
	ASSERT_RETFALSE(idx >= 0 && idx <= MAX_DOWNLOADS); //extra index for hashlist
	ASSERT(gPatchClient.m_LastAckedBytesArray[idx] < receivedBytes);
	bool bHitAckReceiveThreshold = receivedBytes - gPatchClient.m_LastAckedBytesArray[idx] >= ACK_RECEIVED_BYTE_THRESHOLD;

	// always ack file is done.  otherwise, ack if message was not full (a trickle message), or we've received more than 
	// the ack threshold, AND we have not hit the token bucket ceiling (which keeps us from DOSing the server).
	if (!bFileDone && bFullMsg && !bHitAckReceiveThreshold)
		return false;	// no reason to ack yet
	if (!bFileDone && !gPatchClient.m_AckTokenBucket.CanSend())
		return false; // hit ack token bucket limits, and we're not strictly required to ack yet
	gPatchClient.m_AckTokenBucket.DecrementTokenCount();
	gPatchClient.m_LastAckedBytesArray[idx] = receivedBytes;

	PATCH_SERVER_SEND_RECEIVED_FILE_STATUS_MSG msg;
	msg.idx = idx;
	msg.bCompleted = bFileDone != 0;
	msg.timeStamp = GetRealClockTime();
	msg.receivedBytes = receivedBytes;

	return ConnectionManagerSend(pChannel, &msg, PATCH_SERVER_SEND_RECEIVED_FILE_STATUS);
}

// Waits for data of a given size from the server
BOOL PatchClientWaitForData(
	UINT8* buffer,
	UINT32 total,
	UINT32& cur)
{
	DWORD size, offset, key_len;
	BYTE* pMsgData = 0;
	PATCH_SERVER_RESPONSE_FILE_DATA_SMALL_MSG msgDataSmall;
	PATCH_SERVER_RESPONSE_FILE_DATA_LARGE_MSG msgDataLarge;
	UINT8 IV[32];
	UINT8 key[ENCRYPTION_KEY_SIZE];
	
	key_len = ENCRYPTION_KEY_SIZE;
	ASSERT_RETFALSE(AuthTicketGetEncryptionKey(PATCH_SERVER, key, ENCRYPTION_KEY_SIZE, key_len));
	ASSERT_RETFALSE(key_len == ENCRYPTION_KEY_SIZE);
	memset(IV, 0, sizeof(IV));

	while (cur < total) {
		if (PatchClientWaitForMessage(PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, &msgDataSmall)) {
			size = MSG_GET_BLOB_LEN((&msgDataSmall), data);
			offset = msgDataSmall.offset;
			ASSERT_RETFALSE(CryptoDecryptBufferWithKey
				(CALG_AES_256, msgDataSmall.data, &size, key, ENCRYPTION_KEY_SIZE, IV));
			pMsgData = msgDataSmall.data;
		}
		else if (PatchClientWaitForMessage(PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, &msgDataLarge)) {
			size = MSG_GET_BLOB_LEN((&msgDataLarge), data);
			offset = msgDataLarge.offset;
			ASSERT_RETFALSE(CryptoDecryptBufferWithKey
				(CALG_AES_256, msgDataLarge.data, &size, key, ENCRYPTION_KEY_SIZE, IV));
			pMsgData = msgDataLarge.data;
		}
		else
			break;

		ASSERT_RETFALSE(MemoryCopy(buffer + offset, total - offset, pMsgData, size));
		cur += size;
	}
	return TRUE;
}

static BOOL sPatchClientInitExeRequestNext(BOOL bFirst);
static BOOL sPatchClientPostLoadHandleRequest(
	ASYNC_DATA* pAsyncData,
	UINT32 fileidx)
{
	PRESULT pr = S_OK;

	if (pAsyncData != NULL) {
		PAKFILE_LOAD_SPEC* pSpec = (PAKFILE_LOAD_SPEC*)pAsyncData->m_UserData;

		if (pAsyncData->m_Bytes == 0 || pAsyncData->m_Bytes != pAsyncData->m_IOSize) {
			if (pAsyncData->m_Buffer != NULL) {
				FREE(pAsyncData->m_Pool, pAsyncData->m_Buffer);
			}
			pAsyncData->m_IOSize = 0;
		} else {
			// If it's a compressed data
			if (gPatchClient.m_CurrentRequests[fileidx].m_iTemp > 0) {
				void* pBuffer = MALLOC(pAsyncData->m_Pool, pAsyncData->m_Bytes);
				ASSERT_GOTO(pBuffer != NULL, _err2);

				gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.bCompress = TRUE;
				UINT32 size = gPatchClient.m_CurrentRequests[fileidx].m_iTemp;
				MemoryCopy(pBuffer, pAsyncData->m_Bytes, pAsyncData->m_Buffer, pAsyncData->m_Bytes);
				ASSERT_GOTO(ZLIB_INFLATE((UINT8*)pBuffer, pAsyncData->m_Bytes, (UINT8*)pAsyncData->m_Buffer, &size), _err2);
				ASSERT_GOTO(size == gPatchClient.m_CurrentRequests[fileidx].m_iTemp, _err2);

				//			TracePersonal("uncompressing %s%s full %d compressed %d\n", pSpec->strFilePath.str, pSpec->strFileName.str, size, pSpec->size_compressed);

				if (pSpec != NULL) {
					pSpec->buffer_compressed = pBuffer;
					pSpec->size_compressed = pAsyncData->m_IOSize;
				} else {
					FREE(pAsyncData->m_Pool, pBuffer);
				}

				pAsyncData->m_IOSize = size;
				pAsyncData->m_Bytes = size;

				goto _next;

_err2:
				if (pBuffer != NULL) {
					FREE(pAsyncData->m_Pool, pBuffer);
				}
				pAsyncData->m_IOSize = 0;
			} else {
				gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.bCompress = FALSE;
			}
		}
_next:
		if (pSpec != NULL) {
			pSpec->bytesread = pAsyncData->m_IOSize;
			//	pSpec->bytestoread = pAsyncData->m_Bytes;
			//	pSpec->size = pAsyncData->m_Bytes;
			if (pAsyncData->m_IOSize != 0) {
				pSpec->frompak = TRUE;
			}

			SYSTEMTIME sysTime;
			FILETIME fileTime;
			GetSystemTime(&sysTime);
			SystemTimeToFileTime(&sysTime, &fileTime);
			pSpec->gentime = ((UINT64)(fileTime.dwHighDateTime) << 32) | (UINT64)(fileTime.dwLowDateTime);
		}

		// Call the callback function for the file io thread
		if (pAsyncData->m_fpFileIOThreadCallback != NULL) {
			V_SAVE( pr, pAsyncData->m_fpFileIOThreadCallback(*pAsyncData) );
		}
	}

	if (gPatchClient.m_CurrentRequests[fileidx].m_hEvent != NULL) {
		// If there's a wait event then it's a blocking request
		SetEvent(gPatchClient.m_CurrentRequests[fileidx].m_hEvent);
	} else {
		// It's non-blocking request
		if (gPatchClient.m_iState != PATCH_CLIENT_RECEIVING_EXE_LIST_FILE &&
			pAsyncData) {
				AsyncFilePostRequestForCallback(pAsyncData);
		}
	}

	if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_EXE_LIST_FILE) {
		CSAutoLock autolock(&gPatchClient.m_csRequests);
		ASSERT_RETFALSE(pAsyncData != NULL);
		ASSERT_RETFALSE(pAsyncData->m_IOSize == pAsyncData->m_Bytes);

		UINT8 sha_hash[SHA1HashSize];
//		TCHAR tmp_fpath[DEFAULT_FILE_WITH_PATH_SIZE+1];

		ASSERT_RETFALSE(SHA1Calculate((UINT8*)pAsyncData->m_Buffer, pAsyncData->m_IOSize, sha_hash));
		ASSERT_RETFALSE(memcmp(sha_hash, gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.sha_hash, SHA1HashSize) == 0);

		if (CreateDirectory(_T(".\\tmp\\"), NULL) == FALSE) {
			DWORD dwError = GetLastError();
			ASSERT_RETFALSE(dwError == ERROR_ALREADY_EXISTS);
		}
//		ASSERT_RETFALSE(GetTempPath(DEFAULT_FILE_WITH_PATH_SIZE, tmp_fpath));
		ASSERT_RETFALSE(GetTempFileName(_T(".\\tmp\\"), _T("pc"), 0, gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.tmp_fname));
		DeleteFile(gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.tmp_fname);

		ASYNC_FILE* hFile = AsyncFileOpen(gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo.tmp_fname, FF_WRITE | FF_CREATE_NEW);
		ASSERT_RETFALSE(hFile != NULL);

		ASYNC_DATA data(hFile, pAsyncData->m_Buffer, 0, pAsyncData->m_IOSize, NULL);
		ASSERT_RETFALSE(AsyncFileWriteNow(data) == pAsyncData->m_IOSize);
		AsyncFileCloseNow(hFile);
		FREE(pAsyncData->m_Pool, pAsyncData->m_Buffer);
		FREE(pAsyncData->m_Pool, pAsyncData);

		gPatchClient.m_exeFinishedRequests.PListPushTailPool(gPatchClient.m_pPool, gPatchClient.m_CurrentRequests[fileidx].m_exeRequestInfo);

		InterlockedIncrement((LONG*)(&gPatchClient.m_iCurrentFilesCount));
		ASSERT_RETFALSE(sPatchClientInitExeRequestNext(FALSE));
	}
	return TRUE;
}


static BOOL sPatchClientRequestFile(
	LPCTSTR fpath,
	LPCTSTR fname,
	UINT32 index)
{
	CHANNEL* pChannel = AppGetPatchChannel();
	ASSERT_RETFALSE(pChannel != NULL);

	ASSERT(fname != NULL);

	PATCH_SERVER_GET_FILE_MSG msgGetFile;
	if (fpath == NULL) {
		msgGetFile.fpath[0] = '\0';
	} else {
		PStrCopy(msgGetFile.fpath, fpath, DEFAULT_FILE_WITH_PATH_SIZE);
	}
	PStrCopy(msgGetFile.fname, fname, DEFAULT_FILE_WITH_PATH_SIZE);
	msgGetFile.idx = (BYTE)index;
	return ConnectionManagerSend(pChannel, &msgGetFile, PATCH_SERVER_GET_FILE);
}

static BOOL sPatchClientRequestFile(
	UINT32 index)
{
	ASSERT_RETFALSE(index < MAX_DOWNLOADS);
	LPCTSTR fpath, fname;

	ASYNC_DATA* pAsyncData = gPatchClient.m_CurrentRequests[index].m_asyncData;
	ASSERT_RETFALSE(pAsyncData != NULL);

	FILE_INDEX_NODE* pFINode = gPatchClient.m_CurrentRequests[index].m_pFINode;
	if (pFINode != NULL) {
		fpath = pFINode->m_FilePath.str;
		fname = pFINode->m_FileName.str;
	} else {
		fpath = gPatchClient.m_CurrentRequests[index].m_exeRequestInfo.fpath;
		fname = gPatchClient.m_CurrentRequests[index].m_exeRequestInfo.fname;
		if (fname == NULL) {
			fpath = gPatchClient.m_CurrentRequests[index].m_strFilePath;
			fname = gPatchClient.m_CurrentRequests[index].m_strFileName;
		}
	}

//	ASSERT_RETFALSE(pFINode != NULL);

	if (sPatchClientRequestFile(fpath, fname, index)) {
		return TRUE;
	} else {
		sPatchClientPostLoadHandleRequest(pAsyncData, index);
		return FALSE;
	}
}

static BOOL sPatchClientUpdateRequest()
{
	CSAutoLock autoLock(&gPatchClient.m_csRequests);

	if (gPatchClient.m_CurrentRequests[0].m_asyncData != NULL) {
		// There is a synchronous file loading going on, don't do anything

	} else if (gPatchClient.m_CurrentRequests[0].m_asyncData == NULL &&
		gPatchClient.m_PendingRequests[NUM_ASYNC_PRIORITIES].Count() > 0) {
		// There is a synchronous file request in queue, request only that
		gPatchClient.m_PendingRequests[NUM_ASYNC_PRIORITIES].PopHead(
			gPatchClient.m_CurrentRequests[0]);

		ASSERT_RETFALSE(sPatchClientRequestFile(0));
	} else for (UINT32 i = 1; i < MAX_DOWNLOADS; i++) {
		if (gPatchClient.m_CurrentRequests[i].m_asyncData == NULL) {
			for (INT32 j = NUM_ASYNC_PRIORITIES - 1; j >= 0; j--) {
				if (gPatchClient.m_PendingRequests[j].Count() > 0) {
					gPatchClient.m_PendingRequests[j].PopHead(
						gPatchClient.m_CurrentRequests[i]);
					ASSERT_RETFALSE(sPatchClientRequestFile(i));
					break;
				}
			}
		}
	}
	return TRUE;
}


void ConsoleStringForceOpen(
	DWORD dwColor,
	const WCHAR * format,
	...);

static VOID sPatchClientFinishRequest(
	ASYNC_DATA* pAsyncData,
	UINT32 fileidx)
{
	if (!sPatchClientPostLoadHandleRequest(pAsyncData, fileidx)) {
		gPatchClient.m_iState = PATCH_CLIENT_ERROR;
		return;
	}

	gPatchClient.m_csRequests.Enter();

	gPatchClient.m_CurrentRequests[fileidx].m_asyncData = NULL;
	gPatchClient.m_CurrentRequests[fileidx].m_hEvent = NULL;
	gPatchClient.m_CurrentRequests[fileidx].m_pFINode = NULL;
	gPatchClient.m_csRequests.Leave();

	sPatchClientUpdateRequest();
}


static VOID sPatchClientDataHandler(
	BYTE idx,
	DWORD uOffset,
	BYTE* pData,
	UINT32 uDataSize,
	UINT32 uMaxDataSize)
{
	ASSERT_RETURN(idx < MAX_DOWNLOADS);
	ASSERT(uDataSize <= uMaxDataSize);

	gPatchClient.m_CurrentRequests[idx].m_csLock.Enter();
	ASYNC_DATA* pAsyncData = gPatchClient.m_CurrentRequests[idx].m_asyncData;
	ASSERT_GOTO(pAsyncData != NULL, _err);
	ASSERT_GOTO(pAsyncData->m_Bytes > uOffset, _err);

	MemoryCopy((UINT8*)pAsyncData->m_Buffer + uOffset,
		pAsyncData->m_Bytes - uOffset,
		pData, uDataSize);

	pAsyncData->m_IOSize += uDataSize;
	ASSERT_GOTO(pAsyncData->m_IOSize <= pAsyncData->m_Bytes, _err);


#if 1
	TCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE];
	TCHAR progress[DEFAULT_FILE_WITH_PATH_SIZE * 2];
	UINT32 iProgress;
	PAKFILE_LOAD_SPEC* pSpec = (PAKFILE_LOAD_SPEC*)pAsyncData->m_UserData;
	
	fname[0] = 0;
	if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_EXE_LIST_FILE) {
		if (gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fpath) {
			if( AppIsHellgate() )
			{
				PStrPrintf(fname, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s\n",
					gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fpath,
					gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fname);
			}
			else
			{
				PStrPrintf(fname, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s",
					gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fpath,
					gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fname);
			}
		} else {
			PStrRemoveEntirePath( fname, DEFAULT_FILE_WITH_PATH_SIZE, gPatchClient.m_CurrentRequests[idx].m_exeRequestInfo.fname );
		}
	} else {
		if (pSpec && pSpec->filename) {
			PStrRemoveEntirePath( fname, DEFAULT_FILE_WITH_PATH_SIZE, pSpec->filename );
		}
	}

	iProgress = (100 * pAsyncData->m_IOSize) / pAsyncData->m_Bytes;

	WCHAR progressPct[DEFAULT_FILE_WITH_PATH_SIZE], progressAmt[10];
	TCHAR szProgressPct[DEFAULT_FILE_WITH_PATH_SIZE];

	PIntToStr(progressAmt, arrsize(progressAmt), iProgress);
	PStrCopy(progressPct, GlobalStringGet(GS_FORMAT_PERCENT), arrsize(progressPct));
	PStrReplaceToken(progressPct, arrsize(progressPct), StringReplacementTokensGet(SR_STRING1), progressAmt);
	PStrCvt(szProgressPct, progressPct, DEFAULT_FILE_WITH_PATH_SIZE);
	PStrPrintf(progress, DEFAULT_FILE_WITH_PATH_SIZE*2, _T("%s - %s"), fname, szProgressPct);

	if (AppIsHellgate() || AppIsTugboat()) {
		CSAutoLock auto1(&sLockProgress);
		PStrCvt(gPatchClient.m_strProgress, progress, 2 * DEFAULT_FILE_WITH_PATH_SIZE);
		if (iProgress == 100) {
			trace("%s\n", progress);
		}
		gPatchClient.m_iProgress = iProgress;
	}
#else
	static UINT32 dlFileCount = 0;
	UINT32 iTmp;
	TCHAR progress[DEFAULT_FILE_WITH_PATH_SIZE];

	iTmp = InterlockedIncrement((LONG*)(&dlFileCount));
	PStrPrintf(progress, DEFAULT_FILE_WITH_PATH_SIZE, _T("Downloaded File #%d"), iTmp);
	if (AppIsHellgate()) {

	}
	if (AppIsTugboat()) {
		CSAutoLock auto1(&sLockProgress);
		PStrCvt(gPatchClient.strProgress, progress, 2*DEFAULT_FILE_WITH_PATH_SIZE);
	}
#endif

	sPatchClientUpdateSendStatusUpdateForFile(
		idx, 
		pAsyncData->m_IOSize,
		pAsyncData->m_IOSize == pAsyncData->m_Bytes,
		uDataSize == uMaxDataSize);

	if (pAsyncData->m_IOSize == pAsyncData->m_Bytes) {
		gPatchClient.m_CurrentRequests[idx].m_csLock.Leave();
		sPatchClientFinishRequest(pAsyncData, idx);
	} else {
		gPatchClient.m_CurrentRequests[idx].m_csLock.Leave();
	}

	return;
_err:
	gPatchClient.m_CurrentRequests[idx].m_csLock.Leave();
	sPatchClientFinishRequest(pAsyncData, idx);
}


static VOID sPatchClientDataSmallHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 msg_size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	PATCH_SERVER_RESPONSE_FILE_DATA_SMALL_MSG msgDataSmall;
	ASSERT_RETURN(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, msg_size, &msgDataSmall));
	sPatchClientDataHandler(msgDataSmall.idx, msgDataSmall.offset, msgDataSmall.data, 
		MSG_GET_BLOB_LEN(&msgDataSmall, data), arrsize(msgDataSmall.data));
}


static VOID sPatchClientDataLargeHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 msg_size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	PATCH_SERVER_RESPONSE_FILE_DATA_LARGE_MSG msgDataLarge;
	ASSERT_RETURN(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, msg_size, &msgDataLarge));
	sPatchClientDataHandler(msgDataLarge.idx, msgDataLarge.offset, msgDataLarge.data, 
		MSG_GET_BLOB_LEN(&msgDataLarge, data), arrsize(msgDataLarge.data));
}


static VOID sPatchClientHeaderHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	ASYNC_DATA* pAsyncData = NULL;
	PATCH_SERVER_RESPONSE_FILE_HEADER_MSG msgHeader;
	ASSERT_GOTO(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, size, &msgHeader), _err2);
	ASSERT_GOTO(msgHeader.idx < MAX_DOWNLOADS, _err2);

	gPatchClient.m_CurrentRequests[msgHeader.idx].m_csLock.Enter();
	gPatchClient.m_LastAckedBytesArray[msgHeader.idx] = 0;
	pAsyncData = gPatchClient.m_CurrentRequests[msgHeader.idx].m_asyncData;

	ASSERT_GOTO(pAsyncData != NULL, _err);
	ASSERT_GOTO(pAsyncData->m_Bytes == 0, _err);
	ASSERT_GOTO(pAsyncData->m_IOSize == 0, _err);
	ASSERT_GOTO(pAsyncData->m_Buffer == NULL, _err);

	ASSERT_GOTO(msgHeader.filesize_compressed < msgHeader.filesize, _err);
	if (msgHeader.filesize_compressed == 0) {
		// Not compressed
		pAsyncData->m_Bytes = msgHeader.filesize;
		gPatchClient.m_CurrentRequests[msgHeader.idx].m_iTemp = 0;
	} else {
		// compressed
		pAsyncData->m_Bytes = msgHeader.filesize_compressed;
		gPatchClient.m_CurrentRequests[msgHeader.idx].m_iTemp = msgHeader.filesize;
	}
	pAsyncData->m_GentimeHigh = msgHeader.gentime_high;
	pAsyncData->m_GentimeLow = msgHeader.gentime_low;

	pAsyncData->m_Buffer = MALLOC(pAsyncData->m_Pool, msgHeader.filesize);
	ASSERT_GOTO(pAsyncData->m_Buffer != NULL, _err);
	gPatchClient.m_CurrentRequests[msgHeader.idx].m_csLock.Leave();
	return;

_err: 
	gPatchClient.m_CurrentRequests[msgHeader.idx].m_csLock.Leave();
_err2:
	sPatchClientFinishRequest(pAsyncData, msgHeader.idx);
}

static VOID sPatchClientErrorHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	PATCH_SERVER_RESPONSE_FILE_NOT_FOUND_MSG msgError;
	ASSERT_RETURN(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, size, &msgError));

	ASSERT_RETURN(msgError.idx < MAX_DOWNLOADS);

	ASYNC_DATA* pAsyncData = gPatchClient.m_CurrentRequests[msgError.idx].m_asyncData;
	ASSERT_RETURN(pAsyncData != NULL);

	sPatchClientFinishRequest(pAsyncData, msgError.idx);
}


extern SIMPLE_DYNAMIC_ARRAY<PAKFILE_DESC> g_PakFileDesc;
static VOID sPatchClientPakFileInfoHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	PATCH_SERVER_RESPONSE_PAKFILE_INFO_MSG msgInfo;
	ASSERT_RETURN(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, size, &msgInfo));

	PAKFILE_DESC desc;
	desc.m_Index = (PAK_ENUM)msgInfo.iIndex;
	desc.m_Count = msgInfo.iCount;
//	desc.m_MaxIndex = msgInfo.iMaxIndex;
	desc.m_AlternativeIndex = (PAK_ENUM)msgInfo.iAlternativeIndex;
	desc.m_Create = msgInfo.bCreate;
	desc.m_GenPakId = 0;
	PStrCopy(desc.m_BaseName, msgInfo.strBaseName, DEFAULT_FILE_WITH_PATH_SIZE);
	PStrPrintf(desc.m_BaseNameX, DEFAULT_FILE_WITH_PATH_SIZE, "x_%s", msgInfo.strBaseName);
	ASSERT_RETURN((UINT32)desc.m_Index <= g_PakFileDesc.Count());

	BOOL bCreate = FALSE;
	if ((UINT32)desc.m_Index == g_PakFileDesc.Count()) {
		ArrayAddItem(g_PakFileDesc, desc);
		if (desc.m_Create) {
			bCreate = TRUE;
		}
	} else {
		ASSERT_RETURN(g_PakFileDesc[desc.m_Index].m_Index == desc.m_Index);
		if (desc.m_Create) {
			if (!g_PakFileDesc[desc.m_Index].m_Create) {
				bCreate = TRUE;
			} else if (PStrICmp(desc.m_BaseNameX, g_PakFileDesc[desc.m_Index].m_BaseNameX) != 0) {
				bCreate = TRUE;
			}
		}
	}

#if ISVERSION(CLIENT_ONLY_VERSION)
	if (bCreate && PStrLen(desc.m_BaseNameX) != 0) {
		UINT32 idPakFile = PakFileCreateNewPakfile(FILE_PATH_PAK_FILES, desc.m_BaseNameX, desc.m_Index);
		ASSERT_RETURN(idPakFile != INVALID_ID);

		g_PakFileDesc[desc.m_Index].m_Create = TRUE;
		g_PakFileDesc[desc.m_Index].m_GenPakId = idPakFile;
	}
#endif
}


static void sPatchClientQueuePositionHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);

	PATCH_SERVER_RESPONSE_QUEUE_POSITION_MSG msgQueuePos;
	ASSERT_RETURN(NetTranslateMessageForRecv(gApp.m_PatchServerCommandTable, data, size, &msgQueuePos));

	CSAutoLock autoLock(&sLockProgress);
	PStrPrintf(
		gPatchClient.m_strProgress,
		2 * DEFAULT_FILE_WITH_PATH_SIZE,
		GlobalStringGet(GS_PATCH_QUEUE_TEXT),
		msgQueuePos.nQueuePosition,
		msgQueuePos.nQueueLength );
}


static UINT8* sPatchClientInitParse1(
	UINT8* buffer,
	LPTSTR fpath,
	UINT32& fpath_len,
	LPTSTR fname,
	UINT32& fname_len,
	UINT8* sha_hash,
	PAK_ENUM& pakType,
	UINT32& cur_len)
{
	if (buffer == NULL || *buffer == 0) {
		return NULL;
	}
	ASSERT_RETNULL(fpath != NULL);
	ASSERT_RETNULL(fname != NULL);
	ASSERT_RETNULL(sha_hash != NULL);

	PStrCvt(fpath, (WCHAR*)buffer, DEFAULT_FILE_WITH_PATH_SIZE);
	fpath_len = PStrLen(fpath);
	buffer += (fpath_len+1) * sizeof(WCHAR);
	cur_len += (fpath_len+1) * sizeof(WCHAR);

	PStrCvt(fname, (WCHAR*)buffer, DEFAULT_FILE_WITH_PATH_SIZE);
	fname_len = PStrLen(fname);
	buffer += (fname_len+1) * sizeof(WCHAR);
	cur_len += (fname_len+1) * sizeof(WCHAR);

	pakType = (PAK_ENUM)ntohl(*((UINT32*)buffer));
	buffer += sizeof(UINT32);
	cur_len += sizeof(UINT32);

	MemoryCopy(sha_hash, SHA1HashSize, buffer, SHA1HashSize);
	buffer += SHA1HashSize;
	cur_len += SHA1HashSize;

	return buffer;
}

static UINT8* sPatchClientInitParse2(
	UINT8* buffer,
	LPTSTR fname,
	UINT32& fname_len,
	UINT8* sha_hash,
	UINT32& cur_len)
{
	if (buffer == NULL || *buffer == 0) {
		return NULL;
	}
	ASSERT_RETNULL(fname != NULL);
	ASSERT_RETNULL(sha_hash != NULL);

	PStrCvt(fname, (WCHAR*)buffer, DEFAULT_FILE_WITH_PATH_SIZE);
	fname_len = PStrLen(fname);
	buffer += (fname_len+1) * sizeof(WCHAR);
	cur_len += (fname_len+1) * sizeof(WCHAR);

	MemoryCopy(sha_hash, SHA1HashSize, buffer, SHA1HashSize);
	buffer += SHA1HashSize;
	cur_len += SHA1HashSize;

	return buffer;
}

static BOOL sPatchClientInitSKUExcludesStringTable(
	LPCSTR fpath)
{
	ASSERT_RETFALSE(fpath != NULL);

	int nSKU = SKUGetCurrent();
	ASSERTX_RETFALSE(INVALID_LINK != nSKU, "Expected current SKU");

	unsigned prefixLen = 0;

	ONCE {
		prefixLen = PStrLen(FILE_PATH_STRINGS);
		if (PStrICmp(fpath, FILE_PATH_STRINGS, prefixLen) == 0) {
			break;
		}

		prefixLen = PStrLen(FILE_PATH_STRINGS_COMMON);
		if (PStrICmp(fpath, FILE_PATH_STRINGS_COMMON, prefixLen) == 0) {
			break;
		}

		prefixLen = PStrLen(".\\language\\");
		if (PStrICmp(fpath, ".\\language\\", prefixLen) == 0) {
			break;
		}

		return FALSE;
	}

	if (fpath[prefixLen] == '\0') {
		return FALSE;
	}

	char language[MAX_PATH];
	PStrCopy(language, fpath + prefixLen, MAX_PATH);
	PStrReplaceChars(language, '/', '\0');
	PStrReplaceChars(language, '\\', '\0');

	LANGUAGE eLang = LanguageGetByName(AppGameGet(), language);
	ASSERTX_RETFALSE(eLang != LANGUAGE_INVALID, "Invalid language in string table path name");

	return !SKUContainsLanguage(nSKU, eLang, FALSE);
}

static BOOL sPatchClientInitDataParseBuffer(UINT8* buffer, UINT32 total_len)
{
	TCHAR fpath[DEFAULT_FILE_WITH_PATH_SIZE+1];
	TCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	UINT8 sha_hash[SHA1HashSize];
	PStrT fpath_str, fname_str;
	UINT32 fpath_len, fname_len;
	UINT32 count, i, len;
	PAK_ENUM pakType;
	FILE_INDEX_NODE* pFINode;

	ASSERT_RETFALSE(buffer != NULL);
	count = ntohl(*((UINT32*)buffer));
	buffer += sizeof(UINT32);
	len = 0;
	for (i = 0; i < count; i++) {
		fpath_len = DEFAULT_FILE_WITH_PATH_SIZE;
		fname_len = DEFAULT_FILE_WITH_PATH_SIZE;
		buffer = sPatchClientInitParse1(buffer, fpath, fpath_len, fname, fname_len, sha_hash, pakType, len);
		ASSERT(len <= total_len);
		fpath_str.Set(fpath, fpath_len);
		fname_str.Set(fname, fname_len);
		pFINode = PakFileLoadGetIndex(fpath_str, fname_str);
		if (pFINode == NULL) {
			pFINode = PakFileAddToHash(fpath_str, fname_str, TRUE);
			ASSERT_RETFALSE(pFINode != NULL);
			pFINode->m_fileSize = 0;
			pFINode->m_iSvrPakType = pakType;
		} else {
			pFINode->m_iSvrPakType = pakType;
//			ASSERT_CONTINUE(pFINode->m_idPakFile == pakfileid);
		}
		pFINode->m_bHasSvrHash = TRUE;
		pFINode->m_SvrHash = sha_hash;
	}

	return TRUE;
}

static BOOL sPatchClientInitExeParseBuffer(
	UINT8* buffer,
	UINT32 total_len,
	PList<PATCH_CLIENT_EXE_REQUEST>& listRequests,
	MEMORYPOOL* pPool)
{
	UINT32 disk_count, pak_count, cur_len, i, fname_len, fpath_len;
	PAK_ENUM pakType;
	TCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	TCHAR fpath[DEFAULT_FILE_WITH_PATH_SIZE+1];
	UINT8 sha_hash[SHA1HashSize];
	UINT8* tmp;
	BOOL bNeedFile;
	HANDLE hFile;
	FILE_INDEX_NODE* pFINode;
	PStrT fname_str, fpath_str;

	disk_count = ntohl(((UINT32*)buffer)[0]);
	pak_count = ntohl(((UINT32*)buffer)[1]);
	tmp = buffer + 2 * sizeof(UINT32);
	cur_len = 2 * sizeof(UINT32);

	for (i = 0; i < disk_count; i++) {
		tmp = sPatchClientInitParse2(tmp, fname, fname_len, sha_hash, cur_len);
		ASSERT_RETFALSE(cur_len <= total_len);

		TraceDownload("[EXE Request] Checking %s", fname);
		if (sPatchClientInitSKUExcludesStringTable(fname)) {
			continue;
		}

		bNeedFile = FALSE;
		hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
			TraceDownload("[EXE Request]       Client does not have file\n");

			bNeedFile = TRUE;
		} else {
			UINT8 sha_hash2[SHA1HashSize];
			memset(sha_hash2, 0, SHA1HashSize);

			if (SHA1GetFileSHA(hFile, sha_hash2)) {
				if (memcmp(sha_hash, sha_hash2, SHA1HashSize) != 0) {
					bNeedFile = TRUE;
				}
				TCHAR hashClt[SHA1HashSize*2+1];
				TCHAR hashSvr[SHA1HashSize*2+1];
				for (UINT32 i = 0; i < SHA1HashSize; i++) {
					UINT8 val = sha_hash2[i];
					PStrPrintf(hashClt+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
					val = sha_hash[i];
					PStrPrintf(hashSvr+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
				}
				TraceDownload("[EXE Request]       Client %s Server %s\r\n", hashClt, hashSvr);
			} else {
				bNeedFile = TRUE;
			}
		}
		CloseHandle(hFile);
		if (bNeedFile) {
			PATCH_CLIENT_EXE_REQUEST request;
			request.bInPak = FALSE;
			request.fpath = NULL;
			request.tmp_fname[0] = NULL;
			request.pakType = PAK_INVALID;
			MemoryCopy(request.sha_hash, SHA1HashSize, sha_hash, SHA1HashSize);

			request.fname = (LPTSTR)MALLOC(pPool, sizeof(TCHAR)*(fname_len+1));
			ASSERT_RETFALSE(request.fname != NULL);
			PStrCopy(request.fname, fname_len+1, fname, fname_len);
			listRequests.PListPushTailPool(pPool, request);
		}
	}
	for (i = 0; i < pak_count; i++) {
		tmp = sPatchClientInitParse1(tmp, fpath, fpath_len, fname, fname_len, sha_hash, pakType, cur_len);
		ASSERT_RETFALSE(cur_len <= total_len);

		if (sPatchClientInitSKUExcludesStringTable(fpath)) {
			continue;
		}

		bNeedFile = FALSE;
		fname_str.Set(fname, fname_len);
		fpath_str.Set(fpath, fpath_len);
		pFINode = PakFileLoadGetIndex(fpath_str, fname_str);
		if (pFINode == NULL) {
			TraceDownload("[EXE Request] --- %s%s", fpath, fname);
			TraceDownload("[EXE Request]       Client does not have file\n");

			bNeedFile = TRUE;
		} else if (memcmp(pFINode->m_Hash.m_Hash, sha_hash, SHA1HashSize) != 0) {
			TraceDownload("[EXE Request] --- %s%s", fpath, fname);

			TCHAR hashClt[SHA1HashSize*2+1];
			TCHAR hashSvr[SHA1HashSize*2+1];
			for (UINT32 i = 0; i < SHA1HashSize; i++) {
				UINT8 val = pFINode->m_Hash.m_Hash[i];
				PStrPrintf(hashClt+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
				val = sha_hash[i];
				PStrPrintf(hashSvr+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
			}
			TraceDownload("[EXE Request]       Client %s Server %s\r\n", hashClt, hashSvr);





			bNeedFile = TRUE;
		}
		if (bNeedFile) {
			PATCH_CLIENT_EXE_REQUEST request;
			request.bInPak = TRUE;
			request.tmp_fname[0] = NULL;
			request.pakType = pakType;
			MemoryCopy(request.sha_hash, SHA1HashSize, sha_hash, SHA1HashSize);

			request.fname = (LPTSTR)MALLOC(pPool, sizeof(TCHAR)*(fname_len+1));
			ASSERT_RETFALSE(request.fname != NULL);
			PStrCopy(request.fname, fname_len+1, fname, fname_len);

			request.fpath = (LPTSTR)MALLOC(pPool, sizeof(TCHAR)*(fpath_len+1));
			ASSERT_GOTO(request.fpath != NULL, _err);
			PStrCopy(request.fpath, fpath_len+1, fpath, fpath_len);

			listRequests.PListPushTailPool(pPool, request);
			continue;
_err:
			FREE(pPool, request.fname);
			return FALSE;
		}
	}
	ASSERT(cur_len == total_len);
	return TRUE;
}

void* PatchClientRequestFileNow(
	LPCTSTR fpath,
	LPCTSTR fname,
	MEMORYPOOL* pPool,
	UINT32& filesize,
	UINT32& gentimeHi,
	UINT32& gentimeLo);


static void sPatchClientListDestroy(
	MEMORYPOOL* pPool,
	PATCH_CLIENT_EXE_REQUEST request)
{
	if (request.fname != NULL) {
		FREE(pPool, request.fname);
	}
	if (request.fpath != NULL) {
		FREE(pPool, request.fpath);
	}
	if (request.tmp_fname != NULL) {
		FREE(pPool, request.tmp_fname);
	}
}

static BOOL sPatchClientInitExeListReplaceFiles(
	PList<PATCH_CLIENT_EXE_REQUEST>& listRequests,
	TCHAR* tmp_fname,
	MEMORYPOOL* pPool)
{
	TCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	PList<PATCH_CLIENT_EXE_REQUEST>::USER_NODE* pNode;
	PATCH_CLIENT_EXE_REQUEST* pRequest;
	void* pBuffer = NULL;
	UINT64 max_size = 0, size, count = 0;
	CMarkup markup;
	
	DeleteFile(tmp_fname);
	for(pNode = listRequests.GetNext(NULL);
		pNode != NULL;
		pNode = listRequests.GetNext(pNode)) {
		pRequest = &pNode->Value;
		if (!pRequest->bInPak) {
			ASSERT_CONTINUE(pRequest->fpath == NULL);
			ASSERT_CONTINUE(pRequest->fname != NULL);
			PStrCopy(fname, pRequest->fname, DEFAULT_FILE_WITH_PATH_SIZE);

			if (!MoveFile(fname, tmp_fname)) {
				trace("MoveFile1: %d\n", GetLastError());
			}
//			ASSERT_CONTINUE(MoveFile(pRequest->tmp_fname, fname));
			if (!MoveFile(pRequest->tmp_fname, fname)) {
				trace("MoveFile2: %d\n", GetLastError());
				continue;
			}
			if (!MoveFile(tmp_fname, pRequest->tmp_fname)) {
				trace("MoveFile3: %d\n", GetLastError());
			}

			ASSERT_CONTINUE(markup.AddElem("delete"));
			ASSERT_CONTINUE(markup.SetData(pRequest->tmp_fname));
			count++;
//			trace("replaced %s\n", pRequest->fname);

			FREE(gPatchClient.m_pPool, pRequest->fname);
			pRequest->fname = NULL;
			pRequest->fpath = NULL;
		} else {
			ASSERT_CONTINUE(pRequest->fpath != NULL);
			ASSERT_CONTINUE(pRequest->fname != NULL);

			PStrPrintf(fname, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s",
				pRequest->fpath, pRequest->fname);

			ASYNC_FILE* pFile = AsyncFileOpen(pRequest->tmp_fname, FF_READ | FF_SHARE_READ);
			ASSERT_CONTINUE(pFile != NULL);

			size = AsyncFileGetSize(pFile);
			ASSERT((size & 0xFFFFFFFF00000000) == 0);
			if (size > max_size) {
				max_size = size;
				if (pBuffer != NULL) {
					FREE(pPool, pBuffer);
				}
				pBuffer = MALLOC(pPool, (UINT32)max_size);
				ASSERT_GOTO(pBuffer != NULL, _err);
			}

			ASYNC_DATA data(pFile, pBuffer, 0, (UINT32)size, 0);
			ASSERT_CONTINUE(AsyncFileReadNow(data) == size);
			AsyncFileCloseNow(pFile);
			DeleteFile(pRequest->tmp_fname);

			UINT8 sha_hash[SHA1HashSize];
			ASSERT_CONTINUE(SHA1Calculate((UINT8*)pBuffer, (UINT32)size, sha_hash));
			ASSERT_CONTINUE(memcmp(pRequest->sha_hash, sha_hash, SHA1HashSize) == 0);

			SYSTEMTIME sysTime;
			FILETIME fileTime;
			LARGE_INTEGER gen;
			GetSystemTime(&sysTime);
			SystemTimeToFileTime(&sysTime, &fileTime);
			gen.HighPart = fileTime.dwHighDateTime;
			gen.LowPart = fileTime.dwLowDateTime;

			DECLARE_LOAD_SPEC(loadSpec, fname);
			loadSpec.buffer = pBuffer;
			loadSpec.size = (UINT32)size;
			loadSpec.gentime = gen.QuadPart;
			loadSpec.pakEnum = pRequest->pakType;
			if (pRequest->bCompress == FALSE) {
				loadSpec.flags |= PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS;
			}
			loadSpec.flags |= PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE;
			loadSpec.flags |= PAKFILE_LOAD_FORCE_ALLOW_PAK_ADDING;
			loadSpec.flags |= PAKFILE_LOAD_FILE_ALREADY_HASHED;
			MemoryCopy(loadSpec.hash.m_Hash, SHA1HashSize, sha_hash, SHA1HashSize);
			PakFileAddFile(loadSpec);

			trace("replaced %s\n", fname);
			FREE(gPatchClient.m_pPool, pRequest->fname);
			FREE(gPatchClient.m_pPool, pRequest->fpath);
			pRequest->fname = NULL;
			pRequest->fpath = NULL;
		}

	}
	if (pBuffer != NULL) {
		FREE(pPool, pBuffer);
	}
	if (count > 0) {
		markup.Save(tmp_fname);
	}
	return TRUE;

_err:
	if (pBuffer != NULL) {
		FREE(pPool, pBuffer);
	}
	if (count > 0) {
		markup.Save(tmp_fname);
	}
	return FALSE;
}

BOOL PatchClientRequestFile(
	LPCTSTR fpath,
	LPCTSTR fname,
	PATCH_CLIENT_EXE_REQUEST exeRequest);

static BOOL sPatchClientInitExeRequestNext(BOOL bFirst)
{
	PATCH_CLIENT_EXE_REQUEST next;
	CHANNEL* pChannel = AppGetPatchChannel();
	ASSERT_RETFALSE(pChannel != NULL);

	if (gPatchClient.m_exeRequests.PopHead(next)) {
		if (next.fpath) {
			TraceDownload("[EXE PATCH] Downloading %s%s\r\n", next.fpath, next.fname);
		} else {
			TraceDownload("[EXE PATCH] Downloading %s\r\n", next.fname);
		}
		// Request the next file, if there is one
		return PatchClientRequestFile(next.fpath, next.fname, next);
	} else {
		// done with exe patching requests
		if (bFirst) {
			// if there was no previous requests, then request data file list
			NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, NULL);
			NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, NULL);
			NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_HEADER, NULL);
			NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND, NULL);

			PATCH_SERVER_GET_DATA_LIST_MSG msgRequest;
			ASSERT_RETFALSE(ConnectionManagerSend(pChannel, &msgRequest, PATCH_SERVER_GET_DATA_LIST));
			gPatchClient.m_listBufferInfo.sizeCompressed = 0;
			gPatchClient.m_listBufferInfo.sizeOriginal = 0;
			gPatchClient.m_iState = PATCH_CLIENT_RECEIVING_DATA_LIST;
			return TRUE;
		} else {
			// If there were previous requests, then replace files and restart
			if (gApp.m_fnameDelete[0] == '\0') {
				if (CreateDirectory(_T(".\\tmp\\"), NULL) == FALSE) {
					DWORD dwError = GetLastError();
					ASSERT_RETFALSE(dwError == ERROR_ALREADY_EXISTS);
				}

//				TCHAR tmp_fpath[DEFAULT_FILE_WITH_PATH_SIZE+1];
//				ASSERT_RETFALSE(GetTempPath(DEFAULT_FILE_WITH_PATH_SIZE, tmp_fpath));
				ASSERT_RETFALSE(GetTempFileName(_T(".\\tmp\\"), _T("ping0"), 0, gApp.m_fnameDelete));
			}

			ASSERT_RETFALSE(sPatchClientInitExeListReplaceFiles(gPatchClient.m_exeFinishedRequests, gApp.m_fnameDelete, gPatchClient.m_pPool));
			gPatchClient.m_iState = PATCH_CLIENT_RESTART;
			return TRUE;
		}
	}
}

static BOOL sPatchClientInitExeList(
	CHANNEL* pChannel,
	BOOL bSkipRecvList = FALSE)
{
	ASSERT_RETFALSE(pChannel != NULL);
	ASSERT_RETFALSE(AppIsPatching());

	if (gPatchClient.m_iState == PATCH_CLIENT_DISCONNECTED) {

		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, NULL);
		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, NULL);
		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_HEADER, NULL);
		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND, NULL);

		// always handled by a callback
		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_PAKFILE_INFO, sPatchClientPakFileInfoHandler);
		NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_QUEUE_POSITION, sPatchClientQueuePositionHandler);

		PATCH_SERVER_SEND_CLIENT_VERSION_MSG msgVersion;
		msgVersion.version = PATCH_CLIENT_CURRENT_VERSION;
#ifdef _WIN64
		msgVersion.buildType = BUILD_64;
#else
		msgVersion.buildType = BUILD_32;
#endif
		ASSERT_RETFALSE(ConnectionManagerSend(pChannel, &msgVersion, PATCH_SERVER_SEND_CLIENT_VERSION));
		gPatchClient.m_iState = PATCH_CLIENT_WAITING_FOR_ACK;
		return TRUE;

	} else if (gPatchClient.m_iState == PATCH_CLIENT_WAITING_FOR_ACK) {

		PATCH_SERVER_RESPONSE_INIT_DONE_MSG msg;
		if (PatchClientWaitForMessage(PATCH_SERVER_RESPONSE_INIT_DONE, &msg)) {
			if (bSkipRecvList) {
				// If we're running fillpak client, just jump straight to finished state
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, sPatchClientDataSmallHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, sPatchClientDataLargeHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_HEADER, sPatchClientHeaderHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND, sPatchClientErrorHandler);
				gPatchClient.m_iState = PATCH_CLIENT_CONNECTED;
			} else {
				PATCH_SERVER_GET_EXE_LIST_MSG msgRequest;
				ASSERT_RETFALSE(ConnectionManagerSend(pChannel, &msgRequest, PATCH_SERVER_GET_EXE_LIST));
				gPatchClient.m_listBufferInfo.sizeCompressed = 0;
				gPatchClient.m_listBufferInfo.sizeOriginal = 0;
				gPatchClient.m_iState = PATCH_CLIENT_RECEIVING_EXE_LIST;
			}
		}
		return TRUE;

	} else if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_EXE_LIST || 
		gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_DATA_LIST) {

		if (gPatchClient.m_listBufferInfo.sizeCompressed == 0 ||
			gPatchClient.m_listBufferInfo.sizeOriginal == 0) {
			PATCH_SERVER_RESPONSE_COMPRESSION_SIZE_MSG msgCompressionSize;
			if (PatchClientWaitForMessage(PATCH_SERVER_RESPONSE_COMPRESSION_SIZE, &msgCompressionSize)) {
				gPatchClient.m_LastAckedBytesArray[HASHLIST_INDEX] = 0;
				gPatchClient.m_listBufferInfo.sizeCompressed = msgCompressionSize.sizeCompressed;
				gPatchClient.m_listBufferInfo.sizeOriginal = msgCompressionSize.sizeOriginal;
				gPatchClient.m_listBufferInfo.sizeCurrent = 0;

				if (gPatchClient.m_listBufferInfo.sizeCompressed > 0) {
					gPatchClient.m_listBufferInfo.pBufferCompressed = MALLOC(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.sizeCompressed);
					ASSERT_GOTO(gPatchClient.m_listBufferInfo.pBufferCompressed != NULL, _err2);
				} else {
					ASSERT_GOTO(gPatchClient.m_listBufferInfo.sizeOriginal > 0, _err2);
				}

				if (gPatchClient.m_listBufferInfo.sizeOriginal > 0) {
					gPatchClient.m_listBufferInfo.pBufferOriginal = MALLOC(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.sizeOriginal);
					ASSERT_GOTO(gPatchClient.m_listBufferInfo.pBufferOriginal != NULL, _err2);
				}

				return TRUE;
			} else {
				return TRUE;
			}
		} else {
			UINT32 total;
			void* pDest;

			if (gPatchClient.m_listBufferInfo.sizeCompressed > 0) {
				total = gPatchClient.m_listBufferInfo.sizeCompressed;
				pDest = gPatchClient.m_listBufferInfo.pBufferCompressed;
			} else {
				total = gPatchClient.m_listBufferInfo.sizeOriginal;
				pDest = gPatchClient.m_listBufferInfo.pBufferOriginal;
			}

			ASSERT_GOTO(PatchClientWaitForData((UINT8*)pDest, total, gPatchClient.m_listBufferInfo.sizeCurrent), _err2);
			ASSERT_GOTO(gPatchClient.m_listBufferInfo.sizeCurrent <= total, _err2);

#if 1
			char sProgress[32];
			UINT32 iProgress;

			iProgress = (50 * gPatchClient.m_listBufferInfo.sizeCurrent) / total;
			if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_DATA_LIST) {
				iProgress += 50;
			}
			
			//PStrPrintf(sProgress, 32, "Progress %d%%", iProgress);  			// not localized
			PStrCopy(sProgress, "", 32);

			if (AppIsTugboat() || AppIsHellgate()) {
				CSAutoLock autoLock(&sLockProgress);
				PStrCvt(gPatchClient.m_strProgress, sProgress, 2*DEFAULT_FILE_WITH_PATH_SIZE);
				gPatchClient.m_iProgress = iProgress;
			}
#endif

			sPatchClientUpdateSendStatusUpdateForFile(
				HASHLIST_INDEX, 
				gPatchClient.m_listBufferInfo.sizeCurrent,
				gPatchClient.m_listBufferInfo.sizeCurrent >= total,
				true);

			if (gPatchClient.m_listBufferInfo.sizeCurrent < total) {
				return TRUE;
			}

			if (gPatchClient.m_listBufferInfo.sizeCompressed > 0) {
				UINT32 sizeOriginal = gPatchClient.m_listBufferInfo.sizeOriginal;
				ASSERT_GOTO(ZLIB_INFLATE((UINT8*)gPatchClient.m_listBufferInfo.pBufferCompressed,
					gPatchClient.m_listBufferInfo.sizeCompressed,
					(UINT8*)gPatchClient.m_listBufferInfo.pBufferOriginal,
					&sizeOriginal), _err2);
				ASSERT_GOTO(sizeOriginal == gPatchClient.m_listBufferInfo.sizeOriginal, _err2);
				gPatchClient.m_listBufferInfo.sizeCompressed = 0;
				FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferCompressed);
				gPatchClient.m_listBufferInfo.pBufferCompressed = NULL;
			}
			
			if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_EXE_LIST) {
				ASSERT_GOTO(sPatchClientInitExeParseBuffer(
					(UINT8*)gPatchClient.m_listBufferInfo.pBufferOriginal,
					gPatchClient.m_listBufferInfo.sizeOriginal,
					gPatchClient.m_exeRequests,
					gPatchClient.m_pPool), _err2);

				FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferOriginal);
				gPatchClient.m_listBufferInfo.pBufferOriginal = NULL;
				gPatchClient.m_listBufferInfo.sizeOriginal = 0;
				gPatchClient.m_iFilesNeeded = gPatchClient.m_exeRequests.Count();

				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, sPatchClientDataSmallHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, sPatchClientDataLargeHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_HEADER, sPatchClientHeaderHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND, sPatchClientErrorHandler);

				gPatchClient.m_iState = PATCH_CLIENT_RECEIVING_EXE_LIST_FILE;
				if (sPatchClientInitExeRequestNext(TRUE) == FALSE) {
					gPatchClient.m_iState = PATCH_CLIENT_ERROR;
				}
			} else {
				ASSERT_GOTO(sPatchClientInitDataParseBuffer(
					(UINT8*)gPatchClient.m_listBufferInfo.pBufferOriginal,
					gPatchClient.m_listBufferInfo.sizeOriginal), _err2);

				FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferOriginal);
				gPatchClient.m_listBufferInfo.pBufferOriginal = NULL;
				gPatchClient.m_listBufferInfo.sizeOriginal = 0;

				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL, sPatchClientDataSmallHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE, sPatchClientDataLargeHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_HEADER, sPatchClientHeaderHandler);
				NetCommandTableRegisterImmediateMessageCallback(gApp.m_PatchServerCommandTable, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND, sPatchClientErrorHandler);

				gPatchClient.m_iState = PATCH_CLIENT_CONNECTED;
			}
			gPatchClient.m_listBufferInfo.sizeOriginal = 0;
			FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferOriginal);
			gPatchClient.m_listBufferInfo.pBufferOriginal = NULL;
			return TRUE;
		}

	} else if (gPatchClient.m_iState == PATCH_CLIENT_RECEIVING_EXE_LIST_FILE) {
		// this state is handled by immediate callback functions
		return TRUE;
	} else {
		return TRUE;
	}

_err2:
	if (gPatchClient.m_listBufferInfo.pBufferOriginal != NULL) {
		FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferOriginal);
		gPatchClient.m_listBufferInfo.pBufferOriginal = NULL;
	}
	if (gPatchClient.m_listBufferInfo.pBufferCompressed != NULL) {
		FREE(gPatchClient.m_pPool, gPatchClient.m_listBufferInfo.pBufferCompressed);
		gPatchClient.m_listBufferInfo.pBufferCompressed = NULL;
	}
	return FALSE;
}

void PatchClientDisconnect()
{
	gPatchClient.Free();
	gPatchClient.Init();
}

BOOL PatchClientConnect(BOOL& bDone, BOOL& bRestart, BOOL bSkipRecvList)
{
	if (AppIsPatching() == FALSE) {
		bDone = TRUE;
		bRestart = FALSE;
		return TRUE;
	}

	CHANNEL* channel = AppGetPatchChannel();
	ASSERT_RETFALSE(channel != NULL);
	ASSERT_RETFALSE(gApp.m_PatchServerCommandTable != NULL);

	ASSERT_RETFALSE(sPatchClientInitExeList(channel, bSkipRecvList));

	bDone = FALSE;
	bRestart = FALSE;
	switch (gPatchClient.m_iState) {
		case PATCH_CLIENT_ERROR:
			return FALSE;
		case PATCH_CLIENT_RESTART:
			bRestart = TRUE;
			break;
		case PATCH_CLIENT_CONNECTED:
			bDone = TRUE;
			break;
		default:
			break;
	}

	return TRUE;
}


void PatchClientSendKeepAliveMsg(void)
{
	// only send keep alive after initial handshake and EXE patch - this should let us downgrade if needed
	if (gPatchClient.m_iState != PATCH_CLIENT_CONNECTED)
		return;
	PATCH_SERVER_KEEP_ALIVE_MSG msgKeepAlive;
	ASSERT(ConnectionManagerSend(AppGetPatchChannel(), &msgKeepAlive, PATCH_SERVER_KEEP_ALIVE));
}


static PRESULT sPatchClientFileLoadingThreadCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * pSpec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(pSpec);

	PRESULT pr = S_OK;

	if (pSpec->buffer == NULL) {
		if (data.m_IOSize == data.m_Bytes && data.m_Bytes > 0) {
			pSpec->bytesread = data.m_IOSize;
			pSpec->buffer = data.m_Buffer;
			pSpec->size = data.m_Bytes;
			pSpec->flags |= PAKFILE_LOAD_FORCE_ALLOW_PAK_ADDING;
			pSpec->flags |= PAKFILE_LOAD_FILE_ALREADY_COMPRESSED;
			ASSERT(PakFileAddFile(*pSpec, FALSE))
		}
		if (pSpec->buffer_compressed != NULL) {
			FREE(pSpec->pool, pSpec->buffer_compressed);
			pSpec->buffer_compressed = NULL;
		}

		if (!TEST_MASK(data.m_flags, ADF_CANCEL) && pSpec->fpLoadingThreadCallback)
		{
			V_SAVE( pr, pSpec->fpLoadingThreadCallback(data) );
		}
	} else {
		void* pTmp;
		DWORD iTmp;

		iTmp = pSpec->size;
		pSpec->size = data.m_IOSize;
		pTmp = pSpec->buffer;
		pSpec->buffer = data.m_Buffer;
		pSpec->flags |= PAKFILE_LOAD_FORCE_ALLOW_PAK_ADDING;
		pSpec->flags |= PAKFILE_LOAD_FILE_ALREADY_COMPRESSED;
		if (data.m_IOSize == data.m_Bytes && data.m_Bytes > 0) {
			ASSERT(PakFileAddFile(*pSpec, FALSE));
		}
		pSpec->buffer = pTmp;
		pSpec->size = iTmp;

		{
			UINT8* pSource = (UINT8*)data.m_Buffer + pSpec->offset;
			UINT8* pDest = (UINT8*)pSpec->buffer;
			DWORD sizeSource = pSpec->bytestoread == 0 ? (data.m_IOSize > pSpec->size ? pSpec->size : data.m_IOSize) : pSpec->bytestoread;
			DWORD sizeDest = pSpec->size;
			for (DWORD i = 0; i < sizeDest && i < sizeSource; i++) {
				pDest[i] = pSource[i];
			}
		}

/*		MemoryCopy(
			pSpec->buffer,
			pSpec->size,
			(UINT8*)data.m_Buffer + pSpec->offset,
			(pSpec->bytestoread == 0 ? 
				(data.m_IOSize > pSpec->size ?
				pSpec->size : data.m_IOSize) :
				pSpec->bytestoread));*/
		if (pSpec->buffer_compressed != NULL) {
			FREE(pSpec->pool, pSpec->buffer_compressed);
			pSpec->buffer_compressed = NULL;
		}

		if (!TEST_MASK(data.m_flags, ADF_CANCEL) && pSpec->fpLoadingThreadCallback)
		{
			V_SAVE( pr, pSpec->fpLoadingThreadCallback(data) );
		}
		if (pSpec->buffer != data.m_Buffer) {
			FREE(data.m_Pool, data.m_Buffer);
			data.m_Buffer = NULL;
		}
	}
	if (TEST_MASK(pSpec->flags, PAKFILE_LOAD_FREE_BUFFER) && pSpec->buffer)
	{
		FREE(pSpec->pool, pSpec->buffer);
		pSpec->buffer = NULL;
	}
	if (TEST_MASK(pSpec->flags, PAKFILE_LOAD_FREE_CALLBACKDATA) && pSpec->callbackData)
	{
		FREE(pSpec->pool, pSpec->callbackData);
		pSpec->callbackData = NULL;
	}

	return pr;
}

void* PatchClientRequestFileNow(
	LPCTSTR fpath,
	LPCTSTR fname,
	MEMORYPOOL* pPool,
	UINT32& filesize,
	UINT32& gentime_high,
	UINT32& gentime_low)
{
	PATCH_CLIENT_REQUEST request(FALSE);
	ASYNC_EVENT* pAsyncEvent = NULL;
	ASYNC_DATA asyncData;
	DWORD result;
	UINT32 i;

	UNREFERENCED_PARAMETER(fname);
	UNREFERENCED_PARAMETER(fpath);
	UNREFERENCED_PARAMETER(i);

	pAsyncEvent = AsyncFileGetEvent();
	ASSERT_RETNULL(pAsyncEvent != NULL);

	request.m_pFINode = NULL;
	request.m_hEvent = AsyncEventGetHandle(pAsyncEvent);
	asyncData = ASYNC_DATA(NULL, NULL, 0, 0, 0, NULL, NULL, NULL, pPool);
	request.m_asyncData = &asyncData;
	request.m_strFileName = fname;
	request.m_strFilePath = fpath;

	gPatchClient.m_csRequests.Enter();
	gPatchClient.m_PendingRequests[NUM_ASYNC_PRIORITIES].PListPushTailPool(gPatchClient.m_pPool, request);
/*	for (i = 0; i < MAX_DOWNLOADS; i++) {
		if (gPatchClient.m_CurrentRequests[i].m_asyncData == NULL) {
			gPatchClient.m_CurrentRequests[i] = request;
			gPatchClient.m_csRequests.Leave();
			ASSERT_GOTO(sPatchClientRequestFile(fpath, fname, i), _err);
			goto _next2;
		}
	}
	gPatchClient.m_PendingRequests.PListPushTail(gPatchClient.m_pPool, request);*/
	gPatchClient.m_csRequests.Leave();
	sPatchClientUpdateRequest();

//_next2:
	result = WaitForSingleObject(AsyncEventGetHandle(pAsyncEvent), INFINITE);
	AsyncFileRecycleEvent(pAsyncEvent);

	ASSERT_GOTO(result == WAIT_OBJECT_0, _err);
	ASSERT_GOTO(asyncData.m_IOSize == asyncData.m_Bytes, _err);
	ASSERT_GOTO(asyncData.m_Buffer != NULL, _err);

	filesize = asyncData.m_Bytes;
	gentime_high = asyncData.m_GentimeHigh;
	gentime_low = asyncData.m_GentimeLow;
	return asyncData.m_Buffer;

_err:
	if (asyncData.m_Buffer != NULL) {
		FREE(pPool, asyncData.m_Buffer);
	}
	return NULL;
}

BOOL PatchClientRequestFile(
	LPCTSTR fpath,
	LPCTSTR fname,
	PATCH_CLIENT_EXE_REQUEST exeRequest)
{
	PATCH_CLIENT_REQUEST request(FALSE);
	ASYNC_DATA* pAsyncData = NULL;
	UINT32 i;
	UNREFERENCED_PARAMETER(fpath);
	UNREFERENCED_PARAMETER(fname);
	UNREFERENCED_PARAMETER(i);

	pAsyncData = (ASYNC_DATA*)MALLOC(gPatchClient.m_pPool, sizeof(ASYNC_DATA));
	ASSERT_GOTO(pAsyncData != NULL, _err);

	new (pAsyncData) ASYNC_DATA(NULL, NULL, NULL, 0, NULL, NULL,
		sPatchClientFileLoadingThreadCallback, NULL, gPatchClient.m_pPool);

	request.m_pFINode = NULL;
	request.m_hEvent = NULL;
	request.m_exeRequestInfo = exeRequest;
	request.m_asyncData = pAsyncData;

	gPatchClient.m_csRequests.Enter();
	gPatchClient.m_PendingRequests[NUM_ASYNC_PRIORITIES-1].PListPushTailPool(gPatchClient.m_pPool, request);
/*	for (i = 0; i < MAX_DOWNLOADS; i++) {
		if (gPatchClient.m_CurrentRequests[i].m_asyncData == NULL) {
			gPatchClient.m_CurrentRequests[i] = request;
			gPatchClient.m_csRequests.Leave();
			ASSERT_GOTO(sPatchClientRequestFile(fpath, fname, i), _err);
			return TRUE;
		}
	}
	gPatchClient.m_PendingRequests.PListPushTail(gPatchClient.m_pPool, request);*/
	gPatchClient.m_csRequests.Leave();
	sPatchClientUpdateRequest();
	return TRUE;

_err:
	if (pAsyncData != NULL) {
		FREE(gPatchClient.m_pPool, pAsyncData);
	}
	return FALSE;
}

BOOL PatchClientRequestFile(
	PAKFILE_LOAD_SPEC& loadSpec,
	FILE_INDEX_NODE* pFINode)
{
	ASYNC_EVENT* async_event = NULL;
	ASYNC_DATA* pAsyncData = NULL;
	PAKFILE_LOAD_SPEC* pSpec = NULL;
	ASYNC_DATA asyncData;
	UINT32 i;
	UNREFERENCED_PARAMETER(i);

	loadSpec.pakEnum = pFINode->m_iSvrPakType;

	PATCH_CLIENT_REQUEST request(FALSE);
	request.m_pFINode = pFINode;

	if (AppGetState() == APP_STATE_INIT) {
		return FALSE;
	}
	ASSERT_RETFALSE(gPatchClient.m_iState == PATCH_CLIENT_CONNECTED);
	
	if (loadSpec.flags & PAKFILE_LOAD_IMMEDIATE) {
		new (&asyncData) ASYNC_DATA(NULL, NULL, NULL, 0, 0, loadSpec.fpFileIOThreadCallback,
			sPatchClientFileLoadingThreadCallback, &loadSpec, loadSpec.pool);
		async_event = AsyncFileGetEvent();
		ASSERT_RETFALSE(async_event != NULL);
		request.m_hEvent = AsyncEventGetHandle(async_event);
		request.m_asyncData = &asyncData;
		request.m_strFileName = loadSpec.bufFileName;
		request.m_strFilePath = loadSpec.bufFilePath;
	} else {
		pAsyncData = (ASYNC_DATA*)MALLOC(loadSpec.pool, sizeof(ASYNC_DATA));
		ASSERT_GOTO(pAsyncData != NULL, _err);

		pSpec = PakFileLoadCopySpec(loadSpec);
		ASSERT_GOTO(pSpec != NULL, _err);

		new (pAsyncData) ASYNC_DATA(NULL, NULL, NULL, 0, ADF_FREEDATA | ADF_LOADINGCALLBACK_ONCANCEL,
			loadSpec.fpFileIOThreadCallback, sPatchClientFileLoadingThreadCallback, pSpec, loadSpec.pool);
		request.m_hEvent = NULL;
		request.m_asyncData = pAsyncData;
	}

	// If there's space for another file load, request the file
	// otherwise add it to the pending list
	gPatchClient.m_csRequests.Enter();

	if (loadSpec.flags & PAKFILE_LOAD_IMMEDIATE) {
		gPatchClient.m_PendingRequests[NUM_ASYNC_PRIORITIES].PListPushTailPool(gPatchClient.m_pPool, request);
	} else {
		loadSpec.priority = PIN(loadSpec.priority, (UINT32)ASYNC_PRIORITY_0, (UINT32) ASYNC_PRIORITY_MAX);
		gPatchClient.m_PendingRequests[loadSpec.priority].PListPushTailPool(gPatchClient.m_pPool, request);
	}
	gPatchClient.m_csRequests.Leave();
	sPatchClientUpdateRequest();


//_next:
	// If the file is requested as LoadNow, then wait for file to finish
	if (loadSpec.flags & PAKFILE_LOAD_IMMEDIATE) {
		DWORD result;

		PRESULT pr = S_OK;

		ASSERT_RETFALSE(async_event != NULL);
		while (TRUE) {
			result = WaitForSingleObject(
				AsyncEventGetHandle(async_event),
				gPatchClient.m_waitInterval);
			if (result == WAIT_TIMEOUT) {
				if (gPatchClient.m_waitFn) {
					gPatchClient.m_waitFn();
				}
			} else {
				break;
			}
		}

		AsyncFileRecycleEvent(async_event);

		if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) && loadSpec.bytesread == 0) {

		} else {
			if (asyncData.m_fpLoadingThreadCallback != NULL) {
				V_SAVE( pr, asyncData.m_fpLoadingThreadCallback(asyncData) );
			}
		}

		if (asyncData.m_UserData && TEST_MASK(asyncData.m_flags, ADF_FREEDATA)) {
			FREE(asyncData.m_Pool, asyncData.m_UserData);
		}

		if (asyncData.m_Buffer && TEST_MASK(asyncData.m_flags, ADF_FREEBUFFER)) {
			FREE(asyncData.m_Pool, asyncData.m_Buffer);
		}

		ASSERT_RETFALSE(result == WAIT_OBJECT_0);
		ASSERT_RETFALSE(asyncData.m_IOSize == asyncData.m_Bytes);

		loadSpec.size = asyncData.m_IOSize;
	}

	return TRUE;

_err:
	if (pAsyncData != NULL) {
		FREE(loadSpec.pool, pAsyncData);
	}
	if (pSpec != NULL) {
		FREE(loadSpec.pool, pSpec);
	}
	return FALSE;
}

UINT32 PatchClientGetProgress()
{
	return gPatchClient.m_iProgress;
}

void PatchClientGetUIProgressMessage(
	WCHAR* buffer,
	UINT32 len)
{
	CSAutoLock autoLock(&sLockProgress);
	ASSERT_RETURN(buffer != NULL);
	PStrCopy(buffer, gPatchClient.m_strProgress, len);
}

void PatchClientClearUIProgressMessage()
{
	CSAutoLock autoLock(&sLockProgress);
	gPatchClient.m_strProgress[0] = L'\0';
	gPatchClient.m_iProgress = 0;
}

UINT32 PatchClientExePatchTotalFileCount()
{
	return gPatchClient.m_iFilesNeeded;
}

UINT32 PatchClientExePatchCurrentFileCount()
{
	return gPatchClient.m_iCurrentFilesCount;
}

// Not thread safe, should only be called in the beginning
void PatchClientSetWaitFunction(
	void (*waitFn)(void),
	DWORD waitInterval)
{
	gPatchClient.m_waitFn = waitFn;
	gPatchClient.m_waitInterval = waitInterval;
}

void* PatchClientDisconnectCallback(
	void* pArg)
{
	UNREFERENCED_PARAMETER(pArg);

	InterlockedExchange(&iErrorStatus, 1);
	PatchClientDisconnect();
	return NULL;
}

LONG PatchClientRetrieveResetErrorStatus()
{
	return InterlockedExchange(&iErrorStatus, 0);
}

#endif

