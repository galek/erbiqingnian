
//----------------------------------------------------------------------------
// player_move_msg.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "player_move_msg.h"
#include "c_message.h"
#ifndef _BOT
#include "prime.h"
#include "units.h"
#include "excel.h"
#include "unitmodes.h"
#include "skills.h"
#include "states.h"
#include "rjdmovementtracker.h"
#include "interact.h"
#include "gameunits.h"
#include "wardrobe.h"
#include "player.h"
#include "objects.h"
#include "uix.h"
#include "c_appearance.h"
#include "movement.h"
#else
#include "..\data_common\excel\unitmodes_hdr.h"
#undef HAVOK_ENABLED
#endif

#define MOVE_MODE_BITS 5
#define LEVELID_BITS 10
#define ROOMID_BITS 12
#define ROOM_RELATIVE_POSITION_BITS 15
#define CHARACTER_RELATIVE_POSITION_BITS 10
#define CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT ((float)(1<<(CHARACTER_RELATIVE_POSITION_BITS-1)) / 100.0f)
#define MAX_TICKS_BETWEEN_UPDATES 20

//#define PLAYER_MOVE_TRACE

#ifdef PLAYER_MOVE_TRACE
static int sgnMessagesSkipped = 0;
static int sgnMessagesSent = 0;
#endif

struct PLAYER_MOVE_FRAME
{
	ROOMID idRoom;
	VECTOR vPosition;
	UNITMODE eForwardMode;
	UNITMODE eSideMode;
	VECTOR vFaceDirection;
	VECTOR pvWeaponPos[ MAX_WEAPONS_PER_UNIT ];
	VECTOR pvWeaponDir[ MAX_WEAPONS_PER_UNIT ];

	VECTOR vMoveDirection;  // is multiplied by speed
#ifdef HAVOK_ENABLED
	BOOL bHasTurn;
	hkQuaternion qTurn;
#endif
	LEVELID idLevel;
};

enum 
{
	PLAYER_REQMOVE_BIT_POSITION = 0,
	PLAYER_REQMOVE_BIT_POSITION_AS_FULL_VECTOR,
	PLAYER_REQMOVE_BIT_MODES,
	PLAYER_REQMOVE_BIT_FACE_DIRECTION,
	PLAYER_REQMOVE_BIT_WEAPON_POS0,
	PLAYER_REQMOVE_BIT_WEAPON_DIR0,
	PLAYER_REQMOVE_BIT_WEAPON_POS1,
	PLAYER_REQMOVE_BIT_WEAPON_DIR1,
	PLAYER_REQMOVE_BIT_MOVE_DIRECTION,
	PLAYER_REQMOVE_BIT_TURN,
	PLAYER_REQMOVE_BIT_LEVELID,

	NUM_PLAYER_REQMOVE_BITS
};

struct PLAYER_MOVE_INFO 
{
	GAME_TICK nLastFrameTick;
	PLAYER_MOVE_FRAME tLastFrameSent;
	PLAYER_MOVE_FRAME tCurrentFrame;
	VECTOR vPrevFaceDirection;
};

#ifndef _BOT
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sShouldSendWeaponPosAndDirection(
									  UNIT * pWeapon,
									  int nIndex )
{
	if ( ! pWeapon )
		return FALSE;

	if ( UnitIsA( pWeapon, UNITTYPE_MELEE ) )
		return FALSE;

	if ( nIndex > 0 && UnitIsA( pWeapon, UNITTYPE_CABALIST_FOCUS ) )
		return FALSE; // we don't pay attention the offhand focus item

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void c_sGetPlayerMoveFrame(
	UNIT * pUnit,
	PLAYER_MOVE_FRAME & tMoveFrame,
	BOOL bInitializing = FALSE )
{
	tMoveFrame.eForwardMode = MODE_IDLE;
	if ( UnitTestMode( pUnit, MODE_RUN ) )
		tMoveFrame.eForwardMode = MODE_RUN;
	else if ( UnitTestMode( pUnit, MODE_BACKUP ) )
		tMoveFrame.eForwardMode = MODE_BACKUP;

	tMoveFrame.eSideMode = MODE_IDLE;
	if ( UnitTestMode( pUnit, MODE_STRAFE_LEFT ) )
		tMoveFrame.eSideMode = MODE_STRAFE_LEFT;
	else if ( UnitTestMode( pUnit, MODE_STRAFE_RIGHT ) )
		tMoveFrame.eSideMode = MODE_STRAFE_RIGHT;

	tMoveFrame.vPosition = UnitGetPosition( pUnit );
	if ( bInitializing & !pUnit->pRoom )
		tMoveFrame.idRoom = INVALID_ID;
	else
		tMoveFrame.idRoom = UnitGetRoomId( pUnit );

	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETURN( nWardrobe != INVALID_ID );

	GAME * pGame = UnitGetGame( pUnit );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
		if ( sShouldSendWeaponPosAndDirection( pWeapon, i ))
		{
			tMoveFrame.pvWeaponPos[ i ] = pUnit->pvWeaponPos[ i ];
			tMoveFrame.pvWeaponDir[ i ] = pUnit->pvWeaponDir[ i ];
		} else {
			tMoveFrame.pvWeaponPos[ i ] = VECTOR( 0.0f );
			tMoveFrame.pvWeaponDir[ i ] = VECTOR( 0.0f );
		}
	}
	tMoveFrame.vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);
	float fSpeed = UnitGetVelocity(pUnit);
	tMoveFrame.vMoveDirection = UnitGetMoveDirection( pUnit ) * fSpeed;

	tMoveFrame.idLevel = UnitGetLevelID( pUnit );

#ifdef HAVOK_ENABLED
	hkVector4 vFaceFrom( pUnit->pPlayerMoveInfo->vPrevFaceDirection.fX, pUnit->pPlayerMoveInfo->vPrevFaceDirection.fY, pUnit->pPlayerMoveInfo->vPrevFaceDirection.fZ );
	hkVector4 vFaceTo( tMoveFrame.vFaceDirection.fX, tMoveFrame.vFaceDirection.fY, tMoveFrame.vFaceDirection.fZ );
	tMoveFrame.qTurn.setShortestRotation( vFaceFrom, vFaceTo );

	if ( ! tMoveFrame.qTurn.isOk() )
		tMoveFrame.qTurn.setIdentity();
	if ( tMoveFrame.qTurn.getAngle() < 0.0001f )
		tMoveFrame.bHasTurn = FALSE;
#endif
}
#endif // !_BOT

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline
BOOL sPlayerFrameIsChanging(
	PLAYER_MOVE_FRAME & tFrame )
{
#ifdef HAVOK_ENABLED
	return !VectorIsZeroEpsilon( tFrame.vMoveDirection ) || tFrame.bHasTurn;
#else
	return TRUE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define XFER_VECTOR_FOR_REQMOVE(xfer, bit, elem) if ( TESTBIT(dwReqMoveBitMask, bit) ) { if (xfer.XferVector(elem) == XFER_RESULT_ERROR) return E_FAIL; }
#define XFER_NVECTOR_FOR_REQMOVE(xfer, bit, elem) if ( TESTBIT(dwReqMoveBitMask, bit) ) { if (xfer.XferNVector(elem) == XFER_RESULT_ERROR) return E_FAIL; }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static inline 
BOOL sXFerMoveFramePositionRelativeTo(
	VECTOR & vPosition,
	const VECTOR & vPreviousPosition,
	XFER<mode> & tXFer)
{
	DWORD dwPosition[3];
	DWORD dwIsFullVector = 0;
	if(tXFer.IsSave())
	{
		// Okay, so if we're saving out, then figure out where we are.

		// Find the vector from the previous position
		VECTOR vDistanceFromPrevious = vPosition - vPreviousPosition;
		// ... in centimeters
		vDistanceFromPrevious *= 100.0f;
		// ... and convert to int
		dwPosition[0] = (DWORD)vDistanceFromPrevious.fX;
		dwPosition[1] = (DWORD)vDistanceFromPrevious.fY;
		dwPosition[2] = (DWORD)vDistanceFromPrevious.fZ;
		if((dwPosition[0] >= (1 << CHARACTER_RELATIVE_POSITION_BITS)) ||
			(dwPosition[1] >= (1 << CHARACTER_RELATIVE_POSITION_BITS)) ||
			(dwPosition[2] >= (1 << CHARACTER_RELATIVE_POSITION_BITS)))
		{
			dwIsFullVector = 1;
		}
	}
	tXFer.XferUInt( dwIsFullVector, 1 );
	if(dwIsFullVector != 0)
	{
		tXFer.XferVector(vPosition);
	}
	else
	{
		tXFer.XferUInt( dwPosition[0], CHARACTER_RELATIVE_POSITION_BITS );
		tXFer.XferUInt( dwPosition[1], CHARACTER_RELATIVE_POSITION_BITS );
		tXFer.XferUInt( dwPosition[2], CHARACTER_RELATIVE_POSITION_BITS );
		if(tXFer.IsLoad())
		{
			VECTOR vOffsetFromPrevious;
			vOffsetFromPrevious.fX = (float)dwPosition[0];
			vOffsetFromPrevious.fY = (float)dwPosition[1];
			vOffsetFromPrevious.fZ = (float)dwPosition[2];

			vOffsetFromPrevious /= 100.0f;

			vPosition = vOffsetFromPrevious + vPreviousPosition;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static
BOOL sXFerMoveFrame(
	GAME * pGame,
	BOOL  bUnitIsInTown,
	PLAYER_MOVE_FRAME & tMoveFrame,
	XFER<mode> & tXFer )
{
	DWORD dwReqMoveBitMask[ DWORD_FLAG_SIZE(NUM_PLAYER_REQMOVE_BITS) ];
	ZeroMemory( &dwReqMoveBitMask, sizeof( dwReqMoveBitMask ) );

	VECTOR vRelativeOffset = tMoveFrame.vPosition;
	vRelativeOffset.fX -= CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT;
	vRelativeOffset.fY -= CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT;
	vRelativeOffset.fZ -= (CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT / 2.0f);

	if ( tXFer.IsSave() )
	{
		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_POSITION);
		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_MODES);
		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_FACE_DIRECTION);
		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_MODES);

		if(!pGame)
		{
			SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_POSITION_AS_FULL_VECTOR);
		}

		BOOL bIsInTown = bUnitIsInTown;
		if ( ! bIsInTown )
		{
			if ( ! VectorIsZero( tMoveFrame.pvWeaponDir[ 0 ] ) )
			{
				SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_POS0);
				SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_DIR0);
			}
			if ( ! VectorIsZero( tMoveFrame.pvWeaponDir[ 1 ] ) )
			{
				SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_POS1);
				SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_DIR1);
			}
		}

		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_MOVE_DIRECTION);

#ifdef HAVOK_ENABLED
		if ( tMoveFrame.bHasTurn )
		{
			SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_TURN);
		}
#endif
		SETBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_LEVELID);

	} else {
		ZeroMemory( &tMoveFrame, sizeof( PLAYER_MOVE_FRAME ) );
	}

	ASSERT_RETFALSE( NUM_PLAYER_REQMOVE_BITS < 32 );
	tXFer.XferUInt( dwReqMoveBitMask[ 0 ], NUM_PLAYER_REQMOVE_BITS );

	if ( TESTBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_MODES ) )
	{
		tXFer.XferUInt( tMoveFrame.eForwardMode, MOVE_MODE_BITS );
		tXFer.XferUInt( tMoveFrame.eSideMode, MOVE_MODE_BITS );
	}

	// We transfer the position as a triplet of integers from the vMin of the room's AABB, in centimeters
	// ...if we have a GAME to work with.  (Bots, apparently, don't have a GAME at this point somehow?)
	if(TESTBIT(dwReqMoveBitMask, PLAYER_REQMOVE_BIT_POSITION_AS_FULL_VECTOR))
	{
		XFER_VECTOR_FOR_REQMOVE( tXFer, PLAYER_REQMOVE_BIT_POSITION, tMoveFrame.vPosition );
	}
	else if(TESTBIT(dwReqMoveBitMask, PLAYER_REQMOVE_BIT_POSITION))
	{
#ifndef _BOT
		DWORD dwPosition[3];
		if(tXFer.IsSave())
		{
			// Okay, so if we're saving out, then figure out where we are.
			ASSERT_RETFAIL(tMoveFrame.idRoom < (1 << ROOMID_BITS));
			ROOM * pRoom = RoomGetByID(pGame, tMoveFrame.idRoom);
			ASSERT_RETFAIL(pRoom);
			const BOUNDING_BOX * pRoomBoundingBox = RoomGetBoundingBoxWorldSpace(pRoom);
			ASSERT_RETFAIL(pRoomBoundingBox);
			BOUNDING_BOX tBox;
			tBox = *pRoomBoundingBox;
			BoundingBoxExpandBySize(&tBox, 5.0f);

			// Find the vector from the room's minimum
			VECTOR vDistanceFromRoomMin = tMoveFrame.vPosition - tBox.vMin;
			// ... in centimeters
			vDistanceFromRoomMin *= 100.0f;
			// if it's only slightly negative, set it to zero, since we'll be casting to unsigned int
			if (vDistanceFromRoomMin.fX >= -50.0f && vDistanceFromRoomMin.fX < 0.0f)
			{
				vDistanceFromRoomMin.fX = 0.0f;
			}
			if (vDistanceFromRoomMin.fY >= -50.0f && vDistanceFromRoomMin.fY < 0.0f)
			{
				vDistanceFromRoomMin.fY = 0.0f;
			}
			if (vDistanceFromRoomMin.fZ >= -50.0f && vDistanceFromRoomMin.fZ < 0.0f)
			{
				vDistanceFromRoomMin.fZ = 0.0f;
			}
			// ... and convert to int
			dwPosition[0] = (DWORD)vDistanceFromRoomMin.fX;
			dwPosition[1] = (DWORD)vDistanceFromRoomMin.fY;
			dwPosition[2] = (DWORD)vDistanceFromRoomMin.fZ;
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT)
			IF_NOT_RETFAIL(dwPosition[0] < (1 << ROOM_RELATIVE_POSITION_BITS));
			IF_NOT_RETFAIL(dwPosition[1] < (1 << ROOM_RELATIVE_POSITION_BITS));
			IF_NOT_RETFAIL(dwPosition[2] < (1 << ROOM_RELATIVE_POSITION_BITS));
#else
			ASSERTV_RETFAIL(dwPosition[0] < (1 << ROOM_RELATIVE_POSITION_BITS), "Error xferring out of bounds unit position (%1.2f, %1.2f, %1.2f) room [%s] aabb min (%1.2f, %1.2f, %1.2f)", tMoveFrame.vPosition.fX, tMoveFrame.vPosition.fY, tMoveFrame.vPosition.fZ, RoomGetDevName(pRoom), tBox.vMin.fX, tBox.vMin.fY, tBox.vMin.fZ);
			ASSERTV_RETFAIL(dwPosition[1] < (1 << ROOM_RELATIVE_POSITION_BITS), "Error xferring out of bounds unit position (%1.2f, %1.2f, %1.2f) room [%s] aabb min (%1.2f, %1.2f, %1.2f)", tMoveFrame.vPosition.fX, tMoveFrame.vPosition.fY, tMoveFrame.vPosition.fZ, RoomGetDevName(pRoom), tBox.vMin.fX, tBox.vMin.fY, tBox.vMin.fZ);
			ASSERTV_RETFAIL(dwPosition[2] < (1 << ROOM_RELATIVE_POSITION_BITS), "Error xferring out of bounds unit position (%1.2f, %1.2f, %1.2f) room [%s] aabb min (%1.2f, %1.2f, %1.2f)", tMoveFrame.vPosition.fX, tMoveFrame.vPosition.fY, tMoveFrame.vPosition.fZ, RoomGetDevName(pRoom), tBox.vMin.fX, tBox.vMin.fY, tBox.vMin.fZ);
#endif
		}
		tXFer.XferUInt( tMoveFrame.idRoom, ROOMID_BITS );
		tXFer.XferUInt( dwPosition[0], ROOM_RELATIVE_POSITION_BITS );
		tXFer.XferUInt( dwPosition[1], ROOM_RELATIVE_POSITION_BITS );
		tXFer.XferUInt( dwPosition[2], ROOM_RELATIVE_POSITION_BITS );
		if(tXFer.IsLoad())
		{
			ROOM * pRoom = RoomGetByID(pGame, tMoveFrame.idRoom);
			// We don't assert here, because it's possible that the client has sent this to the server, which has relayed it to another client, which doesn't know about the room
			if(!pRoom)
			{
				return E_FAIL;
			}
			// We DO assert here, because we'd damn well BETTER have a bounding box
			const BOUNDING_BOX * pRoomBoundingBox = RoomGetBoundingBoxWorldSpace(pRoom);
			ASSERT_RETFAIL(pRoomBoundingBox);
			BOUNDING_BOX tBox;
			tBox = *pRoomBoundingBox;
			BoundingBoxExpandBySize(&tBox, 5.0f);

			VECTOR vOffsetFromRoomMin;
			vOffsetFromRoomMin.fX = (float)dwPosition[0];
			vOffsetFromRoomMin.fY = (float)dwPosition[1];
			vOffsetFromRoomMin.fZ = (float)dwPosition[2];

			vOffsetFromRoomMin /= 100.0f;

			tMoveFrame.vPosition = vOffsetFromRoomMin + tBox.vMin;

			// We've gotten a new position, so let's recalculate the relative position
			vRelativeOffset = tMoveFrame.vPosition;
			vRelativeOffset.fX -= CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT;
			vRelativeOffset.fY -= CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT;
			vRelativeOffset.fZ -= (CHARACTER_RELATIVE_POSITION_BOX_OFFSET_FLOAT / 2.0f);
		}
#else
		ASSERT_RETFAIL(0);
#endif
	}

	XFER_NVECTOR_FOR_REQMOVE( tXFer, PLAYER_REQMOVE_BIT_FACE_DIRECTION,	tMoveFrame.vFaceDirection );
	if(TESTBIT(dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_POS0))
	{
		sXFerMoveFramePositionRelativeTo(tMoveFrame.pvWeaponPos[ 0 ], vRelativeOffset, tXFer);
	}
	XFER_NVECTOR_FOR_REQMOVE( tXFer, PLAYER_REQMOVE_BIT_WEAPON_DIR0,	tMoveFrame.pvWeaponDir[ 0 ] );
	if(TESTBIT(dwReqMoveBitMask, PLAYER_REQMOVE_BIT_WEAPON_POS1))
	{
		sXFerMoveFramePositionRelativeTo(tMoveFrame.pvWeaponPos[ 1 ], vRelativeOffset, tXFer);
	}
	XFER_NVECTOR_FOR_REQMOVE( tXFer, PLAYER_REQMOVE_BIT_WEAPON_DIR1,	tMoveFrame.pvWeaponDir[ 1 ] );
	XFER_VECTOR_FOR_REQMOVE( tXFer, PLAYER_REQMOVE_BIT_MOVE_DIRECTION,	tMoveFrame.vMoveDirection );

#ifdef HAVOK_ENABLED
	if ( TESTBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_TURN ) )
	{
		if ( tXFer.IsSave() )
		{
			for ( int i = 0; i < 4; i++ )
			{
				float fVal = tMoveFrame.qTurn.m_vec.getSimdAt( i );
				tXFer.XferFloat( fVal );
			}
		} else {

			float pfVals[ 4 ];
			tXFer.XferFloat( pfVals[ 0 ] );
			tXFer.XferFloat( pfVals[ 1 ] );
			tXFer.XferFloat( pfVals[ 2 ] );
			tXFer.XferFloat( pfVals[ 3 ] );
			tMoveFrame.qTurn.m_vec.set( pfVals[ 0 ], pfVals[ 1 ], pfVals[ 2 ], pfVals[ 3 ] );
		}
		tMoveFrame.bHasTurn = TRUE;
	} else {
		tMoveFrame.bHasTurn = FALSE;
	}
#endif

	if ( TESTBIT( dwReqMoveBitMask, PLAYER_REQMOVE_BIT_LEVELID ) )
	{
		tXFer.XferUInt( tMoveFrame.idLevel, LEVELID_BITS );
	}

	/*
#ifndef _BOT
	if(tXFer.IsSave())
	{
		trace("[%c] %d - Length of Player Move Buffer: %d\n", HOST_CHARACTER(pGame), GameGetTick(pGame), tXFer.GetCursor());
	}
#endif
	// */

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef HAVOK_ENABLED
static
void sRotateVector(
	VECTOR & vFinal,
	const hkQuaternion & qRotation,
	const VECTOR & vSourceIn )
{
	hkVector4 vSource( vSourceIn.fX, vSourceIn.fY, vSourceIn.fZ );
	hkVector4 vOut;
	vOut.setRotatedDir( qRotation, vSource );
	vFinal.fX = vOut.getSimdAt( 0 );
	vFinal.fY = vOut.getSimdAt( 1 );
	vFinal.fZ = vOut.getSimdAt( 2 );
}
#endif

//----------------------------------------------------------------------------
// predict the next move position of the player given the last one and some additional info
//----------------------------------------------------------------------------
static
void sPredictNextPlayerMoveFrame(
	PLAYER_MOVE_FRAME & tMoveFrameNext,
	const PLAYER_MOVE_FRAME & tMoveFramePrev,
	int nGameTicksToAdvance )
{
	MemoryCopy( &tMoveFrameNext, sizeof( PLAYER_MOVE_FRAME ), &tMoveFramePrev, sizeof( PLAYER_MOVE_FRAME ) );

#ifdef HAVOK_ENABLED
	hkQuaternion qTurnCurr( tMoveFramePrev.qTurn );
	if ( tMoveFramePrev.bHasTurn && tMoveFramePrev.qTurn.isOk() && tMoveFramePrev.qTurn.getAngle() > 0.01f )
	{
		if ( nGameTicksToAdvance > 1 )
		{
			hkVector4 vAxis;
			qTurnCurr.getAxis( vAxis );
			if ( vAxis.lengthSquared3() > 0.0f )
				qTurnCurr.setAxisAngle( vAxis, nGameTicksToAdvance * qTurnCurr.getAngle() );
			else
				qTurnCurr.setIdentity();
		}
		sRotateVector( tMoveFrameNext.vFaceDirection,	qTurnCurr, tMoveFramePrev.vFaceDirection );
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			if ( ! VectorIsZero( tMoveFramePrev.pvWeaponPos[ i ] ))
				sRotateVector( tMoveFrameNext.pvWeaponDir[ i ], qTurnCurr, tMoveFramePrev.pvWeaponDir[ i ] );
		}
		sRotateVector( tMoveFrameNext.vMoveDirection,	qTurnCurr, tMoveFramePrev.vMoveDirection );
	}
#endif

	float fTimeDelta = nGameTicksToAdvance * GAME_TICK_TIME_FLOAT;
	VECTOR vPosDelta = tMoveFrameNext.vMoveDirection * fTimeDelta;
	BOOL bZeroPosDelta = VectorIsZero( vPosDelta );
	if ( ! bZeroPosDelta )
	{
		tMoveFrameNext.vPosition = tMoveFramePrev.vPosition + vPosDelta;
	}
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( VectorIsZero( tMoveFramePrev.pvWeaponPos[ i ] ))
			continue;

		if ( ! bZeroPosDelta )
			tMoveFrameNext.pvWeaponPos[ i ] = tMoveFramePrev.pvWeaponPos[ i ] + vPosDelta;

#ifdef HAVOK_ENABLED
		if ( tMoveFramePrev.bHasTurn )
		{// rotate the weapons relative to the body
			VECTOR vWeaponToPosition = tMoveFrameNext.pvWeaponPos[ i ] - tMoveFrameNext.vPosition;
			sRotateVector( vWeaponToPosition, qTurnCurr, vWeaponToPosition );
			tMoveFrameNext.pvWeaponPos[ i ] = tMoveFrameNext.vPosition + vWeaponToPosition;
		}
#endif
	}
}

//----------------------------------------------------------------------------
// predict the next move position of the player given the last one and some additional info
//----------------------------------------------------------------------------
static
BOOL sPlayerMoveFrameIsCloseEnough(
#ifndef _BOT
	UNIT * pUnit,
#else
	BOOL bInTown,
#endif
	const PLAYER_MOVE_FRAME & tFirst,
	const PLAYER_MOVE_FRAME & tSecond )
{
	if ( tFirst.eForwardMode != tSecond.eForwardMode )
		return FALSE;

	if ( tFirst.eSideMode != tSecond.eSideMode )
		return FALSE;

#ifndef _BOT
	BOOL bIsInTown = UnitIsInTown( pUnit );
	BOOL bCheckWeapons = !bIsInTown;
	if ( bCheckWeapons && IS_CLIENT( pUnit ) )
	{
		int nModel = c_UnitGetModelId( pUnit );
		bCheckWeapons = c_AppearanceIsAggressive( nModel );
	}
#else
	BOOL bIsInTown = bInTown;
	BOOL bCheckWeapons = FALSE;
#endif

	VECTOR pDirectionsToCompare[ 3 ][ 2 ] =
	{
		{ tFirst.vFaceDirection, tSecond.vFaceDirection },
		{ tFirst.pvWeaponDir[ 0 ],	tSecond.pvWeaponDir[ 0 ] },
		{ tFirst.pvWeaponDir[ 1 ],	tSecond.pvWeaponDir[ 1 ] },
	};
	VECTOR pPositionsToCompare[ 3 ][ 2 ] =
	{
		{ tFirst.vPosition,		tSecond.vPosition },
		{ tFirst.pvWeaponPos[ 0 ],	tSecond.pvWeaponPos[ 0 ] },
		{ tFirst.pvWeaponPos[ 1 ],	tSecond.pvWeaponPos[ 1 ] },
	};

	float fDistanceErrorSquared = bIsInTown ? (1.5f * 1.5f) : (0.25f * 0.25f);
	float fDirectionError = bIsInTown ? 0.8f : 0.95f;

	for (unsigned int ii = 0; ii < 3; ii++)
	{
		if ( ii > 0 && !bCheckWeapons )
			break;  // we don't care about weapons in town
		float distsq = VectorDistanceSquared( pPositionsToCompare[ ii ][ 0 ], pPositionsToCompare[ ii ][ 1 ] );
		if ( distsq > fDistanceErrorSquared )
		{
#ifdef PLAYER_MOVE_TRACE
			trace( "Not Close Enough - Position:%d\n", ii );
#endif
			return FALSE;
		}
		if ( VectorDot( pDirectionsToCompare[ ii ][ 0 ], pDirectionsToCompare[ ii ][ 1 ] ) < fDirectionError )
		{
#ifdef PLAYER_MOVE_TRACE
			trace( "Not Close Enough - Direction:%d\n", ii );
#endif
			return FALSE;
		}
	}

#ifdef PLAYER_MOVE_TRACE
	trace( "Close Enough... %s\n", UnitTestModeGroup( pUnit, MODEGROUP_MOVE ) ? "Moving" : "" );
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PLAYER_MOVE_INFO * PlayerMoveInfoInit(
	MEMORYPOOL *pPool)
{
	return (PLAYER_MOVE_INFO *)MALLOCZ(pPool, sizeof(PLAYER_MOVE_INFO));
}

//----------------------------------------------------------------------------
// Determine whether we should send a move based upon whether our prediction
// of our position in the elapsed time is close enough to our actual position.
//----------------------------------------------------------------------------
BOOL PlayerMoveIsPredictedClosely(
#ifndef _BOT
	UNIT *data, //*pUnit,
#else
	BOOL data, //bInTown 
#endif
	PLAYER_MOVE_INFO *pPlayerMoveInfo,
	const PLAYER_MOVE_FRAME &tMoveFrameToSend,
	int nTickDelta)
{
	ASSERT_RETFALSE(pPlayerMoveInfo);
	BOOL bSend = FALSE;

	if ( nTickDelta > MAX_TICKS_BETWEEN_UPDATES &&
		sPlayerFrameIsChanging( pPlayerMoveInfo->tLastFrameSent ) )
		bSend = TRUE;

	if ( !bSend )
	{
		PLAYER_MOVE_FRAME tMoveFramePredicted;
		sPredictNextPlayerMoveFrame( tMoveFramePredicted, pPlayerMoveInfo->tLastFrameSent, nTickDelta );
		if ( ! sPlayerMoveFrameIsCloseEnough( data, tMoveFramePredicted, tMoveFrameToSend ) )
			bSend = TRUE;
	}

	return bSend;
}


//----------------------------------------------------------------------------
// To be more realistic to players, we're sending with the same criteria.
//
// Sadly, Guy's modification to the CCMD_REQMOVE format has utterly broken
// creating the buffer correctly for bots.  Therefore, we're sending a
// CCMD_BOT_CHEAT with the move frame data and re-constituting it on
// the server.
//----------------------------------------------------------------------------
#define BOT_ANGLE_GRANULARITY 10000

#ifdef _BOT
BOOL PlayerMoveMsgSendBotMovement(
	MSG_CCMD_BOT_CHEAT & tMsg,
	PLAYER_MOVE_INFO * pPlayerMoveInfo,
	GAME_TICK nCurrentTick,
	BOOL bIsInTown,
	const VECTOR & vPosition,
	const VECTOR & vFaceDirection,
	float fSpeed,
	UNITMODE eForwardMode,
	UNITMODE eSideMode,
	LEVELID idLevel )
{
	PLAYER_MOVE_FRAME tPlayerMoveFrame;
	ZeroMemory( &tPlayerMoveFrame, sizeof( PLAYER_MOVE_FRAME ) );

	tPlayerMoveFrame.vPosition = vPosition;
	tPlayerMoveFrame.vFaceDirection = vFaceDirection;
	tPlayerMoveFrame.vMoveDirection = vFaceDirection * fSpeed;
	tPlayerMoveFrame.eForwardMode = MODE_RUN;

	//Determine whether to send this frame.
	BOOL bSend = FALSE;
	if(!pPlayerMoveInfo)
	{//If we're not using the fancy data for the fancy algorithm, just always send.
		bSend = TRUE;
	}
	else
	{
		int nTickDelta = 
			(pPlayerMoveInfo->nLastFrameTick ? 
			nCurrentTick - pPlayerMoveInfo->nLastFrameTick : 0);
		
		if(PlayerMoveIsPredictedClosely(
			bIsInTown, pPlayerMoveInfo, tPlayerMoveFrame, nTickDelta))
		{
			bSend = TRUE;
		}
	}

	if(bSend)
	{
		/*BIT_BUF tBuf( tMsg.buffer, MAX_REQMOVE_MSG_BUFFER_SIZE * BITS_PER_BYTE );
		XFER<XFER_SAVE> tXFer( &tBuf );

		sXFerMoveFrame<XFER_SAVE>( NULL, FALSE, tPlayerMoveFrame, tXFer );
		MSG_SET_BBLOB_LEN(tMsg, buffer, tXFer.GetLen() );
		*/

		tMsg.vPosition = tPlayerMoveFrame.vPosition;
		//Represent move direction as two DWORDs.  We'll translate back on the other side.
		tMsg.dwParam1 = DWORD((signed long) (tPlayerMoveFrame.vMoveDirection.fX * BOT_ANGLE_GRANULARITY));
		tMsg.dwParam2 = DWORD((signed long) (tPlayerMoveFrame.vMoveDirection.fY * BOT_ANGLE_GRANULARITY));

		if(pPlayerMoveInfo) 
		{
			pPlayerMoveInfo->tLastFrameSent = tPlayerMoveFrame;
			pPlayerMoveInfo->nLastFrameTick = nCurrentTick;
		}
	}
	return bSend;
}
#endif //_BOT

//----------------------------------------------------------------------------
// End bot-useful functions.  Bot does not look at any code after this point.
#ifndef _BOT

//----------------------------------------------------------------------------
// send message to server requesting movement
//----------------------------------------------------------------------------
void c_PlayerSendMove(
	GAME * pGame,
	BOOL bSkipPredictionTest /* = FALSE */)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(IS_CLIENT(pGame));

	UNIT * pUnit = GameGetControlUnit(pGame);
	ASSERT_RETURN(pUnit);
	if (!UnitGetRoom(pUnit))
	{
		return;
	}

	ASSERT_RETURN( pUnit->pPlayerMoveInfo );

	// if I wasn't moving, and now I am, send
	BOOL bSend = FALSE;

	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		bSend = TRUE;
	}

	PLAYER_MOVE_FRAME tMoveFrameToSend;
	c_sGetPlayerMoveFrame( pUnit, tMoveFrameToSend );
	pUnit->pPlayerMoveInfo->vPrevFaceDirection = UnitGetFaceDirection( pUnit, FALSE );

	int nTickDelta = pUnit->pPlayerMoveInfo->nLastFrameTick ? GameGetTick( pGame ) - pUnit->pPlayerMoveInfo->nLastFrameTick : 0;

	if(!bSend && 
	    ( bSkipPredictionTest ||
		  UnitGetLevelID(pUnit) != pUnit->pPlayerMoveInfo->tLastFrameSent.idLevel ||
		  PlayerMoveIsPredictedClosely(pUnit, pUnit->pPlayerMoveInfo, tMoveFrameToSend, nTickDelta) ))
	{
		bSend = TRUE;
	}

	if (UIShowingLoadingScreen())
	{
		bSend = FALSE;
	}

	if (IsUnitDeadOrDying(pUnit))
	{
		bSend = FALSE;
	}
	
	if (bSend)
	{
		MSG_CCMD_REQMOVE tMsg;
		BIT_BUF tBuf( tMsg.buffer, MAX_REQMOVE_MSG_BUFFER_SIZE * BITS_PER_BYTE );
		XFER<XFER_SAVE> tXFer( &tBuf );

		sXFerMoveFrame<XFER_SAVE>( pGame, UnitIsInTown(pUnit), tMoveFrameToSend, tXFer );
		MSG_SET_BBLOB_LEN(tMsg, buffer, tXFer.GetLen() );	

		c_SendMessage(CCMD_REQMOVE, &tMsg);

		pUnit->pPlayerMoveInfo->tLastFrameSent = tMoveFrameToSend;
		pUnit->pPlayerMoveInfo->nLastFrameTick = GameGetTick( pGame );

#ifdef PLAYER_MOVE_TRACE
		sgnMessagesSent++;
#endif
	} else {
#ifdef PLAYER_MOVE_TRACE
		sgnMessagesSkipped++;
		if ( sgnMessagesSkipped % 1000 == 0 )
		{
			trace( "%d/%d Messages Skipped\n", sgnMessagesSkipped, sgnMessagesSkipped+sgnMessagesSent );
		}
#endif
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleModeChangesForPlayer(
	UNIT * pUnit,
	UNITMODE eInputMode,
	UNITMODE eModeOption1,
	UNITMODE eModeOption2 )
{
	UNITLOG_ASSERT_RETURN( eInputMode == MODE_IDLE || eInputMode == eModeOption1 || eInputMode == eModeOption2, pUnit );

	if ( eInputMode == MODE_IDLE )
	{
		if ( UnitTestMode( pUnit, eModeOption1 ) )
			UnitEndMode( pUnit, eModeOption1, 0, FALSE );
		if ( UnitTestMode( pUnit, eModeOption2 ) )
			UnitEndMode( pUnit, eModeOption2, 0, FALSE );
	}
	else 
	{
		UNITMODE eModeOther = eInputMode == eModeOption1 ? eModeOption2 : eModeOption1;

		if ( UnitTestMode( pUnit, eInputMode ) == 0 )
			s_UnitSetMode( pUnit, eInputMode, FALSE );
		if ( UnitTestMode( pUnit, eModeOther ) )
			UnitEndMode( pUnit, eModeOther, 0, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetOrClearMode(
	GAME * game,
	UNIT * pUnit,
	UNITMODE eMode,
	UNITMODE eInputMode )
{
	BOOL bHasMode = UnitTestMode( pUnit, eMode );
	if ( eMode == eInputMode )
	{
		if ( ! bHasMode )
		{
			c_UnitSetMode( pUnit, eMode );
		}
	} 
	else
	{
		if ( bHasMode )
		{
			UnitEndMode( pUnit, eMode );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sPlayerCanDoMove(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// you cannot move if you are not in a room
	if (UnitGetRoom( pUnit ) == NULL)
	{
		return FALSE;
	}
	
	if (IsUnitDeadOrDying(pUnit))
	{
		return FALSE;
	}

	if (UnitIsInLimbo(pUnit))
	{
		return FALSE;
	}
	
	return TRUE;
	
}

#define LATERAL_MOVEMENT_EPSILON (0.001)
//----------------------------------------------------------------------------
// SPEED HACK DETECTION METHOD:
// A legitimate client should send a movement every pGame tick.  Regardless of
// lag, his total movements will always be within the movement tolerance.
// Note that this is NOT true of a single pGame tick, only of movements added
// up over a period.

// Thus, all we need to do is verify that the movement is within bounds,
// and verify that the player is only sending us one movement per pGame tick.
// This IS skewed by lag, but we'll do things like keep track of their alltime
// and their short term message rate.  Players could stand still then speed
// burst, but this is indistinguishable from lag anyway.
//
// Detection strategy: detect the hack, but don't respond to it or ignore the
// message.  Merely flag the account.
//----------------------------------------------------------------------------
#define VELOCITY_LAG_TOLERANCE 1.0f
#define MAX_MUZZLE_DISTANCE_FROM_PLAYER_SQUARED (4.0f * 4.0f)
static BOOL s_sPlayerMoveMsgHandleRequest(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * pUnit,
	PLAYER_MOVE_FRAME & tMoveFrame )
{

	BOOL bStopMoving = FALSE;
	
	if (s_sPlayerCanDoMove( pUnit ) == FALSE)
	{
		bStopMoving = TRUE;
	}
	
	if(SkillIsOnWithFlag(pUnit, SKILL_FLAG_PLAYER_STOP_MOVING))
	{
		bStopMoving = TRUE;
	}

	VECTOR vStopMovingPosition(0);
	int nStopMovingState = UnitGetFirstStateOfType(pGame, pUnit, STATE_STOP_MOVING);
	if (nStopMovingState >= 0)
	{
		vStopMovingPosition = UnitGetStatVector(pUnit, STATS_STATE_SET_POINT_X, nStopMovingState);

		const float MAX_STOP_MOVING_DISTANCE = 1.0f;
		const float MAX_STOP_MOVING_DISTANCE_SQ = MAX_STOP_MOVING_DISTANCE * MAX_STOP_MOVING_DISTANCE;
		if(VectorDistanceSquared(vStopMovingPosition, tMoveFrame.vPosition) > MAX_STOP_MOVING_DISTANCE_SQ)
		{
			bStopMoving = TRUE;
		}
	}

	// handle mode changes first so that we can get a proper velocity
	// ROBERT - we really should check to see whether these modes make sense with the face direction and move direction
	float fDistanceMoved = 0.0f;
	VECTOR vPositionPrevious = UnitGetPosition(pUnit);
	VECTOR vMoveDelta(0.0f);
	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit ); 
	//if ( ! bStopMoving )
	{
		sHandleModeChangesForPlayer(pUnit, tMoveFrame.eForwardMode,	MODE_RUN,		    MODE_BACKUP);
		sHandleModeChangesForPlayer(pUnit, tMoveFrame.eSideMode,		MODE_STRAFE_RIGHT,	MODE_STRAFE_LEFT);

		//Keep track of both where the server and the client think they are.
		//This avoids false alarms from client and server desynchronization.
		VECTOR vPositionPreviousClientSide = client->m_vPositionLastReported;
		VECTOR vMoveDeltaClientSide = tMoveFrame.vPosition - vPositionPreviousClientSide;
		vMoveDeltaClientSide.fZ = 0.0f;
		VECTOR vMoveDirectionClientSide; 
		float fDistanceMovedClientSide = VectorNormalize( vMoveDirectionClientSide, vMoveDeltaClientSide );

		vMoveDelta = tMoveFrame.vPosition - vPositionPrevious;
		vMoveDeltaClientSide.fZ = 0.0f; //ignore Z component.
		fDistanceMoved = VectorNormalize( vMoveDirection, vMoveDelta );

		// try to make sure that they aren't moving too fast.
		float fVelocity = UnitGetVelocity( pUnit );

		client->m_pMovementTracker->AddMovement(GameGetTick(pGame), 
			MIN(fDistanceMoved, fDistanceMovedClientSide), fVelocity);

		//Turned to assert once for Colin's sake.  Will do more thorough database
		//data logging in server version.  Assert is here so that we know if it goes
		//off for no reason in legitimate play.

		// moving forces you to stop talking to anybody
		s_InteractCancelUnitsTooFarAway(pUnit);


		float zDelta = tMoveFrame.vPosition.fZ - vPositionPrevious.fZ;
		UnitSetPosition(pUnit, tMoveFrame.vPosition);

		{// check to see whether they are on the ground
			float THETA = 0.5f;
			float DISTANCE_OFFSET = 1.0f;
			//float fTestDistance = UnitGetPosition(pUnit).z;
			//float fTestRadius = 0;//UnitGetCollisionRadius( pUnit ) / 2.0f;
			LEVEL * pLevel = UnitGetLevel( pUnit );
			float fToGround = THETA;
			if(pLevel)
			{
				fToGround = LevelLineCollideLen(pGame, pLevel, tMoveFrame.vPosition+ VECTOR(0, 0, DISTANCE_OFFSET), VECTOR(0, 0, -1), 10);
			}
			BOOL bOnGround = ( (fToGround <= DISTANCE_OFFSET + THETA) || 0.1f < abs(zDelta));
			//Combining on ground test to be farther from the ground, but you can't be moving up and down too much - bmanegold
			UnitSetFlag( pUnit, UNITFLAG_ONGROUND, bOnGround );

		}
	}

	VECTOR pvWeaponPosition [ MAX_WEAPONS_PER_UNIT ];
	VECTOR pvWeaponDirection[ MAX_WEAPONS_PER_UNIT ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pvWeaponPosition[ i ] = tMoveFrame.pvWeaponPos[ i ];
		pvWeaponDirection[ i ] = tMoveFrame.pvWeaponDir[ i ];
	}

	// validate weapon positions as being sane
	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETFALSE( nWardrobe != INVALID_ID );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
		if ( ! pWeapon )
			continue;
		if ( VectorIsZero( pvWeaponPosition[ i ] ) )
			continue;
		VECTOR vDelta = pvWeaponPosition[ i ] - tMoveFrame.vPosition;
		//float fLengthSquared = VectorLengthSquared( vDelta );
		// we can turn this back on when data is better... plus it seems to break as you warp through portals
		//UNITLOG_ASSERT ( fLengthSquared < MAX_MUZZLE_DISTANCE_FROM_PLAYER_SQUARED, pUnit );

		// we could insert a directional check here - to verify that you aren't shooting backwards and such
		// however, templar spin attacks would mess it up... it 
	}
	if ( !bStopMoving )
	{
		if ( fDistanceMoved <= 0.0f )
		{
			vMoveDirection = UnitGetMoveDirection( pUnit );
		}

		PlayerSetMovementAndAngle(pGame, ClientGetId(client), tMoveFrame.vFaceDirection, pvWeaponPosition, pvWeaponDirection,
			vMoveDirection);
		if (!UnitGetRoom(pUnit))
		{
			return FALSE;
		}
		UnitUpdateRoomFromPosition(pGame, pUnit, &vPositionPrevious);
		ObjectsCheckForTriggers(pGame, client, pUnit);

		if (pUnit && !UnitTestFlag( pUnit, UNITFLAG_FREED ))
		{
			// when we do lateral movement
			VECTOR vPositionCurrent = UnitGetPosition( pUnit );
			if (fabs(vMoveDelta.fX) > LATERAL_MOVEMENT_EPSILON || fabs(vMoveDelta.fY) > LATERAL_MOVEMENT_EPSILON)
			{

				// player has moved
				s_PlayerMoved( pUnit );

			}	

			if ( AppIsHellgate() )
			{
				SkillsUpdateMovingFiringError( pUnit, fDistanceMoved );
			}
		}
		
	} 
	else 
	{
		pUnit->vFaceDirection = tMoveFrame.vFaceDirection;

		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			pUnit->pvWeaponPos[ i ] = pvWeaponPosition[ i ];
			pUnit->pvWeaponDir[ i ] = pvWeaponDirection[ i ];
		}
	}

	client->m_vPositionLastReported = tMoveFrame.vPosition;
	
	// return TRUE if we still have a unit and it is NOT freed
	return pUnit && !UnitTestFlag( pUnit, UNITFLAG_FREED );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void s_sUpdateOtherPlayers(
	GAME * pGame,
	UNIT * pUnit,
	PLAYER_MOVE_FRAME & tMoveFrame)
{
	GAMECLIENT * pClient = UnitGetClient( pUnit );
	ASSERT_RETURN( pClient );
	GAMECLIENTID idClient = ClientGetId( pClient );

	MSG_SCMD_PLAYERMOVE tMsg;
	BIT_BUF tBuf( tMsg.buffer, MAX_REQMOVE_MSG_BUFFER_SIZE * BITS_PER_BYTE );
	XFER<XFER_SAVE> tXFer( &tBuf );
	sXFerMoveFrame( pGame, UnitIsInTown(pUnit), tMoveFrame, tXFer );
	MSG_SET_BBLOB_LEN(tMsg, buffer, tXFer.GetLen() );	

	tMsg.idUnit = UnitGetId( pUnit );
	s_SendUnitMessage( pUnit, SCMD_PLAYERMOVE, &tMsg, idClient );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerMoveMsgHandleRequest(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * pUnit,
	BYTE * data)
{
	ASSERT_RETURN(pUnit);

	UNITLOG_ASSERT_RETURN( AppIsHellgate(), pUnit );

	if (s_sPlayerCanDoMove(pUnit) == FALSE)
	{
		return;
	}
	
	PLAYER_MOVE_FRAME tMoveFrame;
	{
		MSG_CCMD_REQMOVE * pMsg = (MSG_CCMD_REQMOVE *)data;
		int nLength = MSG_GET_BLOB_LEN( pMsg, buffer ) * BITS_PER_BYTE;
		BIT_BUF tBuf( pMsg->buffer, nLength );
		XFER<XFER_LOAD> tXFer( &tBuf );
		if ( ! sXFerMoveFrame( pGame, UnitIsInTown(pUnit), tMoveFrame, tXFer ) )
			return;
	}

	if (s_sPlayerMoveMsgHandleRequest( pGame, client, pUnit, tMoveFrame ))
	{	

		ASSERT_RETURN( pUnit->pPlayerMoveInfo );
		pUnit->pPlayerMoveInfo->tLastFrameSent = tMoveFrame;
		pUnit->pPlayerMoveInfo->nLastFrameTick = GameGetTick( pGame );

		s_sUpdateOtherPlayers( pGame, pUnit, pUnit->pPlayerMoveInfo->tLastFrameSent );
		
	}
	
}

//----------------------------------------------------------------------------
// Like the player reqmove, but translates the move frame out of the cheat.
//----------------------------------------------------------------------------
void s_BotMoveMsgHandleRequest(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * pUnit,
	BYTE * data)
{
	ASSERT_RETURN(pUnit);

	UNITLOG_ASSERT_RETURN( AppIsHellgate(), pUnit );

	if (s_sPlayerCanDoMove(pUnit) == FALSE)
	{
		return;
	}

	PLAYER_MOVE_FRAME tMoveFrame;
	{
		MSG_CCMD_BOT_CHEAT * pMsg = (MSG_CCMD_BOT_CHEAT *)data;
		
		tMoveFrame.eForwardMode = MODE_RUN;
		tMoveFrame.eSideMode = MODE_IDLE;
		tMoveFrame.vPosition = pMsg->vPosition;
		tMoveFrame.idRoom = UnitGetRoomId(pUnit);
		tMoveFrame.idLevel = UnitGetLevelID(pUnit);
		VECTOR vDirection(0.0f, 0.0f, 0.0f);
		vDirection.fX = float((signed long) (pMsg->dwParam1))/BOT_ANGLE_GRANULARITY;
		vDirection.fY = float((signed long) (pMsg->dwParam2))/BOT_ANGLE_GRANULARITY;
		tMoveFrame.vMoveDirection = vDirection;
		VectorNormalize(vDirection);
		tMoveFrame.vFaceDirection = vDirection;
	}

	if (s_sPlayerMoveMsgHandleRequest( pGame, client, pUnit, tMoveFrame ))
	{

		ASSERT_RETURN( pUnit->pPlayerMoveInfo );
		pUnit->pPlayerMoveInfo->tLastFrameSent = tMoveFrame;
		pUnit->pPlayerMoveInfo->nLastFrameTick = GameGetTick( pGame );

		s_sUpdateOtherPlayers( pGame, pUnit, pUnit->pPlayerMoveInfo->tLastFrameSent );
		
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerUpdateMovement(
	GAME * pGame, 
	UNIT * pUnit, 
	const struct EVENT_DATA & event_data)
{
	if (IS_CLIENT(pGame) && pUnit == GameGetControlUnit(pGame))
	{
		return FALSE;
	}

	BOOL bUnitIsValid = TRUE;
	
	if (s_sPlayerCanDoMove(pUnit))
	{
		GAME_TICK nTickCurr = GameGetTick( pGame );
		if ( pUnit->pPlayerMoveInfo->nLastFrameTick != nTickCurr &&
			 UnitGetLevelID( pUnit ) == pUnit->pPlayerMoveInfo->tLastFrameSent.idLevel &&
			 sPlayerFrameIsChanging( pUnit->pPlayerMoveInfo->tLastFrameSent ))
		{
			int nTickDelta = pUnit->pPlayerMoveInfo->nLastFrameTick ? nTickCurr - pUnit->pPlayerMoveInfo->nLastFrameTick : 0;
			sPredictNextPlayerMoveFrame( pUnit->pPlayerMoveInfo->tCurrentFrame, pUnit->pPlayerMoveInfo->tLastFrameSent, nTickDelta );

			if ( IS_SERVER( pGame ) )
			{
				GAMECLIENT * client = UnitGetClient( pUnit );

				bUnitIsValid = s_sPlayerMoveMsgHandleRequest( pGame, client, pUnit, pUnit->pPlayerMoveInfo->tCurrentFrame );
			}
		}
		
	}
	else
	{
		if (pUnit->pPlayerMoveInfo)
		{
			pUnit->pPlayerMoveInfo->nLastFrameTick = GameGetTick( pGame );
		}
	}

	if (bUnitIsValid)
	{	
		UnitRegisterEventTimed(pUnit, sPlayerUpdateMovement, EVENT_DATA(), 1);
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerMoveMsgInitPlayer( 
	UNIT * pUnit )
{
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( ! pUnit->pPlayerMoveInfo );
	pUnit->pPlayerMoveInfo = PlayerMoveInfoInit(GameGetMemPool(pGame));
	if (IS_CLIENT(pGame) && pUnit != GameGetControlUnit(pGame))
	{
		c_sGetPlayerMoveFrame( pUnit, pUnit->pPlayerMoveInfo->tLastFrameSent, TRUE );
		pUnit->pPlayerMoveInfo->vPrevFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
	}
	UnitRegisterEventTimed( pUnit, sPlayerUpdateMovement, EVENT_DATA(), 1 );
}

#define PLAYER_SYNC_WARP_DISTANCE		( 10.0f )
#define PLAYER_SYNC_SIMULATE_DISTANCE	( 0.01f )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerMoveMsgUpdateOtherPlayer(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// sanity
	ASSERTX_RETURN( pUnit != GameGetControlUnit( pGame ), "This is for other players, not the local player" );

	// get movement information
	PLAYER_MOVE_FRAME tMoveFrame;
	GAME_TICK nTickCurr = GameGetTick( pGame );

	ASSERT_RETURN( pUnit->pPlayerMoveInfo );
	if ( pUnit->pPlayerMoveInfo->nLastFrameTick == nTickCurr )
	{
		tMoveFrame = pUnit->pPlayerMoveInfo->tLastFrameSent;
	} else {
		int nTicksToAdvance = nTickCurr - pUnit->pPlayerMoveInfo->nLastFrameTick;
		nTicksToAdvance = MAX( 1, nTicksToAdvance );
		sPredictNextPlayerMoveFrame( tMoveFrame, pUnit->pPlayerMoveInfo->tLastFrameSent, nTicksToAdvance );
	}

	// get their new position 
	VECTOR vOldPosition = UnitGetPosition( pUnit );
	VECTOR vNewPosition = tMoveFrame.vPosition;

	// if the unit is jumping or recovering from a jump, ignore their current z 
	// according to the server in favor of the jump z we are simulating locally
	// An updated z on the ground should be received from the server by the 
	// time the recovery is finished
	BOOL bJumpMode = (UnitModeIsJumping( pUnit ) || UnitModeIsJumpRecovering( pUnit ) );
	if ( bJumpMode )
	{
		vNewPosition.fZ = vOldPosition.fZ;
	}

	VECTOR vUp( 0.0f, 0.0f, 1.0f );
	ROOM * pRoom = UnitGetRoom( pUnit );
	if ( !pRoom )
	{
		// if the player doesn't have a room defined yet,
		// then get the room from their location
		LEVEL* level = UnitGetLevel( GameGetControlUnit( pGame ) );		// assume same level for now...
		pRoom = RoomGetFromPosition( pGame, level, &vNewPosition );
		//ASSERTX_RETURN( pRoom, "Expected room" );
	}

	VECTOR vMoveDirection = vNewPosition - UnitGetPosition( pUnit );
	float fDistanceFromCurr = VectorNormalize( vMoveDirection );
	float fErrorTolerance = fabsf( UnitGetVelocity( pUnit ) * GAME_TICK_TIME ) + PLAYER_SYNC_SIMULATE_DISTANCE;
	BOOL bWarpUnit = FALSE;
	if ( fDistanceFromCurr > PLAYER_SYNC_WARP_DISTANCE )
	{
		UnitSetFlag( pUnit, UNITFLAG_CLIENT_PLAYER_MOVING, FALSE );
		UnitStepListRemove( pGame, pUnit );
		bWarpUnit = TRUE;
	}
	else if ( bJumpMode || fDistanceFromCurr > fErrorTolerance )
	{
		UnitSetFlag( pUnit, UNITFLAG_CLIENT_PLAYER_MOVING, TRUE );
		UnitStepListAdd( pGame, pUnit );
	} 
	else if ( !bJumpMode )
	{
		UnitSetFlag( pUnit, UNITFLAG_CLIENT_PLAYER_MOVING, FALSE );
		UnitStepListRemove( pGame, pUnit );
	}

	VECTOR vFaceDirection = tMoveFrame.vFaceDirection;
	float fDot = VectorDot( vMoveDirection, vFaceDirection );
	UNITMODE eForwardMode = tMoveFrame.eForwardMode;
	if ( fDistanceFromCurr > 0.5f && !bWarpUnit )
	{
		if ( fDot > 0.1f )
			eForwardMode = MODE_RUN;
		else if ( fDot < -0.1f )
			eForwardMode = MODE_BACKUP;
	}

	VECTOR vSide;
	if ( fabsf( vMoveDirection.fZ ) != 1.0f )
	{
		VectorCross( vSide, vMoveDirection, VECTOR(0,0,1) );
		VectorNormalize( vSide );
	}
	else
		vSide = VECTOR(0,1,0);
	float fSideDot = VectorDot( vSide, vFaceDirection );

	UNITMODE eSideMode = tMoveFrame.eSideMode;
	if ( fDistanceFromCurr > 0.5f && !bWarpUnit )
	{
		if ( fSideDot > 0.1f )
			eSideMode = MODE_STRAFE_RIGHT;
		else if ( fSideDot < -0.1f )
			eSideMode = MODE_STRAFE_LEFT;
	}

	sSetOrClearMode( pGame, pUnit, MODE_RUN,	eForwardMode );
	sSetOrClearMode( pGame, pUnit, MODE_BACKUP, eForwardMode );
	sSetOrClearMode( pGame, pUnit, MODE_STRAFE_LEFT,  eSideMode );
	sSetOrClearMode( pGame, pUnit, MODE_STRAFE_RIGHT, eSideMode );
	UnitCalcVelocity(pUnit);	// update the player's velocity for the mode, the set or clear mode calls will not calc the correct velocity for all possible modes

	if ( bWarpUnit )
	{
		LEVEL* level = UnitGetLevel( GameGetControlUnit( pGame ) );		// assume same level for now...
		ASSERTX_RETURN( level, "Expected level" );
		pRoom = RoomGetFromPosition( pGame, level, &vNewPosition );
		//ASSERTX_RETURN( pRoom, "Expected room" );
		if(!pRoom) goto justmove;
		UnitWarp( pUnit, pRoom, vNewPosition, tMoveFrame.vFaceDirection, VECTOR( 0, 0, 1 ), 0, FALSE );
	} else {
justmove:
		UnitSetMoveDirection( pUnit, vMoveDirection );
		UnitSetFaceDirection( pUnit, tMoveFrame.vFaceDirection );
	}

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pUnit->pvWeaponPos[ i ] = tMoveFrame.pvWeaponPos[ i ];
		pUnit->pvWeaponDir[ i ] = tMoveFrame.pvWeaponDir[ i ];
	}

	SkillsUpdateMovingFiringError( pUnit, fDistanceFromCurr );

	//UnitWarp( pUnit, pRoom, vNewPosition, tMoveFrame.vDirection, vUp, 0, FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_PlayerMoveMsg(
	GAME * pGame,
	MSG_SCMD_PLAYERMOVE * pMsg)
{
	UNIT * pUnit = UnitGetById( pGame, pMsg->idUnit );
	if (pUnit == NULL)
	{
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf(
			szMessage,
			MAX_MESSAGE,
			"Client received player move message for unknown unit id '%d'\n",
			pMsg->idUnit);
		ASSERTX_RETURN( 0, szMessage );
	}	
	
	ASSERT_RETURN( pUnit->pPlayerMoveInfo );

	int nLength = MSG_GET_BLOB_LEN( pMsg, buffer ) * BITS_PER_BYTE;
	BIT_BUF tBuf( pMsg->buffer, nLength );
	XFER<XFER_LOAD> tXFer( &tBuf );
	BOOL bXFerSuccess = sXFerMoveFrame( pGame, UnitIsInTown(pUnit), pUnit->pPlayerMoveInfo->tLastFrameSent, tXFer );
	pUnit->pPlayerMoveInfo->nLastFrameTick = GameGetTick( pGame ); // or should this come from the message?

	// We must check bXFerSuccess here because the level may not be valid if the XFer failed
	if(bXFerSuccess && pUnit->pPlayerMoveInfo->tLastFrameSent.idLevel == UnitGetLevelID(GameGetControlUnit(pGame)))
	{
		c_UnitSetNoDraw(pUnit, FALSE);
	}

	// process what we just received
	c_PlayerMoveMsgUpdateOtherPlayer( pUnit );

}
#endif //!ISVERSION(SERVER_VERSION)

#endif //!_BOT