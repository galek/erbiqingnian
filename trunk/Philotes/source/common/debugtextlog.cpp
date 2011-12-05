//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include "debugtextlog.h"

//-----------------------------------------------------------------------------

template<>
void DebugTextLog<char>::_Write(const char_type * pFmt, va_list Args)
{
	vtrace(pFmt, Args);
}

//-----------------------------------------------------------------------------

#if 0
template<>
void DebugTextLog<wchar_t>::_Write(const char_type * pFmt, va_list Args)
{
	char_type str[4096];
	PStrVprintf(str, _countof(str), pFmt, Args);
	::OutputDebugStringW(str);
}
#endif
