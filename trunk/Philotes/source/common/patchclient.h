//----------------------------------------------------------------------------
// patchclient.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
#ifndef _PATCH_CLIENT_H_
#define _PATCH_CLIENT_H_

#include "PatchServerMsg.h"
#include "asyncfile.h"

#define DEFAULT_WAIT_TIME 10

BOOL PatchClientWaitForMessage(
	PATCH_SERVER_RESPONSE_COMMANDS cmd,
	MSG_STRUCT* pMSG);

BOOL PatchClientWaitForData(
	UINT8* buffer,
	UINT32 total,
	UINT32& cur);

BOOL PatchClientConnect(
	BOOL& bDone,
	BOOL& bRestart,
	BOOL bSkipRecvList = FALSE);

void PatchClientDisconnect();

void PatchClientSendKeepAliveMsg(void);

BOOL PatchClientRequestFile(
	PAKFILE_LOAD_SPEC& loadSpec,
	FILE_INDEX_NODE* pFINode);

void PatchClientGetUIProgressMessage(
	WCHAR* buffer,
	UINT32 len);

void PatchClientClearUIProgressMessage();

void PatchClientSetWaitFunction(
	void (*waitFn)(void),
	DWORD waitInterval);

void* PatchClientDisconnectCallback(
	void* pArg);

UINT32 PatchClientExePatchTotalFileCount();

UINT32 PatchClientExePatchCurrentFileCount();

UINT32 PatchClientGetProgress();

LONG PatchClientRetrieveResetErrorStatus();

#endif // _PATCH_CLIENT_H_
#endif // !SERVER_VERSION