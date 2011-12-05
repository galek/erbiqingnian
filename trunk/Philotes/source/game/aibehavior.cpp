//----------------------------------------------------------------------------
// aibehavior.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ai.h"
#include "ai_priv.h"
#include "s_message.h"
#include "states.h"
#include "movement.h"
#include "skills.h"
#include "player.h"
#include "items.h"
#include "quests.h"
#include "level.h"
#include "monsters.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "config.h"
#include "spawnclass.h"
#include "room_layout.h"
#include "room_path.h"
#include "excel_private.h"
#include "picker.h"
#include "gameunits.h"
#include "wardrobe.h"

//----------------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------------
static void sVerifyBehavior(
	const AI_BEHAVIOR_DEFINITION * pBehaviorDef,
	const char * pszName )
{
	AI_BEHAVIOR_DATA * pBehaviorData = (AI_BEHAVIOR_DATA *) ExcelGetData( NULL, DATATABLE_AI_BEHAVIOR, pBehaviorDef->nBehaviorId );
	ASSERT( pBehaviorData );
	if ( pBehaviorData )
	{
		for ( int i = 0; i < AI_BEHAVIOR_NUM_SKILLS; i++ )
		{
			if ( pBehaviorData->pbUsesSkill[ 0 ] )
			{
//				ASSERTX( pBehaviorDef->nSkillId != INVALID_ID, pszName );
			}
			if ( pBehaviorData->pbUsesSkill[ 1 ] )
			{
				ASSERTX( pBehaviorDef->nSkillId2 != INVALID_ID, pszName );
			}
			if ( pBehaviorData->bUsesState )
			{
				ASSERTX( pBehaviorDef->nStateId != INVALID_ID, pszName );
			}
			if ( pBehaviorData->bUsesStat )
			{
				ASSERTX( pBehaviorDef->nStatId != INVALID_ID, pszName );
			}
			if ( pBehaviorData->bUsesMonster )
			{
				ASSERTX( pBehaviorDef->nMonsterId != INVALID_ID, pszName );
			}
			if ( pBehaviorData->bUsesSound )
			{
				ASSERTX( pBehaviorDef->nSoundId != INVALID_ID, pszName );
			}
			if ( pBehaviorData->bUsesString )
			{
				ASSERTX( pBehaviorDef->pszString[ 0 ] != 0, pszName );
			}
		}
	}

	for ( int i = 0; i < pBehaviorDef->tTable.nBehaviorCount; i++ )
	{
		sVerifyBehavior( &pBehaviorDef->tTable.pBehaviors[ i ], pszName );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL AIBehaviorPostProcess(
	void * pDef,
	BOOL bForceSyncLoad)
{
	if ( ! pDef )
		return FALSE;

#if ISVERSION( DEVELOPMENT )
	AI_DEFINITION * pAIDef = (AI_DEFINITION *) pDef;

	for ( int i = 0; i < pAIDef->tTable.nBehaviorCount; i++ )
	{
		sVerifyBehavior( &pAIDef->tTable.pBehaviors[ i ], pAIDef->tHeader.pszName );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline float sBehaviorGetFloat(
	GAME * pGame, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	int nIndex,
	BOOL bCheckZero,
	const char * pszError )
{
	float fVal = pBehavior->pDefinition->pfParams[ nIndex ];
	if ( bCheckZero )
	{
		ASSERTX( fVal != 0.0f, pszError );
	}
	return fVal;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAME_TICK sAITimerGet(
	 UNIT * pUnit,
	 AI_BEHAVIOR_INSTANCE * pBehavior )
{
	AI_BEHAVIOR_DATA * pBehaviorData = (AI_BEHAVIOR_DATA *) ExcelGetData( NULL, DATATABLE_AI_BEHAVIOR, pBehavior->pDefinition->nBehaviorId );
	ASSERT_RETZERO( pBehaviorData );
	return UnitGetStat( pUnit, STATS_AI_TIMER, (WORD) pBehaviorData->nTimerIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAITimerSet(
	 UNIT * pUnit,
	 AI_BEHAVIOR_INSTANCE * pBehavior,
	 int nDelta )
{
	AI_BEHAVIOR_DATA * pBehaviorData = (AI_BEHAVIOR_DATA *) ExcelGetData( NULL, DATATABLE_AI_BEHAVIOR, pBehavior->pDefinition->nBehaviorId );
	ASSERT_RETURN( pBehaviorData );

	GAME * pGame = UnitGetGame( pUnit );
	UnitSetStat( pUnit, STATS_AI_TIMER, (WORD) pBehaviorData->nTimerIndex, GameGetTick( pGame ) + nDelta );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sGetTargetForSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	int nSkill,
	AI_CONTEXT & tContext,
	BOOL bWarp = FALSE,
	BOOL bUseSkillRange = FALSE, 
	BOOL bSearch = TRUE,
	const VECTOR * pvLocation = NULL )
{
	// if we have an immobile ai, make sure they use skill range - they can't reach anything else!
	if( AppIsTugboat() )
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );

		if( pUnitData->tVelocities[ MODE_VELOCITY_WALK ].fVelocityBase == 0 && 
			pUnitData->tVelocities[ MODE_VELOCITY_RUN ].fVelocityBase == 0 )
		{
			bUseSkillRange = TRUE;
		}		
	}

	if(!bUseSkillRange && !bWarp)
	{
		for ( int i = 0; i < tContext.nTargetCount; i++ )
		{
			if ( tContext.pTargets[ i ].nSkill == nSkill )
				return tContext.pTargets[ i ].pUnit;
		}
	}
	if ( ! bSearch )
		return NULL;

	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, FALSE );
	BOOL bForceMelee = FALSE;
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		int nWeaponSkill = ItemGetPrimarySkill( pWeaponArray[ i ] );
		if ( nWeaponSkill != INVALID_ID )
		{
			const SKILL_DATA * pWeaponSkill = SkillGetData( pGame, nWeaponSkill );
			if ( SkillDataTestFlag( pWeaponSkill, SKILL_FLAG_IS_MELEE ) )
				bForceMelee = TRUE;;
		}
	}

	DWORD pdwSearchFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
	ZeroMemory( pdwSearchFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
	SETBIT( pdwSearchFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	if ( bForceMelee )
	{
		SETBIT( pdwSearchFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
	}

	DWORD dwAIGetTargetFlags = 0;
	SETBIT(dwAIGetTargetFlags, AIGTF_USE_SKILL_RANGE_BIT, bUseSkillRange);
	SETBIT(dwAIGetTargetFlags, AIGTF_DONT_VALIDATE_RANGE_BIT, bWarp);
	UNIT * pTarget = AI_GetTarget( pGame, pUnit, nSkill, pdwSearchFlags, dwAIGetTargetFlags, 0.0f, pvLocation );
	if ( ! bUseSkillRange && tContext.nTargetCount < MAX_CONTEXT_TARGETS )
	{
		tContext.pTargets[ tContext.nTargetCount ].nSkill = nSkill;
		tContext.pTargets[ tContext.nTargetCount ].pUnit  = pTarget;
		tContext.nTargetCount++;
	}
	return pTarget;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetTargetForSkill(
	GAME * pGame, 
	UNIT * pTarget, 
	int nSkill,
	AI_CONTEXT & tContext )
{
	for ( int i = 0; i < tContext.nTargetCount; i++ )
	{
		if ( tContext.pTargets[ i ].nSkill == nSkill )
		{
			tContext.pTargets[ i ].pUnit = pTarget;
			return;
		}
	}

	if ( tContext.nTargetCount < MAX_CONTEXT_TARGETS )
	{
		tContext.pTargets[ tContext.nTargetCount ].nSkill = nSkill;
		tContext.pTargets[ tContext.nTargetCount ].pUnit  = pTarget;
		tContext.nTargetCount++;
	} else { // just slam the last one
		tContext.pTargets[ tContext.nTargetCount - 1 ].nSkill = nSkill;
		tContext.pTargets[ tContext.nTargetCount - 1 ].pUnit  = pTarget;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoToTargetUnit(
	GAME * pGame, 
	UNIT * pUnit, 
	MOVE_TYPE eType,
	UNITMODE eMode,
	float fMinAbove,
	float fMaxAbove,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fDistanceOverride = -1 )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	if (eType == MOVE_TYPE_TO_UNIT_FLY)
	{
		fMinAbove += UnitGetPosition(pTarget).fZ;
		fMaxAbove += UnitGetPosition(pTarget).fZ;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( pUnitData && pUnitData->nAppearanceDefId != INVALID_ID ) // invisible monsters without appearances can do anything
		{
			eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
			UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
			if (!bHasMode)
			{
				return FALSE;
			}
		}
	}

	float fRangeDesired = SkillGetDesiredRange( pUnit, nSkill );
	if( fDistanceOverride != -1 )
	{
		fRangeDesired = fDistanceOverride;
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.pTarget = pTarget;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fMinAltitude = fMinAbove;
	tMoveRequest.fMaxAltitude = fMaxAbove;
	tMoveRequest.fDistanceMin = 1.0f;
	tMoveRequest.fDistance = UnitsGetDistance(pUnit, UnitGetPosition(pTarget));
//	tMoveRequest.fStopDistance = fRangeDesired / 2.0f;				// KCK: Why is this /2.0f here?
	tMoveRequest.fStopDistance = fRangeDesired;					
	tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "goto target unit" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorCircleTargetUnit(
	GAME * pGame, 
	UNIT * pUnit, 
	MOVE_TYPE eType,
	UNITMODE eMode,
	float fDistanceToMove,
	float fMinAbove,
	float fMaxAbove,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	REF(pSkillData);

	if (eType == MOVE_TYPE_LOCATION_FLY)
	{
		fMinAbove += UnitGetPosition(pTarget).fZ;
		fMaxAbove += UnitGetPosition(pTarget).fZ;
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = eMode;
	tMoveRequest.eType   = eType;
	tMoveRequest.fDistance = fDistanceToMove;

	if (eType == MOVE_TYPE_CIRCLE)
	{
		tMoveRequest.pTarget = pTarget;
	}
	else
	{
		float fAngle = RandGetFloat(pGame, 0.0f, TWOxPI);

		tMoveRequest.pTarget = NULL;

		tMoveRequest.vTarget.fX = pTarget->vPosition.fX + ( cosf( fAngle ) * fDistanceToMove );
		tMoveRequest.vTarget.fY = pTarget->vPosition.fY + ( sinf( fAngle ) * fDistanceToMove );
		tMoveRequest.vTarget.fZ = RandGetFloat(pGame, fMinAbove,fMaxAbove);

		// KCK: If the point we chose is behind us, go to the opposite side.
		if (VectorDot(tMoveRequest.vTarget-pUnit->vPosition, pUnit->vFaceDirection) < 0.0f)
		{
			fAngle += PI;
			tMoveRequest.vTarget.fX = pTarget->vPosition.fX + ( cosf( fAngle ) * fDistanceToMove );
			tMoveRequest.vTarget.fY = pTarget->vPosition.fY + ( sinf( fAngle ) * fDistanceToMove );
		}
	}

	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "circle target unit" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorStrafe(
	GAME * pGame, 
	UNIT * pUnit, 
	UNITMODE eMode,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	float fDistanceToMove = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Param for Strafe Target" );

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.pTarget = pTarget;
	tMoveRequest.eType	 = MOVE_TYPE_STRAFE;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		tMoveRequest.eType = MOVE_TYPE_STRAFE_FLY;
		tMoveRequest.fMinAltitude = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Param for Min Altitude" );
		tMoveRequest.fMaxAltitude = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Param for Max Altitude" );
	}
	tMoveRequest.fDistance = fDistanceToMove;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "strafe" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAI_TurnTowardsTargetXYZ(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vTarget,
	const VECTOR & vUp )
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

	MSG_SCMD_UNITTURNXYZ msg;
	msg.id = UnitGetId( pUnit );
	msg.vFaceDirection = vDirection;
	msg.vUpDirection   = vUp;
	msg.bImmediate = FALSE;
	s_SendUnitMessage( pUnit, SCMD_UNITTURNXYZ, &msg);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStopSkillHandler(
	GAME* game, 
	UNIT* unit, 
	const struct EVENT_DATA& event_data)
{
	int nSkill = (int)event_data.m_Data1;
	BOUNDED_WHILE( SkillIsRequested( unit, nSkill), 100 )
	{
		SkillStopRequest(unit, nSkill, TRUE, FALSE );
	}
	return TRUE;
}

static BOOL sAIStartSkill(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pTarget,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fHoldTime = 0.0f)
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if ( ! pSkillData )
		return FALSE;
	if ( pTarget && ! SkillDataTestFlag( pSkillData, SKILL_FLAG_DONT_FACE_TARGET ) )
	{
		if( SkillDataTestFlag( pSkillData, SKILL_FLAG_FACE_TARGET_POSITION ) )
		{
			AI_TurnTowardsTargetXYZ( pGame, pUnit, UnitGetPosition( pTarget ), UnitGetUpDirection( pUnit ) );
		}
		else
		{
			AI_TurnTowardsTargetId( pGame, pUnit, pTarget );
		}
	}

	if(AppIsTugboat())
	{
		// ought to be based on 'stop' flag in skill.
		if(UnitPathIsActive(pUnit))
		{
			UnitPathAbort(pUnit);
		}
	}

	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	if ( SkillStartRequest( pGame, pUnit, nSkill, pTarget ? UnitGetId( pTarget ) : INVALID_ID, 
		pTarget ? UnitGetPosition( pTarget ) : VECTOR(0), dwFlags, SkillGetNextSkillSeed( pGame ) ) )
	{
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_ALL  ) || 
			SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_FIRE ) || 
			SkillDataTestFlag( pSkillData, SKILL_FLAG_TRACK_REQUEST ) )
		{
			int nHoldTime = int(fHoldTime*GAME_TICKS_PER_SECOND);
			if (nHoldTime <= 0)
			{
				nHoldTime = 1;
			}
			if ( ! UnitHasTimedEvent( pUnit, sStopSkillHandler, CheckEventParam1, &EVENT_DATA(nSkill) ))
			{
				UnitRegisterEventTimed(pUnit, sStopSkillHandler, EVENT_DATA(nSkill), nHoldTime);
			}
		}

		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI     = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoSkill(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pTarget,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fHoldTime = 0.0f )
{
	if ( SkillShouldStart( pGame, pUnit, nSkill, pTarget, NULL ) )
	{
		return sAIStartSkill(pGame, pUnit, nSkill, pTarget, pBehavior, tContext, fHoldTime );
	}
	return FALSE;
}

//****************************************************************************
// Behaviors
//****************************************************************************

static BOOL sBehaviorDoSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	ASSERTX_RETFALSE( nSkill != INVALID_ID, pUnit->pUnitData->szName );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	UNIT * pTarget = NULL;
	
	float fHoldTime = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) || 
		 SkillDataTestFlag( pSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) )
		pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );


	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_MUST_TARGET_UNIT ) && ! pTarget )
		return FALSE;

	return sDoSkill(pGame, pUnit, nSkill, pTarget, pBehavior, tContext, fHoldTime );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDoSkillArbitrarily(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	UNIT * pTarget = NULL;

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) || 
		SkillDataTestFlag( pSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) )
		pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );

	float fHoldTime = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );

	return sAIStartSkill(pGame, pUnit, nSkill, pTarget, pBehavior, tContext, fHoldTime);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTurnToTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_TURN) )
		return FALSE;

	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	VECTOR vFaceDir = UnitGetFaceDirection( pUnit, FALSE );
	VECTOR vDirection = UnitGetPosition( pTarget ) - UnitGetPosition( pUnit );
	VectorNormalize( vDirection );

	float fMinDot = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );

	float fDotToTarget = VectorDot( vDirection, vFaceDir );
	if ( fDotToTarget > fMinDot )
		return FALSE;

	// figure out which mode to use to turn
	VECTOR vSide;
	VectorCross( vSide, UnitGetUpDirection( pUnit ), vFaceDir );
	VectorNormalize( vSide );

	BOOL bTurnRight = VectorDot( vSide, vDirection ) > 0.0f;

	UNITMODE eMode = bTurnRight ? MODE_TURN_RIGHT : MODE_TURN_LEFT;

	BOOL bHasMode;
	float fTime = UnitGetModeDuration( pGame, pUnit, eMode, bHasMode );

	if( AppIsHellgate() )
	{
		if ( fDotToTarget < -0.5f )
			fTime *= 4.0f;
		else if ( fDotToTarget < 0.2f )
			fTime *= 3.0f;
		else if ( fDotToTarget < 0.6f )
			fTime *= 2.0f;
	}

	if(bHasMode)
	{
		if ( ! s_UnitSetMode( pUnit, eMode, 0, fTime, 0, TRUE ) )
			return FALSE;
	}

	AI_TurnTowardsTargetId( pGame, pUnit, pTarget );

	if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
	{
		tContext.bStopAI = TRUE;
		tContext.bAddAIEvent = FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTurnToCopyTargetFacing(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_TURN) )
		return FALSE;

	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	VECTOR vFaceDirCurr    = UnitGetFaceDirection( pUnit, FALSE );
	VECTOR vFaceDirDesired = UnitGetFaceDirection( pTarget, FALSE );

	float fMinDot = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );

	float fDotToTarget = VectorDot( vFaceDirCurr, vFaceDirDesired );
	if ( fDotToTarget > fMinDot )
		return FALSE;

	// figure out which mode to use to turn
	VECTOR vSide;
	VectorCross( vSide, UnitGetUpDirection( pUnit ), vFaceDirCurr );
	VectorNormalize( vSide );

	BOOL bTurnRight = VectorDot( vSide, vFaceDirDesired ) > 0.0f;

	UNITMODE eMode = bTurnRight ? MODE_TURN_RIGHT : MODE_TURN_LEFT;

	BOOL bHasMode;
	float fTime = UnitGetModeDuration( pGame, pUnit, eMode, bHasMode );
	if ( bHasMode )
	{
		if ( fDotToTarget < -0.5f )
			fTime *= 4.0f;
		else if ( fDotToTarget < 0.2f )
			fTime *= 3.0f;
		else if ( fDotToTarget < 0.6f )
			fTime *= 2.0f;

		if ( ! s_UnitSetMode( pUnit, eMode, 0, fTime, 0, TRUE ) )
			return FALSE;
	}


	AI_TurnTowardsTargetXYZ( pGame, pUnit, UnitGetPosition( pUnit ) + vFaceDirDesired, UnitGetUpDirection( pUnit ) );

	if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
	{
		tContext.bStopAI = TRUE;
		tContext.bAddAIEvent = ! bHasMode;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorInsertBehaviors(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	const AI_BEHAVIOR_DEFINITION * pBehaviorDef = pBehavior->pDefinition;
	ASSERT_RETFALSE( pBehaviorDef );
	const AI_BEHAVIOR_DEFINITION_TABLE * pBehaviorDefTable = &pBehaviorDef->tTable;
	ASSERT_RETFALSE( pBehaviorDefTable );

	if ( ! pBehaviorDefTable->nBehaviorCount )
		return FALSE;

	int nTimeoutMin = int( pBehaviorDef->pfParams[0] * GAME_TICKS_PER_SECOND );
	int nTimeoutMax = int( pBehaviorDef->pfParams[2] * GAME_TICKS_PER_SECOND );
	if(nTimeoutMax < nTimeoutMin)
	{
		nTimeoutMax = nTimeoutMin;
	}
	int nActualTimeout = RandGetNum(pGame, nTimeoutMin, nTimeoutMax);

	if(!AI_InsertTable(pUnit, pBehaviorDefTable, NULL, nActualTimeout))
		return FALSE;

	if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
	{
		tContext.bStopAI = TRUE;
		tContext.bAddAIEvent = TRUE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDoSkillInsertBehaviors(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	BOOL bSuccess = sBehaviorDoSkill(pGame, pUnit, pBehavior, tContext);

	if (bSuccess)
	{
		bSuccess = sBehaviorInsertBehaviors(pGame, pUnit, pBehavior, tContext);
	}
	return bSuccess;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDoSkillBranch(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	BOOL bSuccess = sBehaviorDoSkill(pGame, pUnit, pBehavior, tContext);

	if (bSuccess)
	{
		tContext.bStopAI     = FALSE;
		tContext.bAddAIEvent = FALSE;
		tContext.nBranchTo = 0;
	}
	return bSuccess;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorWander(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_RANDOM_WANDER;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP)
	{
		eType = MOVE_TYPE_WARP;
	}
	else if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_RANDOM_WANDER_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}

	if ( UnitTestMode( pUnit, MODE_WALK ) || UnitTestMode( pUnit, MODE_RUN ) )
	{// we are already walking... just keep going.
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}

	GAME_TICK tiChangeTick = sAITimerGet( pUnit, pBehavior );
	if (tiChangeTick > GameGetTick(pGame))
	{ // we started walking recently
		return FALSE;
	}

	float fDistanceToMove = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Move Distance for Wander" );

	float fWanderRange	  = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Wander Range for Wander" );

	float fMinDistance;
	float fMinAltitude;
	float fMaxAltitude;

	if (eType == MOVE_TYPE_RANDOM_WANDER_FLY)
	{
		fMinDistance = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Min Distance for Wander" );
		fMinAltitude = sBehaviorGetFloat( pGame, pBehavior, 3, TRUE, "Bad Min Altitude for Wander" );
		fMaxAltitude = sBehaviorGetFloat( pGame, pBehavior, 4, TRUE, "Bad Max Altitude for Wander" );
	}
	else
	{
		fMinDistance = fMinAltitude = fMaxAltitude = 0.0f;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
		UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
		if (!bHasMode)
		{
			if (eType != MOVE_TYPE_WARP) 
				return FALSE;
		}
	}

	VECTOR vSpawnPoint = UnitGetStatVector( pUnit, STATS_AI_ANCHOR_POINT_X, 0 );
	if(VectorIsZero(vSpawnPoint) || vSpawnPoint == UnitGetPosition(pUnit))
	{
		if (eType == MOVE_TYPE_RANDOM_WANDER_FLY)
			vSpawnPoint = UnitGetPosition(pUnit) + RandGetVector(pGame);
		else
			vSpawnPoint = UnitGetPosition(pUnit) + RandGetVectorXY(pGame);
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = eMode;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fDistance = fDistanceToMove;
	tMoveRequest.fDistanceMin = fMinDistance;
	tMoveRequest.fMinAltitude = fMinAltitude;
	tMoveRequest.fMaxAltitude = fMaxAltitude;
	tMoveRequest.fStopDistance = 0.5f;
	tMoveRequest.fWanderRange = fWanderRange;
	tMoveRequest.vTarget = vSpawnPoint;
	tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_CHEAP_LOOKUPS_ONLY;
	tMoveRequest.ePathType = PATH_WANDER;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "wander" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}

		RAND tRand;
		RandInit( tRand );

		float fDelayBetweenWalksMin = sBehaviorGetFloat( pGame, pBehavior, 5, FALSE, "Bad Time delay for Wander" );
		fDelayBetweenWalksMin = MAX( fDelayBetweenWalksMin, 0.5f );
		float fDelayBetweenWalks = fDelayBetweenWalksMin + RandGetFloat( tRand, fDelayBetweenWalksMin );

		float fVelocity = UnitGetVelocity( pUnit );
		float fTimeWalking = fVelocity == 0.0f ? 1.0f : fDistanceToMove / fVelocity;
		int nTickDelay = (int)(GAME_TICKS_PER_SECOND * (fTimeWalking + fDelayBetweenWalks));
		sAITimerSet( pUnit, pBehavior, nTickDelay );
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct HAPPY_ROOM_WANDER_ROOM_CONTEXT
{
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
	ROOM_LAYOUT_GROUP * pGroup;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorHappyRoomWander(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_LOCATION;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}

	if ( UnitTestMode( pUnit, MODE_WALK ) || UnitTestMode( pUnit, MODE_RUN ) )
	{// we are already walking... just keep going.
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
		UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
		if (!bHasMode)
		{
			return FALSE;
		}
	}

	if(!pBehavior->pDefinition->pszString || !pBehavior->pDefinition->pszString[0])
	{
		return FALSE;
	}

	const int MAX_ROOM_HISTORY = 4;

	ROOM * pCurrentRoom = UnitGetRoom(pUnit);
	if ( !pCurrentRoom )
		return FALSE;

	ROOMID idLastRoom[MAX_ROOM_HISTORY];
	for(int i=0; i<MAX_ROOM_HISTORY; i++)
	{
		idLastRoom[i] = (ROOMID)UnitGetStat(pUnit, STATS_AI_PARAMETER_ID, i);
	}
	HAPPY_ROOM_WANDER_ROOM_CONTEXT pContexts[MAX_CONNECTIONS_PER_ROOM];
	memclear(pContexts, MAX_CONNECTIONS_PER_ROOM * sizeof(HAPPY_ROOM_WANDER_ROOM_CONTEXT));

	int nActuallyAvailableCount = 0;
	int nConnectedCount = RoomGetConnectedRoomCount(pCurrentRoom);
	for(int i=0; i<nConnectedCount; i++)
	{
		ROOM * pConnectedRoom = RoomGetConnectedRoom(pCurrentRoom, i);
		if(!pConnectedRoom)
			continue;

		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * pGroup = RoomGetLabelNode(pConnectedRoom, pBehavior->pDefinition->pszString, &pOrientation);
		if(!pGroup)
			continue;

		pContexts[nActuallyAvailableCount].pRoom = pConnectedRoom;
		pContexts[nActuallyAvailableCount].pGroup = pGroup;
		pContexts[nActuallyAvailableCount].pOrientation = pOrientation;
		nActuallyAvailableCount++;
	}
	if(nActuallyAvailableCount <= 0)
	{
		return FALSE;
	}

	int nIndex = INVALID_ID;
	for(int i=0; i<(MAX_ROOM_HISTORY+1) && nIndex < 0; i++)
	{
		PickerStart(pGame, picker);
		for(int j=0; j<nActuallyAvailableCount; j++)
		{
			HAPPY_ROOM_WANDER_ROOM_CONTEXT * pContext = &pContexts[j];

			BOOL bSkipOut = FALSE;
			ROOMID idConnectedRoom = RoomGetId(pContext->pRoom);
			for(int k=0; k<(MAX_ROOM_HISTORY-i); k++)
			{
				if(idConnectedRoom == idLastRoom[k])
				{
					bSkipOut = TRUE;
					break;
				}
			}
			if(bSkipOut)
				continue;
			
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, j, 1));
		}
		nIndex = PickerChoose(pGame);
	}

	if(nIndex < 0)
	{
		return FALSE;
	}

	ROOM * pSelectedRoom = pContexts[nIndex].pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = pContexts[nIndex].pOrientation;
	ROOM_LAYOUT_GROUP * pLayoutGroup = pContexts[nIndex].pGroup;
	if(!pSelectedRoom || !pLayoutGroup || !pOrientation)
	{
		return FALSE;
	}
	
	VECTOR vTargetPosition;
	if(pOrientation->pNearestNode)
	{
		vTargetPosition = RoomPathNodeGetExactWorldPosition(pGame, pSelectedRoom, pOrientation->pNearestNode);
	}
	else
	{
		MatrixMultiply(&vTargetPosition, &pOrientation->vPosition, &pSelectedRoom->tWorldMatrix);
	}

	for(int i=MAX_ROOM_HISTORY-1; i>=1; i--)
	{
		UnitSetStat(pUnit, STATS_AI_PARAMETER_ID, i, UnitGetStat(pUnit, STATS_AI_PARAMETER_ID, i-1));
	}
	UnitSetStat(pUnit, STATS_AI_PARAMETER_ID, 0, RoomGetId(pCurrentRoom));

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = eMode;
	tMoveRequest.eType	 = eType;
	tMoveRequest.vTarget = vTargetPosition;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "happy room wander" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}

		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoToTargetUnit(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_TO_UNIT;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_TO_UNIT_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}
	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Min Above for Go To Target Unit" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Max Above for Go To Target Unit" );
	return sBehaviorGoToTargetUnit(pGame, pUnit, eType, eMode, fMinAbove, fMaxAbove, pBehavior, tContext);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorChangeAltitude(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_LOCATION_FLY;
	UNITMODE eMode = MODE_WALK;

	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}
	float fDistance = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Distance for Adjust Altitude" );
	float fMinAltitude = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Altitude for Adjust Altitude" );
	float fMaxAltitude = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Altitude for Adjust Altitude" );

	float fCurAltitude = AIGetAltitude( pGame, pUnit );
	if ( fCurAltitude < fMaxAltitude && fCurAltitude > fMinAltitude )
		return FALSE;

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( pUnitData && pUnitData->nAppearanceDefId != INVALID_ID ) // invisible monsters without appearances can do anything
		{
			eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
			UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
			if (!bHasMode)
			{
				return FALSE;
			}
		}
	}

	VECTOR vTarget = UnitGetPosition( pUnit );
	if ( fCurAltitude < fMinAltitude )
	{
		fDistance = MIN( fDistance, ( fMinAltitude - fCurAltitude ) );
		vTarget.fZ += fDistance;
	}
	else
	{
		fDistance = MIN( fDistance, ( fCurAltitude - fMaxAltitude ) );
		vTarget.fZ -= fDistance;
	}


	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.pTarget = NULL;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fMinAltitude = fMinAltitude;
	tMoveRequest.fMaxAltitude = fMaxAltitude;
	tMoveRequest.vTarget = vTarget;
	tMoveRequest.fDistanceMin = 1.0f;
	tMoveRequest.fDistance = fDistance;
	tMoveRequest.fStopDistance = 1.0f;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "fly down" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoToLabelNode(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_LOCATION;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_LOCATION_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}

	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM * pTargetRoom = NULL;
	const ROOM_LAYOUT_GROUP * pLayoutGroup = LevelGetLabelNode( UnitGetLevel( pUnit ), pBehavior->pDefinition->pszString, &pTargetRoom, &pOrientation );
	if(!pLayoutGroup || !pTargetRoom || !pOrientation)
		return FALSE;

	VECTOR vTargetPosition;
	if(pOrientation->pNearestNode)
	{
		vTargetPosition = RoomPathNodeGetExactWorldPosition(pGame, pTargetRoom, pOrientation->pNearestNode);
	}
	else
	{
		MatrixMultiply(&vTargetPosition, &pOrientation->vPosition, &pTargetRoom->tWorldMatrix);
	}

	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Min Above for Go To Target Unit" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Max Above for Go To Target Unit" );
	if (eType == MOVE_TYPE_TO_UNIT_FLY)
	{
		fMinAbove += vTargetPosition.fZ;
		fMaxAbove += vTargetPosition.fZ;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( pUnitData && pUnitData->nAppearanceDefId != INVALID_ID ) // invisible monsters without appearances can do anything
		{
			eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
			UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
			if (!bHasMode)
			{
				return FALSE;
			}
		}
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.vTarget = vTargetPosition;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fMinAltitude = fMinAbove;
	tMoveRequest.fMaxAltitude = fMaxAbove;
	tMoveRequest.fDistanceMin = 0.01f;
	tMoveRequest.fDistance = VectorLength(UnitGetPosition(pUnit) - vTargetPosition);
	tMoveRequest.fStopDistance = 0.01f;
	tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "goto label node" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoTowardSpawnLocation(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_LOCATION;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_LOCATION_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}

	VECTOR vSpawnPoint = UnitGetStatVector(pUnit, STATS_AI_ANCHOR_POINT_X, 0);
	if(VectorIsZero(vSpawnPoint))
	{
		return FALSE;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( pUnitData && pUnitData->nAppearanceDefId != INVALID_ID ) // invisible monsters without appearances can do anything
		{
			eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
			UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
			if (!bHasMode)
			{
				return FALSE;
			}
		}
	}

	float fMinRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "Bad Min Range for Go To Spawn Point" );
	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "Bad Max Range for Go To Spawn Point" );
	float fDistance = sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, "Bad Random Distance for Go To Spawn Point" );

	if(fDistance > 0.0f)
	{
		float fRandomAngle = RandGetFloat(pGame, 0.0f, TWOxPI);
		float fRandomDistance = RandGetFloat(pGame, 0.0f, fDistance);

		vSpawnPoint.fX += fRandomDistance * cos(fRandomAngle);
		vSpawnPoint.fY += fRandomDistance * sin(fRandomAngle);
	}

	float fDistanceToSpawnPoint = VectorDistanceSquared(vSpawnPoint, UnitGetPosition(pUnit));
	if(fMinRange > 0 && fDistanceToSpawnPoint < (fMinRange*fMinRange))
	{
		return FALSE;
	}
	if(fMaxRange > 0 && fDistanceToSpawnPoint > (fMaxRange*fMaxRange))
	{
		return FALSE;
	}

	if ( pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP )
		eType = MOVE_TYPE_WARP;

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.vTarget = vSpawnPoint;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fMinAltitude = 0.0f;
	tMoveRequest.fMaxAltitude = 0.0f;
	tMoveRequest.fDistanceMin = 0.01f;
	tMoveRequest.fDistance = VectorLength(UnitGetPosition(pUnit) - vSpawnPoint);
	tMoveRequest.fStopDistance = 0.01f;
	tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "go toward spawn location" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorApproachTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	float fRunDistance,
	float fMinAbove,
	float fMaxAbove,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fDistanceOverride = -1.0f)
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	// KCK: This is using distanced squared do math operations with collision radii. Distance squared is only
	// really useful for comparasions. Instead take the performance hit and use distance here.
//	float fDistanceSquared = AppIsHellgate() ? UnitsGetDistanceSquared(pUnit, pTarget) : UnitsGetDistanceSquaredXY(pUnit, pTarget );
	float fDistance = AppIsHellgate() ? sqrtf(UnitsGetDistanceSquared(pUnit, pTarget)) : sqrtf(UnitsGetDistanceSquaredXY(pUnit, pTarget ));
	float fCollisionUnit = UnitGetCollisionRadius(pUnit);
	float fCollisionTarget = UnitGetCollisionRadius(pTarget);
	if (fDistance - fCollisionTarget - fCollisionUnit < 1.0f && fDistanceOverride >= -1.01f)
	{
		return FALSE;
	}

	UNITMODE eMode = MODE_WALK;
	if(AppIsTugboat())
	{
		if ( UnitCanRun( pUnit ) && (fDistance > fRunDistance) )
		{
			eMode = MODE_RUN;
		}
	}
	else
	{
		if ( UnitTestMode( pUnit, MODE_RUN ) ||
			(fDistance > fRunDistance) )
		{
			eMode = MODE_RUN;
		}
	}

	MOVE_TYPE eType = MOVE_TYPE_TO_UNIT;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_TO_UNIT_FLY;
		fDistanceOverride = fCollisionTarget + fCollisionUnit;	// KCK: Use the override, as collision is turned off.
	}

	return sBehaviorGoToTargetUnit( pGame, pUnit, eType, eMode, fMinAbove, fMaxAbove, pBehavior, tContext, fDistanceOverride );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorApproachTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	float fDistance = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Above for Approach Target Unit" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Above for Approach Target Unit" );
	return sBehaviorApproachTarget( pGame, pUnit, fDistance, fMinAbove, fMaxAbove, pBehavior, tContext );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorApproachTargetAtRange(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	float fMinDistance = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Distance for Approach Target At Range" );
	float fMaxDistance = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Distance for Approach Target At Range" );

	float fDistance = AppIsHellgate() ? UnitsGetDistanceSquared(pUnit, pTarget) : UnitsGetDistanceSquaredXY(pUnit, pTarget);
	BOOL bOutOfRange = FALSE;
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, FALSE );
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		int nWeaponSkill = ItemGetPrimarySkill( pWeaponArray[ i ] );
		if ( nWeaponSkill != INVALID_ID )
		{
			pSkillData = SkillGetData( pGame, nWeaponSkill );
		}
	}

	// check LOS
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) && pTarget )
	{
		//if ( ! SkillTestLineOfSight( pGame, pUnit, nSkill, pSkillData, pWeapon, pUnitTarget, pvTarget ) )
		if( !UnitInLineOfSight( pGame, pUnit, pTarget ) )
		{
			bOutOfRange = TRUE;
		}
	}

	if(fDistance < fMinDistance*fMinDistance && !bOutOfRange)
	{
		return FALSE;
	}

	if (fDistance > fMaxDistance*fMaxDistance && !bOutOfRange)
	{
		return FALSE;
	}

	float fRunDistance = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 3, TRUE, "Bad Min Above for Approach Target At Range" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 4, TRUE, "Bad Max Above for Approach Target At Range" );
	if( bOutOfRange )
	{
		return sBehaviorApproachTarget( pGame, pUnit, fRunDistance, fMinAbove, fMaxAbove, pBehavior, tContext, sqrt( fDistance ) * .5f );
	}
	else
	{
		return sBehaviorApproachTarget( pGame, pUnit, fRunDistance, fMinAbove, fMaxAbove, pBehavior, tContext );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBackUpFromTargetUnit(
	GAME * pGame, 
	UNIT * pUnit, 
	MOVE_TYPE eType, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fDistanceToMove,
	float fMinDistance,
	float fMaxDistance )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	REF(pSkillData);

	float fMinAltitude, fMaxAltitude;

	if (eType == MOVE_TYPE_BACKUP_FLY)
	{
		fMinAltitude	  = sBehaviorGetFloat( pGame, pBehavior, 3, TRUE, "Bad Min Altitude for Backup" );
		fMaxAltitude	  = sBehaviorGetFloat( pGame, pBehavior, 4, TRUE, "Bad Max Altitude for Backup" );
	}
	else
	{
		fMinAltitude = fMaxAltitude = 0.0f;
	}

	if ( ( AppIsHellgate() ? UnitsGetDistanceSquared( pUnit, pTarget ) : UnitsGetDistanceSquaredXY( pUnit, pTarget ) ) > fMaxDistance * fMaxDistance )
		return FALSE;

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = AppIsHellgate() ? MODE_BACKUP : MODE_RUN;
	tMoveRequest.pTarget = pTarget;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fDistance = RandGetFloat(pGame, fMinDistance, fDistanceToMove);
	tMoveRequest.fMinAltitude = fMinAltitude;
	tMoveRequest.fMaxAltitude = fMaxAltitude;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "backup from target unit" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorNoStandingOnTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext ) 
{
	// This won't happen through other mechanisms, now - so stubbing it out completely.
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBackupFromTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_BACKUP;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_BACKUP_FLY;
	}

	return sBehaviorBackUpFromTargetUnit(pGame, pUnit, eType, pBehavior, tContext,
		sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Move Distance for Backup" ),
		sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Min Distance for Backup"  ),
		sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Max Distance for Backup"  ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGetToRange(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	float fDesiredRange = SkillGetDesiredRange(pUnit, nSkill);
	float fDesiredMinRange = fDesiredRange * .3f;
	float fMaxRange = SkillGetRange(pUnit, nSkill);

	float fDistanceSquared = UnitsGetDistanceSquared(pUnit, pTarget);

	BOOL bOutOfRange = FALSE;
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, FALSE );
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		int nWeaponSkill = ItemGetPrimarySkill( pWeaponArray[ i ] );
		if ( nWeaponSkill != INVALID_ID )
		{
			pSkillData = SkillGetData( pGame, nWeaponSkill );
		}
	}
	// check LOS
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) && pTarget )
	{
		//if ( ! SkillTestLineOfSight( pGame, pUnit, nSkill, pSkillData, pWeapon, pUnitTarget, pvTarget ) )
		if( !UnitInLineOfSight( pGame, pUnit, pTarget ) )
		{
			bOutOfRange = TRUE;
		}
	}

	if ( bOutOfRange || fDistanceSquared > fMaxRange*fMaxRange)
	{
		if( bOutOfRange )
		{
			float fDistance = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
			float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Above for Approach Target Unit" );
			float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Above for Approach Target Unit" );
			sBehaviorApproachTarget( pGame, pUnit, fDistance, fMinAbove, fMaxAbove, pBehavior, tContext, sqrt( fDistanceSquared ) * .5f );
		}
		else
		{
			sBehaviorApproachTarget(pGame, pUnit, pBehavior, tContext );
		}
	}
	else if (fDistanceSquared < fDesiredRange*fDesiredRange)
	{
		float fActualDistance = UnitsGetDistance(pUnit, UnitGetPosition(pTarget));

		return sBehaviorBackUpFromTargetUnit(pGame, pUnit, MOVE_TYPE_BACKUP, pBehavior, tContext,
												fDesiredMinRange - fActualDistance,
												1.0f,
												fMaxRange - fActualDistance );
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorFollowLeader(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP) );
	if ( ! pTarget )
		return FALSE;

	float fRunThreshold = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );
	float fMinRange		= sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	float fMaxRange		= sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, NULL );
	float fLeaderBuffer	= sBehaviorGetFloat( pGame, pBehavior, 3, FALSE, NULL );
	float fRunAheadMult	= sBehaviorGetFloat( pGame, pBehavior, 4, FALSE, NULL );
	BOOL bNoRunning		= ( sBehaviorGetFloat( pGame, pBehavior, 5, FALSE, NULL ) == 1.0f );

	VECTOR vTargetSide;
	VECTOR vTargetFaceDirection = UnitGetFaceDirection( pTarget, FALSE );
	VectorCross( vTargetSide, vTargetFaceDirection, UnitGetUpDirection( pTarget ) );
	VectorNormalize( vTargetSide );

	VECTOR vToUnit = UnitGetPosition( pUnit ) - UnitGetPosition( pTarget );
	float fDistanceToUnit = VectorNormalize( vToUnit );

	if ( VectorDot( vToUnit, vTargetSide ) < 0.0f )
		vTargetSide = - vTargetSide;

	if ( fDistanceToUnit < 1.0f )
	{// this is to remove a dithering effect when moving on top of the player
		if ( VectorDot( vTargetSide, UnitGetFaceDirection( pUnit, FALSE ) ) < 0.0f  )
			vTargetSide = - vTargetSide;
	}

	VECTOR vSideOffset = vTargetSide;
	float fUnitCollisionRadius = AppIsTugboat() ? UnitGetRawCollisionRadius(pUnit) : UnitGetCollisionRadius( pUnit );
	float fTargetCollisionRadius = AppIsTugboat() ? UnitGetRawCollisionRadius(pTarget) : UnitGetCollisionRadius( pTarget );
	vSideOffset *= fUnitCollisionRadius + fTargetCollisionRadius + fLeaderBuffer;
	VECTOR vIdealPosition = UnitGetPosition( pTarget ) + vSideOffset;
	vTargetFaceDirection.fZ = 0.0f;
	vIdealPosition += vTargetFaceDirection; // one meter in front of the guy looks better

	// try to keep up with the target's movements
	if ( UnitTestModeGroup( pTarget, MODEGROUP_MOVE ) )
	{
		VECTOR vTargetMoveDirection = UnitGetMoveDirection( pTarget );
		vTargetMoveDirection *= fRunAheadMult * UnitGetVelocityForMode( pUnit, MODE_RUN );
		vIdealPosition += vTargetMoveDirection;
	}

	if ( pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP )
	{
		ROOM * pRoom = UnitGetRoom( pTarget );
		ROOM * pDestinationRoom = pRoom;
		ROOM_PATH_NODE * pPathNode = NULL;
		DWORD dwNearestNodeFlags = 0;
		SETBIT(dwNearestNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		pPathNode = RoomGetNearestFreePathNode(pGame, UnitGetLevel( pTarget ), vIdealPosition, &pDestinationRoom, pUnit, 0.0f, 0.0f, NULL, dwNearestNodeFlags);
		if( pPathNode )
		{
			vIdealPosition = RoomPathNodeGetExactWorldPosition(pGame, pDestinationRoom, pPathNode);
		} else {
			return FALSE;
		}
	}

	VECTOR vToIdeal = vIdealPosition - UnitGetPosition( pUnit );
	float fDistanceSquared = VectorLengthSquared( vToIdeal );

	if ( fDistanceSquared > fMaxRange * fMaxRange)
		return FALSE;

	if( fDistanceToUnit <= MAX(fMinRange, 4.0f) )
	{
		if ( VectorDot( vToUnit, vTargetFaceDirection ) > 0.0f )
		{
			return FALSE;
		}
	}

	if ( fDistanceSquared > fMinRange * fMinRange)
	{
		BOOL bRun = FALSE;		
		if ( !bNoRunning )
		{
			bRun = fDistanceSquared > fRunThreshold * fRunThreshold;
			if ( UnitTestModeGroup( pTarget, MODEGROUP_MOVE ) )
				bRun = TRUE;
		}

		MOVE_TYPE eMoveType = (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY) ? MOVE_TYPE_LOCATION_FLY : MOVE_TYPE_LOCATION;
		if ( pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP )
			eMoveType = MOVE_TYPE_WARP;

		MOVE_REQUEST_DATA tMoveRequest;
		tMoveRequest.eMode = bRun ? MODE_RUN : MODE_WALK;
		tMoveRequest.pTarget = pTarget;
		tMoveRequest.eType	 = eMoveType;
		tMoveRequest.vTarget = vIdealPosition;
		tMoveRequest.fDistance = sqrt( fDistanceSquared );
		tMoveRequest.fStopDistance = 0.001f; // movement code adds 0.5 if you pass in zero
		tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES;
		if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "follow leader" ) )
		{
			if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
			{
				tContext.bStopAI	 = TRUE;
				tContext.bAddAIEvent = TRUE;
			}
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorForceToGround(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UnitForceToGround(pUnit);
	tContext.bStopAI = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoAheadOrBehindTarget(
	GAME * pGame,
	UNIT * pUnit,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	float fDistanceAheadOrBehind,
	float fMinAltitude,
	float fMaxAltitude,
	float fMinDistanceToMove,
	float fMaxAngle,
	float fFaceDirectionSign)
{
	// define "in front", or "behind" as within the cone of the target unit 
	// facing direction, with a spread angle of nMaxAngle
	int nMaxAngle = PrimeFloat2Int ( fMaxAngle );					// integer trig uses table lookup, may be faster
	//float fMinAngleCosine = gfCos360Tbl[nMaxAngle];

	ASSERT(pGame && pUnit && pBehavior);
	ASSERT(fFaceDirectionSign == 1.0f || fFaceDirectionSign == -1.0f);
	if ( UnitTestMode( pUnit, MODE_WALK ) || UnitTestMode( pUnit, MODE_RUN ) )
	{// we are already walking... just keep going.
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}

	// get the target unit to go in front or behind of
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTargetUnit = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP) );
	if ( ! pTargetUnit )
		return FALSE;

	VECTOR vTargetUnitPosition = UnitGetPosition(pTargetUnit);

	VECTOR vTargetUnitFacing = fFaceDirectionSign * UnitGetFaceDirection(pTargetUnit, FALSE);
	VectorNormalize(vTargetUnitFacing); // TODO cmarch: optimization- isn't this already normalized?

	// check distance to reference position (not randomized, for early out)
	VECTOR vDeltaFromReferenceDestination = vTargetUnitFacing;
	vDeltaFromReferenceDestination *= fDistanceAheadOrBehind;
	vDeltaFromReferenceDestination += vTargetUnitPosition;
	vDeltaFromReferenceDestination.z += ( fMinAltitude + fMaxAltitude ) / 2.0f;
	vDeltaFromReferenceDestination -= UnitGetPosition(pUnit);

	// already within desired distance in front or behind of target unit?
	if( VectorLengthSquared(vDeltaFromReferenceDestination ) < 
		fMinDistanceToMove * fMinDistanceToMove  ) 
		return FALSE;

	// pick an offset from the target unit in front or behind, with a little randomization
	VECTOR vModifiedFacing = vTargetUnitFacing;
	VectorZRotationInt(vModifiedFacing, int(RandGetNum(pGame, 0, nMaxAngle*2)) - nMaxAngle);	
	VectorNormalize(vModifiedFacing);
	if (VectorIsZeroEpsilon(vModifiedFacing))
		vModifiedFacing = vTargetUnitFacing;

	// limit the distance in front or behind to the nearest collision
	LEVEL * pLevel = UnitGetLevel( pUnit );
	VECTOR vCollisionSearchStart = vTargetUnitPosition;
	vCollisionSearchStart.fZ += UnitGetCollisionHeight( pTargetUnit );	// move the collision start point up to avoid hitting the floor?

	fDistanceAheadOrBehind = LevelLineCollideLen(pGame, pLevel, vCollisionSearchStart, vModifiedFacing, fDistanceAheadOrBehind);
	// maybe the destination position should be relative to the target unit position, instead of the offset collision start position?
	// (requires some math if facing direction is not horizontal)
	VECTOR vDestination = vCollisionSearchStart + vModifiedFacing * fDistanceAheadOrBehind;

	float fDistanceToDestination = 0.0f;
	
	float fAltitude = RandGetFloat(pGame, fMinAltitude, fMaxAltitude );
	if(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		// find a valid point above the desired XZ position, up to the
		// unit altitude, and avoid going through ceilings
		FREE_PATH_NODE tNearestPathNode;
		if ( AI_FindPathNode( pGame, pUnit, vDestination, tNearestPathNode ) )
		{
			// just check the distance, don't pass the raised position
			// to the AI move code, since it will cause problems
			// finding the starting path node in a multi story level
			UnitSetStatFloat( pUnit, STATS_ALTITUDE, 0, fAltitude );
			VECTOR vDestinationTemp = UnitCalculatePathTargetFly( pGame, pUnit, tNearestPathNode.pRoom, tNearestPathNode.pNode );
			VECTOR vDeltaToDestinationTemp = vDestinationTemp - UnitGetPosition( pUnit );
			fDistanceToDestination = VectorLength( vDeltaToDestinationTemp );			
		}
		else
		{
			// if there's no path node near the destination, the AI can't reach it
			return FALSE;
		}
	}
	else
	{
		VECTOR vDeltaToDestination = vDestination - UnitGetPosition( pUnit );
		fDistanceToDestination = VectorLength( vDeltaToDestination );
	}

	if ( fMinDistanceToMove > fDistanceToDestination )
		return FALSE;

	UNITMODE eUnitMode = MODE_RUN;
	MOVE_TYPE eMoveType = (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY) ? MOVE_TYPE_LOCATION_FLY : MOVE_TYPE_LOCATION;
	if(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP)
	{
		eMoveType = MOVE_TYPE_WARP;
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eUnitMode;
	tMoveRequest.vTarget = vDestination;
	tMoveRequest.eType	 = eMoveType;
	tMoveRequest.fDistance = fDistanceToDestination;
	tMoveRequest.fMinAltitude = fAltitude;
	tMoveRequest.fMaxAltitude = fAltitude;
	tMoveRequest.dwFlags |= MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "go ahead of target" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoAheadOfTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	float fDistanceAhead		= sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	float fMinAltitude			= sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, NULL );
	float fMaxAltitude			= sBehaviorGetFloat( pGame, pBehavior, 3, FALSE, NULL );
	float fMinDistanceToMove	= sBehaviorGetFloat( pGame, pBehavior, 4, FALSE, NULL );
	float fMaxAngle				= sBehaviorGetFloat( pGame, pBehavior, 5, FALSE, NULL );	

	return sBehaviorGoAheadOrBehindTarget(pGame, pUnit, pBehavior, tContext, fDistanceAhead, fMinAltitude, fMaxAltitude, fMinDistanceToMove, fMaxAngle, 1.0f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoBehindTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	float fDistanceAhead		= sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	float fMinAltitude			= sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, NULL );
	float fMaxAltitude			= sBehaviorGetFloat( pGame, pBehavior, 3, FALSE, NULL );
	float fMinDistanceToMove	= sBehaviorGetFloat( pGame, pBehavior, 4, FALSE, NULL );
	float fMaxAngle				= sBehaviorGetFloat( pGame, pBehavior, 5, FALSE, NULL );

	return sBehaviorGoAheadOrBehindTarget(pGame, pUnit, pBehavior, tContext, fDistanceAhead, fMinAltitude, fMaxAltitude, fMinDistanceToMove, fMaxAngle, -1.0f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoStraight(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	float fDistance = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Above for Approach Target Unit" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Above for Approach Target Unit" );

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = MODE_WALK;
	tMoveRequest.eType   = MOVE_TYPE_LOCATION_FLY;
	tMoveRequest.fDistance = fDistance;
	tMoveRequest.pTarget = NULL;
	tMoveRequest.vTarget.fX = pUnit->vPosition.fX + ( pUnit->vFaceDirection.fX * fDistance );
	tMoveRequest.vTarget.fY = pUnit->vPosition.fY + ( pUnit->vFaceDirection.fY * fDistance );
	tMoveRequest.vTarget.fZ = RandGetFloat(pGame, fMinAbove, fMaxAbove);

	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "move straight ahead" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
		}
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorGoAwayFromTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_AWAY_FROM_UNIT;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_AWAY_FROM_UNIT_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN)
	{
		if( AppIsHellgate() || ( AppIsTugboat() && UnitCanRun( pUnit ) ) )
		{
			eMode = MODE_RUN;
		}
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
		UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
		if (!bHasMode)
		{
			return FALSE;
		}
	}

	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	float fDistanceToMove	= sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Move Distance for Go Away From Target" );
	float fMaxDistance		= sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Max Distance for Go Away From Target" );
	float fMinAltitude		= sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Min Altitude for Go Away From Target" );
	float fMaxAltitude		= sBehaviorGetFloat( pGame, pBehavior, 3, TRUE, "Bad Max Altitude for Go Away From Target" );

	if ( ( AppIsHellgate() ? UnitsGetDistanceSquared( pUnit, pTarget ) : UnitsGetDistanceSquaredXY( pUnit, pTarget ) ) > fMaxDistance * fMaxDistance )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	REF(pSkillData);

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.dwFlags = MOVE_REQUEST_FLAG_RUN_AWAY_ON_ANGLE;
	tMoveRequest.eMode = eMode;
	tMoveRequest.pTarget = pTarget;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fDistance = fDistanceToMove;
	tMoveRequest.fMinAltitude = fMinAltitude;
	tMoveRequest.fMaxAltitude = fMaxAltitude;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "go away from target" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorCircleTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_CIRCLE;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_LOCATION_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN)
	{
		if( AppIsHellgate() || ( AppIsTugboat() && UnitCanRun( pUnit ) ) )
		{
			eMode = MODE_RUN;
		}
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
		UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
		if (!bHasMode)
		{
			return FALSE;
		}
	}

	float fDistanceToMove = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Move Distance for Circle Target" );

	float fMinAbove = 0.0f;
	float fMaxAbove = 0.0f;
	if(eType == MOVE_TYPE_LOCATION_FLY)
	{
		fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Above for Circle Target" );
		fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Abive for Circle Target" );
	}

	return sBehaviorCircleTargetUnit(pGame, pUnit, eType, eMode, fDistanceToMove, fMinAbove, fMaxAbove, pBehavior, tContext);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorFindCover(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eMoveType = MOVE_TYPE_LOCATION;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN)
	{
		eMode = MODE_RUN;
	}

	BOOL bHasMode;
	UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = (eMode == MODE_WALK) ? MODE_RUN : MODE_WALK;
		UnitGetModeDuration(pGame, pUnit, eMode, bHasMode);
		if (!bHasMode)
		{
			return FALSE;
		}
	}

	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP) );
	if ( ! pTarget )
		return FALSE;

	float fMinDistance = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad min distance for Find Cover" );
	float fMaxDistance = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad max distance for Find Cover" );

	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM * pDestinationRoom = UnitGetRoom(pUnit);
	ROOM_LAYOUT_GROUP * pAINode = RoomGetAINodeFromPoint(pDestinationRoom, UnitGetPosition(pUnit), UnitGetPosition(pTarget), ROOM_LAYOUT_FLAG_AI_NODE_CROUCH, &pDestinationRoom, &pOrientation, fMinDistance, fMaxDistance);
	if(!pAINode || !pDestinationRoom)
	{
		return FALSE;
	}
	if ( pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP )
		eMoveType = MOVE_TYPE_WARP;

	VECTOR vIdealPosition(0);
	MatrixMultiply(&vIdealPosition, &pOrientation->vPosition, &pDestinationRoom->tWorldMatrix);

	VECTOR vToIdeal = vIdealPosition - UnitGetPosition( pUnit );
	float fDistanceSquared = VectorLengthSquared( vToIdeal );

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = eMode;
	tMoveRequest.pTarget = pTarget;
	tMoveRequest.eType	 = eMoveType;
	tMoveRequest.vTarget = vIdealPosition;
	tMoveRequest.fDistance = sqrt( fDistanceSquared );
	tMoveRequest.fStopDistance = 0.001f; // movement code adds 0.5 if you pass in zero
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "find cover" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorCopyTargetPosition(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT(pGame && pUnit && pBehavior);

	// get the target unit to go in front or behind of
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTargetUnit = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_WARP) );
	if ( ! pTargetUnit )
		return FALSE;

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = MODE_RUN;
	tMoveRequest.vTarget = UnitGetPosition(pTargetUnit);
	tMoveRequest.eType	 = MOVE_TYPE_WARP;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "copy target position" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI	 = TRUE;
			tContext.bAddAIEvent = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorStrafeRight(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	return sBehaviorStrafe( pGame, pUnit, AppIsHellgate() ? MODE_STRAFE_RIGHT : MODE_RUN, pBehavior, tContext );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorStrafeLeft(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	return sBehaviorStrafe( pGame, pUnit, AppIsHellgate() ? MODE_STRAFE_LEFT : MODE_RUN, pBehavior, tContext );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorStopMoving(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	if (UnitEndModeGroup(pUnit, MODEGROUP_MOVE))
	{
		UnitStepListRemove( pGame, pUnit );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorYell(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if(!pTarget)
	{
		return FALSE;
	}

	BOOL bHasMode;
	float fDuration = UnitGetModeDuration( pGame, pUnit, MODE_WAKEUP, bHasMode );
	if ( bHasMode )
	{
		if (UnitEndModeGroup(pUnit, MODEGROUP_MOVE))
		{
			UnitStepListRemove( pGame, pUnit );
		}
		if ( s_UnitSetMode( pUnit, MODE_WAKEUP, 0, fDuration )  )
		{
			if (pTarget)
			{
				AI_TurnTowardsTargetId( pGame, pUnit, pTarget );
			}

			if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
			{
				tContext.bAddAIEvent = TRUE;
				tContext.bStopAI = TRUE;
			}
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorMinionUpdateCharge(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId != INVALID_ID );
	const SKILL_DATA * pSkillData = SkillGetData( pGame, pBehavior->pDefinition->nSkillId );
	ASSERT_RETFALSE( pSkillData->nRequiredState != INVALID_ID );

	int nGunStages = (int) sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Minion needs gun stages" );
	int nFirstState = pSkillData->nRequiredState - nGunStages + 1;

	int nGunStageCurr = INVALID_ID;
	for ( int i = 0; i < nGunStages; i++ )
	{
		if (UnitHasExactState(pUnit, nFirstState + i))
		{
			nGunStageCurr = i;
			break;
		}
	}

	// If we haven't started or we have finished, then stop
	if ( nGunStageCurr == nGunStages - 1 )
		return TRUE;

	GAME_TICK tiChangeTick = sAITimerGet( pUnit, pBehavior );
	BOOL bResetTimer = FALSE;
	if (tiChangeTick == 0)
	{
		bResetTimer = TRUE;
	} 
	else if (tiChangeTick < GameGetTick(pGame))
	{
		if ( nGunStageCurr != INVALID_ID )
		{// turn off old state
			s_StateClear( pUnit, UnitGetId( pUnit ), nFirstState + nGunStageCurr, 0 );
		}
		// turn on next state
		nGunStageCurr++;
		s_StateSet( pUnit, pUnit, nFirstState + nGunStageCurr, 0 );
		bResetTimer = TRUE;
	}
	if ( bResetTimer )
	{
		int nMinDelay = (int) sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Minion needs delay for guns" );
		int nMaxDelay = (int) pBehavior->pDefinition->pfParams[ 2 ];
		ASSERTX( nMaxDelay > nMinDelay, "Minion gun delays wrong" );
		int nTickDelay = RandGetNum(pGame, nMinDelay, nMaxDelay);
		sAITimerSet( pUnit, pBehavior, nTickDelay );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define LEAP_VELOCITY				10.0f
#define MAX_LEAP_DISTANCE 20.0f
#define MIN_LEAP_DISTANCE  5.0f

static BOOL sBehaviorRavagerLeapAtTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	ASSERT( nSkill != INVALID_ID );

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	float fDistanceToTargetSquared = UnitsGetDistanceSquared( pUnit, pTarget );

	// do I still have the leap/jump skill going?
	if ( UnitGetStat( pUnit, STATS_SKILL_EVENT_ID ) != INVALID_ID )
		return FALSE;

	// is he in range?
	if ( fDistanceToTargetSquared > MAX_LEAP_DISTANCE * MAX_LEAP_DISTANCE || 
		 fDistanceToTargetSquared < MIN_LEAP_DISTANCE * MIN_LEAP_DISTANCE )
		return FALSE;

	if ( ! SkillShouldStart( pGame, pUnit, nSkill, pTarget, NULL ) )
	{
		return FALSE;
	}

	VECTOR vDirection = UnitGetPosition( pTarget ) - UnitGetPosition( pUnit );
	VectorNormalize( vDirection );

	// no Z jumping for now
	if ( fabsf( vDirection.fZ ) > 1.0f )
		return FALSE;

	// leap past the player so that we are aiming at his head instead...
	float fKludgePast = UnitGetCollisionHeight( pTarget );
	VECTOR vDestination;
	vDestination.fX = pTarget->vPosition.fX + ( vDirection.fX * fKludgePast );
	vDestination.fY = pTarget->vPosition.fY + ( vDirection.fY * fKludgePast );
	vDestination.fZ = pTarget->vPosition.fZ;

	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	if ( SkillStartRequest( pGame, pUnit, nSkill, UnitGetId( pTarget ), vDestination, dwFlags, SkillGetNextSkillSeed( pGame ) ) )
	{
		AI_TurnTowardsTargetId( pGame, pUnit, pTarget );

		tContext.bStopAI = TRUE;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranch(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorSetPeriodOverride(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	tContext.nPeriodOverride = int(pBehavior->pDefinition->pfParams[0]);
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchSelectOnce(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pTable );
	if ( pBehavior->pTable->nBehaviorCount > 1 )
	{
		int nSelectedChild = AIPickRandomChild( pGame, pBehavior->pTable );
		if ( nSelectedChild == INVALID_ID )
			return FALSE;
		ASSERT_RETFALSE( nSelectedChild < pBehavior->pTable->nBehaviorCount );

		int nGUID = pBehavior->pTable->pBehaviors[ nSelectedChild ].nGUID;

		for(int i=0; i<pBehavior->pTable->nBehaviorCount; i++)
		{
			if (pBehavior->pTable->pBehaviors[ i ].nGUID != nGUID)
			{
				AI_RemoveBehavior(pUnit, pBehavior->pTable, i, &tContext);
				i--; // this assumes that AI_RemoveBehavior actually removes the behavior
			}
		}
	} 
	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchSelectOne(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pTable );
	ASSERT_RETFALSE( pBehavior->pTable->nBehaviorCount > 0 );
	int nSelectedChild = AIPickRandomChild( pGame, pBehavior->pTable );
	tContext.nBranchTo = nSelectedChild;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckSkillTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, FALSE );
	if ( ! pTarget )
		return FALSE;

	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckSkillTargetCount(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);

	UNITID pnUnitIds[1];
	SKILL_TARGETING_QUERY tTargetQuery(pSkillData);
	tTargetQuery.pSeekerUnit = pUnit;
	tTargetQuery.fMaxRange = float(UnitGetStat(pUnit, STATS_AI_SIGHT_RANGE));
	tTargetQuery.pnUnitIds = pnUnitIds;
	tTargetQuery.nUnitIdMax = 1;
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	SkillTargetQuery(pGame, tTargetQuery);

	int nCountMin = int(sBehaviorGetFloat(pGame, pBehavior, 0, FALSE, ""));
	int nCountMax = int(sBehaviorGetFloat(pGame, pBehavior, 1, FALSE, ""));
	if(nCountMin >= 0 && tTargetQuery.nUnitIdCount < nCountMin)
	{
		return FALSE;
	}

	if(nCountMax >=0 && tTargetQuery.nUnitIdCount > nCountMax)
	{
		return FALSE;
	}

	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchSkillTargetState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, FALSE );
	if ( ! pTarget )
		return FALSE;

	if ( !UnitHasState( pGame, pTarget, pBehavior->pDefinition->nStateId ) )
		return FALSE;
	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchSkillTargetNotState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, FALSE );
	if ( ! pTarget || !UnitHasState( pGame, pTarget, pBehavior->pDefinition->nStateId ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckNoSkillTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, FALSE );
	if ( pTarget )
		return FALSE;

	tContext.nBranchTo = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckCanStartSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext,
	BOOL bCheckCanStart )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext, FALSE );
	// First check no weapon:
	UNITID idWeapon = INVALID_ID;
	BOOL bCanStart = SkillCanStart(pGame, pUnit, nSkill, idWeapon, pTarget, FALSE, FALSE, 0);
	if (! bCheckCanStart )
		bCanStart = !bCanStart;
	if(bCanStart)
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}

	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, FALSE );
	for(int i=0; i<MAX_WEAPON_LOCATIONS_PER_SKILL; i++)
	{
		if(!pWeaponArray[i])
			continue;

		idWeapon = UnitGetId(pWeaponArray[i]);
		bCanStart = SkillCanStart(pGame, pUnit, nSkill, idWeapon, pTarget, FALSE, FALSE, 0);
		if (! bCheckCanStart )
			bCanStart = !bCanStart;
		if(bCanStart)
		{
			tContext.nBranchTo = 0;
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckCanStartSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	return sBehaviorBranchCheckCanStartSkill( pGame, pUnit, pBehavior, tContext, TRUE );
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckCannotStartSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	return sBehaviorBranchCheckCanStartSkill( pGame, pUnit, pBehavior, tContext, FALSE );
}


//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckSkillNotCooling(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	// First check no weapon:
	UNITID idWeapon = INVALID_ID;
	BOOL bIsCooling = SkillIsCooling( pUnit, nSkill, idWeapon );
	if(bIsCooling)
	{
		return FALSE;
	}

	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, FALSE );
	for(int i=0; i<MAX_WEAPON_LOCATIONS_PER_SKILL; i++)
	{
		if(!pWeaponArray[i])
			continue;

		idWeapon = UnitGetId(pWeaponArray[i]);
		bIsCooling = SkillIsCooling( pUnit, nSkill, idWeapon);
		if(bIsCooling)
		{
			return FALSE;
		}
	}
	tContext.nBranchTo = 0;
	return TRUE;
}


//----------------------------------------------------------------------------
static BOOL sAICheckSkillLOS(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext)
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill);
	ASSERT_RETFALSE(pSkillData);

	return SkillTestLineOfSight(pGame, pUnit, nSkill, pSkillData, NULL, pTarget, NULL);
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckLOS(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	BOOL nLOS = sAICheckSkillLOS(pGame, pUnit, pBehavior, tContext);
	if(nLOS)
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckNotLOS(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	BOOL nLOS = sAICheckSkillLOS(pGame, pUnit, pBehavior, tContext);
	if(!nLOS)
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
static BOOL sBehaviorSkillDoNothing(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
		return FALSE;
	if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
	{
		tContext.bStopAI = TRUE;
		tContext.bAddAIEvent = TRUE;

		if(pBehavior->pDefinition->pfParams[0] > 0.0f)
		{
			int nMinTime = int(pBehavior->pDefinition->pfParams[0]);
			int nMaxTime = int(pBehavior->pDefinition->pfParams[1]);
			if(nMaxTime < nMinTime)
			{
				nMaxTime = nMinTime;
			}
			tContext.nPeriodOverride = RandGetNum(pGame, nMinTime, nMaxTime);
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDoSkillAtRange(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	UNIT * pTarget = NULL;

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) || 
		SkillDataTestFlag( pSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) )
		pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_MUST_TARGET_UNIT ) && ! pTarget )
		return FALSE;

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) && ! pTarget )
		return FALSE;

	float fMinRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );
	//ASSERTX( fMinRange != 0.0f, "Bad Min Range for Skill At Range" );
	if (fMinRange <= 0.0f)
	{
		fMinRange = SkillGetDesiredRange(pUnit, nSkill);
	}

	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	//ASSERTX( fMaxRange != 0.0f, "Bad Max Range for Skill At Range" );
	if (fMaxRange <= 0.0f)
	{
		fMaxRange = SkillGetRange(pUnit, nSkill);
	}

	float fHoldTime = sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, NULL );

	float fDistanceBetweenUnitsSquared = 0.0f;

	if(pTarget)
	{
		fDistanceBetweenUnitsSquared = AppIsHellgate() ? UnitsGetDistanceSquared(pUnit, pTarget) : UnitsGetDistanceSquaredXY(pUnit, pTarget);
	}

	if (pTarget && fDistanceBetweenUnitsSquared < fMinRange*fMinRange)
		return FALSE;

	if (pTarget && fDistanceBetweenUnitsSquared > fMaxRange*fMaxRange)
		return FALSE;

	return sDoSkill(pGame, pUnit, nSkill, pTarget, pBehavior, tContext, fHoldTime);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDoNothing(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	tContext.bStopAI	 = TRUE;
	tContext.bAddAIEvent = TRUE;
	if(pBehavior->pDefinition->pfParams[0] > 0.0f)
	{
		int nMinTime = int(pBehavior->pDefinition->pfParams[0]);
		int nMaxTime = int(pBehavior->pDefinition->pfParams[1]);
		if(nMaxTime < nMinTime)
		{
			nMaxTime = nMinTime;
		}
		tContext.nPeriodOverride = RandGetNum(pGame, nMinTime, nMaxTime);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTurnUpright(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_TURN) )
		return FALSE;

	VECTOR vUpDir = UnitGetUpDirection( pUnit );
	if ( vUpDir.fZ < 1.0f )
	{
		vUpDir = VECTOR( 0, 0, 1 );
		UnitSetUpDirection( pUnit, vUpDir );
		VECTOR vFaceDir = UnitGetFaceDirection( pUnit, FALSE );
		if ( VectorDot( vUpDir, vFaceDir ) != 0 )
		{
			VECTOR vSide;
			VectorCross( vSide, vFaceDir, vUpDir );
			if ( VectorIsZero( vSide ) )
				vSide = VECTOR( 0, 1, 0 );
			VectorCross( vFaceDir, vUpDir, vSide );
			VectorNormalize( vFaceDir, vFaceDir );
			ASSERT_RETFALSE( ! VectorIsZero( vFaceDir ) );
			UnitSetFaceDirection( pUnit, vFaceDir, FALSE, FALSE );
		}

		MSG_SCMD_UNITTURNXYZ msg;
		msg.id = UnitGetId( pUnit );
		msg.vFaceDirection = vFaceDir;
		msg.vUpDirection   = vUpDir;
		msg.bImmediate = FALSE;
		s_SendUnitMessage(pUnit, SCMD_UNITTURNXYZ, &msg);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStateId != INVALID_ID );

	if ( UnitHasState( pGame, pUnit, pBehavior->pDefinition->nStateId ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckNotState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStateId != INVALID_ID );

	if ( ! UnitHasState( pGame, pUnit, pBehavior->pDefinition->nStateId ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckStateTime(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nStateId = pBehavior->pDefinition->nStateId;
	ASSERT_RETFALSE( nStateId != INVALID_ID );
	if ( !UnitHasState( pGame, pUnit, nStateId ) )
		return FALSE;

	int nStateTimeTicks = 
		GameGetTick( pGame ) - UnitGetStat( pUnit, STATS_STATE_SET_TICK, nStateId );

	if ( nStateTimeTicks < 0 )
		nStateTimeTicks = 0;

	int nMinTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" ));
	int nMaxTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "" ));

	if ( nStateTimeTicks >= nMinTicks &&  
		( nMaxTicks == 0 || nStateTimeTicks <= nMaxTicks ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckStatPercent(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	int maxStat = StatsDataGetMaxStat(pGame, pBehavior->pDefinition->nStatId);
	ASSERT_RETFALSE(maxStat != INVALID_ID);

	int nStatVal = UnitGetStat(pUnit, pBehavior->pDefinition->nStatId);
	int nMaxVal  = UnitGetStat(pUnit, maxStat);

	float fPercent = ( 100.0f * nStatVal ) / nMaxVal;

	float fPercentMin = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	float fPercentMax = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "" );

	if ( fPercent >= fPercentMin && fPercent <= fPercentMax )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckHasNoWeapons(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nWardrobe = UnitGetWardrobe( pUnit );
	if (   nWardrobe == INVALID_ID || 
		(! WardrobeGetWeapon( pGame, nWardrobe, 0 ) && ! WardrobeGetWeapon( pGame, nWardrobe, 1 ) ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckContainerHasSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UNIT * pContainer = UnitGetContainer( pUnit );
	if ( ! pContainer )
		return FALSE;

	int nSkillLevel = int(pBehavior->pDefinition->pfParams[0]);

	ASSERT_RETFALSE( nSkillLevel > 0 );

	if ( UnitGetStat( pContainer, STATS_SKILL_LEVEL, pBehavior->pDefinition->nSkillId) >= nSkillLevel )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckContainerCheckState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UNIT * pContainer = UnitGetContainer( pUnit );
	if ( ! pContainer )
		return FALSE;

	int nSkillLevel = int(pBehavior->pDefinition->pfParams[0]);

	ASSERT_RETFALSE( nSkillLevel > 0 );

	if ( UnitHasState( pGame, pContainer, pBehavior->pDefinition->nStateId ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}

	return FALSE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckStatValue(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	const STATS_DATA * pStatsData = StatsGetData( pGame, pBehavior->pDefinition->nStatId );
	ASSERT_RETFALSE( pStatsData );

	int nStatVal = UnitGetStat( pUnit, pBehavior->pDefinition->nStatId );

	float fValue = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );

	if ( int(fValue) == nStatVal )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckStatNotValue(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	const STATS_DATA * pStatsData = StatsGetData( pGame, pBehavior->pDefinition->nStatId );
	ASSERT_RETFALSE( pStatsData );

	int nStatVal = UnitGetStat( pUnit, pBehavior->pDefinition->nStatId );

	float fValue = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );

	if ( int(fValue) != nStatVal )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckNonAggressiveTime(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nTicksSinceLastAggressive = 
		UnitGetTicksSinceLastAggressiveSkill( pGame, pUnit );

	if ( nTicksSinceLastAggressive < 0 )
		nTicksSinceLastAggressive = 0;

	int nMinTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" ));
	int nMaxTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "" ));

	if ( nTicksSinceLastAggressive >= nMinTicks &&  
		 ( nMaxTicks == 0 || nTicksSinceLastAggressive <= nMaxTicks ) )
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckRange(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
	ASSERT_RETFALSE(pSkillData);

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if( ! pTarget )
		return FALSE;

	float fDistanceToTargetSquared = VectorDistanceSquared(UnitGetPosition(pUnit), UnitGetPosition(pTarget));

	float fMinRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );
	if (fMinRange < 0.0f)
	{
		fMinRange = SkillGetDesiredRange(pUnit, nSkill);
	}

	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	if (fMaxRange < 0.0f)
	{
		fMaxRange = SkillGetRange(pUnit, nSkill);
	}

	fMinRange *= fMinRange;
	fMaxRange *= fMaxRange;

	if(fDistanceToTargetSquared >= fMinRange && 
		(fDistanceToTargetSquared <= fMaxRange || fMaxRange <= 0.0f ))
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckZRange(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	int nSkill = pBehavior->pDefinition->nSkillId;
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
	ASSERT_RETFALSE(pSkillData);

	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if( ! pTarget )
		return FALSE;

	// Special case:
	// If we have a target, then it must also be on the ground - we don't want to do anything while the character is falling.
	if(!UnitIsOnGround(pTarget))
		return FALSE;

	VECTOR vUnitPosition = UnitGetPosition(pUnit);
	VECTOR vTargetPosition = UnitGetPosition(pTarget);
	float fDistanceToTargetSquared = vUnitPosition.fZ - vTargetPosition.fZ;
	fDistanceToTargetSquared *= fDistanceToTargetSquared;

	float fMinRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );
	if (fMinRange < 0.0f)
	{
		fMinRange = SkillGetDesiredRange(pUnit, nSkill);
	}

	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	if (fMaxRange < 0.0f)
	{
		fMaxRange = SkillGetRange(pUnit, nSkill);
	}

	fMinRange *= fMinRange;
	fMaxRange *= fMaxRange;

	if(fDistanceToTargetSquared >= fMinRange && 
		(fDistanceToTargetSquared <= fMaxRange || fMaxRange <= 0.0f ))
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorBranchCheckRangeToAnchor(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	VECTOR vAnchor = UnitGetStatVector(pUnit, STATS_AI_ANCHOR_POINT_X, 0);

	float fDistanceToTargetSquared = VectorDistanceSquared(UnitGetPosition(pUnit), vAnchor);

	float fMinRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL );
	if (fMinRange < 0.0f)
	{
		return FALSE;
	}

	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL );
	if (fMaxRange < 0.0f)
	{
		return FALSE;
	}

	fMinRange *= fMinRange;
	fMaxRange *= fMaxRange;

	if(fDistanceToTargetSquared >= fMinRange && 
		(fDistanceToTargetSquared <= fMaxRange || fMaxRange <= 0.0f ))
	{
		tContext.nBranchTo = 0;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTraverseLayoutNodes( 
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	MOVE_TYPE eType = MOVE_TYPE_LOCATION;
	UNITMODE eMode = MODE_WALK;
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_FLY)
	{
		eType = MOVE_TYPE_LOCATION_FLY;
	}
	if (pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_RUN && UnitCanRun( pUnit ))
	{
		eMode = MODE_RUN;
	}

	ROOM * pLabelNodeRoom = NULL;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pLayoutGroup = LevelGetLabelNode(UnitGetLevel(pUnit), pBehavior->pDefinition->pszString, &pLabelNodeRoom, &pOrientation);
	if ( !pLayoutGroup || !pOrientation )
	{
		return FALSE;
	}
	if(pLayoutGroup->nGroupCount <= 0)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(pLayoutGroup->pGroups);

	int nAIPathIndex = UnitGetStat(pUnit, STATS_AI_PATH_INDEX);
	if(nAIPathIndex < 0 || nAIPathIndex >= pLayoutGroup->nGroupCount)
	{
		nAIPathIndex = 0;
	}

	float fDistanceFromNode = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Distance for Traverse Layout Nodes" );
	float fMinAbove = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Min Above for Traverse Layout Nodes" );
	float fMaxAbove = sBehaviorGetFloat( pGame, pBehavior, 2, TRUE, "Bad Max Above for Traverse Layout Nodes" );

	ROOM_LAYOUT_SELECTION_ORIENTATION * pAIPathOrientation = RoomLayoutGetOrientationFromGroup(pLabelNodeRoom, &pLayoutGroup->pGroups[nAIPathIndex]);
	ASSERT_RETFALSE(pAIPathOrientation);

	VECTOR vWorldPosition = RoomPathNodeGetExactWorldPosition(pGame, pLabelNodeRoom, pAIPathOrientation->pNearestNode);
	if(VectorDistanceSquared(vWorldPosition, UnitGetPosition(pUnit)) < (fDistanceFromNode * fDistanceFromNode))
	{
		MATRIX tRotate;
		MatrixRotationZ(tRotate, DegreesToRadians(pAIPathOrientation->fRotation));
		VECTOR vFaceDirection(0, 1, 0);
		MatrixMultiplyNormal(&vFaceDirection, &vFaceDirection, &tRotate);
		VECTOR vTarget = UnitGetPosition(pUnit) + vFaceDirection;
		sAI_TurnTowardsTargetXYZ(pGame, pUnit, vTarget, UnitGetUpDirection(pUnit));

		nAIPathIndex++;
		UnitSetStat(pUnit, STATS_AI_PATH_INDEX, nAIPathIndex);
		return FALSE;
	}

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode   = eMode;
	tMoveRequest.vTarget = vWorldPosition;
	tMoveRequest.eType	 = eType;
	tMoveRequest.fMinAltitude = fMinAbove;
	tMoveRequest.fMaxAltitude = fMaxAbove;
	tMoveRequest.fDistanceMin = 1.0f;
	tMoveRequest.fDistance = VectorLength(UnitGetPosition(pUnit) - vWorldPosition);
	tMoveRequest.fStopDistance = fDistanceFromNode / 2.0f;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "traverse layout nodes" ) )
	{
		if(!(pBehavior->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_DONT_STOP))
		{
			tContext.bStopAI = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sIncrementPathIndex(
	GAME* game,
	UNIT* pUnit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	ROOM * pRoom = UnitGetRoom(pUnit);
	ASSERT(pRoom);

	int nPathIndex = UnitGetStat(pUnit, STATS_AI_PATH_INDEX);
	nPathIndex++;

	char pszLabelIndex[8];
	PStrPrintf(pszLabelIndex, 8, "%d", nPathIndex);

	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pLabelNode = RoomGetLabelNode(pRoom, pszLabelIndex, &pOrientation);
	if(!pLabelNode)
	{
		nPathIndex = 0;
	}

	UnitSetStat(pUnit, STATS_AI_PATH_INDEX, nPathIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorKharMove( 
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE(pUnit);
	VECTOR vPosition = UnitGetPosition(pUnit);
	ROOM * pRoom = UnitGetRoom(pUnit);
	ASSERT_RETFALSE(pRoom);

	int nPathIndex = UnitGetStat(pUnit, STATS_AI_PATH_INDEX);

	char pszLabelIndex[8];
	PStrPrintf(pszLabelIndex, 8, "%d", nPathIndex);

	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pLabelNode = RoomGetLabelNode(pRoom, pszLabelIndex, &pOrientation);
	if(!pLabelNode)
	{
		tContext.bAddAIEvent = TRUE;
		return TRUE;
	}

	/*
	int nSkill = pBehavior->pDefinition->nSkillId;
	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, nSkill, tContext );
	if ( ! pTarget )
	{
	tContext.bAddAIEvent = TRUE;
	return;
	}
	// */

	VECTOR vNodePosition;
	MatrixMultiply(&vNodePosition, &pOrientation->vPosition, &pRoom->tWorldMatrix);

	VECTOR vDiff = vPosition - vNodePosition;

	if(!UnitHasEventHandler(pGame, pUnit, EVENT_REACHED_PATH_END, sIncrementPathIndex))
	{
		UnitRegisterEventHandler(pGame, pUnit, EVENT_REACHED_PATH_END, sIncrementPathIndex, EVENT_DATA());
	}

	UnitSetUpDirection(pUnit, pOrientation->vNormal);
	MSG_SCMD_WALLWALK_ORIENT msg;
	msg.id = UnitGetId(pUnit);
	msg.vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);
	msg.vUpDirection = UnitGetUpDirection(pUnit);
	s_SendUnitMessage(pUnit, SCMD_WALLWALK_ORIENT, &msg);

	UNITMODE eMode = MODE_IDLE;
	float fUpDown = vDiff.fZ;
	VECTOR vCheck = vDiff;
	vCheck.fZ = 0.0f;
	float fLeftRight = VectorLength(vCheck);
	if(fabs(fUpDown) > fabs(fLeftRight))
	{
		if(vPosition.fZ > vNodePosition.fZ)
		{
			eMode = MODE_WALK;
		}
		else if(vPosition.fZ < vNodePosition.fZ)
		{
			eMode = MODE_BACKUP;
		}
	}
	else
	{
		VECTOR vCross;
		VectorCross(vCross, UnitGetUpDirection(pUnit), vDiff);
		if(vCross.fZ < 0)
		{
			eMode = MODE_STRAFE_RIGHT;
		}
		else
		{
			eMode = MODE_STRAFE_LEFT;
		}
	}

	ASSERT(eMode != MODE_IDLE);

	MOVE_REQUEST_DATA tMoveRequest;
	tMoveRequest.eMode = eMode;
	tMoveRequest.eType	 = MOVE_TYPE_FACE_DIRECTION;
	tMoveRequest.dwFlags = MOVE_REQUEST_FLAG_DONT_TURN;
	tMoveRequest.vTarget = vNodePosition;
	tMoveRequest.bMoveFlags = MOVE_FLAG_USE_DIRECTION;
	tMoveRequest.fStopDistance = 0.0001f;
	if ( AI_MoveRequest( pGame, pUnit, tMoveRequest, "khar move" ) )
	{
		tContext.bStopAI	 = TRUE;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorPlaySound( 
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERTONCE_RETVAL( pBehavior->pDefinition->nSoundId != INVALID_ID, FALSE );

	MSG_SCMD_UNITPLAYSOUND msg;
	msg.id = UnitGetId( pUnit );
	msg.idSound = pBehavior->pDefinition->nSoundId;
	s_SendUnitMessage( pUnit, SCMD_UNITPLAYSOUND, &msg );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorSpawnAndAttachAI( 
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_ABSOLUTELYNOMONSTERS) != 0 )
		return FALSE;
#endif

	ROOM * pRoom = UnitGetRoom( pUnit );
	ASSERT_RETFALSE( pRoom );
	ASSERT_RETFALSE( pBehavior->pDefinition->nMonsterId != INVALID_ID );

	LEVEL * pLevel = RoomGetLevel( pRoom );

	int nExperienceLevel = LevelGetMonsterExperienceLevel( pLevel );
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ];
	int nSpawnCount = SpawnClassEvaluate( 
		pGame, 
		pLevel->m_idLevel, 
		pBehavior->pDefinition->nMonsterId, 
		nExperienceLevel, 
		tSpawns);

	int nBehaviorSpawnCount = (int) pBehavior->pDefinition->pfParams[ 0 ];
	ASSERT_RETFALSE( nBehaviorSpawnCount );

	VECTOR vPosition = UnitGetPosition( pUnit );

	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNodeSet = RoomGetNodeSet( pRoom, pBehavior->pDefinition->pszString, &pOrientation );
	if ( pNodeSet )
	{
		nBehaviorSpawnCount = pNodeSet->nGroupCount;
	}

	for ( int i = 0; i < nBehaviorSpawnCount; i++ )
	{
		ASSERT_CONTINUE(pNodeSet->pGroups);
		if ( pOrientation )
		{
			ROOM_LAYOUT_SELECTION_ORIENTATION * pSubOrientation = RoomLayoutGetOrientationFromGroup(pRoom, &pNodeSet->pGroups[i]);
			ASSERT_CONTINUE(pSubOrientation);
			MatrixMultiply( &vPosition, &pSubOrientation->vPosition, &pRoom->tWorldMatrix );
		}
		for ( int k = 0; k < nSpawnCount; k++ )
		{
			SPAWN_ENTRY *pSpawn = &tSpawns[ k ];
			int nMonsterClass = pSpawn->nIndex;
			
			MONSTER_SPEC tSpec;
			tSpec.nClass = nMonsterClass;
			tSpec.nExperienceLevel = nExperienceLevel;
			tSpec.pRoom = pRoom;
			tSpec.vPosition = vPosition;
			tSpec.vDirection = UnitGetFaceDirection( pUnit, FALSE );
			
			UNIT * pSpawnedUnit = s_MonsterSpawn( pGame, tSpec );
			if ( ! pSpawnedUnit )
				continue;

			UnitSetStat( pSpawnedUnit, STATS_SPAWN_SOURCE, UnitGetId( pUnit ) );

			AI_INSTANCE * pSpawnedAI = pSpawnedUnit->pAI;
			if ( ! pSpawnedAI )
				continue;
			AI_InsertTable(pSpawnedUnit, &pBehavior->pDefinition->tTable);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetShareTargetArea(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId2 != INVALID_ID );

	UNIT * pUnitToFollow = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext );
	if ( ! pUnitToFollow )
		return FALSE;

	float fDirectionTolerance  = sBehaviorGetFloat( pGame, pBehavior, 1, TRUE, "Bad Param for Share Target Direction Tolerance" );

	// first try to find a target within the follow unit's range
	VECTOR vFollowUnitLocation = UnitGetPosition(pUnitToFollow);
	DWORD dwAIGetTargetFlags = 0;
	UNIT * pTargetsTarget = AI_GetTarget(pGame, pUnit, pBehavior->pDefinition->nSkillId2, NULL, dwAIGetTargetFlags, fDirectionTolerance, &vFollowUnitLocation);

	// look to see if they have an easy target to use 
	if(!pTargetsTarget)
	{
		pTargetsTarget = SkillGetAnyTarget( pUnitToFollow );
		if ( ! pTargetsTarget || ! SkillIsValidTarget( pGame, pUnit, pTargetsTarget, NULL, pBehavior->pDefinition->nSkillId2, TRUE ) )
		{
			float fTimeSinceLastAttack = sBehaviorGetFloat( pGame, pBehavior, 0, TRUE, "Bad Param for Share Target Area Time" );
			int nTicksSinceLastAttack = (int)(fTimeSinceLastAttack * GAME_TICKS_PER_SECOND);

			// see if they have attacked recently
			int nTicksSinceLastAggressive = UnitGetTicksSinceLastAggressiveSkill( pGame, pUnitToFollow );
			if ( nTicksSinceLastAggressive < nTicksSinceLastAttack )
			{
				pTargetsTarget = AI_GetTarget( pGame, pUnit, pBehavior->pDefinition->nSkillId2, NULL,
					dwAIGetTargetFlags, fDirectionTolerance, NULL, pUnitToFollow );

				if ( ! SkillIsValidTarget( pGame, pUnit, pTargetsTarget, NULL, pBehavior->pDefinition->nSkillId2, TRUE ) )
					pTargetsTarget = NULL;

			} else {
				pTargetsTarget = NULL;
			}
		}
	}

	sSetTargetForSkill( pGame, pTargetsTarget, pBehavior->pDefinition->nSkillId2, tContext );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetUseTargetsAttacker(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId2 != INVALID_ID );

	// if we already have a target, then don't do this
	if ( sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId2, tContext, FALSE, FALSE ) )
		return FALSE;

	UNIT * pUnitToFollow = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext );
	if ( ! pUnitToFollow )
		return FALSE;

	UNITID idLastAttacker = UnitGetStat( pUnitToFollow, STATS_AI_LAST_ATTACKER_ID );
	UNIT * pLastAttacker = UnitGetById( pGame, idLastAttacker );
	if ( pLastAttacker && ! SkillIsValidTarget( pGame, pUnit, pLastAttacker, NULL, pBehavior->pDefinition->nSkillId2, TRUE ) )
		pLastAttacker = NULL;

	if ( pLastAttacker )
		sSetTargetForSkill( pGame, pLastAttacker, pBehavior->pDefinition->nSkillId2, tContext );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetUseContainersTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );

	// if we already have a target, then don't do this
	if ( sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId2, tContext, FALSE, FALSE ) )
		return FALSE;

	UNIT* pContainer = UnitGetContainer(pUnit);

	UNIT * pTarget = sGetTargetForSkill( pGame, pContainer, pBehavior->pDefinition->nSkillId, tContext );
	if ( ! pTarget )
		return FALSE;

	sSetTargetForSkill( pGame, pTarget, pBehavior->pDefinition->nSkillId, tContext );
	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetUseAttacker(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );

	// if we already have a target, then don't do this
	if ( sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext, FALSE, FALSE ) )
		return FALSE;

	UNITID idLastAttacker = UnitGetStat( pUnit, STATS_AI_LAST_ATTACKER_ID );
	UNIT * pLastAttacker = UnitGetById( pGame, idLastAttacker );
	if ( pLastAttacker && ! SkillIsValidTarget( pGame, pUnit, pLastAttacker, NULL, pBehavior->pDefinition->nSkillId, TRUE ) )
		pLastAttacker = NULL;

	if ( pLastAttacker )
		sSetTargetForSkill( pGame, pLastAttacker, pBehavior->pDefinition->nSkillId, tContext );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetNearSecondTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId2 != INVALID_ID );

	// if we already have a target, then don't do this
	UNIT * pSearchTarget = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId2, tContext, FALSE, TRUE );
	if ( ! pSearchTarget )
		return FALSE;

	VECTOR vTargetLocation = UnitGetCenter( pSearchTarget );
	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext, FALSE, FALSE, TRUE, &vTargetLocation );

	if ( pTarget )
		sSetTargetForSkill( pGame, pTarget, pBehavior->pDefinition->nSkillId, tContext );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetNearAnchorPoint(
	GAME * pGame,
	UNIT * pUnit,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );

	VECTOR vSpawnPoint = UnitGetStatVector( pUnit, STATS_AI_ANCHOR_POINT_X, 0 );
	vSpawnPoint.fZ += 0.5f;
	UNIT * pTarget = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext, FALSE, FALSE, TRUE, &vSpawnPoint );

	if (!pTarget)
		return FALSE;

	float fMaxRange = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "Bad Max Range for Target Near Anchor Point" );
	float fDistanceToAnchorPoint = VectorDistanceSquared(vSpawnPoint, UnitGetPosition(pUnit));

	if(fMaxRange > 0 && fDistanceToAnchorPoint > (fMaxRange*fMaxRange))
		return FALSE;

	sSetTargetForSkill( pGame, pTarget, pBehavior->pDefinition->nSkillId, tContext );
	return TRUE;
}

//----------------------------------------------------------------------------
// This attempts to target players first, even if at longer range.
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetPreferPlayers(
	GAME * pGame,
	UNIT * pUnit,
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, pBehavior->pDefinition->nSkillId );
	UNIT * pTarget = NULL;

	VECTOR vSearchCenter = UnitGetPosition( pUnit );
	UNITID idUnits[ MAX_TARGETS_PER_QUERY ];

	// setup the targeting query
	SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
	tTargetingQuery.fMaxRange = pSkillData->fRangeMax;
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pvLocation = &vSearchCenter;
	tTargetingQuery.pnUnitIds = idUnits;
	tTargetingQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );

	// get the units in range
	SkillTargetQuery( pGame, tTargetingQuery );

	// Check if any are players (KCK NOTE: We could do this within SkillTargetQuery with a flag)
	// Note: We may want to sort these by distance to get the closest player
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTargetUnit = NULL;
		pTargetUnit = UnitGetById( pGame, idUnits[ i ] );
		ASSERT( pTargetUnit );
		if ( UnitIsA( pTargetUnit, UNITTYPE_PLAYER ) )
			pTarget = pTargetUnit;
	}

	// If we couldn't target a player, just grab anything
	if ( pTarget == NULL )
		pTarget = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext, FALSE, TRUE );

	UnitSetAITarget( pUnit, pTarget );
	sSetTargetForSkill( pGame, pTarget, pBehavior->pDefinition->nSkillId, tContext );

	if ( pTarget == NULL)
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetUseTargetsTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId  != INVALID_ID );
	ASSERT_RETFALSE( pBehavior->pDefinition->nSkillId2 != INVALID_ID );

	// clear AI target first?
	if ( sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, NULL ) > 0.0f )
		UnitSetAITarget( pUnit, INVALID_ID, TRUE );

	UNIT * pFirstTarget = sGetTargetForSkill( pGame, pUnit, pBehavior->pDefinition->nSkillId, tContext, FALSE, TRUE );
	if ( ! pFirstTarget )
		return FALSE;

	if ( SkillIsOnWithFlag( pFirstTarget, SKILL_FLAG_SKILL_IS_ON ) )
	{
		UNIT * pTarget = NULL;
		// "just validate" the target's target with skill 2?
		if ( sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, NULL ) > 0.0f )
		{

			pTarget = SkillGetAnyValidTarget( pFirstTarget, pBehavior->pDefinition->nSkillId2 );
		}
		else
		{
			pTarget = SkillGetTarget( pFirstTarget, pBehavior->pDefinition->nSkillId2, INVALID_ID );
		}

		if ( pTarget )
			UnitSetAITarget( pUnit, pTarget, TRUE );
	}
	return FALSE; // we don't want to stop on this
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorTargetClearAITarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UnitSetAITarget( pUnit, INVALID_ID, FALSE );

	return FALSE; // we don't want to stop on this
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorSetState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE(pBehavior->pDefinition->nStateId != INVALID_ID);
	float fTimeoutMin = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "Bad Param for Set State" );
	float fTimeoutMax = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "Bad Param for Set State" );
	fTimeoutMin = MAX(fTimeoutMin, 0.0f);
	fTimeoutMax = MAX(fTimeoutMax, 0.0f);

	int nActualTimeout = RandGetNum(pGame, int(fTimeoutMin), int(fTimeoutMax));

	if(IS_SERVER(pGame))
	{
		s_StateSet(pUnit, NULL, pBehavior->pDefinition->nStateId, nActualTimeout);
	}
	else
	{
		c_StateSet(pUnit, NULL, pBehavior->pDefinition->nStateId, 0, nActualTimeout, INVALID_ID);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorSetStateAfterClearFor(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	float fTimeoutMin = sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "Bad Param for SetStateAfterClearFor" );
	float fTimeoutMax = sBehaviorGetFloat( pGame, pBehavior, 1, FALSE, "Bad Param for SetStateAfterClearFor" );
	fTimeoutMin = MAX(fTimeoutMin, 0.0f);
	fTimeoutMax = MAX(fTimeoutMax, 0.0f);

	int nActualTimeout = RandGetNum(pGame, int(fTimeoutMin), int(fTimeoutMax));

	// how long is required between states
	float flClearOfStateInSeconds = sBehaviorGetFloat( pGame, pBehavior, 2, FALSE, "Bad Param for SetStateAfterClearFor" );
	DWORD dwClearOfStateInTicks = (DWORD)(GAME_TICKS_PER_SECOND_FLOAT * flClearOfStateInSeconds);

	// what state are we checking
	ESTATE eState = (ESTATE)pBehavior->pDefinition->nStateId;			

	// the state has to be clear now
	if (UnitHasExactState( pUnit, eState ) == FALSE)
	{
		DWORD dwStateClearedOnTick = UnitGetStat( pUnit, STATS_STATE_CLEAR_TICK, eState );
		DWORD dwNow = GameGetTick( pGame );

		// if the state has never been set, or enough has time has passed since the state
		// was cleared, set it again		
		if (dwStateClearedOnTick == 0 || dwNow - dwStateClearedOnTick >= dwClearOfStateInTicks)
		{
			if(IS_SERVER(pGame))
			{
				s_StateSet(pUnit, NULL, eState, nActualTimeout);
			}
			else
			{
				c_StateSet(pUnit, NULL, eState, 0, nActualTimeout, INVALID_ID);
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorClearState(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE(pBehavior->pDefinition->nStateId != INVALID_ID);

	if(IS_SERVER(pGame))
	{
		s_StateClear(pUnit, UnitGetId(pUnit), pBehavior->pDefinition->nStateId, 0);
	}
	else
	{
		c_StateClear(pUnit, UnitGetId(pUnit), pBehavior->pDefinition->nStateId, 0);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
// sBehaviorUseOwnerAnchor
// Copies anchor data from the pet's owner (STATS_PET_DESIRED_LOCATION_X)
//----------------------------------------------------------------------------
static BOOL sBehaviorUseOwnerAnchor(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UNIT * pContainer = UnitGetContainer( pUnit );
	if ( ! pContainer )
		return FALSE;

	int	nGroup = (int) sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	VECTOR	vAnchor = UnitGetStatVector( pContainer, STATS_PET_DESIRED_LOCATION_X, nGroup );
	UnitSetStatVector( pUnit, STATS_AI_ANCHOR_POINT_X, 0, vAnchor );

	return TRUE;
}

//----------------------------------------------------------------------------
// sBehaviorUseOwnerTarget
// Copies anchor data from the pet's owner (STATS_PET_DESIRED_TARGET)
//----------------------------------------------------------------------------
static BOOL sBehaviorUseOwnerTarget(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	UNIT * pContainer = UnitGetContainer( pUnit );
	if ( ! pContainer )
		return FALSE;

	int	nGroup = (int) sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );
	UNITID	nTargetId = (UNITID) UnitGetStat( pContainer, STATS_PET_DESIRED_TARGET, nGroup );
	UnitSetAITarget(pUnit, nTargetId, TRUE);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorSetStat(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	const STATS_DATA * pStatsData = StatsGetData( pGame, pBehavior->pDefinition->nStatId );
	ASSERT_RETFALSE( pStatsData );

	int	nValue = (int) sBehaviorGetFloat( pGame, pBehavior, 0, FALSE, "" );

	UnitSetStat( pUnit, pBehavior->pDefinition->nStatId, nValue);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorIncStat(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	const STATS_DATA * pStatsData = StatsGetData( pGame, pBehavior->pDefinition->nStatId );
	ASSERT_RETFALSE( pStatsData );

	int	nValue = UnitGetStat( pUnit, pBehavior->pDefinition->nStatId );

	UnitSetStat( pUnit, pBehavior->pDefinition->nStatId, nValue + 1 );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBehaviorDecStat(
	GAME * pGame, 
	UNIT * pUnit, 
	AI_BEHAVIOR_INSTANCE * pBehavior,
	AI_CONTEXT & tContext )
{
	ASSERT_RETFALSE( pBehavior->pDefinition->nStatId != INVALID_ID );

	const STATS_DATA * pStatsData = StatsGetData( pGame, pBehavior->pDefinition->nStatId );
	ASSERT_RETFALSE( pStatsData );

	int	nValue = UnitGetStat( pUnit, pBehavior->pDefinition->nStatId );

	UnitSetStat( pUnit, pBehavior->pDefinition->nStatId, nValue - 1 );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct AI_BEHAVIOR_FUNCTION_LOOKUP
{
	const char *pszName;
	FP_BEHAVIOR fpFunction;
};

AI_BEHAVIOR_FUNCTION_LOOKUP gpfnBehaviorFunctionLookup[] =
{			
	{	"skill - do skill",					sBehaviorDoSkill,					},
	{	"skill - do nothing",				sBehaviorSkillDoNothing,			},
	{	"skill - do skill at range",		sBehaviorDoSkillAtRange,			},
	{	"skill - do skill and branch",		sBehaviorDoSkillBranch,				},
	{	"skill - do skill and insert",		sBehaviorDoSkillInsertBehaviors,	},
	{	"skill - do skill arbitrarily",		sBehaviorDoSkillArbitrarily,		},
	{	"move - turn to face target",		sBehaviorTurnToTarget,				},
	{	"move - turn to copy target facing",sBehaviorTurnToCopyTargetFacing,	},
	{	"move - no standing on target",		sBehaviorNoStandingOnTarget,		},
	{	"move - strafe right",				sBehaviorStrafeRight,				},
	{	"move - strafe left",				sBehaviorStrafeLeft,				},
	{	"move - stop moving",				sBehaviorStopMoving,				},
	{	"move - adjust altitude",			sBehaviorChangeAltitude,			},
	{	"move - go to target",				sBehaviorGoToTargetUnit,			},
	{	"move - go toward spawn location",	sBehaviorGoTowardSpawnLocation,		},
	{	"move - go to label node",			sBehaviorGoToLabelNode,				},
	{	"move - approach target",			sBehaviorApproachTarget,			},
	{	"move - get to range",				sBehaviorGetToRange,				},
	{	"move - approach target at range",	sBehaviorApproachTargetAtRange,		},
	{	"move - go ahead of target",		sBehaviorGoAheadOfTarget,			},
	{	"move - go behind target",			sBehaviorGoBehindTarget,			},
	{	"move - go straight",				sBehaviorGoStraight,				},
	{	"move - turn upright",				sBehaviorTurnUpright,				},
	{	"move - wander",					sBehaviorWander,					},
	{	"move - happy room wander",			sBehaviorHappyRoomWander,			},
	{	"move - backup from target",		sBehaviorBackupFromTarget,			},
	{	"move - go away from target",		sBehaviorGoAwayFromTarget,			},
	{	"move - circle target",				sBehaviorCircleTarget,				},
	{	"move - traverse layout nodes",		sBehaviorTraverseLayoutNodes,		},
	{	"move - follow leader",				sBehaviorFollowLeader,				},
	{	"move - find cover",				sBehaviorFindCover,					},
	{	"move - force to ground",			sBehaviorForceToGround,				},
	{	"move - copy target position",		sBehaviorCopyTargetPosition,		},
	{	"yell",								sBehaviorYell,						},
	{	"specific - minion charge",			sBehaviorMinionUpdateCharge,		},
	{	"specific - ravager leap attack",	sBehaviorRavagerLeapAtTarget,		},
	{	"branch",							sBehaviorBranch,					},
	{	"branch - select once",				sBehaviorBranchSelectOnce,			},
	{	"branch - check state",				sBehaviorBranchCheckState,			},
	{	"branch - check not state",			sBehaviorBranchCheckNotState,		},
	{	"branch - check state time",		sBehaviorBranchCheckStateTime,		},
	{	"branch - check stat percent",		sBehaviorBranchCheckStatPercent,	},
	{	"branch - check has no weapons",	sBehaviorBranchCheckHasNoWeapons,	},
	{	"branch - check container has skill",sBehaviorBranchCheckContainerHasSkill,	},
	{	"branch - check container has state", sBehaviorBranchCheckContainerCheckState, },
	{	"ai - insert behaviors",			sBehaviorInsertBehaviors,			},
	{	"do nothing",						sBehaviorDoNothing,					},
	{	"sound - play",						sBehaviorPlaySound,					},
	{	"spawn - attach AI",				sBehaviorSpawnAndAttachAI,			},
	{	"set period override",				sBehaviorSetPeriodOverride,			},
	{	"specific - khar move",				sBehaviorKharMove,					},
	{	"branch - check can start skill",	sBehaviorBranchCheckCanStartSkill,	},
	{	"branch - check cannot start skill",sBehaviorBranchCheckCannotStartSkill,},
	{	"branch - check skill not cooling",	sBehaviorBranchCheckSkillNotCooling,},
	{	"branch - check skill target",		sBehaviorBranchCheckSkillTarget,	},
	{	"branch - check skill target count",sBehaviorBranchCheckSkillTargetCount,},
	{	"branch - skill target state",		sBehaviorBranchSkillTargetState,	},
	{	"branch - skill target not state",	sBehaviorBranchSkillTargetNotState,	},
	{	"branch - check no skill target",	sBehaviorBranchCheckNoSkillTarget,	},
	{	"branch - check stat value",		sBehaviorBranchCheckStatValue,		},
	{	"branch - check stat not value",	sBehaviorBranchCheckStatNotValue,	},
	{	"branch - check non-aggressive time",sBehaviorBranchCheckNonAggressiveTime, },
	{	"branch - check LOS",				sBehaviorBranchCheckLOS,			},
	{	"branch - check not LOS",			sBehaviorBranchCheckNotLOS,			},
	{	"target - share target area",		sBehaviorTargetShareTargetArea,		},
	{	"target - use target's attacker",	sBehaviorTargetUseTargetsAttacker,	},
	{	"target - use container's target",	sBehaviorTargetUseContainersTarget,	},
	{	"target - use attacker",			sBehaviorTargetUseAttacker,			},
	{	"target - near second target",		sBehaviorTargetNearSecondTarget,	},
	{	"target - use target's target",		sBehaviorTargetUseTargetsTarget,	},
	{	"target - clear ai target",			sBehaviorTargetClearAITarget,		},
	{	"target - near anchor point",		sBehaviorTargetNearAnchorPoint,		},
	{	"target - prefer players",			sBehaviorTargetPreferPlayers,		},
	{	"branch - select one",				sBehaviorBranchSelectOne,			},
	{	"branch - check range",				sBehaviorBranchCheckRange,			},
	{	"branch - check Z range",			sBehaviorBranchCheckZRange,			},
	{	"branch - check range to anchor",	sBehaviorBranchCheckRangeToAnchor,	},
	{	"state - set state",				sBehaviorSetState,					},
	{	"state - set state after clear for",sBehaviorSetStateAfterClearFor,		},
	{	"state - clear state",				sBehaviorClearState,				},
	{	"pet - use owner anchor",			sBehaviorUseOwnerAnchor,			},
	{	"pet - use owner target",			sBehaviorUseOwnerTarget,			},
	{   "stat - set stat",                  sBehaviorSetStat,                   },
	{   "stat - inc stat",                  sBehaviorIncStat,					},
	{	"stat - dec stat",					sBehaviorDecStat,					},
	{	"",									NULL								},
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AiBehaviorDataExcelPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	for (unsigned int i = 0; i < ExcelGetCountPrivate(table); ++i)
	{
		AI_BEHAVIOR_DATA * pData = (AI_BEHAVIOR_DATA *)ExcelGetDataPrivate(table, i);

		pData->pfnFunction = NULL;

		const char * pszFunctionName = pData->szFunction;
		if (!pszFunctionName)
		{
			continue;
		}

		int nFunctionNameLength = PStrLen(pszFunctionName);
		if (nFunctionNameLength <= 0)
		{
			continue;
		}

		for (unsigned int j = 0; gpfnBehaviorFunctionLookup[j].pszName[0] != '\0'; ++j)
		{
			const AI_BEHAVIOR_FUNCTION_LOOKUP * pLookup = &gpfnBehaviorFunctionLookup[j];
			if (PStrICmp(pLookup->pszName, pszFunctionName) == 0)
			{
				pData->pfnFunction = pLookup->fpFunction;
				break;
			}
		}

		if (pData->pfnFunction == NULL)
		{
			ASSERTV_CONTINUE(0, "Unable to find AI behavior handler '%s' for '%s' on row '%d'", pData->szFunction, pData->pszName, i);
		}
	}

	return TRUE;
}	
