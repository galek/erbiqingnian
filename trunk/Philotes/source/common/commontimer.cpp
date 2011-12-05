//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "commontimer.h"
#include "ptime.h"

#include "winos.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define APP_PAUSE_THRESHOLD_MSECS						(1 * MSECS_PER_SEC)


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum APP_STEP_SPEED
{
	APP_STEP_SPEED_FULL,
	APP_STEP_SPEED_ONE,
	APP_STEP_SPEED_TWO,
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct COMMON_TIMER
{
	UINT64					iiPerfTicksPerSec;	// perf counter ticks per second
	UINT64					iiPerfTicksPerMsec;	// perf counter ticks per msec (note there may be a remainder, so not precise)
	UINT64					iiPerfCounterLast;	// last perf counter value since call to CommonTimerUpdate()
	UINT64					iiPerfCounterRem;	// perf counter remainder from time elapsed calculation

	TIME					tiAppElapsedTime;	// last iiCurTime we called CommonTimerElapse()

	COMMON_TIMER_PUBLIC		tPublic;			// Stuff referenced by inlines

	// simulation framerate counter
	TIME					tiSimCounterStart;	// last AppGetAbsTime() we zeroed the sim counter
	unsigned int			nSimCounter;

	// display framerate counter
	TIME					tiDrawCounterStart;		// last AppGetAbsTime() we zeroed the draw counter
	TIME					tiDrawCounterLast;		// last AppGetAbsTime() we updated the draw counter
	TIME					tiDrawCounterLongest;	// longest duration frame since we updated the draw counter
	unsigned int			nDrawCounter;
};

struct APP_STEP_SPEED_DEF
{
	int m_nFrameRateDivisor;
	const WCHAR* m_wszString;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const APP_STEP_SPEED_DEF gStepSpeedDef[] =
{
	{  1,	L"20 fps (full speed)"	},
	{  2,	L"10 fps (half speed)"	},
	{  4,	L"5 fps (1/4 speed)"	},
};

void (*pTimestampResetFunc)(void) = NULL; 


//----------------------------------------------------------------------------
// DEFINE
// Somewhat "bad."  We don't want to call the functions which get the global
// timer instead of work on our local copy of it, thus we re-define all
// these functions to be a direct lookup.  This is functionality duplication.
// TODO: refactor defines so both the appcommontimer and these depend upon
// a unified source.
//----------------------------------------------------------------------------
#define AppCommonGetCurTime() timer->tPublic.tiAppCurTime
#define AppCommonGetPauseTime() timer->tPublic.tiAppPauseTime
#define AppCommonGetAbsTime() (AppCommonGetCurTime() + AppCommonGetPauseTime() )
#define AppCommonGetLastSimTime() timer->tPublic.tiAppLastSimTime

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CommonTimerInit(
	COMMON_TIMER*& timer,
	DWORD cpu_num)
{
	timer = (COMMON_TIMER*)MALLOCZ(NULL, sizeof(COMMON_TIMER));

	// for the time being, until we do something to put timers in separate thread, this
	// is to make GetPerformanceFrequency consistent
	{
		HANDLE hThread = GetCurrentThread();
//		DWORD dwThreadAffinityMask = 1;
		if (!SetThreadAffinityMask(hThread, 1 << cpu_num)) {
			trace("SetThreadAffinityMask() - error %d\n", GetLastError());
			return FALSE;
		}
	}

#ifdef TIME64
	timer->tPublic.tiAppStartTime = PGetSystemTimeIn100NanoSecs();
#else
	timer->tPublic.tiAppStartTime = GetTickCount();
#endif

	timer->iiPerfTicksPerSec = PGetPerformanceFrequency();
	if (timer->iiPerfTicksPerSec <= MSECS_PER_SEC)
	{
		return FALSE;
	}
	timer->iiPerfCounterLast = PGetPerformanceCounter();
	timer->iiPerfTicksPerMsec = timer->iiPerfTicksPerSec / MSECS_PER_SEC;
	timer->tPublic.dwTickCount = (DWORD)timer->tPublic.tiAppStartTime;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerFree(
	COMMON_TIMER* timer)
{
	ASSERT(timer);
	if (timer)
	{
		FREE(NULL, timer);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerUpdate(
	COMMON_TIMER * timer)
{
	ASSERT_RETURN(timer);
	UINT64 curtick = PGetPerformanceCounter();
	// following is necessary due to inconsistencies in QueryPerformanceCounter
	if (curtick < timer->iiPerfCounterLast)
	{
		curtick = timer->iiPerfCounterLast;
	}
	UINT64 elapsedticks = curtick - timer->iiPerfCounterLast + timer->iiPerfCounterRem;

	TIME elapsedmsec = (TIME)(elapsedticks * MSECS_PER_SEC / timer->iiPerfTicksPerSec);
	timer->iiPerfCounterRem = elapsedticks - (TIME)(elapsedmsec * timer->iiPerfTicksPerSec / MSECS_PER_SEC);
	timer->iiPerfCounterLast = curtick;
	timer->tPublic.dwTickCount += (DWORD)elapsedmsec;

	if (timer->tPublic.bUserPause)
	{
		if (timer->tPublic.bSingleStep)
		{
			timer->tPublic.tiAppPauseTime += elapsedmsec - MSECS_PER_GAME_TICK;
			timer->tPublic.tiAppCurTime += MSECS_PER_GAME_TICK;
			timer->tPublic.bSingleStep = FALSE;
			
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION) && !ISVERSION(CLIENT_ONLY_VERSION)
			// KCK: For single player servers, reset our last frame timestamp so the server also only processes one frame
			if (pTimestampResetFunc)
				pTimestampResetFunc();
#endif
		}

		else
		{
			timer->tPublic.tiAppPauseTime += elapsedmsec;
		}
	}
	else if (timer->tPublic.nStepSpeed != APP_STEP_SPEED_FULL)
	{
		if (elapsedmsec > APP_PAUSE_THRESHOLD_MSECS)
		{
			timer->tPublic.tiAppPauseTime += elapsedmsec - MSECS_PER_GAME_TICK;
			timer->tPublic.tiAppCurTime += MSECS_PER_GAME_TICK;
		}
		else
		{
			TIME slow_elapsed = elapsedmsec / gStepSpeedDef[timer->tPublic.nStepSpeed].m_nFrameRateDivisor;
			TIME pause_time = elapsedmsec - slow_elapsed;
			timer->tPublic.tiAppPauseTime += pause_time;
			timer->tPublic.tiAppCurTime += slow_elapsed;
		}
	}
	else
	{
		if (elapsedmsec > APP_PAUSE_THRESHOLD_MSECS)
		{
			timer->tPublic.tiAppPauseTime += elapsedmsec - MSECS_PER_GAME_TICK;
			timer->tPublic.tiAppCurTime += MSECS_PER_GAME_TICK;
		}
		else
		{
			timer->tPublic.tiAppCurTime += elapsedmsec;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerElapse(
	COMMON_TIMER * timer)
{
	ASSERT_RETURN(timer);
	int elapsed_delta = 0;
	int elapsed_tick_delta = 0;

	elapsed_delta = (int)(AppCommonGetCurTime() - timer->tiAppElapsedTime);
	elapsed_tick_delta = (int)(AppCommonGetCurTime() - AppCommonGetLastSimTime());

	timer->tPublic.fAppElapsedTime = float(double(elapsed_delta) / double(MSECS_PER_SEC));
	timer->tPublic.fAppElapsedTickTime = float(double(elapsed_tick_delta) / double(MSECS_PER_SEC));
	timer->tiAppElapsedTime = AppCommonGetCurTime();

	//if ( AppGetCltGame())
	//{
	//	trace("Elapsed %.3f tiElapsed %I64u Elasped Tick %.3f Curr Time %I64u Last Sim %I64u Game Tick %d\n",
	//		timer->fAppElapsedTime, 
	//		timer->tiAppElapsedTime,
	//		timer->fAppElapsedTickTime, 
	//		AppGetCurTime(),
	//		AppGetLastSimTime(),
	//		GameGetTick(AppGetCltGame()) );
	//}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerSimulationFramerateUpdate(
	COMMON_TIMER* timer,
	unsigned int sim_frames)
{
	ASSERT_RETURN(timer);
	timer->nSimCounter += sim_frames;
	if (AppCommonGetAbsTime() - timer->tiSimCounterStart >= MSECS_PER_SEC)
	{
		timer->tPublic.fSimFrameRate = float(double(timer->nSimCounter * 1000) / double(AppCommonGetAbsTime() - timer->tiSimCounterStart));
		timer->tiSimCounterStart = AppCommonGetAbsTime();
		timer->nSimCounter = 0;
	}
}


//----------------------------------------------------------------------------
// return # of simulation frames to run
//----------------------------------------------------------------------------
unsigned int CommonTimerSimulationUpdate(
	COMMON_TIMER* timer, bool sleepToNextFrame)
{
	REF( sleepToNextFrame );
	ASSERT_RETZERO(timer);
	CommonTimerUpdate(timer);

#if ISVERSION(SERVER_VERSION)
    if (sleepToNextFrame && (AppCommonGetCurTime() - AppCommonGetLastSimTime()) < MSECS_PER_GAME_TICK)
    {
        Sleep((MSECS_PER_GAME_TICK - (DWORD)(AppCommonGetCurTime()- AppCommonGetLastSimTime()))/2);
    }   
#else
	REF( sleepToNextFrame );
#endif

	TIME lastsimtick = AppCommonGetLastSimTime() / MSECS_PER_GAME_TICK;
	TIME cursimtick =  AppCommonGetCurTime() / MSECS_PER_GAME_TICK;

	if (cursimtick < lastsimtick)
	{
		return 0;
	}

	unsigned int sim_frames = (unsigned int)(cursimtick - lastsimtick);
	if (sim_frames > 0)
	{
		timer->tPublic.tiAppLastSimTime = AppCommonGetCurTime();
	}

	CommonTimerSimulationFramerateUpdate(timer, sim_frames);

	return sim_frames;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerDisplayFramerateUpdate(
	COMMON_TIMER* timer)
{
	ASSERT_RETURN(timer);
	timer->nDrawCounter++;
	TIME tCurrent = AppCommonGetAbsTime();

	// track the longest frame this period
	TIME tLastFrame = tCurrent - timer->tiDrawCounterLast;
	if ( tLastFrame > timer->tiDrawCounterLongest )
		timer->tiDrawCounterLongest = tLastFrame;
	timer->tiDrawCounterLast = tCurrent;

	if (tCurrent - timer->tiDrawCounterStart >= MSECS_PER_SEC)
	{
		timer->tPublic.fDrawFrameRate = float(double(timer->nDrawCounter * 1000) / double(tCurrent - timer->tiDrawCounterStart));
		timer->tPublic.nDrawLongestFrameMs = (unsigned)timer->tiDrawCounterLongest;
		timer->tiDrawCounterStart = AppCommonGetAbsTime();
		timer->tiDrawCounterLast = timer->tiDrawCounterStart;
		timer->tiDrawCounterLongest = 0;
		timer->nDrawCounter = 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR* CommonTimerGetStepSpeedString(
	COMMON_TIMER* timer)
{
	ASSERT_RETVAL(timer, L"");
	ASSERT_RETVAL(timer->tPublic.nStepSpeed >= APP_STEP_SPEED_FULL && timer->tPublic.nStepSpeed <= APP_STEP_SPEED_TWO,L"");
	timer->tPublic.nStepSpeed = PIN(timer->tPublic.nStepSpeed, (int)APP_STEP_SPEED_FULL, (int)APP_STEP_SPEED_TWO);
	ASSERT_RETVAL(timer->tPublic.nStepSpeed < arrsize(gStepSpeedDef),L"");
	return gStepSpeedDef[timer->tPublic.nStepSpeed].m_wszString;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonTimerSetPause(
	COMMON_TIMER* timer,
	BOOL pause)
{
	ASSERT_RETURN(timer);
	timer->tPublic.bUserPause = pause;
	if (!pause)
	{
		timer->tPublic.bSingleStep = FALSE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CommonTimerSetStepSpeed(
	COMMON_TIMER* timer,
	int nStepSpeed)
{
	ASSERT_RETZERO(timer);
	nStepSpeed = PIN(nStepSpeed, (int)APP_STEP_SPEED_FULL, (int)APP_STEP_SPEED_TWO);
	timer->tPublic.nStepSpeed = nStepSpeed;
	CommonTimerSetPause(timer, FALSE);
	return nStepSpeed;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
COMMON_TIMER_PUBLIC * CommonTimerGetPublic(
	COMMON_TIMER* timer)
{
	ASSERT_RETNULL(timer);
	return &(timer->tPublic);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long GetRealClockTime()
{
	return clock();
}
