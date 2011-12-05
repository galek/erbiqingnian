//----------------------------------------------------------------------------
// vector.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _VECTOR_H_
#define _VECTOR_H_

//#define POINTER_64_BUG_FIX

enum PLANE_CLASSIFICATION
{
	KPlaneFront,
	KPlaneBack,
	KPlaneOn,
	KPlaneSpanning
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct VECTOR2
{
	union
	{
		struct
		{
			float fX;
			float fY;
		};
		struct
		{
			float x;
			float y;
		};
		float element[2];
	};

	VECTOR2(void) {}

	VECTOR2(float x, float y) : fX(x), fY(y) {}

	VECTOR2(float val) : fX(val), fY(val) {}

	inline float & operator[](unsigned int i) { return element[i]; }

	inline float & operator[](int i) { return element[i]; }

	//------------------------------------------------------------------------
	friend VECTOR2 operator *(
		float scale,
		const VECTOR2 & v)
	{
		VECTOR2 vRet;
		vRet.x = v.x * scale;
		vRet.y = v.y * scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline void operator=(
		const VECTOR2 & v)
	{
		x = v.x;
		y = v.y;
	}

	//------------------------------------------------------------------------
	// this helps convert things like D3DVECTOR2 to our VECTOR class - without needing to include d3dx9.h
	inline void operator=(
		const float pfArray[2])
	{
		x = pfArray[0];
		y = pfArray[1];
	}

	//------------------------------------------------------------------------
	inline VECTOR2 operator-(
		void) const
	{
		VECTOR2 vRet;
		vRet.fX = -x;
		vRet.fY = -y;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline void operator*=(
		float scale)
	{
		x *= scale;
		y *= scale;
	}

	//------------------------------------------------------------------------
	inline void operator/=(
		float scale)
	{
		x /= scale;
		y /= scale;
	}

	//------------------------------------------------------------------------
	inline void operator+=(
		const VECTOR2 & v)
	{
		x += v.x;
		y += v.y;
	}

	//------------------------------------------------------------------------
	inline void operator-=(
		const VECTOR2 & v)
	{
		x -= v.x;
		y -= v.y;
	}

	//------------------------------------------------------------------------
	inline VECTOR2 operator+(
		const VECTOR2 & v) const
	{
		VECTOR2 vRet;
		vRet.x = v.x + x;
		vRet.y = v.y + y;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR2 operator-(
		const VECTOR2 & v) const
	{
		VECTOR2 vRet;
		vRet.x = x - v.x;
		vRet.y = y - v.y;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR2 operator*(
		float scale) const
	{
		VECTOR2 vRet;
		vRet.x = x * scale;
		vRet.y = y * scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR2 operator/(
		float scale) const
	{
		VECTOR2 vRet;
		vRet.x = x / scale;
		vRet.y = y / scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator==(
		const VECTOR2 & a,
		const VECTOR2 & b)
	{
		return (a.x == b.x && a.y == b.y);
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator!=(
		const VECTOR2 & a,
		const VECTOR2 & b)
	{
		return (!(a.x == b.x && a.y == b.y));
	}
};

//----------------------------------------------------------------------------
struct VECTOR
{
	union
	{
		struct
		{
			float fX;
			float fY;
			float fZ;
		};
		struct
		{
			float x;
			float y;
			float z;
		};
		float element[3];
	};

	VECTOR(void) {}		// i guess we don't zero to save cost?

	VECTOR(float x, float y, float z) : fX(x), fY(y), fZ(z) {}

	VECTOR(float val) : fX(val), fY(val), fZ(val) {}

	VECTOR(float x, float y) : fX(x), fY(y), fZ(0.0f) {}

	inline float & operator[](unsigned int i) { return element[i]; }

	inline float & operator[](int i) { return element[i]; }

	inline void Set( float _x, float _y, float _z ) { fX = _x; fY = _y; fZ = _z; }

	//------------------------------------------------------------------------
	inline void operator=(
		const VECTOR & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	//------------------------------------------------------------------------
	// this helps convert things like D3DVECTOR3 to our VECTOR class - without needing to include d3dx9.h
	inline void operator=(
		const float pfArray[3])
	{
		x = pfArray[0];
		y = pfArray[1];
		z = pfArray[2];
	}

	//------------------------------------------------------------------------
	inline VECTOR operator-(
		void) const
	{
		VECTOR vRet;
		vRet.x = -x;
		vRet.y = -y;
		vRet.z = -z;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline friend VECTOR operator*(
		float scale, 
		const VECTOR & v)
	{
		VECTOR vRet;
		vRet.x = v.x * scale;
		vRet.y = v.y * scale;
		vRet.z = v.z * scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR operator*(
		float scale) const
	{
		VECTOR vRet;
		vRet.x = x * scale;
		vRet.y = y * scale;
		vRet.z = z * scale;
		return vRet;
	}
	//------------------------------------------------------------------------
	inline VECTOR operator*(
		VECTOR vScale) const
	{
		VECTOR vRet;
		vRet.x = x * vScale.x;
		vRet.y = y * vScale.y;
		vRet.z = z * vScale.z;
		return vRet;
	}
	//------------------------------------------------------------------------
	inline void operator*=(
		float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
	}

	//------------------------------------------------------------------------
	inline void operator/=(
		float scale)
	{
		x /= scale;
		y /= scale;
		z /= scale;
	}

	//------------------------------------------------------------------------
	inline void operator+=(
		const VECTOR & v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}

	//------------------------------------------------------------------------
	inline void operator-=(
		const VECTOR & v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	//------------------------------------------------------------------------
	inline VECTOR operator+(
		const VECTOR & v) const
	{
		VECTOR vRet;
		vRet.x = v.x + x;
		vRet.y = v.y + y;
		vRet.z = v.z + z;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR operator-(
		const VECTOR & v) const
	{
		VECTOR vRet;
		vRet.x = x - v.x;
		vRet.y = y - v.y;
		vRet.z = z - v.z;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR operator/(
		float scale) const
	{
		VECTOR vRet;
		vRet.x = x / scale;
		vRet.y = y / scale;
		vRet.z = z / scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator==(
		const VECTOR & a, 
		const VECTOR & b)
	{
		return (a.x == b.x && a.y == b.y && a.z == b.z);
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator!=(
		const VECTOR & a,
		const VECTOR & b)
	{
		return (!(a.x == b.x && a.y == b.y && a.z == b.z));
	}
};
typedef VECTOR	VECTOR3;

//----------------------------------------------------------------------------
struct VECTOR4
{
	union
	{
		struct
		{
			float fX;
			float fY;
			float fZ;
			float fW;
		};
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
		float element[4];
	};

	VECTOR4(void) {}		// i guess we don't zero to save cost?

	VECTOR4(float x, float y, float z, float w) : fX(x), fY(y), fZ(z), fW(w) {}

	VECTOR4(float x, float y, float z) : fX(x), fY(y), fZ(z), fW(1.0f) {}

	VECTOR4(float x, float y) : fX(x), fY(y), fZ(0.0f), fW(1.0f) {}

	VECTOR4(const VECTOR & v, float w) : fX(v.x), fY(v.y), fZ(v.z), fW(w) {}

	inline float & operator[](unsigned int i) { return element[i]; }

	inline float & operator[](int i) { return element[i]; }

	//------------------------------------------------------------------------
	inline void operator=(
		const VECTOR4 & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
	}

	//------------------------------------------------------------------------
	inline void operator=(
		const VECTOR & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = 1.0f;
	}

	//------------------------------------------------------------------------
	// this helps convert things like D3DVECTOR3 to our VECTOR class - without needing to include d3dx9.h
	inline void operator=(
		const float pfArray[4])
	{
		x = pfArray[0];
		y = pfArray[1];
		z = pfArray[2];
		w = pfArray[3];
	}

	//------------------------------------------------------------------------
	inline VECTOR4 operator-(
		void) const
	{
		VECTOR4 vRet;
		vRet.x = -x;
		vRet.y = -y;
		vRet.z = -z;
		vRet.w = -w;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline friend VECTOR4 operator*(
		float scale, 
		const VECTOR4 & v)
	{
		VECTOR4 vRet;
		vRet.x = v.x * scale;
		vRet.y = v.y * scale;
		vRet.z = v.z * scale;
		vRet.w = v.w * scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR4 operator*(
		float scale) const
	{
		VECTOR4 vRet;
		vRet.x = x * scale;
		vRet.y = y * scale;
		vRet.z = z * scale;
		vRet.z = w * scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline void operator*=(
		float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
		w *= scale;
	}

	//------------------------------------------------------------------------
	inline void operator/=(
		float scale)
	{
		x /= scale;
		y /= scale;
		z /= scale;
		w /= scale;
	}

	//------------------------------------------------------------------------
	inline void operator+=(
		const VECTOR4 & v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
	}

	//------------------------------------------------------------------------
	inline void operator-=(
		const VECTOR4 & v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
	}

	//------------------------------------------------------------------------
	inline VECTOR4 operator+(
		const VECTOR4 & v) const
	{
		VECTOR4 vRet;
		vRet.x = v.x + x;
		vRet.y = v.y + y;
		vRet.z = v.z + z;
		vRet.w = v.z + w;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR4 operator-(
		const VECTOR4 & v) const
	{
		VECTOR4 vRet;
		vRet.x = x - v.x;
		vRet.y = y - v.y;
		vRet.z = z - v.z;
		vRet.w = z - v.w;
		return vRet;
	}

	//------------------------------------------------------------------------
	inline VECTOR4 operator/(
		float scale) const
	{
		VECTOR4 vRet;
		vRet.x = x / scale;
		vRet.y = y / scale;
		vRet.z = z / scale;
		vRet.w = w / scale;
		return vRet;
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator==(
		const VECTOR4 & a, 
		const VECTOR4 & b)
	{
		return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
	}

	//------------------------------------------------------------------------
	// useful for comparing vs. invalid position
	inline friend bool operator!=(
		const VECTOR4 & a,
		const VECTOR4 & b)
	{
		return (!(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w));
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorCross ( VECTOR & vDest, const VECTOR & vVect1, const VECTOR & vVect2 ) 
{
	vDest.fX = vVect1.fY * vVect2.fZ - vVect1.fZ * vVect2.fY; 
	vDest.fY = vVect1.fZ * vVect2.fX - vVect1.fX * vVect2.fZ; 
	vDest.fZ = vVect1.fX * vVect2.fY - vVect1.fY * vVect2.fX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorDot ( const VECTOR & vVect1, const VECTOR & vVect2 ) 
{
	return vVect1.fX * vVect2.fX + 
		   vVect1.fY * vVect2.fY + 
		   vVect1.fZ * vVect2.fZ;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorSubtract ( VECTOR & vDest, const VECTOR & vVect1, const VECTOR & vVect2 )
{
	vDest.fX = vVect1.fX - vVect2.fX; 
	vDest.fY = vVect1.fY - vVect2.fY; 
	vDest.fZ = vVect1.fZ - vVect2.fZ; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorAdd ( VECTOR & vDest, const VECTOR & vVect1, const VECTOR & vVect2 )
{
	vDest.fX = vVect1.fX + vVect2.fX; 
	vDest.fY = vVect1.fY + vVect2.fY; 
	vDest.fZ = vVect1.fZ + vVect2.fZ; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorScaleAdd( VECTOR & vDest, float fScale, const VECTOR & vVect1, const VECTOR & vVect2 )
{
	vDest.fX = fScale * vVect1.fX + vVect2.fX;
	vDest.fY = fScale * vVect1.fY + vVect2.fY;
	vDest.fZ = fScale * vVect1.fZ + vVect2.fZ;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLength ( const VECTOR & vVect ) 
{
	return sqrtf ( vVect.fX * vVect.fX +
				  vVect.fY * vVect.fY + 
				  vVect.fZ * vVect.fZ);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLength ( const VECTOR2 & vVect ) 
{
	return sqrtf ( vVect.fX * vVect.fX +
				  vVect.fY * vVect.fY);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLengthXY ( const VECTOR & vVect ) 
{
	return sqrtf ( vVect.fX * vVect.fX +
		vVect.fY * vVect.fY );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLengthSquared ( const VECTOR & vVect ) 
{
	return	vVect.fX * vVect.fX +
			vVect.fY * vVect.fY + 
			vVect.fZ * vVect.fZ;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLengthSquaredXY ( const VECTOR & vVect ) 
{
	return	vVect.fX * vVect.fX +
			vVect.fY * vVect.fY;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorLengthSquared ( const VECTOR2 & vVect ) 
{
	return	vVect.fX * vVect.fX +
			vVect.fY * vVect.fY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorDistanceSquared ( const VECTOR & V1, const VECTOR & V2 ) 
{
	return	(V1.x-V2.x) * (V1.x-V2.x) +
			(V1.y-V2.y) * (V1.y-V2.y) +
			(V1.z-V2.z) * (V1.z-V2.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorDistanceSquaredXY(
	const VECTOR & vVect1,
	const VECTOR & vVect2) 
{
	return (vVect2.fX - vVect1.fX) * (vVect2.fX - vVect1.fX) + (vVect2.fY - vVect1.fY) * (vVect2.fY - vVect1.fY);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorDistanceXY(
	const VECTOR & vVect1,
	const VECTOR & vVect2) 
{
	return sqrtf( (vVect2.fX - vVect1.fX) * (vVect2.fX - vVect1.fX) + 
				  (vVect2.fY - vVect1.fY) * (vVect2.fY - vVect1.fY) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorNormalize(
	VECTOR& vector)
{
	float fDistSq = vector.fX * vector.fX + vector.fY * vector.fY + vector.fZ * vector.fZ;
	if (fDistSq >= 1.0f - EPSILON && fDistSq <= 1.0f + EPSILON)
	{
		return 1.0f;
	}
	float fLength = sqrtf(fDistSq);
	if (fLength != 0.0f)
	{
		vector.fX = vector.fX / fLength;
		vector.fY = vector.fY / fLength;
		vector.fZ = vector.fZ / fLength;
	} else {
		vector.fX = 0.0f;
		vector.fY = 0.0f;
		vector.fZ = 1.0f;
	}
	return fLength;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorNormalize(
	VECTOR2& vector)
{
	float fDistSq = vector.fX * vector.fX + vector.fY * vector.fY;
	if (fDistSq >= 1.0f - EPSILON && fDistSq <= 1.0f + EPSILON)
	{
		return 1.0f;
	}
	float fLength = sqrtf(fDistSq);
	if (fLength != 0.0f)
	{
		vector.fX = vector.fX / fLength;
		vector.fY = vector.fY / fLength;
	}
	return fLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorNormalize ( VECTOR & vDest, const VECTOR & vVect ) 
{
	float fDistSq = vVect.fX * vVect.fX + vVect.fY * vVect.fY + vVect.fZ * vVect.fZ;
	if (fDistSq >= 1.0f - EPSILON && fDistSq <= 1.0f + EPSILON)
	{
		vDest = vVect;
		return 1.0f;
	}
	float fLength = sqrtf(fDistSq);
	if (fLength != 0.0f)
	{
		vDest.fX = vVect.fX / fLength;
		vDest.fY = vVect.fY / fLength;
		vDest.fZ = vVect.fZ / fLength;
	}
	return fLength;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorInterpolate ( VECTOR & vDest, float fPercent2, const VECTOR & vVect1, const VECTOR & vVect2 ) 
{
	vDest.fX = vVect2.fX * fPercent2 + vVect1.fX * ( 1.0f - fPercent2 );
	vDest.fY = vVect2.fY * fPercent2 + vVect1.fY * ( 1.0f - fPercent2 );
	vDest.fZ = vVect2.fZ * fPercent2 + vVect1.fZ * ( 1.0f - fPercent2 );
}


inline void VectorInterpolate ( VECTOR2 & vDest, float fPercent2, const VECTOR2 & vVect1, const VECTOR2 & vVect2 ) 
{
	vDest.fX = vVect2.fX * fPercent2 + vVect1.fX * ( 1.0f - fPercent2 );
	vDest.fY = vVect2.fY * fPercent2 + vVect1.fY * ( 1.0f - fPercent2 );
}

//----------------------------------------------------------------------------
// needs to be optimized
//----------------------------------------------------------------------------
inline float VectorToAngle( 
	const VECTOR & vVect1,
	const VECTOR & vVect2) 
{
	return atan2(vVect2.fY - vVect1.fY, vVect2.fX - vVect1.fX);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int VectorDirectionToAngleInt( 
	const VECTOR & vVect1) 
{
	VECTOR vDirection = vVect1;
	if ( vDirection.fZ != 0.0f )
	{
		vDirection.fZ = 0.0f;
		VectorNormalize( vDirection, vDirection );
	}
	float fRadians = atan2( vDirection.fY, vDirection.fX );
	return NORMALIZE( (int)(fRadians * 360.0f / (2.0f * PI)), 360 ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorDirectionToAngle( 
	const VECTOR & vVect1) 
{
	VECTOR vDirection = vVect1;
	if ( vDirection.fZ != 0.0f )
	{
		vDirection.fZ = 0.0f;
		VectorNormalize( vDirection, vDirection );
	}
	return atan2( vDirection.fY, vDirection.fX );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
inline void VectorDirectionFromAnglePitch( VECTOR & vDirection, int nAngle, int nPitch )
{
	vDirection.fZ = gfSin360Tbl[nPitch];
	vDirection.fX = gfCos360Tbl[nAngle];
	vDirection.fY = gfSin360Tbl[nAngle];
	// now normalize it - assuming that Z is already normalized
	float fXYTotal = 1.0f - vDirection.fZ * vDirection.fZ;
	float fNewX = gfSqrtFractionTbl[ PrimeFloat2Int( 100.0f * fXYTotal * vDirection.fX * vDirection.fX ) ];
	float fNewY = gfSqrtFractionTbl[ PrimeFloat2Int( 100.0f * fXYTotal * vDirection.fY * vDirection.fY ) ];
	if ( vDirection.fX < 0.0f )
		vDirection.fX = - fNewX;
	else
		vDirection.fX = fNewX;
	if ( vDirection.fY < 0.0f )
		vDirection.fY = - fNewY;
	else
		vDirection.fY =   fNewY;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VectorDirectionFromAnglePitchRadians( VECTOR & vDirection, float fAngle, float fPitch );

inline void VectorDirectionFromAngleRadians( VECTOR & vDirection, float fAngle )
{
	vDirection.fX = cosf( fAngle );
	vDirection.fY = sinf( fAngle );
	vDirection.fZ = 0.0f;
	VectorNormalize( vDirection );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorScale( VECTOR & vDest, const VECTOR & vSource, float fScalar )
{
	vDest.fX = vSource.fX * fScalar;
	vDest.fY = vSource.fY * fScalar;
	vDest.fZ = vSource.fZ * fScalar;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline VECTOR VectorLerp( const VECTOR & vFirst, const VECTOR & vSecond, float fFirstPercent )
{
	float fSecondPercent = 1.0f - fFirstPercent;
	VECTOR vRet;
	vRet.fX = vFirst.fX * fFirstPercent + vSecond.fX * fSecondPercent;
	vRet.fY = vFirst.fY * fFirstPercent + vSecond.fY * fSecondPercent;
	vRet.fZ = vFirst.fZ * fFirstPercent + vSecond.fZ * fSecondPercent;
	return vRet;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorZRotation( VECTOR & vDest, float fRotationAngle )
{
	float fCosZ = cosf( fRotationAngle );
	float fSinZ = sinf( fRotationAngle );
	float fVal = ( vDest.fX * fCosZ ) - ( vDest.fY * fSinZ );
	vDest.fY = ( vDest.fX * fSinZ ) + ( vDest.fY * fCosZ );
	vDest.fX = fVal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void VectorZRotationInt( VECTOR & vDest, int nAngle )
{
	float fVal = ( vDest.fX * gfCos360Tbl[nAngle] ) - ( vDest.fY * gfSin360Tbl[nAngle] );
	vDest.fY = ( vDest.fX * gfSin360Tbl[nAngle] ) + ( vDest.fY * gfCos360Tbl[nAngle] );
	vDest.fX = fVal;
	vDest.fZ = 0.0f;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL VectorIsZero(
	const VECTOR& vDest)
{
	if (vDest.fX != 0.0f || vDest.fY != 0.0f || vDest.fZ != 0.0f)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int GetClockDirection(
	VECTOR& pt1,
	VECTOR& pt2,
	VECTOR& pt3)
{
	return GetClockDirection(pt1.fX, pt1.fY, pt2.fX, pt2.fY, pt3.fX, pt3.fY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL VectorIsZeroEpsilon(
	const VECTOR& v)
{
	return (ABS(v.fX) < EPSILON && ABS(v.fY) < EPSILON && ABS(v.fZ) < EPSILON);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline VECTOR GetNormalUpVector(const VECTOR & vNormalIn)
{
	VECTOR vNormal = vNormalIn;
	if (VectorIsZero(vNormal))
	{
		vNormal = VECTOR(0, 0, 1);
	}

	VECTOR vNormalUp( 0, 0, 1 );
	if ( VectorLengthSquared( vNormal - vNormalUp ) < 0.01f )
	{
		vNormalUp	 = VECTOR( 0, -1, 0 );
	} else if ( VectorLengthSquared( vNormal + vNormalUp ) < 0.01f ) {
		vNormalUp	 = VECTOR( 0,  1, 0 );
	} else {
		VECTOR vNormalRight;
		VectorCross( vNormalRight, vNormalUp, vNormal );
		VectorNormalize( vNormalRight, vNormalRight );
		VectorCross( vNormalUp, vNormalRight, vNormal );
		VectorNormalize( vNormalUp );
	}

	return vNormalUp;
}

//----------------------------------------------------------------------------
// pfDistaceAlongRaySquared is multiplied by the dot product of the point on
// the ray and the direction.  Thus, it's possible for it to be negative.
// To find the actual distance along the ray, the routine would be:
// (using D as the value of *pfDistanceAlongRaySquared)
// (D / fabs(D)) * sqrt(fabs(D))
//----------------------------------------------------------------------------
inline float VectorGetDistanceToLineSquared(
	const VECTOR & vRaySource,
	const VECTOR & vRayDirection, // assumed to be normalized
	const VECTOR & vPoint,
	float * pfDistAlongRaySquared )
{
	VECTOR vDeltaFromPoint = vRaySource - vPoint;
	VECTOR vPerp;
	VectorCross( vPerp, vDeltaFromPoint, vRayDirection );
	float fDistSquared = VectorLengthSquared( vPerp );
	if ( pfDistAlongRaySquared )
	{
		VECTOR vPointToRay;
		VectorCross( vPointToRay, vPerp, vRayDirection );
		VectorNormalize( vPointToRay );
		float fDist = sqrt( fDistSquared );
		vPointToRay *= fDist;
		VECTOR vPointOnRay = vPoint - vPointToRay;
		VECTOR vDeltaToPointOnRay = vPointOnRay - vRaySource;
		*pfDistAlongRaySquared = VectorLengthSquared( vDeltaToPointOnRay );
		VectorNormalize( vDeltaToPointOnRay );
		// This value should be either -1 or +1
		*pfDistAlongRaySquared *= VectorDot(vDeltaToPointOnRay, vRayDirection);
	}
	return fDistSquared;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float VectorAngleBetween( const VECTOR& A,	// vector a
								const VECTOR& B )	// vector b
{
	return ( float ) acos( A.fX * B.fX + A.fY * B.fY + A.fZ * B.fZ );
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

BOOL RayIntersectTriangle(  
	const VECTOR *pOrig, 
	const VECTOR *pDir,
	const VECTOR &vVert0, 
	const VECTOR &vVert1, 
	const VECTOR &vVert2,
	float *pfDist, 
	float *pfUi, 
	float *pfVi );

void VectorCatMullRom(
	VECTOR & vPosition,
	const VECTOR & vPointA,
	const VECTOR & vPointB,
	const VECTOR & vPointC,
	const VECTOR & vPointD,
	float fWeight);

PLANE_CLASSIFICATION ClassifyPoint(
	const VECTOR& Point,			// point to classify
	const VECTOR& VertexOnPlane,	// a vertex on the plane
	const VECTOR& Normal );			// the normal of the plane

PLANE_CLASSIFICATION ClassifyPointForSphere(
	const VECTOR& Point, 				// point to classify
	const VECTOR& VertexOnPlane, 		// a vertex on the plane
	const VECTOR& Normal, 				// the normal of the plane
	float Radius );						// the radius of the sphere

void ClosestPointOnLine(
	const VECTOR& PointA,		// point a of line segment
	const VECTOR& PointB,		// point b of line segment
	const VECTOR& Point,		// point to check from
	VECTOR& ClosestPoint );		// resultant point to be filled

void ClosestPointOnTriangle( 
	const VECTOR& CornerA,		// corner a of the triangle
	const VECTOR& CornerB,		// corner b of the triangle 
	const VECTOR& CornerC,		// corner c of the triangle 
	const VECTOR& Point, 		// point to start from
	VECTOR& ClosestPoint );		// closest point to be filled

BOOL GetLinePlaneIntersection( 
	const VECTOR& LineStart,			// line segment start 
	const VECTOR& LineEnd, 				// line segment end
	const VECTOR& VertexOnPlane,		// point on plane
	const VECTOR& Normal,				// normal of plane
	VECTOR& Intersection );				// intersection point to fill

BOOL GetSpherePlaneIntersection( 
	const VECTOR& LineStart,		// line segment start
	const VECTOR& LineEnd,			// line segment end
	const VECTOR& VertexOnPlane,	// point on plane
	const VECTOR& Normal,			// normal of plane
	VECTOR& Intersection );			// intersection point to fill

BOOL VectorPosIsInFrontOf(
	const VECTOR &vPos,
	const VECTOR &vFrontOfPos,
	const VECTOR &vFrontOfDir);

BOOL VectorPosIsInBehindOf(
	const VECTOR &vPos,
	const VECTOR &vBehindOfPos,
	const VECTOR &vBehindOfDir);

PRESULT VectorProject(
	VECTOR * pvOut,
	const VECTOR * pvSrc,
	const struct MATRIX * pmProj,
	const struct MATRIX * pmView  = NULL,
	const struct MATRIX * pmWorld = NULL,
	const RECT * pViewport = NULL );

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern const VECTOR cgvNone;
extern const VECTOR cgvNull;
extern const VECTOR cgvXAxis;
extern const VECTOR cgvYAxis;
extern const VECTOR cgvZAxis;

#endif // _VECTOR_H_
