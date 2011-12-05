//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _FILETEXTLOG_H_
#define _FILETEXTLOG_H_

#include "textlog.h"
#include "ospathchar.h"
#include <stdio.h>	// for FILE

template<class T>
class FileTextLog : public TextLog<T>
{
	public:
		FileTextLog(void);
		~FileTextLog(void);
		bool Open(OS_PATH_CHAR * pFileName);
		void Close(void);

	protected:
		virtual void _Write(const char_type * pFmt, va_list Args);

	private:
		FILE * pFile;
};

typedef FileTextLog<char> FileTextLogA;
typedef FileTextLog<wchar_t> FileTextLogW;

#endif
