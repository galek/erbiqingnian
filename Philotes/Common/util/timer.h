
#pragma once

_NAMESPACE_BEGIN

struct CounterFrequencyToTensOfNanos
{
	uint64 mNumerator;
	uint64 mDenominator;
	CounterFrequencyToTensOfNanos( uint64 inNum, uint64 inDenom )
		: mNumerator( inNum )
		, mDenominator( inDenom )
	{
	}

	//quite slow.
	uint64 toTensOfNanos( uint64 inCounter ) const
	{
		return ( inCounter * mNumerator ) / mDenominator;
	}
};

//////////////////////////////////////////////////////////////////////////

class Timer
{
public:
	typedef double Second;

	static const uint64 sNumTensOfNanoSecondsInASecond = 100000000;

	//This is supposedly guaranteed to not change after system boot
	//regardless of processors, speedstep, etc.
	static const CounterFrequencyToTensOfNanos sCounterFreq;

	static CounterFrequencyToTensOfNanos getCounterFrequency();

	static uint64 getCurrentCounterValue();

	//SLOW!!
	//Thar be a 64 bit divide in thar!
	static uint64 getCurrentTimeInTensOfNanoSeconds()
	{
		uint64 ticks = getCurrentCounterValue();
		return sCounterFreq.toTensOfNanos( ticks );
	}

	Timer();
	
	Second getElapsedSeconds();
	
	Second peekElapsedSeconds();
	
	Second getLastTime() const 
	{
		return mLastTime; 
	}

private:

	Second  mLastTime;
};

_NAMESPACE_END