//----------------------------------------------------------------------------
// unitmodes.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "game.h"
#include "units.h"
#include "unitmodes.h"
#include "ai.h"
#include "s_message.h"
#include "c_animation.h"
#include "c_appearance.h"
#include "c_message.h"
#include "movement.h"
#include "skills.h"
#include "states.h"
#include "console.h"
#include "unit_priv.h"
#include "gameunits.h"
#include "windowsmessages.h"
#include "uix.h"
#include "combat.h"
#include "unittag.h"
#include "c_camera.h"
#include "objects.h"
#include "level.h"
#include "excel_private.h"
#include "picker.h"
#include "c_footsteps.h"
#include "player_move_msg.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
static const int sgnUnitModeFlagSize = DWORD_FLAG_SIZE(NUM_UNITMODES);
static const BOOL sgbModeTrace = FALSE;
static const DWORD RECENT_AGGRESSIVE_MODE_TIME = GAME_TICKS_PER_SECOND * 2;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct ANIMATION;

BOOL UnitMode_End(
	GAME* pGame,
	UNIT* unit,
	const EVENT_DATA& event_data);

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ticks: indeterminate length if <= 0
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitSetModeCallback(
	UNIT* unit,
	UNITMODE mode,
	float fDurationInSeconds,
	BOOL bForce,
	ANIMATION_RESULT *pAnimResult)
{
	int nModelId = c_UnitGetModelId(unit);
	if (nModelId != INVALID_ID)
	{
		const UNITMODE_DATA* pModeData = UnitModeGetData( mode );	
		float fUnitSpeed = UnitGetVelocity(unit);
		REF(fUnitSpeed);

		// setup animation flags		
		DWORD dwFlags = 0;
		
		// modes that aren't given durations should loop ... unless the mode explicitly
		// doesn't loop (which happens when executing client side modes with unknown
		// durations, like jumping)
		if (pModeData->bNoLoop == FALSE)
		{
			dwFlags = fDurationInSeconds == 0.0f ? PLAY_ANIMATION_FLAG_LOOPS : 0;
		}
		dwFlags |= PLAY_ANIMATION_FLAG_ONLY_ONCE;
		if (bForce)
		{
			dwFlags |= PLAY_ANIMATION_FLAG_FORCE_TO_PLAY;
		}
		
		// play animations
		c_AnimationPlayByMode(unit, mode, fDurationInSeconds, dwFlags, pAnimResult);
		
	}
	//trace("[%s]  unit" ID_PRINTFIELD " set mode %d\n", 
	//	IS_SERVER(unit) ? "SRV" : "CLT", 
	//	ID_PRINT(UnitGetId(unit)), 
	//	mode);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sUnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	BOOL value = TRUE)
{
	ASSERT(unit);
	ASSERT((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));
	GAME *game = UnitGetGame( unit );
			
	// set bit for mode
	SETBIT(unit->pdwModeFlags, mode, value);
	
	if (sgbModeTrace)
	{
		BOOL bModeIsOn = UnitTestMode( unit, mode );

		trace( 
			"%s[%d]: Unit Mode '%d' (%s) now %s for '%s'\n", 
			IS_CLIENT( game ) ? "CLIENT" : "SERVER",
			GameGetTick( game ),
			mode, 
			UnitModeGetData( mode )->szName,
			bModeIsOn ? "ON" : "OFF",
			UnitGetDevName( unit ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline static void sUnitClearMode(
	UNIT* unit,
	UNITMODE mode,
	const UNITMODE_DATA * pModeData )
{
	ASSERT(unit);
	ASSERT((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));
	GAME * game = UnitGetGame( unit );
	CLEARBIT(unit->pdwModeFlags, mode);
	if (sgbModeTrace)
	{
		trace( 
			"%s[%d]: Unit Mode '%d' (%s) now %s for '%s'\n", 
			IS_CLIENT( game ) ? "CLIENT" : "SERVER",
			GameGetTick( game ),			
			mode, 
			UnitModeGetData( mode )->szName,
			UnitTestMode( unit, mode ) ? "ON" : "OFF",
			UnitGetDevName( unit ) );
	}
	if( AppIsTugboat() )
	{
		if( ( IsUnitDeadOrDying( unit ) && ( mode == MODE_DYING || mode == MODE_DEAD ) ) || mode == MODE_SPAWNDEAD )
		{
			if( mode == MODE_SPAWNDEAD )
			{
				c_AppearanceSetAnimationPercent( c_UnitGetModelId(unit),
									  MODE_SPAWNDEAD,
									  1 );
			}
			c_AppearanceFreeze( c_UnitGetModelId(unit), TRUE );
		}
	}

	if (IS_CLIENT(game))
	{
		c_AnimationEndByMode(unit, mode);
	}

	if( AppIsTugboat() )
	{
		// when we do these, we are resurrecting, we want to clear any pending freezes or anything else
		if( !IsUnitDeadOrDying( unit ) && ( mode == MODE_DYING || mode == MODE_DEAD || mode == MODE_SPAWNDEAD ) )
		{
			c_AppearanceCancelAllEvents( c_UnitGetModelId(unit) );
			c_AppearanceFreeze( c_UnitGetModelId(unit), FALSE );
		}
	}

	if ( IS_SERVER( game ) && pModeData->nClearStateEnd != INVALID_ID )
	{
		StateClearAllOfType( unit, pModeData->nClearStateEnd );
	}
}


//----------------------------------------------------------------------------
// return TRUE if the event is for the given mode
//----------------------------------------------------------------------------
BOOL UnitModeDo_UnregisterCheck(
	GAME* game,
	UNIT* unit,
	const EVENT_DATA& event_data, 
	const EVENT_DATA* check_data)
{
	ASSERT_RETFALSE(check_data);

	UNITMODE mode = (UNITMODE)((int)(check_data->m_Data1));
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	return mode == (UNITMODE)((int)event_data.m_Data1);
}


//----------------------------------------------------------------------------
// return TRUE if the event is for the given mode
//----------------------------------------------------------------------------
BOOL UnitModeEnd_UnregisterCheck(
	GAME* game,
	UNIT* unit,
	const EVENT_DATA& event_data, 
	const EVENT_DATA* check_data)
{
	ASSERT_RETFALSE(check_data);

	UNITMODE mode = (UNITMODE)((int)(check_data->m_Data1));
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	return mode == (UNITMODE)((int)event_data.m_Data1);
}

#define FIXUP_ACCEPTABLE_TIME_DIFFERENCE GAME_TICKS_PER_SECOND
#define FIXUP_ACCEPTABLE_DISTANCE (0.5f)
#define FIXUP_ACCEPTABLE_DISTANCE_SQ (FIXUP_ACCEPTABLE_DISTANCE)*(FIXUP_ACCEPTABLE_DISTANCE)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitFixupPathingToLastKnownGoodServerLocation(
	UNIT * pUnit)
{
	ASSERT_RETTRUE(pUnit);
	
	//return TRUE;
#if 1
	if(!UnitIsA(pUnit, UNITTYPE_MONSTER))
	{
		return TRUE;
	}
	
	GAME * pGame = UnitGetGame(pUnit);
	ASSERT_RETTRUE(pGame);
	if(!IS_CLIENT(pGame))
	{
		return TRUE;
	}

	if(UnitTestFlag(pUnit, UNITFLAG_NO_CLIENT_FIXUP))
	{
		return TRUE;
	}

	if(IsUnitDeadOrDying(pUnit))
	{
		return FALSE;
	}

	if(UnitIsJumping(pUnit))
	{
		return TRUE;
	}

	UNIT_TAG_LAST_KNOWN_GOOD_POSITION * pTag = UnitGetLastKnownGoodPositionTag(pUnit);
	GAME_TICK tiCurrentTick = GameGetTick(pGame);
	if((tiCurrentTick - pTag->tiLastKnownGoodPositionReceivedTick) > FIXUP_ACCEPTABLE_TIME_DIFFERENCE)
	{
		return TRUE;
	}

	if(VectorDistanceSquared(UnitGetPosition(pUnit), pTag->vLastKnownGoodPosition) < FIXUP_ACCEPTABLE_DISTANCE_SQ)
	{
		return TRUE;
	}

	UnitSetMoveTarget(pUnit, pTag->vLastKnownGoodPosition, pTag->vLastKnownGoodPosition - UnitGetPosition(pUnit));
	if(ClientCalculatePath(pGame, pUnit, MODE_WALK))
	{
		return FALSE;
	}
	return TRUE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_Clear_Step(
	UNIT* unit,
	UNITMODE from,
	UNITMODE mode)
{
	const UNITMODE_DATA * pModeDataFrom = UnitModeGetData(from);
	const UNITMODE_DATA * pModeDataClear = UnitModeGetData(mode);
	if(!pModeDataClear->bSteps || !pModeDataFrom->bSteps)
	{
		UnitStepListRemove(UnitGetGame(unit), unit, SLRF_DONT_SEND_PATH_POSITION_UPDATE);
	}

	// doing client side path fixups like this cause lots of
	// uncertainty in tug, with phantom hits coming in while monsters path,
	// and also ends up with lots of monsters out of range and unattackable
	// for now we're going to be more warp-aggressive on Tug to have a smoother play
	// experience
	if( AppIsHellgate() )
	{
		if((!ModeTestModeGroup(from, MODEGROUP_DYING) &&
			!ModeTestModeGroup(from, MODEGROUP_DEAD) &&
			!UnitTestModeGroup(unit, MODEGROUP_DYING) &&
			!UnitTestModeGroup(unit, MODEGROUP_DEAD))
		)
		{
			sUnitFixupPathingToLastKnownGoodServerLocation(unit);
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_Clear_Emote(
						UNIT* unit,
						UNITMODE from,
						UNITMODE mode)
{
	if( UnitTestModeGroup( unit, MODEGROUP_EMOTE ) )
	{
		return TRUE;
	}
	int nModelId = c_UnitGetModelId(unit);
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		nModelId = c_UnitGetModelIdThirdPersonOverride( unit );

	// make sure all weapons show if we prematurely kill an emote
	c_AppearanceHideWeapons( nModelId, FALSE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_ClearDyingOrDead(
	UNIT* unit,
	UNITMODE from,
	UNITMODE mode)
{
	if ( IS_CLIENT( unit ) )
	{
		GAME * pGame = UnitGetGame( unit );
		if ( unit == GameGetControlUnit( pGame ) )
		{
			c_CameraRestoreViewType();
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
struct CLEAR_LOOKUP
{
	const char* pszName;
	PFN_UNITMODE_CLEAR pfnFunction;
};

//----------------------------------------------------------------------------
static const CLEAR_LOOKUP sgtClearLookupTable[] =
{
	{ "ClearStep",			UnitMode_Clear_Step },
	{ "ClearDyingOrDead",	UnitMode_ClearDyingOrDead },
	{ "ClearEmote",			UnitMode_Clear_Emote },
	
	{ NULL, NULL }  // keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_UNITMODE_CLEAR sLookupClearFunction(
	const char* pszName)
{

	// check for invalid or empty string
	if (pszName == NULL || PStrLen( pszName ) == 0)
	{
		return NULL;
	}

	for (int i = 0; sgtClearLookupTable[ i ].pszName != NULL; ++i)
	{
		const CLEAR_LOOKUP* pLookup = &sgtClearLookupTable[ i ];
		if (PStrICmp( pLookup->pszName, pszName, MAX_FUNCTION_STRING_LENGTH ) == 0)
		{
			return pLookup->pfnFunction;
		}
	}
	ASSERTX_RETNULL( 0, "Unable to find clear function" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoClearMode(
	UNIT * unit,
	UNITMODE efrom,
	UNITMODE clearmode,
	const UNITMODE_DATA* unitmode_data)
{
	ASSERT(unit && unitmode_data);
	if (unitmode_data->bEndEvent)
	{
		EVENT_DATA event_data(clearmode, 0);
		UnitUnregisterTimedEvent(unit, UnitMode_End, UnitModeEnd_UnregisterCheck, &event_data);
	}

	sUnitClearMode( unit, clearmode, unitmode_data );

	// call the mode end function if they don't have an event for it - some modes always ne
	if (unitmode_data->pfnClear)
	{
		unitmode_data->pfnClear( unit, efrom, clearmode );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void ClearModes(
	UNIT* unit,
	UNITMODE eMode,
	const UNITMODE_DATA* unitmode_data)
{
	BOOL bModeBlends = FALSE;
	APPEARANCE_DEFINITION* pAppearanceDef = UnitGetAppearanceDef(unit);

	if (pAppearanceDef && c_AppearanceDefinitionTestModeBlends( pAppearanceDef, (int)eMode) )
	{
		bModeBlends = TRUE;
	}

	if (bModeBlends)
	{
		return;
	}

	for (int ii = 0; ii < MAX_UNITMODES; ii++)
	{
		if (unitmode_data->pnClearModes[ii] < 0)
		{
			break;
		}

		UNITMODE clearmode = (UNITMODE)unitmode_data->pnClearModes[ii];
		if (!UnitTestMode(unit, clearmode))
		{
			continue;
		}

		if (pAppearanceDef && c_AppearanceDefinitionTestModeBlends( pAppearanceDef, (int)clearmode) &&
			!unitmode_data->bForceClear )
		{
			continue; // don't clear modes when they can both blend
		}

		const UNITMODE_DATA* clearmode_data = UnitModeGetData( clearmode );
		if (!clearmode_data)
		{
			break;
		}
		sDoClearMode(unit, eMode, clearmode, clearmode_data);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitEndModeGroup(
	UNIT * pUnit,
	UNITMODE_GROUP eGroup)
{
	GAME * pGame = UnitGetGame(pUnit);
	int nModeCount = ExcelGetNumRows(pGame, DATATABLE_UNITMODES);
	for(int nMode = 0; nMode < nModeCount; nMode++)
	{
		const UNITMODE_DATA * pModeData = UnitModeGetData( nMode );
		ASSERT(pModeData);
		if (pModeData->nGroup == eGroup && UnitTestMode(pUnit, (UNITMODE)nMode))
		{
			//sDoClearMode(pUnit, (UNITMODE)nMode, pModeData);
			UnitEndMode(pUnit, (UNITMODE)nMode);
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitSetModeInt(
	UNIT* unit,
	UNITMODE mode,
	const UNITMODE_DATA* unitmode_data,
	WORD wParam,
	float fDurationInSeconds,
	float fDurationForEndEventInSeconds = 0 )
{
	if (!unitmode_data)
	{
		unitmode_data = UnitModeGetData( mode );
	}
	if (!unitmode_data)
	{
		return;
	}

	if (unitmode_data->bClearSkill)
	{ // this will probably place the unit in idle mode
		SkillStopAll(unit->pGame, unit);
	}

	// clear modes
	ClearModes(unit, mode, unitmode_data);

	sUnitSetMode(unit, mode);

	if ( unitmode_data->nVelocityIndex != INVALID_ID )
	{
		UnitCalcVelocity(unit);
	}
	
	if (unitmode_data->bDoEvent && unitmode_data->pfnDo != NULL)
	{
		unitmode_data->pfnDo( unit, mode );		
	}

	float flDurationForEndEventInSeconds = fDurationForEndEventInSeconds ? fDurationForEndEventInSeconds : fDurationInSeconds;
	if (IS_SERVER(unit->pGame))
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		if ( ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_MODES_IGNORE_AI) )
		{
			if (unitmode_data->bClearAi )
			{
				AI_Unregister(unit);
			}
			else if (unitmode_data->bRestoreAi)
			{
				int ai = UnitGetStat(unit, STATS_AI);
				if ( ai > 0 )
				{
					AI_Register( unit, FALSE, 1 );
				}
			}
		}

		if ( unitmode_data->nClearState != INVALID_ID )
		{
			StateClearAllOfType( unit, unitmode_data->nClearState ); 
		}

	}
	else
	{
			
		// do client callback
		ANIMATION_RESULT tNewAnimations;		
		c_UnitSetModeCallback( unit, mode, fDurationInSeconds, FALSE, &tNewAnimations );
		
		// if the mode uses the duration of the animation, use the largest one
		// instead of the duration specified
		if (unitmode_data->bDurationFromAnims)
		{
			float flLargestDuration = -1.0f;
			for (int i = 0; i < tNewAnimations.nNumResults; ++i)
			{
				int nAppearanceId = tNewAnimations.tResults[ i ].nAppearanceId;
				int nAnimationId = tNewAnimations.tResults[ i ].nAnimationId;
				float flDuration = c_AppearanceAnimationGetDuration( nAppearanceId, nAnimationId );
				if (flDuration > flLargestDuration)
				{
					flLargestDuration = flDuration;

				}
			}
			
			// use duration found (if any)
			if (flLargestDuration >= 0.0f)
			{
				flDurationForEndEventInSeconds = flLargestDuration;
			}
			
		}
		else if(AppIsTugboat() && unitmode_data->bDurationFromContactPoint )
		{

			int nAppearanceDef = UnitGetAppearanceDefId( unit );
			int nAnimationGroup = UnitGetAnimationGroup( unit );
			flDurationForEndEventInSeconds = AppearanceDefinitionGetContactPoint( nAppearanceDef, nAnimationGroup, mode, flDurationForEndEventInSeconds );

		}
		
	}

	// post mode end event
	if (unitmode_data->bEndEvent)
	{
	
		int ticks = PrimeFloat2Int( flDurationForEndEventInSeconds * GAME_TICKS_PER_SECOND_FLOAT );
		if (flDurationForEndEventInSeconds > 0.0f)
		{
			ticks = max(1, ticks);
		}

		if (ticks > 0)
		{	
			EVENT_DATA event_data(mode, wParam);
			UnitRegisterEventTimed(unit, UnitMode_End, &event_data, ticks);
		}
		
	}
	if( AppIsTugboat() )
	{

		if( mode == MODE_OPEN ||
			mode == MODE_OPENED )
		{
#if !ISVERSION(SERVER_VERSION)
			if( mode == MODE_OPENED && IS_CLIENT( unit->pGame ) && ObjectIsDoor( unit ) )
			{
				LevelDirtyVisibility( UnitGetLevel( unit ) );
			}
#endif
			ObjectClearBlockedPathNodes( unit );
		}

		/*else if( mode == MODE_CLOSE )
		{
			ObjectReserveBlockedPathNodes( unit );
		}*/ // FOR NOW NO CLOSING!
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_Do_BeginDead(
	UNIT *pUnit,
	UNITMODE eMode)
{
	SkillsEnteredDead( UnitGetGame( pUnit ), pUnit );	
	return TRUE;
}

//----------------------------------------------------------------------------
struct DO_LOOKUP
{
	const char* pszName;
	PFN_UNITMODE_DO pfnFunction;
};

//----------------------------------------------------------------------------
static const DO_LOOKUP sgtDoLookupTable[] =
{
	{ "BeginDead",	UnitMode_Do_BeginDead },
	// insert do functions here
		
	{ NULL, NULL }		// keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_UNITMODE_DO sLookupDoFunction(
	const char* pszName)
{

	// check for invalid or empty string
	if (pszName == NULL || PStrLen( pszName ) == 0)
	{
		return NULL;
	}
	
	for (int i = 0; sgtDoLookupTable[ i ].pszName != NULL; ++i)
	{
		const DO_LOOKUP* pLookup = &sgtDoLookupTable[ i ];
		if (PStrICmp( pLookup->pszName, pszName, MAX_FUNCTION_STRING_LENGTH ) == 0)
		{
			return pLookup->pfnFunction;
		}
	}
	
	ASSERTX_RETNULL( 0, "Unable to find do function" );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sCanModeBeSet(
	UNIT* unit,
	UNITMODE eMode)
{
	const UNITMODE_DATA* pModeData = UnitModeGetData( eMode );
	const int* pnBlockingModes = pModeData->pnBlockingModes;

	// get hit cannot be set if the unit cannot be interrupted, 
	if (pModeData->bCheckCanInterrupt == TRUE && UnitGetCanInterrupt( unit ) == FALSE)
	{
		return FALSE;
	}
	
	// if a blocking mode is set, then it's illegal to set the given mode
	APPEARANCE_DEFINITION* pAppearanceDef = UnitGetAppearanceDef(unit);
	
	BOOL bModeBlends = pAppearanceDef ? c_AppearanceDefinitionTestModeBlends( pAppearanceDef, (int)eMode) : FALSE;

	for (int ii = 0; ii < MAX_UNITMODES; ii++)
	{
		if (pnBlockingModes[ii] < 0)
		{
			break;
		}
		// if both modes blend, then don't block
		
		if (bModeBlends && pAppearanceDef && c_AppearanceDefinitionTestModeBlends( pAppearanceDef, pnBlockingModes[ii] ) )
		{
			continue;
		}

		if (UnitTestMode(unit, (UNITMODE)pnBlockingModes[ii]))
		{
			return FALSE;
		}
	}
	
	// check for blocking when on ground 
	if (pModeData->bBlockOnGround)
	{
		if (UnitIsOnGround( unit ))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITMODE SetPendingMode(
	UNIT* unit,
	UNITMODE end_mode)
{
	if (unit->nPendingMode <= 0)
	{
		return end_mode;
	}

	const UNITMODE_DATA* mode_data = UnitModeGetData( unit->nPendingMode );
	if (!mode_data)
	{
		return end_mode;
	}

	for (int ii = 0; ii < MAX_UNITMODES; ii++)
	{
		if (mode_data->pnBlockingModes[ii] < 0)
		{
			break;
		}
		if (mode_data->pnBlockingModes[ii] == end_mode)
		{
			return end_mode;
		}
	}
	return (UNITMODE)unit->nPendingMode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End_Generic(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam)
{
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End_Emote(
						  UNIT* unit,
						  UNITMODE mode,
						  WORD wParam)
{
	if( UnitTestModeGroup( unit, MODEGROUP_EMOTE ) )
	{
		return TRUE;
	}
	int nModelId = c_UnitGetModelId(unit);
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		nModelId = c_UnitGetModelIdThirdPersonOverride( unit );

	// make sure all weapons show if we prematurely kill an emote
	c_AppearanceHideWeapons( nModelId, FALSE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End_StopStep(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam)
{
	if ( UnitGetMoveMode( unit ) == MODE_INVALID && ! SkillIsOnWithFlag( unit, SKILL_FLAG_MOVES_UNIT ) &&
		wParam == 0 )
	{
		UnitStepListRemove(UnitGetGame(unit), unit, SLRF_DONT_SEND_PATH_POSITION_UPDATE);
		UnitTriggerEvent(UnitGetGame(unit), EVENT_REACHED_DESTINATION, unit, NULL, NULL);
		if(AppIsHellgate() &&
			!ModeTestModeGroup(mode, MODEGROUP_DYING) &&
			!ModeTestModeGroup(mode, MODEGROUP_DEAD) &&
			!UnitTestModeGroup(unit, MODEGROUP_DYING) &&
			!UnitTestModeGroup(unit, MODEGROUP_DEAD))
		{
			return sUnitFixupPathingToLastKnownGoodServerLocation(unit);
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End_Dead(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam)
{
	ASSERT(unit);
	GAME *game = UnitGetGame(unit);
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game) && unit == GameGetControlUnit(game))
	{
		UISendMessage(WM_RESTART, UnitGetId(unit), 0);
	}
#else
	UNREFERENCED_PARAMETER(game);
#endif
	// make sure that we don't have an end event around for this
	sDoClearMode( unit, MODE_DEAD, MODE_DYING, UnitModeGetData( MODE_DYING ) );

	unit->nPendingMode = MODE_IDLE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End_Knockback(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam)
{
	GAME * pGame = UnitGetGame( unit );
	UnitStepListRemove(pGame, unit, SLRF_DONT_SEND_PATH_POSITION_UPDATE);
	if(UnitGetStat(unit, STATS_CAN_FLY) == 0)
	{
		UnitForceToGround(unit);
	}
	if (unit->m_pPathing)
	{
		PathingUnitWarped( unit );
	}
	UnitTriggerEvent(UnitGetGame(unit), EVENT_REACHED_DESTINATION, unit, NULL, NULL);
	return TRUE;
}

//----------------------------------------------------------------------------
struct END_LOOKUP
{
	const char* pszName;
	PFN_UNITMODE_END pfnFunction;
};

//----------------------------------------------------------------------------
static const END_LOOKUP sgtEndLookupTable[] =
{
	{ "EndGeneric",		UnitMode_End_Generic },
	{ "EndStopStep",	UnitMode_End_StopStep },
	{ "EndDead",		UnitMode_End_Dead },	
	{ "EndKnockback",	UnitMode_End_Knockback },
	{ "EndEmote",		UnitMode_End_Emote },
	
	{ NULL, NULL }  // keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_UNITMODE_END sLookupEndFunction(
	const char* pszName)
{

	// check for invalid or empty string
	if (pszName == NULL || PStrLen( pszName ) == 0)
	{
		return NULL;
	}

	for (int i = 0; sgtEndLookupTable[ i ].pszName != NULL; ++i)
	{
		const END_LOOKUP* pLookup = &sgtEndLookupTable[ i ];
		if (PStrICmp( pLookup->pszName, pszName, MAX_FUNCTION_STRING_LENGTH ) == 0)
		{
			return pLookup->pfnFunction;
		}
	}
	ASSERTX_RETNULL( 0, "Unable to find end function" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMode_End(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA & event_data)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}
	
	UNITMODE mode = (UNITMODE)((int)event_data.m_Data1);
	WORD wParam = (WORD)((int)event_data.m_Data2);

	const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
	if (!unitmode_data)
	{
		return FALSE;
	}

#if (ISVERSION(DEVELOPMENT))
	if (GameGetDebugFlag(UnitGetGame(unit), DEBUGFLAG_MODE_TRACE))
	{
		UNIT* debug_unit = GameGetDebugUnit(UnitGetGame(unit));
		if (debug_unit == unit)
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, "%s tick %d unit %d [%s] clear mode: %s", IS_SERVER(game) ? "server" : "client", GameGetTick(UnitGetGame(unit)), (DWORD)UnitGetId(unit), UnitGetDevName(unit), unitmode_data->szName);
		}
	}
#endif
	sUnitClearMode( unit, mode, unitmode_data );

	if(unitmode_data->nState != INVALID_ID)
		StateClearAllOfType(unit, unitmode_data->nState);
	// run end function (if any)
	if (unitmode_data->pfnEnd)
	{
		if (!unitmode_data->pfnEnd( unit, mode, wParam ))
		{
			return FALSE;
		}
	}

	if (unitmode_data->nEndMode >= 0)
	{
		BOOL bSetIdle = TRUE;
		APPEARANCE_DEFINITION* pAppearanceDef = UnitGetAppearanceDef(unit);
		if (pAppearanceDef && c_AppearanceDefinitionTestModeBlends( pAppearanceDef, mode ))
		{
			for (int i = 0; i < MAX_UNITMODES / 32; i++)
			{
				if (unit->pdwModeFlags[i])
				{
					bSetIdle = FALSE;
				}
			}
		}

		if (bSetIdle)
		{
			UNITMODE end_mode = (UNITMODE)unitmode_data->nEndMode;
			end_mode = SetPendingMode(unit, end_mode);
			if (sCanModeBeSet( unit, end_mode ))
			{
				unit->nPendingMode = MODE_IDLE;
				sUnitSetModeInt(unit, end_mode, NULL, wParam, 0);
			}
		} else {
			int ai = UnitGetStat(unit, STATS_AI);
			const UNIT_DATA * pUnitData = UnitGetData( unit );
			if ( ai > 0 && ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_MODES_IGNORE_AI) )
			{
				AI_Register( unit, FALSE, 1 );
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TryUnitSetMode(
	UNIT * unit,
	UNITMODE mode)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
	if (!unitmode_data)
	{
		return FALSE;
	}

	// if I already am doing this, just stop already...
	if ( UnitTestMode( unit, mode ) )
	{
		return TRUE;
	}

	// check blocking modes
	if (!sCanModeBeSet(unit, mode))
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam,
	float fDurationInSeconds,
	float fDurationForEndEventInSeconds /* = 0 */,
	BOOL bSend /* = TRUE */)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
	if (!unitmode_data)
	{
		return FALSE;
	}

	// if I already am doing this, just stop already...
	if ( UnitTestMode( unit, mode ) )
	{
		return TRUE;
	}

	// check blocking modes
	if (!sCanModeBeSet(unit, mode))
	{
		for (int ii = 0; ii < MAX_UNITMODES; ii++)
		{
			if (unitmode_data->pnWaitModes[ii] < 0)
			{
				break;
			}
			if (UnitTestMode(unit, (UNITMODE)unitmode_data->pnWaitModes[ii]))
			{
				unit->nPendingMode = mode;
			}
		}
		return FALSE;
	}

#if (ISVERSION(DEVELOPMENT))
	if (GameGetDebugFlag(UnitGetGame(unit), DEBUGFLAG_MODE_TRACE))
	{
		UNIT* debug_unit = GameGetDebugUnit(UnitGetGame(unit));
		if (debug_unit == unit)
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, "server tick %d unit %d [%s] set mode: %s", GameGetTick(UnitGetGame(unit)), (DWORD)UnitGetId(unit), UnitGetDevName(unit), unitmode_data->szName);
		}
	}
#endif
	sUnitSetModeInt(unit, mode, unitmode_data, wParam, fDurationInSeconds, fDurationForEndEventInSeconds);

	// send change mode message
	if (bSend)
	{
		if (UnitCanSendMessages( unit ))
		{			
			// TODO: Colin if unit is not in room it could be in an inventory, we would still
			// need to send mode
			ASSERTX( UnitGetRoom( unit ), "Setting mode on unit not in a room ... need to support this apparently" );
				
			// setup message			
			MSG_SCMD_UNITMODE msg;
			msg.id = UnitGetId(unit);
			msg.mode = mode;
			msg.wParam = wParam;
			msg.duration = fDurationInSeconds;
			msg.position = UnitGetPosition(unit);
			
			// send it
			s_SendUnitMessage(unit, SCMD_UNITMODE, &msg);
			
		}
		
	}

	const UNITMODE_DATA* pUnitModeData = UnitModeGetData(mode);
	if(pUnitModeData && pUnitModeData->nState != INVALID_ID)
	{
			s_StateSet(unit, unit, pUnitModeData->nState, (int)(GAME_TICKS_PER_SECOND*fDurationInSeconds));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	BOOL bSend)
{
	s_UnitSetMode(unit, mode, 0, 0.0f, 0, bSend);
	return UnitTestMode(unit, mode);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_UnitSetRandomModeInGroup(
	UNIT* unit,
	UNITMODE_GROUP modegroup)
{
	GAME * pGame = UnitGetGame(unit);
	PickerStart(pGame, picker);
	for(int i=0; i<NUM_UNITMODES; i++)
	{
		if(ModeTestModeGroup((UNITMODE)i, modegroup))
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
		}
	}
	int nPick = PickerChoose(pGame);
	if(nPick < 0)
	{
		return FALSE;
	}
	return c_UnitSetMode(unit, (UNITMODE)nPick, 0, 0.0f);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitEndMode(
	UNIT * unit,
	UNITMODE mode,
	WORD wParam,
	BOOL bSend)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	const UNITMODE_DATA * unitmode_data = UnitModeGetData( mode );
	if (!unitmode_data)
	{
		return FALSE;
	}

	EVENT_DATA event_data(mode, wParam);
	BOOL bRet = UnitMode_End(unit->pGame, unit, event_data);
	UnitUnregisterTimedEvent(unit, UnitMode_End, CheckEventParam12, &event_data);

	if (bRet && bSend && IS_SERVER(unit))
	{
		MSG_SCMD_UNITMODEEND msg;
		msg.id = UnitGetId(unit);
		msg.mode = mode;
		msg.position = UnitGetPosition(unit);
		s_SendUnitMessage(unit, SCMD_UNITMODEEND, &msg);
	}

	return bRet;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam,
	float fDurationInSeconds)
{

	if (AppIsHammer())
	{
		return TRUE;
	}
	
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE((unsigned int)mode < ExcelGetNumRows(NULL, DATATABLE_UNITMODES));

	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return TRUE;
	}

	if ( mode == MODE_DEAD && ! IsUnitDeadOrDying( unit ) )
	{
		UnitDie( unit, NULL );
		return TRUE;
	}

	const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
	if (!unitmode_data)
	{
		return FALSE;
	}

	if (!sCanModeBeSet(unit, mode))
	{
		return FALSE;
	}

#if (ISVERSION(DEVELOPMENT))
	if (GameGetDebugFlag(UnitGetGame(unit), DEBUGFLAG_MODE_TRACE))
	{
		UNIT* debug_unit = GameGetDebugUnit(UnitGetGame(unit));
		if (debug_unit == unit)
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, "client tick %d unit %d [%s] set mode: %s", GameGetTick(UnitGetGame(unit)), (DWORD)UnitGetId(unit), UnitGetDevName(unit), unitmode_data->szName);
		}
	}
#endif

	if(UnitPathIsActiveClient(unit) && !c_UnitModeOverrideClientPathing(mode))
	{
		return FALSE;
	}
	const UNITMODE_DATA* pUnitModeData = UnitModeGetData(mode);
	if(pUnitModeData && pUnitModeData->nState != INVALID_ID)
	{
			c_StateSet(unit, unit, pUnitModeData->nState, NULL, (int)(GAME_TICKS_PER_SECOND*fDurationInSeconds));
	}


	sUnitSetModeInt(unit, mode, unitmode_data, wParam, fDurationInSeconds);

	return TRUE;
}
		
//----------------------------------------------------------------------------
static UNITMODE sgtJumpModes[] = 
{
	MODE_JUMP,
	MODE_JUMP_UP,
	MODE_JUMP_UP_RUN,
	MODE_JUMP_UP_RUN_FORWARD,
	MODE_JUMP_UP_RUN_LEFT,
	MODE_JUMP_UP_RUN_RIGHT,
	MODE_JUMP_UP_RUN_BACK,
	MODE_JUMP_FALL,
	MODE_JUMP_SWING,

	MODE_INVALID		// keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitModeIsJumping(
	UNIT *pUnit)
{
	for (int i = 0; sgtJumpModes[ i ] != MODE_INVALID; ++i)
	{
		UNITMODE eMode = sgtJumpModes[ i ];	
		if (UnitTestMode( pUnit, eMode ) == TRUE)
		{
			return TRUE;
		}
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
static UNITMODE sgtJumpRecoveryModes[] = 
{
	MODE_JUMP_RECOVERY_TO_IDLE,
	MODE_JUMP_RECOVERY_TO_RUN_FORWARD,
	MODE_JUMP_RECOVERY_TO_RUN_BACK,
	MODE_JUMP_RECOVERY_TO_RUN_LEFT,
	MODE_JUMP_RECOVERY_TO_RUN_RIGHT,

	MODE_INVALID		// keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitModeIsJumpRecovering(
	UNIT *pUnit)
{
	for (int i = 0; sgtJumpRecoveryModes[ i ] != MODE_INVALID; ++i)
	{
		UNITMODE eMode = sgtJumpRecoveryModes[ i ];	
		if (UnitTestMode( pUnit, eMode ) == TRUE)
		{
			return TRUE;
		}
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
float JUMP_VELOCITY	= 8.0f; // Eric/Erich tested and approved #s

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
void UnitModeStartJumping(
	UNIT *pUnit)
{

	// this is a hack because jump fall animations are sometimes never cleared
//	c_AnimationEndByMode( unit, MODE_JUMP_FALL );
	
	UNITMODE eJumpMode = MODE_JUMP_UP;
	if (UnitTestMode( pUnit, MODE_RUN )) 
	{
		eJumpMode = MODE_JUMP_UP_RUN_FORWARD;
	}
	else if(UnitTestMode( pUnit, MODE_STRAFE_LEFT ))
	{
		eJumpMode = MODE_JUMP_UP_RUN_LEFT;
	}
	else if(UnitTestMode( pUnit, MODE_STRAFE_RIGHT ))
	{
		eJumpMode = MODE_JUMP_UP_RUN_RIGHT;
	}
	else if (UnitTestMode(pUnit, MODE_BACKUP))
	{
		eJumpMode = MODE_JUMP_UP_RUN_BACK;
	}

	// start the jump	
	c_UnitSetMode( pUnit, eJumpMode );

	// no longer on ground anymore
	UnitSetOnGround( pUnit, FALSE );

	// start jumping
	UnitSetZVelocity( pUnit, JUMP_VELOCITY );

	GAME *pGame = UnitGetGame( pUnit );
	if (GameGetControlUnit( pGame ) == pUnit)
	{
	
		// send message to server
		MSG_CCMD_JUMP_BEGIN tMessage;
		c_SendMessage( CCMD_JUMP_BEGIN, &tMessage );

	}
			
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitModeEndJumping(
	UNIT *pUnit,
	BOOL bWarping )
{
	GAME *pGame = UnitGetGame( pUnit );
	
	// end all jumping modes
	for (int i = 0; sgtJumpModes[ i ] != MODE_INVALID; ++i)
	{
		UNITMODE eMode = sgtJumpModes[ i ];
		if (UnitTestMode( pUnit, eMode ) == TRUE)
		{
			UnitEndMode( pUnit, eMode, 0, FALSE );
		}
	}	
	
	// go into mode for landing based on which way we're moving	
	UNITMODE eLandMode = MODE_JUMP_RECOVERY_TO_IDLE;
	if (UnitTestMode( pUnit, MODE_RUN ))
	{
		eLandMode = MODE_JUMP_RECOVERY_TO_RUN_FORWARD;
	}
	else
	{
		if(AppIsHellgate())
		{
			if (UnitTestMode( pUnit, MODE_BACKUP ))
			{
				eLandMode = MODE_JUMP_RECOVERY_TO_RUN_BACK;
			}
			else if (UnitTestMode( pUnit, MODE_STRAFE_RIGHT ))
			{
				eLandMode = MODE_JUMP_RECOVERY_TO_RUN_RIGHT;
			}
			else if (UnitTestMode( pUnit, MODE_STRAFE_LEFT ))
			{
				eLandMode = MODE_JUMP_RECOVERY_TO_RUN_LEFT;
			}
		}
	}
	
	// set landing mode
	if ( ! bWarping )
	{
		BOOL bHasMode;
		float flDuration = UnitGetModeDuration( pGame, pUnit, eLandMode, bHasMode );
		c_UnitSetMode( pUnit, eLandMode, 0, flDuration );
	}

	// set on ground
	UnitSetOnGround( pUnit, TRUE );
	
	// if its our player, tell server we're done
	if (!bWarping && GameGetControlUnit( pGame ) == pUnit)
	{

		int nCameraViewType = c_CameraGetViewType();
		if(nCameraViewType == VIEW_1STPERSON)
		{
			const UNIT_DATA * pUnitData = UnitGetData(pUnit);
			if(pUnitData && pUnitData->nFootstepLand1stPerson >= 0)
			{
				c_FootstepPlay(pUnitData->nFootstepLand1stPerson, UnitGetPosition(pUnit));
			}
		}

		// send message to server
		MSG_CCMD_JUMP_END tMessage;
		c_SendMessage( CCMD_JUMP_END, &tMessage );

		c_PlayerSendMove( pGame, TRUE );

	}

}

#endif //!SERVER_VERSION

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearStepModes(
	UNIT *unit)
{
	GAME * game = UnitGetGame( unit );
	int nModesMax = ExcelGetNumRows(game, DATATABLE_UNITMODES);
	BOOL bIsControlUnit = (IS_CLIENT( unit ) ? GameGetControlUnit( game ) == unit : FALSE);

	for (int ii = 0; ii < nModesMax; ii++)
	{
		UNITMODE mode = (UNITMODE)ii;
		if (UnitTestMode(unit, mode))
		{
			const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
			// the client updates some of the control unit's modes for you... don't bother clearing those modes
			if (unitmode_data->bSteps && (! bIsControlUnit || ! unitmode_data->bLazyEndForControlUnit ) )
			{
				UnitEndMode(unit, mode, 0, TRUE);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStepTargetReached(
	UNIT* unit)
{
	UnitClearStepModes( unit );
	UnitTriggerEvent(UnitGetGame(unit), EVENT_REACHED_PATH_END, unit, NULL, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITMODE UnitGetMoveMode(
	UNIT* unit)
{
	int nModesMax = ExcelGetNumRows(unit->pGame, DATATABLE_UNITMODES);
	int nVelocityPriorityMax = INVALID_ID;
	UNITMODE eModeMax = MODE_INVALID;
	for (int ii = 0; ii < nModesMax; ii++)
	{
		UNITMODE mode = (UNITMODE)ii;
		if (UnitTestMode(unit, mode))
		{
			const UNITMODE_DATA* unitmode_data = UnitModeGetData( mode );
			if (unitmode_data->nVelocityPriority > nVelocityPriorityMax)
			{
				nVelocityPriorityMax = unitmode_data->nVelocityPriority;
				eModeMax = mode;
			}
		}
	}
	return (UNITMODE)(int)eModeMax;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_MODE_GROUPS	32

static void sConvertGroupsToModes( 
	int nMode,
	BOOL bDontAddSelf,
	int pnModeArray[MAX_UNITMODES], 
	int pnGroupToModeChart[MAX_MODE_GROUPS][MAX_UNITMODES])
{
	ASSERTX_RETURN(MAX_MODE_GROUPS > ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_UNITMODE_GROUPS), "Max unit mode groups exceeded");
	
	int pnGroupArray[MAX_UNITMODES];
	MemoryCopy( pnGroupArray, MAX_UNITMODES * sizeof(int), pnModeArray, sizeof( pnGroupArray ) );

	int nCurrent = 0;
	for ( int i = 0; pnGroupArray[ i ] != INVALID_ID; i++ )
	{
		int nGroup = pnGroupArray[ i ];
		for ( int j = 0; pnGroupToModeChart[ nGroup ][ j ] != INVALID_ID; j++ )
		{
			int nModeToAdd = pnGroupToModeChart[ nGroup ][ j ];
			if ( bDontAddSelf && nModeToAdd == nMode )
				continue;
			pnModeArray[ nCurrent ] = nModeToAdd;
			nCurrent++;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelUnitModesPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	int GroupToModeChart[MAX_MODE_GROUPS][MAX_UNITMODES];
	memset(GroupToModeChart, -1, sizeof(int) * MAX_MODE_GROUPS * MAX_UNITMODES);

	unsigned int modeCount = ExcelGetCountPrivate(table);
	ASSERT_RETFALSE(modeCount <= MAX_UNITMODES);

	for (unsigned int ii = 0; ii < modeCount; ++ii)
	{
		UNITMODE_DATA * mode_data = (UNITMODE_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(mode_data);
		ASSERT_RETFALSE(mode_data->nGroup != INVALID_ID);

		// lookup function pointers
		mode_data->pfnDo = sLookupDoFunction(mode_data->szDoFunction);
		mode_data->pfnClear = sLookupClearFunction(mode_data->szClearFunction);
		mode_data->pfnEnd = sLookupEndFunction(mode_data->szEndFunction);
				
		unsigned int cur = 0;
		while (GroupToModeChart[mode_data->nGroup][cur] != INVALID_ID)
		{
			++cur;
		}
		ASSERT_RETFALSE(cur < MAX_UNITMODES);
		GroupToModeChart[mode_data->nGroup][cur] = (int)ii;

		UNITMODEGROUP_DATA * modegroup_data = (UNITMODEGROUP_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_UNITMODE_GROUPS), mode_data->nGroup);
		ASSERT_CONTINUE(modegroup_data);
		SETBIT(modegroup_data->pdwModeFlags, ii, TRUE);
	}

	for (unsigned int ii = 0; ii < modeCount; ++ii)
	{
		UNITMODE_DATA * mode_data = (UNITMODE_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(mode_data);

		sConvertGroupsToModes(ii, FALSE, mode_data->pnBlockingModes, GroupToModeChart);
		sConvertGroupsToModes(ii, TRUE, mode_data->pnWaitModes,	GroupToModeChart);
		sConvertGroupsToModes(ii, TRUE, mode_data->pnClearModes, GroupToModeChart);
		sConvertGroupsToModes(ii, TRUE, mode_data->pnBlockEndModes,	GroupToModeChart);
	}

#if ISVERSION(DEVELOPMENT)
	for (unsigned int ii = 0; ii < modeCount; ++ii)
	{
		UNITMODE_DATA * mode_data = (UNITMODE_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(mode_data);

		ASSERT_RETFALSE(mode_data->nVelocityIndex < NUM_MODE_VELOCITIES);
	}
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const UNITMODE_DATA* UnitModeGetData(
	int nModeIndex)
{
	return (const UNITMODE_DATA*)ExcelGetData( NULL, DATATABLE_UNITMODES, nModeIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ModeTestModeGroup(
	UNITMODE eMode,
	UNITMODE_GROUP eModeGroup)
{
	UNITMODEGROUP_DATA * pGroupData = (UNITMODEGROUP_DATA *) ExcelGetData( NULL, DATATABLE_UNITMODE_GROUPS, eModeGroup );
	return TESTBIT(pGroupData->pdwModeFlags, eMode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitTestModeGroup(
	UNIT * pUnit,
	UNITMODE_GROUP eModeGroup )
{
	GAME * pGame = UnitGetGame( pUnit );
	UNITMODEGROUP_DATA * pGroupData = (UNITMODEGROUP_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODE_GROUPS, eModeGroup );
	for ( int i = 0; i < sgnUnitModeFlagSize; i++ )
	{
		if ( pUnit->pdwModeFlags[ i ] & pGroupData->pdwModeFlags[ i ] )
			return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_UnitModeOverrideClientPathing(
	UNITMODE eMode)
{
	const UNITMODE_DATA * pUnitModeData = UnitModeGetData(eMode);
	ASSERT_RETFALSE(pUnitModeData);

	return pUnitModeData->bForceClear;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitInitializeAnimations(
	UNIT* unit)
{
	GAME * pGame = UnitGetGame( unit );
	int nModelId = c_UnitGetModelId( unit );
	int nAnimGroup = UnitGetAnimationGroup( unit );
	if ( nAnimGroup )
		c_AppearanceUpdateAnimationGroup( unit, pGame, nModelId, UnitGetAnimationGroup( unit ) );

	ASSERT( MODE_IDLE == 0 );
	DWORD dwPlayAnimFlags = PLAY_ANIMATION_FLAG_ONLY_ONCE | PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_DEFAULT_ANIM;
	c_AnimationPlayByMode( unit, pGame, nModelId, MODE_IDLE, nAnimGroup, 0.0f, dwPlayAnimFlags );

	if ( AppIsHellgate() && UnitTestMode( unit, MODE_DEAD ) && ! UnitTestMode( unit, MODE_DYING ) )
	{
		c_UnitDoDelayedDeath( unit );
	}

	for(int i = MODE_IDLE + 1; i<NUM_UNITMODES; i++)
	{
		if (UnitTestMode(unit, (UNITMODE)i) && !c_AppearanceIsPlayingAnimation(nModelId, i))
		{
			c_UnitSetModeCallback(unit, (UNITMODE)i, 0.0f, TRUE, NULL);
		}
	}

	if (unit->pGfx->nModelIdPaperdoll != INVALID_ID)
	{
		c_AnimationPlayByMode(unit, pGame, unit->pGfx->nModelIdPaperdoll, MODE_IDLE_PAPERDOLL, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS);
	}

	if (unit->pGfx->nModelIdInventory != INVALID_ID)
	{
		c_AnimationPlayByMode(unit, pGame, unit->pGfx->nModelIdInventory, MODE_ITEM_IN_INVENTORY, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS);
	}

	if (unit->pGfx->nModelIdInventoryInspect != INVALID_ID)
	{
		c_AnimationPlayByMode(unit, pGame, unit->pGfx->nModelIdInventoryInspect, MODE_ITEM_IN_INVENTORY, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS);
	}
}
