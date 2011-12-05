//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------


#include "ospathchar.h"

const char * WIDE_CHAR_TO_ASCII_FOR_DEBUG_TRACE_ONLY(
	const wchar_t * pStr)
{
	// lazy pseudo thread-safety w/o using tls
	const int nMaxSimultaneousStrings = 32;	// allow this many at a time
	static char Strings[nMaxSimultaneousStrings][MAX_PATH];
	static volatile long nNextString = 0;

	int nString = InterlockedIncrement(&nNextString) % nMaxSimultaneousStrings;

#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;
	wcstombs_s(&nCharsConverted, Strings[nString], arrsize(Strings[nString]), pStr, _TRUNCATE);
#else
	wcstombs(Strings[nString], pStr, arrsize(Strings[nString]));
#endif

	return Strings[nString];
}
