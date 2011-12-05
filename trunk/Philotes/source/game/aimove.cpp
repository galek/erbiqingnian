//----------------------------------------------------------------------------
// aimove.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ai.h"
#include "ai_priv.h"
#include "s_message.h"
#include "movement.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "astar.h"
#include "performance.h"
#include "room_path.h"
#include "skills.h"
#include "gameclient.h"

#define SHORT_RANGE_TEST 0.15f

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_FindPathNode(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vDestination,
	FREE_PATH_NODE & tNearestPathNode,
	UNIT * pTargetUnit)
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pUnit);

	if(!pUnit->m_pPathing)
	{
		return FALSE;
	}

	NEAREST_NODE_SPEC tSpec;
	if( AppIsHellgate() )
	{
		SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
		SETBIT(tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES);
	}
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
	SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
	if( AppIsTugboat() )	// tugboat wants to bias nodes on the other side of our target farther away.
	{
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);

		tSpec.vFaceDirection = UnitGetPosition( pUnit ) - vDestination;
		tSpec.vFaceDirection.fZ = 0;
		VectorNormalize( tSpec.vFaceDirection );
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
		if( UnitIsPlayer( pUnit ) )
		{
			tSpec.fMaxDistance = 20;
		}
	}
	ROOM * pSourceRoom = pTargetUnit ? UnitGetRoom(pTargetUnit) : UnitGetRoom(pUnit);
	int nCount = RoomGetNearestPathNodes(pGame, pSourceRoom, vDestination, 1, &tNearestPathNode, &tSpec);
	return (nCount == 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_FindPathNode(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vDestination,
	UNIT * pTargetUnit)
{
	FREE_PATH_NODE tNearestPathNode;
	if (AI_FindPathNode(pGame, pUnit, vDestination, tNearestPathNode, pTargetUnit))
	{
		vDestination = RoomPathNodeGetExactWorldPosition(pGame, tNearestPathNode.pRoom, tNearestPathNode.pNode);
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_TurnTowardsTargetId(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTarget,
	BOOL bImmediate,
	BOOL bFaceAway)
{
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_TURN) )
		return;

	VECTOR vDelta = UnitGetPosition( pTarget ) - UnitGetPosition( pUnit );
	VectorNormalize( pUnit->pvWeaponDir[ 0 ], vDelta );
	vDelta.fZ = 0.0f;

	VECTOR vDirection;
	if ( VectorIsZero( vDelta ) )
	{
		vDirection = VECTOR ( 0, 1, 0 );
	} else {
		VectorNormalize( vDirection, vDelta );
	}

	UnitSetFaceDirection( pUnit, vDirection );

	if(IS_SERVER(pGame))
	{
		MSG_SCMD_UNITTURNID msg;
		msg.id = UnitGetId( pUnit );
		msg.targetid = UnitGetId( pTarget );
		SETBIT(&msg.bFlags, UT_IMMEDIATE, bImmediate);
		SETBIT(&msg.bFlags, UT_FACEAWAY, bFaceAway);
		s_SendUnitMessage(pUnit, SCMD_UNITTURNID, &msg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_TurnTowardsTargetXYZ(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vTarget,
	const VECTOR & vUp,
	BOOL bImmediate)
{
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_TURN) )
		return;

	VECTOR vDelta = vTarget - UnitGetPosition( pUnit );
	vDelta.fZ = 0.0f;
	VECTOR vDirection;
	if ( VectorIsZero( vDelta ) )
		vDirection = VECTOR ( 0, 1, 0 );
	else 
		VectorNormalize( vDirection, vDelta );

	//pUnit->vWeaponDir = vDirection;
	UnitSetUpDirection  ( pUnit, vUp );
	UnitSetFaceDirection( pUnit, vDirection );

	if(IS_SERVER(pGame))
	{
		MSG_SCMD_UNITTURNXYZ msg;
		msg.id = UnitGetId( pUnit );
		msg.vFaceDirection = vDirection;
		msg.vUpDirection   = vUp;
		msg.bImmediate = (BYTE)bImmediate;
		s_SendUnitMessage(pUnit, SCMD_UNITTURNXYZ, &msg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR sAiGetRandomDirection(
	GAME* game,
	UNIT* unit)
{
	VECTOR vRet = RandGetVector(game);
	VectorNormalize(vRet, vRet);
	return vRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float AIGetAltitude( 
	GAME * pGame, 
	UNIT * pUnit )
{
#define MAX_ALTITUDE 100.0f
	LEVEL * pLevel = UnitGetLevel( pUnit );
	float fDist = LevelLineCollideLen( pGame, pLevel, UnitGetPosition( pUnit ), 
		VECTOR( 0, 0, -1.0f), MAX_ALTITUDE );
	if ( fDist == MAX_ALTITUDE )
		return 0.0f; // we are probably poking through the floor a little bit
	return fDist;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL CanMoveInDirection(
	GAME* pGame,
	UNIT* unit,
	VECTOR& vDirection)
{
	if( AppIsHellgate() )
	{
		VECTOR vDeltaShort = vDirection;
		vDeltaShort *= SHORT_RANGE_TEST;
		VECTOR vTempPosition = UnitGetPosition(unit) + vDeltaShort;
		COLLIDE_RESULT result;
		RoomCollide(unit, UnitGetPosition(unit), vDeltaShort, &vTempPosition, UnitGetPathingCollisionRadius( unit ), UnitGetCollisionHeight( unit ), MOVEMASK_TEST_ONLY, &result);
		return (!TESTBIT(&result.flags, MOVERESULT_COLLIDE));
	}
	else
	{
		VECTOR vStart = UnitGetPosition( unit );
		vStart.fZ += .5f;
		
		VECTOR vCollisionNormal;

		float fCollideDistance = LevelLineCollideLen( pGame, UnitGetLevel( unit ), vStart, vDirection, SHORT_RANGE_TEST, &vCollisionNormal);

		if (fCollideDistance >= SHORT_RANGE_TEST)
		{
			COLLIDE_RESULT result;
			DWORD dwMoveFlags = 0;
			dwMoveFlags |= MOVEMASK_TEST_ONLY;
			dwMoveFlags |= MOVEMASK_SOLID_MONSTERS;
			dwMoveFlags |= MOVEMASK_NOT_WALLS;

			VECTOR vDeltaShort = vDirection * SHORT_RANGE_TEST;
			VECTOR TestPoint = vStart + vDeltaShort;
			RoomCollide(unit, vStart, vDeltaShort, &TestPoint, UnitGetCollisionRadius( unit ), 5.0f, dwMoveFlags, &result);
			return (!TESTBIT(&result.flags, MOVERESULT_COLLIDE_UNIT));
		}
		else
		{
			return FALSE;
		}	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sCanMoveTowardsUnit(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget )
		return FALSE;

	// how far away are we?
	VECTOR vDelta = UnitGetPosition( tMoveRequest.pTarget ) - UnitGetPosition( pUnit );
	float fTargetCollisionRadius = AppIsTugboat() ? UnitGetRawCollisionRadius(tMoveRequest.pTarget) : UnitGetCollisionRadius( tMoveRequest.pTarget );
	float fDistance = VectorLength( vDelta ) - fTargetCollisionRadius - UnitGetCollisionRadius( pUnit );

	float fMultiplier = AppIsTugboat() ? 1.0f : 2.0f;
	// am I really close
	if ( fDistance < fMultiplier*tMoveRequest.fStopDistance )
	{
		return FALSE;
	}

	VectorNormalize( vDirection, vDelta );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitWallFollowTowardsLocation( 
	GAME * pGame, 
	UNIT * pUnit, 
	VECTOR & vDirection, 
	VECTOR & vDeltaOut, 
	float fDistance )
{
	VECTOR vCollisionNormal;
	VECTOR vTempPosition = UnitGetPosition( pUnit );
	vTempPosition.fZ += AppIsTugboat() ? 0.5f : UnitGetCollisionHeight( pUnit ) / 2.0f;
	float fUnitRadius = UnitGetCollisionRadius( pUnit );
	LEVEL * pLevel = UnitGetLevel( pUnit );
	float fDistanceToCollide = LevelLineCollideLen(pGame, pLevel, vTempPosition, vDirection, 
		fDistance + fUnitRadius, &vCollisionNormal);
	if ( fDistanceToCollide < fUnitRadius )
		return FALSE;
	VECTOR vDirectionNew;
	if ( fDistanceToCollide - fUnitRadius > fDistance / 2.0f )
	{
		vDirectionNew = vDirection;
		VectorScale( vDeltaOut, vDirection, fDistanceToCollide - fUnitRadius );
	} else {
		VECTOR vUp( 0.0f, 0.0f, 1.0f );
		VectorCross( vDirectionNew, vUp, vCollisionNormal );
		if ( VectorDot( vDirectionNew, vDirection ) < 0 )
			VectorScale( vDirectionNew, vDirectionNew, -1.0f );
		VectorScale( vDeltaOut, vDirectionNew, fDistance );
	}

	return CanMoveInDirection( pGame, pUnit, vDirectionNew );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanCircleTarget(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	VECTOR vUp( 0.0f, 0.0f, 1.0f );
	VECTOR vDirectionToTarget = tMoveRequest.vTarget - UnitGetPosition( pUnit );
	VectorNormalize( vDirectionToTarget, vDirectionToTarget );
	VectorCross( vDirection, vUp, vDirectionToTarget );

	int nCircleDirection = UnitGetStat( pUnit, STATS_AI_CIRCLE_DIRECTION );
	if ( nCircleDirection )
		vDirection *= -1.0f;
	ASSERT( nCircleDirection == 0 || nCircleDirection == 1 );

	VECTOR vDelta;
	if ( ! sUnitWallFollowTowardsLocation( pGame, pUnit, vDirection, vDelta, tMoveRequest.fDistance ) )
	{
		vDirection *= -1.0f;
		if ( ! sUnitWallFollowTowardsLocation( pGame, pUnit, vDirection, vDelta, tMoveRequest.fDistance ) )
			return FALSE;
		UnitSetStat( pUnit, STATS_AI_CIRCLE_DIRECTION, nCircleDirection ? 0 : 1 );
	} else {
		UnitSetStat( pUnit, STATS_AI_CIRCLE_DIRECTION, nCircleDirection ? 1 : 0 );
	}
	VectorNormalize( vDirection, vDelta );
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;
	tMoveRequest.ePathType = PATH_WANDER;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveAwayFrom( 
	GAME * pGame, 
	UNIT * pUnit, 
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	vDirection = UnitGetPosition( pUnit ) - tMoveRequest.vTarget;
	vDirection.fZ = 0.0f;
	// for now just pick a direction if you are on top of the target 
	if ( VectorIsZero( vDirection ) )
		vDirection.fX = 1.0f;

	VectorNormalize( vDirection, vDirection );
	if ( tMoveRequest.dwFlags & MOVE_REQUEST_FLAG_RUN_AWAY_ON_ANGLE )
	{
		VECTOR vSide;
		VectorCross( vSide, vDirection, UnitGetUpDirection( pUnit ) );
		VectorNormalize( vSide, vSide );
		if (RandSelect(pGame, 1, 2) == 0)
		{
			vSide = -vSide;
		}
		vDirection += vSide;
		VectorNormalize( vDirection, vDirection );
	}

	VECTOR vDelta;
	ASSERT( tMoveRequest.fDistance != 0.0f );
	if ( ! sUnitWallFollowTowardsLocation( pGame, pUnit, vDirection, vDelta, tMoveRequest.fDistance ) )
		return FALSE;

	VectorNormalize( vDirection, vDelta );

	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;
	tMoveRequest.ePathType = PATH_WANDER;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveAwayFromFly( 
	GAME * pGame, 
	UNIT * pUnit, 
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	VECTOR vPosition = UnitGetPosition( pUnit );
	vDirection = vPosition - tMoveRequest.vTarget;
	// for now just pick a direction if you are on top of the target 
	if ( VectorIsZero( vDirection ) )
		vDirection.fX = 1.0f;

	VectorNormalize( vDirection, vDirection );
	if ( tMoveRequest.dwFlags & MOVE_REQUEST_FLAG_RUN_AWAY_ON_ANGLE )
	{
		VECTOR vSide;
		VectorCross( vSide, vDirection, UnitGetUpDirection( pUnit ) );
		VectorNormalize( vSide, vSide );
		if (RandSelect(pGame, 1, 2) == 0)
		{
			vSide = - vSide;
		}
		vDirection += vSide;
		VectorNormalize( vDirection, vDirection );
	}

	VECTOR vDelta;
	ASSERT( tMoveRequest.fDistance != 0.0f );
	if ( ! sUnitWallFollowTowardsLocation( pGame, pUnit, vDirection, vDelta, tMoveRequest.fDistance ) )
		return FALSE;

	VectorNormalize( vDirection, vDelta );

	tMoveRequest.vTarget = vPosition + vDelta;

	float fAltitude = AIGetAltitude(pGame, pUnit);
	float fFloor = vPosition.fZ - fAltitude;

	if (tMoveRequest.vTarget.fZ < tMoveRequest.fMinAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMinAltitude + fFloor;
	}

	if (tMoveRequest.vTarget.fZ > tMoveRequest.fMaxAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMaxAltitude + fFloor;
	}

	vDelta = tMoveRequest.vTarget - vPosition;
	VectorNormalize(vDirection, vDelta);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveRandomWander(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	// if you are already moving, then keep going in that direction
	if ( UnitTestMode( pUnit, tMoveRequest.eMode ) )
	{
		vDirection = UnitGetMoveDirection( pUnit );
	} else {
		VECTOR vSpawnPoint = UnitGetStatVector( pUnit, STATS_AI_ANCHOR_POINT_X, 0 );
		if ( VectorIsZero( vSpawnPoint ) )
		{
			vDirection.fX = RandGetFloat(pGame, -1.0f, 1.0f);
			vDirection.fY = RandGetFloat(pGame, -1.0f, 1.0f);
			vDirection.fZ = 0.0f;
			VectorNormalize( vDirection, vDirection );
		} else {
			VECTOR vTarget;
			vTarget.fX = vSpawnPoint.fX + RandGetFloat(pGame, -1.0f, 1.0f) * tMoveRequest.fWanderRange;
			vTarget.fY = vSpawnPoint.fY + RandGetFloat(pGame, -1.0f, 1.0f) * tMoveRequest.fWanderRange;
			vTarget.fZ = vSpawnPoint.fZ;

			vDirection = vTarget - UnitGetPosition( pUnit );
			VectorNormalize( vDirection, vDirection );
		}
	}

	ASSERT( tMoveRequest.fDistance != 0.0f );
	VectorScale( tMoveRequest.vTarget, vDirection, tMoveRequest.fDistance );
	tMoveRequest.vTarget += UnitGetPosition( pUnit );
	tMoveRequest.ePathType = PATH_WANDER;

	if(!UnitTestFlag(pUnit, UNITFLAG_PATHING))
	{
		// check to see if I can take a step in that direction at all
		return CanMoveInDirection( pGame, pUnit, vDirection );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveStrafe(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget )
		return FALSE;

	BOOL bRight = tMoveRequest.eMode == MODE_STRAFE_RIGHT;

	VECTOR vTargetDirection = UnitGetPosition( tMoveRequest.pTarget ) - UnitGetPosition( pUnit );
	VectorNormalize( vTargetDirection, vTargetDirection );

	VectorCross( vDirection, vTargetDirection, pUnit->vUpDirection );
	if ( bRight )
		vDirection = - vDirection;

	if ( ! CanMoveInDirection( pGame, pUnit, vDirection ) )
		return FALSE;

	VECTOR vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;
	//tMoveRequest.ePathType = PATH_WANDER;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveBackup(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget )
		return FALSE;

	VECTOR vTargetDirection = UnitGetPosition( tMoveRequest.pTarget ) - UnitGetPosition( pUnit );
	VectorNormalize( vTargetDirection, vTargetDirection );

	// Pick a direction away from the target
	vDirection = - vTargetDirection;
	if ( ! CanMoveInDirection( pGame, pUnit, vDirection ) )
	{
		tMoveRequest.eMode = MODE_WALK;
		return sCanCircleTarget(pGame, pUnit, tMoveRequest, vDirection);
	}

	VECTOR vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;
	tMoveRequest.ePathType = PATH_WANDER;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveBackupFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget )
		return FALSE;

	VECTOR vPosition = UnitGetPosition( pUnit );
	VECTOR vRandom = sAiGetRandomDirection(pGame, pUnit);
	vRandom.fZ = 0.0f;
	VECTOR vTargetDirection = UnitGetPosition( tMoveRequest.pTarget ) - vPosition;
	VectorNormalize( vTargetDirection, vTargetDirection );
	vTargetDirection += vRandom;
	VectorNormalize( vTargetDirection, vTargetDirection );

	// Pick a direction away from the target
	vDirection = - vTargetDirection;
	if ( ! CanMoveInDirection( pGame, pUnit, vDirection ) )
	{
		tMoveRequest.eMode = MODE_WALK;
		return sCanCircleTarget(pGame, pUnit, tMoveRequest, vDirection);
	}

	VECTOR vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = vPosition + vDelta;

	// Check min/max height
	float fAltitude = AIGetAltitude(pGame, pUnit);
	float fFloor = vPosition.fZ - fAltitude;
	if (tMoveRequest.vTarget.fZ > tMoveRequest.fMaxAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMaxAltitude + fFloor;
	}
	if (tMoveRequest.vTarget.fZ < tMoveRequest.fMinAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMinAltitude + fFloor;
	}

	vDelta = tMoveRequest.vTarget - vPosition;
	VectorNormalize(vDirection, vDelta);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveStrafeFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget )
		return FALSE;

	BOOL bRight = tMoveRequest.eMode == MODE_STRAFE_RIGHT;

	VECTOR vPosition = UnitGetPosition( pUnit );
	VECTOR vTargetDirection = UnitGetPosition( tMoveRequest.pTarget ) - vPosition;
	VectorNormalize( vTargetDirection, vTargetDirection );

	VectorCross( vDirection, vTargetDirection, pUnit->vUpDirection );
	if ( bRight )
		vDirection = - vDirection;

	if ( ! CanMoveInDirection( pGame, pUnit, vDirection ) )
	{
		return FALSE;
	}

	VECTOR vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = vPosition + vDelta;

	// Check min/max height
	float fAltitude = AIGetAltitude(pGame, pUnit);
	float fFloor = vPosition.fZ - fAltitude;
	if (tMoveRequest.vTarget.fZ > tMoveRequest.fMaxAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMaxAltitude + fFloor;
	}
	if (tMoveRequest.vTarget.fZ < tMoveRequest.fMinAltitude + fFloor)
	{
		tMoveRequest.vTarget.fZ = tMoveRequest.fMinAltitude + fFloor;
	}

	vDelta = tMoveRequest.vTarget - vPosition;
	VectorNormalize(vDirection, vDelta);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveLocationCommon(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	VECTOR vDelta;
	ASSERT( tMoveRequest.fDistance != 0.0f );
	VECTOR vTempPosition = UnitGetPosition( pUnit );
	vTempPosition.fZ += UnitGetCollisionHeight( pUnit ) / 2.0f;
	float fCollisionRadius = UnitGetCollisionRadius( pUnit );
	VECTOR vCollisionNormal;
	LEVEL * pLevel = UnitGetLevel( pUnit );
	float fDistanceToCollide = LevelLineCollideLen(pGame, pLevel, vTempPosition, vDirection, 
		tMoveRequest.fDistance + fCollisionRadius, &vCollisionNormal);
	if ( fDistanceToCollide < ( fCollisionRadius + 0.1f ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveLocation(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	// Whether or not a pathing unit can move to a location is determined by
	// whether or not it can path to that location, not by some arbitrary raycast
	if(UnitTestFlag(pUnit, UNITFLAG_PATHING))
	{
		if(!RoomGetFromPosition(pGame, UnitGetRoom(pUnit), &tMoveRequest.vTarget))
		{
			ROOM * pDestinationRoom = tMoveRequest.pTarget ? UnitGetRoom(tMoveRequest.pTarget) : UnitGetRoom(pUnit);
			if(!pDestinationRoom)
			{
				return FALSE;
			}
			// Adjust the location by placing it at a pathnode.
			DWORD dwFreePathNodeFlags = 0;
			SETBIT(dwFreePathNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
			ROOM_PATH_NODE * pDestinationNode = RoomGetNearestFreePathNode(pGame, UnitGetLevel(pUnit), tMoveRequest.vTarget, &pDestinationRoom, pUnit, 0.0f, 0.0f, pDestinationRoom, dwFreePathNodeFlags);
			if(!pDestinationRoom || !pDestinationNode)
			{
				return FALSE;
			}
			tMoveRequest.vTarget = RoomPathNodeGetExactWorldPosition(pGame, pDestinationRoom, pDestinationNode);
		}

		// We need to calculate the direction either way
		VectorSubtract( vDirection, tMoveRequest.vTarget, UnitGetPosition( pUnit ) );
		vDirection.fZ = 0.0f;
		return TRUE;
	}

	// We need to calculate the direction either way
	VectorSubtract( vDirection, tMoveRequest.vTarget, UnitGetPosition( pUnit ) );
	vDirection.fZ = 0.0f;

	// if no direction that's bad
	if ( vDirection.fX == 0.0f && vDirection.fY == 0.0f && vDirection.fZ == 0.0f )
		return FALSE;
	VectorNormalize( vDirection, vDirection );

	return sCanMoveLocationCommon(pGame, pUnit, tMoveRequest, vDirection);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveLocationFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	vDirection = tMoveRequest.vTarget - UnitGetPosition( pUnit );
	// if no direction that's bad
	if ( VectorIsZero( vDirection ) )
		return FALSE;
	VectorNormalize( vDirection );

	return sCanMoveLocationCommon(pGame, pUnit, tMoveRequest, vDirection);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sAICanFlyTowards( 
	GAME * pGame, 
	UNIT * pUnit, 
	const VECTOR & vDirection,
	float fDistance )
{
	float fMinDist = fDistance;
	VECTOR vTestPos = UnitGetPosition( pUnit );
	// test from bottom, middle and top of pUnit
	float fCollisionRadius = UnitGetCollisionRadius( pUnit );
	float fCollisionHeight = UnitGetCollisionHeight( pUnit );
	LEVEL * pLevel = UnitGetLevel( pUnit );
	for ( int i = 0; i < 3; i++ )
	{
		float fDistToCollide = LevelLineCollideLen( pGame, pLevel, vTestPos, 
			vDirection, fDistance + fCollisionRadius);

		if ( fDistToCollide - fCollisionRadius < fMinDist )
			fMinDist = fDistToCollide - fCollisionRadius;

		vTestPos.fZ += fCollisionHeight / 2.0f;
	}

	return fMinDist;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveRandomWanderFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( tMoveRequest.fDistance == 0.0f )
		return FALSE;

	// check our altitude
	float fAltitude = AIGetAltitude( pGame, pUnit );
	float fDeltaZ = 0.0f;
	if ( fAltitude > tMoveRequest.fMaxAltitude )
		fDeltaZ = tMoveRequest.fMaxAltitude - fAltitude;
	else if ( fAltitude < tMoveRequest.fMinAltitude )
		fDeltaZ = tMoveRequest.fMinAltitude - fAltitude;

	// default to going in our current direction
	if ( tMoveRequest.pTarget || ! VectorIsZero( tMoveRequest.vTarget ) )
	{
		VECTOR vUnitPos = UnitGetPosition( pUnit );
		VECTOR vCenter;
		if ( tMoveRequest.pTarget )
		{
			vCenter = UnitGetPosition( tMoveRequest.pTarget );
			vCenter.fZ = vUnitPos.fZ;
		} else {
			vCenter = tMoveRequest.vTarget;
		}
		VECTOR vToCenter = vCenter - vUnitPos;
		VectorNormalize( vToCenter, vToCenter );
		VECTOR vUp( 0, 0, 1 );
		if (RandSelect(pGame, 1, 2))
			vUp.fZ = -1.0f;
		VECTOR vSide;
		VectorCross( vSide, vToCenter, vUp );

		VECTOR vDelta = vSide;
		vDelta *= tMoveRequest.fWanderRange;
		VECTOR vTarget = vCenter + vDelta;

		vDelta = vTarget - vUnitPos;
		VectorNormalize( vDirection, vDelta );

	} else {
		vDirection = UnitGetMoveDirection( pUnit );
	}

	VECTOR vPosition = UnitGetPosition( pUnit );
	VECTOR vTarget;
	{
		VECTOR vDelta = vDirection;
		vDelta *= tMoveRequest.fDistance;
		vTarget = vPosition + vDelta;
	}

	// adjust altitude
	BOOL bForceRandom = FALSE;
	if ( fDeltaZ != 0.0 )
	{
		vTarget.fZ = vPosition.fZ + fDeltaZ;
		vDirection = vTarget - vPosition;
		VectorNormalize( vDirection, vDirection );
	} else if (fabsf( vDirection.fZ ) > 0.8f || VectorIsZero(vDirection) || RandSelect(pGame, 1, 4))
	{
		bForceRandom = TRUE;
	}

	// try default and then variations on a random direction
	float fDistanceCanMove = bForceRandom ? 0.0f : sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
	if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
	{
		vDirection = sAiGetRandomDirection(pGame, pUnit);
		{
			VECTOR vDelta = vDirection;
			vDelta *= tMoveRequest.fDistance;
			vTarget = vPosition + vDelta;
			if ( fDeltaZ != 0.0 ) 
			{
				vTarget.fZ = vPosition.fZ + fDeltaZ;
				vDirection = vTarget - vPosition;
				VectorNormalize( vDirection, vDirection );
			}
		}

		fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
		if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
		{
			vDirection.fX = -vDirection.fX; // -X, +Y
			fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
			if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
			{
				vDirection.fX = -vDirection.fX; // +X
				vDirection.fY = -vDirection.fY; // -Y
				fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
				if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
				{
					vDirection.fX = -vDirection.fX; // -X, -Y
					fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
					if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
					{
						return FALSE;
					}
				}
			}
		}
	}
	tMoveRequest.fDistance = fDistanceCanMove;

	VECTOR vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveTowardsUnitFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget || ! pUnit )
		return FALSE;

	// how far away are we?
	VECTOR vDelta = UnitGetPosition( tMoveRequest.pTarget ) - UnitGetPosition( pUnit );
	float fDistance = VectorLength( vDelta ) - UnitGetCollisionRadius( tMoveRequest.pTarget ) - UnitGetCollisionRadius( pUnit );

	// am I really close
	if ( fDistance < tMoveRequest.fStopDistance )
		return FALSE;

	VECTOR vPosition = UnitGetPosition( pUnit );
	VECTOR vTarget	 = UnitGetPosition( tMoveRequest.pTarget );
	vTarget.fZ = RandGetFloat(pGame, tMoveRequest.fMinAltitude, tMoveRequest.fMaxAltitude);
	vDelta = vTarget - vPosition;
	if ( VectorIsZero( vDelta ) )
		return FALSE; // way too close
	vDirection = vDelta;
	VectorNormalize( vDirection, vDirection );

	float fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
	if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
		return FALSE; // can't fly there

	tMoveRequest.fDistance = fDistanceCanMove;

	vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveAboveUnitFly(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	if ( ! tMoveRequest.pTarget || ! pUnit )
		return FALSE;
	VECTOR vPosition = UnitGetPosition( pUnit );
	VECTOR vTarget	 = UnitGetPosition( tMoveRequest.pTarget );
	vTarget.fZ = vPosition.fZ;
	VECTOR vDelta = vTarget - vPosition;
	if ( VectorIsZero( vDelta ) )
		return FALSE; // way too close
	vDirection = vDelta;
	VectorNormalize( vDirection, vDirection );

	//if ( fDistance < tMoveRequest.fDistanceMin )
	//	return FALSE; // too close

	float fDistanceCanMove = sAICanFlyTowards( pGame, pUnit, vDirection, tMoveRequest.fDistance );
	if ( fDistanceCanMove < tMoveRequest.fDistanceMin )
		return FALSE; // can't fly there

	tMoveRequest.fDistance = fDistanceCanMove;

	vDelta = vDirection;
	vDelta *= tMoveRequest.fDistance;
	tMoveRequest.vTarget = UnitGetPosition( pUnit ) + vDelta;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Note: for this test, it just uses the unit's facing direction, modified by
// mode
static BOOL sCanMoveFaceDirection(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	VECTOR vCheckDirection = UnitGetFaceDirection(pUnit, FALSE);
	switch(tMoveRequest.eMode)
	{
	case MODE_WALK:
	case MODE_RUN:
	    {
			vCheckDirection = UnitGetFaceDirection(pUnit, FALSE);
	    }
		break;
	case MODE_BACKUP:
	    {
			vCheckDirection = -UnitGetFaceDirection(pUnit, FALSE);
	    }
		break;
	case MODE_STRAFE_LEFT:
		{
			VectorCross(vCheckDirection, UnitGetFaceDirection(pUnit, FALSE), UnitGetUpDirection(pUnit));
		}
		break;
	case MODE_STRAFE_RIGHT:
		{
			VectorCross(vCheckDirection, UnitGetUpDirection(pUnit), UnitGetFaceDirection(pUnit, FALSE));
		}
		break;
	}
	vDirection = tMoveRequest.vTarget - UnitGetPosition(pUnit);
	VectorNormalize( vDirection );

	VECTOR vTempPosition = UnitGetPosition( pUnit );
	vTempPosition.fZ += UnitGetCollisionHeight( pUnit ) / 2.0f;

	LEVEL * pLevel = UnitGetLevel( pUnit );

	float fRadius = UnitGetCollisionRadius( pUnit );
	VECTOR vCollisionNormal;
	float fDistanceToCollide = LevelLineCollideLen(pGame, pLevel, vTempPosition, vCheckDirection, 
		fRadius * 2.0f, &vCollisionNormal);

	if ( fDistanceToCollide < ( fRadius + 0.1f ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanMoveWarp(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	VECTOR & vDirection )
{
	ROOM * pDestinationRoom = RoomGetFromPosition(pGame, UnitGetLevel(pUnit), &tMoveRequest.vTarget);
	if ( ! pDestinationRoom )
		return FALSE;
	if(IS_SERVER(pGame))
	{
		UNIT * pUltimateOwner = UnitGetUltimateOwner(pUnit);
		if(pUltimateOwner && UnitHasClient(pUltimateOwner))
		{
			GAMECLIENT * pClient = UnitGetClient(pUltimateOwner);
			if(pClient && !ClientIsRoomKnown(pClient, pDestinationRoom, TRUE))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AICopyPathToMessageBuffer(
	GAME* pGame,
	UNIT* pUnit,
	BYTE* pBuffer,
	BOOL bEmptyPath)
{
	ZeroMemory( pBuffer, MAX_PATH_BUFFER );
	BIT_BUF tBuffer( pBuffer, MAX_PATH_BUFFER );

	if (UnitTestFlag(pUnit, UNITFLAG_PATHING) && pUnit->m_pPathing)
	{
		if( AppIsTugboat() )
		{
			CopyPathToSpline( pGame, pUnit, UnitGetMoveTarget( pUnit ), TRUE );
		}
		struct PATH_STATE * pPathing = pUnit->m_pPathing;
		if(bEmptyPath)
		{
			pPathing = NULL;
		}
		PathStructSerialize(pGame, pPathing, tBuffer);
	}

	return tBuffer.GetLen();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef BOOL AI_MOVE_FUNC( GAME * pGame, UNIT * pUnit, MOVE_REQUEST_DATA & tMoveRequest, VECTOR & vDirection );
enum
{
	MOVE_TOWARDS_UNIT,
	MOVE_TOWARDS_XYZ,
	MOVE_TOWARDS_WARP,
};
enum
{
	MOVE_FACE_TARGET,
	MOVE_FACE_DIRECTION,
	MOVE_FACE_AWAY,
	MOVE_FACE_NONE,
};
struct MOVE_TYPE_CHART
{
	int		nMoveTowards;
	int		nTurnTowards;
	AI_MOVE_FUNC * pfnMove;
};
static const MOVE_TYPE_CHART sgtMoveTypeChart[] = 
{//		move				turn					function
	{	MOVE_TOWARDS_UNIT,	MOVE_FACE_NONE,			sCanMoveTowardsUnit		},	//	MOVE_TYPE_TO_UNIT,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_DIRECTION,	sCanMoveTowardsUnitFly	},	//	MOVE_TYPE_TO_UNIT_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_DIRECTION,	sCanMoveAboveUnitFly	},	//	MOVE_TYPE_ABOVE_UNIT_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanCircleTarget		},	//	MOVE_TYPE_CIRCLE,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveAwayFrom		},	//	MOVE_TYPE_AWAY_FROM_UNIT,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_DIRECTION,	sCanMoveAwayFromFly		},	//	MOVE_TYPE_AWAY_FROM_UNIT_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveRandomWander	},	//	MOVE_TYPE_RANDOM_WANDER,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_DIRECTION,	sCanMoveRandomWanderFly },	//	MOVE_TYPE_RANDOM_WANDER_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveStrafe			},	//	MOVE_TYPE_STRAFE,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveStrafeFly		},	//	MOVE_TYPE_STRAFE_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveBackup			},	//	MOVE_TYPE_BACKUP,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_AWAY,			sCanMoveBackupFly		},	//	MOVE_TYPE_BACKUP_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveLocation		},	//  MOVE_TYPE_LOCATION,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_DIRECTION,	sCanMoveLocationFly		},	//  MOVE_TYPE_LOCATION_FLY,
	{	MOVE_TOWARDS_XYZ,	MOVE_FACE_NONE,			sCanMoveFaceDirection	},	//  MOVE_TYPE_FACE_DIRECTION,
	{	MOVE_TOWARDS_WARP,	MOVE_FACE_DIRECTION,	sCanMoveWarp			},	//  MOVE_TYPE_WARP,
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_MoveRequest(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	const char *pszLabel )
{

	UNITID idMoveTargetSave = UnitGetMoveTargetId( pUnit );
	VECTOR vMoveTargetSave;
	if (UnitGetMoveTarget( pUnit ))
	{
		vMoveTargetSave = *UnitGetMoveTarget( pUnit );
	}
	const VECTOR vMoveDirectionSave = UnitGetMoveDirection( pUnit );

#ifdef TRACE_SYNCH
	static int snMoveRequest = 0;	
	snMoveRequest++;

	{
		trace( 
			"AI_MoveRequest #%d for UNITID:%d - '%s' targetv=(%.2f, %.2f, %.2f), targetunit=%d\n", 
			snMoveRequest,
			UnitGetId( pUnit ), 
			pszLabel ? pszLabel : "UNKNOWN",
			tMoveRequest.vTarget.fX,
			tMoveRequest.vTarget.fY,
			tMoveRequest.vTarget.fZ,
			tMoveRequest.pTarget ? UnitGetId( tMoveRequest.pTarget ) : INVALID_ID );
	}
#endif
	
	// first test to see if we can do this, and what direction we will go
	VECTOR vDirection;
	if ( VectorIsZero( tMoveRequest.vTarget ) && tMoveRequest.pTarget )
	{
		tMoveRequest.vTarget = UnitGetPosition( tMoveRequest.pTarget );
	}

	ASSERT_RETFALSE( tMoveRequest.eType >= 0 && tMoveRequest.eType < sizeof( sgtMoveTypeChart ) / sizeof( MOVE_TYPE_CHART ) );

	// altitude stat is only used when flying and pathing
	if ( TESTBIT( pUnit->pdwMovementFlags, MOVEFLAG_FLY ) &&
		 UnitTestFlag(pUnit, UNITFLAG_PATHING) && !UnitTestFlag(pUnit, UNITFLAG_TEMPORARILY_DISABLE_PATHING) && pUnit->m_pPathing )
		UnitSetStatFloat( pUnit, STATS_ALTITUDE, 0, RandGetFloat(pGame, tMoveRequest.fMinAltitude, tMoveRequest.fMaxAltitude ) );

	// save stop distance, then set for canmove functions
	// *** NOTE: YOU MUST RESET THIS DISTANCE IF YOU FAIL OUT OF THIS FUNCTION!! ***
	float fPreviousStopDistance = UnitGetStatFloat( pUnit, STATS_STOP_DISTANCE );
	UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, tMoveRequest.fStopDistance );

	const MOVE_TYPE_CHART * pMoveTypeChart = & sgtMoveTypeChart[ tMoveRequest.eType ];
	if ( ! pMoveTypeChart->pfnMove( pGame, pUnit, tMoveRequest, vDirection ) )
	{
		UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, fPreviousStopDistance );
		goto failed;
	}

	int nModeVelocityOld = UnitGetBaseStat( pUnit, STATS_VELOCITY_BONUS );
	//UnitSetStat( pUnit, STATS_VELOCITY_BONUS, tMoveRequest.nVelocityPercent );

	if(!AppIsTugboat())
	{
		if(!(tMoveRequest.dwFlags & MOVE_REQUEST_FLAG_DONT_TURN))
		{
			BOOL bImmediate = (pMoveTypeChart->nMoveTowards == MOVE_TOWARDS_WARP) ? TRUE : FALSE;
			switch(pMoveTypeChart->nTurnTowards)
			{
			case MOVE_FACE_TARGET:
				{
					AI_TurnTowardsTargetId( pGame, pUnit, tMoveRequest.pTarget, bImmediate );
				}
				break;
			case MOVE_FACE_AWAY:
				{
					AI_TurnTowardsTargetId( pGame, pUnit, tMoveRequest.pTarget, bImmediate );
				}
				break;
			case MOVE_FACE_DIRECTION:
				{
					AI_TurnTowardsTargetXYZ( pGame, pUnit, tMoveRequest.vTarget, UnitGetUpDirection(pUnit), bImmediate );
				}
				break;
			case MOVE_FACE_NONE:
			default:
				{
				}
				break;
			}
		}
	}

	// These are required for UnitCalculatePath
	BOOL bCalculatePath = TRUE;
	BOOL bSetMode = TRUE;
	BOOL bUseExistingPath = FALSE;
	UNITMODE eLastModeSet = (UNITMODE)UnitGetStat(pUnit, STATS_AI_MODE);
	UNITID idTarget = tMoveRequest.pTarget ? UnitGetId( tMoveRequest.pTarget ) : INVALID_ID;
	switch(pMoveTypeChart->nMoveTowards)
	{
	case MOVE_TOWARDS_UNIT:
	    {
			if ( ( UnitGetMoveTargetId(pUnit) == idTarget ) && UnitPathIsActive( pUnit ) )
			{
				drbSuperTracker( pUnit );
				bCalculatePath = FALSE;
				if(tMoveRequest.eMode == eLastModeSet)
				{
					bSetMode = FALSE;
				}
				bUseExistingPath = TRUE;
			}
			else
			{
				drbSuperTracker( pUnit );
				UnitSetMoveTarget(pUnit, idTarget);
			}
		}
		break;
	case MOVE_TOWARDS_XYZ:
		{
			UnitSetMoveTarget( pUnit, tMoveRequest.vTarget, vDirection );
		}
		break;
	case MOVE_TOWARDS_WARP:
		{
			bCalculatePath = FALSE;
			bSetMode = FALSE;
		}
		break;
	default:
	    {
			// What?  Nah.  Still, just to be safe.
			bCalculatePath = FALSE;
			bSetMode = FALSE;
		}
		break;
	}

	if(bCalculatePath)
	{
		DWORD dwFlags = 0;
		if(UnitGetMoveTargetId(pUnit) == INVALID_ID)
		{
			AI_FindPathNode(pGame, pUnit, tMoveRequest.vTarget);
			SETBIT(dwFlags, PCF_ALREADY_FOUND_PATHNODE_BIT);
		}
		if(tMoveRequest.dwFlags & MOVE_REQUEST_FLAG_CHEAP_LOOKUPS_ONLY)
		{
			SETBIT(dwFlags, PCF_DISALLOW_EXPENSIVE_PATHNODE_LOOKUPS_BIT);
		}
		if(tMoveRequest.dwFlags & MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES)
		{
			SETBIT(dwFlags, PCF_USE_HAPPY_PLACES_BIT);
		}
		// KCK: Added this for Hellgate to allow AIs to move towards the player even if no path exists (generally
		// this will be due to a player blocker like a bone wall). Ideally I'd like to see a system which returns
		// what it is that's blocking our path, but that's something for the future.
		// Travis says this is bad-bad-bad for Mythos, so keeping it Hellgate only.
		if( AppIsHellgate() )
		{
			SETBIT(dwFlags, PCF_ACCEPT_PARTIAL_SUCCESS_BIT);
		}
		TIMER_STARTEX("Pathfinding", 2);
		drbSuperTracker( pUnit );
		if(IS_SERVER(pGame))
		{
			if (!UnitCalculatePath(pGame, pUnit, tMoveRequest.ePathType, dwFlags))
			{
				UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, fPreviousStopDistance );
				UnitSetStat( pUnit, STATS_VELOCITY_BONUS, nModeVelocityOld );
				goto failed;
			}
		}
		else
		{
			if (!ClientCalculatePath(pGame, pUnit, tMoveRequest.eMode, tMoveRequest.ePathType, dwFlags))
			{
				UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, fPreviousStopDistance );
				UnitSetStat( pUnit, STATS_VELOCITY_BONUS, nModeVelocityOld );
				goto failed;
			}
		}
	}

	BOOL bSetModeSucceeded = FALSE;
	if(bSetMode)
	{
		if( tMoveRequest.eMode == MODE_RUN && !UnitCanRun( pUnit ))
		{
			tMoveRequest.eMode = MODE_WALK;
		}
		if(IS_SERVER(pGame))
		{
			bSetModeSucceeded = s_UnitSetMode( pUnit, tMoveRequest.eMode, 0, 0.0f, 0, FALSE );
		}
		else
		{
			bSetModeSucceeded = c_UnitSetMode( pUnit, tMoveRequest.eMode, 0, 0.0f );
			if(UnitPathIsActiveClient(pUnit) && (tMoveRequest.eMode == MODE_RUN || tMoveRequest.eMode == MODE_WALK))
			{
				bSetModeSucceeded = TRUE;
			}
		}
		if ( ! bSetModeSucceeded )
		{
			UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, fPreviousStopDistance );
			UnitSetStat( pUnit, STATS_VELOCITY_BONUS, nModeVelocityOld );
			goto failed;	
		}
		UnitSetStat(pUnit, STATS_AI_MODE, tMoveRequest.eMode);
	}

	if(IS_SERVER(pGame))
	{
		BYTE bFlags = tMoveRequest.bMoveFlags;
		if(bUseExistingPath)
		{
			SETBIT(&bFlags, UMID_USE_EXISTING_PATH);
		}

		if ( pMoveTypeChart->nMoveTowards == MOVE_TOWARDS_UNIT )
		{		
			s_SendUnitMoveID( pUnit, bFlags, tMoveRequest.eMode, idTarget, vDirection );
		}
		else if ( pMoveTypeChart->nMoveTowards == MOVE_TOWARDS_XYZ )
		{
			s_SendUnitMoveXYZ( pUnit, bFlags, tMoveRequest.eMode, tMoveRequest.vTarget, vDirection, NULL, TRUE );
		}
		else if ( pMoveTypeChart->nMoveTowards == MOVE_TOWARDS_WARP )
		{
			ROOM * pRoom = RoomGetFromPosition(pGame, UnitGetLevel(pUnit), &tMoveRequest.vTarget);
			ASSERT(pRoom);

			VECTOR vPosition = tMoveRequest.vTarget;
			VECTOR vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);
			VECTOR vUpDirection = UnitGetUpDirection(pUnit);
			UnitWarp(pUnit, pRoom, vPosition, vFaceDirection, vUpDirection, 0);

			s_SendUnitWarp( pUnit, pRoom, vPosition, vFaceDirection, vUpDirection, 0 );

		}
		else
		{
			s_SendUnitMoveDirection( pUnit, bFlags, tMoveRequest.eMode, vDirection );
		}
	}

	if(bCalculatePath || bSetMode)
	{
		// this is a fucked up situation... UnitCalcuatePath automatically sets up all the pathing stuff,
		// but then the set mode above removes you from the step list and takes it all away... so, we put you back here
		drbSuperTracker( pUnit );
		UnitStepListAdd(pGame, pUnit);
		UnitActivatePath( pUnit, !bCalculatePath );
	}

	return TRUE;
	
failed:

	if (idMoveTargetSave != INVALID_ID)
	{
		UnitSetMoveTarget( pUnit, idMoveTargetSave );
	}
	else
	{
		UnitSetMoveTarget( pUnit, vMoveTargetSave, vMoveDirectionSave );
	}
	
	return FALSE;
	
}
