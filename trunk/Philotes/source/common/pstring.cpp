//----------------------------------------------------------------------------
// Prime v2.0
//
// pstring.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "fontcolor.h"
#include "stringtable.h"
#include "colors.h"
#include "language.h"

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrTok(
	__out_z WCHAR * token,
	__in_z const WCHAR * buf,
	__in_z const WCHAR * delim,
	int token_len,
	int	& buf_len,
	int * length,
	WCHAR * delimeter)
{
	ASSERT_RETNULL(token);
	ASSERT_RETNULL(delim);
	*token = 0;
	if (length)
	{
		*length = 0;
	}
	if (!buf)
	{
		return NULL;
	}

	const WCHAR * start = buf;
	const WCHAR * end = buf + buf_len;

	buf = PStrSkipWhitespace(buf, buf_len);
	if (buf >= end)
	{
		return NULL;
	}
	if (*buf && *buf == L'`')
	{
		buf++;
	}
	if (buf >= end || *buf == 0)
	{
		return NULL;
	}

	unsigned int delim_len = PStrLen(delim) + 1;		// include 0 term

	BOOL quoted_string = FALSE;

	// copy token
	WCHAR * tok = token;
	token_len = MIN(token_len, buf_len - (int)(buf - start));

	const WCHAR * tok_end = tok + token_len - 1;
	while (tok < tok_end)
	{
		if (quoted_string)
		{
			if (*buf == L'\"')
			{
				quoted_string = FALSE;
				buf++;
				if (*buf == NULL)
				{
					if (delimeter)
					{
						*delimeter = *buf;
					}
					goto gottok;
				}
				continue;
			}
		}
		else
		{
			if (*buf == L'\"')
			{
				quoted_string = TRUE;
				buf++;
				continue;
			}
			for (unsigned int ii = 0; ii < delim_len; ii++)
			{
				if (*buf == delim[ii])
				{
					if (delimeter)
					{
						*delimeter = *buf;
					}
					// don't inc buffer when found NULL delimiter
					if (delim[ii] != 0)
					{
						buf++;
					}
					goto gottok;
				}
			}
		}
		*tok = *buf;
		tok++;
		buf++;
	}
	if (tok == tok_end && *buf == 0)
	{
		goto gottok;
	}
	*token = 0;
	return NULL;

gottok:
	*tok = 0;
	if (tok != token)
	{
		if (length)
		{
			*length = (int)(tok - token);
		}
		buf_len = (int)(end - buf);
		return buf;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrTok(
	__out_z char * token,
	__in_z const char * buf,
	__in_z const char * delim,
	int token_len,
	int	& buf_len,
	int * length,
	char * delimeter)
{
	*token = 0;
	if (length)
	{
		*length = 0;
	}

	const char * start = buf;
	const char * end = buf + buf_len;

	buf = PStrSkipWhitespace(buf, buf_len);
	if (buf >= end)
	{
		return NULL;
	}
	if (*buf && *buf == '`')	// this let's us have leading spaces in input
	{
		buf++;
	}
	if (buf >= end || *buf == 0)
	{
		return NULL;
	}

	int delim_len = PStrLen(delim) + 1;		// include 0 term

	BOOL quoted_string = FALSE;

	// copy token
	char * tok = token;
	token_len = MIN(token_len, buf_len - (int)(buf - start));

	const char * tok_end = tok + token_len - 1;
	while (tok < tok_end)
	{
		if (quoted_string)
		{
			if (*buf == L'\"')
			{
				buf++;
				continue;
			}
		}
		else
		{
			if (*buf == L'\"')
			{
				quoted_string = TRUE;
				buf++;
				if (*buf == NULL)
				{
					if (delimeter)
					{
						*delimeter = *buf;
					}
					goto gottok;
				}
				continue;
			}
			for (int ii = 0; ii < delim_len; ii++)
			{
				if (*buf == delim[ii])
				{
					if (delimeter)
					{
						*delimeter = *buf;
					}
					// don't inc buffer when found NULL delimiter
					if ( delim[ii] != 0 )
						buf++;
					goto gottok;
				}
			}
		}
		*tok = *buf;
		tok++;
		buf++;
	}
	if (tok == tok_end && *buf == 0)
	{
		goto gottok;
	}
	*token = 0;
	return NULL;

gottok:
	*tok = 0;
	if (tok != token)
	{
		if (length)
		{
			*length = (int)(tok - token);
		}
		buf_len = (int)(end - buf);
		return buf;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrSkipWhitespace(
	const char * txt,
	int len)
{
	UNREFERENCED_PARAMETER(len);
	while (isspace((unsigned char)(*txt)))
	{
		txt++;
	}
	return txt;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrSkipWhitespace(
	const WCHAR * txt,
	int len)
{
	UNREFERENCED_PARAMETER(len);
	while (iswspace(*txt))
	{
		txt++;
	}
	return txt;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrCopy(
	WCHAR * dst,
	const WCHAR * src,
	int destlen)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	wcsncpy_s(dst, destlen, src, _TRUNCATE);
	// New secure functions automatically null-terminate
#else
	wcsncpy(dst, src, destlen);
	dst[destlen-1] = 0;
#endif
	return dst;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrCopy(
	char * dst,
	const char * src,
	int destlen)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	strncpy_s(dst, destlen, src, _TRUNCATE);
	// New secure functions automatically null-terminate
#else
	strncpy(dst, src, destlen);
	dst[destlen-1] = 0;
#endif
	return dst;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrCopy(
	WCHAR * dest,
	int destlen,
	const WCHAR * src,
	int srclen)
{
	ASSERT_RETZERO(destlen > 0);				// otherwise, no room to add 0 terminator!
	int maxlen = MIN(destlen, srclen) - 1;		// null term

	WCHAR * cur = dest;
	const WCHAR * end = src + maxlen;
	while (src < end && *src)
	{
		*cur++ = *src++;
	}
	*cur = 0;

	return (unsigned int)(cur - dest);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrCopy(
	char * dest,
	int destlen,
	const char * src,
	int srclen)
{
	ASSERT_RETZERO(destlen > 0);				// otherwise, no room to add 0 terminator!
	int maxlen = MIN(destlen, srclen) - 1;		// null term

	char * cur = dest;
	const char * end = src + maxlen + 1;
	while (src < end && *src)
	{
		*cur++ = *src++;
	}
	*cur = 0;

	return (unsigned int)(cur - dest);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrLen(
	const WCHAR * str)
{
	return (unsigned int)wcslen(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrLen(
	const WCHAR * str,
	unsigned int len)
{
	unsigned int length = 0;
	while (length < len && *str)
	{
		str++;
		length++;
	}
	return length;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrLen(
	const char * str)
{
	return (unsigned int)strlen(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PStrLen(
	const char * str,
	unsigned int len)
{
	unsigned int length = 0;
	while (length < len && *str)
	{
		str++;
		length++;
	}
	return length;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrCat(
	char * str,
	const char * cat,
	int strlen)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	strncat_s(str, strlen, cat, _TRUNCATE);
	return str;
#else
	return strncat(str, cat, MIN(PStrLen(cat), len - PStrLen(str)));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrCat(
	WCHAR * str,
	const WCHAR * cat,
	int strlen)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	wcsncat_s(str, strlen, cat, _TRUNCATE);
	return str;
#else
	return wcsncat(str, cat, MIN(PStrLen(cat), len - PStrLen(str)));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrNCat(
	char * str,
	int strlen,
	const char * cat,
	int characters)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	strncat_s(str, strlen, cat, characters);
	return str;
#else
	return strncat(str, cat, characters);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrNCat(
	WCHAR * str,
	int strlen,
	const WCHAR * cat,
	int characters)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	wcsncat_s(str, strlen, cat, characters);
	return str;
#else
	return wcsncat(str, cat, characters);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrCvt(
	WCHAR * dest,
	const char * src,
	int dest_len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;
	mbstowcs_s(&nCharsConverted, dest, dest_len, src, _TRUNCATE);
#else
	mbstowcs(dest, src, dest_len);
#endif
	return dest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrCvt(
	char * dest,
	const WCHAR * src,
	int dest_len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;

#if ISVERSION(DEVELOPMENT)
	ASSERTV(wcstombs_s(&nCharsConverted, dest, dest_len, src, _TRUNCATE) != EILSEQ, "Failed to convert WCHAR string to char* string; attempted to convert: %s", src); 
#else
	wcstombs_s(&nCharsConverted, dest, dest_len, src, _TRUNCATE); 
#endif

#else
	ASSERT(wcstombs(dest, src, dest_len)==0);
#endif


	return dest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrCvt(
	WCHAR * dest,
	const WCHAR * src,
	int dest_len)
{
	PStrCopy(dest, dest_len, src, dest_len);
	return dest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrCvt(
	char * dest,
	const char * src,
	int dest_len)
{
	PStrCopy(dest, dest_len, src, dest_len);
	return dest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
size_t PStrCvtLen(
	WCHAR * dest,
	const char * src,
	int dest_len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;
	mbstowcs_s(&nCharsConverted, dest, dest_len, src, _TRUNCATE);
	return nCharsConverted;
#else
	return mbstowcs(dest, src, dest_len);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
size_t PStrCvtLen(
	char * dest,
	const WCHAR * src,
	int dest_len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;
	wcstombs_s(&nCharsConverted, dest, dest_len, src, _TRUNCATE);
	return nCharsConverted;
#else
	return wcstombs(dest, src, dest_len);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCSpn(
	const WCHAR * buf,
	const WCHAR * delim,
	int buf_len)
{
	int delim_len = PStrLen(delim);

	const WCHAR * start = buf;
	const WCHAR * end = buf + buf_len;

	while (buf < end && *buf != 0)
	{
		for (int ii = 0; ii < delim_len; ii++)
		{
			if (*buf == delim[ii])
			{
				return (int)(buf - start);
			}
		}
		buf++;
	}
	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCSpn(
	const WCHAR * buf,
	const WCHAR * delim,
	int buf_len,
	int delim_len)
{
	const WCHAR * start = buf;
	const WCHAR * end = buf + buf_len;

	while (buf < end && *buf != 0)
	{
		for (int ii = 0; ii < delim_len; ii++)
		{
			if (*buf == delim[ii])
			{
				return (int)(buf - start);
			}
		}
		buf++;
	}
	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCSpn(
	const char * buf,
	const char * delim,
	int buf_len)
{
	int delim_len = PStrLen(delim);

	const char * start = buf;
	const char * end = buf + buf_len;

	while (buf < end && *buf != 0)
	{
		for (int ii = 0; ii < delim_len; ii++)
		{
			if (*buf == delim[ii])
			{
				return (int)(buf - start);
			}
		}
		buf++;
	}
	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCSpn(
	const char * buf,
	const char * delim,
	int buf_len,
	int delim_len)
{
	const char * start = buf;
	const char * end = buf + buf_len;

	while (buf < end && *buf != 0)
	{
		for (int ii = 0; ii < delim_len; ii++)
		{
			if (*buf == delim[ii])
			{
				return (int)(buf - start);
			}
		}
		buf++;
	}
	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrUpper(
	WCHAR * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_wcsupr_s(buf, len);
	return buf;
#else
	return wcsupr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrUpper(
	char * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_strupr_s(buf, len);
	return buf;
#else
	return strupr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrLower(
	WCHAR * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_wcslwr_s(buf, len);
	return buf;
#else
	return wcslwr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrLower(
	char * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_strlwr_s(buf, len);
	return buf;
#else
	return strlwr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrICmp(
	const char * a,
	const char * b,
	int len)
{
	return _strnicmp(a, b, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrUpperL(
	WCHAR * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_wcsupr_s_l(buf, len, LanguageGetLocale());
	return buf;
#else
	return wcsupr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrUpperL(
	char * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_strupr_s_l(buf, len, LanguageGetLocale());
	return buf;
#else
	return strupr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrLowerL(
	WCHAR * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_wcslwr_s_l(buf, len, LanguageGetLocale());
	return buf;
#else
	return wcslwr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrLowerL(
	char * buf,
	int len)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_strlwr_s_l(buf, len, LanguageGetLocale());
	return buf;
#else
	return strlwr(buf);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrICmpL(
	const char * a,
	const char * b,
	int len)
{
	return _strnicmp_l(a, b, len, LanguageGetLocale());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrICmp(
	const WCHAR * a,
	const WCHAR * b,
	int len)
{
	for (int ii = 0; ii < len; ++ii, ++a, ++b)
	{
		if (*a == 0)
		{
			if (*b == 0)
			{
				return 0;
			}
			return -1;
		}
		if (*b == 0)
		{
			return 1;
		}
		WCHAR la = towlower(*a);
		WCHAR lb = towlower(*b);
		if (la < lb)
		{
			return -1;
		}
		if (la > lb)
		{
			return 1;
		}
	}
	return 0;
//	REF(len);
//	return CompareStringW(gdw_LanguageLocale, SORT_STRINGSORT, a, -1, b, -1) - 2; // see CompareStringW documentation
//	return _wcsnicmp(a, b, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrICmp(
	const char a,
	const char b)
{
	return (tolower(a) == tolower(b));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrICmp(
	const WCHAR a,
	const WCHAR b)
{
	return (towlower(a) == towlower(b));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCmp(
	const char * a,
	const char * b,
	int len)
{
	return strncmp(a, b, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrCmp(
	const WCHAR * a,
	const WCHAR * b,
	int len)
{
	return wcsncmp(a, b, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float PStrToFloat(
	const WCHAR * str)
{
	if (!str)
	{
		return 0.0f;
	}
	return (float)_wtof(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float PStrToFloat(
	const char * str)
{
	return (float)atof(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrToInt(
	const WCHAR * str)
{
	if (!str)
	{
		return 0;
	}
	return _wtoi(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrToInt(
	const char * str)
{
	if (!str)
	{
		return 0;
	}
	return atoi(str);
}


//----------------------------------------------------------------------------
// returns false if string is missing or not a number.
//----------------------------------------------------------------------------
BOOL PStrToInt(
	const WCHAR * str,
	int & val)
{
	val = 0;

	if (!str || !str[0])
	{
		return FALSE;
	}

	str = PStrSkipWhitespace(str);

	if (*str == 0)
	{
		return FALSE;
	}

	int neg = 1;
	if (*str == L'-')
	{
		neg = -1;
		++str;
	}

	const WCHAR * beg = str;
	while (*str >= L'0' && *str <= L'9')
	{
		val = (val * 10) + (*str - L'0');
		++str;
	}
	if (str <= beg)
	{
		return FALSE;
	}
	val *= neg;

	str = PStrSkipWhitespace(str);
	if (*str != 0)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// returns false if string is missing or not a number.
//----------------------------------------------------------------------------
BOOL PStrToInt(
	const char * str,
	int & val,
	BOOL bRequireNullTerminated)
{
	val = 0;

	if (!str || !str[0])
	{
		return FALSE;
	}

	str = PStrSkipWhitespace(str);

	if (*str == 0)
	{
		return FALSE;
	}

	int neg = 1;
	if (*str == '-')
	{
		neg = -1;
		++str;
	}

	const char * beg = str;
	while (*str >= '0' && *str <= '9')
	{
		val = (val * 10) + (*str - '0');
		++str;
	}
	if (str <= beg)
	{
		return FALSE;
	}
	val *= neg;

	str = PStrSkipWhitespace(str);
	if (*str != 0 && bRequireNullTerminated)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// returns false if string is missing or not a number.
//----------------------------------------------------------------------------
BOOL PStrToInt(
			   const WCHAR * str,
			   unsigned __int64 & val)
{
	val = 0;

	if (!str || !str[0])
	{
		return FALSE;
	}

	str = PStrSkipWhitespace(str);

	if (*str == 0)
	{
		return FALSE;
	}

	int neg = 1;
	if (*str == L'-')
	{
		neg = -1;
		++str;
	}

	const WCHAR * beg = str;
	while (*str >= L'0' && *str <= L'9')
	{
		val = (val * 10) + (*str - L'0');
		++str;
	}
	if (str <= beg)
	{
		return FALSE;
	}
	val *= neg;

	str = PStrSkipWhitespace(str);
	if (*str != 0)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// returns false if string is missing or not a number.
//----------------------------------------------------------------------------
BOOL PStrToInt(
			   const char * str,
			   unsigned __int64 & val)
{
	val = 0;

	if (!str || !str[0])
	{
		return FALSE;
	}

	str = PStrSkipWhitespace(str);

	if (*str == 0)
	{
		return FALSE;
	}

	int neg = 1;
	if (*str == '-')
	{
		neg = -1;
		++str;
	}

	const char * beg = str;
	while (*str >= '0' && *str <= '9')
	{
		val = (val * 10) + (*str - '0');
		++str;
	}
	if (str <= beg)
	{
		return FALSE;
	}
	val *= neg;

	str = PStrSkipWhitespace(str);
	if (*str != 0)
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PIntToStr(
	WCHAR * str,
	int len,
	int value)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_itow_s(value, str, len, 10);
	return str;
#else
	return _itow(value, str, 10);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PIntToStr(
	char * str,
	int len,
	int value)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	_itoa_s(value, str, len, 10);
	return str;
#else
	return itoa(value, str, 10);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrPrintf(
	char * str,
	int buf_len,
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

#ifdef USE_VS2005_STRING_FUNCTIONS
	return _vsnprintf_s(str, buf_len, _TRUNCATE, format, args);
#else
	return _vsnprintf(str, buf_len, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrPrintf(
	WCHAR * str,
	int buf_len,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

#ifdef USE_VS2005_STRING_FUNCTIONS
	return _vsnwprintf_s(str, buf_len, _TRUNCATE, format, args);
#else
	return _vsnwprintf(str, buf_len, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrVprintf(
	char * str,
	int buf_len,
	const char * format,
	va_list& args)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	return _vsnprintf_s(str, buf_len, _TRUNCATE, format, args);
#else
	return _vsnprintf(str, buf_len, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PStrVprintf(
	WCHAR * str,
	int buf_len,
	const WCHAR * format,
	va_list & args)
{
#ifdef USE_VS2005_STRING_FUNCTIONS
	return _vsnwprintf_s(str, buf_len, _TRUNCATE, format, args);
#else
	return _vsnwprintf(str, buf_len, format, args);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrStr(
	const char * str,
	const char * substr)
{
	return strstr(str, substr);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrStr(
	const WCHAR * str,
	const WCHAR * substr)
{
	return wcsstr(str, substr);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrStrI(
	const char * str,
	const char * substr)
{
	return StrStrIA(str, substr);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrStrI(
	const WCHAR * str,
	const WCHAR * substr)
{
	return StrStrIW(str, substr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrPbrk(
	char * str,
	const char * charset)
{
	return strpbrk(str, charset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrPbrk(
	const char * str,
	const char * charset)
{
	return strpbrk(str, charset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrPbrk(
	WCHAR * str,
	const WCHAR * charset)
{
	return wcspbrk(str, charset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrPbrk(
	const WCHAR * str,
	const WCHAR * charset)
{
	return wcspbrk(str, charset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * PStrChr(
	char * str,
	int c)
{
	return strchr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrChr(
	const char * str,
	int c)
{
	return strchr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrChr(
	WCHAR * str,
	WCHAR c)
{
	return wcschr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrChr(
	const WCHAR * str,
	WCHAR c)
{
	return wcschr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * PStrRChr(
	const char * str,
	int c)
{
	return strrchr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * PStrRChr(
	const WCHAR * str,
	WCHAR c)
{
	return wcsrchr(str, c);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrForceBackslash(
	char * str,
	int buflen)
{
	int len = (int)PStrLen(str);
	if ( len == 0 || (len + 1) >= buflen )
		return;
	if ( str[len-1] != '\\' && str[len-1] != '/' )
	{
#ifdef USE_VS2005_STRING_FUNCTIONS
		strcat_s(str, buflen, "\\");
#else
		strcat(str, "\\");
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrForceBackslash(
	WCHAR * str,
	int buflen)
{
	int len = (int)PStrLen(str);
	if ( len == 0 || (len + 1) >= buflen )
		return;
	if ( str[len-1] != L'\\' && str[len-1] != L'/' )
	{
		PStrCat(str, L"\\", buflen);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
void _PStrReplaceExtension(
	T * newstring,
	int buflen,
	const T * oldstring,
	const T * extension)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(oldstring);
	ASSERT_RETURN(extension);

	int oldlength = PStrLen(oldstring, buflen);
	const T * ptr = oldstring + oldlength - 1;
	while (ptr >= oldstring)
	{
		T c = *ptr;
		if ( c == (T)'.' )
			break;
		else if ( c == (T)'\\' || c == (T)'/' )
			ptr = oldstring;

		ptr--;
	}
	if (ptr < oldstring)
	{
		if (newstring != oldstring)
		{
			memcpy(newstring, oldstring, oldlength * sizeof(T));
		}
		newstring[oldlength] = '.';
		PStrCopy(newstring + oldlength + 1, extension, buflen - oldlength - 1);
	}
	else
	{
		int len = (int)(ptr - oldstring + 1);
		if (newstring != oldstring)
		{
			memcpy(newstring, oldstring, len * sizeof(T));
		}
		PStrCopy(newstring + len, extension, buflen - len);
	}
}

void PStrReplaceExtension(char * newstring, int buflen, const char * oldstring, const char * extension)
{
	_PStrReplaceExtension(newstring, buflen, oldstring, extension);
}

void PStrReplaceExtension(WCHAR * newstring, int buflen, const WCHAR * oldstring, const WCHAR * extension)
{
	_PStrReplaceExtension(newstring, buflen, oldstring, extension);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
const T * _PStrGetExtension(
	const T * str)
{
	ASSERT_RETNULL(str);

	int len = PStrLen(str);
	const T * ptr = str + len - 1;
	while (ptr >= str)
	{
		T c = *ptr;
		if ( c == (T)'.' )
			break;
		else if ( c == (T)'\\' || c == (T)'/' )
			ptr = str;

		ptr--;
	}
	if (ptr < str)
	{
		return NULL;
	}
	else
	{
		return ptr;
	}
}

const char * PStrGetExtension(const char * str)
{
	return _PStrGetExtension(str);
}

const WCHAR * PStrGetExtension(const WCHAR * str)
{
	return _PStrGetExtension(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
void _PStrRemoveExtension(
	T * newstring,
	int buflen,
	const T * oldstring)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(oldstring);

	int oldlength = PStrLen(oldstring, buflen);
	const T * ptr = oldstring + oldlength - 1;
	while (ptr >= oldstring)
	{
		T c = *ptr;
		if ( c == (T)'.' )
			break;
		else if ( c == (T)'\\' || c == (T)'/' )
			ptr = oldstring;

		ptr--;
	}
	if (ptr < oldstring)
	{
		if (newstring != oldstring)
		{
			memcpy(newstring, oldstring, (oldlength + 1) * sizeof(T));
		}
	}
	else
	{
		int len = (int)(ptr - oldstring);
		if (newstring != oldstring)
		{
			memcpy(newstring, oldstring, len * sizeof(T));
		}
		newstring[len] = (T)'\0';
	}
}

void PStrRemoveExtension(
	char * newstring,
	int buflen,
	const char * oldstring)
{
	_PStrRemoveExtension(newstring, buflen, oldstring);
}

void PStrRemoveExtension(
	WCHAR * newstring,
	int buflen,
	const WCHAR * oldstring)
{
	_PStrRemoveExtension(newstring, buflen, oldstring);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
int _PStrGetPath(
	T * newstring,
	int buflen,
	const T * oldstring)
{
	ASSERT_RETZERO(newstring);
	ASSERT_RETZERO(oldstring);
	ASSERT_RETZERO(buflen > 0);

	buflen--;		// set aside 0 terminator

	T * cur = newstring;
	int last_slash_pos = -1;
	int len = 0;
	while (*oldstring)
	{
		if (*oldstring == '\\' || *oldstring == '/')
		{
			last_slash_pos = len;
		}
		*cur++ = *oldstring++;
		len++;
		ASSERT(len < buflen);
		if (len >= buflen)
		{
			*newstring = 0;
			return 0;
		}
	}
	if (last_slash_pos < 0)
	{
		*newstring = 0;
		return 0;
	}
	*(newstring + last_slash_pos + 1) = 0;
	return last_slash_pos;
}

int PStrGetPath(char * newstring, int buflen, const char * oldstring)
{
	return _PStrGetPath(newstring, buflen, oldstring);
}

int PStrGetPath(WCHAR * newstring, int buflen, const WCHAR * oldstring)
{
	return _PStrGetPath(newstring, buflen, oldstring);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
void _PStrRemoveEntirePath(
	T * newstring,
	int buflen,
	const T * oldstring)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(oldstring);

	int oldlength = PStrLen(oldstring, buflen);
	const T * ptr = oldstring + oldlength - 1;
	while (ptr >= oldstring)
	{
		if (*ptr == '\\' || *ptr == '/')
		{
			ptr++;
			break;
		}
		ptr--;
	}

	if (ptr < oldstring)
	{
		memcpy(newstring, oldstring, (oldlength + 1) * sizeof(T));
	}
	else
	{
		int len = oldlength - (int)(ptr - oldstring);
		memcpy(newstring, ptr, (len + 1) * sizeof(T));
		newstring[ len + 1 ] = NULL;  // add terminating NULL
	}
}

void PStrRemoveEntirePath(char * newstring, int buflen, const char * oldstring)
{
	_PStrRemoveEntirePath(newstring, buflen, oldstring);
}

void PStrRemoveEntirePath(WCHAR * newstring, int buflen, const WCHAR * oldstring)
{
	_PStrRemoveEntirePath(newstring, buflen, oldstring);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrRemovePath(
	char * newstring,
	int buflen,
	const char * path,
	const char * oldstring)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(path);
	ASSERT_RETURN(oldstring);

	char * substr = PStrStrI(oldstring, path);
	if (!substr)
	{
		PStrCopy(newstring, oldstring, buflen);
		return;
	}
	TCHAR * src = substr + PStrLen(path);
	PStrCopy(newstring, src, buflen);
}

void PStrRemovePath(
	char * newstring,
	int buflen,
	const WCHAR * path,
	const char * oldstring)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(path);
	ASSERT_RETURN(oldstring);


	WCHAR wold[MAX_PATH];
	PStrCvt(wold, oldstring, MAX_PATH);
	WCHAR * substr = PStrStrI(wold, path);
	if (!substr)
	{
		PStrCopy(newstring, oldstring, buflen);
		return;
	}
	WCHAR * src = substr + PStrLen(path);
	PStrCvt(newstring, src, buflen);
}

void PStrRemovePath(
	WCHAR * newstring,
	int buflen,
	const WCHAR * path,
	const WCHAR * oldstring)
{
	ASSERT_RETURN(newstring);
	ASSERT_RETURN(path);
	ASSERT_RETURN(oldstring);


	WCHAR * substr = PStrStrI(oldstring, path);
	if (!substr)
	{
		PStrCopy(newstring, oldstring, buflen);
		return;
	}
	WCHAR * src = substr + PStrLen(path);
	PStrCvt(newstring, src, buflen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrIsNumeric(
	const char * txt)
{
	while (txt && *txt)
	{
		if (!isdigit((unsigned char)(*txt)))
		{
			return FALSE;
		}
		txt++;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrIsNumeric(
	const WCHAR * txt)
{
	BOOL bFirstCharacter = TRUE;
	while (txt && *txt)
	{
		if (!iswdigit(*txt))
		{
			if (bFirstCharacter)
			{
				if (*txt != L'-' && *txt != L'+')  // allow the first digit to be a + or -
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		txt++;
		bFirstCharacter = FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrReplaceChars(
	char * szString,
	char c1,
	char c2)
{
	char * s = szString;
	while (s && *s)
	{
		if (*s == c1)
		{
			*s = c2;
		}
		s++;
	}
}
void PStrReplaceChars(
	WCHAR * szString,
	WCHAR c1,
	WCHAR c2)
{
	WCHAR * s = szString;
	while (s && *s)
	{
		if (*s == c1)
		{
			*s = c2;
		}
		s++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrGroupThousands(
	WCHAR* pszNumber,
	int nBufLen)
{
	int nLen = PStrLen(pszNumber);
	if ( nLen > nBufLen )
		return;

	for (int ii = nLen - 3; ii > 0; ii -= 3)
	{
		ASSERT_RETURN(MemoryMove(&pszNumber[ii + 1], (nBufLen - ii) * sizeof(WCHAR), &pszNumber[ii], sizeof(WCHAR) * (nLen - ii + 1)));
		nLen++;
		pszNumber[ii] = L',';
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrGroupThousands(
	char* pszNumber,
	int nBufLen)
{
	int nLen = PStrLen(pszNumber);
	if ( nLen > nBufLen )
		return;

	for (int ii = nLen - 3; ii > 0; ii -= 3)
	{
		ASSERT_RETURN(MemoryMove(&pszNumber[ii + 1], (nBufLen - ii) * sizeof(char), &pszNumber[ii], sizeof(char) * (nLen - ii + 1)));
		nLen++;
		pszNumber[ii] = ',';
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoTokenReplace( 
	WCHAR *puszString,
	int nStringBufferSize,
	const WCHAR *puszToken,
	const WCHAR *puszReplacement)
{

	WCHAR *pTokenPos = PStrStrI( puszString, puszToken );
	while (pTokenPos)
	{
		// find remainder of string after token
		const WCHAR *pRemainder = MIN(pTokenPos + PStrLen( puszToken ), puszString + nStringBufferSize);
		
		// copy remainder of string to temp buffer
		WCHAR uszTempRemainderBuffer[ MAX_STRING_ENTRY_LENGTH ];	
		ASSERT( PStrLen( pRemainder ) < MAX_STRING_ENTRY_LENGTH );	
		PStrCopy( uszTempRemainderBuffer, pRemainder, MAX_STRING_ENTRY_LENGTH );

		// terminate string at token
		*pTokenPos = 0;
		
		// from our replacement start location, how many characters are there to
		// the position we start the replacement at (we call this the prefix)
		int nPrefixLength = PStrLen( puszString );
		
		// cat replacement on string
		PStrCat( puszString, puszReplacement, nStringBufferSize );
		
		// cat rest of string
		PStrCat( puszString, uszTempRemainderBuffer, nStringBufferSize );
		
		// figure out where we will resume replacement
		int nCharactersToSkip = nPrefixLength + PStrLen( puszReplacement );
		
		// move the local puszString pointer to just beyond what we replaced
		puszString += nCharactersToSkip;  // danger, could be beyond buffer max, but we alter the buffer max below
		
		// consider all the data we've skipped past thus far to be unavailable
		nStringBufferSize -= nCharactersToSkip;
		
		// keep going if needed
		pTokenPos = NULL;
		if (nStringBufferSize > 0)
		{
			pTokenPos = PStrStrI( puszString, puszToken );
		}

	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrReplaceToken( 
	WCHAR *puszString,
	int nStringBufferSize,
	const WCHAR *puszTokens,
	const WCHAR *puszReplacement)
{	
	ASSERTX_RETURN( puszString, "Expected string" );
	ASSERTX_RETURN( puszTokens, "Expected token string" );
	ASSERTX_RETURN( puszReplacement, "Expected replacement string" );

	// we will search for all the tokens found in puszToken	separated by commas
	const WCHAR *uszSeps = L",";
	
	// poor mans parsing through all the tokens we want to look for
	const int MAX_TOKEN = 256;
	WCHAR szToken[ MAX_TOKEN ];
	const WCHAR *pTokenBuffer = puszTokens;
	int nAllTokensLength = PStrLen( puszTokens ) + 1;
	pTokenBuffer = PStrTok( szToken, pTokenBuffer, uszSeps, MAX_TOKEN, nAllTokensLength );
	while (szToken[ 0 ])
	{
	
		// replace this token
		sDoTokenReplace( puszString, nStringBufferSize, szToken, puszReplacement );
				
		// get next token
		pTokenBuffer = PStrTok( szToken, pTokenBuffer, uszSeps, MAX_TOKEN, nAllTokensLength );
		
	}
		
}


//----------------------------------------------------------------------------
// replaces '\n' with designated character and '\r' with space
// returns pointing to end of text (0) character
//----------------------------------------------------------------------------
char * PStrTextFileFixup(
	char * text,
	DWORD size,
	char lf)
{
	char * end = text + size - 1;
	while (text < end)
	{
		switch (*text)
		{
		case 0:
			return text;
		case '\n':
			*text = lf;
			break;
		case '\r':
			*text = ' ';
			break;
		default:
			break;
		}
		++text;
	}
	*text = 0;
	return text;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrCvtToCRLF(
	char * text,
	DWORD size)
{
	ASSERTX_RETURN( text, "Expected buffer" );
	ASSERTX_RETURN( size > 0, "Invalid buffer length" );

	char * ch = text;
	char * end = text + size;
	BOUNDED_WHILE( NULL != ( ch = PStrChr( ch, '\n' ) ), 100000 )
	{
		size_t cpy = ( end - ch ) * sizeof( char );
		ASSERT_BREAK(MemoryMove( ch + 1, cpy, ch, cpy - 1 ));
		*ch = '\r';
		ch += 2;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrPrintfTokenStringValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const WCHAR **puszValues,
	int nNumValues,
	BOOL bAddDigitSeparators /*= FALSE*/)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLength > 0, "Invalid buffer length" );
	ASSERTX_RETURN( puszFormat, "Expected format string" );
	ASSERTX_RETURN( puszTokenBase, "Expected token base" );
	ASSERTX_RETURN( puszValues, "Expected value array" );
	ASSERTX_RETURN( nNumValues > 0, "Invalid value array size" );

	// copy format string to destination
	PStrCopy( puszBuffer, puszFormat, nBufferLength );
				
	// replace each token
	for (int i = 0; i < nNumValues; ++i)
	{
		const WCHAR *puszValue = puszValues[ i ];
		
		// construct this token that we will replace ... string1, string2, etc ...
		const int MAX_TOKEN_LENGTH = 16;
		WCHAR uszToken[ MAX_TOKEN_LENGTH ];		
		PStrPrintf( uszToken, MAX_TOKEN_LENGTH, L"[%s%d]", puszTokenBase, i + 1 );

		// construct the replacement for this token
		const int MAX_REPLACEMENT = 256;
		WCHAR uszReplacementBuffer[ MAX_REPLACEMENT ];
		const WCHAR *puszReplacement = puszValue;
		if (bAddDigitSeparators)
		{
			LanguageFormatNumberString( uszReplacementBuffer, MAX_REPLACEMENT, puszValue );
			puszReplacement = uszReplacementBuffer;
		}
				
		// replace the token with the value
		PStrReplaceToken( puszBuffer, nBufferLength, uszToken, puszReplacement );
				
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStrPrintfTokenIntValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const int *pnValues,
	int nNumValues)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLength > 0, "Invalid buffer length" );
	ASSERTX_RETURN( puszFormat, "Expected format string" );
	ASSERTX_RETURN( puszTokenBase, "Expected token base" );
	ASSERTX_RETURN( pnValues, "Expected value array" );
	ASSERTX_RETURN( nNumValues > 0, "Invalid value array size" );

	// copy format string to destination
	PStrCopy( puszBuffer, puszFormat, nBufferLength );
				
	// replace each token
	for (int i = 0; i < nNumValues; ++i)
	{
		
		// construct this token that we will replace ... string1, string2, etc ...
		const int MAX_TOKEN_LENGTH = 16;
		WCHAR uszToken[ MAX_TOKEN_LENGTH ];		
		PStrPrintf( uszToken, MAX_TOKEN_LENGTH, L"[%s%d]", puszTokenBase, i + 1 );
		
		// get value as string
		const int MAX_VALUE_LEN = 64;
		WCHAR uszValue[ MAX_VALUE_LEN ];
		LanguageFormatIntString( uszValue, MAX_VALUE_LEN, pnValues[ i ] );
		
		// replace the token with the value		
		PStrReplaceToken( puszBuffer, nBufferLength, uszToken, uszValue );
				
	}
	
}


void PStrPrintfTokenNumberValues( 
	WCHAR *puszBuffer,
	int nBufferLength,
	const WCHAR *puszFormat,
	const WCHAR *puszTokenBase,
	const float *pnValues,
	const BOOL *pbFloat,
	int nNumValues)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLength > 0, "Invalid buffer length" );
	ASSERTX_RETURN( puszFormat, "Expected format string" );
	ASSERTX_RETURN( puszTokenBase, "Expected token base" );
	ASSERTX_RETURN( pnValues, "Expected value array" );
	ASSERTX_RETURN( pbFloat, "Expected boolean array for floatiness" );
	ASSERTX_RETURN( nNumValues > 0, "Invalid value array size" );

	// copy format string to destination
	PStrCopy( puszBuffer, puszFormat, nBufferLength );

	// replace each token
	for (int i = 0; i < nNumValues; ++i)
	{

		// construct this token that we will replace ... string1, string2, etc ...
		if(pbFloat[i])
		{
			const int MAX_TOKEN_LENGTH = 16;
			WCHAR uszToken[ MAX_TOKEN_LENGTH ];		
			PStrPrintf( uszToken, MAX_TOKEN_LENGTH, L"[%s%d]", puszTokenBase, i + 1 );

			// get value as string
			const int MAX_VALUE_LEN = 64;
			WCHAR uszValue[ MAX_VALUE_LEN ];
			LanguageFormatFloatString( uszValue, MAX_VALUE_LEN, pnValues[ i ], 1 );

			PStrReplaceToken( puszBuffer, nBufferLength, uszToken, uszValue );
		}
		else
		{
			const int MAX_TOKEN_LENGTH = 16;
			WCHAR uszToken[ MAX_TOKEN_LENGTH ];		
			PStrPrintf( uszToken, MAX_TOKEN_LENGTH, L"[%s%d]", puszTokenBase, i + 1 );

			// get value as string
			const int MAX_VALUE_LEN = 64;
			WCHAR uszValue[ MAX_VALUE_LEN ];
			LanguageFormatIntString( uszValue, MAX_VALUE_LEN, (int)pnValues[ i ] );

			// replace the token with the value		
			PStrReplaceToken( puszBuffer, nBufferLength, uszToken, uszValue );
		}
	}

}


//----------------------------------------------------------------------------
// PStr 1337!
//----------------------------------------------------------------------------
static BOOL sb1337Initialized = FALSE;
static BOOL b1337MatchTable[256][256];

//Hardcoded 1337 translation table.
//Format: the first character of a string is the 1337 number.
//The remaining letters are the letters a number may translate to.
//  Lowercase.

static const char * b1337TranslationTable[] =
{
	"0od",
	"1ilt",
	"2zr",
	"3e",
	"4a",
	"5s",
	"6bg",
	"7tly",
	"8b",
	"9gq"
};
static const int n1337Symbols = arrsize( b1337TranslationTable ); //Number of elements in above array.

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PStr1337Init()
{
	memclear(b1337MatchTable, sizeof(b1337MatchTable) );

	for(int i = 0; i < n1337Symbols; i++)
	{
		const char * str = b1337TranslationTable[i];
		for(const char * p = str + 1; *p != 0; p++)
		{
			b1337MatchTable[*str][*p] = TRUE;
			b1337MatchTable[*p][*str] = TRUE;	
		}
	}

	sb1337Initialized = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sb1337eq(WCHAR a, WCHAR b)
{
	if(tolower(a) == tolower(b)) return TRUE;
	if(a < 256 && b < 256) 
		return b1337MatchTable[tolower(a)][tolower(b)];
	return FALSE;
}

//----------------------------------------------------------------------------
// 1337 insensitive strstr.  Checks for a substring of "a" equivalent
// (including 1337 equivalency) to "b."  Returns a pointer to the beginning
// of the substring in "a".
//----------------------------------------------------------------------------
WCHAR * PStrStrI1337(WCHAR *a, int nSizeA, const WCHAR *b, int nSizeB)
{
	if(!sb1337Initialized) PStr1337Init();
	for(int i = 0; i < nSizeA && *(a+i); i++)
	{
		BOOL bMatch = TRUE;
		for(int j = 0; j < nSizeB && *(b+j); j++)
		{
			if(!sb1337eq(*(a+i+j),*(b+j)))
			{
				bMatch = FALSE;
				break;
			}
		}
		if(bMatch) return (a+i);	
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * PStrStrI1337(WCHAR *a, const WCHAR *b, int nSize)
{
	return PStrStrI1337(a, nSize, b, nSize);
}

//----------------------------------------------------------------------------
// l33t insensitive string comparison.  Returns non-zero for different strings
// also case insensitive.
//----------------------------------------------------------------------------
int PStrCmpI1337(const WCHAR *a, const WCHAR *b, int nLength)
{
	if(!sb1337Initialized) PStr1337Init();

	for(int i = 0; i < nLength; i++)
	{
		WCHAR cA = (WCHAR)tolower(*(a+i));
		WCHAR cB = (WCHAR)tolower(*(b+i));
		int toRet = cB-cA;
		if(cA == 0 || cB == 0) return toRet;
		if(toRet == 0) continue;
		if(!sb1337eq(cA, cB)) return toRet;
	}
	return 0;
}

//----------------------------------------------------------------------------
// For foreign characters such as Korean, we don't want to consider every
// character as a word delimeter, else we substring filter in all cases.
// Therefore we consider NO characters above 256 as delimeters.
//----------------------------------------------------------------------------
static BOOL sIsWordDelimeter(WCHAR wc)
{
	return(!isalpha(wc) && wc < 256);
}

//----------------------------------------------------------------------------
// 1337 and case insensitive search and replace on a string
// Filter-specific, overwrites "in place"
// Annoyingly complicated to account for our needs to variously filter by
// whole word, substring, prefix, and postfix.
//----------------------------------------------------------------------------
void PStrReplaceTokenI1337( 
	WCHAR *puszString,
	int nStringBufferSize,
	const WCHAR *puszToken,
	const WCHAR *puszReplacement,
	BOOL bAllowPrefix /*= TRUE*/,
	BOOL bAllowSuffix /*= TRUE*/)
{
	WCHAR *pszStart = puszString;
	WCHAR *psEnd = puszString + nStringBufferSize;
	WCHAR *psToReplace = PStrStrI1337(puszString, puszToken, nStringBufferSize);
	int nTokenLetters = 0;
	for(int i = 0; i < nStringBufferSize; i++)
	{
		if(*(puszToken + i) == 0) break;
		nTokenLetters++;
	}

	BOUNDED_WHILE(psToReplace && psToReplace < psEnd && *psToReplace, 50)
	{
		//We've found a word to replace.  Substitute in the puszReplacement characters.
		if(!bAllowPrefix)
		{	//Check whether word is prefixed, if so pass over it and advance the pointer.
			BOOL bPrefixed = (psToReplace != pszStart) && !sIsWordDelimeter(psToReplace[-1]);

			if(bPrefixed)
			{
				puszString = psToReplace + 1;
				if(puszString >= psEnd) break;
				psToReplace = PStrStrI1337(puszString, puszToken, int(psEnd - puszString));
				continue;
			}
		}
		if(!bAllowSuffix)
		{
			BOOL bSuffixed = (psToReplace + nTokenLetters) < psEnd && 
				!sIsWordDelimeter(*(psToReplace + nTokenLetters));
			//Check whether word is suffixed, if so pass over it and advance the pointer.

			if(bSuffixed)
			{
				puszString = psToReplace + 1;
				if(puszString >= psEnd) break;
				psToReplace = PStrStrI1337(puszString, puszToken, int(psEnd - puszString));
				continue;
			}
		}

		int i = 0;
		while(*psToReplace && psToReplace < psEnd)
		{
			if(*(puszReplacement+i) == 0) break;
			*psToReplace = *(puszReplacement+i);
			psToReplace++;
			i++;
		}

		if(*psToReplace == 0) break; //If we're at the end of the line, quit
		else puszString = psToReplace; //Otherwise only search the remaining string.

		//Get another word.
		psToReplace = PStrStrI1337(puszString, puszToken, int(psEnd - puszString));
	}
}

//----------------------------------------------------------------------------
// Replaces Printf-style control characters in a string (ie, %) with escape 
// versions (ie, %%).
//----------------------------------------------------------------------------
void PStrRemoveControlCharacters( 
	WCHAR *puszString,
	int nStringBufferSize,
	int nMaxReplacements)
{
	int nNumReplacements = 0;
	for (int i=0; i<nStringBufferSize && puszString[i]; ++i)
	{
		if (puszString[i] == L'%')
		{
			const int iStrLen = (int)PStrLen(puszString);
			if (iStrLen+2 < nStringBufferSize && nNumReplacements < nMaxReplacements)
			{
				for (int j=iStrLen; j >= i; --j)
				{
					puszString[j+1] = puszString[j];
				}
				++nNumReplacements;
				++i;
			}
			else
			{
				puszString[i] = L' ';
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrSeparateServer(
	const WCHAR *puszServer,
	WCHAR *puszServerBase,
	int nMaxServerBase,
	WCHAR *puszServerInitPath,
	int nMaxServerInitPath)
{
	ASSERTX_RETFALSE( puszServer, "Expected server buffer" );
	ASSERTX_RETFALSE( puszServerBase, "Expected server base buffer" );
	ASSERTX_RETFALSE( nMaxServerBase != 0, "Invalid server base buffer size" );
	ASSERTX_RETFALSE( puszServerInitPath, "Expected server init path buffer" );
	ASSERTX_RETFALSE( nMaxServerInitPath != 0, "Invalid server init path buffer size" );
	ASSERTX_RETFALSE( puszServer != puszServerBase && puszServerBase != puszServerInitPath, "Invalid arguments" );	
	
	// copy server to server base
	PStrCopy( puszServerBase, puszServer, nMaxServerBase );

	// init the server init path to empty
	puszServerInitPath[ 0 ] = 0;

	// scan the server base and terminate at the first slash or backslash
	int nLenServerBase = PStrLen( puszServerBase );
	for (int i = 0; i < nLenServerBase; ++i)
	{
		WCHAR uc = puszServerBase[ i ];
		if (uc == '/' || uc == '\\')
		{
		
			// copy from this position to the init path
			PStrCopy( puszServerInitPath, &puszServerBase[ i ], nMaxServerInitPath );
			
			// terminate server base at this character
			puszServerBase[ i ] = 0;
			
			break;  // we're done
			
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PStrIsPrintable(
	WCHAR c)
{
	if (c == 0xFFFE || c == 0xFFFF)
		return FALSE;

	if (c == 0x2122)	//trademark symbol
		return TRUE;

	if (c == 0x30fc || c == 0x3005)	//important Japanese characters
		return TRUE;

	return _iswprint_l(c, LanguageGetLocale());
}

