//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include "textlog.h"

//-----------------------------------------------------------------------------

template<>
void TextLog<char>::WriteVA(const char_type * pFmt, va_list Args)
{
	_Write(pFmt, Args);
}

template<>
void TextLog<char>::WriteLnVA(const char_type * pFmt, va_list Args)
{
	WriteVA(pFmt, Args);
	NewLine();
}

template<>
void TextLog<char>::NewLine(void)
{
	_Write("\n", 0);
}

//-----------------------------------------------------------------------------

template<>
void TextLog<wchar_t>::Write(const char_type * pFmt, ...)
{
	va_list Args;
	va_start(Args, pFmt);
	_Write(pFmt, Args);
}

template<>
void TextLog<wchar_t>::WriteLn(const char_type * pFmt, ...)
{
	va_list Args;
	va_start(Args, pFmt);
	_Write(pFmt, Args);
	NewLine();
}

template<>
void TextLog<wchar_t>::NewLine(void)
{
	_Write(L"\r\n", 0);
}
