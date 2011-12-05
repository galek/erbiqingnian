//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _DEBUGTEXTLOG_H_
#define _DEBUGTEXTLOG_H_

#include "textlog.h"

template<class T>
class DebugTextLog : public TextLog<T>
{
	protected:
		virtual void _Write(const char_type * pFmt, va_list Args);
};

typedef DebugTextLog<char> DebugTextLogA;
typedef DebugTextLog<wchar_t> DebugTextLogW;

#endif
