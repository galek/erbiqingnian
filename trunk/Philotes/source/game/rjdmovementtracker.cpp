#include "stdafx.h"
#include "rjdmovementtracker.h"

#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
float fDecayRates[] = {.999, .98 };
//----------------------------------------------------------------------------
// Note: ifdef all this stuff out, only included for development and/or server
// version.  Reasoning: it's better for people to not be able to figure out
// our movement hack tolerances.
//----------------------------------------------------------------------------
MOVEMENT_HACK_LEVEL rjdMovementTracker::AddTick(DWORD dwTick)
{
	ASSERT(dwStart <= dwTick);
	ASSERT(dwLast <= dwTick);

	int i;

	if(dwStart == 0 || dwStart > dwTick) //First use.
	{
		memclear(this, sizeof(*this));
		dwStart = dwLast = dwTick;
	}
	if(dwLast == dwTick)
	{
		nMessagesAtCurrentTick++;
		if(nMessagesAtCurrentTick > LAG_BURST_TICK_THRESHHOLD)
			dwLastLagBurst = nMessagesAtCurrentTick;
	}
	else nMessagesAtCurrentTick = 1;
	nTotalMessages++;
	
	for(i = 0; i < DECAY_TRACKERS; i++)
	{
		fMessagesDecayed[i] *= fDecayRates[i];
		fMessagesDecayed[i] += 1.0f;
	}
	for(i = 0; i < DECAY_TRACKERS; i++)
	{
		fTicksDecayed[i] *= fDecayRates[i];
	//	fTicksDecayed[i] += 1.0f;
		fTicksDecayed[i] += (dwTick - dwLast);		
	}

	dwLast = dwTick;

	DWORD ticksElapsed = dwLast - dwStart;
	if(nTotalMessages > 
		(ticksElapsed*TICK_PROPORTIONAL_TOLERANCE + TICK_CONSTANT_TOLERANCE))
		return MOVEMENT_HACK_PROBABLE;

	for(i = 0; i < DECAY_TRACKERS; i++) if(fMessagesDecayed[i] > 
		(ticksElapsed*TICK_PROPORTIONAL_TOLERANCE + TICK_CONSTANT_TOLERANCE))
		return MOVEMENT_HACK_LAGGING;

	return MOVEMENT_HACK_NONE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MOVEMENT_HACK_LEVEL rjdMovementTracker::AddVelocity(
	float fMoveDistance,
	float fVelocityExpected)
{
	int i;
	float fVelocityActual = fMoveDistance * GAME_TICKS_PER_SECOND;
	
	fTotalVelocityActual += fVelocityActual;
	for(i = 0; i < DECAY_TRACKERS; i++)
	{
		fVelocityActualDecayed[i]*=fDecayRates[i];
		fVelocityActualDecayed[i] += fVelocityActual;
	}

	fTotalVelocityExpected += fVelocityExpected;
	for(i = 0; i < DECAY_TRACKERS; i++)
	{
		fVelocityExpectedDecayed[i]*=fDecayRates[i];
		fVelocityExpectedDecayed[i] += fVelocityExpected;
	}

	//if(fTotalVelocityActual > 
	//	(fTotalVelocityExpected*VELOCITY_PROPORTIONAL_TOLERANCE + VELOCITY_CONSTANT_TOLERANCE))
	//	return MOVEMENT_HACK_PROBABLE;
	//Only flag on short term movement spikes, so the effect of 
	//one strange teleport doesn't false positive forever

	for(i = 0; i < DECAY_TRACKERS; i++) if(fVelocityActualDecayed[i] > 
		(fVelocityExpectedDecayed[i]*VELOCITY_PROPORTIONAL_TOLERANCE + VELOCITY_CONSTANT_TOLERANCE))
		return MOVEMENT_HACK_PROBABLE;

	return MOVEMENT_HACK_NONE;
}
#endif //#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MOVEMENT_HACK_LEVEL rjdMovementTracker::AddMovement(
	DWORD dwTick,
	float fMoveDistance,
	float fVelocityExpected)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
	MOVEMENT_HACK_LEVEL tickLevel = AddTick(dwTick);
	MOVEMENT_HACK_LEVEL velocityLevel = AddVelocity(fMoveDistance, fVelocityExpected);

	MOVEMENT_HACK_LEVEL toRet = MAX(tickLevel, velocityLevel);

	if(toRet != MOVEMENT_HACK_NONE) nFlagCount++;
	return toRet;
#else
	UNREFERENCED_PARAMETER(dwTick);
	UNREFERENCED_PARAMETER(fMoveDistance);
	UNREFERENCED_PARAMETER(fVelocityExpected);
	return MOVEMENT_HACK_NONE;
#endif
}
