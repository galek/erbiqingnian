//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _TEXTLOG_H_
#define _TEXTLOG_H_

template<class T>
class TextLog
{
	public:
		typedef T char_type;

		virtual ~TextLog(void) throw() {}

		void Write(const char_type * pFmt, ...);
		void WriteLn(const char_type * pFmt, ...);
		void NewLine(void);

		void WriteVA(const char_type * pFmt, va_list Args);
		void WriteLnVA(const char_type * pFmt, va_list Args);

	protected:
		virtual void _Write(const char_type * pFmt, va_list Args) = 0;
};

typedef TextLog<char> TextLogA;
typedef TextLog<wchar_t> TextLogW;

#endif
