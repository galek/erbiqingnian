//----------------------------------------------------------------------------
// e_quaternion.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_QUATERNION__
#define __E_QUATERNION__


//namespace FSSE
//{;

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

typedef union
{
	//Point in 3D space
	struct { float x, y, z, w; };
	struct { float q[4]; };
	//struct { float v[4]; }; //Makes for easier porting of code
	struct { VECTOR v; float w; };

	inline void Set( float _x, float _y, float _z, float _w )	{ x = _x; y = _y; z = _z; w = _w; }
	inline void Set( const VECTOR & _v, float _w )				{ v = _v; w = _w; }
	inline void Set( const VECTOR & _v )						{ Set( _v, 0.f ); }
} Quaternion;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void QuatNormalize (Quaternion *quat);
void QuatToMatrix (const Quaternion *quat, MATRIX & matrix);
void MatrixToQuat (const MATRIX & matrix, Quaternion *quat);
void QuaternionToGlRotation (Quaternion *quat, float *x, float *y, float *z, float *degrees);
void GlRotationToQuaternion (Quaternion *quat, float x, float y, float z, float angle);
void QuatMul (Quaternion *q1, Quaternion *q2, Quaternion *result);
void QuatCopy (Quaternion *q1, Quaternion *q2);
void QuatLog (Quaternion *q1, Quaternion *q2);
float QuatDot (Quaternion *q1, Quaternion *q2);
void QuatInverse (Quaternion *q, Quaternion *result);
void QuatExp (Quaternion *q1, Quaternion *q2);
void QuatSlerpShortestPath (Quaternion *result, Quaternion *from, Quaternion *to, float t);
void QuatSlerp (Quaternion *result, Quaternion *from, Quaternion *to, float t);
void QuatSQuad (float t, Quaternion *q0, Quaternion *q1, Quaternion *q2, Quaternion *q3, Quaternion *result);

void QuatAxBxC (Quaternion *q1, Quaternion *q2, Quaternion *q3, Quaternion *result);
float Determinant3x3(float det[3][3]);

void QuatFromDirectionVector( Quaternion & tQuat, const VECTOR & vDirection );


// Defined in dxC_matrix.cpp:
void MatrixFromQuaternion(
	struct MATRIX * pmOrientation,
	const Quaternion * ptOrientation );
void EulerToQuat( Quaternion & tQuat, float roll, float pitch, float yaw );


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void EulerToQuat( Quaternion & tQuat, const VECTOR & v )
{
	EulerToQuat( tQuat, v.x, v.z, v.y );
};


//}; // FSSE

#endif // __E_CURVE__