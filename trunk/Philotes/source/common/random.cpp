//----------------------------------------------------------------------------
//	random.cpp
//
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------





//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

void RandInit(
	RAND & rand,
	DWORD seed1 /*= 0*/,
	DWORD seed2 /*= 0*/,
	BOOL bIgnoreFixedSeeds /*= FALSE*/ )
{
	if ( ! bIgnoreFixedSeeds && ( AppCommonGetDemoMode() || AppTestDebugFlag( ADF_FIXED_SEEDS ) ) )
	{
		rand.hi_seed = RAND_DEFAULT_HI_SEED;
		rand.lo_seed = 1;
		return;
	}

	rand.hi_seed = seed2;
    rand.lo_seed = seed1;

	while (rand.lo_seed == 0)
	{
		rand.lo_seed = GetTickCount() + (DWORD)PGetPerformanceCounter();
	}
	if (rand.hi_seed == 0)
	{
		rand.hi_seed = RAND_DEFAULT_HI_SEED;
	}
}
