//----------------------------------------------------------------------------
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

#ifndef	_STDAFX_H_
#define	_STDAFX_H_



#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN			// exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER					// allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0500			// change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT			// allow use of features specific to Windows NT 5 or later.
#define _WIN32_WINNT 0x0501		// change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS			// allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410	// change this to the appropriate value to target Windows Me or later.
#endif

#if defined( _DEBUG )			// must define ahead of <afxwin.h> if we want to get debug heap info
//#define _CRTDBG_MAP_ALLOC		// because <afxwin.h> includes <crtdbg.h>
#endif

#ifndef HAMMER

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
// #include <windows.h>

#else	// HAMMER

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#endif	// HAMMER

#define DIRECTINPUT_VERSION 0x0800

#ifndef DONT_INCLUDE_MAYHEM
#pragma warning(push)
#include "mayhem.h"
#pragma warning(pop)
#pragma warning(default : 6011 6385 6386 6031 4200 4714 )
#endif

#include <limits.h>
#include <float.h>
//#include <stdio.h>
#include <shlwapi.h>
#include <tchar.h>
#include <math.h>
#include <io.h>
#include <time.h>
#include <direct.h>
#include <winsock2.h>
#include <sql.h>			// database
#include <sqlext.h>			// database
#include <odbcinst.h>		// database
#include <winnt.h>
#include <intrin.h>
#include "commontypes.h"
#include "common.h"
#include "presult.h"
#include "globals.h"
#include "debug.h"
#include "memory.h"
#include "bitmanip.h"
#include "pstring.h"
#include "id.h"
#include "ptime.h"
#include "array.h"
#include "log.h"
#include "critsect.h"
#include "hash.h"
#include "maths.h"
#include "matrix.h"
#include "vector.h"
#include "definition_common.h"		// here because otherwise random.h will try to include it, but for dev version only
#include "random.h"
#include "net.h"
#include "netmsg.h"
#include "xfer.h"
#include "asyncfile.h"
#include "fileio.h"
#include "excel_common.h"


#endif	// _STDAFX_H_
