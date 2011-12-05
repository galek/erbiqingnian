#ifndef _FILE_MANAGE_H_
#define _FILE_MANAGE_H_

#include "patchstd.h"
#include "list.h"
#include "simple_hash.h"
#include "file_info.h"

BOOL DirectoryExists(LPCWSTR dir);
BOOL getAllFiles(LPWSTR fname, UINT32 len, PList<PWIN32_FIND_DATAW>& fileList, MEMORYPOOL* pPool);

VOID splitFilename(LPCWSTR filename, LPWSTR fpath, LPWSTR fname);

UINT32 fillFileTable(CSimpleHash<LPWSTR, PFILEINFO>& fileTable, PList<PWIN32_FIND_DATAW>& fileList, MEMORYPOOL* pPool);
BOOL writeFileTable(CSimpleHash<LPWSTR, PFILEINFO>& fileTable, LPCWSTR idx_filename);
BOOL readFileTable(CSimpleHash<LPWSTR, PFILEINFO>& fileTable, LPCWSTR idx_fname, MEMORYPOOL* pPool);
UINT32 getFileIndexCount(LPCWSTR idx_fname);
BOOL fillFileTableFromPakIndex(CSimpleHash<LPWSTR, PFILEINFO>& fileTable, MEMORYPOOL* pPool, BOOL checkSHA = FALSE);
BOOL getFileSHA(LPCWSTR filename, UINT8* sha_hash);

UINT8* PakIndexFillString(
						  MEMORYPOOL* pPool,
						  UINT32* size);

FILEINFO* FillFileInfoFromPakIndex(
	CSimpleHash<LPWSTR, PFILEINFO>& fileTable,
	FILE_INDEX_NODE* pFINode,
	MEMORYPOOL* pPool);

#endif
