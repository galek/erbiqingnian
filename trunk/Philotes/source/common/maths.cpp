

typedef int FLOAT32_AS_INT;

#define FLOAT32_MANTISSA_BITS	23
#define FLOAT32_EXPONENT_BITS	8
#define FLOAT32_EXPONENT_BIAS	127

#define FLOAT32_MANTISSA_MASK	((1 << FLOAT32_MANTISSA_BITS) - 1)
#define FLOAT32_EXPONENT_MASK	(((1 << FLOAT32_EXPONENT_BITS) - 1) << FLOAT32_MANTISSA_BITS)
#define FLOAT32_SIGN_MASK		(1 << (FLOAT32_MANTISSA_BITS + FLOAT32_EXPONENT_BITS))

#define FLOAT32_MANTISSA_GET(x)				((x) & FLOAT32_MANTISSA_MASK)
#define FLOAT32_EXPONENT_GET_BIASED(x)		(((x) & FLOAT32_EXPONENT_MASK) >> FLOAT32_MANTISSA_BITS)
#define FLOAT32_EXPONENT_GET_UNBIASED(x)	(FLOAT32_EXPONENT_GET_BIASED(x) - FLOAT32_EXPONENT_BIAS)

C_ASSERT(sizeof(float) == sizeof(FLOAT32_AS_INT));

//-----------------------------------------------------------------------------
//
// Math_GetInfinity - returns positive infinity.
//
//-----------------------------------------------------------------------------

float Math_GetInfinity(void)
{
	const FLOAT32_AS_INT nBits = FLOAT32_EXPONENT_MASK;
	return *static_cast<const float *>(static_cast<const void *>(&nBits));
}

//-----------------------------------------------------------------------------
//
// Math_GetNaN - returns the real indefinite quiet not-a-number.
//
//-----------------------------------------------------------------------------

float Math_GetNaN(void)
{
	const FLOAT32_AS_INT nNan = ((static_cast<FLOAT32_AS_INT>(4) << FLOAT32_EXPONENT_BITS) - 1) << (FLOAT32_MANTISSA_BITS - 1);
	return *static_cast<const float *>(static_cast<const void *>(&nNan));
}

//-----------------------------------------------------------------------------
//
// Math_IsInfinity - returns true if the input is +/- infinity.
//
//-----------------------------------------------------------------------------

bool Math_IsInfinity(float x)
{
	const FLOAT32_AS_INT n = *static_cast<const FLOAT32_AS_INT *>(static_cast<const void *>(&x));
	// Mantissa of 0 is infinity; non-zero is NaN.
	return (n & (FLOAT32_EXPONENT_MASK | FLOAT32_MANTISSA_MASK)) == FLOAT32_EXPONENT_MASK;
}

//-----------------------------------------------------------------------------
//
// Math_IsNaN - returns true if the input is not-a-number.
//
//-----------------------------------------------------------------------------

bool Math_IsNaN(float x)
{
	const FLOAT32_AS_INT n = *static_cast<const FLOAT32_AS_INT *>(static_cast<const void *>(&x));
	// Mantissa of 0 is infinity; non-zero is NaN.
	return ((n & FLOAT32_EXPONENT_MASK) == FLOAT32_EXPONENT_MASK) && ((n & FLOAT32_MANTISSA_MASK) != 0);
}

//-----------------------------------------------------------------------------
//
// Math_IsFinite - returns true if the input is neither NaN nor +/- Inf.
//
//-----------------------------------------------------------------------------

bool Math_IsFinite(float x)
{
	return ! ( Math_IsInfinity(x) || Math_IsNaN(x) );
}

//-----------------------------------------------------------------------------
//
// Math_Modf32 - extract integral and fractional parts from a 32-bit
// floating-point value.
//
// This function has been tested exhaustively to be correct for
// all inputs that are neither infinity nor NaN.
//
//-----------------------------------------------------------------------------

float Math_Modf32(const float x, float * const pIntOut)
{
	const FLOAT32_AS_INT i = *reinterpret_cast<const FLOAT32_AS_INT *>(&x);

	// Get the exponent.
	int nExp = FLOAT32_EXPONENT_GET_UNBIASED(i);

	// If the exponent is the number of mantissa bits or greater,
	// then there's no fractional portion.
	if (nExp >= FLOAT32_MANTISSA_BITS) {
		*pIntOut = x;
		int r = i & FLOAT32_SIGN_MASK;	// +/- 0
		return *reinterpret_cast<float *>(&r);
	}

	// If the exponent is less than zero, there's no integer
	// portion. This case also correctly handles denormals.
	if (nExp < 0) {
		int r = i & FLOAT32_SIGN_MASK;	// +/- 0
		*pIntOut = *reinterpret_cast<float *>(&r);
		return x;
	}

	// Separate them out: mask off the fractional portion.
	int r = (-(1 << (FLOAT32_MANTISSA_BITS - nExp))) & i;
	*pIntOut = *reinterpret_cast<float *>(&r);
	float t = x - *pIntOut;
	// Ensure the sign is right, which is only an issue
	// when the input is integral. Continue in floating
	// point space since the result is already there.
	return ((t == 0.0f) && (x < 0.0f)) ? -t : t;
}

//-----------------------------------------------------------------------------

float Math_Floor32(float x)
{
	if (x < 0.0f) {
		float f = Math_Modf32(-x, &x);
		if (f != 0.0f) {
			x += 1.0f;
		}
		return -x;
	}
	else {
		Math_Modf32(x, &x);
		return x;
	}
}
