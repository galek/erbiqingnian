#include "MathUtil.h"

_NAMESPACE_BEGIN

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

	bool IsFloat3Zero( const Float3& vec )
	{
		// TODO:¸ü¾«È·
		if (vec.x == 0 && vec.y == 0 && vec.z == 0)
		{
			return true;
		}
		return false;
	}
}

_NAMESPACE_END