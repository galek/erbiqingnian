//-------------------------------------------------------------------------------------------------
// FILE: performance.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------

#include "performance.h"

#include "perfhier.h"
#define INCLUDE_PERFORMANCE_TABLE
#include "perf_hdr.h"
#undef	INCLUDE_PERFORMANCE_TABLE

#ifdef PERFORMANCE_ENABLE_TIMER

#include "winos.h"
#include "ptime.h"

#if ISVERSION(SERVER_VERSION)
#include "performance.cpp.tmh"
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int CTimer::m_nTimerCount = 0;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void CTimer::LogTimer(
	const UINT64 iiMsecs)
{

	// buffer to hold the indent/depth
	#define MAX_INDENT (128)
	char szBuffer[ MAX_INDENT ];

	// this is the sequence we will print for every level of timer depth/count	
	const char *pszIndentSequence = "*---";
	int iIndentSize = PStrLen(pszIndentSequence);

	// put indent sequence in front of log message based on how many times are currently active
	int iDestIndex = 0;
	szBuffer[ 0 ] = 0;	
	for (int i = 0; i < m_nTimerCount; i++)
	{
		if (iDestIndex + iIndentSize < MAX_INDENT)
		{
			PStrCopy( &szBuffer[ iDestIndex ], pszIndentSequence, MAX_INDENT-iDestIndex );
			iDestIndex += iIndentSize;
		}
		szBuffer[ iDestIndex ] = 0;
	}

	// log to file
	LogMessage(TIMER_LOG, "%s%I64u msecs - %s", szBuffer, iiMsecs, m_pszName ? m_pszName : "[unknown]");

}
#endif !ISVERSION(SERVER_VERSION)

//------------------------------------------------------------------------
// Moving this here to allow tracing preprocessor.
//------------------------------------------------------------------------
UINT64 CTimer::End(
		   void)
{
	// remove timer count if started
	if (m_iiStartTime != 0)
	{
		m_nTimerCount--;
	}

	UINT64 iiMsecs = GetTime();
	if (iiMsecs > m_iiThreshold)
	{
		// log standard timer message
		
		TraceVerbose( TRACE_FLAG_GAME_MISC_LOG,	"TIMER: %I64u msecs - %s", iiMsecs, m_pszName ? m_pszName : "[unknown]");

#if !ISVERSION(SERVER_VERSION)
		// log timer log message
		LogTimer(iiMsecs);
#endif // !ISVERSION(SERVER_VERSION)
	}
	m_bHasOutput = TRUE;
	return iiMsecs;
}

#endif // ISVERSION(DEVELOPMENT)