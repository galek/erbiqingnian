//----------------------------------------------------------------------------
// appcommontimer.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _APPCOMMONTIMER_H_
#define _APPCOMMONTIMER_H_

#include "appcommon.h"
#include "commontimer.h"

//----------------------------------------------------------------------------
inline TIME AppCommonGetStartTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->tiAppStartTime;
}

//----------------------------------------------------------------------------
inline TIME AppCommonGetTrueTime(
	void)
{
	COMMON_TIMER_PUBLIC * timer = CommonTimerGetPublic(AppCommonGetTimer());
	ASSERT_RETVAL(timer, TIME_ZERO);
	return timer->tiAppStartTime + timer->tiAppCurTime + timer->tiAppPauseTime;
}

//----------------------------------------------------------------------------
inline TIME AppCommonGetCurTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->tiAppCurTime;
}

//----------------------------------------------------------------------------
inline TIME AppCommonGetPauseTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->tiAppPauseTime;
}

//----------------------------------------------------------------------------
inline TIME AppCommonGetAbsTime(
	void)
{
	return AppCommonGetCurTime() + AppCommonGetPauseTime();
}

//----------------------------------------------------------------------------
// this wraps around after 49 days
inline DWORD AppCommonGetRealTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->dwTickCount;
}


//----------------------------------------------------------------------------
inline TIME AppCommonGetLastSimTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->tiAppLastSimTime;
}

//----------------------------------------------------------------------------
inline float AppCommonGetSimFrameRate(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->fSimFrameRate;
}

//----------------------------------------------------------------------------
inline float AppCommonGetDrawFrameRate(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->fDrawFrameRate;
}

//----------------------------------------------------------------------------
inline DWORD AppCommonGetDrawLongestFrameMs(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->nDrawLongestFrameMs;
}

//----------------------------------------------------------------------------
inline float AppCommonGetElapsedTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->fAppElapsedTime;
}

//----------------------------------------------------------------------------
inline float AppCommonGetElapsedTickTime(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->fAppElapsedTickTime;
}

//----------------------------------------------------------------------------
inline void AppCommonSetPause(
	BOOL bPause)
{
	CommonTimerGetPublic(AppCommonGetTimer())->bPaused = bPause;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonIsPaused(
	void)
{
	return CommonTimerGetPublic(AppCommonGetTimer())->bPaused;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetUserPause(
	void)
{
	return CommonTimerGetPause(AppCommonGetTimer());
}

//----------------------------------------------------------------------------
inline BOOL AppCommonIsAnythingPaused(
	void)
{
	return AppCommonIsPaused() 
		|| ( AppCommonGetUserPause() && ! CommonTimerGetSingleStep( AppCommonGetTimer() ) );
}

//----------------------------------------------------------------------------
inline int AppCommonGetStepSpeed(
	void)
{
	return CommonTimerGetStepSpeed(AppCommonGetTimer());
}

//----------------------------------------------------------------------------
inline int AppCommonSetStepSpeed(
	int nStepSpeed)
{
	return CommonTimerSetStepSpeed(AppCommonGetTimer(), nStepSpeed);
}

//----------------------------------------------------------------------------
inline const WCHAR* AppCommonGetStepSpeedString(
	void)
{
	return CommonTimerGetStepSpeedString(AppCommonGetTimer());
}

#endif
