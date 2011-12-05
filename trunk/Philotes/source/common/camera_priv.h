// ---------------------------------------------------------------------------
// camera_priv.h
// ---------------------------------------------------------------------------
#ifndef __CAMERA_PRIV__
#define __CAMERA_PRIV__


struct BOUNDING_BOX; 
//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum PROJ_TYPE
{
	PROJ_PERSPECTIVE = 0,
	PROJ_ORTHO,

	NUM_PROJ_TYPES
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct CAMERA_INFO
{
	// view
	VECTOR vPosition;
	VECTOR vLookAt;
	VECTOR vAimAt;
	VECTOR vDirection;
	float fAngle;
	float fPitch;

	// projection
	PROJ_TYPE eProjType;
	float fVerticalFOV;
	float fOrthoWidth;
	float fOrthoHeight;
	float fOrthoNear;
	float fOrthoFar;
};


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef const CAMERA_INFO * (*PFN_CAMERA_GET_INFO)();
typedef void (*PFN_CAMERA_UPDATE)();
typedef void (*PFN_CAMERA_GET_NEAR_FAR)( float & fNear, float & fFar );


//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
void InitCameraFunctions(
	PFN_CAMERA_GET_INFO pfnGetInfo,
	PFN_CAMERA_UPDATE pfnUpdate,
	PFN_CAMERA_GET_NEAR_FAR pfnGetNearFar);

const CAMERA_INFO * CameraGetInfo(
	void);

void CameraUpdate(
	void);

void CameraGetNearFar(
	float & fNear,
	float & fFar);

void CameraMakeFlyEye(
	VECTOR *pvEye,
	const VECTOR *pvLookAt,
	float fRotation,
	float fPitch,
	float fDistance,
	BOOL bAllowStaleData = FALSE);

void CameraGetViewMatrix(
	const CAMERA_INFO * pInfo,
	struct MATRIX * mView,
	BOOL bAllowStaleData = FALSE);

void CameraGetProjMatrix(
	const CAMERA_INFO * pInfo,
	MATRIX * mProj,
	BOOL bAllowStaleData = FALSE,
	BOUNDING_BOX * pViewport = NULL,
	GAME_WINDOW * pGameWindow = NULL );
void CameraSetUpdated( BOOL bUpdated );
BOOL CameraGetUpdated();

#endif
