//----------------------------------------------------------------------------
// c_camera.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"

#include "c_camera.h"
#include "camera_priv.h"
#include "units.h" // also includes game.h
#include "player.h"
#include "appcommontimer.h"

#include "c_appearance.h"
//#ifdef HAMMER
#include "holodeck.h"
//#endif
#include "debugwindow.h"
#include "s_gdi.h"
#include "uix.h"
#include "spline.h"

#include "inventory.h"

#include "e_model.h"
#include "e_main.h"
#include "e_math.h"
#include "e_settings.h"
#include "c_appearance_priv.h"
#include "level.h"
#include "uix_priv.h"
#include "states.h"

#include "room_layout.h"
#include "room_path.h"
#include "skills.h"

#include "unitmodes.h"

#include "demolevel.h"
#include "c_input.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define FIRST_PERSON_Z_DELTA		-0.05f
#define CAMERA_HEAD_FEET_OFFSET		0.5f

int sgnViewType = VIEW_3RDPERSON;
static int sgnViewTypePrev = VIEW_3RDPERSON;

static int sgnViewTypeLocked = 0; // reference counts requests

static CAMERA_INFO sgtCameraInfoFirst;
static CAMERA_INFO sgtCameraInfoView;
static BOOL sgbHasUpdatedOnce = FALSE;

static VECTOR sgvDefaultEyePosition = VECTOR( 0.0f, 0.0f, 0.0f );
static int sgnDefaultModel = INVALID_ID;
static int sgnCameraBone = INVALID_ID;
static float sgfLastImpactTime = 0;

//float sgfDefaultFOV_Tugboat = DEG_TO_RAD(float(40));
//float sgfDefaultFOV_1stPerson = DEG_TO_RAD(float(100));	// Horizontal FOV in radians
//float sgfDefaultFOV_3rdPerson = DEG_TO_RAD(float(110));	// Horizontal FOV in radians

const float MAX_HORIZONTAL_FOV_RAD = DEG_TO_RAD( 120.0f );

// Vertical FOV in radians
float sgfDefaultVertFOV_Tugboat	  = DEG_TO_RAD( 30.0f );
float sgfDefaultVertFOV_1stPerson = DEG_TO_RAD( 85.0f );
float sgfDefaultVertFOV_3rdPerson = DEG_TO_RAD( 80.0f );


float sgfVertFOV_Tugboat   = sgfDefaultVertFOV_Tugboat;
float sgfVertFOV_1stPerson = sgfDefaultVertFOV_1stPerson;
float sgfVertFOV_3rdPerson = sgfDefaultVertFOV_3rdPerson;

static const INVENTORY_VIEW_INFO * sgpCharacterCamera = NULL;
static VECTOR sgvCharacterFocus = VECTOR( 0 );

//----------------------------------------------------------------------------
// 3rd person info

#define MAX_3RDPERSON_DISTANCE			5.0f
#define MIN_3RDPERSON_DISTANCE			1.5f
#define THIRD_PERSON_ZOOM_ADD			0.5f
#define THIRD_PERSON_CAMERA_HEIGHT		( PLAYER_HEIGHT + 0.4f ) //tugboat
#define START_3RDPERSON_DISTANCE		( ( MAX_3RDPERSON_DISTANCE - MIN_3RDPERSON_DISTANCE ) / 2.0f )
//#define START_3RDPERSON_DISTANCE		( MIN_3RDPERSON_DISTANCE )

#define CAMERA_COLLIDE_BUMP				0.2f
#define CAMERA_VERTICAL_BUMP			( CAMERA_COLLIDE_BUMP * 2.0f )
#define CAMERA_HORIZONTAL_BUMP			( CAMERA_COLLIDE_BUMP * 2.0f )
#define RTS_CAMERA_COLLIDE_BUMP			0.5f
#define THIRD_PERSON_LOOKAT_DISTANCE	0.2f
#define CAMERA_MOVE_IN_FACTOR			0.2f
#define MAX_VANITY_DISTANCE				10.0f
#define MIN_VANITY_DISTANCE				1.5f
#define CAMERA_RADIUS					0.3f
#define RTS_CAMERA_RADIUS				0.5f
#define MAX_VANITY_DISTANCE_TUGBOAT		20.0f
#define MIN_VANITY_DISTANCE_TUGBOAT		3.0f

static VECTOR sgvLastPosition;
static VECTOR sgvLastDirection;

static float sgfOldThirdPersonXYDistanceSquared = START_3RDPERSON_DISTANCE;

// DRB_3RD_PERSON
//tugboat code
static VECTOR sgvLastLookAt;
static float sgfLastPitch = 0.0f;
static BOOL sgbCameraForceRotate = TRUE;
static BOOL sgbCameraRotateDetached = FALSE;

static BOOL sgb3rdPersonInited = FALSE;
static float sgfThirdDistance = START_3RDPERSON_DISTANCE;
static float sgfThirdDistanceCurr = START_3RDPERSON_DISTANCE;

#define VANITY_DISTANCE_DEFAULT 2.0f
#define VANITY_DISTANCE_DEFAULT_TUGBOAT 5.5f
#define VANITY_PITCH_DEFAULT 0.8f
#define VANITY_PITCH_DEFAULT_TUGBOAT .69f
#define FOLLOW_PITCH_DEFAULT_TUGBOAT .65f
#define FOLLOW_DISTANCE_DEFAULT_TUGBOAT 12.0f
static float sgfVanityDistance = VANITY_DISTANCE_DEFAULT;
static float sgfVanityDistanceTugboat = VANITY_DISTANCE_DEFAULT_TUGBOAT;
static float sgfVanityAngle = 0.0f;
static float sgfVanityPitch = VANITY_PITCH_DEFAULT;

#define MIN_FOLLOW_PITCH  .5f

static float sgfFollowAngle = 0.0f;
static float sgfFollowAngleCurrent = 0.0f;
static float sgfFollowPitch = FOLLOW_PITCH_DEFAULT_TUGBOAT;

// Hellgate Character Create face zoom
#define MIN_CHARACTER_SCREEN_DISTANCE		1.5f
#define MAX_CHARACTER_SCREEN_DISTANCE		3.08f

static float sgfCharacterScreenDistance = MAX_CHARACTER_SCREEN_DISTANCE;
static float sgfCurrentCharScreenDistance = MAX_CHARACTER_SCREEN_DISTANCE;

//----------------------------------------------------------------------------
// client camera shaking from soft hits

static BOOL sgbShake = FALSE;
static VECTOR sgvShake, sgvShakeDirection;
static float sgfShakeTime;
static float sgfShakeMagnitude;

#define SHAKE_SCALE		0.005f
#define SHAKE_DOWN		0.00001f
#define SHAKE_TIME		0.5f

static BOOL sgbRandomShake = FALSE;
static VECTOR sgvRandomShake, sgvRandomShakeDirection;
static float sgfRandomShakeTime = 0.0f, sgfRandomShakeStartTime = 0.0f;
static float sgfRandomShakeDegrade = 0.0f;
static float sgfRandomShakeMagnitude = 0.0f;

#define CAMERA_DOLLY_INCREMENT		.5f
#define CAMERA_DOLLY_INCREMENT_VANITY		1.0f
#define CAMERA_DOLLY_NEAR_CAP		6.6f
#define CAMERA_DOLLY_FAR_CAP		29.7f
#define CAMERA_DOLLY_START			29.7f
#define CAMERA_FOLLOW_NEAR_CAP		12.0f
#define CAMERA_DOLLY_LOW_CAP		17.0f
//TUGBOAT

// Follow Path timing
static BOOL sgbFollowPath = FALSE;
static TIME sgFollowPathDeltaTime, sgFollowPathMoveStartTime;
static VECTOR sgvCameraPathPosition;

// Demo level run
static BOOL sgbDemoLevelPath = FALSE;


// RTS
#define RTS_SCROLL_RATE		0.4f
#define RTS_DISTANCE		12.0f

static VECTOR sgvRTS_TargetPosition = VECTOR(0);
static VECTOR sgvRTS_CurrentPosition = VECTOR(0);

//----------------------------------------------------------------------------

static float sgfCameraCrouchDelta = 0.0f;
static float sgCameraDolly = CAMERA_DOLLY_START; //tugboat
static float sgLastFollowDolly = CAMERA_DOLLY_START; //tugboat
static float sgLastCameraDolly = CAMERA_DOLLY_START; //tugboat
static float sgLastVanityDolly = VANITY_DISTANCE_DEFAULT_TUGBOAT; //tugboat

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static float sGetVerticalFOV( int nViewTypeOverride = INVALID_ID )
{
	if( AppIsTugboat() )
		return sgfVertFOV_Tugboat;
	int nViewType = nViewTypeOverride;
	if ( nViewType == INVALID_ID )
		nViewType = sgnViewType;

	float fFOV;
	switch (nViewType)
	{
	case VIEW_3RDPERSON:
		fFOV = sgfVertFOV_3rdPerson;
	case VIEW_1STPERSON:
	default:
		fFOV = sgfVertFOV_1stPerson;
	}

#if ISVERSION(SERVER_VERSION)
	return fFOV;
#else
	// Limit the horizontal FOV
	float fH = e_GetHorizontalFOV( fFOV );
	fH = MIN( fH, MAX_HORIZONTAL_FOV_RAD );
	return e_GetVerticalFOV( fH );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraSetVerticalFov( 
	int nVertFOVDegrees, 
	int nViewType )
{
	float * pfFOV = NULL;
	if( AppIsTugboat() )
		pfFOV = &sgfVertFOV_Tugboat;
	else
	{
		switch (nViewType)
		{
		case VIEW_3RDPERSON:
			pfFOV = &sgfVertFOV_3rdPerson;
			break;

		case VIEW_1STPERSON:
		default:
			pfFOV = &sgfVertFOV_1stPerson;
			break;
		}
	}

	*pfFOV = DEG_TO_RAD( (float)nVertFOVDegrees );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_CameraGetVerticalFovDegrees( 
	int nViewType )
{
	float fFOV = sGetVerticalFOV( nViewType );
	return (int)RAD_TO_DEG( fFOV );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_CameraGetHorizontalFovDegrees( 
	int nViewType )
{
#if ! ISVERSION(SERVER_VERSION)
	float fFOV = sGetVerticalFOV( nViewType );
	fFOV = e_GetHorizontalFOV( fFOV );
	return (int)RAD_TO_DEG( fFOV );
#else
	REF( nViewType );
	return 0;
#endif // SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraRestoreFov()
{
	// Vertical FOV in radians
	sgfVertFOV_Tugboat   = sgfDefaultVertFOV_Tugboat;
	sgfVertFOV_1stPerson = sgfDefaultVertFOV_1stPerson;
	sgfVertFOV_3rdPerson = sgfDefaultVertFOV_3rdPerson;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_CameraGetViewType()
{
	return sgnViewType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sGetThirdPersonCameraHeight(
	UNIT * pUnit )
{
	if( AppIsTugboat() )
		return THIRD_PERSON_CAMERA_HEIGHT;
	float fHeight = PLAYER_HEIGHT + 0.3f;

	int nModelId = c_UnitGetModelId( pUnit );
	if ( nModelId == INVALID_ID )
		return fHeight;

	fHeight *= c_AppearanceGetHeightFactor( nModelId );

	return fHeight;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_MakeFlyCameraEye(
	VECTOR *pvEye,
	const VECTOR *pvLookAt,
	float fRotation,
	float fPitch,
	float fDistance,
	BOOL bAllowStaleData )
{
	CameraMakeFlyEye( pvEye, pvLookAt, fRotation, fPitch, fDistance, bAllowStaleData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sBumpCamera( GAME * pGame, LEVEL * pLevel, VECTOR & vPosition, float fAngle )
{
	// send a ray down from the camera to see if we are close to some floor
	VECTOR vNormal;
	VECTOR vRayDir = VECTOR( 0.0f, 0.0f, -1.0f );
	float fRayLength = CAMERA_VERTICAL_BUMP;
	float fDistance = LevelLineCollideLen( pGame, pLevel, vPosition, vRayDir, fRayLength, &vNormal );
	if ( fDistance < fRayLength )
	{
		float fDelta = CAMERA_VERTICAL_BUMP - fDistance;
		vPosition += vNormal * fDelta;
	}
	// and if we are close to the ceiling...
	vRayDir = VECTOR( 0.0f, 0.0f, 1.0f );
	fDistance = LevelLineCollideLen( pGame, pLevel, vPosition, vRayDir, fRayLength, &vNormal );
	if ( fDistance < fRayLength )
	{
		float fDelta = CAMERA_VERTICAL_BUMP - fDistance;
		vPosition += vNormal * fDelta;
	}
	// left side
	vRayDir.fX = cosf( fAngle - ( PI / 2.0f ) );
	vRayDir.fY = sinf( fAngle - ( PI / 2.0f ) );
	vRayDir.fZ = 0.0f;
	fDistance = LevelLineCollideLen( pGame, pLevel, vPosition, vRayDir, fRayLength, &vNormal );
	if ( fDistance < fRayLength )
	{
		float fDelta = CAMERA_HORIZONTAL_BUMP - fDistance;
		vPosition += vNormal * fDelta;
	}
	// right side
	vRayDir.fX = cosf( fAngle + ( PI / 2.0f ) );
	vRayDir.fY = sinf( fAngle + ( PI / 2.0f ) );
	vRayDir.fZ = 0.0f;
	fDistance = LevelLineCollideLen( pGame, pLevel, vPosition, vRayDir, fRayLength, &vNormal );
	if ( fDistance < fRayLength )
	{
		float fDelta = CAMERA_HORIZONTAL_BUMP - fDistance;
		vPosition += vNormal * fDelta;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static VECTOR sgvCameraCurve[MAX_SPLINE_KNOTS];
static int sgnCameraKnots[MAX_SPLINE_KNOTS+SPLINE_T];
static int sgnNumCameraKnots = 0;

#if (0)

#define TESTCURVE_SCALE		8.0f
#define Y_DELTA				300

static void sDrawTestCurve()
{
	for ( int i = 0; i < sgnNumCameraKnots; i++ )
	{
		int x1 = (int)FLOOR( ( sgvCameraCurve[i].fX - sgvCameraCurve[0].fX ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
		int y1 = (int)FLOOR( ( sgvCameraCurve[i].fY - sgvCameraCurve[0].fY ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
		gdi_drawbox( x1-1,y1-1,x1+1,y1+1, gdi_color_green );
	}

	VECTOR old = VECTOR( 0.0f, 0.0f, 0.0f );
	for ( float t = 0.0f; t < 1.0f; t += 0.05f )
	{
		VECTOR newpos;
		SplineGetPosition( &sgvCameraCurve[0], sgnNumCameraKnots, &sgnCameraKnots[0], &newpos, t );
		newpos -= sgvCameraCurve[0];

		int nX1 = (int)FLOOR( old.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
		int nY1 = (int)FLOOR( old.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
		REF(nX1);
		REF(nY1);

		int nX2 = (int)FLOOR( newpos.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
		int nY2 = (int)FLOOR( newpos.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
		REF(nX2);
		REF(nY2);

		//gdi_drawline( nX1, nY1, nX2, nY2, gdi_color_white );
		gdi_drawpixel( nX2, nY2, gdi_color_white );

		old = newpos;
	}

	old = VECTOR( 0.0f, 0.0f, 0.0f );
	for ( int k = 0; k < sgnNumCameraKnots; k++ )
	{
		for ( float t = 0.0f; t < 1.0f; t += 0.1f )
		{
			VECTOR newpos, newdir;
			SplineGetCatmullRomPosition( &sgvCameraCurve[0], sgnNumCameraKnots, k, t, &newpos, &newdir );
			newpos -= sgvCameraCurve[0];

			int nX1 = (int)FLOOR( old.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
			int nY1 = (int)FLOOR( old.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
			REF(nX1);
			REF(nY1);

			int nX2 = (int)FLOOR( newpos.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
			int nY2 = (int)FLOOR( newpos.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;

			//gdi_drawline( nX1, nY1, nX2, nY2, gdi_color_red );
			gdi_drawpixel( nX2, nY2, gdi_color_red );

			old = newpos;
		}
	}

	VECTOR newpos;
	SplineGetPosition( &sgvCameraCurve[0], sgnNumCameraKnots, &sgnCameraKnots[0], &newpos, 1.0f );
	newpos -= sgvCameraCurve[0];

	int nX1 = (int)FLOOR( old.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
	int nY1 = (int)FLOOR( old.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
	REF(nX1);
	REF(nY1);

	int nX2 = (int)FLOOR( newpos.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
	int nY2 = (int)FLOOR( newpos.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;

	//gdi_drawline( nX1, nY1, nX2, nY2, gdi_color_white );
	gdi_drawpixel( nX2, nY2, gdi_color_white );
}

#define TEST_TIME_DELTA			3.0f

void drbTestCurve()
{
/*	sgvCameraCurve[0] = VECTOR( 10.0f,  0.0f, 0.0f );
	sgvCameraCurve[1] = VECTOR( 10.0f, 10.0f, 0.0f );
	sgvCameraCurve[2] = VECTOR(  0.0f, 10.0f, 0.0f );
	sgvCameraCurve[3] = VECTOR(  0.0f, 20.0f, 0.0f );
	sgvCameraCurve[4] = VECTOR( 10.0f, 20.0f, 0.0f );
	sgvCameraCurve[5] = VECTOR( 15.0f, 15.0f, 0.0f );
	sgnNumCameraKnots = 6;*/

	SplineSetKnotTable( &sgnCameraKnots[0], sgnNumCameraKnots );

	//sgFollowPathDeltaTime = (TIME)FLOOR( ( TEST_TIME_DELTA * (float)MSECS_PER_SEC ) + 0.5f );
	//sgFollowPathMoveStartTime = AppCommonGetCurTime();

	DaveDebugWindowSetDrawFn( sDrawTestCurve );
	DaveDebugWindowShow();
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCameraFollowPath()
{
	ASSERT( sgnNumCameraKnots >= 2 && sgnNumCameraKnots <= MAX_SPLINE_KNOTS );

	float fTimePercent = (float)( ( (double)AppCommonGetCurTime() - (double)sgFollowPathMoveStartTime ) / (double)sgFollowPathDeltaTime );

	if ( fTimePercent > 1.0f )
	{
		fTimePercent = 1.0f;
	}

	SplineGetPosition( &sgvCameraCurve[0], sgnNumCameraKnots, &sgnCameraKnots[0], &sgvCameraPathPosition, fTimePercent );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct RTS_CAMERA_DATA 
{
	float		fRayLength;
	float		fDistance;
};

static void sRTSCameraCollideCallback(
	struct ROOM_INDEX * pRoomIndex,
	struct ROOM * pRoom,
	VECTOR vLocation,
	int nTriangleNumber,
	float fHitFraction,
	void * userdata)
{
	REF(pRoom);
	REF(vLocation);
	if(!pRoomIndex)
		return;
	if(nTriangleNumber < 0)
		return;
	if(!userdata)
		return;
	
	// ignore some stuff (mainly props)
	if ( pRoomIndex->bRTSCameraIgnore )
		return;

	RTS_CAMERA_DATA * pData = (RTS_CAMERA_DATA *)userdata;

	float fHitDistance = pData->fRayLength * fHitFraction;
	if ( fHitDistance < pData->fDistance )
		pData->fDistance = fHitDistance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FP_HAMMER_GET_CAMERA_POSITION gfpHammerGetCameraPosition = NULL;

void c_CameraSetHammerUpdatePositionFunction(
	FP_HAMMER_GET_CAMERA_POSITION fpHammerGetCameraPosition)
{
	gfpHammerGetCameraPosition = fpHammerGetCameraPosition;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CROUCH_HEIGHT		1.0f
#define CROUCH_DELTA_ADD	( ( PLAYER_HEIGHT - CROUCH_HEIGHT ) / 3.0f )		// distance divided by number of game ticks (1/2 second = 10)

BOOL PlayerIsCrouching( UNIT * player );

void c_CameraUpdate(
	GAME * pGame,
	int sim_frames)
{
	if (!pGame)
	{
		return;
	}
#if !ISVERSION(SERVER_VERSION)
	ASSERT(IS_CLIENT(pGame));
	float fDeltaTime =  AppCommonGetElapsedTime();
	UNIT * camera_unit = GameGetCameraUnit(pGame);
	if ( !camera_unit && !AppGetDetachedCamera() )
	{
		GameSetCameraUnit(pGame,INVALID_ID);
		
		UNIT * pControlUnit = GameGetControlUnit( pGame );
		GameSetCameraUnit( pGame, UnitGetId( pControlUnit ) );
		camera_unit = pControlUnit;
	}

	static int sgnCameraModelPrev = INVALID_ID;

	int nCameraModelId = sgnDefaultModel;
	if ( nCameraModelId == INVALID_ID )
	{
		UNIT* unit = GameGetCameraUnit(pGame);
		if ( unit )
			nCameraModelId = c_UnitGetModelId( unit );
	}

	if ( sgnCameraModelPrev != INVALID_ID && nCameraModelId != sgnCameraModelPrev )
	{
		e_ModelSetFlagbit(sgnCameraModelPrev, MODEL_FLAGBIT_FIRST_PERSON_PROJ, FALSE);
	}

	sgnCameraModelPrev = nCameraModelId;

	if ( ! sgbHasUpdatedOnce )
	{
		sgtCameraInfoFirst.eProjType = PROJ_PERSPECTIVE;
		sgtCameraInfoView.eProjType  = PROJ_PERSPECTIVE;
	}

	float fFlyDistance = 0.0f;
	if (!AppIsHammer())
	{
		if (!AppGetCameraPosition(&sgtCameraInfoFirst.vPosition, &sgtCameraInfoFirst.fAngle, &sgtCameraInfoFirst.fPitch))
		{
			if( AppIsTugboat() )
			{
				
			}
			else
			{
				return;
			}
		}
		float fVertFOV = sGetVerticalFOV();
		sgtCameraInfoFirst.fVerticalFOV = fVertFOV;
 		sgtCameraInfoView.fVerticalFOV = fVertFOV;
	}
	else
	{
		if (gfpHammerGetCameraPosition)
		{
			gfpHammerGetCameraPosition(sgtCameraInfoFirst.vPosition, sgtCameraInfoFirst.fAngle, sgtCameraInfoFirst.fPitch, fFlyDistance, sgtCameraInfoFirst.fVerticalFOV);
			if ( sgtCameraInfoFirst.fVerticalFOV < 1e-5f )
				sgtCameraInfoFirst.fVerticalFOV = sGetVerticalFOV();
		}
	}

	VECTOR vOriginalPosition = sgtCameraInfoView.vPosition;
	
	sgtCameraInfoView.vPosition = sgtCameraInfoFirst.vPosition;
	if(!camera_unit || !UnitHasState(pGame, camera_unit, STATE_CAMERA_LERP))
	{
		sgtCameraInfoView.fAngle = sgtCameraInfoFirst.fAngle;
		sgtCameraInfoView.fPitch = sgtCameraInfoFirst.fPitch;
	}

	// get position from bone in model
	if ( sgnCameraBone == INVALID_ID && nCameraModelId != INVALID_ID )
	{
		int nAppearanceDefId = e_ModelGetAppearanceDefinition( nCameraModelId );
		//ASSERT( nAppearanceDefId != INVALID_ID );
		if (nAppearanceDefId != INVALID_ID)
		{
			sgnCameraBone = c_AppearanceDefinitionGetBoneNumber( nAppearanceDefId, "CameraBone" );
		}
		//ASSERTX_RETURN( sgnCameraBone != INVALID_ID, "Camera Bone is missing" );
	}

	if ( c_CameraGetViewType() == VIEW_1STPERSON )
	{
		// we have to update the crouch delta before we apply it
		UNIT * player = GameGetCameraUnit( pGame );

		if ( PlayerIsCrouching( player ) )
		{
			if ( sgfCameraCrouchDelta < CROUCH_HEIGHT )
				sgfCameraCrouchDelta += CROUCH_DELTA_ADD;
			if ( sgfCameraCrouchDelta > CROUCH_HEIGHT )
				sgfCameraCrouchDelta = CROUCH_HEIGHT;
		}
		else
		{
			if ( sgfCameraCrouchDelta > 0.0f )
				sgfCameraCrouchDelta -= CROUCH_DELTA_ADD;
			if ( sgfCameraCrouchDelta < 0.0f )
				sgfCameraCrouchDelta = 0.0f;
		}
	}

	if ( sgnCameraBone != INVALID_ID )
	{
		MATRIX mBone;
		BOOL fRet = c_AppearanceGetBoneMatrix ( nCameraModelId, &mBone, sgnCameraBone );
		ASSERT( fRet );
		MATRIX mRotation;
		MatrixTransform( &mRotation, NULL, sgtCameraInfoFirst.fAngle + PI / 2.0f); // all animated models are off by 90 degrees
		MatrixMultiply( &mBone, &mBone, &mRotation );
		VECTOR vBoneOffest( 0.0f );
		MatrixMultiply( &vBoneOffest, &vBoneOffest, &mBone );
		sgtCameraInfoFirst.vPosition += vBoneOffest;
		//this should be zero for Tugboat, so no need to change anything
		sgtCameraInfoFirst.vPosition.fZ += c_CameraGetZDelta();

		MATRIX mLookAtTransform;
		MatrixTransform( &mLookAtTransform, &sgtCameraInfoFirst.vPosition, sgtCameraInfoFirst.fAngle, sgtCameraInfoFirst.fPitch );
		MatrixMultiply( &sgtCameraInfoFirst.vLookAt, &cgvXAxis, &mLookAtTransform );
		sgtCameraInfoFirst.vAimAt = sgtCameraInfoFirst.vLookAt;
	}

	VECTOR vOriginalLookAt = sgtCameraInfoView.vLookAt;

	if ( c_CameraGetViewType() == VIEW_1STPERSON )
	{
		sgtCameraInfoView = sgtCameraInfoFirst;
		if (c_CameraIsFirstPersonFOVEnabled(pGame))
		{
			if (!AppGetDetachedCamera())
			{
				e_ModelSetFlagbit(nCameraModelId, MODEL_FLAGBIT_FIRST_PERSON_PROJ, TRUE);
			}
		}
	}
	else if ( sgbDemoLevelPath )
	{
		VECTOR vFace;
		DemoLevelFollowPath( &sgvCameraPathPosition, &vFace );

		//VECTOR vFirst = sgtCameraInfoFirst.vPosition;

		// look out infront of the player just a bit
		//sgtCameraInfoView.vLookAt.fX = vFirst.fX + ( cosf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
		//sgtCameraInfoView.vLookAt.fY = vFirst.fY + ( sinf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
		//sgtCameraInfoView.vLookAt.fZ = vFirst.fZ + PLAYER_HEIGHT;
		sgtCameraInfoView.vLookAt = sgvCameraPathPosition + vFace;

		sgtCameraInfoView.vPosition = sgvCameraPathPosition;
	}
	else if ( sgbFollowPath )
	{
		sCameraFollowPath();

		VECTOR vFirst = sgtCameraInfoFirst.vPosition;

		// look out infront of the player just a bit
		sgtCameraInfoView.vLookAt.fX = vFirst.fX + ( cosf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
		sgtCameraInfoView.vLookAt.fY = vFirst.fY + ( sinf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
		sgtCameraInfoView.vLookAt.fZ = vFirst.fZ + PLAYER_HEIGHT;

		sgtCameraInfoView.vPosition = sgvCameraPathPosition;
	}
	else if ( ! AppIsHammer() && c_CameraGetViewType() == VIEW_3RDPERSON )
	{
		//Tugboat did GameGetControlUnit, but this appears to be an updated version of that function
		UNIT * player = GameGetCameraUnit( pGame );
		if(player == NULL)
		{
			return;
		}

		LEVEL * pLevel = UnitGetLevel( player );
		if (! pLevel)
			return;

		VECTOR vDestPosition;
		VECTOR vFirst = sgtCameraInfoFirst.vPosition;
		if( AppIsHellgate() ) //camera control for hellgate
		{
			float fThirdPersonCameraHeight = sGetThirdPersonCameraHeight( player );
			if ( !UnitIsOnGround( player ) )
			{
				VECTOR vRayDir = VECTOR( 0.0f, 0.0f, 1.0f );
				float fRayLength = fThirdPersonCameraHeight - PLAYER_HEIGHT;
				vFirst.fZ += PLAYER_HEIGHT;
				float fDistance = LevelLineCollideLen( pGame, pLevel, vFirst, vRayDir, fRayLength );
				vFirst.fZ += fDistance;
				if ( fDistance < fRayLength )
				{
					vFirst.fZ -= CAMERA_COLLIDE_BUMP;
				}
			}
			else
			{
				vFirst.fZ += fThirdPersonCameraHeight;
			}

			// look out infront of the player just a bit
			sgtCameraInfoView.vLookAt.fX = vFirst.fX + ( cosf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
			sgtCameraInfoView.vLookAt.fY = vFirst.fY + ( sinf( sgtCameraInfoView.fAngle ) * THIRD_PERSON_LOOKAT_DISTANCE );
			sgtCameraInfoView.vLookAt.fZ = vFirst.fZ;

#define THIRD_PERSON_DISTANCE_LERP 0.85f
			{
				float fZoomLerp = THIRD_PERSON_DISTANCE_LERP;
				sgfThirdDistanceCurr = (fZoomLerp * sgfThirdDistanceCurr) + (1.0f - fZoomLerp) * sgfThirdDistance;
			}
			vDestPosition.fX = vFirst.fX + ( cosf( sgtCameraInfoView.fAngle + PI ) * sgfThirdDistanceCurr );
			vDestPosition.fY = vFirst.fY + ( sinf( sgtCameraInfoView.fAngle + PI ) * sgfThirdDistanceCurr );

			VECTOR vPlayerPosition = UnitGetPosition( player );
			vDestPosition.fZ = vFirst.fZ - ( sinf( sgtCameraInfoView.fPitch ) * sgfThirdDistanceCurr );

			// send a ray from the player's feet up - to make sure that we aren't having trouble with the head being in the ceiling
			{
				VECTOR vRayCastPosition = vPlayerPosition;
				vRayCastPosition.z += CAMERA_HEAD_FEET_OFFSET;
				float fDistanceToHead = sgtCameraInfoView.vLookAt.z - vRayCastPosition.z;
				VECTOR vRayDir = VECTOR( 0.0f, 0.0f, 1.0f );
				fDistanceToHead = LevelLineCollideLen( pGame, pLevel, vRayCastPosition, vRayDir, fDistanceToHead );
				sgtCameraInfoView.vLookAt.z = vRayCastPosition.z + fDistanceToHead;
			}

			// send a ray from the player's head to the look at position to prevent the lookat to camera ray from going through walls
			VECTOR vRayDir = sgtCameraInfoView.vLookAt - vFirst;
			float fRayLength = VectorLength( vRayDir );
			vRayDir /= fRayLength;
			fRayLength += CAMERA_MOVE_IN_FACTOR;
			float fDistance = LevelSphereCollideLen( pGame, pLevel, vFirst, vRayDir, CAMERA_RADIUS, fRayLength, RCSFLAGS_ALL_AND_CAMERA );
			if ( fDistance < 0.0f )
				fDistance = 0.0f;
			vRayDir *= fDistance;
			sgtCameraInfoView.vLookAt = vFirst + vRayDir;

			if ( !UnitIsOnGround( player ) )
			{
				// send 2 rays, one in the Z plane, then one in the x,y
				// from the look at position to the camera to see if it collides
				VECTOR vTestPosition = sgtCameraInfoView.vLookAt;
				vRayDir = vDestPosition - vTestPosition;
				vRayDir.fZ = 0.0f;
				VectorNormalize( vRayDir );
				fRayLength = sqrtf( sgfOldThirdPersonXYDistanceSquared );
				VECTOR vNormal;
				fDistance = LevelSphereCollideLen( pGame, pLevel, vTestPosition, vRayDir, CAMERA_RADIUS, fRayLength, RCSFLAGS_ALL_AND_CAMERA );
				if ( fDistance < 0.1f )
					fDistance = 0.1f;
				vRayDir *= fDistance;
				vTestPosition = vTestPosition + vRayDir;

				// 2nd ray
				vRayDir.x = vRayDir.y = 0.0f;
				vRayDir.z = vDestPosition.z - sgtCameraInfoView.vLookAt.z;
				fRayLength = VectorLength( vRayDir );
				if ( fRayLength )
				{
					vRayDir /= fRayLength;
					fDistance = LevelSphereCollideLen( pGame, pLevel, vTestPosition, vRayDir, CAMERA_RADIUS, fRayLength, RCSFLAGS_ALL_AND_CAMERA );
					if ( fDistance < 0.0f )
						fDistance = 0.0f;
					vRayDir *= fDistance;
					vDestPosition = vTestPosition + vRayDir;
				}
				else
				{
					vDestPosition = vTestPosition;
				}
			}
			else
			{
				// send a ray from the look at position to the camera to see if it collides
				vRayDir = vDestPosition - sgtCameraInfoView.vLookAt;
				fRayLength = VectorLength( vRayDir );
				vRayDir /= fRayLength;
				fDistance = LevelSphereCollideLen( pGame, pLevel, sgtCameraInfoView.vLookAt, vRayDir, CAMERA_RADIUS, fRayLength, RCSFLAGS_ALL_AND_CAMERA );
				if ( fDistance < 0.1f )
					fDistance = 0.1f;
				vRayDir *= fDistance;
				vDestPosition = sgtCameraInfoView.vLookAt + vRayDir;
				sgfOldThirdPersonXYDistanceSquared = VectorDistanceSquaredXY( sgtCameraInfoView.vLookAt, vDestPosition );
				sgfThirdDistanceCurr = MIN( fDistance, sgfThirdDistanceCurr );
			}
		}
		else if ( ! AppIsHammer() ) //camera control for tugboat
		{

			if( InputGetAdvancedCamera() )
			{
				if( sgfFollowAngleCurrent != sgfFollowAngle )
				{

					float Delta = sgfFollowAngleCurrent - sgfFollowAngle;
					float Half = DegreesToRadians( 180.0f );
					while( Delta > Half)
					{
						Delta -= Half * 2.0f;
						sgfFollowAngleCurrent -= Half * 2.0f;
					}
					while( Delta < -Half)
					{
						Delta += Half * 2.0f;
						sgfFollowAngleCurrent += Half * 2.0f;
					}
					Delta *= pow( .3f, fDeltaTime ) ;
					if( fabs( Delta ) < .1f )
					{
						Delta = .1f * ( Delta < 0 ) ? -1.0f : 1.0f;
					}
					if( abs( Delta ) > abs( sgfFollowAngleCurrent - sgfFollowAngle ) )
					{
						Delta = sgfFollowAngleCurrent - sgfFollowAngle;
					}
					sgfFollowAngleCurrent = sgfFollowAngle + Delta;
				}

				VECTOR vFirst = sgtCameraInfoFirst.vPosition;
				sgtCameraInfoView.vLookAt = vFirst;
				vDestPosition = vFirst;
				sgtCameraInfoView.vLookAt.fZ += UnitGetData( player )->fVanityHeight;
				vDestPosition.fZ += UnitGetData( player )->fVanityHeight;
				VECTOR vSavedDestPosition = vDestPosition;

				//VECTOR Dolly( 1.6f, -1.6f, 2.5f );

				VECTOR Dolly( -2.26f, 0, 0 );

				LEVEL * pLevel = UnitGetLevel( player );
				if (! pLevel)
					return;

				float fMinHeight = UnitGetPosition( player ).fZ + ( pLevel->m_pLevelDefinition->bContoured ? 1.0f : 9.0f );



				VECTOR Tilt( 2.5f, 2.26f, 0 );

				float fFollowPitch = sgfFollowPitch;
				if( !pLevel->m_pLevelDefinition->bContoured )
				{
					if( fFollowPitch >= .65f )
					{
						fFollowPitch = .65f;
					}
				}
				VectorZRotation( Tilt, InputGetLockedPitch() ? 0.0f : sgfFollowPitch );

				VectorZRotation( Dolly, sgfFollowAngleCurrent + ( InputGetLockedPitch() ? DEG_TO_RAD( 135.0f ) : 0 ) );
				Dolly.fZ += Tilt.fX;

				float fNearCap = ( InputGetAdvancedCamera() && !InputGetLockedPitch() ) ? CAMERA_FOLLOW_NEAR_CAP : CAMERA_DOLLY_NEAR_CAP;

				float fDollyPercentage = MAX( 0.0f, sgCameraDolly - fNearCap ) / CAMERA_DOLLY_FAR_CAP;

				VectorNormalize( Dolly, Dolly );

				VECTOR vLastDolly = Dolly;
				Dolly *= CAMERA_DOLLY_FAR_CAP * pLevel->m_pLevelDefinition->flCameraDollyScale;
				if( sgLastFollowDolly < fNearCap && sgfLastImpactTime <= 1 )
				{
					sgLastFollowDolly = LERP( sgLastFollowDolly, fNearCap, .1f );
				}
				vLastDolly *= sgLastFollowDolly;

				VECTOR vXY = Dolly;
				vXY.fZ = 0;
				VECTOR vCameraAngle = Dolly;
				VectorNormalize( vCameraAngle );
				float fTilt = vCameraAngle.fZ - .25f;
				fTilt *= 2.0f;
				fTilt = MAX( 0.0f, fTilt );
				fTilt = MIN( 1.0f, fTilt );
				
				fTilt = ( fNearCap + ( CAMERA_DOLLY_FAR_CAP - fNearCap ) * fTilt ) / ( CAMERA_DOLLY_FAR_CAP );

				/*if( VectorLength( vXY ) > MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale )
				{
					VectorNormalize( vXY );
					vXY *= MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale ;
					vXY.fZ = Dolly.fZ;
					Dolly = vXY;
				}

				vXY = vLastDolly;
				vXY.fZ = 0;
				if( VectorLength( vXY ) > MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale )
				{
					VectorNormalize( vXY );
					vXY *= MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale ;
					vXY.fZ = vLastDolly.fZ;
					vLastDolly = vXY;
				}*/

				VECTOR vStartPosition = vDestPosition;
				VECTOR vOriginalPosition = vDestPosition + vLastDolly;
				float fZ = Dolly.fZ;
				Dolly.fZ = 0;
				VECTOR vTargetDestPosition = vDestPosition;				
				vTargetDestPosition += Dolly * ( fDollyPercentage * fTilt );
				vTargetDestPosition += Dolly * ( fNearCap / CAMERA_DOLLY_FAR_CAP );
				vTargetDestPosition.fZ += fZ;
				if( sgfLastImpactTime <= 1 )
				{
					vDestPosition  = vTargetDestPosition;
				}
				else
				{
					vDestPosition += vLastDolly;

				}

				if( vDestPosition.fZ < fMinHeight )
				{

					VECTOR vDelta = vDestPosition - vStartPosition;
					float Hypotenuse = VectorLength( vDelta );					

					float a2 = fabs( fMinHeight - vStartPosition.fZ );
					float b2 = ( Hypotenuse * Hypotenuse ) - ( a2 * a2 );
					if( b2 < .001f )
					{
						b2 = .001f;
					}
					b2 = sqrt( b2 );
					vDelta.fZ = 0;
					VectorNormalize( vDelta );
					vDelta *= b2;
					vDestPosition = vStartPosition + vDelta;
					vDestPosition.fZ = fMinHeight;
					
				}
				if( vOriginalPosition.fZ < fMinHeight )
				{
					VECTOR vDelta = vOriginalPosition - vStartPosition;
					float Hypotenuse = VectorLength( vDelta );					

					float a2 = fabs( fMinHeight - vStartPosition.fZ );
					float b2 = ( Hypotenuse * Hypotenuse ) - ( a2 * a2 );
					if( b2 < .001f )
					{
						b2 = .001f;
					}
					b2 = sqrt( b2 );
					vDelta.fZ = 0;
					VectorNormalize( vDelta );
					vDelta *= b2;
					vOriginalPosition = vStartPosition + vDelta;
					vOriginalPosition.fZ = fMinHeight;

				}

				int nHits = 0;
				if( pLevel->m_pLevelDefinition->bContoured )
				{
					// send a ray from the lookat to the position to make sure it doesnt go underground
					VECTOR vRayStart = sgtCameraInfoView.vLookAt;
					vRayStart.fZ = vTargetDestPosition.fZ;
					vRayStart.fZ -= 2.5f;
					vRayStart.fZ = MAX( vRayStart.fZ, sgtCameraInfoView.vLookAt.fZ );

					VECTOR vRayDir = vTargetDestPosition - vRayStart;
					vRayDir.fZ = 0;
					float fRayLength = VectorLength( vRayDir );
					VectorNormalize( vRayDir );
					VECTOR vNormal;

					vRayStart -= vRayDir;
					fRayLength += 1.0f;
					VECTOR vPerpendicular = vRayDir;
					vPerpendicular.fZ = 0;
					VectorNormalize( vPerpendicular, vPerpendicular );
					VectorZRotation( vPerpendicular, DegreesToRadians( 90.0f ) );

					VECTOR vSecondary = vRayStart + vPerpendicular * 2.0f;



					SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
					ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);

					LevelSortFaces(pGame, pLevel, SortedFaces, vSecondary, vRayDir, fRayLength );

					if ( LevelLineCollideLen( pGame, pLevel, SortedFaces, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
					{
						nHits++;
					}
					vSecondary = vRayStart - vPerpendicular * 2.0f;
					if ( LevelLineCollideLen( pGame, pLevel, SortedFaces, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
					{
						nHits++;
					}
					/*vSecondary = vRayStart + VECTOR( 0, 0, .5f );
					if ( LevelLineCollideLen( pGame, pLevel, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
					{
					nHits++;
					}*/
					vSecondary = vRayStart - VECTOR( 0, 0, .5f );
					if ( LevelLineCollideLen( pGame, pLevel, SortedFaces, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
					{
						nHits++;
					}
					float fDistance = LevelLineCollideLen( pGame, pLevel, SortedFaces, vRayStart, vRayDir, fRayLength, &vNormal );
					if ( fDistance < fRayLength )
					{
						nHits++;
						if( nHits >= 3 )
						{
							sgfLastImpactTime = 2.0f;
							vDestPosition = vRayStart + ( vRayDir * ( fDistance - .01f ) ) ;
						}
					}

					SortedFaces.Destroy();
				}

	

				float CAMERA_LERP = .9f;
				if( nHits >= 3 )
				{
					CAMERA_LERP *= .8f;
				}
				if( sgfLastImpactTime < 1 )
				{
					CAMERA_LERP += sgfLastImpactTime * .1f;
				}
				sgtCameraInfoView.vPosition = VectorLerp(vOriginalPosition , vDestPosition, CAMERA_LERP);

				if( sgfLastImpactTime > 0 )
				{
					sgfLastImpactTime -= fDeltaTime;
					sgfLastImpactTime = MAX( 0.0f, sgfLastImpactTime );
				}
				VECTOR vNewDolly = sgtCameraInfoView.vPosition - vSavedDestPosition;
				float fLastFollowDolly = VectorLength( vNewDolly );
				if( sgfLastImpactTime < 1 )
				{
					vLastDolly = vNewDolly;
					sgLastFollowDolly =fLastFollowDolly;
					vDestPosition = sgtCameraInfoView.vPosition;

				}
				else if( fLastFollowDolly < sgLastFollowDolly )
				{
					vLastDolly = vNewDolly;
					sgLastFollowDolly =fLastFollowDolly;
					vDestPosition = sgtCameraInfoView.vPosition;
				}
				else
				{
					sgtCameraInfoView.vPosition = vOriginalPosition;
					vDestPosition = sgtCameraInfoView.vPosition;
				}
	
			}
			else
			{
				sgtCameraInfoView.vLookAt = vFirst;
				vDestPosition = vFirst;
				sgtCameraInfoView.vLookAt.fZ += UnitGetData( player )->fVanityHeight;
				vDestPosition.fZ += UnitGetData( player )->fVanityHeight;

				VECTOR Dolly( 1.6f, -1.6f, 2.5f );
				
				LEVEL * pLevel = UnitGetLevel( player );
				if ( pLevel )
				{
					float fCamOrientation = DegreesToRadians( pLevel->m_pLevelDefinition->fFixedOrientation );
					VectorZRotation( Dolly, fCamOrientation );
				}

				VectorNormalize( Dolly, Dolly );

				Dolly *= sgCameraDolly * pLevel->m_pLevelDefinition->flCameraDollyScale;

				vDestPosition += Dolly;

			}
			

		}
		if ( !sgb3rdPersonInited )
		{
			sgvLastPosition = vDestPosition;
			sgb3rdPersonInited = TRUE;
		}

		// no camera lag debug
		sgtCameraInfoView.vPosition = vDestPosition;

	}
	else if ( c_CameraGetViewType() == VIEW_CHARACTER_SCREEN )
	{
		UNIT * pUnit = GameGetCameraUnit( pGame ); //tugboat called old function
		int nModelId = pUnit ? c_UnitGetModelId( pUnit ) : INVALID_ID;
		int nAppearanceDef = c_AppearanceGetDefinition( nModelId );
		if ( ! sgpCharacterCamera && nAppearanceDef != INVALID_ID )
		{
			sgpCharacterCamera = AppearanceDefinitionGetInventoryView( nAppearanceDef, CAMERA_CHARACTER_SCREEN );
		}



	
		if( AppIsTugboat() )
		{

			
			if( pUnit )
			{
				float fRotation = -20.0f;
				VECTOR vLookOffset( 0, 0, 1.25f );
				VECTOR vSwing( 7.25f, 0, 1.2f );
				VectorZRotation( vSwing, DegreesToRadians( fRotation ) );

				sgtCameraInfoView.fVerticalFOV	  = .5f;

				//const UNIT_DATA* pUnitData = UnitGetData( pUnit );
				//VECTOR vPosition = VECTOR( pUnitData->fCameraEye[0], pUnitData->fCameraEye[1],  pUnitData->fCameraEye[2] );
				//vPosition += VECTOR( -120.033f, -93.4927f, 7.39875f );
				//VECTOR vLookAt = VECTOR( pUnitData->fCameraTarget[0], pUnitData->fCameraTarget[1],  pUnitData->fCameraTarget[2] );
				//vLookAt += VECTOR( -120.033f, -93.4927f, 7.39875f );
				VECTOR vLookAt = VECTOR(  -116.9987, -221.0f, 7.167f );


				VECTOR vPosition = vLookAt + vSwing;
					vLookAt += vLookOffset;
				const float CAMERA_LERP = .8f;
				sgtCameraInfoView.vLookAt = VectorLerp(sgtCameraInfoView.vLookAt, vLookAt, CAMERA_LERP);
				sgtCameraInfoView.vPosition = VectorLerp(vOriginalPosition , vPosition, CAMERA_LERP);

			}
			else
			{
				float fRotation = -20.0f;
				VECTOR vLookOffset( 0, 0, 1.25f );
				VECTOR vSwing( 7.25f, 0, 1.2f );
				VectorZRotation( vSwing, DegreesToRadians( fRotation ) );

				sgtCameraInfoView.fVerticalFOV	  = .5f;

				//sgtCameraInfoView.vPosition = VECTOR( 1.846f, -3.148f,  1.342f );
				//sgtCameraInfoView.vPosition += VECTOR(  -126.75f, -221.0f, 7.7623f );

				sgtCameraInfoView.vLookAt = VECTOR(  -116.9987, -221.0f, 7.167f );
				
				sgtCameraInfoView.vPosition = sgtCameraInfoView.vLookAt + vSwing;

				sgtCameraInfoView.vLookAt += vLookOffset;

			}

		}
		else
		{
			sgtCameraInfoView.fVerticalFOV	  = 1.0472f;
			float fDistance = MAX_CHARACTER_SCREEN_DISTANCE;
			float z;
			if ( pUnit )
			{ // this was data-driven, but it takes too long to load the data... it needs to be ready faster
				fDistance = sgfCurrentCharScreenDistance;
				float fRatio = sgfCurrentCharScreenDistance / MAX_CHARACTER_SCREEN_DISTANCE;
				VECTOR vDelta = VECTOR( 0.5f, 0.6991f, 1.0143f );
				vDelta.x = vDelta.x * fRatio;
				vDelta.y = vDelta.y * fRatio;
				float fMinMaxRatio = ( MAX_CHARACTER_SCREEN_DISTANCE - sgfCurrentCharScreenDistance ) / ( MAX_CHARACTER_SCREEN_DISTANCE - MIN_CHARACTER_SCREEN_DISTANCE );
				vDelta.z = vDelta.z + ( 0.7f * fMinMaxRatio );
				sgtCameraInfoView.vLookAt = pUnit->vPosition + vDelta;
				z = pUnit->vPosition.z + 1.0143f + ( 0.35f * fMinMaxRatio );

				if ( sgfCurrentCharScreenDistance < sgfCharacterScreenDistance )
				{
					sgfCurrentCharScreenDistance += 0.05f;
					if ( sgfCurrentCharScreenDistance > MAX_CHARACTER_SCREEN_DISTANCE )
						sgfCurrentCharScreenDistance = MAX_CHARACTER_SCREEN_DISTANCE;
				}
				if ( sgfCurrentCharScreenDistance > sgfCharacterScreenDistance )
				{
					sgfCurrentCharScreenDistance -= 0.05f;
					if ( sgfCurrentCharScreenDistance < MIN_CHARACTER_SCREEN_DISTANCE )
						sgfCurrentCharScreenDistance = MIN_CHARACTER_SCREEN_DISTANCE;
				}
			}
			else
			{
				sgtCameraInfoView.vLookAt = VECTOR( -0.31260356f, -3.4472833f, 1.0143f );
				z = sgtCameraInfoView.vLookAt.z;
			}
			sgtCameraInfoView.vPosition.fX = sgtCameraInfoView.vLookAt.fX + ( cosf( -1.39f ) * fDistance );
			sgtCameraInfoView.vPosition.fY = sgtCameraInfoView.vLookAt.fY + ( sinf( -1.39f ) * fDistance );
			sgtCameraInfoView.vPosition.fZ = z + ( sinf( 0.205f ) * fDistance );
		}
		sgtCameraInfoFirst.fVerticalFOV	  = sgtCameraInfoView.fVerticalFOV;
		
		//if ( sgpCharacterCamera && pUnit )
		//{
			//sgtCameraInfoView.vLookAt = UnitGetPosition( pUnit ) + sgpCharacterCamera->vCamFocus;
			//sgtCameraInfoView.fFOV	  = sgpCharacterCamera->fCamFOV;
			//sgtCameraInfoFirst.fFOV	  = sgpCharacterCamera->fCamFOV;
			//sgtCameraInfoView.vPosition.fX = sgtCameraInfoView.vLookAt.fX + ( cosf( sgpCharacterCamera->fCamRotation ) * sgpCharacterCamera->fCamDistance );
			//sgtCameraInfoView.vPosition.fY = sgtCameraInfoView.vLookAt.fY + ( sinf( sgpCharacterCamera->fCamRotation ) * sgpCharacterCamera->fCamDistance );
			//sgtCameraInfoView.vPosition.fZ = sgtCameraInfoView.vLookAt.fZ + ( sinf( -sgpCharacterCamera->fCamPitch )    * sgpCharacterCamera->fCamDistance );
		//}
	}
	else if ( c_CameraGetViewType() == VIEW_VANITY )
	{
		UNIT * player = GameGetCameraUnit( pGame );
		ASSERT_RETURN( player );

		BOOL bCharacterScreen = c_CameraGetViewType() == VIEW_CHARACTER_SCREEN;

		sgtCameraInfoView.vLookAt = sgtCameraInfoView.vPosition;
		float fHeightAdjustment = ((AppIsHellgate())?0.6f:0.7f) * PLAYER_HEIGHT;
		int nModel = c_UnitGetModelId( player );
		if ( nModel != INVALID_ID )
		{
			fHeightAdjustment *= c_AppearanceGetHeightFactor( nModel );
		}
		sgtCameraInfoView.vLookAt.fZ += fHeightAdjustment;
		if( AppIsTugboat()  )
		{
			if( !bCharacterScreen )
			{

				/*
				LEVEL * pLevel = UnitGetLevel( player );
				if (! pLevel)
					return;
				VECTOR vDestPosition;
				VECTOR vFirst = sgtCameraInfoFirst.vPosition;

				sgtCameraInfoView.vLookAt = vFirst;

				vDestPosition = vFirst;

				VECTOR Tilt( 0, sgfVanityDistance, 0 );
				VectorZRotation( Tilt, sgfVanityPitch );

				VECTOR Dolly = player->vFaceDirectionDrawn;				

				Dolly *= sgfVanityDistance;
				sgtCameraInfoView.vLookAt.fZ += UnitGetData( player )->fVanityHeight;
				VectorZRotation( Dolly, sgfVanityAngle );
				Dolly.fZ += Tilt.fX;

				vDestPosition += Dolly;

				if ( !sgb3rdPersonInited )
				{
					sgvLastPosition = vDestPosition;
					sgb3rdPersonInited = TRUE;
				}

				// no camera lag debug
				sgtCameraInfoView.vPosition = vDestPosition;

				// send a ray from the lookat to the position to make sure it doesnt go underground
				VECTOR vRayDir = sgtCameraInfoView.vPosition - sgtCameraInfoView.vLookAt;
				float fRayLength = VectorLength( vRayDir );
				vRayDir /= fRayLength;
				VECTOR vNormal;
				float fDistance = LevelLineCollideLen( pGame, pLevel, sgtCameraInfoView.vLookAt, vRayDir, fRayLength, &vNormal );
				if ( fDistance < fRayLength )
				{
					sgtCameraInfoView.vPosition = sgtCameraInfoView.vLookAt + ( vRayDir * ( fDistance - 0.01f ) );
					sgtCameraInfoView.vPosition += vNormal * CAMERA_COLLIDE_BUMP;
				}*/


				VECTOR vDestPosition;
				VECTOR vFirst = sgtCameraInfoFirst.vPosition;
				sgtCameraInfoView.vLookAt = vFirst;
				vDestPosition = vFirst;
				sgtCameraInfoView.vLookAt.fZ += UnitGetData( player )->fVanityHeight;
				vDestPosition.fZ += UnitGetData( player )->fVanityHeight;
				VECTOR vSavedDestPosition = vDestPosition;

				//VECTOR Dolly( 1.6f, -1.6f, 2.5f );

				VECTOR Dolly( -2.26f, 0, 0 );

				LEVEL * pLevel = UnitGetLevel( player );
				if (! pLevel)
					return;

				VECTOR Tilt( 2.5f, 2.26f, 0 );
				VectorZRotation( Tilt, sgfVanityPitch );

				VectorZRotation( Dolly, sgfVanityAngle/* + ( DEG_TO_RAD( 135.0f ) )*/ );
				Dolly.fZ += MAX( -0.08f, Tilt.fX );

				float fDollyPercentage = sgfVanityDistance  / MAX_VANITY_DISTANCE_TUGBOAT;


				VectorNormalize( Dolly, Dolly );
	
				VECTOR vLastDolly = Dolly;
				Dolly *= MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale;
				if( sgLastVanityDolly < MIN_VANITY_DISTANCE_TUGBOAT )
				{
					sgLastVanityDolly = LERP( sgLastVanityDolly, MIN_VANITY_DISTANCE_TUGBOAT, .1f );
				}
				vLastDolly *= sgLastVanityDolly;

				VECTOR vXY = Dolly;
				vXY.fZ = 0;

				if( VectorLength( vXY ) > MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale )
				{
					VectorNormalize( vXY );
					vXY *= MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale ;
					vXY.fZ = Dolly.fZ;
					Dolly = vXY;
				}

				vXY = vLastDolly;
				vXY.fZ = 0;
				if( VectorLength( vXY ) > MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale )
				{
					VectorNormalize( vXY );
					vXY *= MAX_VANITY_DISTANCE_TUGBOAT * pLevel->m_pLevelDefinition->flCameraDollyScale ;
					vXY.fZ = vLastDolly.fZ;
					vLastDolly = vXY;
				}

				VECTOR vStartPosition = vDestPosition;
				VECTOR vOriginalPosition = vDestPosition + vLastDolly;

				
				vDestPosition += Dolly * fDollyPercentage;
		
				
				// send a ray from the lookat to the position to make sure it doesnt go underground
				VECTOR vRayStart = sgtCameraInfoView.vLookAt;
				vRayStart.fZ = vDestPosition.fZ;
				VECTOR vRayDir = vDestPosition - vRayStart;
				float fRayLength = VectorLength( vRayDir );
				vRayDir /= fRayLength;
				VECTOR vNormal;
				int nHits = 0;
				vRayStart -= vRayDir;
				fRayLength += 1.0f;

				VECTOR vPerpendicular = vRayDir;
				vPerpendicular.fZ = 0;
				VectorNormalize( vPerpendicular, vPerpendicular );
				VectorZRotation( vPerpendicular, DegreesToRadians( 90.0f ) );

				VECTOR vSecondary = vRayStart + vPerpendicular * 2.0f;

				if ( LevelLineCollideLen( pGame, pLevel, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
				{
					nHits++;
				}
				vSecondary = vRayStart - vPerpendicular * 2.0f;
				if ( LevelLineCollideLen( pGame, pLevel, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
				{
					nHits++;
				}
				/*vSecondary = vRayStart + VECTOR( 0, 0, .5f );
				if ( LevelLineCollideLen( pGame, pLevel, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
				{
					nHits++;
				}*/
				vSecondary = vRayStart - VECTOR( 0, 0, .5f );
				if ( LevelLineCollideLen( pGame, pLevel, vSecondary, vRayDir, fRayLength, &vNormal ) < fRayLength )
				{
					nHits++;
				}
				float fDistance = LevelLineCollideLen( pGame, pLevel, vRayStart, vRayDir, fRayLength, &vNormal );
				if ( fDistance < fRayLength )
				{
					nHits++;
					if( nHits >= 3 )
						vDestPosition = sgtCameraInfoView.vLookAt + ( vRayDir * ( fDistance - 0.01f ) );
				}

				const float CAMERA_LERP = .9f;
				sgtCameraInfoView.vPosition = VectorLerp(vOriginalPosition , vDestPosition, CAMERA_LERP);
				vLastDolly = sgtCameraInfoView.vPosition - vSavedDestPosition;
				sgLastVanityDolly = VectorLength( vLastDolly );


			}
			
		}
		else //hellgate
		{
			REF(bCharacterScreen);
			LEVEL * pLevel = UnitGetLevel( player );
			if (! pLevel)
				return;

			float fCameraAngleZ = PI / 8.0f;
			REF(fCameraAngleZ);

			sgtCameraInfoView.vPosition.fX = sgtCameraInfoView.vLookAt.fX + ( cosf( sgfVanityAngle ) * sgfVanityDistance );
			sgtCameraInfoView.vPosition.fY = sgtCameraInfoView.vLookAt.fY + ( sinf( sgfVanityAngle ) * sgfVanityDistance );
			sgtCameraInfoView.vPosition.fZ = sgtCameraInfoView.vLookAt.fZ + ( sinf( sgfVanityPitch ) * sgfVanityDistance );

			// send a ray from the lookat to the position to make sure it doesnt go underground
			VECTOR vRayDir = sgtCameraInfoView.vPosition - sgtCameraInfoView.vLookAt;
			float fRayLength = VectorLength( vRayDir );
			vRayDir /= fRayLength;
			VECTOR vNormal;
			float fDistance = LevelLineCollideLen( pGame, pLevel, sgtCameraInfoView.vLookAt, vRayDir, fRayLength, &vNormal );
			if ( fDistance < fRayLength )
			{
				sgtCameraInfoView.vPosition = sgtCameraInfoView.vLookAt + ( vRayDir * ( fDistance - 0.01f ) );
				sgtCameraInfoView.vPosition += vNormal * CAMERA_COLLIDE_BUMP;
			}
		}
	}
	else if ( c_CameraGetViewType() == VIEW_FLY )
	{
		sgtCameraInfoView.vLookAt = sgtCameraInfoView.vPosition;
		sgtCameraInfoView.vPosition = VECTOR ( fFlyDistance, 0.0f, 0.0f );
		MATRIX matRotateEye;
		MatrixTransform ( &matRotateEye, NULL, sgtCameraInfoView.fAngle, -sgtCameraInfoView.fPitch );
		MatrixMultiply( &sgtCameraInfoView.vPosition, &sgtCameraInfoView.vPosition, &matRotateEye );
		sgtCameraInfoView.vPosition += sgtCameraInfoView.vLookAt;
	}
	else if ( c_CameraGetViewType() == VIEW_RTS )
	{
#if HELLGATE_ONLY
		UNIT * player = GameGetCameraUnit( pGame );
		ASSERT_RETURN( player );

		LEVEL * pLevel = UnitGetLevel( player );
		if ( !pLevel )
			return;

		if ( VectorIsZero( sgvRTS_TargetPosition ) )
		{
			sgvRTS_TargetPosition = sgvRTS_CurrentPosition;
		}

		// look out infront of the player just a bit
		VECTOR vDelta = sgvRTS_TargetPosition - sgvRTS_CurrentPosition;
		float fLength = VectorLength( vDelta );
		if ( fLength > 0.1f )
		{
			VectorNormalize( vDelta );
			float fRateMod = fLength / 10.0f;
			if ( fRateMod > 1.0f )
				fRateMod = 1.0f;
			if ( fRateMod < 0.05f )
				fRateMod = 0.05f;
			vDelta *= ( RTS_SCROLL_RATE * fRateMod );
			sgvRTS_CurrentPosition += vDelta;
		}

		sgtCameraInfoView.vLookAt = sgvRTS_CurrentPosition;

		sgtCameraInfoView.vPosition.fX = sgvRTS_CurrentPosition.fX;
		float fRad = ( 80.0f * PI ) / 180.0f;
		sgtCameraInfoView.vPosition.fY = sgvRTS_CurrentPosition.fY + ( cosf( fRad ) * ( RTS_DISTANCE * 3.0f ) );
		sgtCameraInfoView.vPosition.fZ = sgvRTS_CurrentPosition.fZ + ( sinf( fRad ) * RTS_DISTANCE );

		// send a ray from the lookat to the position and check if hitting buildings
		VECTOR vStart = sgtCameraInfoView.vLookAt;
		vStart.fZ += RTS_CAMERA_RADIUS + 0.5f;
		VECTOR vRayDir = sgtCameraInfoView.vPosition - vStart;
		float fRayLength = VectorLength( vRayDir );
		vRayDir /= fRayLength;
		float fDistance = LevelSphereCollideLen( pGame, pLevel, vStart, vRayDir, RTS_CAMERA_RADIUS, fRayLength, RCSFLAGS_ALL_AND_RTS_CAMERA );
		if ( fDistance < 0.0f )
			fDistance = 0.0f;
		sgtCameraInfoView.vPosition = vRayDir * fDistance;
		sgtCameraInfoView.vPosition += vStart;
		// okay, now that it is moved in front of the building, try going straight up a bit...
		sgtCameraInfoView.vPosition.z += fRayLength - fDistance;
#endif
	}

	if ( sgbShake && !AppGetDetachedCamera() && !sgbFollowPath )
	{
		// shake it baby!
		VECTOR vTemp;
		//vTemp.fX = RandGetFloat(pGame, 1.0f) * sgvShakeDirection.fX * sgfShakeMagnitude;
		//vTemp.fY = RandGetFloat(pGame, 1.0f) * sgvShakeDirection.fY * sgfShakeMagnitude;
		//vTemp.fZ = RandGetFloat(pGame, 1.0f) * sgvShakeDirection.fZ * sgfShakeMagnitude;
		vTemp.fX = ( RandGetFloat(pGame, -1.0f, 1.0f) * SHAKE_SCALE ) + ( sgvShakeDirection.fX * sgfShakeMagnitude );
		vTemp.fY = ( RandGetFloat(pGame, -1.0f, 1.0f) * SHAKE_SCALE ) + ( sgvShakeDirection.fY * sgfShakeMagnitude );
		vTemp.fZ = ( RandGetFloat(pGame, -2.0f, 2.0f) * SHAKE_SCALE ) + ( sgvShakeDirection.fZ * sgfShakeMagnitude );

		if ( VectorLength( sgvShake - vTemp ) <= ( VectorLength( sgvShake ) * 1.05f ) )
		{
			sgvShake -= vTemp;
			if ( sgfShakeMagnitude > 0.0f )
				sgfShakeMagnitude -= SHAKE_DOWN;
		}

		sgtCameraInfoView.vPosition += sgvShake;
		if (GameGetTimeFloat(pGame) > sgfShakeTime)
			sgbShake = FALSE;
	}

	if(sgbRandomShake && !AppGetDetachedCamera() && !sgbFollowPath )
	{

		float fTotalTime = sgfRandomShakeTime - sgfRandomShakeStartTime;
		float fCurrentTime = GameGetTimeFloat(pGame) - sgfRandomShakeStartTime;
		fCurrentTime = PIN(fCurrentTime, 0.0f, fTotalTime);
		float fClampedMagnitude = sgfRandomShakeMagnitude > 20.0f * SHAKE_SCALE ? 20.0f  * SHAKE_SCALE : sgfRandomShakeMagnitude;
		float fCurrentValue = (1.0f - EASE_INOUT(fCurrentTime / fTotalTime));
		fCurrentValue *= fClampedMagnitude;
		fCurrentValue *= sinf(100.0f * fCurrentTime);
		VECTOR vTemp = sgvRandomShakeDirection * fCurrentValue;

		if ( sgfRandomShakeMagnitude > 0.0f )
			sgfRandomShakeMagnitude -= sgfRandomShakeDegrade * SHAKE_DOWN;

		sgtCameraInfoView.vPosition += vTemp;
		if (GameGetTimeFloat(pGame) > sgfRandomShakeTime)
			sgbRandomShake = FALSE;

	}

	if(camera_unit && UnitHasState(pGame, camera_unit, STATE_CAMERA_LERP))
	{
		const float MIN_LERP_DISTANCE = 0.1f;
		const float MIN_LERP_DISTANCE_SQ = MIN_LERP_DISTANCE * MIN_LERP_DISTANCE;
		if(VectorDistanceSquared(sgtCameraInfoView.vLookAt, vOriginalLookAt) < MIN_LERP_DISTANCE_SQ &&
			VectorDistanceSquared(sgtCameraInfoView.vPosition, vOriginalPosition) < MIN_LERP_DISTANCE_SQ)
		{
			c_StateClear(camera_unit, UnitGetId(camera_unit), STATE_CAMERA_LERP, 0);
		}
		else
		{
			const float CAMERA_LERP = AppIsHellgate() ? 0.89f : .6f;
			sgtCameraInfoView.vLookAt = VectorLerp(vOriginalLookAt, sgtCameraInfoView.vLookAt, CAMERA_LERP);
			sgtCameraInfoView.vPosition = VectorLerp(vOriginalPosition, sgtCameraInfoView.vPosition, CAMERA_LERP);
		}
	}

	sgtCameraInfoFirst.vDirection = sgtCameraInfoFirst.vLookAt - sgtCameraInfoFirst.vPosition;
	VectorNormalize( sgtCameraInfoFirst.vDirection, sgtCameraInfoFirst.vDirection );

	sgtCameraInfoView.vDirection = sgtCameraInfoView.vLookAt - sgtCameraInfoView.vPosition;
	VectorNormalize( sgtCameraInfoView.vDirection, sgtCameraInfoView.vDirection );

	sgtCameraInfoView.fVerticalFOV = sgtCameraInfoFirst.fVerticalFOV;

#define FOCUS_MAX_DISTANCE		30.0f
	switch ( c_CameraGetViewType() )
	{
	case VIEW_3RDPERSON:
	case VIEW_1STPERSON:
		{
			UNIT * pUnit = GameGetCameraUnit( pGame );
			if( pUnit && UnitGetLevel( pUnit ) && !IsUnitDeadOrDying( pUnit ) )
			{
				float fDistance = (AppIsTugboat())?FOCUS_MAX_DISTANCE:LevelLineCollideLen(pGame, UnitGetLevel( pUnit ), sgtCameraInfoView.vPosition, sgtCameraInfoView.vDirection, FOCUS_MAX_DISTANCE );
				VECTOR vDelta = sgtCameraInfoView.vDirection;
				vDelta *= fDistance;
				VECTOR vFocusPoint = sgtCameraInfoView.vPosition + vDelta;
				c_AttachmentSetCameraFocus( vFocusPoint );
			}
		}
		break;
	case VIEW_CHARACTER_SCREEN:
	case VIEW_VANITY:
		//tugboat doesn't use
		if( AppIsTugboat() && c_CameraGetViewType() == VIEW_VANITY )
			break;
		c_AttachmentSetCameraFocus( VECTOR(0.0f) ); // this tells the attachment system to ignore the focus point
		break;
	}

	sgbHasUpdatedOnce = TRUE;

	// mark camera data as fresh
	CameraSetUpdated( TRUE );
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CAMERA_INFO * c_CameraGetInfo(
	BOOL bForceFirstPerson )
{
	if ( ! sgbHasUpdatedOnce )
		return NULL;

	return ( AppIsHellgate() && bForceFirstPerson )? &sgtCameraInfoFirst : &sgtCameraInfoView;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CAMERA_INFO * c_CameraGetInfoCommon()
{
	return c_CameraGetInfo( FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_CameraIsFirstPersonFOVEnabled(
	GAME * pGame )
{
	return c_CameraGetViewType() == VIEW_1STPERSON;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraRestoreViewType()
{
	if ( sgnViewTypeLocked > 0 )
		return;

	c_CameraSetViewType( sgnViewTypePrev, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraDollyIn( void )
{
#if !ISVERSION( SERVER_VERSION )
	sgCameraDolly -= ( InputGetAdvancedCamera() ? CAMERA_DOLLY_INCREMENT_VANITY : CAMERA_DOLLY_INCREMENT );
	if( sgnViewType == VIEW_VANITY )
	{
		if( sgCameraDolly < MIN_VANITY_DISTANCE_TUGBOAT )
		{
			sgCameraDolly = MIN_VANITY_DISTANCE_TUGBOAT;
		}
	}
	else
	{
		float fNearCap = ( InputGetAdvancedCamera() && !InputGetLockedPitch() ) ? CAMERA_FOLLOW_NEAR_CAP : CAMERA_DOLLY_NEAR_CAP;
		if( sgCameraDolly < fNearCap )
		{
			sgCameraDolly = fNearCap;
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraDollyOut( void )
{
#if !ISVERSION( SERVER_VERSION )
	sgCameraDolly += ( InputGetAdvancedCamera() ? CAMERA_DOLLY_INCREMENT_VANITY : CAMERA_DOLLY_INCREMENT );
	sgfLastImpactTime = 0;
	if( sgCameraDolly > CAMERA_DOLLY_FAR_CAP )
	{
		sgCameraDolly = CAMERA_DOLLY_FAR_CAP;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sUnitAllowsFirstPerson(
	UNIT * pUnit)
{
	if( UnitIsA( pUnit, UNITTYPE_MELEE ) ||
		UnitIsA( pUnit, UNITTYPE_SHIELD ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraSetViewType(
	int nType,
	BOOL bTemporary)
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	ASSERT_RETURN(pGame);
	ASSERT(IS_CLIENT(pGame));
	//leaving in for tugboat
	if ( sgnViewTypeLocked > 0 )
		return;

	if ( nType == VIEW_DEMOLEVEL && NULL == DemoLevelGetDefinition() )
		return;

	if ( nType == VIEW_DEMOLEVEL )
	{
		nType = VIEW_3RDPERSON;
		sgbDemoLevelPath = TRUE;
	}
	else if ( nType == VIEW_FOLLOWPATH )
	{
		nType = VIEW_3RDPERSON;
		sgbFollowPath = TRUE;
	}
	else if ( nType == VIEW_MURMUR )
	{
		nType = VIEW_3RDPERSON;
		sgbFollowPath = FALSE;
	}
	else
	{
		sgbFollowPath = FALSE;
	}

	int current = c_CameraGetViewType();

	// we only want to save this one if we can go back to it.  If this one is not temp, then no need to save the old one.
	if ( !bTemporary && ( current != nType ) )  
	//if ( !bTemporary )  
		sgnViewTypePrev = nType;

	if ( ( current == VIEW_1STPERSON && nType != VIEW_1STPERSON ) ||
		( current != VIEW_1STPERSON && nType == VIEW_1STPERSON ) )
	{
		sgnCameraBone = INVALID_ID;
		sgnDefaultModel = INVALID_ID;
	}

	if( AppIsTugboat() )
	{
		if ( nType == VIEW_3RDPERSON )
		{
			 sgCameraDolly = sgLastCameraDolly;

			// reset 3rd person camera
			if ( c_CameraGetViewType() != VIEW_3RDPERSON )
				sgb3rdPersonInited = FALSE;

			if( InputGetAdvancedCamera() )
			{
				if( !InputGetLockedPitch() )
				{
					sgfFollowAngle = VectorDirectionToAngle( UnitGetFaceDirection( pGame->pCameraUnit, FALSE ) );
					if( nType != current )
					{
						sgfFollowPitch = FOLLOW_PITCH_DEFAULT_TUGBOAT;
						sgCameraDolly = FOLLOW_DISTANCE_DEFAULT_TUGBOAT;
					}
					sgLastFollowDolly = sgCameraDolly;
				}
				else
					sgfFollowAngle = 0;
			}

		}

	}
	else if( AppIsHellgate() )
	{
		UNIT * pCameraUnit = GameGetCameraUnit( pGame );
		if ( nType == VIEW_1STPERSON )
		{
			if ( pCameraUnit && IsUnitDeadOrDying( pCameraUnit ) )
			{
				nType = VIEW_3RDPERSON;
			}
			// 1st person melee restriction
			else
			{
				if ( pCameraUnit )
				{
					UNIT * weapon1 = UnitInventoryGetByLocation( pCameraUnit, INVLOC_RHAND );
					if ( weapon1 && ! c_sUnitAllowsFirstPerson( weapon1 ) )
					{
						nType = VIEW_3RDPERSON;
					}
					UNIT * weapon2 = UnitInventoryGetByLocation( pCameraUnit, INVLOC_LHAND );
					if ( weapon2 && ! c_sUnitAllowsFirstPerson( weapon2 ) )
					{
						nType = VIEW_3RDPERSON;
					}
					if ( UnitHasState( pGame, pCameraUnit, STATE_NO_FIRST_PERSON ) )
					{
						nType = VIEW_3RDPERSON;
					}
				}
			}
		}
		else if ( current == VIEW_1STPERSON && nType == VIEW_3RDPERSON )
		{
			sgfThirdDistance = MAX( MIN_3RDPERSON_DISTANCE, sgfThirdDistance );
			sgb3rdPersonInited = TRUE;

		}
		else if ( nType == VIEW_3RDPERSON )
		{
			// reset 3rd person camera
			if ( c_CameraGetViewType() != VIEW_3RDPERSON )
				sgb3rdPersonInited = FALSE;
		}

		UNIT * pControlUnit = GameGetControlUnit( pGame );
		if ( pControlUnit && UnitHasState( pGame, pControlUnit, STATE_FIRST_PERSON_ONLY ) )
		{
			nType = VIEW_1STPERSON;
		}
	}

	if ( nType == VIEW_VANITY )
	{
		// KCK: Don't use Game Control unit, use current camera unit
//		GameGetControlPosition( pGame, NULL, &sgfVanityAngle, NULL );
		sgfVanityAngle = VectorDirectionToAngle( UnitGetFaceDirection( pGame->pCameraUnit, FALSE ) );

		if( AppIsHellgate() )
		{
			sgfVanityDistance = VANITY_DISTANCE_DEFAULT;
			sgfVanityPitch = VANITY_PITCH_DEFAULT;
		}
		else
		{
			sgLastCameraDolly = sgCameraDolly;
			
			sgfVanityPitch = VANITY_PITCH_DEFAULT_TUGBOAT;
			sgfVanityAngle = VectorDirectionToAngle( UnitGetFaceDirection( pGame->pCameraUnit, FALSE ) ) - DEG_TO_RAD( 180.0f );
			sgfVanityDistance = VANITY_DISTANCE_DEFAULT_TUGBOAT;

		}

	}

	if ( nType == VIEW_RTS )
	{
		UNIT * pPlayer = GameGetCameraUnit( pGame );
		ASSERT_RETURN( pPlayer );
		ROOM * pRoom;
		LEVEL * pLevel = UnitGetLevel( pPlayer );
		ASSERT_RETURN( pLevel );

		VECTOR vPosition;

		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "RTS_Camera", &pRoom, &pOrientation );
		if ( pNode && pRoom && pOrientation )
		{
			VECTOR vDirection, vUp;
			RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );
		}
		else
		{
			vPosition = pPlayer->vPosition;
		}

		sgvRTS_CurrentPosition = vPosition;
		sgvRTS_TargetPosition = vPosition;
	}

	struct CAMERA_TYPE_DATA 
	{
		float fDepthOfFieldNear;
		float fDepthOfFieldFocus;
		float fDepthOfFieldFar;
	};
	const CAMERA_TYPE_DATA ctCameraTypeData[ NUM_VIEWS ] =
	{//		DOF Near	DOF Focus	DOF Far
		{	0.01f,		1.0f,		150.0f	},//VIEW_1STPERSON=0,	
		{	0.01f,		10.0f,		25.0f	},//PLACHOLEDER,			
		{	0.01f,		10.0f,		25.0f	},//PLACHOLEDER,			
		{	0.01f,		10.0f,		25.0f	},//PLACHOLEDER,			
		{	0.01f,		10.0f,		25.0f	},//PLACHOLEDER,			
		{	0.01f,		1.0f,		150.0f	},//VIEW_FLY=5,			
		{	0.01f,		3.0f,		150.0f	},//VIEW_3RDPERSON,
		{	0.01f,		1.0f,		150.0f	},//VIEW_VANITY,
		{	0.01f,		1.0f,		150.0f	},//VIEW_RTS,			
		{	0.01f,		1.0f,		150.0f	},//VIEW_DIABLO,
		{	0.01f,		1.0f,		150.0f	},//VIEW_DIABLOROTATE,
		{	0.01f,		1.0f,		150.0f	},//VIEW_DIABLOZOOM,
		{	0.01f,		1.0f,		150.0f	},//VIEW_DIABLOZR,
		{	0.01f,		1.0f,		150.0f	},//VIEW_LETTERBOX,
		{	0.01f,		1.0f,		150.0f	},//VIEW_FOLLOWPATH,
		{	0.01f,		1.0f,		150.0f	},//VIEW_MURMUR,
		{	0.01f,		1.0f,		150.0f	},//VIEW_DETACHED_CAMERA,
		{	0.01f,		1.0f,		150.0f	},//VIEW_CHARACTER_SCREEN,
	};

	const CAMERA_TYPE_DATA * pCameraTypeData = &ctCameraTypeData[ nType ];
	V( e_DOFSetParams( pCameraTypeData->fDepthOfFieldNear, pCameraTypeData->fDepthOfFieldFocus, 
		pCameraTypeData->fDepthOfFieldFar, 0.0f ) );
											  
	sgnViewType = nType;					  
											  
	UNIT * pUnit = GameGetControlUnit( pGame );
	if ( pUnit )
		c_UnitUpdateViewModel( pUnit, FALSE );
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraZoom(
	BOOL bZoomIn )
{
	int nViewTypeCurr = c_CameraGetViewType();
	if ( nViewTypeCurr == VIEW_1STPERSON )
	{
		if ( bZoomIn )
			c_CameraSetViewType( VIEW_3RDPERSON );
	}
	else if ( nViewTypeCurr == VIEW_3RDPERSON )
	{
		sgfThirdDistance += bZoomIn ? THIRD_PERSON_ZOOM_ADD : -THIRD_PERSON_ZOOM_ADD;

		if ( sgfThirdDistance < MIN_3RDPERSON_DISTANCE )
		{
			sgfThirdDistance = MIN_3RDPERSON_DISTANCE;
			c_CameraSetViewType( VIEW_1STPERSON );
		}
		else if ( sgfThirdDistance >= MAX_3RDPERSON_DISTANCE )
		{
			sgfThirdDistance = MAX_3RDPERSON_DISTANCE;
		}
		else if ( sgfThirdDistance >= MIN_3RDPERSON_DISTANCE )
		{
			c_CameraSetViewType( VIEW_3RDPERSON );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraLockViewType()
{
	ASSERT_RETURN( sgnViewTypeLocked >= 0 );
	sgnViewTypeLocked++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraUnlockViewType()
{
	ASSERT_RETURN( sgnViewTypeLocked > 0 );
	sgnViewTypeLocked--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// called once a control unit is initialized
void c_CameraInitViewType(
	int nType )
{
	if ( nType == VIEW_1STPERSON )
	{
		sgnViewType = VIEW_1STPERSON;
		sgnViewTypePrev = VIEW_1STPERSON;
		sgfThirdDistance = MIN_3RDPERSON_DISTANCE;
	}
	else if ( nType == VIEW_3RDPERSON )
	{
		sgnViewType = VIEW_3RDPERSON;
		sgnViewTypePrev = VIEW_3RDPERSON;
		sgfThirdDistance = START_3RDPERSON_DISTANCE;
		sgb3rdPersonInited = FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraVanityDistance( BOOL bZoomIn )
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );
	ASSERT( sgnViewType == VIEW_VANITY );
	if( AppIsHellgate() )
	{
		if ( bZoomIn )
		{
			if ( sgfVanityDistance > MIN_3RDPERSON_DISTANCE )
			{
				sgfVanityDistance -= THIRD_PERSON_ZOOM_ADD;
			}
		}
		else
		{
			if ( sgfVanityDistance < MAX_3RDPERSON_DISTANCE )
			{
				sgfVanityDistance += THIRD_PERSON_ZOOM_ADD;
			}
		}
	}
	else
	{
		if ( bZoomIn )
		{
			if ( sgfVanityDistance > MIN_VANITY_DISTANCE )
			{
				sgfVanityDistance -= THIRD_PERSON_ZOOM_ADD;
			}
		}
		else
		{
			if ( sgfVanityDistance < MAX_VANITY_DISTANCE )
			{
				sgfVanityDistance += THIRD_PERSON_ZOOM_ADD;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraVanityRotate( float delta )
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );
	ASSERT( sgnViewType == VIEW_VANITY );

	sgfVanityAngle += delta;

	while ( sgfVanityAngle < 0.0f )
		sgfVanityAngle += MAX_RADIAN;
	while ( sgfVanityAngle >= MAX_RADIAN )
		sgfVanityAngle -= MAX_RADIAN;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraVanityPitch( float delta )
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );
	ASSERT( sgnViewType == VIEW_VANITY );

	if( AppIsHellgate() )
	{
		sgfVanityPitch += delta * (PI / (180.f * 6.0f));

		while ( sgfVanityPitch < 0.0f )
			sgfVanityPitch += MAX_RADIAN;
		while ( sgfVanityPitch >= MAX_RADIAN )
			sgfVanityPitch -= MAX_RADIAN;
	}
	else
	{

		sgfVanityPitch += DEG_TO_RAD(delta) * .5f;


		if( sgfVanityPitch < 0.0f )
			sgfVanityPitch = 0;
		if( sgfVanityPitch >= .9f )
			sgfVanityPitch = .9f;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraSetVanityPitch( float fPitch )
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );
	ASSERT( sgnViewType == VIEW_VANITY );

	sgfVanityPitch = fPitch;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraFollowLerp( float fTargetAngle,
						 float fDeltaTime )
{
	if( sgfFollowAngle != fTargetAngle )
	{

		float Delta = sgfFollowAngle - fTargetAngle;
		float Half = DegreesToRadians( 180.0f );
		while( Delta > Half)
		{
			Delta -= Half * 2.0f;
			sgfFollowAngle += Half * 2.0f;
		}
		while( Delta < -Half)
		{
			Delta += Half * 2.0f;
			sgfFollowAngle -= Half * 2.0f;
		}
		Delta *= pow( .3f, fDeltaTime ) ;
		if( fabs( Delta ) < .1f )
		{
			Delta = .1f * ( Delta < 0 ) ? -1.0f : 1.0f;
		}
		if( abs( Delta ) > abs( sgfFollowAngle - fTargetAngle ) )
		{
			Delta = sgfFollowAngle - fTargetAngle;
		}
		sgfFollowAngle = fTargetAngle + Delta;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraFollowRotate( float delta, 
						   BOOL bNoLerp /* = TRUE */)
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );
	sgfFollowAngle += delta;
	if( bNoLerp )
	{
		sgfFollowAngleCurrent = sgfFollowAngle;
	}
	while ( sgfFollowAngle < 0.0f )
	{
		sgfFollowAngle += MAX_RADIAN;
		sgfFollowAngleCurrent += MAX_RADIAN;
	}
	while ( sgfFollowAngle >= MAX_RADIAN )
	{
		sgfFollowAngle -= MAX_RADIAN;
		sgfFollowAngleCurrent -= MAX_RADIAN;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraFollowPitch( float delta )
{
	ASSERT( IS_CLIENT( AppGetCltGame() ) );

	sgfFollowPitch += DEG_TO_RAD(delta) * .2f;

	if( sgfFollowPitch < 0.0f )
		sgfFollowPitch = 0;
	if( sgfFollowPitch >= .8f )
		sgfFollowPitch = .8f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraResetFollow( void )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT( IS_CLIENT( AppGetCltGame() ) );

	UNIT * pPlayer = GameGetControlUnit( AppGetCltGame() );
	if ( !pPlayer )
		return;
	float fBefore = sgfFollowAngle;
	REF(fBefore);
	if( InputGetAdvancedCamera() )
	{
		if( InputGetLockedPitch() )
		{
			sgfFollowAngle = 0;
		}
		else
		{
			sgfFollowAngle = VectorDirectionToAngle( UnitGetFaceDirection( pPlayer, FALSE ) );
		}
	}

	while ( sgfFollowAngle < 0.0f )
	{
		sgfFollowAngle += MAX_RADIAN;
		//sgfFollowAngleCurrent += MAX_RADIAN;
	}
	while ( sgfFollowAngle >= MAX_RADIAN )
	{
		sgfFollowAngle -= MAX_RADIAN;
		//sgfFollowAngleCurrent -= MAX_RADIAN;
	}
	sgfLastImpactTime = 0;

#endif
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraFollowPath(
	GAME * game,
	VECTOR * pvNodeList,
	int nNumNodes,
	float fTime )
{
	ASSERT( IS_CLIENT( game ) );
	ASSERT( nNumNodes < MAX_SPLINE_KNOTS );

#if !ISVERSION(SERVER_VERSION)
	c_CameraSetViewType( VIEW_FOLLOWPATH, TRUE );
	UISetCinematicMode(TRUE);

	if ( pvNodeList )
	{
		for ( int i = 0; i < nNumNodes; i++ )
		{
			sgvCameraCurve[i] = pvNodeList[i];
		}
		SplineSetKnotTable( &sgnCameraKnots[0], nNumNodes );
		sgnNumCameraKnots = nNumNodes;
	}

	sgFollowPathDeltaTime = (TIME)(float)FLOOR((fTime * (float)MSECS_PER_SEC) + 0.5f);
	sgFollowPathMoveStartTime = AppCommonGetCurTime();
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraSetDefaultModel( int nModelId )
{
	sgnDefaultModel = nModelId;
	sgnCameraBone = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraResetBone()
{
	sgnCameraBone = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraShake( GAME * game, VECTOR & vDirection )
{
	ASSERT(IS_CLIENT(game));

	// set up shaking
	vDirection.fZ = 0.0f;
	VectorNormalize( sgvShakeDirection, vDirection );
	sgvShake = sgvShakeDirection * 0.10f;
	sgfShakeTime = GameGetTimeFloat(game) + SHAKE_TIME;
	sgfShakeMagnitude = SHAKE_SCALE;
	sgbShake = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraShakeRandom(
	GAME * pGame,
	const VECTOR & vDirection,
	const float fDuration,
	const float fMagnitude,
	const float fDegrade)
{
	ASSERT(IS_CLIENT(pGame));

	if(sgbRandomShake && sgfRandomShakeMagnitude >= SHAKE_SCALE * fMagnitude)
	{
		return;
	}

	// set up shaking
	sgvRandomShakeDirection = vDirection;
	sgvRandomShakeDirection.fZ = 0.0f;
	VectorNormalize( sgvRandomShakeDirection );
	sgvRandomShake = VECTOR(0.0f);
	sgfRandomShakeStartTime = GameGetTimeFloat(pGame);
	if(fDuration <= 0.0f)
	{
		sgfRandomShakeTime = FLT_MAX;
	}
	else
	{
		// For some reason, the durations are doubled in length?
		// Weird, but I'm not sure how to track it down.  Cutting the lengths
		// in half has the desired effect, anyway.
		sgfRandomShakeTime = sgfRandomShakeStartTime + (fDuration / 2.0f);
	}
	sgfRandomShakeMagnitude = SHAKE_SCALE * fMagnitude;
	sgfRandomShakeDegrade = fDegrade;
	sgbRandomShake = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraShakeOFF()
{
	// depends on game time... might be switching games
	sgbShake = FALSE;
	sgbRandomShake = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float c_CameraGetCosFOV(
	float fWidenFactor )
{
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return 1.0f;
	}

#if ISVERSION(SERVER_VERSION)
	return 1.f;
#else
	// get a generous cos FOV assuming a square aspect in largest dimension
	float fHorizFOV = e_GetHorizontalFOV( pCameraInfo->fVerticalFOV );
	float fFOVX = fHorizFOV * 0.5f; // get the half-angle width FOV
	fFOVX = fFOVX / ( PI * 0.5f ); //  convert from radians
	return fFOVX * fWidenFactor;
#endif // SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraAdjustPointForFirstPersonFOV(
	GAME * pGame,
	VECTOR & vPosition,
	BOOL bAllowStaleData )
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! c_CameraIsFirstPersonFOVEnabled( pGame ) )
		return;

	MATRIX mProj;
	MATRIX mProj2;
	MATRIX mView;
	e_GetWorldViewProjMatricies( NULL, &mView, &mProj, &mProj2, bAllowStaleData );
	MATRIX mToFP;
	MatrixMultiply( &mToFP,   &mView, &mProj2 );
	MATRIX mFromFP;
	MatrixMultiply( &mFromFP, &mView, &mProj );
	MatrixInverse( &mFromFP, &mFromFP );

	MATRIX mTotal;
	MatrixMultiply( &mTotal, &mToFP, &mFromFP );
	MatrixMultiply( &vPosition, &vPosition, &mTotal );
#endif //!SERVER_VERSION
}

void c_CameraGetViewMatrix(
	const CAMERA_INFO * pCamera,
	MATRIX * pmatDest )
{
	CameraGetViewMatrix( pCamera, pmatDest );
}

void c_CameraGetProjMatrix(
	const CAMERA_INFO * pCamera,
	MATRIX * pmatDest )
{
	CameraGetProjMatrix( pCamera, pmatDest );
}

VECTOR c_GetCameraLocation(
	GAME * pGame )
{
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if ( pCameraInfo )
	{
		return pCameraInfo->vPosition;
	}
	return (VECTOR &)sgvDefaultEyePosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraUpdateCallback()
{
	c_CameraUpdate( AppGetCltGame(), 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_CameraGetNearFar( float & fNear, float & fFar )
{
#if !ISVERSION(SERVER_VERSION)
	fNear = e_GetNearClipPlane();
	fFar  = e_GetFarClipPlane();
#endif //ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VECTOR CameraInfoGetPosition(
	const CAMERA_INFO * pCamera)
{
	if (pCamera)
	{
		return pCamera->vPosition;
	}
	return VECTOR(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VECTOR CameraInfoGetDirection(
	const CAMERA_INFO * pCamera)
{
	if (pCamera)
	{
		return pCamera->vDirection;
	}
	return VECTOR(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VECTOR CameraInfoGetLookAt(
	const struct CAMERA_INFO * pCamera)
{
	if (pCamera)
	{
		return pCamera->vLookAt;
	}
	return VECTOR(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float CameraInfoGetPitch(
	const CAMERA_INFO * pCamera)
{
	if (pCamera)
	{
		return pCamera->fPitch;
	}
	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float CameraInfoGetAngle(
	const CAMERA_INFO * pCamera)
{
	if (pCamera)
	{
		return pCamera->fAngle;
	}
	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_3RD_PERSON
void CameraForceRotate()
{
	sgbCameraForceRotate = TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_3RD_PERSON
BOOL CameraRotateDetached( VECTOR & vDirection )
{
	if ( sgbCameraRotateDetached )
	{
		vDirection = sgvLastLookAt - sgvLastPosition;
		VectorNormalize( vDirection );
	}
	return sgbCameraRotateDetached;
}
#endif

float c_CameraGetZDelta()
{
	float fScale = 1.0f;
	{
		UNIT * pCameraUnit = GameGetCameraUnit( AppGetCltGame() );
		if ( pCameraUnit )
		{
			int nModelId = c_UnitGetModelIdThirdPerson( pCameraUnit );
			if ( nModelId != INVALID_ID )
				fScale = c_AppearanceGetHeightFactor( nModelId );
		}
	}

	return -sgfCameraCrouchDelta + FIRST_PERSON_Z_DELTA - (PLAYER_HEIGHT - PLAYER_HEIGHT*fScale);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float c_CameraGetFirstPersonScaling()
{
	UNIT * pCameraUnit = GameGetCameraUnit( AppGetCltGame() );
	if ( ! pCameraUnit )
		return 1.0f;
	int nModelId = c_UnitGetModelIdFirstPerson( pCameraUnit );
	if ( nModelId == INVALID_ID )
		nModelId = c_UnitGetModelIdThirdPerson( pCameraUnit );
	if ( nModelId != INVALID_ID )
		return e_ModelGetScale( nModelId ).fX;
	return 1.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraSetRTS_TargetPosition(
	VECTOR & vTarget )
{
#if HELLGATE_ONLY
	GAME * pGame = AppGetCltGame();
	if ( !pGame )
		return;

	UNIT * pPlayer = GameGetControlUnit( pGame );
	if ( !pPlayer )
		return;

	LEVEL * pLevel = UnitGetLevel( pPlayer );
	if ( !pLevel )
		return;

	ROOM * pRoom = RoomGetFromPosition( AppGetCltGame(), pLevel, &vTarget );
	if ( !pRoom )
		return;

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pRoom, vTarget, &pDestRoom );
	if ( !pNode )
		return;

	if ( pDestRoom != pRoom )
		return;

	VECTOR vWorldPosition;
	MatrixMultiply( &vWorldPosition, &pNode->vPosition, &pDestRoom->tWorldMatrix );

	if ( fabsf( vWorldPosition.z - vTarget.z ) > 0.5f )
		return;
	
	sgvRTS_TargetPosition = vWorldPosition;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CameraCharacterScreenDistance( BOOL bZoomIn )
{
	ASSERT_RETURN( IS_CLIENT( AppGetCltGame() ) );
	ASSERT_RETURN( sgnViewType == VIEW_CHARACTER_SCREEN );
	ASSERT_RETURN( AppIsHellgate() );

	if ( bZoomIn )
	{
		sgfCharacterScreenDistance = MIN_CHARACTER_SCREEN_DISTANCE;
	}
	else
	{
		sgfCharacterScreenDistance = MAX_CHARACTER_SCREEN_DISTANCE;
	}
}
