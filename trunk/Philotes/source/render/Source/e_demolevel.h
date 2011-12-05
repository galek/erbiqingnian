//----------------------------------------------------------------------------
// e_demolevel.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_DEMOLEVEL__
#define __E_DEMOLEVEL__

namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DEMO_LEVEL_STATUS
{
	enum STATE
	{
		INACTIVE = 0,
		INIT,
		PRE_COUNTDOWN,
		COUNTDOWN,
		READY_TO_RUN,
		RUNNING,
		SHOW_STATS,
		//
		NUM_STATES
	};

	struct STATS
	{
		DWORD						dwTotalFrames;
		DWORD						dwTotalMS;
		DWORD						dwTotalGPUMS;
		DWORD						dwTotalCPUMS;

		DWORD						dwTotalCPUCycles;
		DWORD						dwTotalFrameCycles;

		DWORD						dwMinFrameMS;
		DWORD						dwMaxFrameMS;
		DWORD						dwMinFrameGPUMS;
		DWORD						dwMaxFrameGPUMS;
		DWORD						dwMinFrameCPUMS;
		DWORD						dwMaxFrameCPUMS;
	};

	TIME tTimeToStart;

	STATE eState;
	STATS tStats;
	BOOL bInterrupted;

	void Init();
	void Reset();

	DEMO_LEVEL_STATUS() : eState(INACTIVE) {}
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void e_DemoLevelSetState( DEMO_LEVEL_STATUS::STATE eState );
BOOL e_DemoLevelIsActive();
BOOL e_DemoLevelIsRunning();
void e_DemoLevelGatherPreFrameStats();
void e_DemoLevelGatherPostFrameStats();


}; // FSSE

#endif // __E_DEMOLEVEL__
