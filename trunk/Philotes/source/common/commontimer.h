//----------------------------------------------------------------------------
// apptimer.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _COMMONTIMER_H_
#define _COMMONTIMER_H_

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct COMMON_TIMER_PUBLIC
{
	TIME					tiAppStartTime;		// app start time in absolute time, in milliseconds
	TIME					tiAppCurTime;		// current time = number of milliseconds elapsed since start of app (not including paused time)
	TIME					tiAppPauseTime;		// amount of time paused

	TIME					tiAppLastSimTime;	// last iiCurTime we ran simulations
	float					fAppElapsedTime;	// delta in seconds between calls to CommonTimerElapse()
	float					fAppElapsedTickTime;// seconds between iiCurTime and iiAppLastSimTime

	DWORD					dwTickCount;		// real time in msecs

	BOOL					bPaused;			// paused due to internal lag
	BOOL					bUserPause;			// deliberate pause
	BOOL					bSingleStep;		// single step
	int						nStepSpeed;

	// simulation framerate counter
	float					fSimFrameRate;

	// display framerate counter
	float					fDrawFrameRate;
	DWORD					nDrawLongestFrameMs;
};

struct COMMON_TIMER;


//----------------------------------------------------------------------------
// EXPORTS
//----------------------------------------------------------------------------
BOOL CommonTimerInit(
	COMMON_TIMER*& timer,
	DWORD cpu_num = 0);

void CommonTimerFree(
	COMMON_TIMER* timer);

void CommonTimerUpdate(
	COMMON_TIMER* timer);

void CommonTimerElapse(
	COMMON_TIMER* timer);

unsigned int CommonTimerSimulationUpdate(
	COMMON_TIMER* timer, bool sleepToNextFrame);

void CommonTimerDisplayFramerateUpdate(
	COMMON_TIMER* timer);

int CommonTimerSetStepSpeed(
	COMMON_TIMER* timer,
	int nStepSpeed);

void CommonTimerSetPause(
	COMMON_TIMER* timer,
	BOOL pause);

const WCHAR* CommonTimerGetStepSpeedString(
	COMMON_TIMER* timer);

COMMON_TIMER_PUBLIC * CommonTimerGetPublic(
	COMMON_TIMER* timer);

long GetRealClockTime();
//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------
inline BOOL CommonTimerGetPause(
	COMMON_TIMER* timer)
{
	ASSERT_RETFALSE(timer);
	return CommonTimerGetPublic(timer)->bUserPause;
}

inline BOOL CommonTimerGetSingleStep(
	COMMON_TIMER* timer)
{
	ASSERT_RETFALSE(timer);
	return CommonTimerGetPublic(timer)->bSingleStep;
}

inline void CommonTimerSingleStep(
	COMMON_TIMER* timer)
{
	ASSERT_RETURN(timer);
	CommonTimerGetPublic(timer)->bUserPause = TRUE;
	CommonTimerGetPublic(timer)->bSingleStep = TRUE;
}


inline int CommonTimerGetStepSpeed(
	COMMON_TIMER* timer)
{
	ASSERT_RETZERO(timer);
	return CommonTimerGetPublic(timer)->nStepSpeed;
}


#endif // _PTIME_H_

