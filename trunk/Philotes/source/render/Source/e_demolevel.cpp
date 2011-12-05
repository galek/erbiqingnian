//----------------------------------------------------------------------------
// e_demolevel.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "chb_timing.h"
#include "e_main.h"
#include "e_demolevel.h"


struct FSSE::DEMO_LEVEL_STATUS gtDemoLevelStatus;



namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static const char gszDemoLevelStates[ DEMO_LEVEL_STATUS::NUM_STATES ][ 16 ] =
{
	"INACTIVE",
	"INIT",
	"PRE-COUNTDOWN",
	"COUNTDOWN",
	"READY-TO-RUN",
	"RUNNING",
	"SHOW_STATS",
};

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DEMO_LEVEL_STATUS::Init()
{
	eState = INIT;
}

void DEMO_LEVEL_STATUS::Reset()
{
	tStats.dwTotalFrames	= 0;

	tStats.dwTotalMS		= 0;
	tStats.dwTotalCPUMS		= 0;
	tStats.dwTotalGPUMS		= 0;

	tStats.dwMinFrameMS		= DWORD(-1);
	tStats.dwMinFrameCPUMS	= DWORD(-1);
	tStats.dwMinFrameGPUMS	= DWORD(-1);
	tStats.dwMaxFrameMS		= 0;
	tStats.dwMaxFrameCPUMS	= 0;
	tStats.dwMaxFrameGPUMS	= 0;

	tTimeToStart = TIME_ZERO;

	bInterrupted = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_DemoLevelIsActive()
{
	return gtDemoLevelStatus.eState != DEMO_LEVEL_STATUS::INACTIVE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_DemoLevelIsRunning()
{
	return gtDemoLevelStatus.eState == DEMO_LEVEL_STATUS::RUNNING;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DemoLevelSetState( DEMO_LEVEL_STATUS::STATE eState )
{
	//trace( "DemoLevelSetState: from %s to %s\n", gszDemoLevelStates[ gtDemoLevelStatus.eState ], gszDemoLevelStates[ eState ] );

	gtDemoLevelStatus.eState = eState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DemoLevelGatherPreFrameStats()
{
	if ( ! e_DemoLevelIsRunning() )
		return;

	int nFrame = CHB_TimingGetCycles( CHB_TIMING_FRAME );
	if ( nFrame < 2 )
		return;

	DWORD dwLastTotal = static_cast<DWORD>( CHB_TimingGetInstantaneous( CHB_TIMING_FRAME ) * 1000 + 0.5f );
	DWORD dwLastCPU = static_cast<DWORD>( CHB_TimingGetInstantaneous( CHB_TIMING_CPU ) * 1000 + 0.5f );
	DWORD dwLastGPU = 0;

	DWORD dwLastCPUCycles   = CHB_TimingGetCycles( CHB_TIMING_CPU );
	DWORD dwLastFrameCycles = CHB_TimingGetCycles( CHB_TIMING_FRAME );

	BOOL bUpdateCPU = ( dwLastCPUCycles > gtDemoLevelStatus.tStats.dwTotalCPUCycles );
	BOOL bUpdateFrame = ( dwLastFrameCycles > gtDemoLevelStatus.tStats.dwTotalFrameCycles );

#if defined(ENGINE_TARGET_DX9) || defined(ENGINE_TARGET_DX10)
	DWORD dxC_GPUTimerGetLast();

	dwLastGPU = dxC_GPUTimerGetLast();
#endif

	if ( bUpdateFrame && gtDemoLevelStatus.tStats.dwTotalFrameCycles > 0 )
	{
		gtDemoLevelStatus.tStats.dwTotalMS += dwLastTotal;
		gtDemoLevelStatus.tStats.dwMinFrameMS = MIN( gtDemoLevelStatus.tStats.dwMinFrameMS, dwLastTotal );
		gtDemoLevelStatus.tStats.dwMaxFrameMS = MAX( gtDemoLevelStatus.tStats.dwMaxFrameMS, dwLastTotal );
	}
	gtDemoLevelStatus.tStats.dwTotalFrameCycles = dwLastFrameCycles;

	if ( bUpdateCPU && gtDemoLevelStatus.tStats.dwTotalCPUCycles > 0 )
	{
		gtDemoLevelStatus.tStats.dwTotalCPUMS += dwLastCPU;
		gtDemoLevelStatus.tStats.dwMinFrameCPUMS = MIN( gtDemoLevelStatus.tStats.dwMinFrameCPUMS, dwLastCPU );
		gtDemoLevelStatus.tStats.dwMaxFrameCPUMS = MAX( gtDemoLevelStatus.tStats.dwMaxFrameCPUMS, dwLastCPU );
	}
	gtDemoLevelStatus.tStats.dwTotalCPUCycles = dwLastCPUCycles;


	gtDemoLevelStatus.tStats.dwTotalFrames++;

	gtDemoLevelStatus.tStats.dwTotalGPUMS += dwLastGPU;

	gtDemoLevelStatus.tStats.dwMinFrameGPUMS = MIN( gtDemoLevelStatus.tStats.dwMinFrameGPUMS, dwLastGPU );
	gtDemoLevelStatus.tStats.dwMaxFrameGPUMS = MAX( gtDemoLevelStatus.tStats.dwMaxFrameGPUMS, dwLastGPU );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DemoLevelGatherPostFrameStats()
{
	if ( ! e_DemoLevelIsRunning() )
		return;

	// Any post-frame stats gathering would go here.
}



}; // FSSE