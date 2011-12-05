//----------------------------------------------------------------------------
// perf_hier.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PERFORMANCE_H_
#define _PERFORMANCE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _WIN_OS_H_
#include "winos.h"
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define DEFAULT_PERF_THRESHOLD_SMALL	10
#define DEFAULT_PERF_THRESHOLD_LARGE	15


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//#if ISVERSION(DEVELOPMENT) || ISVERSION(CHEATS_ENABLED)
#define PERFORMANCE_ENABLE_TIMER 1
//#endif

#if ISVERSION(SERVER_VERSION)
#define ENABLE_ALL_PERF_TIMERS
#endif

#if defined(PERFORMANCE_ENABLE_TIMER) && defined(ENABLE_ALL_PERF_TIMERS)
#define TIMER_START(name)						CTimer tTimer(name); tTimer.Start();
#define TIMER_STARTEX(name, thresh)				CTimer tTimer(name, thresh); tTimer.Start();
#define TIMER_END()								tTimer.End()
#else
#define TIMER_START(name)				
#define TIMER_STARTEX(name, thresh)		
#define TIMER_END()								((UINT64)0L)
#endif

#ifdef PERFORMANCE_ENABLE_TIMER
#define TIMER_START2(name)						CTimer tTimer(name); tTimer.Start();
#define TIMER_STARTEX2(name, thresh)			CTimer tTimer(name, thresh); tTimer.Start();
#define TIMER_END2()							tTimer.End()
#else
#define TIMER_START2(name)				
#define TIMER_STARTEX2(name, thresh)		
#define TIMER_END2()							((UINT64)0L)
#endif

#ifdef PERFORMANCE_ENABLE_TIMER
#define TIMER_START3(fmt, ...)					char UIDEN(SZ_TIMER_)[256]; PStrPrintf(UIDEN(SZ_TIMER_), arrsize(UIDEN(SZ_TIMER_)), fmt, __VA_ARGS__); CTimer UIDEN(TIMER_)(UIDEN(SZ_TIMER_)); UIDEN(TIMER_).Start();
#define TIMER_STARTEX3(thresh, fmt, ...)		char UIDEN(SZ_TIMER_)[256]; PStrPrintf(UIDEN(SZ_TIMER_), arrsize(UIDEN(SZ_TIMER_)), fmt, __VA_ARGS__); CTimer UIDEN(TIMER_)(UIDEN(SZ_TIMER_), thresh); UIDEN(TIMER_).Start();
#else
#define TIMER_START3(fmt, ...)					
#define TIMER_STARTEX3(thresh, fmt, ...)		
#endif

#ifdef PERFORMANCE_ENABLE_TIMER
#define TIMER_START_NEEDED(name)				CTimer tTimer(name); tTimer.Start();
#define TIMER_START_NEEDED_EX(name, thresh)		CTimer tTimer(name, thresh); tTimer.Start();
#define TIMER_END_NEEDED()						tTimer.End()
#define TIMER_GET_TIME()						tTimer.GetTime()
#else
#define TIMER_START_NEEDED(name)
#define TIMER_START_NEEDED_EX(name, thresh)
#define TIMER_END_NEEDED()						((UINT64)0L)
#define TIMER_GET_TIME()						((UINT64)0L)
#endif


//----------------------------------------------------------------------------
class CTimer
{
protected:
	static const UINT64							DEFAULT_TIMER_THRESHOLD = 0;

	const char *								m_pszName;
	UINT64										m_iiStartTime;
	UINT64										m_iiThreshold;
	BOOL										m_bHasOutput;
	static int									m_nTimerCount;

	void LogTimer(
		const UINT64 iiMsecs);
	
public:
	//------------------------------------------------------------------------
	CTimer(
		void) : m_pszName(NULL), m_iiStartTime(0), m_iiThreshold(DEFAULT_TIMER_THRESHOLD), m_bHasOutput(FALSE)
	{
	}

	//------------------------------------------------------------------------
	CTimer(
		const char * pszName) : m_pszName(pszName), m_iiStartTime(0), m_iiThreshold(DEFAULT_TIMER_THRESHOLD), m_bHasOutput(FALSE)
	{
	}

	//------------------------------------------------------------------------
	CTimer(
		const char * pszName,
		UINT64 iiThreshold) : m_pszName(pszName), m_iiStartTime(0), m_iiThreshold(iiThreshold), m_bHasOutput(FALSE)
	{
	}

	//------------------------------------------------------------------------
	~CTimer(
		void)
	{
		if (!m_bHasOutput)
		{
			End();
		}
	}

	//------------------------------------------------------------------------
	void Start(
		void)
	{
		m_iiStartTime = PGetPerformanceCounter();
		m_nTimerCount++;
	}

	//------------------------------------------------------------------------
	UINT64 End(
		void);

	//------------------------------------------------------------------------
	UINT64 GetTime(
		void)
	{
		return ((PGetPerformanceCounter() - m_iiStartTime) * MSECS_PER_SEC) / PGetPerformanceFrequency();
	}
};


#endif // _PERFORMANCE_H_