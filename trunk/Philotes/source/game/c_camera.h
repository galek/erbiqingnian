//----------------------------------------------------------------------------
// c_camera.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_CAMERA_H_
#define _C_CAMERA_H_

//#include "camera_priv.h"

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	VIEW_1STPERSON=0,	// set for definition file compatibilty
	VIEW_FLY=5,			// set for definition file compatibilty
	VIEW_3RDPERSON,
	VIEW_VANITY,
	VIEW_RTS,			// special view for the RTS quest
	VIEW_DIABLO,
	VIEW_DIABLOROTATE,
	VIEW_DIABLOZOOM,
	VIEW_DIABLOZR,
	VIEW_LETTERBOX,
	VIEW_FOLLOWPATH,
	VIEW_MURMUR,
	VIEW_DETACHED_CAMERA,
	VIEW_CHARACTER_SCREEN,
	VIEW_DEMOLEVEL,

	NUM_VIEWS
};

#define VIEW_DRAW_NONE			0
#define VIEW_DRAW_1STPERSON		1
#define VIEW_DRAW_3RDPERSON		3
#define VIEW_DRAW_BOTH			4

int c_CameraGetViewType();

typedef void (*FP_HAMMER_GET_CAMERA_POSITION)(VECTOR & vPosition, float & fAngle, float & fPitch, float & fFlyDistance, float & fFOV);

void c_CameraSetHammerUpdatePositionFunction(
	FP_HAMMER_GET_CAMERA_POSITION fpHammerGetCameraPosition);

void c_CameraUpdate(
	struct GAME * pGame,
	int sim_frames);

const struct CAMERA_INFO * c_CameraGetInfo(
	BOOL bForceFirstPerson );

const struct CAMERA_INFO * c_CameraGetInfoCommon();

void c_CameraRestoreViewType();
void c_CameraSetViewType(
	int nType,
	BOOL bTemporary = FALSE);

void c_CameraZoom(
	BOOL bZoomIn );

void c_CameraInitViewType(
	int nType );

void c_CameraLockViewType();
void c_CameraUnlockViewType();

void c_CameraVanityDistance( BOOL bZoomIn );
void c_CameraVanityRotate( float delta );
void c_CameraVanityPitch( float delta );
void c_CameraSetVanityPitch( float fPitch );

void c_CameraFollowLerp( float fTargetAngle,
						float fDeltaTime );

void c_CameraFollowRotate( float delta,
						   BOOL bNoLerp = TRUE);
void c_CameraFollowPitch( float delta );
void c_CameraResetFollow( void );

void c_CameraSetDefaultModel(
	int nModelId );

float c_CameraGetZDelta();
float c_CameraGetFirstPersonScaling();

void c_CameraResetBone();

void c_MakeFlyCameraEye(
	struct VECTOR *pvEye,
	const struct VECTOR *pvLookAt,
	float fRotation,
	float fPitch,
	float fDistance,
	BOOL bAllowStaleData = FALSE );

void c_CameraShake(
   struct GAME *game,
   struct VECTOR & vDirection );

void c_CameraShakeRandom(
	struct GAME * pGame,
	const struct VECTOR & vDirection,
	const float fDuration,
	const float fMagnitude,
	const float fDegrade);

void c_CameraShakeOFF();

BOOL c_CameraIsFirstPersonFOVEnabled(
	struct GAME * pGame );

void c_CameraAdjustPointForFirstPersonFOV(
	struct GAME * pGame,
	struct VECTOR & vPosition,
	BOOL bAllowStaleData = FALSE );

void c_CameraDollyIn( void );

void c_CameraDollyOut( void );
void c_CameraSetLetterbox(
	struct GAME * game,
	struct VECTOR * pvNodeList,
	int nNumNodes,
	float fTime );


void c_CameraFollowPath(
	struct GAME * game,
	struct VECTOR * pvNodeList,
	int nNumNodes,
	float fTime );

void c_CameraGetViewMatrix(
	const struct CAMERA_INFO * pCamera,
	struct MATRIX * pmatDest );

void c_CameraGetProjMatrix(
	const struct CAMERA_INFO * pCamera,
	struct MATRIX * pmatDest );

VECTOR c_GetCameraLocation(
	struct GAME * pGame );

void c_CameraUpdateCallback();

void c_CameraGetNearFar(
	float & fNear,
	float & fFar );

VECTOR CameraInfoGetPosition(
	const struct CAMERA_INFO * pCamera);

VECTOR CameraInfoGetDirection(
	const struct CAMERA_INFO * pCamera);

VECTOR CameraInfoGetLookAt(
	const struct CAMERA_INFO * pCamera);

float CameraInfoGetPitch(
	const struct CAMERA_INFO * pCamera);

float CameraInfoGetAngle(
	const struct CAMERA_INFO * pCamera);

float c_CameraGetCosFOV(
	float fWidenFactor );

// DRB_3RD_PERSON
void CameraForceRotate();
BOOL CameraRotateDetached( VECTOR & vDirection );

// RTS
void c_CameraSetRTS_TargetPosition(
	VECTOR & vTarget );

void c_CameraSetVerticalFov( 
	int nVertFOVDegrees, 
	int nViewType = INVALID_ID );

int c_CameraGetVerticalFovDegrees( 
	int nViewType = INVALID_ID );

int c_CameraGetHorizontalFovDegrees( 
	int nViewType );

void c_CameraRestoreFov();

void c_CameraCharacterScreenDistance( BOOL bZoomIn );

#endif // _C_CAMERA_H_
