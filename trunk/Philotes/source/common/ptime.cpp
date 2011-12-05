//----------------------------------------------------------------------------
// FILE: ptime.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "ptime.h"
#include "pstring.h"
#include "time.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TimeTMToString(
	const tm *pT,
	WCHAR *puszBuffer,
	int nMaxBuffer)
{
	ASSERT_RETURN( pT );
	ASSERT_RETURN( puszBuffer );
	ASSERT_RETURN( nMaxBuffer );
	
	SYSTEMTIME tSystemTime;
	TimeTMToSystemTime( pT, &tSystemTime );
	
	PStrPrintf(
		puszBuffer,
		nMaxBuffer,
		L"%d/%d/%d %d:%d:%d",
		tSystemTime.wYear,
		tSystemTime.wMonth,
		tSystemTime.wDay,
		tSystemTime.wHour,
		tSystemTime.wMinute,
		tSystemTime.wSecond);
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TimeTMToSystemTime(
	const tm *pT,
	SYSTEMTIME *pSystemTime)
{
	ASSERT_RETURN( pT );
	ASSERT_RETURN( pSystemTime );
	
	pSystemTime->wYear = static_cast<WORD>(pT->tm_year + 1900);
	pSystemTime->wMonth = static_cast<WORD>(pT->tm_mon + 1);
	pSystemTime->wDay = static_cast<WORD>(pT->tm_mday);
	pSystemTime->wHour = static_cast<WORD>(pT->tm_hour);
	pSystemTime->wMinute = static_cast<WORD>(pT->tm_min);
	pSystemTime->wSecond = static_cast<WORD>(pT->tm_sec);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UTCTimeAddDay(
	time_t *pTime,
	int nNumDays)
{
	ASSERTX_RETURN( pTime, "Expected time struct" );
	const int NUM_SECONDS_PER_DAY = 86400;
	
	// add/subtract seconds from the time for the number of days requested
	*pTime = *pTime + (NUM_SECONDS_PER_DAY * nNumDays );
	
}
