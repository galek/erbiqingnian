#ifndef __MATRIX_H__
#define __MATRIX_H__

#if _MSC_VER >= 1300  // VC7
#define MATRIX_ALIGN16 __declspec(align(16))
#else
#define MATRIX_ALIGN16  // Earlier compiler may not understand this, do nothing.
#endif

#include "vector.h"

// made to mimick D3DMATRIX
MATRIX_ALIGN16 struct MATRIX
{
	union
	{
		struct
		{
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;
		};
		float m[4][4];
		float _m[16];
	};

	inline operator float* ()
	{
		return (float*)&_11;
	}
	inline operator const float* () const
	{
		return (const float*)&_11;
	}

	inline bool operator==(const MATRIX& mat)
	{
		return ( 0 == memcmp( m, &mat.m, sizeof(float)*16 ) );
	}

	friend MATRIX operator * ( float, const MATRIX& );

	void SetRight( const VECTOR & vRight )				{	*reinterpret_cast<VECTOR*>(&_11) = vRight;		}
	void SetForward( const VECTOR & vForward )			{	*reinterpret_cast<VECTOR*>(&_21) = vForward;	}
	void SetUp( const VECTOR & vUp )					{	*reinterpret_cast<VECTOR*>(&_31) = vUp;			}
	void SetTranslate( const VECTOR & vTranslate )		{	*reinterpret_cast<VECTOR*>(&_41) = vTranslate;	}
	void Identity()										{	memset( _m, 0, sizeof(_m) ); _11 = _22 = _33 = _44 = 1.f;	 }
};

#undef MATRIX_ALIGN16

// made to mimick D3DXQUATERNION
struct QUATERNION
{
	float x, y, z, w;
};

void QuaternionRotationMatrix( QUATERNION * pOutQuat, const MATRIX * pMatrix );

void MatrixMultiply ( MATRIX * pOutMatrix, const MATRIX * pMatrixFirst, const MATRIX * pMatrixSecond );
void MatrixMultiply ( MATRIX * pMatrix, float fScalar );
void MatrixMultiply ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix );
void MatrixMultiply ( struct VECTOR4 * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix );
void MatrixMultiply ( struct VECTOR4 * pOutVector, const struct VECTOR4 * pVector, const MATRIX * pMatrix );
void MatrixMultiplyNormal ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix );
// Transform vector through matrix, projecting back into w == 1
void MatrixMultiplyCoord ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix );
int	 MatrixTransformRotation ( const MATRIX * pMatrix, int nInRotation );
void MatrixTranspose ( MATRIX * pOutMatrix, const MATRIX * pInMatrix );
void MatrixTranslation ( MATRIX * pOutMatrix, const VECTOR * pPosition );
void MatrixTransform ( MATRIX * pOutMatrix, const VECTOR * pPosition, float fRotation, float fPitch = 0.0f );
void MatrixRotationYawPitchRoll ( MATRIX & mOutMatrix, float fYaw, float fPitch, float fRoll );
void MatrixRotationX ( MATRIX & mOutMatrix, float fRotation );
void MatrixRotationY ( MATRIX & mOutMatrix, float fRotation );
void MatrixRotationZ ( MATRIX & mOutMatrix, float fRotation );
void MatrixRotationAxis( MATRIX & mOutMatrix, const VECTOR & vAxis, float fRadians );
void MatrixFromVectors ( MATRIX & mOutMatrix, const VECTOR & vPosition, const VECTOR & vUp, const VECTOR & vForward );
void MatrixTranslationScale ( MATRIX * pOutMatrix, const VECTOR * pPosition, const VECTOR * pvScale );
void MatrixScale ( MATRIX * pOutMatrix, const VECTOR * pvScale );
void MatrixScale ( MATRIX * pOutMatrix, float fScale );
void MatrixInverse ( MATRIX * pOutMatrix, const MATRIX * pInMatrix );
void MatrixIdentity ( MATRIX * pMatrix );
float MatrixDeterminant ( const MATRIX * pMatrix );
void MatrixLookAt ( MATRIX * pMatrix, VECTOR * pvEye, VECTOR * pvLookAt );
// returns true if same, false if not
BOOL MatrixCompare( const MATRIX * pMatrix1, const MATRIX *pMatrix2, float fError = 0.001f );
// fRotationDeg is in degrees, not radians
void GetAlignmentMatrix(MATRIX * pMatrixOut, MATRIX & tWorldMatrix, const VECTOR & vPosition, VECTOR & vNormalIn, float fRotationDeg, VECTOR * vNormalUpIn = NULL, const VECTOR * vScale = NULL);
void MatrixMakeView (
	MATRIX & mOutMatrix, 
	const VECTOR & vEye, 
	const VECTOR & vAt, 
	const VECTOR & vUp );
void MatrixMakePerspectiveProj (
	MATRIX & mOutMatrix, 
	float fFOVy, 
	float fAspect, 
	float fZNear, 
	float fZFar );
void MatrixMakeOffCenterPerspectiveProj (
	MATRIX & mOutMatrix, 
	RECT * tOrigRect,
	RECT * tScreenRect,
	float fFOVy,
	float fZNear, 
	float fZFar );
void MatrixMakeOrthoProj ( 
	MATRIX & mOutMatrix, 
	float fWidth, 
	float fHeight, 
	float fZNear, 
	float fZFar );
void MatrixMakeWFriendlyProjection(
	MATRIX * pmProj );

#endif // __MATRIX_H__