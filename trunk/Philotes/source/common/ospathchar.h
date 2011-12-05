//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#ifndef _OS_PATH_CHAR_H_
#define _OS_PATH_CHAR_H_

#define OS_PATH_CHAR_IS_UNICODE 1

#ifdef OS_PATH_CHAR_IS_UNICODE
typedef wchar_t OS_PATH_CHAR;
#define __OS_PATH_TEXT(x) L##x
#define OS_PATH_FUNC(x) x##W
#define OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(x) WIDE_CHAR_TO_ASCII_FOR_DEBUG_TRACE_ONLY(x)	// For use ONLY in debug traces, etc.
#else
typedef char OS_PATH_CHAR;
#define __OS_PATH_TEXT(x) x
#define OS_PATH_FUNC(x) x##A
#define OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(x) x
#endif
#define OS_PATH_TEXT(x) __OS_PATH_TEXT(x)

const char * WIDE_CHAR_TO_ASCII_FOR_DEBUG_TRACE_ONLY(const wchar_t * pStr);	// For use ONLY in debug traces, etc.

#endif
