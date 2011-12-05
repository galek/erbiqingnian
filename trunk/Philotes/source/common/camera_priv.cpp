// ---------------------------------------------------------------------------
// camera_priv.cpp
// ---------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "camera_priv.h"
#include "boundingbox.h"

#include "boundingbox.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
namespace
{
	PFN_CAMERA_GET_INFO		gpfn_CameraGetInfo		= NULL;
	PFN_CAMERA_UPDATE		gpfn_CameraUpdate		= NULL;
	PFN_CAMERA_GET_NEAR_FAR gpfn_CameraGetNearFar	= NULL;
	BOOL					gbCameraInfoUpdated		= FALSE;
};


//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitCameraFunctions(
	PFN_CAMERA_GET_INFO pfnGetInfo,
	PFN_CAMERA_UPDATE pfnUpdate,
	PFN_CAMERA_GET_NEAR_FAR pfnGetNearFar )
{
	gpfn_CameraGetInfo		= pfnGetInfo;
	gpfn_CameraUpdate		= pfnUpdate;
	gpfn_CameraGetNearFar	= pfnGetNearFar;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CAMERA_INFO * CameraGetInfo()
{
	ASSERT_RETNULL( gpfn_CameraGetInfo )
	return gpfn_CameraGetInfo();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraUpdate()
{
	ASSERT_RETURN( gpfn_CameraUpdate );
	gpfn_CameraUpdate();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraGetNearFar( float & fNear, float & fFar )
{
	ASSERT_RETURN( gpfn_CameraGetNearFar );
	gpfn_CameraGetNearFar( fNear, fFar );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraMakeFlyEye(
	VECTOR *pvEye,
	const VECTOR *pvLookAt,
	float fRotation,
	float fPitch,
	float fDistance,
	BOOL bAllowStaleData )
{
	UNREFERENCED_PARAMETER(bAllowStaleData);

	//ASSERT( bAllowStaleData || CameraGetUpdated() );
	ASSERT_RETURN( pvEye && pvLookAt );
	*pvEye = VECTOR ( fDistance, 0.0f, 0.0f );
	MATRIX matRotateEye;
	MatrixTransform ( &matRotateEye, NULL, fRotation, -fPitch );
	MatrixMultiply( pvEye, pvEye, &matRotateEye );
	*pvEye += *pvLookAt;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraGetViewMatrix(
	const CAMERA_INFO * pInfo,
	MATRIX * pmView,
	BOOL bAllowStaleData )
{
	ASSERT_RETURN( pInfo );
	ASSERT_RETURN( pmView );
	ASSERT( bAllowStaleData || CameraGetUpdated() );

	VECTOR vUp( 0.f, 0.f, 1.f );
	MatrixMakeView(
		*pmView, 
		pInfo->vPosition, 
		pInfo->vLookAt, 
		vUp );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraGetProjMatrix(
	const CAMERA_INFO * pInfo,
	MATRIX * pmProj,
	BOOL bAllowStaleData /*= FALSE*/,
	BOUNDING_BOX* pViewportBox /*= NULL*/,
	GAME_WINDOW * pGameWindow /*= NULL*/ )
{
#if !ISVERSION( SERVER_VERSION )
	ASSERT_RETURN( pInfo );
	ASSERT_RETURN( pmProj );
	ASSERT( bAllowStaleData || CameraGetUpdated() );

	float fNear = 0.5f, fFar = 10.0f;  // should be starkly erroneous
	CameraGetNearFar( fNear, fFar );

	switch (pInfo->eProjType)
	{
	case PROJ_PERSPECTIVE:
		if( pViewportBox )
		{
			MatrixMakePerspectiveProj(
				*pmProj,
				pInfo->fVerticalFOV,
				( pViewportBox->vMax.fX - pViewportBox->vMin.fX ) / ( pViewportBox->vMax.fY - pViewportBox->vMin.fY ),
				fNear,
				fFar );
		}
		else
		{
			int nWindowWidth = pGameWindow ? pGameWindow->m_nWindowWidth : AppCommonGetWindowWidth();
			int nWindowHeight = pGameWindow ? pGameWindow->m_nWindowHeight : AppCommonGetWindowHeight();

			MatrixMakePerspectiveProj(
				*pmProj,
				pInfo->fVerticalFOV,
				nWindowWidth / (float)nWindowHeight,
				fNear,
				fFar );
		}
		// enables linear depth in the projected .w
		MatrixMakeWFriendlyProjection( pmProj );
		break;

	case PROJ_ORTHO:
		MatrixMakeOrthoProj(
			*pmProj,
			pInfo->fOrthoWidth,
			pInfo->fOrthoHeight,
			pInfo->fOrthoNear,
			pInfo->fOrthoFar );
		break;
	default:
		ASSERTX( 0, "Invalid camera projection type!" );
	}
#else
	REF( pInfo );
	REF( pmProj );
	REF( bAllowStaleData );
	REF( pViewportBox );
	REF( pGameWindow );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CameraSetUpdated( BOOL bUpdated )
{
	gbCameraInfoUpdated = bUpdated;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CameraGetUpdated()
{
	return gbCameraInfoUpdated;
}
