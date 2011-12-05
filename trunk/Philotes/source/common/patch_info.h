#ifndef _PATCH_INFO_H_
#define _PATCH_INFO_H_

LPCWSTR PATCHER_FILENAME = L"HGPatcher.exe";

LPCWSTR EXE_PATCH_FILELIST[] = {
	L"Hellgate.exe"
};

const UINT32 EXE_PATCH_FILECOUNT = (sizeof(EXE_PATCH_FILELIST)/sizeof(LPCWSTR));

#endif
