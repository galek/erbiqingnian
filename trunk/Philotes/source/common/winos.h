//----------------------------------------------------------------------------
// winos.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _WIN_OS_H_
#define _WIN_OS_H_

#include "ospathchar.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define CMDLINE_RUN_MAXIMIZED	MAKE_MASK(0)
#define CMDLINE_RUN_MINIMIZED	MAKE_MASK(1)
#define CMDLINE_RUN_NOMSG		MAKE_MASK(2)
#define CMDLINE_RUN_ASYNC		MAKE_MASK(3)
#define CMDLINE_HIDE_WINDOW		MAKE_MASK(4)

#define CMDLINE_RUN_DEFAULT		(0)

#define INTEL_CHANGES

#if defined( _WIN64 ) && FALSE
	// we are having thread priority/affinity control issues in 64 bit.
	#undef INTEL_CHANGES
#endif

// Based on major, minor, syspack major, syspack minor.  Used for numerical version comparison.
#define OS_XP_ID				5100
#define OS_VISTA_ID				6000

#define MS_HOTFIX_940105_URL	_T("http://support.microsoft.com/default.aspx/kb/940105")

//----------------------------------------------------------------------------
// Uncomment to disable trace outputs
//#define DISABLE_OUTPUT_DEBUG_STRING
//----------------------------------------------------------------------------



#ifdef DISABLE_OUTPUT_DEBUG_STRING
#	undef OutputDebugString
#	define OutputDebugString(str)	REF(str)
#endif

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// STRUCT
//----------------------------------------------------------------------------
struct CPU_INFO
{
	unsigned int	m_TotalLogical;
	unsigned int	m_AvailLogical;
	unsigned int	m_AvailCore;
	unsigned int	m_AvailPhysical;
};
#ifdef INTEL_CHANGES
extern CPU_INFO gtCPUInfo;
#endif

enum GDI_LOGO_TYPE
{
	LOGO_SPLASH = 0,
	LOGO_STANDBY,
	// count
	NUM_GDI_LOGO_TYPES
};

#ifdef WIN32
// ---------------------------- WINDOWS-ONLY ---------------------------------


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct FIND_DATA
{
	DWORD		dwFileAttributes; 
	PTIME		tCreationTime;
	PTIME		tLastAccessTime;
	PTIME		tLastWriteTime;
	QWORD		nFileSize;
	TCHAR		cFileName[MAX_PATH];
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL PGetModuleDirectory(
	OS_PATH_CHAR * path,
	int size);

BOOL PSetCurrentDirectory(
	const OS_PATH_CHAR * path);

BOOL PGetMemoryStatus(
	MEMORYSTATUSEX & memory_status);

UINT64 PGetSystemTimeIn100NanoSecs(
	void);

UINT64 PGetPerformanceFrequency(
	void);

BOOL CPUInfo(
	CPU_INFO & info);

BOOL CPUHasSSE();

BOOL IsDebuggerAttached(
	void);

BOOL PExecuteCmdLine(
	const OS_PATH_CHAR * CmdLine,
	DWORD dwFlags = CMDLINE_RUN_DEFAULT,
	HANDLE * pProcess = NULL,
	DWORD * pProcessID = 0 );

// Shortcut path must end in .lnk (eg, "Shortcut to program.exe.lnk")
PRESULT PResolveShortcut(/*in*/ const OS_PATH_CHAR * lpszShortcutPath,
						 int nMaxLenShortcut,
						 /*out*/ OS_PATH_CHAR * lpszFilePath,
						 int nMaxLenFile);

void GDI_Init( HWND hWnd, HINSTANCE hInstance );
void GDI_Destroy();
void GDI_ClearWindow( HWND hWnd );
void GDI_ShowLogo( HWND hWnd, GDI_LOGO_TYPE eType = LOGO_STANDBY );
void GDI_ShowLogo( HWND hWnd, HDC hDC, GDI_LOGO_TYPE eType = LOGO_STANDBY );	// CHB 2007.10.01

BOOL WindowsIsXP();
BOOL WindowsIsVistaOrLater();

BOOL WindowsIsX86();
inline BOOL WindowsIsX64() {return !WindowsIsX86();};

bool IsVistaKB940105Required();

const char * WindowsDebugGetMessageName(unsigned nMsg);	// CHB 2007.08.27

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
inline UINT64 PGetPerformanceCounter(
	void)
{
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	return (UINT64)temp.QuadPart;
}

#if DEBUG_USE_TRACES
inline void PDebugTrace(
	const char * str)
{
	OutputDebugString((char*)str);
}
#else
inline void PDebugTrace(const char *) {}
#endif


#endif // _WIN_OS_H_
#endif