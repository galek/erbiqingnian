//*****************************************************************************
//
// Helper functions for memory profiling.
//
// This file must be compiled with link-time code generation OFF.
//
//*****************************************************************************

#include "e_pfprof.h"

#ifdef ENABLE_PAGE_FAULT_PROFILING

//-----------------------------------------------------------------------------

#include <windows.h>

__declspec(naked)
unsigned __stdcall DoZwReadFile(HANDLE /*FileHandle*/, HANDLE /*Event*/, void * /*ApcRoutine*/, void * /*ApcContext*/, void * /*IoStatusBlock*/, void * /*Buffer*/, unsigned /*Length*/, LARGE_INTEGER * /*ByteOffset*/, ULONG * /*Key*/)
{
/*
7C90E27C B8 B7 00 00 00   mov         eax,0B7h 
7C90E281 BA 00 03 FE 7F   mov         edx,7FFE0300h 
7C90E286 FF 12            call        dword ptr [edx] 
7C90E288 C2 24 00         ret         24h  
*/
	__asm
	{
		mov eax,0xb7;
		mov edx,0x7FFE0300;
		call [edx]
		ret 36;
	}
}

__declspec(naked)
unsigned __stdcall DoZwMapViewOfSection(HANDLE /*SectionHandle*/, HANDLE /*ProcessHandle*/, void * * /*BaseAddress*/, unsigned * /*ZeroBits*/, unsigned /*CommitSize*/, LARGE_INTEGER * /*SectionOffset*/, unsigned * /*ViewSize*/, int /*InheritDisposition*/, unsigned /*AllocationType*/, unsigned /*Win32Protect*/)
{
/*
7C90DC55 B8 6C 00 00 00   mov         eax,6Ch 
7C90DC5A BA 00 03 FE 7F   mov         edx,7FFE0300h 
7C90DC5F FF 12            call        dword ptr [edx] 
7C90DC61 C2 28 00         ret         28h  
*/
	__asm
	{
		mov eax,0x6c;
		mov edx,0x7FFE0300;
		call [edx]
		ret 40;
	}
}

__declspec(naked)
unsigned __stdcall DoZwAllocateVirtualMemory(HANDLE /*hProcess*/, void * * /*pAddr*/, unsigned /*nZeroBits*/, unsigned * /*pSize*/, unsigned /*nType*/, unsigned /*nProtect*/)
{
/*
7C90D4DE B8 11 00 00 00   mov         eax,11h 
7C90D4E3 BA 00 03 FE 7F   mov         edx,7FFE0300h 
7C90D4E8 FF 12            call        dword ptr [edx] 
7C90D4EA C2 18 00         ret         18h  
*/
	__asm
	{
		mov eax,0x11;
		mov edx,0x7FFE0300;
		call [edx]
		ret 24;
	}
}

//-----------------------------------------------------------------------------

#endif
