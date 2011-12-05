#pragma once
#include "autoref.h"
#include "patchstd.h"
#pragma warning(disable:6011)
#include "stlallocator.h"
#pragma warning(default:6011)
#include "list.h"
#include "tokenbucket.h"

#define PATCH_SERVER_NAME		"Patch Server"
#define ROOT_DIR				L"c:\\prime\\"
#define FILE_IDX_FILE			L"files.idx"
#define DATA_DIR				L"data\\"
#define EXE_LIST_FILE			"ntlfl.cfg"
#define MAX_SYNC_DOWNLOADS		16

void* PatchServerInit(MEMORYPOOL* pMemPool, SVRID SvrId, const class SVRCONFIG* SvrConfig);
void PatchServerEntryPoint(void* pContext);
void PatchServerCommandCallback(void* pContext, SR_COMMAND Command);
void PatchServerXmlMessageHandler(LPVOID pContext, 
									struct XMLMESSAGE *	senderSpecification,
									struct XMLMESSAGE *	msgSpecification,
									LPVOID userData );
BOOL PatchServerInitConnectionTable(CONNECTION_TABLE* tbl);


struct PATCH_CLIENT_CTX;
struct PATCH_REQUEST_CTX;


struct PATCH_SERVER_CTX
{
	MEMORYPOOL*													m_pMemPool;
	SVRID														m_SvrId;

	HANDLE														m_hEventDone;
	HANDLE														m_hEventWork;
	HANDLE														m_hOneSecondTimer;

	CSimpleHash<LPWSTR, PFILEINFO>								m_fileTable;
	UINT8														m_shaPatcher[SHA1HashSize];
	UINT8*														m_bufPakIdx;
	UINT32														m_sizePakIdxOriginal;
	UINT32														m_sizePakIdxCompressed;
	UINT8*														m_bufExeIdx;
	UINT32														m_sizeExeIdxOriginal;
	UINT32														m_sizeExeIdxCompressed;
	UINT8*														m_bufExeIdx64;
	UINT32														m_sizeExeIdx64Original;
	UINT32														m_sizeExeIdx64Compressed;
	UINT32														m_uTotalBandwidthLimit;
	UINT32														m_uTotalUnackedLimit;
	UINT32														m_uClientUnackedSoftLimit;
	UINT32														m_uClientUnackedHardLimit;
	UINT32														m_uUnackedBytes;
	TokenBucketEx												m_TokenBucket;
	BOOL														m_bUpdateLauncherFiles;
//	DWORD														m_dwDataListCRC;

	PList<FILEINFO*>											m_listImportantFile;
	PList<FILEINFO*>											m_listImportantFile64;
	PList<FILEINFO*>											m_listTrivialFile;
	CCriticalSection											m_CriticalSection;
	PERF_INSTANCE(PatchServer)*									m_pPerfInstance;

	typedef set_mp<PATCH_CLIENT_CTX*>::type TClientSet;
	typedef deque_mp<PATCH_CLIENT_CTX*>::type TClientQueue;

	// a set of clients connected to the server, used to avoid handling messages after they are deleted
	TClientSet m_ClientSet;
	// a set of clients that are disconnected and waiting for deletion 
	// we avoid deleting accounts in the detach callback, because it could be during a send request (see bug 38229)
	TClientSet m_DisconnectedClientSet;
	// a queue of clients waiting for available server bandwidth to enter patching queue
	TClientQueue m_WaitingClientQueue;
	// a queue of clients waiting for available server bandwidth to send next data packet
	TClientQueue m_ReadyClientQueue;
	// a set of clients that are stalled at their unacked limit, used to trickle data to them once a second
	TClientSet m_StalledClientSet;

	PATCH_SERVER_CTX(MEMORYPOOL* pPool, SVRID SvrId, const class PatchServerConfig& Config); /*throw(std::exception)*/
	~PATCH_SERVER_CTX(void);

	void ThreadEntryPoint(void);
	void Shutdown(void);

	void AddClient(PATCH_CLIENT_CTX* pClientCtx);
	void DisconnectClient(PATCH_CLIENT_CTX* pClientCtx);
	void ActivateClient(PATCH_CLIENT_CTX* pClientCtx);

protected:
	static void CALLBACK OneSecondTimerCallback(void* pThis, BOOLEAN bTimerOrWaitFired);
	void TrickleDataToStalledClients(void);
	void SendQueuePositionMsgsToWaitingClients(void);
	void ServeRequestsToClient(PATCH_CLIENT_CTX* pClientCtx, bool bTrickle);
	bool PromoteWaitingClient(void);
	static void DeleteAllClientsInSet(MEMORYPOOL* pMemPool, TClientSet* pClientSet);

private:
	PATCH_SERVER_CTX(const PATCH_SERVER_CTX&); // non-copyable
	PATCH_SERVER_CTX& operator =(const PATCH_SERVER_CTX&); // non-copyable
};


struct PATCH_CLIENT_CTX
{
	PATCH_SERVER_CTX& ServerCtx;

	SERVICEUSERID ClientId;
	UINT32 uClientVersion;
	UINT8 uClientBuildType;

	enum EState { DISCONNECTED = 0, IDLE = 1, WAITING = 2, READY = 3, STALLED = 4 };
	EState eState;

	UINT32 uUnsentBytes;
	UINT32 uUnackedBytes;
	
	typedef vector_mp<UINT8>::type TEncryptionKey;
	TEncryptionKey EncryptionKey;
	
	typedef deque_mp<PATCH_REQUEST_CTX*>::type TRequestQueue;
	TRequestQueue RequestQueue;
	
	PATCH_CLIENT_CTX(PATCH_SERVER_CTX& ServerCtx, SERVICEUSERID ClientId, const UINT8* pKey, UINT32 uKeyLen);
	~PATCH_CLIENT_CTX(void);

	bool IsAllowedToSend(void) const;
	bool IsStalledByUnackedData(void) const;
	
	bool AddRequest(PATCH_REQUEST_CTX* pRequestCtx);
	bool AckRequest(UINT8 uFileIdx, UINT32 uAckedBytes, bool bCompleted);

	bool ServeRequests(bool bTrickle);
	bool SendNextMsgForRequest(PATCH_REQUEST_CTX& RequestCtx, bool bTrickle);
	bool SendBufferImmediately(const UINT8* pBuffer, UINT32 uOriginalSize, UINT32 uCompressedSize);
	UINT32 SendMsg(const UINT8* pSourceBuffer, UINT32 uSentBytes, UINT32 uTotalBytes, UINT8 uFileIdx, 
		bool bTrickle, bool bEncrypt);
	// this version of SvrNetSendResponseToClient() tracks our bandwidth 
	SRNET_RETURN_CODE SendResponse(MSG_STRUCT& MsgStruct, NET_CMD NetCmd);

	TRequestQueue::iterator FindRequest(UINT8 uFileIdx);

	static bool RequestPriorityCmp(const PATCH_REQUEST_CTX* r1, const PATCH_REQUEST_CTX* r2);

private:
	PATCH_CLIENT_CTX(const PATCH_CLIENT_CTX&); // non-copyable
	PATCH_CLIENT_CTX& operator =(const PATCH_CLIENT_CTX&); // non-copyable
};


struct PATCH_REQUEST_CTX
{
	PATCH_CLIENT_CTX& ClientCtx;
	const FILEINFO& FileInfo;
	UINT8 uFileIdx;
	UINT32 uSentBytes;
	UINT32 uAckedBytes;

	PATCH_REQUEST_CTX(PATCH_CLIENT_CTX& ClientCtx, const FILEINFO& FileInfo, UINT8 uFileIdx);

	UINT32 GetTotalBytes(void) const;
	UINT32 GetUnsentBytes(void) const;
	UINT32 GetUnackedBytes(void) const;

private:
	PATCH_REQUEST_CTX(const PATCH_REQUEST_CTX&); // non-copyable
	PATCH_REQUEST_CTX& operator =(const PATCH_REQUEST_CTX&); // non-copyable
};
