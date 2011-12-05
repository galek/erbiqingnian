//----------------------------------------------------------------------------
// perfhier.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PERFHIER_H_
#define _PERFHIER_H_

#include "winos.h"
#include "perf_hdr.h"

#ifdef _DEBUG
#define PERF_ACTIVE
#endif

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct PERF_TABLE
{
	UINT64			total;
	UINT64			start;
	UINT64			display;
	const char*		label;
	DWORD			hits;
	DWORD			hitdisplay;
	BOOL			expand;
	BOOL			reset;
	BOOL			drawn;
	int				spaces;
};


struct HITCOUNT_TABLE
{
	DWORD			hits;
	DWORD			display;
	const char*		label;
	BOOL			expand;
	BOOL			drawn;
	int				spaces;
};


//----------------------------------------------------------------------------
// EXTERN
//----------------------------------------------------------------------------
extern PERF_TABLE		gPerfTable[];
extern __int64			gqwPerfFrames;
extern __int64			gqwPerfFramesDisp;
extern int				gnPerfWindow;

extern HITCOUNT_TABLE	gHitCountTable[];
extern DWORD			gdwHitCountFrames;
extern DWORD			gdwHitCountFramesDisp;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class PERF_CLASS
{
	int index;
public:
	PERF_CLASS(
		int i) : index(i)
	{
		if ( gPerfTable[ index ].drawn )
		{
			gPerfTable[index].start = PGetPerformanceCounter();
			gPerfTable[index].reset = FALSE;
			gPerfTable[index].hits++;
		}
	}
	~PERF_CLASS(
		void)
	{
		if ( gPerfTable[ index ].drawn )
		{
			gPerfTable[index].total += (PGetPerformanceCounter() - gPerfTable[index].start);
		}
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef PERF_ACTIVE
#define PERF_START(x)		{ if ( gPerfTable[PERF_##x].drawn ) { gPerfTable[PERF_##x].start = PGetPerformanceCounter(); gPerfTable[PERF_##x].reset = FALSE; gPerfTable[PERF_##x].hits++; } }
#define PERF_END(x)			{ if ( gPerfTable[PERF_##x].drawn ) { if (!gPerfTable[PERF_##x].reset) { gPerfTable[PERF_##x].total += (PGetPerformanceCounter() - gPerfTable[PERF_##x].start); } } }
#define PERF(x)				PERF_CLASS UIDEN(perf)(PERF_##x);
#define PERF_INC()			{ gqwPerfFrames++; }

#define HITCOUNT(x)			{ gHitCountTable[HITCOUNT_##x].hits++; }
#define HITCOUNT_INC()		{ gdwHitCountFrames++; }

#else
#define PERF_START(x)
#define PERF_END(x)
#define PERF(x)
#define PERF_INC()

#define HITCOUNT(x)
#define HITCOUNT_INC()

#endif


#endif // _PERFHIER_H_