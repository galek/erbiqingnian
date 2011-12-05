//----------------------------------------------------------------------------
//	maths.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _MATHS_H_
#define _MATHS_H_


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define PI								3.14159265359f
#define TWOxPI							(2.0f * PI)
#define PI_OVER_TWO						(PI / 2.0f)
#define PI_OVER_FOUR					(PI / 4.0f)

#define MAX_RADIAN						((360.0f * PI) / 180.0f)
#define MIN_RADIAN						(0.0f)

#define KILO							1024
#define MEGA							(KILO * KILO)
#define GIGA							(MEGA * KILO)

#define CLOCKWISE						1
#define COUNTER_CLOCKWISE				-1
#define LINE							0

#define FLOAT_PCT_MULTIPLIER			(0.01f)

#define EPSILON							(0.00001f)
#define INFINITY						(9999999.9f)
#define INVALID_FLOAT					(FLT_MAX)

enum AXIS
{
	X,
	Y,
	Z,
	W,
};


//----------------------------------------------------------------------------
// CONSTANTS (used for float to integer conversion)
//----------------------------------------------------------------------------
#define USE_CVT_MAGIC					1									// 0 = crt, 1 = float, 2 = double (2 only works if D3D is set to D3DCREATE_FPU_PRESERVE)

#ifdef _BIG_ENDIAN
#define CVT_EXPONENT					0
#define CVT_MANTISSA					1
#else
#define CVT_EXPONENT					1
#define CVT_MANTISSA					0
#endif

#define CVT_MAGIC 						double(6755399441055744.0)			// 2^52 * 1.5, uses limited precisicion to floor
#define CVT_MAGIC_DELTA 				double(1.5e-8)							
#define CVT_MAGIC_ROUNDEPS				double(.5f - CVT_MAGIC_DELTA)		// almost .5f = .5f - 1e^(number of exp bit)

#define CVT_MAGIC_DELTAF 				float(1.5e-8)							
#define CVT_MAGIC_ROUNDEPSF				float(.5f - CVT_MAGIC_DELTA)		// almost .5f = .5f - 1e^(number of exp bit)
//#define CVT_FLOAT_LESSONE				float(1.0f - float(1.0e-7))			// almost 1.0f			although this is accurate, looks like d3d sets rounding differently
#define CVT_FLOAT_LESSONE				float(0.9999f)						// almost 1.0f			although this is accurate, looks like d3d sets rounding differently


//----------------------------------------------------------------------------
// note these fast conversion functions only work for floats >= -(1 << 31) && < (1 << 31)
// CVT_MAGIC doesn't work if D3D is initialized w/o D3DCREATE_FPU_PRESERVE
// so don't use CVT_MAGIC
//----------------------------------------------------------------------------
#if (USE_CVT_MAGIC == 2)
inline int FLOAT_TO_INT_HELPER(
	double val)
{
    val	= val + CVT_MAGIC;
	return ((int *)&val)[CVT_MANTISSA];
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (USE_CVT_MAGIC == 2)

#define FLOAT_TO_INT(f)			(int(f))			// uses SSE, is fastest conversion, otherwise ((f >= 0) ? FLOAT_TO_INT_HELPER(double(f) - CVT_MAGIC_ROUNDEPS) : FLOAT_TO_INT_HELPER(double(f) + CVT_MAGIC_ROUNDEPS))
#define FLOAT_TO_INT_FLOOR(f)	(int(FLOAT_TO_INT_HELPER(double(f) - CVT_MAGIC_ROUNDEPS)))
#define FLOAT_TO_INT_CEIL(f)	(int(FLOAT_TO_INT_HELPER(double(f) + CVT_MAGIC_ROUNDEPS)))
#define FLOAT_TO_INT_ROUND(f)	(int(FLOAT_TO_INT_HELPER(double(f) + CVT_MAGIC_DELTA)))
#define FLOAT_ABS(f)			fabs(f)				// DWORD dw = ((*(DWORD *)&val) & 0x7fffffff)  <=  slower than fast new fabs()

#elif (USE_CVT_MAGIC == 1)

#define FLOAT_TO_INT(f)			(int(f))			// uses SSE, is fastest conversion, otherwise ((f >= 0) ? FLOAT_TO_INT_HELPER(double(f) - CVT_MAGIC_ROUNDEPS) : FLOAT_TO_INT_HELPER(double(f) + CVT_MAGIC_ROUNDEPS))
__forceinline int FLOAT_TO_INT_FLOOR(float f)		{ f = f + f - 0.5f; __m128 xmmR = _mm_load_ss(&f); return _mm_cvtss_si32(xmmR) >> 1; }
__forceinline int FLOAT_TO_INT_ROUND(float f)		{ f = f + f + 0.5f; __m128 xmmR = _mm_load_ss(&f); return _mm_cvtss_si32(xmmR) >> 1; }
__forceinline int FLOAT_TO_INT_CEIL(float f)		{ f = -0.5f - (f + f); __m128 xmmR = _mm_load_ss(&f); return -(_mm_cvtss_si32(xmmR) >> 1); }
#define FLOAT_ABS(f)			fabs(f)				// DWORD dw = ((*(DWORD *)&val) & 0x7fffffff)  <=  slower than fast new fabs()

#else

#define FLOAT_TO_INT(f)			(int(f))
#define FLOAT_TO_INT_FLOOR(f)	(int(floor(f)))
#define FLOAT_TO_INT_CEIL(f)	(int(ceil(f)))
#define FLOAT_TO_INT_ROUND(f)	(int(floor((f) + .5)))
#define FLOAT_ABS(f)			fabs(f)

#endif


//----------------------------------------------------------------------------
// MACROS - max, pin, etc.
//----------------------------------------------------------------------------
#define FLOOR(x)						FLOAT_TO_INT_FLOOR(x)
#define CEIL(x)							FLOAT_TO_INT_CEIL(x)

#define FLOORF(x)						FLOAT_TO_INT_FLOOR(x)
#define CEILF(x)						FLOAT_TO_INT_CEIL(x)

#define PrimeFloat2Int(x)				FLOAT_TO_INT_ROUND(x)
#define ROUND(x)						FLOAT_TO_INT_ROUND(x)

#define FABS(x)							FLOAT_ABS(x)

#define FDIV2(x)						FLOOR((x)/2.0f)

#define PctToFloat(x)					((float)(x) * FLOAT_PCT_MULTIPLIER)

#define DegreesToRadians( degree )		((degree) * (PI / 180.0f))
#define RadiansToDegrees( radian )		((radian) * (180.0f / PI))

#define LERP( a, b, f )					( (a) + (f) * ( (b) - (a) ) )
#define SATURATE( fScalar )				( MIN( 1.f, MAX( 0.f, (fScalar) ) ) )


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float Math_Modf32(const float x, float * const pIntOut);
float Math_Floor32(float x);
inline float Math_GetFractionalComponent(const float x) { float t; return Math_Modf32(x, &t); }
float Math_GetInfinity(void);	// CHB 2006.10.20
float Math_GetNaN(void);		// CHB 2007.03.08
bool Math_IsInfinity(float x);	// CHB 2007.03.08
bool Math_IsNaN(float x);		// CHB 2007.03.08
bool Math_IsFinite(float x);


//----------------------------------------------------------------------------
// GLOBAL LOOKUP TABLES
//----------------------------------------------------------------------------
extern float gfCos360Tbl[360];
extern float gfSin360Tbl[360];
extern float gfSqrtFractionTbl[101];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
inline T NORMALIZE(
	T x,
	T period)
{
	if (x >= period)
	{
		x -= period;
		if (x < (T)0)
		{
			x = (T)0;
		}
	}
	else if (x < (T)0)
	{
		x += period;
		if (x >= period)
		{
			x = (T)0;
		}
	}
	ASSERT(x >= (T)0 && x < period);
	return x;
}


//----------------------------------------------------------------------------
// written this way because in vast majority of cases x is within the period
//----------------------------------------------------------------------------
template<class T>
inline T NORMALIZE_EX(
	T x,
	T period)
{
	while (x >= period)
	{
		x -= period;
		if (x < (T)0)
		{
			x = (T)0;
		}
	}

	while (x < (T)0)
	{
		x += period;
		if (x >= period)
		{
			x = (T)0;
		}
	}
	ASSERT(x >= (T)0 && x < period);
	return x;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
inline T ROUNDUP(
	T value,
	T radix)
{
	DBG_ASSERT(radix > 0);
	
	return ((value + radix - 1) / radix) * radix;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT(
	int value,
	int pct)
{
	if (pct == 100)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return value / 100 * pct;
	}
	else
	{
		return value * pct / 100;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT(
	int value,
	float pct)
{
	if (pct == 100.0f)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return int(float(value) / 100.0f * pct);
	}
	else
	{
		return int(float(value) * pct / 100.0f);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT_ROUND(
	int value,
	int pct)
{
	if (pct == 100)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return (value + 50) / 100 * pct;
	}
	else
	{
		return (value * pct + 50) / 100;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT_ROUNDUP(
	int value,
	int pct)
{
	if (pct == 100)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return (value + 99) / 100 * pct;
	}
	else
	{
		return (value * pct + 99) / 100;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT_ROUND(
	int value,
	float pct)
{
	if (pct == 100.0f)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return int((value + 50.0f) / 100.0f * pct);
	}
	else
	{
		return int((value * pct + 50.0f) / 100.0f);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int PCT_ROUNDUP(
	int value,
	float pct)
{
	if (pct == 100.0f)
	{
		return value;
	}
	if (value > USHRT_MAX)
	{
		return int((value + 99.0f) / 100.0f * pct);
	}
	else
	{
		return int((value * pct + 99.0f) / 100.0f);
	}
}


//----------------------------------------------------------------------------
// return -1 on overflow, both a & b must not have most sig bit set
//----------------------------------------------------------------------------
inline DWORD ADD_CHECK_OVERFLOW(
	DWORD a,
	DWORD b)
{
    DWORD t1 = a + b;   
    DWORD t2 = (DWORD)((int)t1 >> 31);
    return (t1 | t2);				// t2 is -1 on overflow, 0 otherwise
}

//----------------------------------------------------------------------------
// return -1 on underflow, both a & b must not have most sig bit set
//----------------------------------------------------------------------------
inline DWORD SUB_CHECK_UNDERFLOW(
	DWORD a,
	DWORD b)
{
	DWORD t1 = a - b;
	DWORD t2 = (DWORD)((int)t1 >> 31);
	return (t1 & ~t2);
}

//----------------------------------------------------------------------------
// 0 is considered power of 2, but we don't care
//----------------------------------------------------------------------------
inline BOOL IsPowerOf2(
	int n)
{
	return !(n & (n - 1));
}


// ---------------------------------------------------------------------------
// DESC:	returns smallest power of 2 >= input
// ---------------------------------------------------------------------------
inline unsigned int GetPowerOf2(
	unsigned int x)
{
	if (IsPowerOf2(x))
	{
		return x;
	}
	return (1 << HIGHBIT(x));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int AnglesGetDelta(
	int first,
	int second)
{
	int delta = second - first;
	if (delta > 180)
	{
		delta -= 360;
	}
	else if (delta < -180)
	{
		delta += 360;
	}
	return delta;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int GetClockDirection(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3)
{
	float fTest = (((x2 - x1) * (y3 - y1)) - ((x3 - x1) * (y2 - y1)));
	if (fTest > 0.0f)
	{
		return CLOCKWISE;
	}
	else if (fTest < 0.0f)
	{
		return COUNTER_CLOCKWISE;
	}
	return LINE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PointInside(
	float x,
	float y,
	float min_x,
	float min_y,
	float max_x,
	float max_y)
{
	return (x >= min_x && x < max_x &&
		y >= min_y && y < max_y);
}

// make sure that a float is a valid number
inline float			VerifyFloat( float Value,		// value to test for validity
									 float Maximum )// maximum value that the float may be
{ 
	if ( _isnan( Value ) )
	{
		Value = 0;
	}
	if ( !_finite( Value) )
	{
		Value = Maximum;
	}

	return Value;
}

// make sure that a double is a valid number
inline double			VerifyDouble( double Value,		// value to test for validity
									  double Maximum )	// maximum value that the float may be
{
	if ( _isnan( Value ) )
	{
		Value = 0;
	}
	if ( !_finite( Value ) )
	{
		Value = Maximum;
	}
	return Value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float round(
	float x,
	int numdecimal)
{
	float fInt;
	float fDec = modf(x, &fInt);

	if (fDec == 0.0f)
		return x;

	fDec *= (float)(powf(10.0f, float(numdecimal)));
	fDec = (float)FLOOR(fDec);
	fDec /= (float)(powf(10.0f, float(numdecimal)));

	return fInt + fDec;
}


//----------------------------------------------------------------------------
// Template function designed for integers
//----------------------------------------------------------------------------
template <typename T>
inline T POWI(
	T x, 
	T n)
{
	if (n <= 0)
	{
		return 0;
	}
	
	T t = 1;
	while (n)
	{
		if (n & 1)
		{
			t = (T)(t * x);
		}
		n >>= 1;
		x = (T)(x * x);
	}
	
	return t;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float FMODF( float x, float y )
{
	if ( x > y || x < -y )
		return fmodf( x, y );
	return x;
}

//----------------------------------------------------------------------------
// Float equality with epsilon
//    Pick a reasonable epsilon for the range of the values for comparison!
//----------------------------------------------------------------------------
inline BOOL FEQ( float a, float b, float eps )
{
	return ( fabs(a-b) <= eps );
}

//----------------------------------------------------------------------------
// Floating point precision truncation
// Truncates off the lower digits of a floating point number past the decimal
// point.  Uses scaled arithmetic, but does no scale checking, so be sure that
// your scaled values fit within the range!
//----------------------------------------------------------------------------
inline float TRUNCATE(
	float fInput,
	int nDigits)
{
	float fMultiplier = powf(10.0f, float(nDigits));
	fInput *= fMultiplier;
	int nInput = int(fInput);
	fInput = float(nInput);
	return fInput / fMultiplier;
}

//----------------------------------------------------------------------------
// Ease [in/out/inout] functions
//    Takes f in the range [0..1], returns f modified to follow a curve smoothed
//    "in" (continuous through 0.f) and/or "out" (continuous through 1.f)
//    **_HQ functions use sin() for precision, others use tables for speed
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
inline float EASE_IN_HQ( float f )			{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return sinf( (f - 1.f) * 0.5f * PI ) + 1.f;		}
inline float EASE_OUT_HQ( float f )			{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return sinf( f * 0.5f * PI );						}
inline float EASE_INOUT_HQ( float f )		{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return sinf( (f - 0.5f) * PI ) * 0.5f + 0.5f;		}

inline float EASE_IN( float f )				{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return gfSin360Tbl[ unsigned( (f + 3.f) * 0.5f * 180.f ) % 360 ] + 1.f;		}
inline float EASE_OUT( float f )			{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return gfSin360Tbl[ unsigned( f * 0.5f * 180.f ) ];							}
inline float EASE_INOUT( float f )			{	ASSERT_RETVAL(f >= 0.0f && f <= 1.0f, f); return gfSin360Tbl[ unsigned( (f + 1.5f) * 180.f ) % 360 ] * 0.5f + 0.5f;		}
#else
inline float EASE_IN_HQ( float f )			{	return sinf( (f - 1.f) * 0.5f * PI ) + 1.f;			}
inline float EASE_OUT_HQ( float f )			{	return sinf( f * 0.5f * PI );						}
inline float EASE_INOUT_HQ( float f )		{	return sinf( (f - 0.5f) * PI ) * 0.5f + 0.5f;		}

inline float EASE_IN( float f )				{	return gfSin360Tbl[ unsigned( (f + 3.f) * 0.5f * 180.f ) % 360 ] + 1.f;			}
inline float EASE_OUT( float f )			{	return gfSin360Tbl[ unsigned( f * 0.5f * 180.f ) ];								}
inline float EASE_INOUT( float f )			{	return gfSin360Tbl[ unsigned( (f + 1.5f) * 180.f ) % 360 ] * 0.5f + 0.5f;		}
#endif // DEVELOPMENT

//----------------------------------------------------------------------------
// Greatest common divisor
//    Uses the Euclidean method to compute the greatest common divisor of two numbers
//----------------------------------------------------------------------------
template< class T >
inline T GCD( T a, T b )
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETZERO( a >= 0 );
	ASSERT_RETZERO( b >= 0 );
#endif
	if ( b > a )
		return GCD(b,a);

	if ( b == 0 )
		return a;

	return GCD( b, a%b );
}


//----------------------------------------------------------------------------
// Lowest common multiple
//    Uses the GCD to compute the lowest common multiple of two numbers
//----------------------------------------------------------------------------
template< class T >
inline T LCM( T a, T b )
{
	T nLCM = ( a / GCD( a, b ) ) * b;
	return nLCM;
}


inline float LCM( float a, float b )
{
	// convert to fixed point and perform integer LCM
	const float cfScale = 100000.f;
	return (float)LCM( (__int64)( a*cfScale ), (__int64)( b*cfScale ) )   / cfScale;
}


//----------------------------------------------------------------------------
// Running Average
//    Templated running average struct for scalar values. T is value type, N is count.
//----------------------------------------------------------------------------

template< class T, int N >
struct RunningAverage
{
	T tBuffer[ N ];
	int nNext;
	DWORD dwUses;

	void Zero()	{ memclear( tBuffer, sizeof(T)*N ); nNext = 0; dwUses = 0; }
	T Avg()
	{
		T sum = 0;
		int nCount = MIN( (DWORD)N, dwUses );
		for ( int i = 0; i < nCount; i++ )
		{
			sum += tBuffer[i];
		}
		return nCount ? (sum / nCount) : 0;
	}
	T Max()
	{
		T max = 0;
		int nCount = MIN( (DWORD)N, dwUses );
		for ( int i = 0; i < nCount; i++ )
		{
			max = MAX( tBuffer[i], max );
		}
		return max;
	}
	void Push( T value )
	{
		tBuffer[ nNext ] = value;
		nNext = ( nNext + 1 ) % N;
		++dwUses;
	}
};

#endif // _MATHS_H_
