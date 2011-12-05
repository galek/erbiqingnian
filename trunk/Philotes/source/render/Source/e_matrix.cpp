//----------------------------------------------------------------------------
// e_matrix.cpp
//
// - Implementation for matrix functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_matrix.h"
#include "appcommon.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void MatrixMultiply ( MATRIX * pMatrix, float fScalar )
{
	float * pfValue = (float *) pMatrix;
	for ( int i = 0; i < 16; i++ )
	{
		*pfValue *= fScalar;
		pfValue++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void MatrixFromVectors ( MATRIX & mOutMatrix, const VECTOR & vPosition, const VECTOR & vUpIn, const VECTOR & vForwardIn )
{
	VECTOR vUp = vUpIn;
	VECTOR vForward = vForwardIn;
	if ( vForward == vUp )
		vUp = VECTOR( 0, 1, 0 );
	else if ( vForward == -vUp )
		vUp = VECTOR( 0, 1, 0 );

	if ( vForward == vUp )
		vUp = VECTOR( 1, 0, 0 );
	else if ( vForward == -vUp )
		vUp = VECTOR( 1, 0, 0 );

	VECTOR vSide;
	VectorCross( vSide, vForward, vUp );
	{
		ASSERT_DO( fabsf( 1.0f - VectorLengthSquared( vUp )	) < 1e-2f ) 
		{
			VectorNormalize( vUp );
		}
	}
	{
		ASSERT_DO( fabsf( 1.0f - VectorLengthSquared( vForward ) ) < 1e-2f ) 
		{
			VectorNormalize( vForward );
		}
	}
	VectorNormalize( vSide, vSide );
	VECTOR vForwardSideCross;
	VectorCross( vForwardSideCross, vSide, vForward );
	VectorNormalize( vForwardSideCross, vForwardSideCross );
	if ( VectorDot( vForwardSideCross, vUpIn ) < 0 )
		vForwardSideCross = -vForwardSideCross;
	mOutMatrix._11 = vSide.fX;
	mOutMatrix._12 = vSide.fY;
	mOutMatrix._13 = vSide.fZ;
	mOutMatrix._21 = vForward.fX;
	mOutMatrix._22 = vForward.fY;
	mOutMatrix._23 = vForward.fZ;
	mOutMatrix._31 = vForwardSideCross.fX;
	mOutMatrix._32 = vForwardSideCross.fY;
	mOutMatrix._33 = vForwardSideCross.fZ;

	mOutMatrix._14 = 0.0f;
	mOutMatrix._24 = 0.0f;
	mOutMatrix._34 = 0.0f;
	mOutMatrix._41 = vPosition.fX;
	mOutMatrix._42 = vPosition.fY;
	mOutMatrix._43 = vPosition.fZ;
	mOutMatrix._44 = 1.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// fRotationDeg is in degrees, not radians
void GetAlignmentMatrix(MATRIX * pMatrixOut, 
						MATRIX & tWorldMatrix,
						const VECTOR & vPosition,
						VECTOR & vNormalIn, 
						float fRotationDeg, 
						VECTOR * vNormalUpIn, /*=NULL*/
						const VECTOR * vScale /*=NULL*/)
{
	ASSERT_RETURN(pMatrixOut);

	VECTOR vNormal = vNormalIn;
	VECTOR vNormalUp;
	if (VectorIsZero(vNormal))
	{
		vNormal = VECTOR(0, 0, 1);
		if(AppIsTugboat())
		{
			vNormalUp = VECTOR(0, 1, 0);
		}
		else
		{
			// This is the "correct" way of doing things.  For some reason, Tugboat uses a left-handed layout system, which is backwards
			vNormalUp = VECTOR(0, -1, 0);
		}
	}
	else
	{
		VectorNormalize(vNormal);
		vNormalUp = GetNormalUpVector(vNormal);
	}

	MATRIX matRotate, matRotateZ;
	MatrixRotationZ(matRotateZ, (fRotationDeg*PI)/180.0f);

	MatrixFromVectors(matRotate, vPosition, vNormal, vNormalUp);

	MATRIX matScale;
	MatrixIdentity( &matScale );
	if( vScale )
	{
		MatrixScale( &matScale, vScale );		
	}

	*pMatrixOut = matRotate;
	MATRIX matWorld = tWorldMatrix;
	MatrixMultiply(pMatrixOut, &matRotateZ, pMatrixOut );

	MatrixMultiply(pMatrixOut, pMatrixOut, &tWorldMatrix);
	MatrixMultiply(pMatrixOut, &matScale, pMatrixOut );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VectorDirectionFromAnglePitchRadians( VECTOR & vDirection, float fAngle, float fPitch )
{
	vDirection.fX = cosf( fAngle );
	vDirection.fY = sinf( fAngle );
	vDirection.fZ = 0.0f;
	if ( fPitch != 0.0f )
	{
		VECTOR vSide;
		vSide.fX = vDirection.fY;
		vSide.fY = -vDirection.fX;
		vSide.fZ = 0.0f;
		MATRIX mRotation;
		MatrixRotationAxis( mRotation, vSide, fPitch );
		MatrixMultiply( &vDirection, &vDirection, &mRotation );
	} else {
	}
	VectorNormalize( vDirection );
}
