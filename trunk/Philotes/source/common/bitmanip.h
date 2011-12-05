//----------------------------------------------------------------------------
// bitmanip.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _BITMAP_H_
#define _BITMAP_H_


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include <intrin.h>


//----------------------------------------------------------------------------
// DEBUG DEFINES
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define BIT_BUF_DEBUG					1
#else
#define BIT_BUF_DEBUG					0
#endif

#if BIT_BUF_DEBUG
#define	BBD_FILE_LINE_ARGS				FILE_LINE_ARGS_ON
#define BBD_FL_ASSERT_RETFALSE(exp)		FL_ASSERT_RETFALSE(exp, file, line)
#define	BBD_FILE_LINE					, __FILE__, __LINE__
#define BBD_FL							, file, line
#else
#define	BBD_FILE_LINE_ARGS				FILE_LINE_ARGS_OFF
#define BBD_FL_ASSERT_RETFALSE(exp)		ASSERT_RETFALSE(exp)
#define	BBD_FILE_LINE					
#define BBD_FL
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define BITS_PER_BYTE					8
#define BITS_PER_WORD					(BITS_PER_BYTE * 2)
#define BITS_PER_DWORD					(BITS_PER_BYTE * 4)
#define BITS_PER_QWORD					(BITS_PER_BYTE * 8)


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define DWORD_FLAG_SIZE(x)				((((x) + (BITS_PER_DWORD - 1)))/BITS_PER_DWORD)
#define BYTE_FLAG_SIZE(x)				((((x) + (BITS_PER_BYTE - 1)))/BITS_PER_BYTE)
#define MAKE_MASK(x)					(DWORD)(1 << (x))
#define MAX_VALUE_GIVEN_BYTES(n)		(unsigned int)(((1U << ((unsigned int)(n) * 8U - 1U)) - 1U) + ((1U << ((unsigned int)(n) * 8U - 1U))))
#define MAKE_PARAM(lo, hi)				(((hi) << (BITS_PER_WORD)) + ((lo) & ((1 << BITS_PER_WORD) - 1)))
#define BIT_SIZE(x)						HIGHBIT(x)

#define TEST_MASK(dw, mask)				(((dw) & (mask)) != 0)
#define SET_MASK(dw, mask)				(dw |= mask)
#define SET_MASKV(dw, mask, val)		(val ? SET_MASK(dw, mask) : CLEAR_MASK(dw, mask))
#define CLEAR_MASK(dw, mask)			(dw &= ~mask)

// return # of bits for a type
#define BITS_IN_TYPE(t)					(sizeof(t) * 8)
// create a mask of b lower bits given a type
#define MAKE_NBIT_MASK(t, b)			((t)((b) == BITS_IN_TYPE(t) ? -1 : (((t)1 << (b)) - 1)))
// extract some number of bits v = value, s = start (low) bit, b = bit count
#define MASK_NBIT(t, v, s, b)			(((v) >> (s)) & MAKE_NBIT_MASK(t, (b)))
// setup bits into mask
#define MAKE_NBIT_VALUE(t, v, s, b)		(((v) & MAKE_NBIT_MASK(t, (b))) << (s))


//----------------------------------------------------------------------------
// EXTERN
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
// CHB 2006.08.15 - Added macro version. As an inline function, the compiler
// does not optimize ideally when a constant bit index is used.
#define TESTBIT_MACRO_ARRAY(pBitSet, nBit) (((pBitSet)[(nBit) / (sizeof(*(pBitSet)) * 8)] & (1 << ((nBit) & ((sizeof(*(pBitSet)) * 8) - 1)))) != 0)
#define TESTBIT_DWORD(dwBitSet, nBit)      ((dwBitSet & (1 << (nBit & (BITS_PER_DWORD - 1)))) != 0)

inline BOOL TESTBIT(
	const DWORD * pdwBitSet,
	int bit)
{
	return (pdwBitSet[bit / BITS_PER_DWORD] & (1 << (bit & (BITS_PER_DWORD - 1)))) != 0;
}

inline BOOL TESTBIT(
	const BYTE * pbBitSet,
	int bit)
{
	return (pbBitSet[bit / BITS_PER_BYTE] & (1 << (bit & (BITS_PER_BYTE - 1)))) != 0;
}

inline BOOL TESTBIT(
	const DWORD & dwBitSet,
	int bit)
{
	return (dwBitSet & (1 << (bit & (BITS_PER_DWORD - 1)))) != 0;
}

inline void SETBIT(
	DWORD * pdwBitSet,
	int bit)
{
	pdwBitSet[bit / BITS_PER_DWORD] |= (1 << (bit & (BITS_PER_DWORD - 1)));
}

inline void SETBIT(
	BYTE * pbBitSet,
	int bit)
{
	pbBitSet[bit / BITS_PER_BYTE] |= (1 << (bit & (BITS_PER_BYTE - 1)));
}

inline DWORD SETBIT(
	DWORD & dwBitSet,
	int bit)
{
	return dwBitSet |= (1 << bit);
}

inline void CLEARBIT(
	DWORD * pdwBitSet,
	int bit)
{
	pdwBitSet[bit / BITS_PER_DWORD] &= ~(1 << (bit & (BITS_PER_DWORD - 1)));
}

inline void CLEARBIT(
	BYTE * pbBitSet,
	int bit)
{
	pbBitSet[bit / BITS_PER_BYTE] &= ~(1 << (bit & (BITS_PER_BYTE - 1)));
}

inline DWORD CLEARBIT(
	DWORD & dwBitSet,
	int bit)
{
	return dwBitSet &= ~(1 << bit);
}

inline void TOGGLEBIT(
	DWORD * pdwBitSet,
	int bit)
{
	pdwBitSet[bit / BITS_PER_DWORD] ^= (1 << (bit & (BITS_PER_DWORD - 1)));
}

inline void TOGGLEBIT(
	BYTE * pbBitSet,
	int bit)
{
	pbBitSet[bit / BITS_PER_BYTE] ^= (1 << (bit & (BITS_PER_BYTE - 1)));
}

inline DWORD TOGGLEBIT(
	DWORD & dwBitSet,
	int bit)
{
	return dwBitSet ^= (1 << bit);
}

inline void SETBIT(
	DWORD * pdwBitSet,
	int bit,
	BOOL val)
{
	pdwBitSet[bit / BITS_PER_DWORD] = val ? 
		(pdwBitSet[bit / BITS_PER_DWORD] | (1 << (bit & (BITS_PER_DWORD - 1)))) 
		: (pdwBitSet[bit / BITS_PER_DWORD] & ~(1 << (bit & (BITS_PER_DWORD - 1))));
}

inline void SETBIT(
	BYTE * pbBitSet,
	int bit,
	BOOL val)
{
	pbBitSet[bit / BITS_PER_BYTE] = val ? 
		(pbBitSet[bit / BITS_PER_BYTE] | (BYTE)(1 << (bit & (BITS_PER_BYTE - 1)))) 
		: (pbBitSet[bit / BITS_PER_BYTE] & (BYTE)(~(1 << (bit & (BITS_PER_BYTE - 1)))));
}

inline DWORD SETBIT(
	DWORD & dwBitSet,
	int bit,
	BOOL val)
{
	return dwBitSet = val ? (dwBitSet | (1 << bit)) : (dwBitSet & ~(1 << bit));
}

inline int COUNTBITS(
	const DWORD dw)
{
	// Comments by Guy
	// Code from Standford, apparently, according to Peter
	// Set every two-bit pair to contain the sum of those two bits
	// Let's look at this calculation for one nibble:
	//   b(3)b(2)b(1)b(0)
	// - 0   b(3)0   b(1)
	//   ----------------
	//   b(3)(b(2) - b(3))b(1)(b(0) - b(1))
	// Which doesn't make a whole lot of sense, until you look at the complete chart of a two-bit pair:
	// b(1) | b(0) | b(1)(b(0) - b(1))
	// -----------------------------
	//   0  |   0  |  00
	//   0  |   1  |  01
	//   1  |   0  |  01
	//   1  |   1  |  10
	// So, as you can see now every two-bit pair contains the count of those two bits
	unsigned int const w = dw - ((dw >> 1) & 0x55555555);
	
	// Sum up the bit pairs so that every nibble will contain the count of the bits in that nibble
	// This works because we've just summed up every two-bit pair.  Recall that:
	// 0x3 = b0011
	// So by anding with 0x33333333 we're isolating every other two-bit pair sum.  The first part performs
	// this isolation, but the second part first shifts over two bits, effectively isolating every OTHER
	// pair (that is, every pair that the first part doesn't isolate), but in the SAME position.
	// Thus, when the numbers are summed up, each nibble will contain the count of the bits in that nibble
	unsigned int const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);

	// This is the final step in the process.  Let's have a look at what it does.
	// The first part should look fairly familiar by this time.  Let's assign each nibble the value s(n),
	// for the count of the bits in that nibble.  Our DWORD now looks like this:
	// s(7)s(6)s(5)s(4)s(3)s(2)s(1)s(0)
	// To this we add:
	//   0 s(7)s(6)s(5)s(4)s(3)s(2)s(1)
	// We know that we won't have any overflows, because all s(n) are less than or equal to four.  So,
	// after summing and then anding with 0x0f0f0f0f (effectively zeroing out every other nibble), we're
	// left with:
	// 0(s(7) + s(6))0(s(5) + s(4))0(s(3) + s(2))0(s(1) + s(0))
	// Let's rename these values so it's shorter:
	// 0S(3)0S(2)0S(1)0S(0)
	// Now, the penultimate step, we multiply by 0x01010101:
	//   0S(3)0S(2)0S(1)0S(0)
	// * 0  1 0  1 0  1 0  1
	//   --------------------
	//   0S(3)0S(2)0S(1)0S(0)
	// + 0S(2)0S(1)0S(0)
	// + 0S(1)0S(0)
	// + 0S(0)
	//   --------------------------------------------------------
	//   0 (S(3)+S(2)+S(1)+S(0)) 0 (S(2)+S(1)+S(0)) 0 (S(1)+S(0)) 0 S(0)
	// We once again rename this to:
	// 0W0X0Y0Z
	// Clearly, W and X can overflow into the nibbles above them, but because they are zero, we can safely ignore that.
	// Now, though, we have a value that contains exactly what we wanted all along: 0W is equal to the count of the 
	// all of the bits in dw.  To isolate 0W we shift right 24 times, and call it done.
	return ((x + (x >> 4) & 0x0f0f0f0f) * 0x01010101) >> 24;
}

inline int COUNTBITS(
	const DWORD * pdwBits,
	int nLength)
{
	int nBitsSet = 0;
	for (int ii = nLength; ii != 0; --ii)
	{
		nBitsSet += COUNTBITS(pdwBits[ii]);
	}
	return nBitsSet;
}

// returns -1 if val < 0 and 1 if val >= 0
inline int SIGN(
	int val)
{
	return 1 | (val >> 31);
}

// 0x6996 == 0110 1001 1001 0110
inline DWORD PARITY(
	DWORD dw)
{
	dw ^= dw >> 16;
	dw ^= dw >> 8;
	dw ^= dw >> 4;
	dw &= 0xf;
	return (0x6996 >> dw) & 1;
}

inline DWORD PARITY(
	BYTE by)
{
	by ^= by >> 4;
	by &= 0xf;
	return (0x6996 >> by) & 1;
}

inline int HIGHBIT(
	DWORD dw)
{
	static const unsigned int table[] =
	{
		0,	1,	2,	2,	3,	3,	3,	3,	4,	4,	4,	4,	4,	4,	4,	4,
		5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,
		6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,
		6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,
		7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,
		7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,
		7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,
		7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,
	};

	if ((dw & 0xffff0000) == 0)
	{
		return ((dw & 0x0000ff00) == 0 ? table[dw & 0xff] : table[(dw >> 8) & 0xff] + 8);
	}
	else
	{
		return ((dw & 0xff000000) == 0 ? table[(dw >> 16) & 0xff] + 16 : table[dw >> 24] + 24);
	}
}

inline DWORD NEXTPOWEROFTWO(
	DWORD dw)
{
	dw--;
	dw |= dw >> 1;
	dw |= dw >> 2;
	dw |= dw >> 4;
	dw |= dw >> 8;
	dw |= dw >> 16;
	dw++;
	return dw;
}

inline DWORD INTERLEAVE(
	DWORD x,
	DWORD y)
{
	const DWORD B[] = { 0x55555555, 0x33333333, 0x0f0f0f0f, 0x00ff00ff };
	const DWORD S[] = { 1, 2, 4, 8 };
	x = (x | (x << S[3])) & B[3];
	y = (y | (y << S[3])) & B[3];
	x = (x | (x << S[2])) & B[2];
	y = (y | (y << S[2])) & B[2];
	x = (x | (x << S[1])) & B[1];
	y = (y | (y << S[1])) & B[1];
	x = (x | (x << S[0])) & B[0];
	y = (y | (y << S[0])) & B[0];
	return (x | (y << 1));
}

inline BOOL HASZEROBYTE(
	DWORD dw)
{
	return (dw - 0x01010101UL) & (~dw & 0x80808080UL);
}

inline WORD FLIP(
	const WORD w)
{
	return (w << 8) | (w >> 8);
}

inline DWORD FLIP(
	const DWORD dw)
{
	return (dw >> 24) | ((dw >> 8) & 0x0000ff00) | ((dw << 8) & 0x00ff0000) | (dw << 24);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define INTRINSICS

#ifdef INTRINSICS
  #ifdef _M_X64
    #define INTRINSICS64
  #endif
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <typename T>
inline int HIGHBIT_BS(
	T value)
{
	if (value == 0)
	{
		return -1;
	}

	unsigned long lsb = 0;
	unsigned long msb = sizeof(T) * 8;
	while (lsb + 1 != msb)
	{
		unsigned long mid = (lsb + msb) / 2;
		if (value & (((T)-1) << mid))
		{
			lsb = mid;
		}
		else
		{
			msb = mid;
		}
	}
	return msb - 1;
}


// ---------------------------------------------------------------------------
// returns high bit (returns 0 if value is 0)
// ---------------------------------------------------------------------------
inline int HIGHBIT32(
	unsigned long value)
{
#ifdef INTRINSICS
	unsigned long index;
	if (BitScanReverse(&index, value) == 0)
	{
		return -1;
	}
	return index;
#else
	return HIGHBIT_BS(value);
#endif
}


// ---------------------------------------------------------------------------
// returns high bit (returns 0 if value is 0)
// ---------------------------------------------------------------------------
inline int HIGHBIT64(
	unsigned __int64 value)
{
#ifdef INTRINSICS64 
	unsigned long index;
	if (BitScanReverse64(&index, value) == 0)
	{
		return -1;
	}
	return index;
#else
	return HIGHBIT_BS(value);
#endif
}


// ---------------------------------------------------------------------------
// calculate minimum number of bits required to store a value
// = index of high bit + 1
// ---------------------------------------------------------------------------
template <typename T>
inline unsigned int BITS_TO_STORE(
	T value)
{
	if (value == 0)
	{
		return 1;
	}
	int nWorkaround = 0; //workaround for compile time constant being banned.
	if (sizeof(value) <= 4 + nWorkaround)
	{
		return HIGHBIT32((unsigned long)value) + 1;
	}
	else if (sizeof(value) <= 8 + nWorkaround)
	{
		return HIGHBIT64((unsigned __int64)value) + 1;
	}
	else
	{
		ASSERT(nWorkaround);
		return 0;	// error
	}
}


// ---------------------------------------------------------------------------
// calculate minimum number of bits required to store a value
// = index of high bit + 1
// ---------------------------------------------------------------------------
template <typename T>
inline unsigned int BITS_TO_STORE_SLOW(
	T value)
{
	T mask = (T)1 << (sizeof(value) * 8 - 1);
	for (unsigned int ii = sizeof(value) * 8; mask; mask >>= 1, --ii)
	{
		if (value & mask)
		{
			return ii;
		}
	}
	return 1;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <typename T>
inline unsigned int RANDOMIZE(
	T value)
{
	static const DWORD terms[64] = 
	{
		2236229587, 3048548373, 4114882211,  207410969, 2925394773, 1341440656, 1610930935, 1784765144,
		2692815487, 2780852923, 1158635073, 3134363713,  958520643, 2877977792, 2414922420, 2842179254,
		 972381013, 1163084218, 3245904054, 1745235948, 2576779639, 4196270049, 3630419756, 1379565238,
		1403967141,  782121146, 1069546844, 4265699926,  840900720, 3594423155, 328316617,  1488429974,
		3209524835, 2083452463, 4257689376,  694245496, 3815178133, 1313404260, 2119549358, 1739602889,
		2752195112, 4130508084, 1674571134, 3918872846, 1604702094, 3647170523, 2903525541, 2660831363,
		2256059701, 3596384636, 3369384885, 4006458293, 2003673618, 3876417372, 203675262,  2156965897,
		2814483599, 3665849029, 1563171314, 1726641170, 2404562614,  811343846, 3425570982,  754902330
	};
	int result = 0;
	for (T ii = 0; ii < sizeof(T) * 8; ++ii)
	{
		if (value & ((T)1 << ii))
		{
			result += terms[ii];
		}
	}
	return result;
}


// ---------------------------------------------------------------------------
// first bit has value 1 (returns 0), last bit has value 2 ^ 31 (returns 31)
// no set bits returns -1
// uses DeBruijn sequence and 2^n isolation, perhaps slightly slower than BSR
// and inline assembly
// ---------------------------------------------------------------------------
inline unsigned int LEAST_SIGNIFICANT_BIT(
	unsigned int x)
{
	static const unsigned int lookup[] =
	{
		31,  0, 22,  1, 28, 23, 13,  2,
		29, 26, 24, 17, 19, 14,  9,  3,
		30, 21, 27, 12, 25, 16, 18,  8,
		20, 11, 15,  7, 10,  6,  5,  4
	};
	static const unsigned int DeBruijn_2_5 = 263826514;
	
	if (0 != x)
	{
		return lookup[((x & (-(int)x)) * DeBruijn_2_5) >> 27];
	}
	return (unsigned int)-1;
}


// ---------------------------------------------------------------------------
// finds the lowest value set bit
// ---------------------------------------------------------------------------
inline unsigned int LEAST_SIGNIFICANT_BIT(
	UINT64 x)
{
	static const unsigned int lookup[] =
	{
		63,  0, 47,  1, 56, 48, 27,  2,
		60, 57, 49, 41, 37, 28, 16,  3,
		61, 54, 58, 35, 52, 50, 42, 21,
		44, 38, 32, 29, 23, 17, 11,  4,
		62, 46, 55, 26, 59, 40, 36, 15,
		53, 34, 51, 20, 43, 31, 22, 10,
		45, 25, 39, 14, 33, 19, 30,  9,
		24, 13, 18,  8, 12,  7,  6,  5
	};
	static const UINT64 DeBruijn_2_6 = 571740426102773010ULL;

	if (0 != x)
	{
		unsigned int index = (unsigned int)(((x & (-(INT64)x)) * DeBruijn_2_6) >> 58);
		__assume(index < 64);
		return lookup[index];
	}
	return (unsigned int)-1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void FFSTest(
	void);

void BSearchTest(
	void);
#endif


//----------------------------------------------------------------------------
//  class BIT_BUF
//	bit buffer
//----------------------------------------------------------------------------
class BIT_BUF
{
private:
	BYTE *				m_buf;												// byte buffer
	unsigned int		m_cur;												// current bit
	unsigned int		m_len;												// length of buffer in bits

	//------------------------------------------------------------------------
	// push bits from an unsigned int onto the buffer
	void sPushUInt(
		unsigned int val,
		unsigned int bits)
	{
		BYTE * curbuf = m_buf + m_cur / 8;
		unsigned int writebits = MIN(8 - (m_cur % 8), bits);
		curbuf[0] &= (BYTE)(~(((1 << writebits) - 1) << (m_cur % 8)));		// clear writebits bits from buffer
		curbuf[0] |= (BYTE)((val & ((1 << writebits) - 1)) << (m_cur % 8));
		val >>= writebits;
		bits -= writebits;

		unsigned int writebytes = ((bits + 7) / 8);
		switch (writebytes)
		{
		case 0:
			break;
		case 1:
			curbuf[1] &= (BYTE)(~((1 << bits) - 1));
			curbuf[1] |= (BYTE)(val & ((1 << bits) - 1));
			break;
		case 2:
			curbuf[1] = (BYTE)(val & 0xff);
			curbuf[2] &= (BYTE)(~((1 << (bits - 8)) - 1));
			curbuf[2] |= (BYTE)((val >> 8) & 0xff);
			break;
		case 3:
			curbuf[1] = (BYTE)(val & 0xff);
			curbuf[2] = (BYTE)((val >> 8) & 0xff);
			curbuf[3] &= (BYTE)(~((1 << (bits - 16)) - 1));
			curbuf[3] |= (BYTE)((val >> 16) & 0xff);
			break;
		case 4:
			curbuf[1] = (BYTE)(val & 0xff);
			curbuf[2] = (BYTE)((val >> 8) & 0xff);
			curbuf[3] = (BYTE)((val >> 16) & 0xff);
			curbuf[4] &= (BYTE)(~((1 << (bits - 24)) - 1));
			curbuf[4] |= (BYTE)((val >> 24) & 0xff);
			break;
		default:
			__assume(0);
		}
		m_cur += bits + writebits;
	}

	//------------------------------------------------------------------------
	// pop bits from the buffer and return as an unsigned int
	unsigned int sPopUInt(
		unsigned int bits)
	{
		BYTE * curbuf = m_buf + m_cur / 8;
		unsigned int readbits = MIN(8 - (m_cur % 8), bits);
		unsigned int val = ((curbuf[0] >> (m_cur % 8)) & ((1 << readbits) - 1));
		bits -= readbits;

		unsigned int readbytes = ((bits + 7) / 8);
		switch (readbytes)
		{
		case 0:
			break;
		case 1:
			val |= ((unsigned int)(curbuf[1] & ((1 << bits) - 1)) << readbits);
			break;
		case 2:
			val |= ((unsigned int)curbuf[1] << readbits);
			val |= (((unsigned int)curbuf[2] & ((1 << (bits - 8)) - 1)) << (readbits + 8));
			break;
		case 3:
			val |= ((unsigned int)curbuf[1] << readbits);
			val |= ((unsigned int)curbuf[2] << (readbits + 8));
			val |= (((unsigned int)curbuf[3] & ((1 << (bits - 16)) - 1)) << (readbits + 16));
			break;
		case 4:
			val |= ((unsigned int)curbuf[1] << readbits);
			val |= ((unsigned int)curbuf[2] << (readbits + 8));
			val |= ((unsigned int)curbuf[3] << (readbits + 16));
			val |= (((unsigned int)curbuf[4] & ((1 << (bits - 24)) - 1)) << (readbits + 24));
			break;
		default:
			__assume(0);
		}

		m_cur += bits + readbits;
		return val;
	}

	//------------------------------------------------------------------------
	// push bits from a byte onto the buffer
	void sPushByte(
		BYTE val,
		unsigned int bits)
	{
		if (bits == 0)
		{
			return;
		}
		unsigned int curbits = 8 - (m_cur % 8);								// bits remaining in current byte
		if (bits <= curbits)
		{
			unsigned int writebits = MIN(curbits, bits);
			m_buf[m_cur / 8] &= (BYTE)(~(((1 << writebits) - 1) << (m_cur % 8)));
			m_buf[m_cur / 8] |= (BYTE)((val & (BYTE)((1 << writebits) - 1)) << (m_cur % 8));
		}
		else
		{
			BYTE * curbuf = m_buf + m_cur / 8;
			unsigned int writebits = MIN(curbits, bits);
			curbuf[0] &= (BYTE)(~(((1 << writebits) - 1) << (m_cur % 8)));	// clear writebits bits from buffer
			curbuf[0] |= (BYTE)((val & (BYTE)((1 << writebits) - 1)) << (m_cur % 8));
			curbuf[1] &= (BYTE)(~((1 << (bits - writebits)) - 1));
			curbuf[1] |= (BYTE)((val >> writebits) & (BYTE)((1 << (bits - writebits)) - 1));
		}
		m_cur += bits;
	}

	//------------------------------------------------------------------------
	// pop bits from the buffer and return a byte
	BYTE sPopByte(
		unsigned int bits)
	{
		if (bits == 0)
		{
			return 0;
		}
		BYTE val;
		unsigned int curbits = 8 - (m_cur % 8);								// bits remaining in current byte
		if (bits <= curbits)
		{
			unsigned int readbits = MIN(curbits, bits);
			val = (BYTE)((m_buf[m_cur / 8] >> (m_cur % 8)) & ((1 << readbits) - 1));
		}
		else
		{
			BYTE * curbuf = m_buf + m_cur / 8;
			unsigned int readbits = MIN(curbits, bits);
			val = (BYTE)((curbuf[0] >> (m_cur % 8)) & ((1 << readbits) - 1));
			val |= (BYTE)((curbuf[1] & ((1 << (bits - readbits)) - 1)) << readbits);
		}
		m_cur += bits;
		return val;
	}

	//------------------------------------------------------------------------
	// push a bunch of bits onto the buffer, where startbit is an offset into the source buf from which we start to read
	void sPushBuf(
		BYTE * buf,
		unsigned int bits,
		unsigned int startbit)
	{
		buf += startbit / 8;												// advance to start byte
		unsigned int writebits = MIN(bits, 8 - (startbit % 8));	
		sPushByte((BYTE)(buf[0] >> (startbit % 8)), writebits);
		bits -= writebits;

		unsigned int writebytes = (bits / 8);
		for (unsigned int ii = 1; ii < writebytes + 1; ++ii)
		{
			sPushByte(buf[ii], 8);
		}

		sPushByte(buf[writebytes + 1], bits % 8);
	}

	//------------------------------------------------------------------------
	// pop a bunch of bits from the buffer into a byte buffer, startbit is offset into destination buffer
	void sPopBuf(
		BYTE * buf,
		unsigned int bits,
		unsigned int startbit)
	{
		buf += startbit / 8;												// advance to start byte
		unsigned int readbits = MIN(bits, 8 - (startbit % 8));	
		BYTE byte = sPopByte(readbits);
		buf[0] &= (BYTE)(~(((1 << readbits) - 1) << (startbit % 8)));		// clear readbits bits from buffer
		buf[0] |= (BYTE)(byte << (startbit % 8));
		bits -= readbits;

		unsigned int readbytes = (bits / 8);
		for (unsigned int ii = 1; ii < readbytes + 1; ++ii)
		{
			buf[ii] = sPopByte(8);
		}

		buf[readbytes + 1] &= (BYTE)(~(((1 << (bits % 8)) - 1)));
		buf[readbytes + 1] |= sPopByte(bits % 8);
	}

public:
	//------------------------------------------------------------------------
	BIT_BUF(
		BYTE * buf,
		unsigned int bytes) : m_buf(buf), m_cur(0), m_len(bytes * 8)
	{
		ASSERT(m_buf != NULL);
	}

	//------------------------------------------------------------------------
	// push an unsigned value
	template <typename T>
	BOOL PushUInt(
		T value,																// value to store
		unsigned int bits														// number of bits to use to store value
		BBD_FILE_LINE_ARGS)
{
		DBG_ASSERT_RETFALSE(bits > 0 && bits <= 32);								// assert bits is in valid range
		BBD_FL_ASSERT_RETFALSE(m_len - m_cur >= bits);							// assert enough room left in buffer to write value
		BBD_FL_ASSERT_RETFALSE(bits == 32 || (unsigned int)value < (unsigned int)(1 << bits));		// assert value is in range of bits

		sPushUInt(value, bits);

		return TRUE;
	}

	//------------------------------------------------------------------------
	// pop an unsigned value
	template <typename T>
	__checkReturn BOOL PopUInt(
		T * value, 
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		DBG_ASSERT_RETFALSE(bits > 0 && bits <= 32);								// assert bits is in valid range
		BBD_FL_ASSERT_RETFALSE(m_len - m_cur >= bits);							// assert enough room left in buffer for value

		*value = (T)sPopUInt(bits);;

		return TRUE;
	}

	//------------------------------------------------------------------------
	// push a signed value
	template <typename T>
	BOOL PushInt(
		T value,																// value to store
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		DBG_ASSERT_RETFALSE(bits > 0 && bits <= 32);								// assert bits is in valid range
		BBD_FL_ASSERT_RETFALSE(m_len - m_cur >= bits);							// assert enough room left in buffer to write value

		BBD_FL_ASSERT_RETFALSE(value >= -(1 << (bits - 1)));					// assert value is in range of bits
		BBD_FL_ASSERT_RETFALSE(value < (1 << (bits - 1)));

		unsigned int val = (unsigned int)value + (1 << (bits - 1));
		sPushUInt(val, bits);

		return TRUE;
	}

	//------------------------------------------------------------------------
	// pop a signed value
	template <typename T>
	__checkReturn BOOL PopInt(
		T * value, 
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		DBG_ASSERT_RETFALSE(bits > 0 && bits <= 32);								// assert bits is in valid range
		BBD_FL_ASSERT_RETFALSE(m_len - m_cur >= bits);							// assert enough room left in buffer for value

		unsigned int val = sPopUInt(bits);

		*value = (T)((int)val - (1 << (bits - 1)));

		return TRUE;
	}

	//------------------------------------------------------------------------
	// shortcut to get an unsigned int off the buffer
	unsigned int GetUInt(
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		unsigned int retval;
		if (!PopUInt(&retval, bits  BBD_FL))
		{
			return 0;
		}
		return retval;
	}

	//------------------------------------------------------------------------
	// shortcut to get a signed int off the buffer
	int GetInt(
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		int retval;
		if (!PopInt(&retval, bits  BBD_FL))
		{
			return 0;
		}
		return retval;
	}

	//------------------------------------------------------------------------
	// push a bunch of bits from a buffer
	BOOL PushBuf(
		BYTE * buf,
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		BBD_FL_ASSERT_RETFALSE(buf);
		BBD_FL_ASSERT_RETFALSE(bits > 0);
		BBD_FL_ASSERT_RETFALSE(m_cur + bits <= m_len);

		sPushBuf(buf, bits, 0);
		return TRUE;
	}

	//------------------------------------------------------------------------
	// push a bunch of bits from a buffer
	BOOL PushBuf(
		BYTE * buf,
		unsigned int bits,
		unsigned int startbit
		BBD_FILE_LINE_ARGS)
	{
		BBD_FL_ASSERT_RETFALSE(buf);
		BBD_FL_ASSERT_RETFALSE(bits > 0);
		BBD_FL_ASSERT_RETFALSE(m_cur + bits <= m_len);

		sPushBuf(buf, bits, startbit);
		return TRUE;
	}

	//------------------------------------------------------------------------
	// pop a bunch of bits off a buffer
	__checkReturn BOOL PopBuf(
		BYTE * buf,
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		BBD_FL_ASSERT_RETFALSE(buf);
		BBD_FL_ASSERT_RETFALSE(bits > 0);
		BBD_FL_ASSERT_RETFALSE(m_cur + bits <= m_len);

		sPopBuf(buf, bits, 0);
		return TRUE;
	}

	//------------------------------------------------------------------------
	// pop a bunch of bits off a buffer
	__checkReturn BOOL PopBuf(
		BYTE * buf,
		unsigned int bits,
		unsigned int startbit
		BBD_FILE_LINE_ARGS)
	{
		BBD_FL_ASSERT_RETFALSE(buf);
		BBD_FL_ASSERT_RETFALSE(bits > 0);
		BBD_FL_ASSERT_RETFALSE(m_cur + bits <= m_len);

		sPopBuf(buf, bits, startbit);
		return TRUE;
	}

	//------------------------------------------------------------------------
	inline BYTE * GetPtr(
		void) const
	{
		ASSERT_RETNULL((m_cur & 7) == 0);
		return m_buf + (m_cur / 8);
	};

	//------------------------------------------------------------------------
	// get the current length in bytes of the buffer
	unsigned int GetLen(
		void) const
	{
		return (m_cur + 7) / 8;
	};

	//------------------------------------------------------------------------
	// get allocated length of buffer
	unsigned int GetAllocatedLen(
		void) const
	{
		return m_len;
	};

	//------------------------------------------------------------------------
	// get the current bit in the buffer
	unsigned int GetCursor(
		void) const
	{
		return m_cur;
	};

	//------------------------------------------------------------------------
	// set the current bit in the buffer
	BOOL SetCursor(
		unsigned int pos)
	{
		ASSERT_RETFALSE(pos < m_len);
		m_cur = pos;
		return TRUE;
	};
	
	//------------------------------------------------------------------------
	// zero buffer
	void ZeroBuffer(
		void)
	{
		memset(m_buf, 0, sizeof(BYTE) * m_len / 8);
	}
};


//----------------------------------------------------------------------------
//  class BYTE_BUF
//----------------------------------------------------------------------------
class BYTE_BUF
{
private:
	BYTE *				buf;													// byte buffer
	unsigned int		cur;													// current byte
	unsigned int		len;													// length in bytes
	BOOL				bDynamic;
	struct MEMORYPOOL*	pool;

	BOOL Resize(
		unsigned int bytes)
	{
		if ( bDynamic )
		{
			if (cur + bytes <= len)
			{
				return TRUE;
			}
			while (cur + bytes >= len)
			{
				len *= 2;
			}
			buf = (BYTE*)REALLOC(pool, buf, len);
			if ( !buf )
			{
				len = 0;
				ASSERT_RETFALSE( buf );
			}
		}
		return TRUE;
	}

public:

	BYTE_BUF(
		struct MEMORYPOOL* _pool = NULL,
		unsigned int startlen = 256 )
	{
		bDynamic = TRUE;
		pool = _pool;
		len = startlen;
		buf = (BYTE*)MALLOC(pool, len);
		cur = 0;

		if (!buf)
		{
			len = 0;
			ASSERT( buf );
		}
	}

	~BYTE_BUF(
		void)
	{
		if ( bDynamic )
		{
			FREE(pool, buf);
			buf = NULL;
		}
	}

	BYTE_BUF(
		void * buffer,
		unsigned int length)
	{
		buf = (BYTE *)buffer;
		cur = 0;
		len = length;
		bDynamic = FALSE;
		pool = NULL;
	}

	template <typename T>
	inline BOOL PushInt(
		T value)
	{
		ASSERT_RETFALSE( Resize( sizeof( T ) ) );
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*(T *)(buf + cur) = value;
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE( Resize( sizeof( T ) ) );
		ASSERT_RETFALSE(bytes == sizeof(T));									// temporary
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*(T *)(buf + cur) = value;
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUInt(
		T value)
	{
		ASSERT_RETFALSE( Resize( sizeof( T ) ) );
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*(T *)(buf + cur) = value;
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUInt(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE( Resize( sizeof( T ) ) );
		ASSERT_RETFALSE(bytes == sizeof(T));									// temporary
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(value) <= len);
		*(T *)(buf + cur) = value;
		cur += sizeof(T);
		return TRUE;
	}

	inline BOOL PushBuf(
		const void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE( Resize( length ) );
		ASSERT_RETFALSE(cur + length <= len);
		memcpy(buf + cur, (BYTE *)buffer + startbyte, length);
		cur += length;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopInt(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*value = *(T *)(buf + cur);
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUInt(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*value = *(T *)(buf + cur);
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopInt(
		T * value,
		unsigned int bytes)
	{
        UNREFERENCED_PARAMETER(bytes);
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*value = *(T *)(buf + cur);
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUInt(
		T * value,
		unsigned int bytes)
	{
        UNREFERENCED_PARAMETER(bytes);
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);
		*value = *(T *)(buf + cur);
		cur += sizeof(T);
		return TRUE;
	}

	inline __checkReturn BOOL PopBuf(
		void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE(cur + length <= len);
		memcpy((BYTE *)buffer + startbyte, buf + cur, length);
		cur += length;
		return TRUE;
	}

	// return remaining buffer space
	inline unsigned int GetRemainder(
		void)
	{
		return len - cur;
	};

	// return # of bytes written
	inline unsigned int GetLen(
		void)
	{
		return cur;
	};

	// return # of bytes allocated
	inline unsigned int GetAllocatedLen(
		void)
	{
		return len;
	};

	// return # of bytes written
	inline unsigned int GetCursor(
		void)
	{
		return cur;
	};

	inline void SetCursor(
		unsigned int pos)
	{
		ASSERT_RETURN(pos < len);
		cur = pos;
	};

	inline BYTE * GetPtr(
		void)
	{
		return buf + cur;
	}
	
	inline void ZeroBuffer(
		void)
	{
		memset( buf, 0, sizeof( BYTE ) * len );
	}	
};


//----------------------------------------------------------------------------
//  class BYTE_BUF_OBFUSCATED
//----------------------------------------------------------------------------
class BYTE_BUF_OBFUSCATED
{
private:
	static const unsigned int MAX_SEQ_LEN = 32;

	BYTE *				buf;													// byte buffer
	unsigned int		cur;													// current byte
	unsigned int		len;													// length in bytes
	
	unsigned int		obs_mult;												// obfuscation multiplier
	unsigned int		obs_add;												// obfuscation adder
	unsigned int		obs_seqlen;												// obfuscation sequence length (in DWORDS)
	DWORD				obs_cache[MAX_SEQ_LEN];									// cache of obs values
	unsigned int		obs_start;												// cache start (bytes)


	void GenObfuscationMask(
		void)
	{
		unsigned int start = (cur / (obs_seqlen * sizeof(DWORD))) * (obs_seqlen * sizeof(DWORD));
		if (obs_start == start)
		{
			return;
		}
		obs_start = start;
		DWORD dw = ((start + 666) * obs_mult) + obs_add;
		obs_cache[0] = dw;
		for (unsigned int ii = 1; ii < obs_seqlen; ++ii)
		{
			dw = dw * obs_mult + obs_add;
			obs_cache[ii] = dw;
		}
	}

	inline BYTE GetObfuscationByte(
		void)
	{
		GenObfuscationMask();
		return ((BYTE *)obs_cache)[(cur - obs_start)];
	}

	inline BYTE ObfuscateByte(
		BYTE b)
	{
		BYTE obsByte = GetObfuscationByte();
		++cur;
		return (BYTE)((b + obsByte));
	}

	inline BYTE UnobfuscateByte(
		BYTE b)
	{
		BYTE obsByte = GetObfuscationByte();
		++cur;
		return (BYTE)((b - obsByte));	
	}

	inline void PushNull(
		unsigned int count)
	{
		if (count == 0)
		{
			return;
		}
		BYTE * dest = buf + cur;
		BYTE * end = dest + count;
		for (; dest < end; ++dest)
		{
			*dest = ObfuscateByte(0);
		}
	}

	inline void PushNullNoObs(
		unsigned int count)
	{
		if (count == 0)
		{
			return;
		}
		BYTE * dest = buf + cur;
		BYTE * end = dest + count;
		for (; dest < end; ++dest)
		{
			*dest = 0;
		}
		cur += count;
	}

public:
	BYTE_BUF_OBFUSCATED(
		void * buffer,
		unsigned int length,
		unsigned int mult,
		unsigned int add,
		unsigned int seqlen) : buf((BYTE *)buffer), cur(0), len(length), obs_mult(mult), obs_add(add), obs_seqlen(seqlen), obs_start((unsigned int)-1)
	{
		ASSERT_DO(obs_seqlen > 0 && obs_seqlen <= MAX_SEQ_LEN)
		{
			obs_seqlen = MAX_SEQ_LEN;
		}
	}

	BYTE_BUF_OBFUSCATED(
		void * buffer,
		unsigned int length,
		unsigned int mult,
		unsigned int add) : buf((BYTE *)buffer), cur(0), len(length), obs_mult(mult), obs_add(add), obs_seqlen(MAX_SEQ_LEN), obs_start((unsigned int)-1)
	{
	}

	inline BOOL PushBuf(
		const void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE(cur + length <= len);
		
		BYTE * src = (BYTE *)buffer + startbyte;
		BYTE * end = src + length;
		BYTE * dest = buf + cur;
		for (; src < end; ++src, ++dest)
		{
			*dest = ObfuscateByte(*src);
		}
		return TRUE;
	}

	inline __checkReturn BOOL PopBuf(
		void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE(cur + length <= len);

		BYTE * src = (BYTE *)buf + cur;
		BYTE * end = src + length;
		BYTE * dest = (BYTE *)buffer + startbyte;
		for (; src < end; ++src, ++dest)
		{
			*dest = UnobfuscateByte(*src);
		}
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUInt(
		T value)
	{
		ASSERT_RETFALSE(cur + sizeof(T) <= len);
		BYTE * dest = buf + cur;
		BYTE * end = dest + sizeof(T);
		while (dest < end)
		{
			*dest = ObfuscateByte(BYTE(value & 0xff));
			dest++;
			value >>= 8;
		}
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value)
	{
		ASSERT_RETFALSE(cur + sizeof(T) <= len);
		BYTE * dest = buf + cur;
		BYTE * end = dest + sizeof(T);
		while (dest < end)
		{
			*dest = ObfuscateByte(BYTE(value & 0xff));
			dest++;
			value >>= 8;
		}
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUInt(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			PushNull(bytes - sizeof(T));
			return PushUInt(value);
		}
		ASSERT_RETFALSE(value < ((T)1 << (bytes * 8 )))
		BYTE * dest = buf + cur;
		BYTE * end = dest + bytes;
		while (dest < end)
		{
			*dest = ObfuscateByte(BYTE(value & 0xff));
			dest++;
			value >>= 8;
		}
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			PushNull(bytes - sizeof(T));
			return PushInt(value);
		}
		ASSERT_RETFALSE(value >= -((T)1 << (bytes * 8 - 1)));
		ASSERT_RETFALSE(value < ((T)1 << (bytes * 8 - 1)));
		value += ((T)1 << (T)(bytes * 8 - 1));
		BYTE * dest = buf + cur;
		BYTE * end = dest + bytes;
		while (dest < end)
		{
			*dest = ObfuscateByte(BYTE(value & 0xff));
			dest++;
			value >>= 8;
		}
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUInt(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + sizeof(T);
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (UnobfuscateByte(*src) << (T)shift);
		}
		*value = val;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopInt(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + sizeof(T);
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (UnobfuscateByte(*src) << (T)shift);
		}
		*value = val;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUInt(
		T * value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes > sizeof(T))
		{
			unsigned int count = bytes - sizeof(T);
			cur += count;
			bytes -= count;
		}

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + bytes;
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= ((T)UnobfuscateByte(*src) << (T)shift);
		}
		*value = val;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopInt(
		T * value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			unsigned int count = bytes - sizeof(T);
			cur += count;
			bytes -= count;
			return PopInt(value);
		}

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + bytes;
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (UnobfuscateByte(*src) << (T)shift);
		}
		val -= ((T)1 << (T)(bytes * 8 - 1));
		*value = val;
		return TRUE;
	}

	inline BOOL PushBufNoObs(
		const void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE(cur + length <= len);
		
		BYTE * src = (BYTE *)buffer + startbyte;
		BYTE * end = src + length;
		BYTE * dest = buf + cur;
		for (; src < end; ++src, ++dest)
		{
			*dest = *src;
		}
		cur += length;
		return TRUE;
	}

	inline __checkReturn BOOL PopBufNoObs(
		void * buffer,
		unsigned int length,
		unsigned int startbyte = 0)
	{
		ASSERT_RETFALSE(cur + length <= len);

		BYTE * src = (BYTE *)buf + cur;
		BYTE * end = src + length;
		BYTE * dest = (BYTE *)buffer + startbyte;
		for (; src < end; ++src, ++dest)
		{
			*dest = *src;
		}
		cur += length;
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUIntNoObs(
		T value)
	{
		ASSERT_RETFALSE(cur + sizeof(T) <= len);
		BYTE * dest = buf + cur;
		BYTE * end = dest + sizeof(T);
		while (dest < end)
		{
			*dest = BYTE(value & 0xff);
			dest++;
			value >>= 8;
		}
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushIntNoObs(
		T value)
	{
		ASSERT_RETFALSE(cur + sizeof(T) <= len);
		BYTE * dest = buf + cur;
		BYTE * end = dest + sizeof(T);
		while (dest < end)
		{
			*dest = BYTE(value & 0xff);
			dest++;
			value >>= 8;
		}
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushUIntNoObs(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			PushNullNoObs(bytes - sizeof(T));
			return PushUIntNoObs(value);
		}
		ASSERT_RETFALSE(value < ((T)1 << (bytes * 8 )))
		BYTE * dest = buf + cur;
		BYTE * end = dest + bytes;
		while (dest < end)
		{
			*dest = BYTE(value & 0xff);
			dest++;
			value >>= 8;
		}
		cur += bytes;
		return TRUE;
	}

	template <typename T>
	inline BOOL PushIntNoObs(
		T value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			PushNullNoObs(bytes - sizeof(T));
			return PushIntNoObs(value);
		}
		ASSERT_RETFALSE(value >= -((T)1 << (bytes * 8 - 1)));
		ASSERT_RETFALSE(value < ((T)1 << (bytes * 8 - 1)));
		value += ((T)1 << (T)(bytes * 8 - 1));
		BYTE * dest = buf + cur;
		BYTE * end = dest + bytes;
		while (dest < end)
		{
			*dest = BYTE(value & 0xff);
			dest++;
			value >>= 8;
		}
		cur += bytes;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUIntNoObs(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + sizeof(T);
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (*src << (T)shift);
		}
		*value = val;
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopIntNoObs(
		T * value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= len);

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + sizeof(T);
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (*src << (T)shift);
		}
		*value = val;
		cur += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopUIntNoObs(
		T * value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes > sizeof(T))
		{
			unsigned int count = bytes - sizeof(T);
			cur += count;
			bytes -= count;
		}

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + bytes;
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (*src << (T)shift);
		}
		*value = val;
		cur += bytes;
		return TRUE;
	}

	template <typename T>
	inline __checkReturn BOOL PopIntNoObs(
		T * value,
		unsigned int bytes)
	{
		ASSERT_RETFALSE(cur + bytes <= len);
		if (bytes >= sizeof(T))
		{
			unsigned int count = bytes - sizeof(T);
			cur += count;
			bytes -= count;
			return PopIntNoObs(value);
		}

		T val = 0;
		BYTE * src = buf + cur;
		BYTE * end = src + bytes;
		unsigned int shift = 0;
		for (; src < end; ++src, shift += 8)
		{
			val |= (*src << (T)shift);
		}
		val -= ((T)1 << (T)(bytes * 8 - 1));
		*value = val;
		cur += bytes;
		return TRUE;
	}

	// set mult and add
	void SetMultAdd(
		unsigned int mult,
		unsigned int add)
	{
		obs_mult = mult;
		obs_add = add;
	}

	// return remaining buffer space
	inline unsigned int GetRemainder(
		void)
	{
		return len - cur;
	};

	// return # of bytes written
	inline unsigned int GetLen(
		void)
	{
		return cur;
	};

	// return # of bytes written
	inline unsigned int GetCursor(
		void)
	{
		return cur;
	};

	inline void SetCursor(
		unsigned int pos)
	{
		ASSERT_RETURN(pos <= len);
		cur = pos;
	};

	inline BYTE * GetPtr(
		void)
	{
		return buf + cur;
	}
	
	inline void ZeroBuffer(
		void)
	{
		memset(buf, 0, sizeof(BYTE) * len);
	}	
};


//----------------------------------------------------------------------------
//  class BYTE_BUF -- implements endian conversion
//----------------------------------------------------------------------------
struct BYTE_BUF_NET
{
	BYTE *			buf;
	unsigned int	cur;
	unsigned int	len;

	BYTE_BUF_NET(
		void * buffer,
		unsigned int length)
	{
		buf = (BYTE *)buffer;
		cur = 0;
		len = length;
	}

	// return remaining buffer space
	inline unsigned int GetRemainder(
		void)
	{
		return (unsigned int )len - cur;
	};

	// return # of bytes written
	inline unsigned int GetLen(
		void)
	{
		return cur;
	};

	// return # of bytes written
	inline unsigned int GetCursor(
		void)
	{
		return cur;
	};

	inline void SetCursor(
		unsigned int pos)
	{
		ASSERT_RETURN(pos <= len);
		cur = pos;
	};

	inline BYTE * GetPtr(
		void)
	{
		return buf + cur;
	}

	inline BYTE * GetBuf(
		void)
	{
		return buf;
	}

	inline void SetBuf(
		BYTE * buffer,
		unsigned int length)
	{
		ASSERT(cur == 0);
		buf = buffer;
		len = length;
	}

	template <typename T>
	BOOL Push(
		T value)
	{
		ASSERT_RETFALSE(cur + (unsigned int )sizeof(value) <= len);
#ifndef _BIG_ENDIAN
		*(T *)(buf + cur) = value;
#else
		*(T *)(buf + cur) = FLIP(value);
#endif
		cur += (unsigned int)sizeof(value);
		return TRUE;
	}

	BOOL Push(
		const WCHAR * str,
		unsigned int count)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(WCHAR) <= len);
		int maxlen = (MIN(count * (unsigned int)sizeof(WCHAR), len - cur) - 1) / (unsigned int)sizeof(WCHAR);
		WCHAR * start = (WCHAR *)(buf + cur);
		WCHAR * sz_cur = start;
		WCHAR * sz_end = start + maxlen;
		while (sz_cur < sz_end && *str)
		{
#ifndef _BIG_ENDIAN
			*sz_cur++ = *str++;
#else
			((BYTE *)sz_cur)[0] = ((BYTE *)str)[1];
			((BYTE *)sz_cur)[1] = ((BYTE *)str)[0];
			sz_cur++;
			str++;
#endif
		}
		*sz_cur++ = 0;
		cur += (unsigned int)((unsigned int)(sz_cur - start) * (unsigned int)sizeof(WCHAR));
		return TRUE;
	}

	BOOL Push(
		const char * str,
		unsigned int count)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(char) <= len);
		int maxlen = (MIN(count * (unsigned int)sizeof(char), len - cur) - 1) / (unsigned int)sizeof(char);
		char * start = (char *)(buf + cur);
		char * sz_cur = start;
		char * sz_end = start + maxlen;
		while (sz_cur < sz_end && *str)
		{
			*sz_cur++ = *str++;
		}
		*sz_cur++ = 0;
		cur += (unsigned int)((unsigned int)(sz_cur - start) * (unsigned int)sizeof(char));
		return TRUE;
	}

	BOOL Push(
		const BYTE * buffer,
		unsigned int size)
	{
		ASSERT_RETFALSE(cur + size <= len);
		memcpy(buf + cur, buffer, size);
		cur += size;
		return TRUE;
	}

	template <typename T>
	BYTE_BUF_NET & operator <<(
		T value)
	{
		Push(value);
		return *this;
	}

	template <typename T>
	__checkReturn BOOL Pop(
		T & value)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(value) <= len);
#ifndef _BIG_ENDIAN
		value = *(T *)(buf + cur);
#else
		value = FLIP(*(T *)(buf + cur));
#endif
		cur += (unsigned int)sizeof(value);
		return TRUE;
	}

	__checkReturn BOOL Pop(
		WCHAR * str,
		unsigned int count)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(WCHAR) <= len);
		int maxlen = (MIN(count * (unsigned int)sizeof(WCHAR), len - cur) - 1) / (unsigned int)sizeof(WCHAR);
		WCHAR * start = (WCHAR *)(buf + cur);
		WCHAR * sz_cur = start;
		WCHAR * sz_end = start + maxlen;
		while (sz_cur <= sz_end && *sz_cur)
		{
#ifndef _BIG_ENDIAN
			*str++ = *sz_cur++;
#else
			((BYTE *)str)[0] = ((BYTE *)sz_cur)[1];
			((BYTE *)str)[1] = ((BYTE *)sz_cur)[0];
			sz_cur++;
			str++;
#endif
		}
		*str++ = 0;
		if (sz_cur <= sz_end)		// skip 0 term
		{
			sz_cur++;	
		}
		cur += (unsigned int)((unsigned int)(sz_cur - start) * (unsigned int)sizeof(WCHAR));
		return TRUE;
	}

	__checkReturn BOOL Pop(
		char * str,
		unsigned int count)
	{
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(char) <= len);
		int maxlen = (MIN(count * (unsigned int)sizeof(char), len - cur) - 1) / (unsigned int)sizeof(char);
		char * start = (char *)(buf + cur);
		char * sz_cur = start;
		char * sz_end = start + maxlen;
		while (sz_cur < sz_end && *sz_cur)
		{
			*str++ = *sz_cur++;
		}
		*str++ = 0;
		if (sz_cur < sz_end)		// skip 0 term
		{
			sz_cur++;	
		}
		cur += (unsigned int)((unsigned int)(sz_cur - start) * (unsigned int)sizeof(char));
		return TRUE;
	}

	__checkReturn BOOL Pop(
		BYTE * buffer,
		unsigned int size)
	{
		ASSERT_RETFALSE(cur + size <= len);
		memcpy(buffer, buf + cur, size);
		cur += size;
		return TRUE;
	}

	template <typename T>
	BYTE_BUF_NET & operator >>(
		T & value)
	{
		ASSERT(Pop(value));
		return *this;
	}
};


//----------------------------------------------------------------------------
//  dynamic array allocated along BUFSIZE
//----------------------------------------------------------------------------
template <typename T, unsigned int BUFSIZE>
struct BUF_DYNAMIC
{
	struct MEMORYPOOL *		pool;
	T *						buf;
	unsigned int			cur;
	unsigned int			len;

	BUF_DYNAMIC(void) : pool(NULL), buf(NULL), cur(0), len(0) {};
	BUF_DYNAMIC(struct MEMORYPOOL * in_pool) : pool(in_pool), buf(NULL), cur(0), len(0) {};
	~BUF_DYNAMIC(void) { ASSERT(0); };	// note if we plan on allocating BUF_DYNAMIC on the stack, change stuff so it works

	void Init(
		struct MEMORYPOOL * in_pool = NULL)
	{
		pool = in_pool;
		buf = NULL;
		cur = 0;
		len = 0;
	}

	void Free(
		void)
	{
		if (buf)
		{
			FREE(pool, buf);
			buf = NULL;
		}
		pool = NULL;
		cur = 0;
		len = 0;
	}

	void AllocateSpace(
		unsigned int size)
	{
		ASSERT(len >= cur);
		unsigned int remaining = len - cur;
		if (remaining >= size)
		{
			return;
		}
		len = ((len + size + BUFSIZE) / BUFSIZE) * BUFSIZE;
		ASSERT(len - cur >= size);
		buf = (BYTE *)REALLOC(pool, buf, len);
	}

	T * GetWriteBuffer(
		unsigned int & in_len)
	{
		AllocateSpace(BUFSIZE);
		ASSERT(len > cur);
		in_len = len - cur;
		return buf + cur;
	}

	void Advance(
		unsigned int in_len)
	{
		ASSERT(len - cur >= in_len);
		cur += in_len;
	}

	unsigned int GetSize(
		void) const
	{
		return len;
	}

	unsigned int GetPos(
		void) const
	{
		return cur;
	}

	unsigned int GetRemaining(
		void) const
	{
		return len - cur;
	}

	BYTE * GetCurBuf(
		void) const
	{
		return buf + cur;
	}

	BYTE * GetBuf(
		unsigned int pos) const
	{
		ASSERT_RETNULL(pos <= cur);
		return buf + pos;
	}
};


#endif // _BITMAP_H_
