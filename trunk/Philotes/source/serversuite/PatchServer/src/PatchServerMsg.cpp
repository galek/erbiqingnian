#include "patchstd.h"
#include "PatchServerMsg.h"
#include "log.h"
#include "PatchServerMsg.cpp.tmh"
#include <pakfile.h>
#include "compression.h"

using namespace std;

// Generate handler array
#undef _PATCH_CMD_HDR_
#undef _PATCH_REQUEST_TABLE_
//#undef _PATCH_RESPONSE_TABLE_
#define PATCH_MSG_REQUEST_HANDLERS
#include "PatchServerMsg.h"

#pragma warning(disable : 4127 )
NET_COMMAND_TABLE* PatchServerGetRequestTbl(void)
{
	//	validate the handler table
	for(INT32 i = 0; i < PATCH_REQUEST_COUNT; i++) {
		ASSERT_RETNULL( PATCH_MSG_HDLR_ARRAY( PATCH_SERVER )[i] );
	}

	//	get a command table
	NET_COMMAND_TABLE* tbl = NetCommandTableInit( PATCH_REQUEST_COUNT );
	ASSERT_RETNULL(tbl != NULL);

	//	fill the command table
#undef _PATCH_CMD_HDR_
#undef _PATCH_REQUEST_TABLE_
#define  PATCH_MSG_REQUEST_REGISTER
#include "PatchServerMsg.h"

	//	validate the table
	if( !NetCommandTableValidate( tbl ) )
	{
		NetCommandTableFree( tbl );
		ASSERT_RETNULL(FALSE);
	}
	return tbl;
}

NET_COMMAND_TABLE* PatchServerGetResponseTbl(void)
{
	//	get a command table
	NET_COMMAND_TABLE* tbl = NetCommandTableInit( PATCH_RESPONSE_COUNT );
	ASSERT_RETNULL(tbl != NULL);

	//	fill the command table
#undef _PATCH_CMD_HDR_
#undef _PATCH_RESPONSE_TABLE_
#define PATCH_MSG_RESPONSE_REGISTER
#include "PatchServerMsg.h"

	//	validate the table
	if(!NetCommandTableValidate(tbl))
	{
		NetCommandTableFree( tbl );
		ASSERT_RETNULL(FALSE);
	}
	return tbl;
}


LPVOID PatchServerCltAttachedHandler(
	LPVOID				svrContext,
	SERVICEUSERID		attachedUser,
	BYTE                *pAcceptData,
	DWORD                dwAcceptDataLen)
{
	TraceVerbose(TRACE_FLAG_PATCH_SERVER, "TraceClientPath: Patch Server - Client 0x%llx attached\n", attachedUser);
	ASSERT_RETNULL(svrContext && attachedUser != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);

	PATCH_CLIENT_CTX* pClientCtx = MEMORYPOOL_NEW(ServerCtx.m_pMemPool) 
		PATCH_CLIENT_CTX(ServerCtx, attachedUser, pAcceptData, dwAcceptDataLen);
	if (pClientCtx)
		ServerCtx.AddClient(pClientCtx);
	else 
		SvrNetDisconnectClient(attachedUser);		
	return pClientCtx;
}


void PatchServerCltDetachedHandler(
	LPVOID				svrContext,
	SERVICEUSERID		detachedUser,
	LPVOID				cltContext )
{
	TraceVerbose(TRACE_FLAG_PATCH_SERVER, "TraceClientPath: Patch Server - Client 0x%llx detached\n", detachedUser);
	ASSERT_RETURN(svrContext && cltContext && detachedUser != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(detachedUser == pClientCtx->ClientId);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	ServerCtx.DisconnectClient(pClientCtx);
}						  


BOOL PatchServerInitConnectionTable(
	CONNECTION_TABLE *tbl)
{
	SetServerType(tbl, (SVRTYPE)PATCH_SERVER);
	OFFER_SERVICE(USER_SERVER,
		PATCH_MSG_HDLR_ARRAY(PATCH_SERVER),
		PatchServerGetRequestTbl,
		PatchServerGetResponseTbl,
		PatchServerCltAttachedHandler,
		PatchServerCltDetachedHandler);
    return TRUE;
}


void PatchServerGetDataList(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	/*msg*/,
	LPVOID			cltContext)
{
	TraceExtraVerbose(TRACE_FLAG_PATCH_SERVER, "Client 0x%llx: Sending Data List\n", sender);
	ASSERT_RETURN(svrContext && cltContext && sender != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(sender == pClientCtx->ClientId);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	if (!pClientCtx->SendBufferImmediately
			(ServerCtx.m_bufPakIdx, ServerCtx.m_sizePakIdxOriginal, ServerCtx.m_sizePakIdxCompressed))
		TraceError("Patch Server: fatal error serving data list to client 0x%llx\n", pClientCtx->ClientId);
}


void PatchServerGetExeList(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	/*msg*/,
	LPVOID			cltContext)
{
	TraceExtraVerbose(TRACE_FLAG_PATCH_SERVER, "Client 0x%llx: Sending Exe List\n", sender);
	ASSERT_RETURN(svrContext && cltContext && sender != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(sender == pClientCtx->ClientId);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	bool bIsBuild64 = pClientCtx->uClientBuildType == BUILD_64;
	const UINT8* pBuffer = bIsBuild64 ? ServerCtx.m_bufExeIdx64 : ServerCtx.m_bufExeIdx;
	UINT32 uOriginalSize = bIsBuild64 ? ServerCtx.m_sizeExeIdx64Original : ServerCtx.m_sizeExeIdxOriginal;
	UINT32 uCompressedSize = bIsBuild64 ? ServerCtx.m_sizeExeIdx64Compressed : ServerCtx.m_sizeExeIdxCompressed;
	if (!pClientCtx->SendBufferImmediately(pBuffer, uOriginalSize, uCompressedSize))
		TraceError("Patch Server: fatal error serving exe list to client 0x%llx\n", pClientCtx->ClientId);
}


// Not using this message right now, so cleared out the message handler for now
void PatchServerGetFileSHA_W(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext != NULL);
	ASSERT_RETURN(cltContext != NULL);
	ASSERT_RETURN(msg != NULL);
	ASSERT_RETURN(sender != INVALID_SERVICEUSERID);
}


// Not using this message right now, so cleared out the message handler for now
void PatchServerGetFileSHA_A(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext != NULL);
	ASSERT_RETURN(cltContext != NULL);
	ASSERT_RETURN(msg != NULL);
	ASSERT_RETURN(sender != INVALID_SERVICEUSERID);
}


static bool sPatchServerGetFile(
	PATCH_SERVER_CTX& ServerCtx,
	PATCH_CLIENT_CTX* pClientCtx,
	const FILEINFO* pFileInfo,
	UINT8 fileidx,
	LPCWSTR filename)
{
	ASSERT_RETFALSE(pClientCtx);
	
	if (pClientCtx->RequestQueue.size() >= MAX_SYNC_DOWNLOADS) {
		TraceError("Patch Server client 0x%llx: exceeded maximum file requests\n", pClientCtx->ClientId);
		// if we're on the message thread, we can be sure not to receive more messages
		ServerCtx.DisconnectClient(pClientCtx);
		return false;
	}
	
 	if (!pFileInfo) {
		TraceError("Patch Server client 0x%llx: requested missing file %ls\n", pClientCtx->ClientId, filename);
		PATCH_SERVER_RESPONSE_FILE_NOT_FOUND_MSG msgResponse;
		msgResponse.idx = fileidx;
		return pClientCtx->SendResponse(msgResponse, PATCH_SERVER_RESPONSE_FILE_NOT_FOUND) == SRNET_SUCCESS;
	} 

	TraceExtraVerbose(TRACE_FLAG_PATCH_SERVER, 
		"Client 0x%llx: Sending File %ls -- idx 0x%x\n", pClientCtx->ClientId, filename, fileidx);
	PATCH_SERVER_RESPONSE_FILE_HEADER_MSG msgHeader;
	msgHeader.filesize = pFileInfo->file_size_low;
	msgHeader.filesize_compressed = pFileInfo->file_size_compressed;
	msgHeader.bInPak = (BYTE)pFileInfo->bFileInPak;
	msgHeader.idx = fileidx;
	msgHeader.gentime_low = pFileInfo->gentime_low;
	msgHeader.gentime_high = pFileInfo->gentime_high;
	if (pClientCtx->SendResponse(msgHeader, PATCH_SERVER_RESPONSE_FILE_HEADER) != SRNET_SUCCESS)
		return false;

	PATCH_REQUEST_CTX* pRequestCtx 
		= MEMORYPOOL_NEW(ServerCtx.m_pMemPool) PATCH_REQUEST_CTX(*pClientCtx, *pFileInfo, fileidx);
	if (pRequestCtx && pClientCtx->AddRequest(pRequestCtx))
		return true;
	MEMORYPOOL_DELETE(ServerCtx.m_pMemPool, pRequestCtx);
	TraceError("Patch Server: fatal error handling request from client 0x%llx\n", pClientCtx->ClientId);
	ServerCtx.DisconnectClient(pClientCtx);
	return false;
}


void PatchServerGetFile_W(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext && cltContext && msg && sender != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(sender == pClientCtx->ClientId);
	PATCH_SERVER_GET_FILE_MSG_W& Msg = *static_cast<PATCH_SERVER_GET_FILE_MSG_W*>(msg);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	wchar_t szFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(szFilename, DEFAULT_FILE_WITH_PATH_SIZE, L"%s%s", Msg.fpath, Msg.fname);
	FILEINFO* pFileInfo = ServerCtx.m_fileTable.GetItem(szFilename);
	if (!pFileInfo) { // Search for it from the real pakfile hash
		TCHAR tfpath[DEFAULT_FILE_WITH_PATH_SIZE];
		TCHAR tfname[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrCvt(tfpath, Msg.fpath, DEFAULT_FILE_WITH_PATH_SIZE);
		PStrCvt(tfname, Msg.fname, DEFAULT_FILE_WITH_PATH_SIZE);
		PStrT strPath(tfpath, PStrLen(tfpath));
		PStrT strName(tfname, PStrLen(tfname));
		if (FILE_INDEX_NODE* pFINode = PakFileLoadGetIndex(strPath, strName))
			FillFileInfoFromPakIndex(ServerCtx.m_fileTable, pFINode, ServerCtx.m_pMemPool);
	}

	sPatchServerGetFile(ServerCtx, pClientCtx, pFileInfo, Msg.idx, szFilename);
}


void PatchServerGetFile_A(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext != NULL);
	ASSERT_RETURN(cltContext != NULL);
	ASSERT_RETURN(msg != NULL);
	ASSERT_RETURN(sender != INVALID_SERVICEUSERID);

	PATCH_SERVER_GET_FILE_MSG_A* pMSG = (PATCH_SERVER_GET_FILE_MSG_A*)msg;
	PATCH_SERVER_GET_FILE_MSG_W msgW;
	//UINT32 i;

	PStrCvt(msgW.fpath, pMSG->fpath, DEFAULT_FILE_WITH_PATH_SIZE);
	PStrCvt(msgW.fname, pMSG->fname, DEFAULT_FILE_WITH_PATH_SIZE);
	msgW.idx = pMSG->idx;

	PatchServerGetFile_W(svrContext, sender, &msgW, cltContext);
}

extern SIMPLE_DYNAMIC_ARRAY<PAKFILE_DESC> g_PakFileDesc;
void PatchServerCltVersionHandler(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext && cltContext && msg && sender != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(sender == pClientCtx->ClientId);
	PATCH_SERVER_SEND_CLIENT_VERSION_MSG& Msg = *static_cast<PATCH_SERVER_SEND_CLIENT_VERSION_MSG*>(msg);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	PATCH_SERVER_RESPONSE_INIT_DONE_MSG msgAck;
	pClientCtx->uClientVersion = Msg.version;
	pClientCtx->uClientBuildType = Msg.buildType;

	if (pClientCtx->uClientVersion >= 5) {
		PATCH_SERVER_RESPONSE_PAKFILE_INFO_MSG msgPakFileInfo;
		for (UINT32 i = 0; i < g_PakFileDesc.Count(); i++) {
			msgPakFileInfo.iIndex = g_PakFileDesc[i].m_Index;
			msgPakFileInfo.iCount = g_PakFileDesc[i].m_Count;
//			msgPakFileInfo.iMaxIndex = g_PakFileDesc[i].m_MaxIndex;
			msgPakFileInfo.bCreate = g_PakFileDesc[i].m_Create;
			msgPakFileInfo.iAlternativeIndex = g_PakFileDesc[i].m_AlternativeIndex;
			PStrCvt(msgPakFileInfo.strBaseName, g_PakFileDesc[i].m_BaseName, DEFAULT_FILE_WITH_PATH_SIZE+1);
			if (pClientCtx->SendResponse(msgPakFileInfo, PATCH_SERVER_RESPONSE_PAKFILE_INFO) != SRNET_SUCCESS)
				return;
		}
	}
	pClientCtx->SendResponse(msgAck, PATCH_SERVER_RESPONSE_INIT_DONE);
}

//----------------------------------------------------------------------------
// Use information as to how much the client has actually received to tune
// our throttling method.
//----------------------------------------------------------------------------
void PatchServerFileStatusHandler(
	LPVOID			svrContext,
	SERVICEUSERID	sender,
	MSG_STRUCT *	msg,
	LPVOID			cltContext)
{
	ASSERT_RETURN(svrContext && cltContext && msg && sender != INVALID_SERVICEUSERID);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(svrContext);
	PATCH_CLIENT_CTX* pClientCtx = static_cast<PATCH_CLIENT_CTX*>(cltContext);
	ASSERT_RETURN(sender == pClientCtx->ClientId);
	PATCH_SERVER_SEND_RECEIVED_FILE_STATUS_MSG& Msg = *static_cast<PATCH_SERVER_SEND_RECEIVED_FILE_STATUS_MSG*>(msg);
	CSAutoLock Lock(&ServerCtx.m_CriticalSection);
	if (ServerCtx.m_ClientSet.count(pClientCtx) == 0)
		return; // client is already disconnected!

	if (pClientCtx->AckRequest(Msg.idx, Msg.receivedBytes, Msg.bCompleted != 0))
		return;
	TraceError("Patch Server: fatal error handling ack from client 0x%llx\n", pClientCtx->ClientId);
	ServerCtx.DisconnectClient(pClientCtx);
}


