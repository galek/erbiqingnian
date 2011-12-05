//----------------------------------------------------------------------------
//	random.h
//
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _RANDOM_H_
#define _RANDOM_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _WIN_OS_H_
#include "winos.h"
#endif

#if ISVERSION(DEVELOPMENT)
#ifndef _CONFIG_H
#include "config.h"
#endif
#endif


//----------------------------------------------------------------------------
// DEBUG
//----------------------------------------------------------------------------
// useful for finding memory leaks, etc.

//#define FORCE_FIXED_SEEDS

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define HIGH_BITS(x)					((x) >> 16)
#define LOW_BITS(x)						((x) & 0xffff)


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define RAND_MULTIPLIER					0x6ac690c5
#define RAND_DEFAULT_HI_SEED			0x466c6170


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct RAND
{
#ifndef NTG_PLATFORM_XBOX2
	DWORD	hi_seed;
	DWORD	lo_seed;
#else
	DWORD	lo_seed;
	DWORD	hi_seed;
#endif
};


//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void RandInit(
	RAND & rand,
	DWORD seed1 = 0,
	DWORD seed2 = 0,
	BOOL bIgnoreFixedSeeds = FALSE);

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------


// [0, DWORD_MAX]
inline DWORD RandGetNum(
	RAND & rand)
{
	__int64 temp = (__int64)rand.lo_seed * (__int64)RAND_MULTIPLIER + (__int64)rand.hi_seed;

#ifndef NTG_PLATFORM_XBOX2
	rand.hi_seed = *((DWORD *)&temp + 1);
	rand.lo_seed = *(DWORD *)&temp;
#else
	rand.hi_seed = *(DWORD *)&temp;
	rand.lo_seed = *((DWORD *)&temp + 1);
#endif

	return (rand.lo_seed);
}

// [0, max)
inline DWORD RandGetNum(
	RAND & rand,
	int max)
{
	if (max <= 0)
	{
		return 0;
	}
	return RandGetNum(rand) % max;
}

// [min, max]
inline DWORD RandGetNum(
	RAND & rand,
	int min,
	int max)
{
	if (min >= max)
	{
		return min;
	}
	return RandGetNum(rand) % (max - min + 1) + min;
}


// [0..UIN64_MAX]
inline UINT64 Rand64(
	RAND & rand)
{
	return UINT64(RandGetNum(rand)) + (UINT64(RandGetNum(rand)) << UINT64(32));
}


// [0, 1]
inline float RandGetFloat(
	RAND & rand)
{
	return (float)RandGetNum(rand) / (float)UINT_MAX;
}

// [0, max]
inline float RandGetFloat(
	RAND & rand,
	float max)
{
	return max * RandGetFloat(rand);
}

// [min, max]
inline float RandGetFloat(
	RAND & rand,
	float min,
	float max)
{
	return min + (max - min) * RandGetFloat(rand);
}

// boolean selection
inline BOOL RandSelect(
	RAND & rand,
	int chance,
	int range)
{
	ASSERT(chance < USHRT_MAX && range < USHRT_MAX);
	return (chance * USHRT_MAX) > (int)RandGetNum(rand, range * USHRT_MAX);
}

// [-1, 1] per component - Not normalized
inline VECTOR RandGetVector(
	RAND &rand)
{
	float x = RandGetFloat(rand, -1.0f, 1.0f);
	float y = RandGetFloat(rand, -1.0f, 1.0f);
	float z = RandGetFloat(rand, -1.0f, 1.0f);
	return VECTOR(x, y, z);
}

// [-1, 1] per component - Not normalized
inline VECTOR RandGetVectorXY(
	RAND &rand)
{
	float x = RandGetFloat(rand, -1.0f, 1.0f);
	float y = RandGetFloat(rand, -1.0f, 1.0f);
	float z = 0.0f;
	return VECTOR(x, y, z);
}

// even distribution
inline VECTOR RandGetVectorEx(
	RAND &rand)
{
    float z = RandGetFloat(rand, -1.0f, 1.0f);
    float a = RandGetFloat(rand, 0.0f, TWOxPI);
    float r = sqrtf(1.0f - z*z);
    float x = r * cosf(a);
    float y = r * sinf(a);

    return VECTOR(x, y, z);
}


#endif // _RANDOM_H_

