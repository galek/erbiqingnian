#include "patchstd.h"
#include "PatchServer.cpp.tmh"
#include <exception>
#include "compression.h"
#include "random.h"
#include "asyncfile.h"
#include "cryptoapi.h"
#include "eventlog.h"
#include "Ping0ServerMessages.h"

using namespace std;
using namespace COMMON_SHA1;

START_SVRCONFIG_DEF(PatchServerConfig)
	SVRCONFIG_STR_FIELD(0, GameType, true, "")
	SVRCONFIG_STR_FIELD(1, BuildType, true, "")
	SVRCONFIG_INT_FIELD(2, TotalBandwidthLimit, false, 40*1024*1024)
	// with a total unacked limit of 1GB and a client unacked limit of 512KB, we could support 2000 clients patching
	// at full speed (with full unacked windows).  But our bandwidth limit is only 30MB/s, which is an average of 
	// 15KB/s per client.  Conclusion:  we should be bandwidth limited, not memory limited.
	SVRCONFIG_INT_FIELD(3, TotalUnackedLimit, false, 1024*1024*1024)
	SVRCONFIG_INT_FIELD(4, ClientUnackedSoftLimit, false, 512*1024) // stop sending data, except trickle
	// cut off the client if their unacked data > ClientUnackedSoftLimit + ClientUnackedHardLimit
	SVRCONFIG_INT_FIELD(5, ClientUnackedHardLimit, false, 32*1024)
	SVRCONFIG_STR_FIELD(6, UpdateLauncherFiles, false, "false")
END_SVRCONFIG_DEF


SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME(PATCH_SERVER) = 
{
	PATCH_SERVER,
	PATCH_SERVER_NAME,
	PatchServerInit,
	SVRCONFIG_CREATOR_FUNC(PatchServerConfig),
	PatchServerEntryPoint,
	PatchServerCommandCallback,
	PatchServerXmlMessageHandler,
	PatchServerInitConnectionTable
};


UINT32 FileInfoStrHash(const LPWSTR& str)
{
	UINT32 k = 0, i;

	for (i = 0; str[i] != '\0'; i++) {
		k =	STR_HASH_ADDC(k, str[i]);
	}
	return k;
}

BOOL FileInfoEqual(const LPWSTR& str, const PFILEINFO &pFI)
{
	return (PStrCmp(str, pFI->filename) == 0);
}

void FileInfoDestroy(struct MEMORYPOOL* pool, PFILEINFO&  fit)
{
	if (fit->buffer != NULL) {
//		UnmapViewOfFile(fit->buffer);
		FREE(pool, fit->buffer);
	}
	if (fit->hFileMap != NULL) {
//		CloseHandle(fit->hFileMap);
	}
	FREE(pool, fit->filename);
	FREE(pool, fit);
}


BOOL sPatchServerInitDataPatching(PATCH_SERVER_CTX* pCTX)
{
	UINT32 hash_size = 0x1000; // TEMP FOR NOW
	// Initializes the hash table for file index
	if (pCTX->m_fileTable.Init(pCTX->m_pMemPool, hash_size, FileInfoStrHash, FileInfoEqual, NULL) == FALSE)
	{
		DesktopServerTrace("Patch server could not initialize data table.");
		TraceVerbose(TRACE_FLAG_PATCH_SERVER, "Error - Cannot Initialize Table.\n");
		return false;;
	}

	// Add the pakidx file into the idx table
	//	printf("Adding PakIndex to FileInfoTable\n");
	fillFileTableFromPakIndex(pCTX->m_fileTable, pCTX->m_pMemPool);

	UINT32 buffer_size, cmp_size;
	UINT8* buffer = PakIndexFillString(pCTX->m_pMemPool, &buffer_size);
	ASSERT_RETFALSE(buffer != NULL);

//	pCTX->m_dwDataListCRC = CRC(0, buffer, buffer_size);

	UINT8* cmp = (UINT8*)MALLOC(pCTX->m_pMemPool, buffer_size);
	ASSERT_RETFALSE(cmp != NULL);
	cmp_size = buffer_size;
	ASSERT_RETFALSE(ZLIB_DEFLATE(buffer, buffer_size, cmp, &cmp_size));
	//	printf("compressing %d bytes to %d bytes\n", buffer_size, cmp_size);
	pCTX->m_sizePakIdxCompressed = cmp_size;
	pCTX->m_sizePakIdxOriginal = buffer_size;
	pCTX->m_bufPakIdx = cmp;

	FREE(pCTX->m_pMemPool, buffer);
	return TRUE;
}

static BOOL sPatchServerInitExePatchingAddFromPak(
	LPWSTR fname,
	PATCH_SERVER_CTX* pCTX,
	PList<FILEINFO*>& fileList)
{
	WCHAR fixedfilename[DEFAULT_FILE_WITH_PATH_SIZE];
	FILEINFO* pFileInfo;

	PStrLower(fname, DEFAULT_FILE_WITH_PATH_SIZE);
	FixFilename(fixedfilename, fname, DEFAULT_FILE_WITH_PATH_SIZE);

	pFileInfo = pCTX->m_fileTable.GetItem(fixedfilename);
	ASSERT_RETFALSE(pFileInfo != NULL);
	ASSERT_RETFALSE(pFileInfo->bFileInPak);

	fileList.PListPushTailPool(pCTX->m_pMemPool, pFileInfo);

	return TRUE;
}

static BOOL sPatchServerInitExePatchingAddFromDisk(
	LPWSTR fname,
	PATCH_SERVER_CTX* pCTX,
	PList<FILEINFO*>& fileList)
{
	ASSERT_RETFALSE(fname != NULL);
	ASSERT_RETFALSE(pCTX != NULL);

	HANDLE hFile = NULL;
	FILEINFO* pFileInfo = NULL;
	BY_HANDLE_FILE_INFORMATION fileInfo;
	UINT32 fname_len;

	PStrLower(fname, DEFAULT_FILE_WITH_PATH_SIZE);
	hFile = CreateFileW(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	ASSERT_GOTO(hFile != INVALID_HANDLE_VALUE, _err);

	pFileInfo = pCTX->m_fileTable.GetItem(fname);
	if (pFileInfo == NULL) {
		pFileInfo = (FILEINFO*)MALLOCZ(pCTX->m_pMemPool, sizeof(FILEINFO));
		ASSERT_GOTO(pFileInfo != NULL, _err);

		ASSERT_GOTO(GetFileInformationByHandle(hFile, &fileInfo), _err);
		ASSERT_GOTO(fileInfo.nFileSizeHigh == 0, _err);
		pFileInfo->file_size_low = fileInfo.nFileSizeLow;
		pFileInfo->gentime_high = fileInfo.ftLastWriteTime.dwHighDateTime;
		pFileInfo->gentime_low = fileInfo.ftLastWriteTime.dwLowDateTime;
		pFileInfo->bFileInPak = FALSE;

		fname_len = PStrLen(fname);
		pFileInfo->filename = (LPWSTR)MALLOC(pCTX->m_pMemPool, (fname_len+1)*sizeof(WCHAR));
		ASSERT_GOTO(pFileInfo->filename != NULL, _err);
		PStrCopy(pFileInfo->filename, fname, fname_len+1);

		ASSERT_GOTO(SHA1GetFileSHA(hFile, pFileInfo->sha_hash), _err);

		//	pFileInfo->hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, fileInfo.nFileSizeLow, NULL);
		//	ASSERT_GOTO(pFileInfo->hFileMap != NULL, _err);

		//	pFileInfo->buffer = MapViewOfFile(pFileInfo->hFileMap, FILE_MAP_READ, 0, 0, fileInfo.nFileSizeLow);
		//	if (pFileInfo->buffer == NULL) {
		//		printf("err %d\n", GetLastError());
		//	}
		//	ASSERT_GOTO(pFileInfo->buffer != NULL, _err);

		pFileInfo->buffer = static_cast<UINT8*>(MALLOC(pCTX->m_pMemPool, fileInfo.nFileSizeLow));
		ASSERT_GOTO(pFileInfo->buffer != NULL, _err);

		DWORD bytesRead;
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		ASSERT_GOTO(ReadFile(hFile, pFileInfo->buffer, fileInfo.nFileSizeLow, &bytesRead, NULL), _err);
		ASSERT_GOTO(bytesRead == fileInfo.nFileSizeLow, _err);

		pCTX->m_fileTable.AddItem(fname, pFileInfo);
	}
	fileList.PListPushTailPool(pCTX->m_pMemPool, pFileInfo);

	CloseHandle(hFile);
	return TRUE;

_err:
	if (pFileInfo != NULL) {
		if (pFileInfo->buffer != NULL) {
//			UnmapViewOfFile(pFileInfo->buffer);
			FREE(pCTX->m_pMemPool, pFileInfo->buffer);
		}
		if (pFileInfo->hFileMap != NULL) {
			CloseHandle(pFileInfo->hFileMap);
		}
		if (pFileInfo->filename != NULL) {
			FREE(pCTX->m_pMemPool, pFileInfo->filename);
		}
		FREE(pCTX->m_pMemPool, pFileInfo);
	}
	if (hFile != NULL) {
		CloseHandle(hFile);
	}
	return FALSE;
}

BOOL sPatchServerInitExePatchingFileList(
	CMarkup& fileXML,
	PATCH_SERVER_CTX* pCTX,
	LPCTSTR sBuildType)
{
	CHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	WCHAR fname_w[DEFAULT_FILE_WITH_PATH_SIZE+1];
	UINT32 len;

	ASSERT_RETFALSE(pCTX != NULL);

	PStrCvt(fname_w, sBuildType, DEFAULT_FILE_WITH_PATH_SIZE);
	len = PStrLen(sBuildType);
	fname_w[len] = L'\\';
	len++;

	ASSERT_RETFALSE(fileXML.FindChildElem("Executable"));
	ASSERT_RETFALSE(fileXML.IntoElem());
	while (fileXML.FindChildElem("File")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile));
	}
	while (fileXML.FindChildElem("FileDev")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile));
	}
	if (pCTX->m_bUpdateLauncherFiles) while (fileXML.FindChildElem("LauncherFile")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile));
	}

	ASSERT_RETFALSE(fileXML.OutOfElem());

	ASSERT_RETFALSE(fileXML.FindChildElem("Executable64"));
	ASSERT_RETFALSE(fileXML.IntoElem());
	while (fileXML.FindChildElem("File")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile64));
	}
	while (fileXML.FindChildElem("FileDev")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile64));
	}
	if (pCTX->m_bUpdateLauncherFiles) while (fileXML.FindChildElem("LauncherFile")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		PStrCvt(fname_w + len, fname, DEFAULT_FILE_WITH_PATH_SIZE - len);
		ASSERT_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listImportantFile64));
	}
	ASSERT_RETFALSE(fileXML.OutOfElem());

	ASSERT_RETFALSE(fileXML.FindChildElem("DataFileList"));
	ASSERT_RETFALSE(fileXML.IntoElem());
	while (fileXML.FindChildElem("FileOnDisk")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		{
			UINT32 length = PStrLen(fname);
			while (length > 0 && (fname[length-1] == '\r' || fname[length-1] == '\n')) {
				fname[length-1] = '\0';
				length--;
			}
		}
		PStrCvt(fname_w, fname, DEFAULT_FILE_WITH_PATH_SIZE);
		ASSERTV_CONTINUE(sPatchServerInitExePatchingAddFromDisk(fname_w, pCTX, pCTX->m_listTrivialFile),
			"Cannot find file on disk %S", fname_w);
	}
	while (fileXML.FindChildElem("FileInPak")) {
		ASSERT_CONTINUE(fileXML.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		{
			UINT32 length = PStrLen(fname);
			while (length > 0 && (fname[length-1] == '\r' || fname[length-1] == '\n')) {
				fname[length-1] = '\0';
				length--;
			}
		}
		PStrCvt(fname_w, fname, DEFAULT_FILE_WITH_PATH_SIZE);
		ASSERTV_CONTINUE(sPatchServerInitExePatchingAddFromPak(fname_w, pCTX, pCTX->m_listTrivialFile),
			"Cannot find file in pak %S", fname_w);
	}
	ASSERT_RETFALSE(fileXML.OutOfElem());

	return TRUE;
}

UINT32 sPatchServerInitExePatchingGetBufferSize(
	PList<FILEINFO*>& fileList,
	UINT32& disk_file_num,
	UINT32& pak_file_num,
	BOOL bFileInPak)
{
	PList<FILEINFO*>::USER_NODE* pNode = NULL;
	UINT32 size = 0;

	pNode = fileList.GetNext(NULL);
	while (pNode != NULL) {
		ASSERT_CONTINUE(pNode->Value->bFileInPak == bFileInPak);
		if (pNode->Value->bFileInPak) {
			size += (PStrLen(pNode->Value->filename)+2) * sizeof(WCHAR);
			size += sizeof(UINT32);
			pak_file_num++;
		} else {
			size += (PStrLen(pNode->Value->filename)+1) * sizeof(WCHAR);
			disk_file_num++;
		}
		size += SHA1HashSize;

		pNode = fileList.GetNext(pNode);
	}
	return size;
}

BOOL sPatchServerInitExePatchingFillBuffer(
	PList<FILEINFO*>& fileList,
	UINT8*& buffer,
	BOOL bFileInPak)
{
	PList<FILEINFO*>::USER_NODE* pNode = NULL;
	UINT32 fname_len, fpath_len;
	UINT8* tmp;

	tmp = buffer;

	for (pNode = fileList.GetNext(NULL); pNode != NULL; pNode = fileList.GetNext(pNode)) {
		ASSERT_CONTINUE(pNode->Value->bFileInPak == bFileInPak);
		if (!pNode->Value->bFileInPak) {
			PStrCopy((WCHAR*)tmp, pNode->Value->filename, PStrLen(pNode->Value->filename)+1);
			tmp += (PStrLen(pNode->Value->filename)+1) * sizeof(WCHAR);
			ASSERT_CONTINUE(MemoryCopy(tmp, SHA1HashSize, pNode->Value->sha_hash, SHA1HashSize));
			tmp += SHA1HashSize;
		} else {
			ASSERT_RETFALSE(pNode->Value->fpath);
			ASSERT_RETFALSE(pNode->Value->fname);

			fpath_len = PStrLen(pNode->Value->fpath);
			PStrCvt((WCHAR*)tmp, pNode->Value->fpath, fpath_len+1);
			tmp += (fpath_len + 1) * sizeof(WCHAR);

			fname_len = PStrLen(pNode->Value->fname);
			PStrCvt((WCHAR*)tmp, pNode->Value->fname, fname_len+1);
			tmp += (fname_len + 1) * sizeof(WCHAR);

			*((UINT32*)tmp) = htonl(PakFileGetTypeByID(PakFileGetID(pNode->Value->pakfile)));
			tmp += sizeof(UINT32);
			ASSERT_RETFALSE(MemoryCopy(tmp, SHA1HashSize, pNode->Value->sha_hash, SHA1HashSize));
			tmp += SHA1HashSize;
		}
	}

	buffer = tmp;
	return TRUE;
}

BOOL sPatchServerInitExePatchingBuildFileListBuffer(
	PList<FILEINFO*>&			listImportantFile,
	PList<FILEINFO*>&			listTrivialFile,
	UINT8*&						pBufExeIdx,
	UINT32&						sizeExeIdxCompressed, 
	UINT32&						sizeExeIdxOriginal,
	MEMORYPOOL*					pPool)
{
	UINT32 size = 0, size_compressed;
	UINT8* buf_original = NULL, *tmp;
	UINT8* buf_compressed = NULL;
	UINT32 disk_file_num = 0, pak_file_num = 0;

	size += 2 * sizeof(UINT32);
	size += sPatchServerInitExePatchingGetBufferSize(listImportantFile, disk_file_num, pak_file_num, FALSE);
	size += sPatchServerInitExePatchingGetBufferSize(listTrivialFile, disk_file_num, pak_file_num, TRUE);

	buf_original = (UINT8*)MALLOC(pPool, size);
	buf_compressed = (UINT8*)MALLOC(pPool, size);
	ASSERT_GOTO(buf_compressed != NULL && buf_original != NULL, _err);

	tmp = buf_original;

	*((UINT32*)tmp) = htonl(disk_file_num);
	tmp += sizeof(UINT32);
	*((UINT32*)tmp) = htonl(pak_file_num);
	tmp += sizeof(UINT32);
	ASSERT_GOTO(sPatchServerInitExePatchingFillBuffer(listImportantFile, tmp, FALSE), _err);
	ASSERT_GOTO(sPatchServerInitExePatchingFillBuffer(listTrivialFile, tmp, TRUE), _err);
	size_compressed = size;
	ASSERT_GOTO(ZLIB_DEFLATE(buf_original, size, buf_compressed, &size_compressed), _err);

	pBufExeIdx = buf_compressed;
	sizeExeIdxCompressed = size_compressed;
	sizeExeIdxOriginal = size;
	FREE(pPool, buf_original);
	return TRUE;

_err:
	if (buf_compressed != NULL) {
		FREE(pPool, buf_compressed);
	}
	if (buf_original != NULL) {
		FREE(pPool, buf_original);
	}
	return FALSE;
}

BOOL sPatchServerInitExePatching(
	PATCH_SERVER_CTX* pCTX,
	LPCSTR sGameType,
	LPCSTR sBuildType)
{
	CMarkup fileXML;
	char sFileList[DEFAULT_FILE_WITH_PATH_SIZE];
	char list_filename[DEFAULT_FILE_WITH_PATH_SIZE];

	ASSERT_RETFALSE(pCTX != NULL);
	ASSERT_RETFALSE(sGameType != NULL);
	ASSERT_RETFALSE(sBuildType != NULL);

	if (PStrICmp(sGameType, "MythosFillPak") == 0 ||
		PStrICmp(sGameType, "HellgateFillPak") == 0) {
		return TRUE;
	}

	PStrPrintf(sFileList, DEFAULT_FILE_WITH_PATH_SIZE, "%sFileList", sGameType);
	PStrPrintf(list_filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, EXE_LIST_FILE);
	if(!fileXML.Load(list_filename))
	{
		DesktopServerTrace(
			"\n*** Patch server could not find the pre-startup patch file list. Filename: %s\n",
			list_filename);
		TraceError(
			"Patch server could not find the pre-startup patch file list. Filename: %s",
			list_filename);
		return FALSE;
	}
	ASSERT_RETFALSE(fileXML.FindElem("FileList"));
	ASSERT_RETFALSE(fileXML.FindChildElem(sFileList));
	ASSERT_RETFALSE(fileXML.IntoElem());
	ASSERT_RETFALSE(sPatchServerInitExePatchingFileList(fileXML, pCTX, sBuildType));

	ASSERT_RETFALSE(sPatchServerInitExePatchingBuildFileListBuffer(
		pCTX->m_listImportantFile,
		pCTX->m_listTrivialFile,
		pCTX->m_bufExeIdx,
		pCTX->m_sizeExeIdxCompressed,
		pCTX->m_sizeExeIdxOriginal,
		pCTX->m_pMemPool));
	ASSERT_RETFALSE(sPatchServerInitExePatchingBuildFileListBuffer(
		pCTX->m_listImportantFile64,
		pCTX->m_listTrivialFile,
		pCTX->m_bufExeIdx64,
		pCTX->m_sizeExeIdx64Compressed,
		pCTX->m_sizeExeIdx64Original,
		pCTX->m_pMemPool));

	return TRUE;
}


void PatchServerEntryPoint(void* pContext)
{
	ASSERT_RETURN(pContext);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(pContext);
	ServerCtx.ThreadEntryPoint();
	MEMORYPOOL_DELETE(ServerCtx.m_pMemPool, &ServerCtx);
}


void* PatchServerInit(MEMORYPOOL* pMemPool, SVRID SvrId, const class SVRCONFIG* pSvrConfig)
{
	CSAutoLock lock(AppGetSvrInitLock());
	if (!pSvrConfig)
		return 0;
	const PatchServerConfig& Config = *static_cast<const PatchServerConfig*>(pSvrConfig);
	PATCH_SERVER_CTX* pServerCtx = 0; 

	try { 
		pServerCtx = MEMORYPOOL_NEW(pMemPool) PATCH_SERVER_CTX(pMemPool, SvrId, Config);

		if (PStrICmp(Config.GameType, "Mythos") == 0 || PStrICmp(Config.GameType, "MythosFillPak") == 0)
			PrimeInitAppNameAndMutexGlobals("-tugboat");
		else if (PStrICmp(Config.GameType, "Hellgate") == 0 || PStrICmp(Config.GameType, "HellgateFillPak") == 0)
			PrimeInitAppNameAndMutexGlobals(0);
		else
			throw exception("unknown game type");

		if (!PakFileInitForApp()) {
			LogErrorInEventLog(EventCategoryPatchServer, MSG_ID_PATCH_SERVER_PAKFILE_ERROR, 0, 0, 0);
			throw exception("failed to load pakfiles from disk");
		}
		pServerCtx->m_bUpdateLauncherFiles = (PStrICmp(Config.UpdateLauncherFiles, _T("true")) == 0);
		FileIOInit();
		ASSERT_THROW(AsyncFileSystemInit(), exception());
		ASSERT_THROW(sPatchServerInitDataPatching(pServerCtx), exception());
		ASSERT_THROW(sPatchServerInitExePatching(pServerCtx, Config.GameType, Config.BuildType), exception());
		PakFileFreeForApp();

		TraceInfo(TRACE_FLAG_PATCH_SERVER, "PatchServer initialized.\n");
		return pServerCtx;
	}
	catch (const exception& Exception) {
		TraceError("PatchServerInit() failed: %s\n", Exception.what());
		MEMORYPOOL_DELETE(pMemPool, pServerCtx);
		return 0;
	}
}

void PatchServerCommandCallback(void* pContext, SR_COMMAND Command)
{
	ASSERT_RETURN(pContext);
	PATCH_SERVER_CTX& ServerCtx = *static_cast<PATCH_SERVER_CTX*>(pContext);

	switch (Command) {
		case SR_COMMAND_SHUTDOWN: ServerCtx.Shutdown(); break;
	}
}

void PatchServerXmlMessageHandler(
	LPVOID pContext, 
	struct XMLMESSAGE *	senderSpecification,
	struct XMLMESSAGE *	msgSpecification,
	LPVOID /*userData*/ )
{
	ASSERT_RETURN(pContext && senderSpecification && msgSpecification);

	PATCH_SERVER_CTX * pServer = (PATCH_SERVER_CTX*)pContext;
	ASSERT_RETURN(pServer->m_pPerfInstance);

	//	for now only handle the "reportmetrics" request
	ASSERT_RETURN(PStrICmp(msgSpecification->MessageName, L"reportmetrics") == 0);

	WCHAR msgBuff[2048];
	PStrPrintf(
		msgBuff,
		2048,
		L"<patchmetrics>"
			L"<clientcount>%u</clientcount>"
			L"<queuelength>%u</queuelength>"
		L"</patchmetrics>",
		(DWORD)pServer->m_pPerfInstance->numCurrentClients,
		(DWORD)pServer->m_pPerfInstance->numWaitingClients);

	ASSERT(SvrClusterSendXMLMessageToServerMonitorFromServer(msgBuff));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PATCH_SERVER_CTX

PATCH_SERVER_CTX::PATCH_SERVER_CTX
		(MEMORYPOOL* pMemPool, SVRID SvrId, const PatchServerConfig& Config) /*throw(exception)*/ :
	m_pMemPool(pMemPool),
	m_SvrId(SvrId),
	m_CriticalSection(true),
	m_hEventDone(0),
	m_hEventWork(0),
	m_hOneSecondTimer(0),
	m_uTotalBandwidthLimit(Config.TotalBandwidthLimit),
	m_uTotalUnackedLimit(Config.TotalUnackedLimit),
	m_uClientUnackedSoftLimit(Config.ClientUnackedSoftLimit),
	m_uClientUnackedHardLimit(Config.ClientUnackedHardLimit),
	m_uUnackedBytes(0),
	m_TokenBucket(Config.TotalBandwidthLimit, Config.TotalBandwidthLimit),
	m_WaitingClientQueue(pMemPool),
	m_ReadyClientQueue(pMemPool),
//	m_dwDataListCRC(0),
	m_StalledClientSet(set_mp<PATCH_CLIENT_CTX*>::type::key_compare(), pMemPool)
{
	try {
		m_hEventDone = CreateEvent(0, false, false, 0);
		ASSERT_THROW(m_hEventDone, exception("failed to create event"));
		m_hEventWork = CreateEvent(0, false, false, 0);
		ASSERT_THROW(m_hEventWork, exception("failed to create semaphore"));

		ASSERT_THROW(CreateTimerQueueTimer
				(&m_hOneSecondTimer, 0, OneSecondTimerCallback, this, 1000, 1000, WT_EXECUTEINIOTHREAD), 
			exception("failed to create timer"));

		wchar_t szInstanceName[100] = L"";
		PStrPrintf(szInstanceName, arrsize(szInstanceName), L"PatchServer_%hu", ServerIdGetInstance(SvrId));
		m_pPerfInstance = PERF_GET_INSTANCE(PatchServer, szInstanceName);
		ASSERT_THROW(m_pPerfInstance, exception("failed to create perf counter"));

		m_listImportantFile.Init();
		m_listImportantFile64.Init();
		m_listTrivialFile.Init();
	}
	catch (...) {
		if (m_hEventDone)
			ASSERT(CloseHandle(m_hEventDone));
		if (m_hEventWork)
			ASSERT(CloseHandle(m_hEventWork));
		if (m_hOneSecondTimer)
			ASSERT(DeleteTimerQueueTimer(0, m_hOneSecondTimer, INVALID_HANDLE_VALUE));
		throw;
	}
}


PATCH_SERVER_CTX::~PATCH_SERVER_CTX(void)
{
	DeleteAllClientsInSet(m_pMemPool, &m_ClientSet);
	DeleteAllClientsInSet(m_pMemPool, &m_DisconnectedClientSet);

	m_fileTable.Destroy(FileInfoDestroy);
	AsyncFileSystemFree();
	FileIOFree();
	//PakFileFreeForApp();

	m_listImportantFile.Destroy(m_pMemPool);
	m_listImportantFile64.Destroy(m_pMemPool);
	m_listTrivialFile.Destroy(m_pMemPool);

	if (m_hEventDone)
		ASSERT(CloseHandle(m_hEventDone));
	if (m_hEventWork)
		ASSERT(CloseHandle(m_hEventWork));
	if (m_hOneSecondTimer)
		ASSERT(DeleteTimerQueueTimer(0, m_hOneSecondTimer, INVALID_HANDLE_VALUE));

	if (m_pPerfInstance)
		PERF_FREE_INSTANCE(m_pPerfInstance);
	if (m_bufPakIdx)
		FREE(m_pMemPool, m_bufPakIdx);
	if (m_bufExeIdx)
		FREE(m_pMemPool, m_bufExeIdx);
	if (m_bufExeIdx64)
		FREE(m_pMemPool, m_bufExeIdx64);
}


void PATCH_SERVER_CTX::Shutdown(void)
{
	CSAutoLock Lock(&m_CriticalSection);
	while (!m_ClientSet.empty())
		DisconnectClient(*m_ClientSet.begin());
	SetEvent(m_hEventDone);
}


void PATCH_SERVER_CTX::AddClient(PATCH_CLIENT_CTX* pClientCtx)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());
	m_ClientSet.insert(pClientCtx);
	m_uUnackedBytes += pClientCtx->uUnackedBytes; // should be 0
	pClientCtx->eState = PATCH_CLIENT_CTX::WAITING;
	m_WaitingClientQueue.push_back(pClientCtx);
	SetEvent(m_hEventWork); // let the worker thread promote him
}


void PATCH_SERVER_CTX::DisconnectClient(PATCH_CLIENT_CTX* pClientCtx)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());

	if (pClientCtx->eState == PATCH_CLIENT_CTX::DISCONNECTED)
		return;
	else if (pClientCtx->eState == PATCH_CLIENT_CTX::WAITING) {
		TClientQueue::iterator it = remove(m_WaitingClientQueue.begin(), m_WaitingClientQueue.end(), pClientCtx);
		ASSERT(distance(it, m_WaitingClientQueue.end()) == 1);
		m_WaitingClientQueue.erase(it, m_WaitingClientQueue.end());
	}
	else if (pClientCtx->eState == PATCH_CLIENT_CTX::READY) {
		TClientQueue::iterator it = remove(m_ReadyClientQueue.begin(), m_ReadyClientQueue.end(), pClientCtx);
		ASSERT(distance(it, m_ReadyClientQueue.end()) == 1);
		m_ReadyClientQueue.erase(it, m_ReadyClientQueue.end());
	}
	else if (pClientCtx->eState == PATCH_CLIENT_CTX::STALLED)
		ASSERT(m_StalledClientSet.erase(pClientCtx) == 1);

	pClientCtx->eState = PATCH_CLIENT_CTX::DISCONNECTED;
	SvrNetDisconnectClient(pClientCtx->ClientId); 
	ASSERT(m_DisconnectedClientSet.insert(pClientCtx).second);
	m_uUnackedBytes -= pClientCtx->uUnackedBytes;
	ASSERT(m_ClientSet.erase(pClientCtx) == 1);
}


void PATCH_SERVER_CTX::ActivateClient(PATCH_CLIENT_CTX* pClientCtx)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());
	if (!pClientCtx->IsAllowedToSend())
		return;
	if (pClientCtx->eState == PATCH_CLIENT_CTX::STALLED)
		m_StalledClientSet.erase(pClientCtx);
	if (pClientCtx->eState != PATCH_CLIENT_CTX::READY) {
		pClientCtx->eState = PATCH_CLIENT_CTX::READY;
		m_ReadyClientQueue.push_back(pClientCtx);
	}
	SetEvent(m_hEventWork); // let the worker thread service the client
}


void PATCH_SERVER_CTX::ThreadEntryPoint(void)
{
	DBG_ASSERT(!m_CriticalSection.IsOwnedByCurrentThread());

	HANDLE WaitHandles[2] = { m_hEventDone, m_hEventWork };
	for (;;) {
		UINT32 uResult = WaitForMultipleObjects(2, WaitHandles, false, 1);
		if (uResult == WAIT_OBJECT_0)
			return;
		ASSERT_RETURN(uResult == WAIT_OBJECT_0 + 1 || uResult == WAIT_TIMEOUT);
		
		for (int i = 0; i < 100; ++i) { 
			// release the lock every iteration, so we can handle incoming messages. yield after 100 iterations.
			// FIXME: use a mailbox for incoming messages, so we can process them within this loop
			CSAutoLock Lock(&m_CriticalSection);
			// for each patching clients, send data up to their unacked limit
			// only check server bandwidth throttle when changing clients, so we don't thrash too hard
			if (!m_TokenBucket.CanSend())
				break;
			if (m_ReadyClientQueue.empty() && !PromoteWaitingClient())
				break;
			PATCH_CLIENT_CTX* pClientCtx = m_ReadyClientQueue.front();
			m_ReadyClientQueue.pop_front();
			ASSERT_CONTINUE(pClientCtx);
			ASSERT(pClientCtx->eState == PATCH_CLIENT_CTX::READY);
			pClientCtx->eState = PATCH_CLIENT_CTX::IDLE; // i.e. no longer in the ready queue
			ServeRequestsToClient(pClientCtx, false);
		}
	}
}


/*static*/ void CALLBACK PATCH_SERVER_CTX::OneSecondTimerCallback(void* pThis, BOOLEAN /*bTimerOrWaitFired*/)
{
	ASSERT_RETURN(pThis);
	PATCH_SERVER_CTX& This = *static_cast<PATCH_SERVER_CTX*>(pThis);
	CurrentSvrSetThreadContext(This.m_SvrId);
	CSAutoLock Lock(&This.m_CriticalSection);

	This.TrickleDataToStalledClients();
	This.SendQueuePositionMsgsToWaitingClients();

	// clean up any clients that have been disconnected
	This.DeleteAllClientsInSet(This.m_pMemPool, &This.m_DisconnectedClientSet);

	This.m_pPerfInstance->numCurrentClients = This.m_ClientSet.size();
	This.m_pPerfInstance->numWaitingClients = This.m_WaitingClientQueue.size();
	This.m_pPerfInstance->numReadyClients = This.m_ReadyClientQueue.size();
	This.m_pPerfInstance->numStalledClients = This.m_StalledClientSet.size();
	This.m_pPerfInstance->UnackedBytes = This.m_uUnackedBytes;
	CurrentSvrSetThreadContext(INVALID_SVRID);
}


void PATCH_SERVER_CTX::TrickleDataToStalledClients(void)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());

	// send a trickle message to stalled clients, in case they forgot to ack us
	vector<PATCH_CLIENT_CTX*> StalledClients;
	StalledClients.reserve(m_StalledClientSet.size());
	for (TClientSet::iterator it = m_StalledClientSet.begin(); it != m_StalledClientSet.end(); ++it)  {
		ASSERT((*it)->eState == PATCH_CLIENT_CTX::STALLED);
		(*it)->eState = PATCH_CLIENT_CTX::IDLE; // i.e. no longer in the stalled set
		StalledClients.push_back(*it);
	}
	m_StalledClientSet.clear();
	
	// if they are still stalled (and they probably will be) they will be re-added to the set
	for (vector<PATCH_CLIENT_CTX*>::iterator it = StalledClients.begin(); it != StalledClients.end(); ++it) 
		ServeRequestsToClient(*it, true);
}


void PATCH_SERVER_CTX::SendQueuePositionMsgsToWaitingClients(void)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());
	PATCH_SERVER_RESPONSE_QUEUE_POSITION_MSG Msg;
	Msg.nQueueLength = static_cast<int>(m_WaitingClientQueue.size());

	vector<PATCH_CLIENT_CTX*> WaitingClients(m_WaitingClientQueue.begin(), m_WaitingClientQueue.end());
	for (unsigned u = 0; u < WaitingClients.size(); ++u) {
		if (WaitingClients[u]->uClientVersion < PATCH_CLIENT_VERSION_QUEUE_POSITION_MSG)
			continue;
		Msg.nQueuePosition = u + 1;
		WaitingClients[u]->SendResponse(Msg, PATCH_SERVER_RESPONSE_QUEUE_POSITION);
	}
}


void PATCH_SERVER_CTX::ServeRequestsToClient(PATCH_CLIENT_CTX* pClientCtx, bool bTrickle)
{
	ASSERT_RETURN(pClientCtx);
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());
	if (!pClientCtx->ServeRequests(bTrickle)) // drop this client  maybe he can recover by relogging in.
		TraceWarn(TRACE_FLAG_PATCH_SERVER, "error serving files to client 0x%llx\n", pClientCtx->ClientId);
	else if (pClientCtx->IsAllowedToSend()) {
		pClientCtx->eState = PATCH_CLIENT_CTX::READY;
		m_ReadyClientQueue.push_back(pClientCtx);
	}
	else if (pClientCtx->IsStalledByUnackedData()) {
		pClientCtx->eState = PATCH_CLIENT_CTX::STALLED;
		m_StalledClientSet.insert(pClientCtx);
	}
	else
		pClientCtx->eState = PATCH_CLIENT_CTX::IDLE;
}


bool PATCH_SERVER_CTX::PromoteWaitingClient(void)
{
	DBG_ASSERT(m_CriticalSection.IsOwnedByCurrentThread());
	// the limit on total unacked bytes is enforced here so that we cannot get in a state where it prevents the 
	// sending of data to non-stalled clients (which never get trickle messages).
	if (m_WaitingClientQueue.empty() || m_uUnackedBytes >= m_uTotalUnackedLimit)
		return false;
	PATCH_CLIENT_CTX* pClientCtx = m_WaitingClientQueue.front();
	m_WaitingClientQueue.pop_front();
	ASSERT(pClientCtx->eState == PATCH_CLIENT_CTX::WAITING);
	pClientCtx->eState = PATCH_CLIENT_CTX::READY;
	m_ReadyClientQueue.push_back(pClientCtx);
	return true;
}


/*static*/ void PATCH_SERVER_CTX::DeleteAllClientsInSet(MEMORYPOOL* pMemPool, TClientSet* pClientSet)
{
	ASSERT_RETURN(pClientSet);
	for (TClientSet::const_iterator it = pClientSet->begin(); it != pClientSet->end(); ++it)
		MEMORYPOOL_DELETE(pMemPool, *it);
	pClientSet->clear();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PATCH_CLIENT_CTX

PATCH_CLIENT_CTX::PATCH_CLIENT_CTX
		(PATCH_SERVER_CTX& SC, SERVICEUSERID CId, const UINT8* pKey, UINT32 uKenLen) :
	ServerCtx(SC), 
	ClientId(CId),
	uClientVersion(0),
	uClientBuildType(BUILD_UNDEFINED),
	eState(IDLE),
	uUnsentBytes(0),
	uUnackedBytes(0),
	EncryptionKey(pKey, pKey + uKenLen, ServerCtx.m_pMemPool), 
	RequestQueue(ServerCtx.m_pMemPool)
{ }


PATCH_CLIENT_CTX::~PATCH_CLIENT_CTX(void) 
{ 
	for (TRequestQueue::const_iterator it = RequestQueue.begin(); it != RequestQueue.end(); ++it)
		MEMORYPOOL_DELETE(ServerCtx.m_pMemPool, *it);
	RequestQueue.clear();
}


bool PATCH_CLIENT_CTX::IsAllowedToSend(void) const 
	{ return eState != WAITING && uUnsentBytes > 0 && uUnackedBytes < ServerCtx.m_uClientUnackedSoftLimit; }


bool PATCH_CLIENT_CTX::IsStalledByUnackedData(void) const 
	{ return eState != WAITING && uUnsentBytes > 0 && uUnackedBytes >= ServerCtx.m_uClientUnackedSoftLimit; }


bool PATCH_CLIENT_CTX::AddRequest(PATCH_REQUEST_CTX* pRequestCtx)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	// requests MUST have a unique uFileIdx, or we will not be able to handle the acks correctly.
	// I think it's worth leaving this check in because we are reliant on the client getting this right!
	ASSERT_RETFALSE(FindRequest(pRequestCtx->uFileIdx) == RequestQueue.end());
	ASSERT_RETFALSE(pRequestCtx->GetUnsentBytes() == pRequestCtx->GetTotalBytes());
	ASSERT_RETFALSE(pRequestCtx->GetUnackedBytes() == 0);

	TRequestQueue::iterator itInsertAt 
		= lower_bound(RequestQueue.begin(), RequestQueue.end(), pRequestCtx, RequestPriorityCmp);
	RequestQueue.insert(itInsertAt, pRequestCtx);
	uUnsentBytes += pRequestCtx->GetUnsentBytes();
	ServerCtx.ActivateClient(this);
	return true;
}


bool PATCH_CLIENT_CTX::AckRequest(UINT8 uFileIdx, UINT32 uAckedBytes, bool bCompleted)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	// ignore acks if there is not corresponding file request, i.e.  acks from exe and data list requests.  
	TRequestQueue::iterator itRequest = FindRequest(uFileIdx);
	if (itRequest == RequestQueue.end())
		return true;
	
	// make sure everything looks consistent.  don't trust the client too much.
	ASSERT_RETFALSE(uAckedBytes >= (*itRequest)->uAckedBytes); // can't reverse previous acks!
	UINT32 uTotalBytes = (*itRequest)->GetTotalBytes();
	ASSERT_RETFALSE(uAckedBytes <= uTotalBytes); // can't ack more than the request total!
	bool bAllBytesAcked = uAckedBytes == uTotalBytes;
	ASSERT(bCompleted == bAllBytesAcked);
	uAckedBytes = bCompleted ? uTotalBytes : uAckedBytes;

	// update the acked by counts of the request and client
	UINT32 uAckedBytesDelta = uAckedBytes - (*itRequest)->uAckedBytes;
	(*itRequest)->uAckedBytes = uAckedBytes;
	if (uAckedBytes == uTotalBytes) {
		MEMORYPOOL_DELETE(ServerCtx.m_pMemPool, *itRequest);
		RequestQueue.erase(itRequest);// done with this request!
	}
	ASSERT_RETFALSE(uUnackedBytes >= uAckedBytesDelta); // can't ack more than client total!
	uUnackedBytes -= uAckedBytesDelta;
	ServerCtx.m_uUnackedBytes -= uAckedBytesDelta;
	ServerCtx.ActivateClient(this);
	return true;
}


bool PATCH_CLIENT_CTX::ServeRequests(bool bTrickle)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	if (uUnsentBytes == 0)
		return true;
	ASSERT_RETFALSE(!RequestQueue.empty()); // !#(@$&!%!!!!! our byte count got out of sync!

	for (TRequestQueue::iterator itRequest = RequestQueue.begin(); itRequest != RequestQueue.end(); ++itRequest) {
		ASSERT_RETFALSE(*itRequest);
		PATCH_REQUEST_CTX& Request = **itRequest;
		if (Request.GetUnsentBytes() == 0)
			continue;
		if (bTrickle) { // send one tiny message for each request 
			if (!SendNextMsgForRequest(Request, true))
				return false;
			continue;
		}
		// complete the requests in order, until we run into client bandwidth limits
		while (Request.GetUnsentBytes() > 0) {
			if (!IsAllowedToSend())
				return true; 
			if (!SendNextMsgForRequest(Request, false))
				return false;
		}
	}
	return true;
}


bool PATCH_CLIENT_CTX::SendNextMsgForRequest(PATCH_REQUEST_CTX& RequestCtx, bool bTrickle)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	if (RequestCtx.GetUnsentBytes() == 0)
		return true;
	if (uUnackedBytes > ServerCtx.m_uClientUnackedSoftLimit + ServerCtx.m_uClientUnackedHardLimit) {
		TraceWarn(TRACE_FLAG_PATCH_SERVER, "client 0x%llx exceeded hard unacked limit\n", ClientId);
		ServerCtx.DisconnectClient(this);
		return false; 
	}

	UINT32 uMsgSentBytes = SendMsg(RequestCtx.FileInfo.buffer, RequestCtx.uSentBytes, RequestCtx.GetTotalBytes(), 
		RequestCtx.uFileIdx, bTrickle, false);
	if (uMsgSentBytes == 0)
		return false;
	RequestCtx.uSentBytes += uMsgSentBytes;
	ASSERT_RETFALSE(RequestCtx.uSentBytes <= RequestCtx.GetTotalBytes());
	uUnsentBytes -= uMsgSentBytes;
	uUnackedBytes += uMsgSentBytes;
	ServerCtx.m_uUnackedBytes += uMsgSentBytes;

	if (RequestCtx.GetUnsentBytes() == 0) {
		++ServerCtx.m_pPerfInstance->numFilesPatched;
		TraceVerbose(TRACE_FLAG_PATCH_SERVER, "client 0x%llx finished file %ls (idx 0x%x)\n",
			ClientId, RequestCtx.FileInfo.filename, RequestCtx.uFileIdx);
	}
	return true;
}


bool PATCH_CLIENT_CTX::SendBufferImmediately(const UINT8* pBuffer, UINT32 uOriginalSize, UINT32 uCompressedSize)
{
	ASSERT_RETFALSE(pBuffer && uCompressedSize <= uOriginalSize);
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	
	PATCH_SERVER_RESPONSE_COMPRESSION_SIZE_MSG msgSize;
	msgSize.sizeOriginal = uOriginalSize;
	msgSize.sizeCompressed = uCompressedSize;
	SRNET_RETURN_CODE iReturn = SendResponse(msgSize, PATCH_SERVER_RESPONSE_COMPRESSION_SIZE);
	if (iReturn != SRNET_SUCCESS)
		return false;

	for (unsigned uSentBytes = 0; uSentBytes < uCompressedSize;) {
		unsigned uMsgSentBytes = SendMsg(pBuffer, uSentBytes, uCompressedSize, 0, false, true);
		if (uMsgSentBytes == 0)
			return false;
		uSentBytes += uMsgSentBytes;
	}
	return true;
}


UINT32 PATCH_CLIENT_CTX::SendMsg(const UINT8* pSourceBuffer, UINT32 uSentBytes, UINT32 uTotalBytes, UINT8 uFileIdx, 
	bool bTrickle, bool bEncrypt)
{
	ASSERT_RETZERO(pSourceBuffer && uSentBytes < uTotalBytes);
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());

	// FIXME: these two structures are currently IDENTICAL!
	static PATCH_SERVER_RESPONSE_FILE_DATA_SMALL_MSG DataSmallMsg;
	static PATCH_SERVER_RESPONSE_FILE_DATA_LARGE_MSG DataLargeMsg;

	bool bUseLargeMsgs = uClientVersion >= PATCH_CLIENT_VERSION_LARGE_MSG_SIZE;
	UINT8* pMsgBuffer = bUseLargeMsgs ? DataLargeMsg.data : DataSmallMsg.data;
	UINT32 uMsgBufferSize = bUseLargeMsgs ? sizeof(DataLargeMsg.data) : sizeof(DataSmallMsg.data);
	// allow a little room for the encryption algorithm to expand the message
	const static unsigned uEncryptionPad = 48;
	UINT32 uMsgSentBytes = min(uTotalBytes - uSentBytes, uMsgBufferSize - (bEncrypt ? uEncryptionPad : 0));
	if (bTrickle) // limit trickle messages to 128 bytes, so that we don't overflow low bandwidth clients
		uMsgSentBytes = min(uMsgSentBytes, 128);

	ASSERT_RETZERO(MemoryCopy(pMsgBuffer, uMsgBufferSize, pSourceBuffer + uSentBytes, uMsgSentBytes));
	DWORD dwDataLen = uMsgSentBytes;
	if (bEncrypt) { 
		UINT8 IV[32];
		memset(IV, 0, sizeof(IV));
		ASSERT_RETZERO(CryptoEncryptBufferWithKey(CALG_AES_256, pMsgBuffer, &dwDataLen, uMsgBufferSize, 
			&EncryptionKey[0], static_cast<DWORD>(EncryptionKey.size()), IV));
	}
	
	// send the message and decrement the token buffer
	if (bUseLargeMsgs) {
		MSG_SET_BLOB_LEN(DataLargeMsg, data, dwDataLen);
		DataLargeMsg.idx = uFileIdx;
		DataLargeMsg.offset = uSentBytes;
		if (SendResponse(DataLargeMsg, PATCH_SERVER_RESPONSE_FILE_DATA_LARGE) != SRNET_SUCCESS)
			return 0;
	} 
	else {
		MSG_SET_BLOB_LEN(DataSmallMsg, data, dwDataLen);
		DataSmallMsg.idx = uFileIdx;
		DataSmallMsg.offset = uSentBytes;
		if (SendResponse(DataSmallMsg, PATCH_SERVER_RESPONSE_FILE_DATA_SMALL) != SRNET_SUCCESS)
			return 0;
	}
	return uMsgSentBytes;
}


SRNET_RETURN_CODE PATCH_CLIENT_CTX::SendResponse(MSG_STRUCT& MsgStruct, NET_CMD NetCmd)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	SRNET_RETURN_CODE Result = SvrNetSendResponseToClient(ClientId, &MsgStruct, NetCmd);
	if (Result != SRNET_SUCCESS) {
		TraceWarn(TRACE_FLAG_PATCH_SERVER, "client 0x%llx SvrNetSendResponseToClient failed\n", ClientId);
		ServerCtx.DisconnectClient(this);
	}
	else {
		ServerCtx.m_TokenBucket.DecrementTokenCount(MsgStruct.hdr.size);
		ServerCtx.m_pPerfInstance->numSendingBytes += MsgStruct.hdr.size;
		++ServerCtx.m_pPerfInstance->numSendingMsgs;
	}
	return Result;
}


PATCH_CLIENT_CTX::TRequestQueue::iterator PATCH_CLIENT_CTX::FindRequest(UINT8 uFileIdx)
{
	DBG_ASSERT(ServerCtx.m_CriticalSection.IsOwnedByCurrentThread());
	for (TRequestQueue::iterator it = RequestQueue.begin(); it != RequestQueue.end(); ++it)
		if ((*it)->uFileIdx == uFileIdx)
			return it;
	return RequestQueue.end();
}
	

/*static*/ bool PATCH_CLIENT_CTX::RequestPriorityCmp(const PATCH_REQUEST_CTX* r1, const PATCH_REQUEST_CTX* r2)
{
	ASSERT_RETZERO(r1 && r2);
	return r1->GetUnsentBytes() < r2->GetUnsentBytes();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PATCH_REQUEST_CTX

PATCH_REQUEST_CTX::PATCH_REQUEST_CTX(PATCH_CLIENT_CTX& cc, const FILEINFO& fi, UINT8 idx) :
	ClientCtx(cc),
	FileInfo(fi),
	uFileIdx(idx),
	uSentBytes(0),
	uAckedBytes(0)
{ }


UINT32 PATCH_REQUEST_CTX::GetTotalBytes(void) const 
	{ return FileInfo.file_size_compressed ? FileInfo.file_size_compressed : FileInfo.file_size_low; }


UINT32 PATCH_REQUEST_CTX::GetUnsentBytes(void) const 
{ 
	unsigned uTotalBytes = GetTotalBytes();
	ASSERT(uTotalBytes >= uSentBytes);
	return uTotalBytes - uSentBytes; 
}


UINT32 PATCH_REQUEST_CTX::GetUnackedBytes(void) const 
{ 
	ASSERT(uSentBytes >= uAckedBytes);
	return uSentBytes - uAckedBytes; 
}
