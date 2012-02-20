#include "MathUtil.h"

namespace MathUtil
{
	INT IntRound( double f )
	{
		return f < 0.0 ? INT(f-0.5) : INT(f+0.5);
	}

	double	Round( double fVal, double fStep )
	{
		if (fStep > 0.f)
			fVal = IntRound(fVal / fStep) * fStep;
		return fVal;
	}
}
