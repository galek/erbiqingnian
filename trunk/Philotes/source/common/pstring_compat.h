//----------------------------------------------------------------------------
// Prime v2.0
//
// pstring.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_PSTRING_COMPAT_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _PSTRING_COMPAT_H_

#if _MSC_VER >= 1400

#define USE_VS2005_STRING_FUNCTIONS 1

#else // _MSC_VER >= 1400

#pragma message ("    HINT: Using Visual Studio 2003 string compatibility functions")
#define USE_VS2003_STRING_FUNCTIONS 1
#define _tcscpy_s(a,b,c) _tcscpy(a,c)
#define _tccpy_s(a,b,c,d) _tccpy(a,d)
#define sscanf_s sscanf

inline char* strtok_s(char* token, const char * delimiter, char ** context)
{
	return strtok(token, delimiter);
}

inline WCHAR * wcstok_s(WCHAR * token, const WCHAR * delimiter, WCHAR ** context)
{
	return wcstok(token, delimiter);
}

inline void localtime_s(struct tm* _tm, const time_t *t)
{
	struct tm * lt = localtime(t);
	*_tm = *lt;
}

inline void fopen_s(FILE ** f, const char * filename, const char * mode)
{
	*f = fopen(filename, mode);
}

#endif // _MSC_VER >= 1400

#endif // _PSTRING_COMPAT_H_
