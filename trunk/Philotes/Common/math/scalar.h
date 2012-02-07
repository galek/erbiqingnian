#pragma once
//------------------------------------------------------------------------------
/**
 
*/

#include "core/types.h"
#include <math.h>

// common platform-agnostic definitions
namespace Math
{
typedef float scalar;

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif

// the half circle
#ifndef N_PI
#define N_PI (Math::scalar(PI))
#endif
// the whole circle
#ifndef N_PI_DOUBLE
#define N_PI_DOUBLE (Math::scalar(6.283185307179586476925286766559))
#endif
// a quarter circle
#ifndef N_PI_HALF
#define N_PI_HALF (Math::scalar(1.5707963267948966192313216916398f))
#endif
// ---HOTFIXED

#ifndef TINY
#define TINY (0.0000001f)
#endif
#define N_TINY TINY

} // namespace Math
//------------------------------------------------------------------------------
