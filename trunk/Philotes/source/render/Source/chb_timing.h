//*****************************************************************************
//
// Profiling helpers
//
//*****************************************************************************

#pragma once

// Uncomment this to enable timing and stats code.
#define CHB_ENABLE_TIMING_AND_STATS 1

//
// Timing.
//

#ifdef CHB_ENABLE_TIMING_AND_STATS

enum CHB_TIMINGS
{
	CHB_TIMING_CPU,
	CHB_TIMING_GPU,
	CHB_TIMING_SYNC,
	CHB_TIMING_FRAME,

	CHB_TIMING_TEST,	// for quick testing without having to touch this header file

	CHB_TIMING_COUNT_	// not a value
};

void CHB_TimingBegin(CHB_TIMINGS nWhich);
void CHB_TimingEnd(CHB_TIMINGS nWhich);
void CHB_TimingEndFrame(void);

float CHB_TimingGetInstantaneous(CHB_TIMINGS nWhich);
unsigned int CHB_TimingGetCycles(CHB_TIMINGS nWhich);

#else

#define CHB_TimingBegin(x)		((void)0)
#define CHB_TimingEnd(x)		((void)0)
#define CHB_TimingEndFrame()	((void)0)

#define CHB_TimingGetInstantaneous(x)	(0.0f)
#define CHB_TimingGetCycles()	(0L)

#endif

//
// Stats.
//

#ifdef CHB_ENABLE_TIMING_AND_STATS

enum CHB_STATS
{
	CHB_STATS_UI_VERTEX_BUFFER_BYTES_UPLOADED,
	CHB_STATS_VERTEX_BUFFER_BYTES_UPLOADED,
	CHB_STATS_INDEX_BUFFER_BYTES_UPLOADED,
	CHB_STATS_TEXTURE_BYTES_UPLOADED,

	CHB_STATS_COUNT_	// not a value
};

void CHB_StatsBeginFrame(void);
void CHB_StatsEndFrame(void);
void CHB_StatsCount(CHB_STATS nWhich, int nValue);
int CHB_StatsGetHits(CHB_STATS nWhich);
int CHB_StatsGetTotal(CHB_STATS nWhich);


#else

#define CHB_StatsBeginFrame()	((void)0)
#define CHB_StatsEndFrame()		((void)0)
#define CHB_StatsCount(a, b)	((void)0)
#define CHB_StatsGetHits(a)		((int)0)
#define CHB_StatsGetTotal(a)	((int)0)
#endif
