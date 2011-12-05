#include "patchstd.h"
#include "file_manage.h"
#include "file_manage.cpp.tmh"
using namespace COMMON_SHA1;
#define BUFFER_SIZE 1024
/*
VOID splitFilename(LPCWSTR filename, LPWSTR fpath, LPWSTR fname)
{
	UINT32 i, j, len;

	len = PStrLen(filename);
	for (i = len-1; filename[i] != '\\'; i--);

	for (j = 0; j <= i; j++) {
		fpath[j] = filename[j];
	}
	fpath[j] = '\0';

	for (i++, j = 0; filename[i] != '\0'; i++, j++) {
		fname[j] = filename[i];
	}
	fname[j] = '\0';
}

BOOL DirectoryExists(
					 LPCWSTR dir)
{
	if (CreateDirectoryW(dir, NULL) == TRUE) {
		RemoveDirectoryW(dir);
		return FALSE;
	} else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL getAllFiles(
				 LPWSTR fname,
				 UINT32 len,
				 PList<PWIN32_FIND_DATAW>& fileList,
				 MEMORYPOOL* pPool)
{
	HANDLE hFind;
	WIN32_FIND_DATAW findData;
	DWORD error;
	UINT32 nameLen = 0;

	fname[len] = '*';
	fname[len+1] = '\0';
	hFind = FindFirstFileW(fname, &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
//		printf("Error: Cannot go through directory %S\n", fname);
		return FALSE;
	}

	do {
		LPCWSTR cFileExt = findData.cFileName + PStrLen(findData.cFileName) - 4;
		if (PStrICmp(cFileExt, L".dat") == 0 ||
			PStrICmp(cFileExt, L".idx") == 0) {
				continue;
		}

		if (PStrCmp(findData.cFileName, L".", 1) != 0 &&
			PStrCmp(findData.cFileName, L"..", 2) != 0) {
				PStrPrintf(fname + len, MAX_PATH - len - 1, L"%s", findData.cFileName);
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					nameLen = PStrLen(fname);
					fname[nameLen] = '\\';
					fname[nameLen+1] = '\0';
					getAllFiles(fname, nameLen+1, fileList, pPool);
				} else {
					PWIN32_FIND_DATAW pFindData = (PWIN32_FIND_DATAW)MALLOC(pPool, sizeof(WIN32_FIND_DATAW));
					ASSERT_GOTO(pFindData != NULL, _error);
					ASSERT_GOTO(MemoryCopy(pFindData, sizeof(WIN32_FIND_DATAW), &findData, sizeof(WIN32_FIND_DATAW)),_error);
					PStrCopy(pFindData->cFileName, fname, MAX_PATH);
					PStrLower(pFindData->cFileName, PStrLen(pFindData->cFileName)+1);
					fileList.PListPushTail(pPool, pFindData);
				}
		}
	} while (FindNextFileW(hFind, &findData) != 0);

	error = GetLastError();
	FindClose(hFind);
	ASSERT_GOTO(error == ERROR_NO_MORE_FILES, _error);

	return TRUE;

_error:
	return FALSE;
}

static BOOL sGetFileSHA(
						HANDLE hFile,
						UINT8* sha_hash)
{
	ASSERT_RETFALSE(hFile != NULL);
	ASSERT_RETFALSE(sha_hash != NULL);

	UINT32 cur, total;
	SHA1Context ctx;
	UINT8 buffer[BUFFER_SIZE];

	ASSERT_RETFALSE(SHA1Reset(&ctx));
	cur = 0;
	total = GetFileSize(hFile, NULL);
	while (cur < total) {
		DWORD bytes_read, read_size = total - cur;
		read_size = (BUFFER_SIZE < read_size ? BUFFER_SIZE : read_size);
		ASSERT_RETFALSE(ReadFile(hFile, buffer, read_size, &bytes_read, NULL));
		ASSERT_RETFALSE(read_size == bytes_read);
		cur += read_size;

		ASSERT_RETFALSE(SHA1Input(&ctx, buffer, read_size));
	}
	ASSERT_RETFALSE(SHA1Result(&ctx, sha_hash));

	return TRUE;
}

static BOOL sFillFileInfo(
						  CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
						  PWIN32_FIND_DATAW pFindData,
						  UINT32& count,
						  MEMORYPOOL* pPool)
{
	FILEINFO* pFIT;
	BOOL struct_new = FALSE;

	ASSERT_RETFALSE(pFindData != NULL);
	if (pFindData->nFileSizeHigh != 0) {
		//printf("Error: File too large %S\n", pFindData->cFileName);
		return FALSE;
	}
	PStrLower(pFindData->cFileName, PStrLen(pFindData->cFileName)+1);
	pFIT = fileTable.GetItem(pFindData->cFileName);
	if (pFIT != NULL) {
		if (pFIT->gentime_high == pFindData->ftLastWriteTime.dwHighDateTime &&
			pFIT->gentime_low == pFindData->ftLastWriteTime.dwLowDateTime) {
				return TRUE;
		}
	} else {
		struct_new = TRUE;
		pFIT = (FILEINFO*) MALLOC(pPool, sizeof(FILEINFO));
		ASSERT_RETFALSE(pFIT != NULL);
		structclear((*pFIT));

		UINT32 len = PStrLen(pFindData->cFileName);
		pFIT->filename = (LPWSTR) MALLOC(pPool, (len+1)*sizeof(WCHAR));
		if (pFIT->filename == NULL) {
			FREE(pPool, pFIT);
			return FALSE;
		}
		PStrCopy(pFIT->filename, pFindData->cFileName, len+1);
		pFIT->buffer = NULL;
		fileTable.AddItem(pFIT->filename, pFIT);
	}
	pFIT->file_size_high = pFindData->nFileSizeHigh;
	pFIT->file_size_low = pFindData->nFileSizeLow;
	pFIT->gentime_high = pFindData->ftLastWriteTime.dwHighDateTime;
	pFIT->gentime_low = pFindData->ftLastWriteTime.dwLowDateTime;
	pFIT->pakfile = NULL;
	pFIT->offset_high = 0;
	pFIT->offset_low = 0;
	pFIT->bFileInPak = FALSE;

//	printf("Added/Updated %S\n", pFIT->filename);
	count++;

	if (getFileSHA(pFIT->filename, pFIT->sha_hash) == FALSE) {
		if (struct_new) {
			FREE(pPool, pFIT->filename);
			FREE(pPool, pFIT);
		}
		return FALSE;
	}

	return TRUE;
}

UINT32 fillFileTable(
					 CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
					 PList<PWIN32_FIND_DATAW>& fileList,
					 MEMORYPOOL* pPool)
{
	PList<PWIN32_FIND_DATAW>::USER_NODE* pNode;
	UINT32 count;
	pNode = NULL;
	count = 0;

	while ((pNode = fileList.GetNext(pNode)) != NULL) {
		sFillFileInfo(fileTable, pNode->Value, count, pPool);
	}
	return count;
}

static BOOL sWriteFileInfo(
						   PFILEINFO& pFI,
						   void* arg)
{
	DWORD bytes_written;
	HANDLE* phFile;

	phFile = (HANDLE*)arg;
	ASSERT_RETFALSE(phFile != NULL);

	UINT32 len = PStrLen(pFI->filename);

	ASSERT_RETFALSE(WriteFile(*phFile, &len, sizeof(UINT32), &bytes_written, NULL));
	ASSERT(bytes_written == sizeof(UINT32));

	ASSERT_RETFALSE(WriteFile(*phFile, pFI->filename, len * sizeof(WCHAR), &bytes_written, NULL));
	ASSERT(bytes_written == len * sizeof(WCHAR));

	ASSERT_RETFALSE(WriteFile(*phFile, pFI->sha_hash, SHA1HashSize, &bytes_written, NULL));
	ASSERT(bytes_written == SHA1HashSize);

	ASSERT_RETFALSE(WriteFile(*phFile, &pFI->file_size_high, sizeof(DWORD), &bytes_written, NULL));
	ASSERT(bytes_written == sizeof(DWORD));

	ASSERT_RETFALSE(WriteFile(*phFile, &pFI->file_size_low, sizeof(DWORD), &bytes_written, NULL));
	ASSERT(bytes_written == sizeof(DWORD));

	ASSERT_RETFALSE(WriteFile(*phFile, &pFI->gentime_high, sizeof(DWORD), &bytes_written, NULL));
	ASSERT(bytes_written == sizeof(DWORD));

	ASSERT_RETFALSE(WriteFile(*phFile, &pFI->gentime_low, sizeof(DWORD), &bytes_written, NULL));
	ASSERT(bytes_written == sizeof(DWORD));

	return TRUE;
}

BOOL writeFileTable(
					CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
					LPCWSTR idx_filename)
{
	UINT32 count;
	DWORD bytes_written;
	HANDLE hFile;

	hFile = CreateFileW(idx_filename, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT_RETFALSE(hFile != INVALID_HANDLE_VALUE);

	count = fileTable.GetCount();
	if (WriteFile(hFile, &count, sizeof(UINT32), &bytes_written, NULL) == FALSE ||
		bytes_written != sizeof(UINT32)) {
			CloseHandle(hFile);
			return FALSE;
	}
	if (fileTable.Apply(sWriteFileInfo, &hFile) == FALSE) {
		CloseHandle(hFile);
		return FALSE;
	} else {
		CloseHandle(hFile);
		return TRUE;
	}
}

static BOOL sReadFileInfo(
						  CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
						  HANDLE hFile,
						  MEMORYPOOL* pPool)
{
	UINT32 fname_len;
	DWORD bytes_read;
	WCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	FILEINFO* pFI = NULL;
	BOOL struct_new = FALSE;

	ASSERT_GOTO(ReadFile(hFile, &fname_len, sizeof(UINT32), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == sizeof(UINT32), _error);
	ASSERT_GOTO(fname_len < DEFAULT_FILE_WITH_PATH_SIZE, _error);

	ASSERT_GOTO(ReadFile(hFile, fname, fname_len * sizeof(WCHAR), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == fname_len * sizeof(WCHAR), _error);
	fname[fname_len] = '\0';

	pFI = fileTable.GetItem(fname);
	if (pFI == NULL) {
		pFI = (PFILEINFO) MALLOC(pPool, sizeof(FILEINFO));
		struct_new = TRUE;
		ASSERT_GOTO(pFI != NULL, _error);
		structclear((*pFI));
		pFI->filename = (LPWSTR) MALLOC(pPool, (fname_len+1)*sizeof(WCHAR));
		ASSERT_GOTO(pFI->filename != NULL, _error);
		PStrCopy(pFI->filename, fname, fname_len+1);
		pFI->buffer = NULL;
		fileTable.AddItem(fname, pFI);
	}

	ASSERT_GOTO(ReadFile(hFile, pFI->sha_hash, SHA1HashSize, &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == SHA1HashSize, _error);

	ASSERT_GOTO(ReadFile(hFile, &pFI->file_size_high, sizeof(DWORD), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == sizeof(DWORD), _error);

	ASSERT_GOTO(ReadFile(hFile, &pFI->file_size_low, sizeof(DWORD), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == sizeof(DWORD), _error);

	ASSERT_GOTO(ReadFile(hFile, &pFI->gentime_high, sizeof(DWORD), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == sizeof(DWORD), _error);

	ASSERT_GOTO(ReadFile(hFile, &pFI->gentime_low, sizeof(DWORD), &bytes_read, NULL), _error);
	ASSERT_GOTO(bytes_read == sizeof(DWORD), _error);

	pFI->bFileInPak = FALSE;
	pFI->pakfile = NULL;
	pFI->offset_high = NULL;
	pFI->offset_low = NULL;

	return TRUE;

_error:
	if (struct_new && pFI != NULL) {
		fileTable.RemoveItem(fname);
		if (pFI->filename != NULL) {
			FREE(pPool, pFI->filename);
		}
		FREE(pPool, pFI);
	}
	return FALSE;
}

BOOL readFileTable(
				   CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
				   LPCWSTR idx_fname,
				   MEMORYPOOL* pPool)
{
	HANDLE hFile;
	UINT32 count, i;
	DWORD bytes_read;

	hFile = CreateFileW(idx_fname, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT_RETFALSE(hFile != INVALID_HANDLE_VALUE);

	if (ReadFile(hFile, &count, sizeof(UINT32), &bytes_read, NULL) == FALSE ||
		bytes_read != sizeof(UINT32)) {
			CloseHandle(hFile);
			return FALSE;
	}

	for (i = 0; i < count; i++) {
		if (sReadFileInfo(fileTable, hFile, pPool) == FALSE) {
			CloseHandle(hFile);
			return FALSE;
		}
	}
	CloseHandle(hFile);
	return TRUE;
}

UINT32 getFileIndexCount(
						 LPCWSTR idx_fname)
{
	HANDLE hFile;
	UINT32 count;
	DWORD bytes_read;

	hFile = CreateFileW(idx_fname, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return ((UINT32)-1);
	}

	if (ReadFile(hFile, &count, sizeof(UINT32), &bytes_read, NULL) == FALSE ||
		bytes_read != sizeof(UINT32)) {
			CloseHandle(hFile);
			return ((UINT32)-1);
	}

	CloseHandle(hFile);
	return count;
}
*/
BOOL sCheckSHA(
			   FILE_INDEX_NODE* pFINode)
{
	/*	SHA1Context ctx;
	UINT8 sha_hash[SHA1HashSize];
	UINT8 buffer[BUFFER_SIZE];
	PAKFILE* pakfile;

	DWORD total = pFINode->size;
	DWORD cur = 0;
	DWORD size;
	UINT64 offset;

	//	_tprintf("Checking SHA %s%s\n", pFINode->m_FilePath.str, pFINode->m_FileName.str);

	pakfile = FileSystemGetPakfileByID(fs, pFINode->pakfileid);
	ASSERT(pakfile != NULL);

	SHA1Reset(&ctx);
	while (cur < total) {
	size = total - cur;
	size = (BUFFER_SIZE < size ? BUFFER_SIZE : size);
	offset = pFINode->offset + cur;
	ASSERT(PakDataRead(fs, pakfile, offset, buffer, size));
	cur += size;

	SHA1Input(&ctx, buffer, size);
	}
	SHA1Result(&ctx, sha_hash);

	if (memcmp(sha_hash, pFINode->SHA1Hash, SHA1HashSize) != 0) {
	//		_tprintf("*** File corrupted %s\n", pFINode->m_FileName.str);
	return FALSE;
	} else {
	return TRUE;
	}*/
	UNREFERENCED_PARAMETER(pFINode);
	return TRUE;
}

void sJoinFilepathFilenameW(
							LPWSTR out,
							LPSTR fpath,
							LPSTR fname)
{
	UINT32 i, j;

	j = 0;
	if (fpath) {
		for (i = 0; fpath[i] != '\0'; i++, j++) {
			out[j] = fpath[i];
		}
	}
	if (fname) {
		for (i = 0; fname[i] != '\0'; i++, j++) {
			out[j] = fname[i];
		}
	}
	out[j] = '\0';
}

void sJoinFilepathFilenameW(
							LPWSTR out,
							LPWSTR fpath,
							LPWSTR fname)
{
	UINT32 i, j;

	j = 0;
	if (fpath) {
		for (i = 0; fpath[i] != '\0'; i++, j++) {
			out[j] = fpath[i];
		}
	}
	if (fname) {
		for (i = 0; fname[i] != '\0'; i++, j++) {
			out[j] = fname[i];
		}
	}
	out[j] = '\0';
}
/*
BOOL getFileSHA(
				LPCWSTR filename,
				UINT8* sha_hash)
{
	HANDLE hFile;

	hFile = CreateFileW(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	ASSERT_RETFALSE(hFile != NULL);
	if (sGetFileSHA(hFile, sha_hash)) {
		CloseHandle(hFile);
		return TRUE;
	} else {
		CloseHandle(hFile);
		return FALSE;
	}
}
*/
FILEINFO* FillFileInfoFromPakIndex(
	CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
	FILE_INDEX_NODE* pFINode,
	MEMORYPOOL* pPool)
{
	UINT32 fname_len;
	FILEINFO* pFI;
	WCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE+1];
	ASSERT_RETNULL(pFINode != NULL);

	fname_len = pFINode->m_FilePath.Len() + pFINode->m_FileName.Len();
	sJoinFilepathFilenameW(fname, pFINode->m_FilePath.str, pFINode->m_FileName.str);
	TraceExtraVerbose(TRACE_FLAG_PATCH_SERVER, "loading %S\n", fname);

	pFI = fileTable.GetItem(fname);
	if (pFI == NULL) {
		pFI = (PFILEINFO) MALLOC(pPool, sizeof(FILEINFO));
		ASSERT_GOTO(pFI != NULL, _err);

		structclear((*pFI));
		pFI->filename = (LPWSTR) MALLOC(pPool, (fname_len+1)*sizeof(WCHAR));
		ASSERT_GOTO(pFI->filename != NULL, _err);

		PStrCopy(pFI->filename, fname, fname_len+1);
		fileTable.AddItem(pFI->filename, pFI);
	}
	pFI->bFileInPak = TRUE;
	pFI->file_size_high = 0;
	pFI->file_size_low = pFINode->m_fileSize;
	pFI->gentime_high = (DWORD)(pFINode->m_GenTime >> (sizeof(DWORD) * 8));
	pFI->gentime_low = (DWORD)(pFINode->m_GenTime);
	pFI->offset_high = (DWORD)(pFINode->m_offset >> (sizeof(DWORD) * 8));
	pFI->offset_low = (DWORD)(pFINode->m_offset);
	pFI->pakfile = PakFileGetPakByID(pFINode->m_idPakFile);
	pFI->fname = pFINode->m_FileName.str;
	pFI->fpath = pFINode->m_FilePath.str;
	pFI->file_size_compressed = pFINode->m_fileSizeCompressed;

	ASSERT_GOTO(pFI->pakfile != NULL, _err);
	
	ASSERT_GOTO(MemoryCopy(pFI->sha_hash, SHA1HashSize, pFINode->m_Hash.m_Hash, SHA1HashSize), _err);

	// Pre-load the file in memory... if we have enough memory
	UINT32 sizeToRead = (pFI->file_size_compressed == 0 ? pFINode->m_fileSize : pFI->file_size_compressed);

	pFI->buffer = static_cast<UINT8*>(MALLOC(pPool, sizeToRead));
	if (pFI->buffer == NULL) {
		BOOL b = FALSE;
		if (sizeToRead == 0) {
			ASSERTV(b, "File %s%s has 0 bytes??", pFI->fpath, pFI->fname);
		} else {
			ASSERTV(b, "Ran out of memory trying to allocate %d bytes for %s%s",
				sizeToRead, pFI->fpath, pFI->fname);
		}
	}

	if (pFI->buffer != NULL) {
		ASYNC_DATA asyncData(PakFileGetAsyncFile(pFI->pakfile), pFI->buffer, pFINode->m_offset, sizeToRead);
		if (AsyncFileReadNow(asyncData) != sizeToRead) {
			FREE(pPool, pFI->buffer);
			pFI->buffer = NULL;
		}
	}

	return pFI;

_err:
	if (pFI != NULL) {
		if (pFI->filename != NULL) {
			FREE(pPool, pFI->filename);
		}
		if (pFI->buffer != NULL) {
			FREE(pPool, pFI->buffer);
		}
		FREE(pPool, pFI);
	}
	return NULL;
}

BOOL fillFileTableFromPakIndex(
							   CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
							   MEMORYPOOL* pPool,
							   BOOL checkSHA)
{
	UINT32 i, count;
	count = PakFileGetIdxCount();

	for (i = 0; i < count; i++) {
		FILE_INDEX_NODE* pFINode = PakFileGetFileByIdx(i);

		if (!pFINode || 
			pFINode->m_fileId == INVALID_FILEID ||
			pFINode->m_iPakType == PAK_INVALID ||
			pFINode->m_FileName.str == NULL) {
				continue;
		}

		if (checkSHA == FALSE ||
			sCheckSHA(pFINode)) {
				FillFileInfoFromPakIndex(fileTable, pFINode, pPool);
		}
	}
	return TRUE;
}

UINT8* PakIndexFillString(
						  MEMORYPOOL* pPool,
						  UINT32* size)
{
	UINT32 i, buf_size, count, idxcount;

	ASSERT_RETNULL(size != NULL);
	ASSERT_RETNULL(pPool != NULL);

	buf_size = 0;
	count = 0;
	idxcount = PakFileGetIdxCount();
	for (i = 0; i < idxcount; i++) {
		FILE_INDEX_NODE* pFINode = PakFileGetFileByIdx(i);
		if (!pFINode || pFINode->m_fileId == 0 || pFINode->m_iPakType == PAK_INVALID)
			continue;

		buf_size += (pFINode->m_FileName.Len() + 1) * sizeof(WCHAR);
		buf_size += (pFINode->m_FilePath.Len() + 1) * sizeof(WCHAR);
		buf_size += SHA1HashSize;
		buf_size += sizeof(UINT32);
		count++;
	}
	buf_size += sizeof(UINT32);
	UINT8* buf = (UINT8*)MALLOC(pPool, buf_size);
	UINT8* tmp = buf;
	ASSERT_RETNULL(buf != NULL);

	*((UINT32*)tmp) = htonl(count);
	tmp += sizeof(UINT32);
	for (i = 0; i < idxcount; i++) {
		FILE_INDEX_NODE* pFINode = PakFileGetFileByIdx(i);
		if (!pFINode || pFINode->m_fileId == 0 || pFINode->m_iPakType == PAK_INVALID)
			continue;

		PStrCvt((WCHAR*)tmp, pFINode->m_FilePath.str, pFINode->m_FilePath.Len()+1);
		tmp += (pFINode->m_FilePath.Len() + 1) * sizeof(WCHAR);
		PStrCvt((WCHAR*)tmp, pFINode->m_FileName.str, pFINode->m_FileName.Len()+1);
		tmp += (pFINode->m_FileName.Len() + 1) * sizeof(WCHAR);
		*((UINT32*)tmp) = htonl(pFINode->m_iPakType);
		tmp += sizeof(pFINode->m_iPakType);
        //TODO ray handle error here
		ASSERT(MemoryCopy(tmp, SHA1HashSize, pFINode->m_Hash.m_Hash, SHA1HashSize));
		tmp += SHA1HashSize;
	}
	*size = buf_size;
	return buf;
}
