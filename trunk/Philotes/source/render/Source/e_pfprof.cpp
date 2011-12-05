//*****************************************************************************
//
// Helper functions for memory profiling.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_pfprof.h"

#ifdef ENABLE_PAGE_FAULT_PROFILING

//-----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>

#include <psapi.h>
#pragma comment(lib,"psapi")

#pragma warning(disable:4996)	// warning C4996: 'sprintf' was declared deprecated

//-----------------------------------------------------------------------------

unsigned __stdcall DoZwReadFile(HANDLE FileHandle, HANDLE Event, void * ApcRoutine, void * ApcContext, void * IoStatusBlock, void * Buffer, unsigned Length, LARGE_INTEGER * ByteOffset, ULONG * Key);
unsigned __stdcall DoZwAllocateVirtualMemory(HANDLE hProcess, void * * pAddr, unsigned nZeroBits, unsigned * pSize, unsigned nType, unsigned nProtect);
unsigned __stdcall DoZwMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, void * * BaseAddress, unsigned * ZeroBits, unsigned CommitSize, LARGE_INTEGER * SectionOffset, unsigned * ViewSize, int InheritDisposition, unsigned AllocationType, unsigned Win32Protect);

//-----------------------------------------------------------------------------

namespace
{

HANDLE hLogFile = 0;
LARGE_INTEGER nStartCounter;
double fCounterRate = 0.0;

//-----------------------------------------------------------------------------

char * _Copy(char * pDst, const char * pSrc)
{
	for (;;) {
		char c = *pSrc++;
		if (c == '\0') {
			break;
		}
		*pDst++ = c;
	}
	return pDst;
}

char * _Hex(char * p, unsigned n)
{
	for (int i = 0; i < 8; ++i) {
		int c = n & 15;
		n >>= 4;
		p[7 - i] = (c > 9) ? (c - 10 + 'A') : (c + '0');
	}
	return p + 8;
}

char * _Hex(char * p, void * n)
{
	return _Hex(p, static_cast<unsigned>(reinterpret_cast<DWORD_PTR>(n)));
}

char * _Dec(char * p, unsigned n)
{
	int i = 0;
	do {
		p[i++] = (n % 10) + '0';
		n /= 10;
	} while (n > 0);

	int h = i / 2;
	--i;
	for (int j = 0; j < h; ++j) {
		char t = p[i - j];
		p[i - j] = p[j];
		p[j] = t;
	}
	return p + i + 1;
}

//-----------------------------------------------------------------------------

inline
unsigned GetNextSerialNumber(void)
{
	static LONG _nSeq = 0;
	return ::InterlockedIncrement(&_nSeq);
}

inline
void Write(const char * pStr, size_t nChars)
{
	DWORD nBytesWritten;
	::WriteFile(
		hLogFile,
		pStr,
		static_cast<DWORD>(nChars),
		&nBytesWritten,
		0
	);
}

inline
void Write(const char * p)
{
	Write(p, strlen(p));
}

unsigned GetTime(void)
{
	LARGE_INTEGER li;
	::QueryPerformanceCounter(&li);
	float fTime = static_cast<float>((li.QuadPart - nStartCounter.QuadPart) * fCounterRate);
	return static_cast<int>(fTime + 0.5f);
}

inline
unsigned WriteLine(const char * pLine, size_t nChars)
{
	unsigned nSn = GetNextSerialNumber();
	char buf[64];
	char * p = buf;
	p = _Dec(p, nSn);
	*p++ = ',';
	p = _Dec(p, GetTime());
	*p++ = ',';
//	p = _Dec(p, ::GetCurrentThreadId());
//	*p++ = ',';
	Write(buf, p - buf);
	Write(pLine, nChars);
	return nSn;
}

inline
unsigned WriteLine(const char * pLine)
{
	return WriteLine(pLine, strlen(pLine));
}

//-----------------------------------------------------------------------------

bool GetFileNameFromHandle(HANDLE hFile, TCHAR * pFileNameOut, unsigned nChars)
{
	bool bResult = false;
	HANDLE hMapping = 0;
	void * pView = 0;

	do {
		hMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 1, 0);
		if (hMapping == 0) {
			break;
		}

		pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 1);
		if (pView == 0) {
			break;
		}

		if (!GetMappedFileName(GetCurrentProcess(), pView, pFileNameOut, nChars)) {
			break;
		}

		bResult = true;
	} while (false);

	if (pView != 0) {
		UnmapViewOfFile(pView);
	}
	if (hMapping != 0) {
		CloseHandle(hMapping);
	}

	return bResult;
}

//-----------------------------------------------------------------------------

unsigned __stdcall MyZwReadFile(HANDLE FileHandle, HANDLE Event, void * ApcRoutine, void * ApcContext, void * IoStatusBlock, void * Buffer, unsigned Length, LARGE_INTEGER * ByteOffset, ULONG * Key)
{
	char fname[512];
	if (!GetFileNameFromHandle(FileHandle, fname, ARRAYSIZE(fname))) {
		fname[0] = '\0';
	}
	char buf[256];
	sprintf(buf, "ZwReadFile,0x%p,%u,\"%s\",%I64d\n", Buffer, Length, fname, (ByteOffset != 0) ? ByteOffset->QuadPart : (__int64)(-1));
	unsigned nSn = WriteLine(buf);
	unsigned nResult = DoZwReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
//	DWORD nError = GetLastError();
	sprintf(buf, "ZwReadFile(Out),%u\n", nSn);
	WriteLine(buf);
//	SetLastError(nError);
	return nResult;
}

unsigned __stdcall MyZwAllocateVirtualMemory(HANDLE hProcess, void * * pAddr, unsigned nZeroBits, unsigned * pSize, unsigned nType, unsigned nProtect)
{
	// Don't use sprintf() in here because it can cause memory to be allocated.
	unsigned nResult = DoZwAllocateVirtualMemory(hProcess, pAddr, nZeroBits, pSize, nType, nProtect);
	if (SUCCEEDED(nResult)) {
		char buf[64];
		char * p = buf;
		p = _Copy(p, "ZwAllocateVirtualMemory,0x");
		p = _Hex(p, *pAddr);
//		p = _Copy(p, ",0x");
		*p++ = ',';
		p = _Dec(p, *pSize);
		*p++ = '\n';
		WriteLine(buf, p - buf);
	}
	return nResult;
}

unsigned __stdcall MyZwMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, void * * BaseAddress, unsigned * ZeroBits, unsigned CommitSize, LARGE_INTEGER * SectionOffset, unsigned * ViewSize, int InheritDisposition, unsigned AllocationType, unsigned Win32Protect)
{
	unsigned nResult = DoZwMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		ZeroBits,
		CommitSize,
		SectionOffset,
		ViewSize,
		InheritDisposition,
		AllocationType,
		Win32Protect
	);
	if (SUCCEEDED(nResult)) {
		//
	}
	return nResult;
}

//-----------------------------------------------------------------------------

void Patch(HMODULE hModule, const char * pFuncName, FARPROC pHook)
{
	FARPROC pProc = ::GetProcAddress(hModule, pFuncName);
	unsigned char * pPatch = (unsigned char *)pProc;
	unsigned char * pNew = (unsigned char *)pHook;
	DWORD nOldProtect;
	::VirtualProtect(pPatch, 5, PAGE_EXECUTE_READWRITE, &nOldProtect);
	unsigned nDelta = pNew - (pPatch + 5);
	pPatch[0] = 0xe9;	// jmp
	pPatch[1] = (nDelta >>  0) & 0xff;
	pPatch[2] = (nDelta >>  8) & 0xff;
	pPatch[3] = (nDelta >> 16) & 0xff;
	pPatch[4] = (nDelta >> 24) & 0xff;
	::VirtualProtect(pPatch, 5, nOldProtect, &nOldProtect);
}

//-----------------------------------------------------------------------------

enum
{
	COUNT_FLAG_USER		= 0,
	COUNT_FLAG_KERNEL	= 1,
	COUNT_FLAG_SOFT		= 0,
	COUNT_FLAG_HARD		= 2,
	COUNT_FLAG_DATA		= 0,
	COUNT_FLAG_CODE		= 4,

	COUNT_SOFT_USER_DATA	= COUNT_FLAG_SOFT | COUNT_FLAG_USER | COUNT_FLAG_DATA,
	COUNT_SOFT_USER_CODE	= COUNT_FLAG_SOFT | COUNT_FLAG_USER | COUNT_FLAG_CODE,
	COUNT_SOFT_KERNEL_DATA	= COUNT_FLAG_SOFT | COUNT_FLAG_KERNEL | COUNT_FLAG_DATA,
	COUNT_SOFT_KERNEL_CODE	= COUNT_FLAG_SOFT | COUNT_FLAG_KERNEL | COUNT_FLAG_CODE,
	COUNT_HARD_USER_DATA	= COUNT_FLAG_HARD | COUNT_FLAG_USER | COUNT_FLAG_DATA,
	COUNT_HARD_USER_CODE	= COUNT_FLAG_HARD | COUNT_FLAG_USER | COUNT_FLAG_CODE,
	COUNT_HARD_KERNEL_DATA	= COUNT_FLAG_HARD | COUNT_FLAG_KERNEL | COUNT_FLAG_DATA,
	COUNT_HARD_KERNEL_CODE	= COUNT_FLAG_HARD | COUNT_FLAG_KERNEL | COUNT_FLAG_CODE,
};

unsigned nCounts[8] = { 0 };
unsigned nOverflows = 0;
PSAPI_WS_WATCH_INFORMATION WorkingSetBuffer[4096];

void Summary(const unsigned nCounts[])
{
	char buf[256];
	sprintf(buf,
		"\n"
		"      |---      Data     ---|  |---      Code     ---|  |---     Total     ---|\n\n"
		"       User   Kernel    Sum     User   Kernel    Sum     User   Kernel    Sum\n"
		"      ------- ------- -------  ------- ------- -------  ------- ------- -------\n"
	);
	Write(buf);
	sprintf(buf,
		"Hard  %7u %7u %7u  %7u %7u %7u  %7u %7u %7u\n",
		nCounts[COUNT_HARD_USER_DATA],
		nCounts[COUNT_HARD_KERNEL_DATA],
		nCounts[COUNT_HARD_USER_DATA] +
			nCounts[COUNT_HARD_KERNEL_DATA],
		nCounts[COUNT_HARD_USER_CODE],
		nCounts[COUNT_HARD_KERNEL_CODE],
		nCounts[COUNT_HARD_USER_CODE] +
			nCounts[COUNT_HARD_KERNEL_CODE],
		nCounts[COUNT_HARD_USER_DATA] +
			nCounts[COUNT_HARD_USER_CODE],
		nCounts[COUNT_HARD_KERNEL_DATA] +
			nCounts[COUNT_HARD_KERNEL_CODE],
		nCounts[COUNT_HARD_USER_DATA] + nCounts[COUNT_HARD_KERNEL_DATA] + nCounts[COUNT_HARD_USER_CODE] + nCounts[COUNT_HARD_KERNEL_CODE]
	);
	Write(buf);
	sprintf(buf,
		"Soft  %7u %7u %7u  %7u %7u %7u  %7u %7u %7u\n",
		nCounts[COUNT_SOFT_USER_DATA],
		nCounts[COUNT_SOFT_KERNEL_DATA],
		nCounts[COUNT_SOFT_USER_DATA] +
			nCounts[COUNT_SOFT_KERNEL_DATA],
		nCounts[COUNT_SOFT_USER_CODE],
		nCounts[COUNT_SOFT_KERNEL_CODE],
		nCounts[COUNT_SOFT_USER_CODE] +
			nCounts[COUNT_SOFT_KERNEL_CODE],
		nCounts[COUNT_SOFT_USER_DATA] +
			nCounts[COUNT_SOFT_USER_CODE],
		nCounts[COUNT_SOFT_KERNEL_DATA]+
			nCounts[COUNT_SOFT_KERNEL_CODE],
		nCounts[COUNT_SOFT_USER_DATA] + nCounts[COUNT_SOFT_KERNEL_DATA] + nCounts[COUNT_SOFT_USER_CODE] + nCounts[COUNT_SOFT_KERNEL_CODE]
	);
	Write(buf);
	sprintf(buf,
		"Total %7u %7u %7u  %7u %7u %7u  %7u %7u %7u\n",
		nCounts[COUNT_SOFT_USER_DATA] +
			nCounts[COUNT_HARD_USER_DATA],
		nCounts[COUNT_SOFT_KERNEL_DATA] +
			nCounts[COUNT_HARD_KERNEL_DATA],
		nCounts[COUNT_SOFT_USER_DATA] +
			nCounts[COUNT_HARD_USER_DATA] +
			nCounts[COUNT_SOFT_KERNEL_DATA] +
			nCounts[COUNT_HARD_KERNEL_DATA],
		nCounts[COUNT_SOFT_USER_CODE] +
			nCounts[COUNT_HARD_USER_CODE],
		nCounts[COUNT_SOFT_KERNEL_CODE] +
			nCounts[COUNT_HARD_KERNEL_CODE],
		nCounts[COUNT_SOFT_USER_CODE] +
			nCounts[COUNT_HARD_USER_CODE] +
			nCounts[COUNT_SOFT_KERNEL_CODE] +
			nCounts[COUNT_HARD_KERNEL_CODE],

		nCounts[COUNT_SOFT_USER_DATA] +
			nCounts[COUNT_SOFT_USER_CODE] +
			nCounts[COUNT_HARD_USER_DATA] +
			nCounts[COUNT_HARD_USER_CODE],

		nCounts[COUNT_SOFT_KERNEL_DATA]+
			nCounts[COUNT_SOFT_KERNEL_CODE] +
			nCounts[COUNT_HARD_KERNEL_DATA] +
			nCounts[COUNT_HARD_KERNEL_CODE],

		nCounts[COUNT_SOFT_USER_DATA] + nCounts[COUNT_SOFT_KERNEL_DATA] + nCounts[COUNT_SOFT_USER_CODE] + nCounts[COUNT_SOFT_KERNEL_CODE] +
			nCounts[COUNT_HARD_USER_DATA] + nCounts[COUNT_HARD_KERNEL_DATA] + nCounts[COUNT_HARD_USER_CODE] + nCounts[COUNT_HARD_KERNEL_CODE]
	);
	Write(buf);
}

CRITICAL_SECTION PfCs;

unsigned ProcessPageFaultData(void)
{
	::EnterCriticalSection(&PfCs);

	HANDLE hProcess = ::GetCurrentProcess();

	if (!::GetWsChanges(hProcess, WorkingSetBuffer, sizeof(WorkingSetBuffer))) {
		::LeaveCriticalSection(&PfCs);
//		ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
		return 0;
	}

	int i;
    for (i = 0; /**/; ++i) {
		void * Pc = WorkingSetBuffer[i].FaultingPc;
		void * Va = WorkingSetBuffer[i].FaultingVa;

		if (Pc == 0) {
			break;
		}
		if (Va == 0) {
			continue;
		}

		unsigned nFlags = 0;

		// What about 3GB mode?
		// What about 64-bit?
		nFlags |= ((reinterpret_cast<DWORD_PTR>(Pc) & 0x80000000) != 0) ? COUNT_FLAG_KERNEL : COUNT_FLAG_USER;

		// Least significant bit indicates hard/soft fault.
		nFlags |= ((reinterpret_cast<DWORD_PTR>(Va) & 1) != 0) ? COUNT_FLAG_SOFT : COUNT_FLAG_HARD;

		// If the faulting VA is the PC, code execution caused the fault.
		Va = reinterpret_cast<void *>(reinterpret_cast<DWORD_PTR>(Va) & (-2));
		nFlags |= (reinterpret_cast<void *>(reinterpret_cast<DWORD_PTR>(Pc) & (-2)) == Va) ? COUNT_FLAG_CODE : COUNT_FLAG_DATA;

		// Tally it.
		++nCounts[nFlags];

		// Only interested in displaying hard faults.
		if ((nFlags & COUNT_FLAG_HARD) == 0) {
			continue;
		}

		//
		char buf[256];
		char * p = buf;
		p = _Copy(p, "PageFault,");
		*p++ = ((nFlags & COUNT_FLAG_HARD) != 0) ? 'H' : 'S';
		*p++ = ',';
		*p++ = ((nFlags & COUNT_FLAG_KERNEL) != 0) ? 'K' : 'U';
		*p++ = ',';
		*p++ = ((nFlags & COUNT_FLAG_CODE) != 0) ? 'C' : 'D';
		p = _Copy(p, ",0x");
		p = _Hex(p, Pc);
		p = _Copy(p, ",0x");
		p = _Hex(p, Va);
		*p++ = '\n';
		WriteLine(buf, p - buf);
	}

    // If the buffer overflowed then the terminating item
	// has a non-zero VA.
	if (WorkingSetBuffer[i].FaultingVa != 0) {
		++nOverflows;
		if ((nOverflows % 10) == 1) {
			char buf[128];
			char * p = buf;
			p = _Copy(p, "PageFault,WARNING: Page fault buffer data lost (");
			p = _Dec(p, nOverflows);
			p = _Copy(p, " overflows)\n");
			WriteLine(buf, p - buf);
		}
	}

	::LeaveCriticalSection(&PfCs);

	return i;
}

volatile bool bShutdown = false;

DWORD WINAPI PageFaultThread(void * /*pParameter*/)
{
	unsigned nSleep = 20;
	while (!bShutdown) {
		if (ProcessPageFaultData() > 0) {
			while (ProcessPageFaultData() > 0);
			nSleep = 20;
		}
		else {
			nSleep = min(nSleep + 10, 40);
		}
		::Sleep(nSleep);
	}
	return 0;
}

HANDLE hPageFaultThread = 0;

bool InitPageFaultMonitor(void)
{
	// Lock the buffer into physical memory so it never incurs a page
	// fault -- to help improve performance when GetWsChanges() copies
	// to the buffer.
	if (!::VirtualLock(WorkingSetBuffer, sizeof(WorkingSetBuffer))) {
		return false;
	}

	// Initialize page fault monitoring.
	::InitializeProcessForWsWatch(::GetCurrentProcess());

	// Create the monitor thread.
	DWORD nThreadId;
	hPageFaultThread = ::CreateThread(
		0,					// LPSECURITY_ATTRIBUTES lpThreadAttributes
		0,					// SIZE_T dwStackSize
		&PageFaultThread,	// LPTHREAD_START_ROUTINE lpStartAddress,
		0,					// LPVOID lpParameter
		0,					// DWORD dwCreationFlags
		&nThreadId			// LPDWORD lpThreadId
	);
	if ((hPageFaultThread == 0) || (hPageFaultThread == INVALID_HANDLE_VALUE)) {
		return false;
	}

	// The thread needs high priority to keep the
	// fault buffer from overflowing.
	if (!::SetThreadPriority(hPageFaultThread, THREAD_PRIORITY_TIME_CRITICAL)) {
		return false;
	}

	//
	WriteLine("InitPageFaultMonitor,Success\n");
	return true;
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

bool e_PfProfInit(void)
{
	//
	::InitializeCriticalSection(&PfCs);

	//
	LARGE_INTEGER li;
	::QueryPerformanceFrequency(&li);
	fCounterRate = 1000.0 / li.QuadPart;	// in ms
	::QueryPerformanceCounter(&nStartCounter);

	//
	SYSTEMTIME st;
	::GetLocalTime(&st);
	char FileName[256];
	sprintf(
		FileName,
		"\\Prof\\chb-%04d%02d%02d-%02d%02d%02d.log",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond
	);

	hLogFile = ::CreateFileA(
		FileName,						// LPCTSTR lpFileName
		GENERIC_READ | GENERIC_WRITE,	// DWORD dwDesiredAccess
		FILE_SHARE_READ,				// DWORD dwShareMode
		0,								// LPSECURITY_ATTRIBUTES lpSecurityAttributes
		CREATE_NEW,						// DWORD dwCreationDisposition
		FILE_ATTRIBUTE_NORMAL,			// DWORD dwFlagsAndAttributes
		0								// HANDLE hTemplateFile
	);
	if ((hLogFile == 0) || (hLogFile == INVALID_HANDLE_VALUE)) {
		return false;
	}
//	Write("SerialNo,TimeMS,ThreadId\n");
	Write("SerialNo,TimeMS\n");

	HMODULE hModule = ::LoadLibraryA("ntdll.dll");
	Patch(hModule, "ZwReadFile", (FARPROC)&MyZwReadFile);
	Patch(hModule, "ZwAllocateVirtualMemory", (FARPROC)&MyZwAllocateVirtualMemory);
//	Patch(hModule, "ZwMapViewOfSection", (FARPROC)&MyZwMapViewOfSection);
	::FreeLibrary(hModule);
	
	if (!InitPageFaultMonitor()) {
		return false;
	}

	return true;
}

void e_PfProfDeinit(void)
{
	bShutdown = true;
	while (::WaitForSingleObject(hPageFaultThread, INFINITE) != WAIT_OBJECT_0);

	Summary(nCounts);
	::CloseHandle(hLogFile);

	::DeleteCriticalSection(&PfCs);
}

void e_PfProfBracket(void)
{
	while (ProcessPageFaultData() > 0);
}

void e_PfProfMark_(const char * pFmt, ...)
{
	e_PfProfBracket();

	va_list va;
	va_start(va, pFmt);

	WriteLine("e_PfProfMark,\"");
	char buf[256];
	int nLen = vsprintf(buf, pFmt, va);
	Write(buf, nLen);
	Write("\"\n");
}

//-----------------------------------------------------------------------------

#endif
