
#include "timer.h"

_NAMESPACE_BEGIN

namespace
{
	double getTimeTicks()
	{
		LARGE_INTEGER a;
		QueryPerformanceCounter (&a);
		return double(a.QuadPart);
	}

	double getTickDuration()
	{
		LARGE_INTEGER a;
		QueryPerformanceFrequency (&a);
		return 1.0f / double(a.QuadPart);
	}

	double sTickDuration = getTickDuration();
}

const CounterFrequencyToTensOfNanos Timer::sCounterFreq = Timer::getCounterFrequency();

CounterFrequencyToTensOfNanos Timer::getCounterFrequency()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency (&freq);
	return CounterFrequencyToTensOfNanos( Timer::sNumTensOfNanoSecondsInASecond, freq.QuadPart );
}


uint64 Timer::getCurrentCounterValue()
{
	LARGE_INTEGER ticks;
	QueryPerformanceCounter (&ticks);
	return ticks.QuadPart;
}

Timer::Timer(): mLastTime(0)
{
	getElapsedSeconds();
}

Timer::Second Timer::getElapsedSeconds()
{
	Second lastTime = mLastTime;
	mLastTime = getTimeTicks() * sTickDuration;
	return mLastTime - lastTime;
}

Timer::Second Timer::peekElapsedSeconds()
{
	return (getTimeTicks() * sTickDuration) - mLastTime;
}

_NAMESPACE_END