#ifndef _FILE_INFO_H_
#define _FILE_INFO_H_

#include "patchstd.h"

struct VERSIONINFO
{
	UINT32 v_major;
	UINT32 v_minor;

	VERSIONINFO(unsigned int v1, unsigned int v2) : v_major(v1), v_minor(v2) {}
	VERSIONINFO() : v_major(0), v_minor(0) {}

	VERSIONINFO& operator= (const VERSIONINFO& v)
	{
		v_major = v.v_major;
		v_minor = v.v_minor;
		return *this;
	}

	BOOL operator== (const VERSIONINFO& v)
	{
		return (v_major == v.v_major && v_minor == v.v_minor);
	}
};

struct FILEINFO
{
	BOOL bFileInPak;
	LPWSTR filename;
	UINT8 sha_hash[SHA1HashSize];
	DWORD file_size_low;
	DWORD file_size_high;
	DWORD file_size_compressed;
	DWORD gentime_low;
	DWORD gentime_high;
	PAKFILE* pakfile;
	DWORD offset_high;
	DWORD offset_low;
	LPCTSTR fname;
	LPCTSTR fpath;
	UINT8* buffer;
	HANDLE hFileMap;
};

typedef FILEINFO* PFILEINFO;
#endif
