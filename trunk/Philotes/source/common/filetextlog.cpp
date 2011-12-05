//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include "filetextlog.h"

//-----------------------------------------------------------------------------

#ifdef OS_PATH_CHAR_IS_UNICODE
#define local_fopen(a, b, c) _wfopen_s(a, b, c)
#else
#define local_fopen(a, b, c) fopen_s(a, b, c)
#endif

//-----------------------------------------------------------------------------

template<>
FileTextLog<char>::FileTextLog(void)
{
	pFile = 0;
}

template<>
FileTextLog<char>::~FileTextLog(void)
{
	Close();
}

template<>
void FileTextLog<char>::Close(void)
{
	if (pFile != 0)
	{
		fclose(pFile);
		pFile = 0;
	}
}

template<>
bool FileTextLog<char>::Open(OS_PATH_CHAR * pFileName)
{
	Close();
	// Use binary, it's simpler that way.
	if (local_fopen(&pFile, pFileName, OS_PATH_TEXT("wt")) == 0)
	{
		return true;
	}
	return false;
}

template<>
void FileTextLog<char>::_Write(const char_type * pFmt, va_list Args)
{
	vfprintf(pFile, pFmt, Args);
}

//-----------------------------------------------------------------------------

template<>
FileTextLog<wchar_t>::FileTextLog(void)
{
	pFile = 0;
}

template<>
FileTextLog<wchar_t>::~FileTextLog(void)
{
	Close();
}

template<>
void FileTextLog<wchar_t>::Close(void)
{
	if (pFile != 0)
	{
		fclose(pFile);
		pFile = 0;
	}
}

template<>
bool FileTextLog<wchar_t>::Open(OS_PATH_CHAR * pFileName)
{
	Close();
	// Use binary, it's simpler that way.
	if (local_fopen(&pFile, pFileName, OS_PATH_TEXT("wb")) == 0)
	{
		// Output byte order mark.
		char_type nBOM = L'\xFEFF';
		fwrite(&nBOM, sizeof(nBOM), 1, pFile);
		return true;
	}
	return false;
}

template<>
void FileTextLog<wchar_t>::_Write(const char_type * pFmt, va_list Args)
{
	vfwprintf(pFile, pFmt, Args);
}
