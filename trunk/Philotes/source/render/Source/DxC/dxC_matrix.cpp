//----------------------------------------------------------------------------
// dx9_matrix.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_target.h"
#include "e_quaternion.h"

#include "e_matrix.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

void QuaternionRotationMatrix( QUATERNION * pOutQuat, const MATRIX * pMatrix )
{
	D3DXQuaternionRotationMatrix( (D3DXQUATERNION*)pOutQuat, (D3DXMATRIXA16*)pMatrix );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMultiply ( MATRIX * pOutMatrix, const MATRIX * pMatrixFirst, const MATRIX * pMatrixSecond )
{
	D3DXMatrixMultiply( (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)pMatrixFirst, (D3DXMATRIXA16*)pMatrixSecond );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMultiply ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix )
{
	D3DXVECTOR4 vVector4;
	D3DXVec3Transform( & vVector4, (D3DXVECTOR3 *)pVector, (D3DXMATRIXA16*)pMatrix);
	pOutVector->fX = vVector4.x;
	pOutVector->fY = vVector4.y;
	pOutVector->fZ = vVector4.z;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMultiply ( struct VECTOR4 * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix )
{
	D3DXVec3Transform( (D3DXVECTOR4*)pOutVector, (D3DXVECTOR3*)pVector, (D3DXMATRIXA16*)pMatrix);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMultiply ( struct VECTOR4 * pOutVector, const struct VECTOR4 * pVector, const MATRIX * pMatrix )
{
	D3DXVec4Transform( (D3DXVECTOR4*)pOutVector, (D3DXVECTOR4*)pVector, (D3DXMATRIXA16*)pMatrix);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMultiplyNormal ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix )
{
	D3DXVec3TransformNormal( (D3DXVECTOR3 *)pOutVector, (D3DXVECTOR3 *)pVector, (D3DXMATRIXA16*)pMatrix);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Transform vector through matrix, projecting back into w == 1
void MatrixMultiplyCoord ( struct VECTOR * pOutVector, const struct VECTOR * pVector, const MATRIX * pMatrix )
{
	D3DXVec3TransformCoord( (D3DXVECTOR3 *)pOutVector, (D3DXVECTOR3 *)pVector, (D3DXMATRIXA16*)pMatrix);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int	 MatrixTransformRotation ( const MATRIX * pMatrix, int nInRotation )
{
	VECTOR vStraight;
	vStraight.fY = vStraight.fZ = 0.0f;
	vStraight.fX = 1.0f;
	VECTOR vTurned;
	D3DXVec3TransformNormal( (D3DXVECTOR3 *)&vTurned, (D3DXVECTOR3 *)&vStraight, (D3DXMATRIXA16*)pMatrix);
	float fRadians = VectorToAngle( vStraight, vTurned );
	int nTurnedRotation = PrimeFloat2Int ( (float)D3DXToDegree( fRadians ) ); // need to add FloatToInt function
	return nInRotation + nTurnedRotation;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixTranspose ( MATRIX * pOutMatrix, const MATRIX * pInMatrix )
{
	D3DXMatrixTranspose( (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)pInMatrix );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixTranslation ( MATRIX * pOutMatrix, const VECTOR * pPosition )
{
	D3DXMatrixTranslation( (D3DXMATRIXA16*)pOutMatrix, pPosition->fX, pPosition->fY, pPosition->fZ );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixTransform ( MATRIX * pOutMatrix, const VECTOR * pPosition, float fRotation, float fPitch )
{
	if ( fPitch )
	{
		D3DXMatrixRotationY( (D3DXMATRIXA16*)pOutMatrix, -fPitch );
	} else {
		D3DXMatrixIdentity( (D3DXMATRIXA16*)pOutMatrix );
	}

	if ( fRotation )
	{
		D3DXMATRIXA16 mRotation;
		D3DXMatrixRotationZ( (D3DXMATRIXA16*)&mRotation, fRotation );
		D3DXMatrixMultiply( (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)&mRotation );
	}

	if ( pPosition )
	{
		D3DXMATRIXA16 mTranslation;
		D3DXMatrixTranslation( (D3DXMATRIXA16*)&mTranslation, pPosition->fX, pPosition->fY, pPosition->fZ );
		D3DXMatrixMultiply( (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)pOutMatrix, (D3DXMATRIXA16*)&mTranslation );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixRotationYawPitchRoll ( MATRIX & mOutMatrix, float fYaw, float fPitch, float fRoll )
{
	D3DXMatrixRotationYawPitchRoll( (D3DXMATRIXA16*)&mOutMatrix, fYaw, fPitch, fRoll );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixRotationX ( MATRIX & mOutMatrix, float fRotation )
{
	D3DXMatrixRotationX( (D3DXMATRIXA16*)&mOutMatrix, fRotation );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixRotationY ( MATRIX & mOutMatrix, float fRotation )
{
	D3DXMatrixRotationY( (D3DXMATRIXA16*)&mOutMatrix, fRotation );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixRotationZ ( MATRIX & mOutMatrix, float fRotation )
{
	D3DXMatrixRotationZ( (D3DXMATRIXA16*)&mOutMatrix, fRotation );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixRotationAxis( MATRIX & mOutMatrix, const VECTOR & vAxis, float fRadians )
{
	D3DXMatrixRotationAxis( (D3DXMATRIXA16*)&mOutMatrix, (const D3DXVECTOR3 *)&vAxis, fRadians );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixTranslationScale ( MATRIX * pOutMatrix, const VECTOR * pPosition, const VECTOR * pvScale )
{
	D3DXMatrixTransformation((D3DXMATRIXA16*)pOutMatrix, 0, 0, (const D3DXVECTOR3 *)pvScale, 0, 0, (D3DXVECTOR3 *)pPosition);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixScale ( MATRIX * pOutMatrix, const VECTOR * pvScale )
{
	D3DXMatrixScaling( (D3DXMATRIXA16*)pOutMatrix, pvScale->x, pvScale->y, pvScale->z );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixScale ( MATRIX * pOutMatrix, float fScale )
{
	D3DXMatrixScaling( (D3DXMATRIXA16*)pOutMatrix, fScale, fScale, fScale );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixInverse ( MATRIX * pOutMatrix, const MATRIX * pInMatrix )
{
	D3DXMatrixInverse( (D3DXMATRIXA16*)pOutMatrix, NULL, (D3DXMATRIXA16*)pInMatrix );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixIdentity ( MATRIX * pMatrix )
{
	D3DXMatrixIdentity( (D3DXMATRIX*)pMatrix );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float MatrixDeterminant ( const MATRIX * pMatrix )
{
	return D3DXMatrixDeterminant( (D3DXMATRIXA16*)pMatrix );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixLookAt ( MATRIX * pMatrix, VECTOR * pvEye, VECTOR * pvLookAt )
{
	D3DXVECTOR3 vEyePt((const float *)pvEye);
	D3DXVECTOR3 vLookatPt((const float *)pvLookAt);
	D3DXVECTOR3 vUpVec(0.0f, 0.0f, 1.0f);
	D3DXMatrixLookAtLH((D3DXMATRIXA16*)pMatrix, &vEyePt, &vLookatPt, &vUpVec);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// returns true if same, false if not
BOOL MatrixCompare( const MATRIX * pMatrix1, const MATRIX *pMatrix2, float fError )
{
	float *pfSrc1 = (float *)pMatrix1;
	float *pfSrc2 = (float *)pMatrix2;
	for ( int i = 0; i < 16; i++, pfSrc1++, pfSrc2++ )
	{
		float fDelta = fabsf( *pfSrc1 - *pfSrc2 );
		if ( fDelta >= fError )
			return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMakeView (
	MATRIX & mOutMatrix, 
	const VECTOR & vEye, 
	const VECTOR & vAt, 
	const VECTOR & vUp )
{
	D3DXMatrixLookAtLH(
		(D3DXMATRIXA16 *)&mOutMatrix,
		(D3DXVECTOR3   *)&vEye,
		(D3DXVECTOR3   *)&vAt,
		(D3DXVECTOR3   *)&vUp );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMakePerspectiveProj (
	MATRIX & mOutMatrix, 
	float fFOVy, 
	float fAspect, 
	float fZNear, 
	float fZFar )
{
	D3DXMatrixPerspectiveFovLH(
		(D3DXMATRIXA16 *)&mOutMatrix,
		fFOVy,
		fAspect,
		fZNear,
		fZFar );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// RECTs are in 0..nWidth and 0..nHeight pixels
void MatrixMakeOffCenterPerspectiveProj (
	MATRIX & mOutMatrix, 
	RECT * ptOrigRect,
	RECT * ptScreenRect,
	float fFOVy,
	float fZNear, 
	float fZFar )
{
	float fAspect = float( ptOrigRect->right - ptOrigRect->left ) / float( ptOrigRect->bottom - ptOrigRect->top );
	ASSERT( fAspect > 0.0f );

	// set up proj matrix to fit screen coords
	// generate the off-center perspective projection, compensating for shrinkage of viewport
	float fH = fZNear / ( 1.0f / tanf( fFOVy * 0.5f ) );
	float fW = fAspect * fH;

	float fOX = float( ptOrigRect->right  - ptOrigRect->left );
	float fOY = float( ptOrigRect->bottom - ptOrigRect->top  );

	float fL, fR, fT, fB;
	fL = ( ptScreenRect->left   - ptOrigRect->left ) / fOX;
	fR = ( ptScreenRect->right  - ptOrigRect->left ) / fOX;
	fT = ( ptScreenRect->top    - ptOrigRect->top  ) / fOY;
	fB = ( ptScreenRect->bottom - ptOrigRect->top  ) / fOY;

	float fT2 = 1.0f - fB;
	float fB2 = 1.0f - fT;
	fT = fT2;
	fB = fB2;

	fL = ( fL * 2.0f - 1.0f ) * fW;
	fR = ( fR * 2.0f - 1.0f ) * fW;
	fT = ( fT * 2.0f - 1.0f ) * fH;
	fB = ( fB * 2.0f - 1.0f ) * fH;

	D3DXMatrixPerspectiveOffCenterLH( (D3DXMATRIXA16*)&mOutMatrix, fL, fR, fT, fB, fZNear, fZFar );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMakeOrthoProj ( 
	MATRIX & mOutMatrix, 
	float fWidth, 
	float fHeight, 
	float fZNear, 
	float fZFar )
{
	D3DXMatrixOrthoLH(
		(D3DXMATRIXA16 *)&mOutMatrix,
		fWidth,
		fHeight,
		fZNear,
		fZFar );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixMakeWFriendlyProjection(
	MATRIX * pmProj )
{
	if ( pmProj->_34 != 1.0f )
	{
		// need to scale entire matrix to be w-friendly
		float fScale = 1.0f / pmProj->_34;
		pmProj->_11 *= fScale;
		pmProj->_22 *= fScale;
		pmProj->_33 *= fScale;
		pmProj->_43 *= fScale;
		pmProj->_34  = 1.0f;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VectorCatMullRom(
	VECTOR & vPosition,
	const VECTOR & vPointA,
	const VECTOR & vPointB,
	const VECTOR & vPointC,
	const VECTOR & vPointD,
	float fWeight)
{
	D3DXVECTOR3 Position( vPosition.fX, vPosition.fY, vPosition.fZ );
	D3DXVECTOR3 PointA( vPointA.fX, vPointA.fY, vPointA.fZ );
	D3DXVECTOR3 PointB( vPointB.fX, vPointB.fY, vPointB.fZ );
	D3DXVECTOR3 PointC( vPointC.fX, vPointC.fY, vPointC.fZ );
	D3DXVECTOR3 PointD( vPointD.fX, vPointD.fY, vPointD.fZ );
	D3DXVec3CatmullRom( &Position,
		&PointA,
		&PointB,
		&PointC,
		&PointD,
		fWeight );

	vPosition.fX = Position.x;
	vPosition.fY = Position.y;
	vPosition.fZ = Position.z;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT VectorProject(
	VECTOR * pvOut,
	const VECTOR * pvSrc,
	const MATRIX * pmProj,
	const MATRIX * pmView  /*= NULL*/,
	const MATRIX * pmWorld /*= NULL*/,
	const RECT * pViewport /*= NULL*/ )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETINVALIDARG( pvOut );
	ASSERT_RETINVALIDARG( pvSrc );
	ASSERT_RETINVALIDARG( pmProj );

	D3DC_VIEWPORT tVP;
	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );
	if ( ! pmView  )			pmView  = &mIdentity;
	if ( ! pmWorld )			pmWorld = &mIdentity;

	if ( ! pViewport )
	{
		V_RETURN( dxC_GetRenderTargetDimensions( (int&)tVP.Width, (int&)tVP.Height ) );
		tVP.TopLeftX = 0;
		tVP.TopLeftY = 0;
	}
	else
	{
		tVP.Width		= pViewport->right - pViewport->left;
		tVP.Height		= pViewport->bottom - pViewport->top;
		tVP.TopLeftX	= pViewport->left;
		tVP.TopLeftY	= pViewport->top;
	}
	tVP.MinDepth = 0.f;
	tVP.MaxDepth = 1.f;

	D3DXVec3Project( (D3DXVECTOR3*)pvOut, (D3DXVECTOR3*)pvSrc, &tVP, (D3DXMATRIXA16*)pmProj, (D3DXMATRIXA16*)pmView, (D3DXMATRIXA16*)pmWorld );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixFromQuaternion( struct MATRIX * pmOrientation, const union Quaternion * ptOrientation )
{
	D3DXMatrixRotationQuaternion( (D3DXMATRIX*)pmOrientation, (const D3DXQUATERNION*)ptOrientation );
}

// build quaternion based on euler angles
void EulerToQuat( Quaternion & tQuat, float roll, float pitch, float yaw )
{
	D3DXQuaternionRotationYawPitchRoll( (D3DXQUATERNION*)&tQuat, yaw, pitch, roll );
	return;

	//roll  *= 0.5f;
	//pitch *= 0.5f;
	//yaw   *= 0.5f;

	//float cr = (float)cos(roll);
	//float cp = (float)cos(pitch);
	//float cy = (float)cos(yaw);

	//float sr = (float)sin(roll);
	//float sp = (float)sin(pitch);
	//float sy = (float)sin(yaw);

	//float cpcy = cp * cy;
	//float spsy = sp * sy;
	//float spcy = sp * cy;
	//float cpsy = cp * sy;

	//tQuat.w = cr * cpcy + sr * spsy;
	//tQuat.x = sr * cpcy - cr * spsy;
	//tQuat.y = cr * spcy + sr * cpsy;
	//tQuat.z = cr * cpsy - sr * spcy;
};

