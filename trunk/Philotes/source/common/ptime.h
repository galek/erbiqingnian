//----------------------------------------------------------------------------
// ptime.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PTIME_H_
#define _PTIME_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define APP_STEP_SPEED_DEFAULT			0

#define GAME_TICKS_PER_SECOND			20
#define GAME_TICKS_PER_SECOND_FLOAT		((float)GAME_TICKS_PER_SECOND)
#define GAME_TICK_TIME					(1.0f / GAME_TICKS_PER_SECOND_FLOAT)

#define MSECS_PER_GAME_TICK				(MSECS_PER_SEC / GAME_TICKS_PER_SECOND)
#define GAME_TICK_TIME_FLOAT			(1.0f / GAME_TICKS_PER_SECOND_FLOAT)

#define UTCTIME_GET_HIGH_DWORD(time)	((DWORD)((time & (time_t)0xFFFFFFFF00000000i64) >> 32i64))
#define UTCTIME_GET_LOW_DWORD(time)		((DWORD)(time & (time_t)0x00000000FFFFFFFFi64))
#define UTCTIME_SPLIT(time, hi, lo)		{ hi = UTCTIME_GET_HIGH_DWORD(time); lo = UTCTIME_GET_LOW_DWORD(time); }
#define UTCTIME_CREATE(hi, lo)			((time_t)(((unsigned __int64)hi << 32i64) + (unsigned __int64)lo))

//----------------------------------------------------------------------------
// TIME
//----------------------------------------------------------------------------
#define TIME64

#ifdef TIME64
typedef UINT64							_TIME;
typedef INT64							TIMEDELTA;
#else
typedef DWORD							_TIME;
typedef int								TIMEDELTA;
#endif

#define TIME_ZERO						((_TIME)0)
#define TIME_LAST						((_TIME)-1)

struct TIME
{
	_TIME		time;

	// constructor
	TIME(
		void) : time(0)
	{
	}
	TIME(
		_TIME i)
	{
		time = i;
	}
/*
	TIME(
		int i)
	{
		time = (_TIME)i;
	}
*/

	TIME(
		float i)
	{
		time = (_TIME)i;
	}
	// assignment
	void operator=(
		const TIME & value)
	{
		time = value.time;
	}
	void operator=(
		const _TIME & value)
	{
		time = value;
	}

	// math
	TIME& operator+=(
		const _TIME & value)
	{
		time += value;
		return *this;
	}

	// type cast
#ifdef TIME64
	operator UINT64(
		void) const
	{
		return time;
	}
#else
	operator DWORD(
		void) const
	{
		return time;
	}
#endif
};


inline DWORD TimeGetElapsed(
	const TIME & old,
	const TIME & cur)
{
	ASSERT_RETZERO(cur >= old);
	ASSERT_RETVAL((cur - old) <= INT_MAX, INT_MAX);
	return (DWORD)(cur - old);
}


/*
inline TIME operator+(
   const TIME & a,
   const TIME & b)
{
	return TIME(a.time + b.time);
}

inline TIME operator+(
   const TIME & a,
   const int & b)
{
	return TIME(_TIME(a.time + b));
}

inline TIME operator+(
   const TIME & a,
   const DWORD & b)
{
	return TIME(_TIME(a.time + b));
}

inline int operator-(
   const TIME & a,
   const TIME & b)
{
	return int(a.time - b.time);
}

inline int operator-(
   const _TIME & a,
   const TIME & b)
{
	return int(a - b.time);
}

inline int operator-(
   const int& a,
   const TIME& b)
{
	return int(_TIME(a) - b.time);
}

inline int operator-(
   const TIME& a,
   const int& b)
{
	return int(a.time - _TIME(b));
}

inline bool operator<(
   const TIME& a,
   const TIME& b)
{
	return int(b - a) > 0;
}

inline bool operator<(
   const TIME& a,
   const int& b)
{
	return int(b - a) > 0;
}

inline bool operator<=(
   const TIME& a,
   const TIME& b)
{
	return int(b - a) >= 0;
}

inline bool operator<=(
   const TIME& a,
   const int& b)
{
	return int(b - a) >= 0;
}

inline bool operator>(
   const TIME& a,
   const TIME& b)
{
	return int(b - a) < 0;
}

inline bool operator>(
   const TIME& a,
   const int& b)
{
	return int(b - a) < 0;
}

inline bool operator>=(
   const TIME& a,
   const TIME& b)
{
	return int(b - a) <= 0;
}

inline bool operator>=(
   const TIME& a,
   const int& b)
{
	return int(b - a) <= 0;
}
*/


//----------------------------------------------------------------------------
// GAME_TICK
//----------------------------------------------------------------------------
struct GAME_TICK
{
	DWORD		tick;

	// constructor
	GAME_TICK(
		void) : tick(0)
	{
	}
	GAME_TICK(
		DWORD i) : tick(i)
	{
	}
	GAME_TICK(
		int i) : tick((DWORD)i)
	{
	}
	GAME_TICK(
		float i) : tick((DWORD)i)
	{
	}
	// assignment
	void operator=(
		const GAME_TICK& value)
	{
		tick = value.tick;
	}
	void operator=(
		const DWORD& value)
	{
		tick = value;
	}

	// math
	GAME_TICK& operator+=(
		const DWORD& value)
	{
		tick += value;
		return *this;
	}

	GAME_TICK& operator++(
		void)
	{
		tick++;
		return *this;
	}

	GAME_TICK& operator++(
		int)
	{
		tick++;
		return *this;
	}


	// type cast
	operator DWORD(
		void) const
	{
		return tick;
	}
};

inline bool operator<(
   const GAME_TICK& a,
   const GAME_TICK& b)
{
	return int(b - a) > 0;
}

inline bool operator<(
   const GAME_TICK& a,
   const int& b)
{
	return int(b - a) > 0;
}

inline bool operator<(
   const int& a,
   const GAME_TICK& b)
{
	return int(b - a) > 0;
}

inline bool operator<(
   const GAME_TICK& a,
   const DWORD& b)
{
	return int(b - a) > 0;
}

inline bool operator<(
   const DWORD& a,
   const GAME_TICK& b)
{
	return int(b - a) > 0;
}

inline bool operator<=(
   const GAME_TICK& a,
   const GAME_TICK& b)
{
	return int(b - a) >= 0;
}

inline bool operator<=(
   const int& a,
   const GAME_TICK& b)
{
	return int(b - a) >= 0;
}

inline bool operator<=(
   const GAME_TICK& a,
   const int& b)
{
	return int(b - a) >= 0;
}

inline bool operator<=(
   const DWORD& a,
   const GAME_TICK& b)
{
	return int(b - a) >= 0;
}

inline bool operator<=(
   const GAME_TICK& a,
   const DWORD& b)
{
	return int(b - a) >= 0;
}

inline bool operator>(
   const GAME_TICK& a,
   const GAME_TICK& b)
{
	return int(b - a) < 0;
}

inline bool operator>(
   const GAME_TICK& a,
   const int& b)
{
	return int(b - a) < 0;
}

inline bool operator>(
   const int& a,
   const GAME_TICK& b)
{
	return int(b - a) < 0;
}

inline bool operator>(
   const GAME_TICK& a,
   const DWORD& b)
{
	return int(b - a) < 0;
}

inline bool operator>(
   const DWORD& a,
   const GAME_TICK& b)
{
	return int(b - a) < 0;
}

inline bool operator>=(
   const GAME_TICK& a,
   const GAME_TICK& b)
{
	return int(b - a) <= 0;
}

inline bool operator>=(
   const int& a,
   const GAME_TICK& b)
{
	return int(b - a) <= 0;
}

inline bool operator>=(
   const GAME_TICK& a,
   const int& b)
{
	return int(b - a) <= 0;
}

inline bool operator>=(
   const DWORD& a,
   const GAME_TICK& b)
{
	return int(b - a) <= 0;
}

inline bool operator>=(
   const GAME_TICK& a,
   const DWORD& b)
{
	return int(b - a) <= 0;
}

enum SYSTEM_TIME_RESULT
{
	STR_LESS,
	STR_EQUAL,
	STR_GREATER
};

inline SYSTEM_TIME_RESULT SystemTimeCompare(
	const SYSTEMTIME &tTimeA,
	const SYSTEMTIME &tTimeB)
{
    if (tTimeA.wYear < tTimeB.wYear) { return STR_LESS; }
    else if (tTimeA.wYear > tTimeB.wYear) { return STR_GREATER; }

    if (tTimeA.wMonth < tTimeB.wMonth) { return STR_LESS; }
    else if (tTimeA.wMonth> tTimeB.wMonth) { return STR_GREATER; }

    if (tTimeA.wDay < tTimeB.wDay) { return STR_LESS; }
    else if (tTimeA.wDay> tTimeB.wDay) { return STR_GREATER; }

    if (tTimeA.wHour < tTimeB.wHour) { return STR_LESS; }
    else if (tTimeA.wHour> tTimeB.wHour) { return STR_GREATER; }

    if (tTimeA.wMinute < tTimeB.wMinute) { return STR_LESS; }
    else if (tTimeA.wMinute> tTimeB.wMinute) { return STR_GREATER; }

    if (tTimeA.wSecond < tTimeB.wSecond) { return STR_LESS; }
    else if (tTimeA.wSecond> tTimeB.wSecond) { return STR_GREATER; }

    if (tTimeA.wMilliseconds < tTimeB.wMilliseconds) { return STR_LESS; }
    else if (tTimeA.wMilliseconds> tTimeB.wMilliseconds) { return STR_GREATER; }
    
    return STR_EQUAL;
    	
}

inline void SystemTimeOffset(
	SYSTEMTIME &tTime,
	const SYSTEMTIME &tTimeOffset)
{
	tTime.wYear = tTime.wYear + tTimeOffset.wYear;
	tTime.wMonth = tTime.wMonth + tTimeOffset.wMonth;
    tTime.wDay = tTime.wDay + tTimeOffset.wDay;
	tTime.wHour = tTime.wHour + tTimeOffset.wHour;
    tTime.wMinute = tTime.wMinute + tTimeOffset.wMinute;
    tTime.wSecond = tTime.wSecond + tTimeOffset.wSecond;
    tTime.wMilliseconds = tTime.wMilliseconds + tTimeOffset.wMilliseconds; 
}	

void TimeTMToSystemTime(
	const struct tm *pT,
	SYSTEMTIME *pSystemTime);

void TimeTMToString(
	const tm *pT,
	WCHAR *puszBuffer,
	int nMaxBuffer);

void UTCTimeAddDay(
	time_t *pTime,
	int nNumDays);
		
//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// EXPORTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------


#endif // _PTIME_H_

