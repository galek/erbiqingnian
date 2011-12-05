//*****************************************************************************
//
// Profiling helpers
//
//*****************************************************************************

#include "e_pch.h"
#include "chb_timing.h"

#ifdef CHB_ENABLE_TIMING_AND_STATS

//-----------------------------------------------------------------------------

struct CHBPerfFreq
{
	__int64 nFreq;

	__int64 Get(void) const { return nFreq; }

	CHBPerfFreq(void);
};

CHBPerfFreq::CHBPerfFreq(void)
{
	LARGE_INTEGER i;
	::QueryPerformanceFrequency(&i);
	nFreq = i.QuadPart;
}

static CHBPerfFreq Freq;

//-----------------------------------------------------------------------------

struct CHBTiming
{
	__int64 nStart;
	__int64 nAccum;

	unsigned int nBegins;

	static const int nHistoryDepth = 11;
	float fSeconds[nHistoryDepth];
	int nHistoryIndex;

	float GetInstantaneous(void) const;
	unsigned int GetCycles(void);
	void Begin(void);
	void End(void);
	void Final(void);
	void Reset(void);

	CHBTiming(void);
};

static CHBTiming Timings[CHB_TIMING_COUNT_];

//-----------------------------------------------------------------------------

float CHBTiming::GetInstantaneous(void) const
{
	int i = (nHistoryIndex + nHistoryDepth - 1) % nHistoryDepth;
	return fSeconds[i];
}

void CHBTiming::Begin(void)
{
	LARGE_INTEGER i;
	::QueryPerformanceCounter(&i);
	nStart = i.QuadPart;
	nBegins++;
}

void CHBTiming::End(void)
{
	LARGE_INTEGER i;
	::QueryPerformanceCounter(&i);
	nAccum += i.QuadPart - nStart;
}

void CHBTiming::Final(void)
{
	fSeconds[nHistoryIndex] = static_cast<float>(static_cast<double>(nAccum) / Freq.Get());
	nHistoryIndex = (nHistoryIndex + 1) % nHistoryDepth;
}

unsigned int CHBTiming::GetCycles(void)
{
	return nBegins;
}

void CHBTiming::Reset(void)
{
	nAccum = 0;
}

CHBTiming::CHBTiming(void)
{
	Reset();
	nHistoryIndex = 0;
	nBegins = 0;
}

//-----------------------------------------------------------------------------

void CHB_TimingBegin(CHB_TIMINGS nWhich)
{
	Timings[nWhich].Begin();
}

void CHB_TimingEnd(CHB_TIMINGS nWhich)
{
	Timings[nWhich].End();
}

void CHB_TimingEndFrame(void)
{
	for (int i = 0; i < CHB_TIMING_COUNT_; ++i) {
		Timings[i].Final();
		Timings[i].Reset();
	}
}

float CHB_TimingGetInstantaneous(CHB_TIMINGS nWhich)
{
	return Timings[nWhich].GetInstantaneous();
}

unsigned int CHB_TimingGetCycles(CHB_TIMINGS nWhich)
{
	return Timings[nWhich].GetCycles();
}

//-----------------------------------------------------------------------------

struct CHBStats
{
	int nAccum;
	int nHits;
	int nMin;
	int nMax;

	static const int nHistoryDepth = 11;
	int nCountHistory[nHistoryDepth];
	int nHitsHistory[nHistoryDepth];
	int nHistoryIndex;
	int nLastFrameIndex;

	void Count(int nValue);
	void Final(void);
	void Reset(void);

	CHBStats(void);
};

static CHBStats Stats[CHB_STATS_COUNT_];

//-----------------------------------------------------------------------------

void CHBStats::Count(int nValue)
{
	nAccum += nValue;
	nHits++;
}

void CHBStats::Final(void)
{
	nCountHistory[nHistoryIndex] = nAccum;
	nHitsHistory[nHistoryIndex] = nHits;
	nLastFrameIndex = nHistoryIndex;
	nHistoryIndex = (nHistoryIndex + 1) % nHistoryDepth;
	nMin = min(nAccum, nMin);
	nMax = max(nAccum, nMax);
}

void CHBStats::Reset(void)
{
	nAccum = 0;
	nHits = 0;
}

CHBStats::CHBStats(void)
{
	nHistoryIndex = 0;
	nLastFrameIndex = 0;
	nMin = 0;
	nMax = 0;
	Reset();
}

//-----------------------------------------------------------------------------

void CHB_StatsBeginFrame(void)
{
}

void CHB_StatsEndFrame(void)
{
	for (int i = 0; i < CHB_STATS_COUNT_; ++i) {
		Stats[i].Final();
		Stats[i].Reset();
	}
}

void CHB_StatsCount(CHB_STATS nWhich, int nValue)
{
	Stats[nWhich].Count(nValue);
}

int CHB_StatsGetHits(CHB_STATS nWhich)
{
	return Stats[nWhich].nHitsHistory[Stats[nWhich].nLastFrameIndex];
}

int CHB_StatsGetTotal(CHB_STATS nWhich)
{
	return Stats[nWhich].nCountHistory[Stats[nWhich].nLastFrameIndex];
}

//-----------------------------------------------------------------------------

#endif
