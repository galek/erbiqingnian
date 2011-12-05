//----------------------------------------------------------------------------
// Prime v2.0
//
// pstring.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_PSTRING_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _PSTRING_H_

#include "pstring_compat.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PSTR_MAX					(USHRT_MAX * 4)


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
const WCHAR * PStrTok(
	__out_z WCHAR * token,
	__in_z const WCHAR * buf,
	__in_z const WCHAR * delim,
	int token_len,
	int	& buf_len,
	int * len = NULL,
	WCHAR * delimeter = NULL);

const char * PStrTok(
	char * token,
	const char * buf,
	const char * delim,
	int token_len,
	int	& buf_len,
	int * len = NULL,
	char * delimeter = NULL);

void PStrGroupThousands(
	WCHAR * pszNumber,
	int nBufLen);

void PStrGroupThousands(
	char * pszNumber,
	int nBufLen);

const char * PStrSkipWhitespace(
	const char * txt,
	int len = PSTR_MAX);

const WCHAR * PStrSkipWhitespace(
	const WCHAR * txt,
	int len = PSTR_MAX);

WCHAR * PStrCopy(
	WCHAR * dst,
	const WCHAR * src,
	int destlen);

char * PStrCopy(
	char * dst,
	const char * src,
	int destlen);

unsigned int PStrCopy(
	WCHAR * dest,
	int destlen,
	const WCHAR * src,
	int srclen);

unsigned int PStrCopy(
	char * dest,
	int destlen,
	const char * src,
	int srclen);

unsigned int PStrLen(
	const WCHAR * str);

unsigned int PStrLen(
	const WCHAR * str,
	unsigned int len);

unsigned int PStrLen(
	const char * str);

unsigned int PStrLen(
	const char * str,
	unsigned int len);

char * PStrCat(
	char * str,
	const char * cat,
	int strlen);

WCHAR * PStrCat(
	WCHAR * str,
	const WCHAR * cat,
	int strlen);

char * PStrNCat(
	char * str,
	int strlen,
	const char * cat,
	int characters);

WCHAR * PStrNCat(
	WCHAR * str,
	int strlen,
	const WCHAR * cat,
	int characters);

WCHAR * PStrCvt(
	WCHAR * dest,
	const char * src,
	int dest_len);

WCHAR * PStrCvt(
	WCHAR * dest,
	const WCHAR * src,
	int dest_len);

char * PStrCvt(
	char * dest,
	const WCHAR * src,
	int dest_len);

char * PStrCvt(
	char * dest,
	const char * src,
	int dest_len);

size_t PStrCvtLen(
	WCHAR * dest,
	const char * src,
	int dest_len);

size_t PStrCvtLen(
	char * dest,
	const WCHAR * src,
	int dest_len);

int PStrCSpn(
	const WCHAR * buf,
	const WCHAR * delim,
	int buf_len);

int PStrCSpn(
	const WCHAR * buf,
	const WCHAR * delim,
	int buf_len,
	int delim_len);

int PStrCSpn(
	const char * buf,
	const char * delim,
	int buf_len);

int PStrCSpn(
	const char * buf,
	const char * delim,
	int buf_len,
	int delim_len);

WCHAR * PStrUpper(
	WCHAR * buf,
	int len);

char * PStrUpper(
	char * buf,
	int len);

WCHAR * PStrLower(
	WCHAR * buf,
	int len);

char * PStrLower(
	char * buf,
	int len);

int PStrICmp(
	const char * a,
	const char * b,
	int len = PSTR_MAX);

WCHAR * PStrUpperL(
	WCHAR * buf,
	int len);

char * PStrUpperL(
	char * buf,
	int len);

WCHAR * PStrLowerL(
	WCHAR * buf,
	int len);

char * PStrLowerL(
	char * buf,
	int len);

int PStrICmpL(
	const char * a,
	const char * b,
	int len = PSTR_MAX);

int PStrICmp(
	const WCHAR * a,
	const WCHAR * b,
	int len = PSTR_MAX);

BOOL PStrICmp(
	const char a,
	const char b);

BOOL PStrICmp(
	const WCHAR a,
	const WCHAR b);

int PStrCmp(
	const char * a,
	const char * b,
	int len = PSTR_MAX);

int PStrCmp(
	const WCHAR * a,
	const WCHAR * b,
	int len = PSTR_MAX);

float PStrToFloat(
	const WCHAR * str);

float PStrToFloat(
	const char * str);

int PStrToInt(
	const WCHAR * str);

int PStrToInt(
	const char * str);

// returns false if string is missing or not a number.
BOOL PStrToInt(
	const WCHAR * str,
	int & val);

// returns false if string is missing or not a number.
BOOL PStrToInt(
	const char * str,
	int & val,
	BOOL bRequireNullTerminated = TRUE);

// returns false if string is missing or not a number.
BOOL PStrToInt(
	const WCHAR * str,
	unsigned __int64 & val);

// returns false if string is missing or not a number.
BOOL PStrToInt(
	const char * str,
	unsigned __int64 & val);

WCHAR * PIntToStr(
	WCHAR * str,
	int len,
	int value);

char * PIntToStr(
	char * str,
	int len,
	int value);

int PStrPrintf(
	char * str,
	int buf_len,
	const char * format,
	...);

int PStrPrintf(
	WCHAR * str,
	int buf_len,
	const WCHAR * format,
	...);

int PStrVprintf(
	char * str,
	int buf_len,
	const char * format,
	va_list& args);

int PStrVprintf(
	WCHAR * str,
	int buf_len,
	const WCHAR * format,
	va_list & args);

const char * PStrStr(
	const char * str,
	const char * substr);

const WCHAR * PStrStr(
	const WCHAR * str,
	const WCHAR * substr);

char * PStrStrI(
	const char * str,
	const char * substr);

WCHAR * PStrStrI(
	const WCHAR * str,
	const WCHAR * substr);

WCHAR * PStrStrI1337(
	WCHAR *a,
	int nSizeA, 
	const WCHAR *b, 
	int nSizeB);

WCHAR * PStrStrI1337(
	WCHAR *a, 
	const WCHAR *b,
	int nSize);

int PStrCmpI1337(
	const WCHAR *a,
	const WCHAR *b,
	int nLength);

void PStrReplaceTokenI1337( 
	WCHAR *puszString,
	int nStringBufferSize,
	const WCHAR *puszToken,
	const WCHAR *puszReplacement,
	BOOL bAllowPrefix = TRUE,
	BOOL bAllowSuffix = TRUE);

void PStrRemoveControlCharacters( 
	WCHAR *puszString,
	int nStringBufferSize,
	int nMaxReplacements);

char * PStrPbrk(
	char * str,
	const char * charset);

const char * PStrPbrk(
	const char * str,
	const char * charset);

WCHAR * PStrPbrk(
	WCHAR * str,
	const WCHAR * charset);

const WCHAR * PStrPbrk(
	const WCHAR * str,
	const WCHAR * charset);

const char * PStrChr(
	const char * str,
	int c);

char * PStrChr(
	char * str,
	int c);

WCHAR * PStrChr(
	WCHAR * str,
	WCHAR c);

const WCHAR * PStrChr(
	const WCHAR * str,
	WCHAR c);

const char * PStrRChr(
	const char * str,
	int c);

const WCHAR * PStrRChr(
	const WCHAR * str,
	WCHAR c);

void PStrForceBackslash(
	char * str,
	int buflen);

void PStrForceBackslash(
	WCHAR * str,
	int buflen);

void PStrReplaceExtension(
	char * newstring,
	int buflen,
	const char * oldstring,
	const char * extension);

void PStrReplaceExtension(
	WCHAR * newstring,
	int buflen,
	const WCHAR * oldstring,
	const WCHAR * extension);

const char *PStrGetExtension(
	const char * str);

const WCHAR *PStrGetExtension(
	const WCHAR * str);

void PStrRemoveExtension(
	char * newstring,
	int buflen,
	const char * oldstring);

void PStrRemoveExtension(
	WCHAR * newstring,
	int buflen,
	const WCHAR * oldstring);

int PStrGetPath(
	char * newstring,
	int buflen,
	const char * oldstring);

int PStrGetPath(
	WCHAR * newstring,
	int buflen,
	const WCHAR * oldstring);

void PStrRemoveEntirePath(
	char * newstring,
	int buflen,
	const char * oldstring);

void PStrRemoveEntirePath(
	WCHAR * newstring,
	int buflen,
	const WCHAR * oldstring);

void PStrRemovePath(
	char * newstring,
	int buflen,
	const char * path,
	const char * oldstring);
	
void PStrRemovePath(
	char * newstring,
	int buflen,
	const WCHAR * path,
	const char * oldstring);

void PStrRemovePath(
	WCHAR * newstring,
	int buflen,
	const WCHAR * path,
	const WCHAR * oldstring);

BOOL PStrIsNumeric(
	const char * txt);

BOOL PStrIsNumeric(
	const WCHAR * txt);

void PStrReplaceChars(
	char * szString,
	char c1,
	char c2);

void PStrReplaceChars(
	WCHAR * szString,
	WCHAR c1,
	WCHAR c2);

void PStrReplaceToken( 
	WCHAR *puszString,
	int nStringBufferSize,
	const WCHAR *puszTokens,		// you can replace multiple tokens by separating them by commas
	const WCHAR *puszReplacement);

void PStrPrintfTokenStringValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const WCHAR **puszValues,
	int nNumValues,
	BOOL bAddDigitSeparators = FALSE);

void PStrPrintfTokenIntValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const int *pnValues,
	int nNumValues);

void PStrPrintfTokenNumberValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const float *pnValues,
	const BOOL *pbFloat,
	int nNumValues);

BOOL PStrIsPrintable(
	WCHAR c);

//----------------------------------------------------------------------------
// replaces '\n' with designated character and '\r' with space
// returns pointing to end of text (0) character
//----------------------------------------------------------------------------
char * PStrTextFileFixup(
	char * text,
	DWORD size,
	char lf);

void PStrCvtToCRLF(
	char * text,
	DWORD size);

BOOL PStrSeparateServer(
	const WCHAR *puszServer,
	WCHAR *puszServerBase,	
	int nMaxServerBase,
	WCHAR *puszServerInitPath,
	int nMaxServerInitPath);

inline BOOL PStrIsPrintable(
	unsigned char c)
{
	return isprint(c);
}

inline BOOL PStrIsPrintable(
	WCHAR * str,
	DWORD strLen )
{
	if (!str)
		return FALSE;
	for (DWORD ii = 0; ii < strLen && str[ii] != L'\0'; ++ii)
	{
		if (!PStrIsPrintable(str[ii]))
			return FALSE;
	}
	return TRUE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
void PStrTrimTrailingSpaces( 
	T *puszString,
	int nNumChars)
{
	UNREFERENCED_PARAMETER(nNumChars);

	int nLength = PStrLen( puszString );
	while (puszString[ nLength - 1 ] == (T)(' '))
	{
		puszString[ nLength - 1 ] = (T)0;
		nLength--;
	}
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
void PStrTrimLeadingSpaces( 
	T *puszString,
	int nMaxChars)
{
	ASSERTX_RETURN( puszString, "Expected string" );
	ASSERTX_RETURN( nMaxChars > 0, "Invalid string buffer size" );	

	int nLength = PStrLen( puszString );
	int nNumLeadingSpaces = 0;
	int nFirstChar = -1;
	for (int i = 0; i < nLength; ++i)
	{
		if (puszString[ i ] == (T)(' '))
		{
			nNumLeadingSpaces++;
		}
		else
		{
			nFirstChar = i;
			break;
		}
	}

	if (nNumLeadingSpaces > 0)
	{
		// if we never found a first non space character, just clear string
		if (nFirstChar == -1)
		{
			puszString[ 0 ] = 0;
			return;
		}

		// move each character up		
		int nDestIndex = 0;
		int nSourceIndex = 0;
		for (nDestIndex = 0, nSourceIndex = nFirstChar; 
			 nSourceIndex < nLength; 
			 ++nSourceIndex, ++nDestIndex)
		{
			puszString[ nDestIndex ] = puszString[ nSourceIndex ];
		}
		
		// set new terminator
		ASSERTX_RETURN( nDestIndex < nMaxChars, "Error in trim leading spaces" );
		puszString[ nDestIndex ] = (T)0;			
	}
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
void PStrFixPathBackslash(
	T * str)
{
	ASSERT_RETURN(str);
			
	// remove any redundant backslashes (if this is a performance issue we should make it faster)
	// and turn any forward slashes into backslashes
	T * dest = str;
	T * src = str;

	for (T c = *src; c != (T)0; c = *(++src))
	{
		if (c == (T)('/'))	// turn forward slashes to backslashes
		{
			c = *src = (T)('\\');			
		}
		if (c == (T)('\\') && (src[1] == (T)('/') || src[1] == (T)('\\')))
		{
			dest = src++;			
			goto _copymode;
		}
	}
	dest = src;
	goto _endmode;

_copymode:
	for (T c = *src; c != (T)0; c = *(++src))
	{
		if (c == (T)('/'))	// turn forward slashes to backslashes
		{
			c = *src = (T)('\\');			
		}
		if (c == (T)('\\') && (src[1] == (T)('/') || src[1] == (T)('\\')))
		{
			continue;
		}
		*dest++ = *src;
	}

_endmode:
	--dest;
	if (*dest == (T)'\\')
	{
		*dest = 0;
	}
	else
	{
		dest[1] = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
inline void PStrCatChar(
	T * szStr,
	T chr,
	int nStrLen)
{
	ASSERT_RETURN(szStr);
	ASSERT_RETURN(nStrLen > 1);
		
	int nCount = 0;
	while (*szStr != (T)0)
	{
		if (++nCount >= nStrLen-1)
			return;

		szStr++;
	}

	*szStr++ = chr;
	*szStr = (T)0;
}


//----------------------------------------------------------------------------
// advances str to end of integer
//----------------------------------------------------------------------------
template <typename C, typename V>
BOOL PStrToIntEx(
	const C * str,
	V & val,
	const C ** end = NULL)
{
	val = 0;

	const C * cur = str;

	if (!cur || *cur == 0)
	{
		goto _error;
	}
	cur = PStrSkipWhitespace(cur);
	if (*cur == 0)
	{
		goto _error;
	}

	int neg = 1;
	if (*cur == (C)('-'))
	{
		neg = -1;
		++cur;
	}

	const C * beg = cur;
	while (*cur >= (C)('0') && *cur <= (C)('9'))
	{
		val = (val * 10) + (*cur - (C)('0'));
		++cur;
	}

	if (cur <= beg)
	{
		goto _error;
	}

	val *= neg;
	if (end)
	{
		*end = cur;
	}
	return TRUE;
	
_error:
	if (end)
	{
		*end = str;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// advances str to end of integer
//----------------------------------------------------------------------------
template <typename C, typename V>
BOOL PStrToUIntEx(
	const C * str,
	V & val,
	const C ** end = NULL)
{
	val = 0;

	const C * cur = str;

	if (!cur || *cur == 0)
	{
		goto _error;
	}
	cur = PStrSkipWhitespace(cur);
	if (*cur == 0)
	{
		goto _error;
	}

	const C * beg = cur;
	while (*cur >= (C)('0') && *cur <= (C)('9'))
	{
		val = (val * 10) + (*cur - (C)('0'));
		++cur;
	}

	if (cur <= beg)
	{
		goto _error;
	}

	if (end)
	{
		*end = cur;
	}
	return TRUE;
	
_error:
	if (end)
	{
		*end = str;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// advances str to end of integer
// could use strtol, but unsure about what it does under error conditions
//----------------------------------------------------------------------------
template <typename C, typename F>
BOOL PStrHexToInt(
	const C * str,
	int len,
	F & val,
	const C ** end = NULL)
{
	val = 0;

	const C * cur = str;

	if (!cur || !cur[0])
	{
		goto _error;
	}

	cur = PStrSkipWhitespace(cur);
	if (*cur == 0)
	{
		goto _error;
	}

	const C * beg = cur;
	const C * _end = str + len;
	while (cur < _end )
	{
		switch (*cur)
		{
		case (C)('0'):	val = val * 16;			break;
		case (C)('1'):	val = val * 16 + 1;		break;
		case (C)('2'):	val = val * 16 + 2;		break;
		case (C)('3'):	val = val * 16 + 3;		break;
		case (C)('4'):	val = val * 16 + 4;		break;
		case (C)('5'):	val = val * 16 + 5;		break;
		case (C)('6'):	val = val * 16 + 6;		break;
		case (C)('7'):	val = val * 16 + 7;		break;
		case (C)('8'):	val = val * 16 + 8;		break;
		case (C)('9'):	val = val * 16 + 9;		break;
		case (C)('a'):
		case (C)('A'):	val = val * 16 + 10;	break;
		case (C)('b'):
		case (C)('B'):	val = val * 16 + 11;	break;
		case (C)('c'):
		case (C)('C'):	val = val * 16 + 12;	break;
		case (C)('d'):
		case (C)('D'):	val = val * 16 + 13;	break;
		case (C)('e'):
		case (C)('E'):	val = val * 16 + 14;	break;
		case (C)('f'):
		case (C)('F'):	val = val * 16 + 15;	break;
		default:
			goto _done;
		}
		++cur;
	}

_done:
	if (cur <= beg)
	{
		goto _error;
	}
	if (end)
	{
		*end = cur;
	}
	return TRUE;

_error:
	if (end)
	{
		*end = str;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// advances str to end of float
//----------------------------------------------------------------------------
template <typename C, typename F>
BOOL PStrToFloat(
	const C * str,
	F & val,
	const C ** end = NULL)
{
	val = 0.0f;

	const C * cur = str;

	if (!cur || *cur == 0)
	{
		goto _error;
	}
	cur = PStrSkipWhitespace(cur);
	if (*cur == 0)
	{
		goto _error;
	}

	INT64 n = 0;
	INT64 neg = 1;
	if (*cur == (C)('-'))
	{
		neg = -1;
		++cur;
	}

	const C * beg = cur;
	while (*cur >= (C)('0') && *cur <= (C)('9'))
	{
		n = n * 10 + (*cur - (C)('0'));
		++cur;
	}
	if (*cur != (C)('.'))
	{
		if (cur <= beg)
		{
			goto _error;
		}
		val = (F)(n * neg);
		goto _success;
	}

	++cur;
	int div = 1;
	beg = cur;
	while (*cur >= (C)('0') && *cur <= (C)('9'))
	{
		n = n * 10 + (*cur - (C)('0'));
		div *= 10;
		++cur;
	}
	if (cur <= beg)
	{
		goto _error;
	}

	long double ld = (long double)(n * neg);
	if (div > 1)
	{
		ld /= (long double)div;
	}

	if (*cur != (C)('e') && *cur != (C)('E'))
	{
		val = (F)ld;
		goto _success;
	}

	int exp_val = 0;
	int sign = 1;
    if (*cur == (C)('+')) 
	{
        ++cur;
    } 
	else if (*cur == (C)('-')) 
	{
        sign = -1;
		++cur;
    }
	beg = cur;
    while (*cur >= (C)('0') && *cur <= (C)('9')) 
	{
        exp_val = (exp_val * 10) + (*cur - (C)('0'));
		++cur;
    }
	if (cur <= beg)
	{
		goto _error;
	}
	exp_val = exp_val * sign;
    ld = ldexp(ld, exp_val);
	val = (F)ld;

_success:
	if (end)
	{
		*end = cur;
	}
	return TRUE;

_error:
	if (end)
	{
		*end = str;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
BOOL PStrWildcardMatch(
	T * strString,
	T * strPattern)
{
	// make initial matches prior to getting a *
	for (;;++strString, ++strPattern)
	{
		if (*strPattern == 0)
		{
			return (*strString == 0);
		}
		else if (*strPattern == (T)'.')
		{
			if (*strString == 0)
			{
				return FALSE;
			}
		}
		else if (*strPattern == (T)'*')
		{
			break;
		}
		else if (*strString != *strPattern)
		{
			return FALSE;
		}
	}

match_star:
	// matching for a *
	for (;;)
	{
		++strPattern;
		if (*strPattern == 0)
		{
			return TRUE;
		}
		else if (*strPattern != (T)'*')
		{
			break;
		}
	}

	T * strMarker = strPattern;
	// match post-star
	for (;;)
	{
		if (*strString == 0)
		{
			while (*strPattern == (T)'*')
			{
				++strPattern;
			}
			return (*strPattern == 0);
		}
		else if (*strPattern == (T)'.')
		{
			if (*strString == 0)
			{
				return FALSE;
			}
			++strString; ++strPattern; 
		}
		else if (*strPattern == (T)'*')
		{
			goto match_star;
		}
		else if (*strPattern == *strString)
		{
			 ++strString; ++strPattern;
		}
		else if (strPattern == strMarker)
		{
			++strString;
		}
		else
		{
			strPattern = strMarker;
		}
	}
}


//----------------------------------------------------------------------------
// local (stack) storage fixed-length string class
//----------------------------------------------------------------------------
struct PStrI
{
	char *			str;
	unsigned int	len;
	unsigned int	size;

	PStrI(
		char * buf, 
		unsigned int buflen) 
		: str(buf), len(0), size(buflen)
	{
		str[0] = 0;
	}

	PStrI(
		char * buf,
		unsigned int buflen,
		const char * val) 
		: str(buf), size(buflen)
	{ 
		len = PStrLen(val);
		len = MIN((unsigned int)size - 1, len);
		memcpy(str, val, len);
		str[len] = 0;
	}

	PStrI(
		const PStrI & val)
	{
		len = val.len;
		size = val.size;
		memcpy(str, val.str, len);
		str[size - 1] = 0;
	}

	operator char *()
	{
		return str;
	}

	operator const char *()
	{
		return str;
	}

	const char & operator [](
		int i)
	{
		return str[i];
	}

	void clear(
		unsigned int i = 0)
	{
		len = MIN(size - 1, i);
		str[len] = 0;
	}

	PStrI & operator <<(
		DWORD i)
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _snprintf_s(str + len, size - len, size - len - 1, "%u", i);
#else
		int out = _snprintf(str + len, size - len - 1, "%u", i);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size - 1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}
		return *this;
	}

	PStrI & operator <<(
		unsigned long long i )
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _snprintf_s(str + len, size - len, size - len - 1, "%llu", i);
#else
		int out = _snprintf(str + len, size - len - 1, "%llu", i);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size - 1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}
		return *this;
	}

	PStrI & operator <<(
		int i)
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _snprintf_s(str + len, size - len, size - len - 1, "%d", i);
#else
		int out = _snprintf(str + len, size - len - 1, "%d", i);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size - 1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}
		return *this;
	}

	PStrI & operator <<(
		float f)
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _snprintf_s(str + len, size - len, size - len - 1, "%4.4f", f);
#else
		int out = _snprintf(str + len, size - len - 1, "%4.4f", f);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size - 1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}
		return *this;
	}

	PStrI & operator <<(
		const char * s)
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _snprintf_s(str + len, size - len, size - len - 1, "%s", s);
#else
		int out = _snprintf(str + len, size - len - 1, "%s", s);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size-1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}
		return *this;
	}

	PStrI & operator <<(
		const WCHAR * s)
	{
		int out = (int)PStrCvtLen(str + len, s, size - len - 1);
		if (out < 0)
		{
			str[len] = 0;	// invalid character, don't convert
		}
		else
		{
			len += out;
			str[len] = 0;	// if the return value is count, the wide-character string is not null-terminated.
		}
		return *this;
	}

	PStrI & printf(
		const char * fmt,
		...)
	{
		va_list args;
		va_start(args, fmt);

#ifdef USE_VS2005_STRING_FUNCTIONS
		int out = _vsnprintf_s(str + len, size - len, size - len - 1, fmt, args);
#else
		int out = _vsnprintf(str + len, size - len - 1, fmt, args);
#endif // USE_VS2005_STRING_FUNCTIONS
		if (out < 0)
		{
			str[size-1] = 0;
			len = size;
		}
		else
		{
			len += out;
		}

		return *this;
	}
};


//----------------------------------------------------------------------------
// TCHAR string class
//----------------------------------------------------------------------------
template <typename T>
struct PStr
{
	T *				str;
	unsigned int	len;

	PStr(
		void) : str(0), len(0)
	{
	}

	PStr(
		T * buf,
		unsigned int buflen)
	{
		str = buf;
		len = buflen;
	}

	BOOL IsEmpty(
		void) const
	{
		return (str == NULL || len == 0);
	}

	void Init(
		void)
	{
		str = NULL;
		len = 0;
	}

	void Set(
		T * buf,
		unsigned int buflen)
	{
		str = buf;
		len = buflen;
	}

	unsigned int Len(
		void) const
	{
		return len;
	}

	void CopyTo(
		T * dest) const
	{
		memcpy(dest, str, (len + 1) * sizeof(T));
	}

	BOOL Copy(
		const PStr & src,
		struct MEMORYPOOL * pool = NULL)
	{
		ASSERT_RETFALSE(str == NULL && len == 0);
		if (src.IsEmpty())
		{
			return TRUE;
		}
		str = (T *)MALLOC(pool, (src.Len() + 1) * sizeof(T));
		ASSERT_RETFALSE(str);
		memcpy(str, src.str, (src.Len() + 1) * sizeof(T));
		len = src.Len();
		return TRUE;
	}

	BOOL Copy(
		const T * src,
		unsigned int maxlen,
		struct MEMORYPOOL * pool = NULL)
	{
		ASSERT_RETFALSE(str == NULL && len == 0);
		if (src == NULL || src[0] == 0)
		{
			return TRUE;
		}
		unsigned int templen = PStrLen(src, maxlen);
		str = (T *)MALLOC(pool, (templen + 1) * sizeof(T));
		ASSERT_RETFALSE(str);
		memcpy(str, src, (templen) * sizeof(T));
		str[templen] = 0;
		len = templen;
		return TRUE;
	}

	void Free(
		struct MEMORYPOOL * pool = NULL)
	{
		if (str)
		{
			FREE(pool, str);
			str = NULL;
			len = 0;
		}
	}

	BOOL operator==(
		const T * str_b)
	{
		return (PStrCmp(str, str_b) == 0);
	}

	BOOL operator==(
		const PStr & str_b) const
	{
		if (Len() != str_b.Len())
		{
			return FALSE;
		}
		return (PStrCmp(str, str_b.str) == 0);
	}

	BOOL operator!=(
		const PStr & str_b) const
	{
		if (Len() != str_b.Len())
		{
			return TRUE;
		}
		return (PStrCmp(str, str_b.str) != 0);
	}

	const T * GetStr(
		void) const
	{
		static const T s_NullStr[1] = { 0 };
		if (!str)
		{
			return s_NullStr;
		}
		return str;
	}
};


#define PStrW				PStr<WCHAR>
#define	PStrA				PStr<char>

#ifdef _UNICODE
#define PStrT				PStrW
#else
#define PStrT				PStrA
#endif

#define PStrTDef(n, s)		TCHAR n##_buf[s]; PStrT n(n##_buf, 0);


#endif // _PSTRING_H_
