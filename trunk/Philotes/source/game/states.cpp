//----------------------------------------------------------------------------
// states.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "units.h"
#include "excel_private.h"
#include "dbunit.h"
#include "states.h"
#include "console.h"
#include "gameunits.h"
#include "s_message.h"
#include "skills.h"
#include "skills_priv.h"
#include "combat.h"
#include "c_particles.h"
#include "c_appearance.h"
#include "c_animation.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "missiles.h"
#include "ai.h"
#include "ai_priv.h"
#include "unit_priv.h"		// for USV_CURRENT_VERSION
#include "uix.h"
#include "uix_priv.h"		// for UI_STATE_MSG_DATA
#include "windowsmessages.h"
#include "script.h"
#include "filepaths.h"
#include "pakfile.h"
#include "inventory.h"
#include "e_effect_priv.h"
#include "e_2d.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "wardrobe.h"
#include "items.h"
#include "c_message.h"
#include "unittag.h"
#include "weaponconfig.h"
#include "c_camera.h"
#include "interact.h"
#include "s_quests.h"
#include "teams.h"
#include "c_sound_util.h"
#include "c_backgroundsounds.h"
#include "globalindex.h"
#include "e_region.h"
#include "e_model.h"
#include "fillpak.h"
#include "player.h"
#include "pets.h"
#include "monsters.h"
#include "gamevariant.h"
#include "s_partyCache.h"
#include "c_network.h"
#include "quests.h"

//#define SAVING_STATES
#ifdef SAVING_STATES
#include "definition_priv.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_STATES_PER_STATE_EVENT_TREE											10
#define MAX_TARGETS_PER_STATE													64			// used by ARC_LIGHTNING_MAX_TARGETS
#define MAX_STATES_PER_STATSLIST 4

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
//#ifdef HAMMER
static GAME_EVENT						sgtHammerEventList;
//#endif
BOOL									gDisableXPVisuals =						TRUE;
unsigned int							sgnNumStates = 0;


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATE_EVENT_DATA_STATE
{
	DWORD								dwEventsPerformedMask;
	int									nEventCount;
	int									nStatesPerformedCount;
	int									nStatesPerformed[MAX_STATES_PER_STATE_EVENT_TREE];
	BOOL								bClear;
	BOOL								bWeaponsOnly;
	BOOL								pbApplyToWeapon[ MAX_WEAPONS_PER_UNIT ];
};


//----------------------------------------------------------------------------
struct STATE_EVENT_DATA
{
	UNIT * pUnit;
	STATE_EVENT * pEvent;
	STATS * pStats;
	int nModelId;
	int nOriginalState;
	int nState;
	UNITID idSource;
	WORD wStateIndex;
	BOOL bHadState;
	BOOL bFirstPerson;
	BOOL bAppearanceOverride;
};


//----------------------------------------------------------------------------
// excel post process, determine if we need to set flag based on ancestor
//----------------------------------------------------------------------------
static BOOL sSetTriggerFromParents(
	const STATE_DATA * state_data)
{
	for (unsigned int ii = 0; ii < MAX_STATE_EQUIV; ++ii)
	{
		if (state_data->pnTypeEquiv[ii] == INVALID_ID)
		{
			continue;
		}

		const STATE_DATA * parent_data = StateGetData(NULL, state_data->pnTypeEquiv[ii]);
		// TRAVIS: now that we use VERSION_APP - we can have NULL state data
		if( !parent_data )
		{
			continue;
		}
		if (StateDataTestFlag( parent_data, SDF_TRIGGER_NO_TARGET_EVENT_ON_SET )|| 
			sSetTriggerFromParents(parent_data))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// excel post process, determine if we need to set flag based on ancestor
//----------------------------------------------------------------------------
static BOOL sSetSavePositionFromParents(
	const STATE_DATA * state_data)
{
	for (unsigned int ii = 0; ii < MAX_STATE_EQUIV; ++ii)
	{
		if (state_data->pnTypeEquiv[ii] == INVALID_ID)
		{
			continue;
		}

		const STATE_DATA * parent_data = StateGetData(NULL, state_data->pnTypeEquiv[ii]);
		// TRAVIS: now that we use VERSION_APP - we can have NULL state data
		if( !parent_data )
		{
			continue;
		}

		if (StateDataTestFlag( parent_data, SDF_SAVE_POSITION_ON_SET ) || 
			sSetSavePositionFromParents(parent_data))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int StatesGetNumStates(
	void)
{
	return sgnNumStates;
}


//----------------------------------------------------------------------------
// excel post process
//----------------------------------------------------------------------------
BOOL ExcelStatesPostProcess(
	EXCEL_TABLE * table)
{
	sgnNumStates = ExcelGetCountPrivate(table);
	for (unsigned int ii = 0; ii < StatesGetNumStates(); ii++)
	{
		STATE_DATA * state_data = (STATE_DATA *)ExcelGetDataPrivate(table, ii);
		if( !state_data )
		{
			continue;
		}

		state_data->pdwIsAStateMask = (DWORD *)MALLOCZ(NULL, DWORD_FLAG_SIZE(StatesGetNumStates()) * sizeof(DWORD));

		// TRAVIS - right now we don't want to load events or pulses for states
		// not used by the other app
		BOOL bValidState = (AppIsHellgate() && StateDataTestFlag( state_data, SDF_USED_IN_HELLGATE )) ||
			(AppIsTugboat() && StateDataTestFlag( state_data, SDF_USED_IN_TUGBOAT ));

		if (!bValidState)
		{
			state_data->nDefinitionId = INVALID_ID;
			continue;
		}

		if (state_data->szEventFile[0] != 0)
		{
			state_data->nDefinitionId = DefinitionGetIdByName(DEFINITION_GROUP_STATES, state_data->szEventFile);
		} 
		else 
		{
			state_data->nDefinitionId = INVALID_ID;
		}
		
		if (state_data->tPulseSkill.szSkillPulse[0])
		{
			state_data->tPulseSkill.nSkillPulse = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_SKILLS, state_data->tPulseSkill.szSkillPulse);
			if (state_data->tPulseSkill.nSkillPulse == INVALID_LINK)
			{
				ASSERTV_MSG("Unable to find skill for state pulse '%s'", state_data->tPulseSkill.szSkillPulse);
			}
		}

		if (!StateDataTestFlag( state_data, SDF_TRIGGER_NO_TARGET_EVENT_ON_SET ))
		{
			SETBIT( state_data->pdwFlags, SDF_TRIGGER_NO_TARGET_EVENT_ON_SET, sSetTriggerFromParents(state_data) );
		}
		if (!StateDataTestFlag( state_data, SDF_SAVE_POSITION_ON_SET ))
		{
			SETBIT( state_data->pdwFlags, SDF_SAVE_POSITION_ON_SET, sSetSavePositionFromParents(state_data) );
		}

		unsigned int setcount = 0;
		for (unsigned int jj = 0; jj < StatesGetNumStates(); ++jj)
		{
			if (ExcelTestIsA(NULL, DATATABLE_STATES, jj, ii))
			{
				SETBIT(state_data->pdwIsAStateMask, jj);
				++setcount;
			}
		}
		if (setcount == 1)
		{
			SETBIT(state_data->pdwFlags, SDF_ONLY_STATE);
		}
	}

	if (AppIsHammer())
	{
		sgtHammerEventList.unitnext = &sgtHammerEventList;
		sgtHammerEventList.unitprev = &sgtHammerEventList;
	}

	return TRUE;
};


//----------------------------------------------------------------------------
// callback on freeing the stats table for each row
//----------------------------------------------------------------------------
void ExcelStatesRowFree(
	struct EXCEL_TABLE * table,
	BYTE * rowdata)
{
	STATE_DATA * state_data = (STATE_DATA *)rowdata;
	ASSERT_RETURN(state_data);

	if (state_data->pdwIsAStateMask)
	{
		FREE(NULL, state_data->pdwIsAStateMask);
		state_data->pdwIsAStateMask = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char * sEventGetStringAndIndexForExcel(
	STATE_EVENT * pStateEvent,
	int nType,
	int ** ppnIndex)
{
	switch (nType)
	{
	case STATE_TABLE_REFERENCE_ATTACHMENT:
		*ppnIndex = &pStateEvent->tAttachmentDef.nAttachedDefId;
		return pStateEvent->tAttachmentDef.pszAttached;
	case STATE_TABLE_REFERENCE_EVENT:
		*ppnIndex = &pStateEvent->nExcelIndex;
		return pStateEvent->pszExcelString;
	default:
		ASSERT_RETNULL(0);
	}
	return NULL;
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------
BOOL StateDefinitionXMLPostLoad(
	void * pData,
	BOOL bForceSyncLoad)
{
	STATE_DEFINITION * pDefinition = (STATE_DEFINITION *)pData;
	for (int i=0; i<pDefinition->nEventCount; i++)
	{
		STATE_EVENT * pEvent = &pDefinition->pEvents[i];
		if ( pEvent->eType == INVALID_ID )
			continue;
		const STATE_EVENT_TYPE_DATA * pTypeData = (STATE_EVENT_TYPE_DATA *) ExcelGetData( NULL, DATATABLE_STATE_EVENT_TYPES, pEvent->eType);
		for ( int j = 0; j < NUM_STATE_TABLE_REFERENCES; j++ )
		{
			if ( pTypeData->pnUsesTable[ j ] == DATATABLE_AI_INIT )
			{
				int * pnIndex;
				const char * pszString = sEventGetStringAndIndexForExcel( pEvent, j, &pnIndex );
				if ( ! pszString )
					continue;
				AI_INIT * pAiData = (AI_INIT *)ExcelGetDataByStringIndex(NULL, DATATABLE_AI_INIT, pszString );
				if ( pAiData )
				{
					AIGetDefinition(pAiData);
				}
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateEventInit( 
	STATE_EVENT *pStateEvent)
{
	ASSERTX_RETURN( pStateEvent, "Expected state event" );
	
	pStateEvent->eType = 0;  // set to first row by default
	pStateEvent->dwFlags = 0;
	pStateEvent->fParam = 0.0f;
	memclear( pStateEvent->pszExcelString, MAX_XML_STRING_LENGTH * sizeof( char ) );
	memclear( pStateEvent->pszData, MAX_XML_STRING_LENGTH * sizeof( char ) );
	pStateEvent->nExcelIndex = INVALID_LINK;
	pStateEvent->nData = 0;
	e_AttachmentDefInit( pStateEvent->tAttachmentDef );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sEventGetIndexForExcel(
	const STATE_EVENT * stateevent,
	int type)
{
	ASSERT_RETINVALID(stateevent);

	switch (type)
	{
	case STATE_TABLE_REFERENCE_ATTACHMENT:
		return stateevent->tAttachmentDef.nAttachedDefId;
	case STATE_TABLE_REFERENCE_EVENT:
		return stateevent->nExcelIndex;
	default:
		ASSERT_RETINVALID(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitInitStateFlags(
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(unit->pdwStateFlags == NULL);

	unit->pdwStateFlags = (DWORD *)GMALLOCZ(UnitGetGame(unit), DWORD_FLAG_SIZE(StatesGetNumStates()) * sizeof(DWORD));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitFreeStateFlags(
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	if (unit->pdwStateFlags)
	{
		GFREE(UnitGetGame(unit), unit->pdwStateFlags);
		unit->pdwStateFlags = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStatsChangeState(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(wStat == STATS_STATE);
	unsigned int num_states = StatesGetNumStates();
	ASSERT_RETURN(dwParam < num_states);
	int state = (int)dwParam;
	REF(data);
	REF(bSend);

	if (nValueOld == 0 && nValueNew > 0)
	{
		// state is on
		ASSERT_RETURN(unit->pdwStateFlags);
		SETBIT(unit->pdwStateFlags, state);
//		const STATE_DATA * state_data = StateGetData(UnitGetGame(unit), state);
//		ASSERT_RETURN(state_data);
//		trace("UNIT [%d: %s]  STATE [%s]  ON\n", UnitGetId(unit), UnitGetDevName(unit), state_data->szName);
	}
	else if (nValueOld != 0 && nValueNew == 0)
	{
		// state is off
		ASSERT_RETURN(unit->pdwStateFlags);
		CLEARBIT(unit->pdwStateFlags, state);
//		const STATE_DATA * state_data = StateGetData(UnitGetGame(unit), state);
//		ASSERT_RETURN(state_data);
//		trace("UNIT [%d: %s]  STATE [%s]  OFF\n", UnitGetId(unit), UnitGetDevName(unit), state_data->szName);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasExactState( 
	UNIT * unit,
	int state)
{
	ASSERT_RETFALSE((unsigned int)state < StatesGetNumStates());
	ASSERT_RETFALSE(unit->pdwStateFlags);
	return (TESTBIT(unit->pdwStateFlags, state));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS * sGetStatsForState( 
	UNIT* unit, 
	int nState, 
	UNITID idSource,
	WORD wIndex,
	BOOL bAnySource = FALSE,
	int nSkill = INVALID_ID )
{
	if ( ! unit )
		return NULL;

	STATS* stats = StatsGetModList(unit, SELECTOR_STATE);
	while (stats)
	{
		if (StatsGetStat(stats,  STATS_STATE, nState) > 0)
		{
			if ( bAnySource )
				return stats;
			if ( nSkill == INVALID_ID ||
				 StatsGetStat( stats, STATS_STATE_SOURCE_SKILL_LEVEL, nSkill ) )
			{
				if (wIndex == 0)
				{
					STATS_ENTRY stats_entries[1];
					if (StatsGetRange(stats, STATS_STATE_SOURCE, 0, stats_entries, 1) != 0)
					{
						if ((UNITID)stats_entries[0].value == idSource)
						{
							return stats;
						}
					}
				} 
				else if ((UNITID)StatsGetStat(stats, STATS_STATE_SOURCE, wIndex) == idSource) 
				{
					return stats;
				}
			}
		}
		stats = StatsGetModNext(stats, SELECTOR_STATE);
	}
	return NULL;
}

STATS* GetStatsForStateGivenByUnit( int nState, UNIT *unit, UNITID idSource, WORD wIndex, BOOL bAnySource, int nSkill )
{
	return sGetStatsForState( unit, nState, idSource, wIndex, bAnySource, nSkill );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasState( 
	GAME * game,
	UNIT * unit,
	int state)
{
	ASSERT_RETFALSE(unit);

	const STATE_DATA * state_data = StateGetData(game, state);
	if (state_data == NULL)
	{
		return FALSE;
	}
	if (TESTBIT(state_data->pdwFlags, SDF_ONLY_STATE))
	{
		return UnitHasExactState(unit, state);
	}
	ASSERT_RETFALSE(state_data->pdwIsAStateMask);
	ASSERT_RETFALSE(unit->pdwStateFlags);

	unsigned int arraysize = DWORD_FLAG_SIZE(StatesGetNumStates());
	for (unsigned int ii = 0; ii < arraysize; ++ii)
	{
		if (unit->pdwStateFlags[ii] & state_data->pdwIsAStateMask[ii])
		{
#if ISVERSION(DEVELOPMENT)
			for (unsigned int jj = 0; jj < BITS_PER_DWORD; ++jj)
			{
				if (TESTBIT(unit->pdwStateFlags[ii], jj) && TESTBIT(state_data->pdwIsAStateMask[ii], jj))
				{
					int s = ii * BITS_PER_DWORD + jj;
					ASSERT(StateIsA(s, state));
				}
			}
#endif
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetFirstStateOfType( 
	GAME * game,
	UNIT * unit,
	int state)
{
	ASSERT_RETFALSE(unit);

	const STATE_DATA * state_data = StateGetData(game, state);
	ASSERT_RETFALSE(state_data);
	if (TESTBIT(state_data->pdwFlags, SDF_ONLY_STATE))
	{
		return UnitHasExactState(unit, state);
	}
	ASSERT_RETFALSE(state_data->pdwIsAStateMask);
	ASSERT_RETFALSE(unit->pdwStateFlags);

	unsigned int arraysize = DWORD_FLAG_SIZE(StatesGetNumStates());
	for (unsigned int ii = 0; ii < arraysize; ++ii)
	{
		if (unit->pdwStateFlags[ii] & state_data->pdwIsAStateMask[ii])
		{
			for (unsigned int jj = 0; jj < BITS_PER_DWORD; ++jj)
			{
				if (TESTBIT(unit->pdwStateFlags[ii], jj) && TESTBIT(state_data->pdwIsAStateMask[ii], jj))
				{
					int s = ii * BITS_PER_DWORD + jj;
#if ISVERSION(DEVELOPMENT)
					ASSERT(StateIsA(s, state));
#endif
					return s;
				}
			}
		}
	}
	return INVALID_ID;
}


//****************************************************************************
// Event Handling
//****************************************************************************

static BOOL sHandleSpin( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData )
{
	STATE_EVENT * pStateEvent = (STATE_EVENT *) tEventData.m_Data1;

	// do this again
//#ifdef HAMMER
	if (AppIsHammer())
	{
		GameEventRegister( pGame, sHandleSpin, NULL, &sgtHammerEventList, &EVENT_DATA((DWORD_PTR)pStateEvent, tEventData.m_Data2 ), 1);
	}
	else
	{
//#else
		UnitRegisterEventTimed( pUnit, sHandleSpin, &EVENT_DATA((DWORD_PTR)pStateEvent, tEventData.m_Data2 ), 1);
	}
//#endif

	ASSERT_RETFALSE( pStateEvent->fParam != 0.0f );
	float fSpinSpeed = GAME_TICK_TIME * pStateEvent->fParam;
	MATRIX mRotation;
	MatrixTransform( &mRotation, NULL, fSpinSpeed );

//#ifdef HAMMER
	if (AppIsHammer())
	{
		int nModelId = (int)tEventData.m_Data2;
		const MATRIX * pmWorld = e_ModelGetWorld( nModelId );
		MatrixMultiply( &mRotation, &mRotation, pmWorld );
		VECTOR vPosition( 0 );
		MatrixMultiply( &vPosition, &vPosition, &mRotation );
		c_ModelSetLocation( nModelId, &mRotation, vPosition, INVALID_ID );
	}
//#else
	else
	{
		ASSERT_RETFALSE( pUnit );
		VECTOR vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		MatrixMultiplyNormal( &vFaceDirection, &vFaceDirection, &mRotation );
		UnitSetFaceDirection( pUnit, vFaceDirection, TRUE );
	}
//#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSendAddTargetMsg( 
	GAME * pGame,
	UNITID idSource, 
	UNITID idUnit, 
	UNITID idTarget, 
	int nState, 
	WORD wStateIndex )
{
	MSG_SCMD_STATEADDTARGET tMsg;
	tMsg.idStateSource = idSource;
	tMsg.idStateHolder = idUnit;
	tMsg.idTarget = idTarget;
	tMsg.wState = (WORD) nState;
	tMsg.wStateIndex = wStateIndex;

	UNIT *pSource = UnitGetById( pGame, idUnit );
	UNIT *pTarget = UnitGetById( pGame, idTarget );
	const int NUM_UNITS = 2;
	UNIT *pUnits[ NUM_UNITS ] = { pSource, pTarget };	
	s_SendMultiUnitMessage( pUnits, NUM_UNITS, SCMD_STATEADDTARGET, &tMsg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSendAddTargetLocationMsg( 
	GAME * pGame,
	UNITID idSource, 
	UNITID idUnit, 
	const VECTOR & vTarget, 
	int nState, 
	WORD wStateIndex )
{
	MSG_SCMD_STATEADDTARGETLOCATION tMsg;
	tMsg.idStateSource = idSource;
	tMsg.idStateHolder = idUnit;
	tMsg.vTarget = vTarget;
	tMsg.wState = (WORD) nState;
	tMsg.wStateIndex = wStateIndex;

	UNIT *pSource = UnitGetById( pGame, idSource );
	s_SendUnitMessage(pSource, SCMD_STATEADDTARGETLOCATION, &tMsg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleArcLightning( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData )
{
	STATE_EVENT * pStateEvent = (STATE_EVENT *)tEventData.m_Data1;
	REF(pStateEvent);
	int nState = (int)tEventData.m_Data2;
	UNITID idSource = (UNITID)tEventData.m_Data3;
	UNIT * pSourceUnit = UnitGetById( pGame, idSource );
	if ( ! pSourceUnit )
	{
		// remove the nState
		return FALSE;
	}

	UNITID idWeapon = (UNITID)tEventData.m_Data4;
	UNIT * pWeapon = UnitGetById(pGame, idWeapon);

	STATS * pStats = sGetStatsForState( pUnit, nState, idSource, 0 );
	if ( ! pStats )
		return FALSE;

	// grab stats from the list.  Make sure that we have the data to arc this thing
	int nArcJumps = StatsGetStat( pStats, STATS_STATE_ARC_JUMPS );
	if ( nArcJumps == -1 )
		return FALSE;
	int nMaxTargets;
	int nArcDuration;
	float fRange = ARC_LIGHTNING_RANGE; 
	if ( nArcJumps == 0 )
	{ // we must be the first one to be hit.  So, grab the originals from the source unit
		nArcJumps = UnitGetStat( pSourceUnit, STATS_STATE_ARC_JUMPS );
		if ( nArcJumps <= 0 )
			return FALSE;
		nMaxTargets = UnitGetStat( pSourceUnit, STATS_STATE_ARC_TARGETS );
		if ( nMaxTargets <= 0 )
			return FALSE;
		nArcDuration = UnitGetStat( pSourceUnit, STATS_STATE_ARC_DURATION );
		if ( nArcDuration == 0 )
			return FALSE;
	} else {
		nMaxTargets = StatsGetStat( pStats, STATS_STATE_ARC_TARGETS );
		if ( nMaxTargets <= 0 )
			return FALSE;
		nArcDuration = StatsGetStat( pStats, STATS_STATE_ARC_DURATION );
		if ( nArcDuration == 0 )
			return FALSE;
	}

#ifdef HAVOK_ENABLED
	HAVOK_IMPACT tImpact;
	HavokImpactInit( pGame, tImpact, 10.0f, UnitGetCenter( pUnit ), NULL );
	tImpact.dwFlags = HAVOK_IMPACT_FLAG_IGNORE_SCALE;
#endif
	UNITID pnNearest[MAX_TARGETS_PER_STATE];

	VECTOR vSearchLocation = UnitGetCenter( pUnit );
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_RANDOM );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_ANY;
	tTargetingQuery.pSeekerUnit = pSourceUnit;
	tTargetingQuery.pvLocation = &vSearchLocation;
	tTargetingQuery.pnUnitIds = pnNearest;
	tTargetingQuery.nUnitIdMax = ARC_LIGHTNING_MAX_TARGETS;
	tTargetingQuery.fMaxRange = fRange;

	BOOL bQueryRun = FALSE;
	int nFirstToSearch = 0;

	nMaxTargets = MIN(nMaxTargets, MAX_TARGETS_PER_STATE);

	UNITID pnCurrent[MAX_TARGETS_PER_STATE];
	for ( int i = 0; i < nMaxTargets; i++ )
	{
		pnCurrent[ i ] =  StatsGetStat( pStats, STATS_STATE_TARGET, i );
	}

	for ( int i = 0; i < nMaxTargets; i++ )
	{
		if ( pnCurrent[ i ] == INVALID_ID )
		{// find a new pTarget - maybe
			if ( ! bQueryRun )
			{
				SkillTargetQuery( pGame, tTargetingQuery );
				bQueryRun = TRUE;
			}
			for ( int j = nFirstToSearch; j < tTargetingQuery.nUnitIdCount; j++ )
			{
				nFirstToSearch ++;

				BOOL bAlreadyArced = FALSE;
				if ( pnNearest[ j ] == UnitGetId( pUnit ) )
					bAlreadyArced = TRUE;
				else
				{
					for ( int k = 0; k < nMaxTargets; k++ )
					{
						if ( pnNearest[ j ] == pnCurrent[ k ] )
						{
							bAlreadyArced = TRUE;
							break;
						}
					}
				}

				if ( ! bAlreadyArced )
				{
					pnCurrent[ i ] = pnNearest[ j ];
					break;
				}
			}

			if ( pnCurrent[ i ] == INVALID_ID )
				continue;

			UNIT * pTargetUnit = UnitGetById( pGame, pnCurrent[ i ] );
			ASSERT_RETFALSE( pTargetUnit ); // we got this from the skill query - it shouldn't be non-existant
			
			if (UnitHasExactState(pTargetUnit, nState)) // this guy already is hit by this nState
			{
				continue;
			}

			UNIT * pWeapons[MAX_WEAPONS_PER_UNIT];
			memclear(pWeapons, MAX_WEAPONS_PER_UNIT * sizeof(UNIT*));
			pWeapons[0] = pWeapon;
#if !ISVERSION(CLIENT_ONLY_VERSION)
			// attack pTarget from source - maybe we need to say what weapon we are using?
			s_UnitAttackUnit( pSourceUnit, pTargetUnit, pWeapons, 0
#ifdef HAVOK_ENABLED
				,&tImpact 
#endif
				);
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

			// regrab the stats - they may have dissappeared.
			pStats = sGetStatsForState( pUnit, nState, idSource, 0 );

			// if pTarget took the nState - hopefully not from someone else....
			if (UnitHasExactState(pTargetUnit, nState) && pStats)
			{
				// save pTarget
				StatsSetStat( pGame, pStats, STATS_STATE_TARGET, i, pnCurrent[ i ] );

				// set stats on pTarget for continuing to arc
				STATS * pTargetStats = sGetStatsForState( pTargetUnit, nState, idSource, 0 );
				if ( pTargetStats )
				{
					if ( nArcJumps > 1 )
					{
						StatsSetStat( pGame, pTargetStats, STATS_STATE_ARC_JUMPS,		nArcJumps - 1 );
						StatsSetStat( pGame, pTargetStats, STATS_STATE_ARC_DURATION,	nArcDuration );
						StatsSetStat( pGame, pTargetStats, STATS_STATE_ARC_TARGETS,		nMaxTargets );
					} else {
						StatsSetStat( pGame, pTargetStats, STATS_STATE_ARC_JUMPS,		-1 );
					}
					StatsSetTimer( pGame, pTargetUnit, pTargetStats, nArcDuration );

					s_sSendAddTargetMsg( pGame, idSource, UnitGetId( pUnit ), pnCurrent[ i ], nState, 0 );
				}
			}
		} 
		else
		{
			UNIT * pTargetUnit = UnitGetById( pGame, pnCurrent[ i ] );
			BOOL bRetarget = RandSelect(pGame, 1, 3);
			//													we should probably check to see that we are the source of this
			if ( pTargetUnit && 
				! IsUnitDeadOrDying( pTargetUnit ) && 
				UnitHasExactState(pTargetUnit, nState) &&
				! bRetarget )
			{
#if !ISVERSION(CLIENT_ONLY_VERSION)
				// attack pTarget from source - maybe we need to say what weapon we are using?
				s_UnitAttackUnit( pSourceUnit, pTargetUnit, NULL, 0
#ifdef HAVOK_ENABLED
					,&tImpact 
#endif
					);
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
			} else {
				// tell client to remove the rope
				MSG_SCMD_STATEREMOVETARGET tMsg;
				tMsg.idStateSource = idSource;
				tMsg.idStateHolder = UnitGetId( pUnit );
				tMsg.idTarget = UnitGetId( pTargetUnit );
				tMsg.wState = (WORD) nState;
				tMsg.wStateIndex = 0;
				
				const int NUM_UNITS = 2;
				UNIT *pUnits[ NUM_UNITS ] = { pSourceUnit, pTargetUnit };	
				s_SendMultiUnitMessage( pUnits, NUM_UNITS, SCMD_STATEREMOVETARGET, &tMsg);
				
				StatsSetStat( pGame, pStats, STATS_STATE_TARGET, i, INVALID_ID );
			}
		}
	}

	UnitRegisterEventTimed( pUnit, sHandleArcLightning, &tEventData, 3 );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventArcLightningClear( 
	GAME* pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( tData.pUnit && pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) )
	UnitUnregisterTimedEvent( tData.pUnit, sHandleArcLightning, NULL, &EVENT_DATA((DWORD_PTR)tData.pEvent, tData.nOriginalState, tData.idSource ) );

	ASSERT_RETURN( tData.pStats );
	UNIT * pSource = UnitGetById( pGame, tData.idSource );
	STATS_ENTRY pStatsEntries[MAX_TARGETS_PER_STATE];
	int nMaxTargets = MIN(ARC_LIGHTNING_MAX_TARGETS, MAX_TARGETS_PER_STATE);
	int nTargets = StatsGetRange( tData.pStats, STATS_STATE_TARGET, 0, pStatsEntries, nMaxTargets);

	for ( int i = 0; i < nTargets; i++ )
	{	
		UNITID idTarget = pStatsEntries[ i ].value;
		UNIT * pTarget = UnitGetById( pGame, idTarget );
		if (!pTarget)
		{
			continue;
		}
		s_StateClear( pTarget, tData.idSource, tData.nOriginalState, 0 );

		MSG_SCMD_STATEREMOVETARGET tMsg;
		tMsg.idStateSource = tData.idSource;
		tMsg.idStateHolder = UnitGetId( tData.pUnit );
		tMsg.idTarget = idTarget;
		tMsg.wState = DOWNCAST_INT_TO_WORD(tData.nOriginalState);
		tMsg.wStateIndex = tData.wStateIndex;

		const int NUM_UNITS = 2;
		UNIT *pUnits[ NUM_UNITS ] = { pSource, pTarget };	
		s_SendMultiUnitMessage( pUnits, NUM_UNITS, SCMD_STATEREMOVETARGET, &tMsg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventModifyAI(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN(pGame && tData.pUnit);
	
	int nAi = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nAi != INVALID_ID );

	AI_INIT * pAiData = (AI_INIT *)ExcelGetData(pGame, DATATABLE_AI_INIT, nAi );
	if (!pAiData)
	{
		return;
	}

	STATS * pStats = sGetStatsForState(tData.pUnit, tData.nOriginalState, tData.idSource, 0);
	if (!pStats)
	{
		return;
	}

	AI_InsertTable( tData.pUnit, pAiData->nTable, pStats );
	AI_Register( tData.pUnit, FALSE, 1 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventModifyAIClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	AI_RemoveTableByStats( tData.pUnit, tData.pStats );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetTargetToSource( 
	GAME * pGame, 
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit && UnitIsA(tData.pUnit, UNITTYPE_CREATURE) )
	{
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT);
		UNIT * pOldTarget = UnitGetAITarget(tData.pUnit, dwAIGetTargetFlags);
		UNIT * pTarget = UnitGetById(pGame, tData.idSource);
		if (!pOldTarget || (pTarget && UnitsGetDistanceSquared(tData.pUnit, pTarget) < UnitsGetDistanceSquared(tData.pUnit, pOldTarget)))
		{
			UnitSetAITarget(tData.pUnit, tData.idSource, TRUE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearTargetToSource( 
	GAME * pGame, 
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit && UnitIsA(tData.pUnit, UNITTYPE_CREATURE) )
	{
		UnitSetAITarget(tData.pUnit, INVALID_ID, TRUE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplySkillStatsOnPulse(
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * , 
	struct EVENT_DATA* pEventData, 
	void * data, 
	DWORD dwId )
{
	ASSERT_RETURN( pUnit );
	int nStateId = (int)pEventData->m_Data1;
	WORD wStateIndex = (WORD) pEventData->m_Data2;
	UNITID idSource = (UNITID)pEventData->m_Data3;
	int nSkill = (int)pEventData->m_Data4;

	ASSERT_RETURN( nStateId != INVALID_ID );
	ASSERT_RETURN( nSkill != INVALID_ID );

	STATS * pStatsList = sGetStatsForState( pUnit, nStateId, idSource, wStateIndex, FALSE, nSkill );
	if ( ! pStatsList )
	{
		UnitUnregisterEventHandler( pGame, pUnit, EVENT_TEMPLAR_PULSE, dwId );
		return;
	}

	int nSkillLevel = SkillGetLevel( pUnit, nSkill );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulse, &code_len );
	if ( code_ptr )
	{
		VMExecI(pGame, pUnit, pStatsList, nSkill, nSkillLevel, nSkill, nSkillLevel, nStateId, code_ptr, code_len );
	}

	if ( IS_SERVER( pGame ) )
	{
		code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulseServerOnly, &code_len );
		if ( code_ptr )
		{
			VMExecI(pGame, pUnit, pStatsList, nSkill, nSkillLevel, nSkill, nSkillLevel, nStateId, code_ptr, code_len );
		}

		if(pSkillData->nPulseSkill != INVALID_ID)
		{
			SkillStartRequest(pGame, pUnit, pSkillData->nPulseSkill, INVALID_ID, VECTOR(0), 
				SKILL_START_FLAG_INITIATED_BY_SERVER, SkillGetSeed(pGame, pUnit, NULL, pSkillData->nPulseSkill));
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoEvents( 
	GAME * pGame,
	STATE_EVENT_DATA & tData,
	STATE_EVENT_DATA_STATE & tStateEventsState);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoParentEvents( 
	GAME * pGame,
	STATE_EVENT_DATA & tData,
	STATE_EVENT_DATA_STATE & tStateEventsState)
{
	const STATE_DATA * pStateData = StateGetData(pGame, tData.nState);
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !pStateData )
	{
		return;
	}

	if(!StateDataTestFlag( pStateData, SDF_EXECUTE_PARENT_EVENTS ))
	{
		return;
	}

	for(int i=0; i<MAX_STATE_EQUIV; i++)
	{
		int nParentState = pStateData->pnTypeEquiv[i];
		if(nParentState < 0)
		{
			break;
		}

		for(int j=0; j<tStateEventsState.nStatesPerformedCount; j++)
		{
			if(tStateEventsState.nStatesPerformed[j] == nParentState)
			{
				continue;
			}
		}

		STATE_EVENT_DATA tDataParent;
		memclear(&tDataParent, sizeof(STATE_EVENT_DATA));
		tDataParent.pStats = tData.pStats;
		tDataParent.bFirstPerson = tData.bFirstPerson;
		tDataParent.bAppearanceOverride = tData.bAppearanceOverride;
		tDataParent.bHadState = tData.bHadState;
		tDataParent.idSource = tData.idSource;
		tDataParent.nModelId = tData.nModelId;
		tDataParent.nState = nParentState;
		tDataParent.nOriginalState = tData.nOriginalState;
		tDataParent.pUnit = tData.pUnit;
		tDataParent.wStateIndex = tData.wStateIndex;

		sStateDoEvents(pGame, tDataParent, tStateEventsState);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sStateCheckConditionOnClear(
	const STATE_EVENT_DATA & tData)
{
	if(tData.pEvent->dwFlags & STATE_EVENT_FLAG_ON_CLEAR &&
		tData.pEvent->dwFlags & STATE_EVENT_FLAG_CHECK_CONDITION_ON_CLEAR)
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoEventOnWeapons( 
	GAME * pGame,
	STATE_EVENT_DATA & tData,
	STATE_EVENT_DATA_STATE & tStateEventsState,
	PFN_STATE_EVENT_HANDLER pfnEventHandler )
{
	ASSERT_RETURN( pfnEventHandler );

	UNIT * pUnitOrig = tData.pUnit;
	int nModelOrig   = tData.nModelId;
	int nWardrobe = tData.pUnit ? UnitGetWardrobe( tData.pUnit ) : INVALID_ID;
	ONCE
	{
		if ( nWardrobe == INVALID_ID )
			break;

		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			if ( tStateEventsState.bWeaponsOnly && ! tStateEventsState.pbApplyToWeapon[ i ] )
				continue;

			UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
			if ( ! pWeapon )
				continue;

			tData.pUnit = pWeapon;
			if ( IS_CLIENT( pUnitOrig ) )
				tData.nModelId = c_UnitGetModelId( pWeapon );

			if ( tData.pEvent->tCondition.nType == INVALID_ID ||
				AppIsHammer() ||
				(tData.pEvent->dwFlags & STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS) != 0 ||
				ConditionEvalute( tData.pUnit, tData.pEvent->tCondition, INVALID_ID, INVALID_ID, tData.pStats ) )
			{
				pfnEventHandler( pGame, tData );
			}
		}
	}
	tData.pUnit = pUnitOrig;
	tData.nModelId = nModelOrig;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoEvents( 
	GAME * pGame,
	STATE_EVENT_DATA & tData,
	STATE_EVENT_DATA_STATE & tStateEventsState)
{
	sStateDoParentEvents(pGame, tData, tStateEventsState);

	const STATE_DEFINITION * pStateDef = StateGetDefinition( tData.nState );
	if ( ! pStateDef )
		return;

	if ( ! pStateDef->nEventCount )
		return;

	BOOL bIsControlUnit = tData.pUnit && IS_CLIENT( tData.pUnit ) && tData.pUnit == GameGetControlUnit( pGame );
	ASSERT_RETURN( tData.pUnit || tData.nModelId != INVALID_ID );

	ASSERT_RETURN( pStateDef->nEventCount <= 32 );

	BOOL bStatsHaveParent = tData.pStats ? (StatsGetParent( tData.pStats ) != NULL) : TRUE;

	int nSkill = INVALID_ID;
	PARAM dwParam = 0;
	if ( tData.pStats && StatsGetStatAny( tData.pStats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam ) )
	{
		nSkill = dwParam;
	}
	ASSERTX_RETURN( pStateDef->nEventCount < 32, pStateDef->tHeader.pszName );
	for ( int i = 0; i < pStateDef->nEventCount; i++ )
	{
		tData.pEvent =  &pStateDef->pEvents[ i ];
		tStateEventsState.nEventCount++;
		ASSERTX( tStateEventsState.nEventCount < 32, pStateDef->tHeader.pszName );

		if((!tStateEventsState.bClear && !sStateCheckConditionOnClear(tData)) ||
			(tStateEventsState.bClear && sStateCheckConditionOnClear(tData)))
		{
			if ( tData.pEvent->tCondition.nType != INVALID_ID &&
				! AppIsHammer() &&
				(tData.pEvent->dwFlags & STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS) == 0 &&
				! ConditionEvalute( tData.pUnit, tData.pEvent->tCondition, INVALID_ID, INVALID_ID, tData.pStats ) )
			{
				continue;
			}
		}

		ASSERT_CONTINUE( tStateEventsState.nEventCount < 32 );
		DWORD dwEventsPerformedMask = (1 << tStateEventsState.nEventCount);
		if ( tStateEventsState.bClear )
		{
			if ( (tStateEventsState.dwEventsPerformedMask & dwEventsPerformedMask) == 0 )
				continue; // we didn't set the state, so don't clear it
		} 
		else if ( ! tStateEventsState.bWeaponsOnly ) 
		{
			if ( (tStateEventsState.dwEventsPerformedMask & dwEventsPerformedMask) != 0 )
				continue; // we have already done this event for this state - it can happen when handling appearance loading and async
		}
		const STATE_EVENT_TYPE_DATA * pTypeData = StateEventTypeGetData( pGame, tData.pEvent->eType );
		ASSERT_CONTINUE(pTypeData);

		if ( pTypeData->bClientOnly && IS_SERVER( pGame ) )
			continue;
		if ( pTypeData->bServerOnly && IS_CLIENT( pGame ) )
			continue;

		if ( tData.pUnit && tStateEventsState.bClear && ! pTypeData->bCanClearOnFree && UnitTestFlag( tData.pUnit, UNITFLAG_JUSTFREED ) )
			continue;

		if ( tData.bFirstPerson && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_FIRST_PERSON) == 0 )
			continue;
		if (!tData.bFirstPerson && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_FIRST_PERSON) != 0 && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_IGNORE_CAMERA) == 0 )
			continue;

		if ( tData.bAppearanceOverride && !pTypeData->bApplyToAppearanceOverride )
			continue;

		if ( (!bIsControlUnit && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_CONTROL_UNIT_ONLY) != 0) || 
				(bIsControlUnit && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_NOT_CONTROL_UNIT) != 0))
			continue;

		if ( pTypeData->bControlUnitOnly && ! bIsControlUnit )
			continue;

		if ( tData.pUnit && (tData.pEvent->dwFlags & STATE_EVENT_FLAG_OWNED_BY_CONTROL) != 0 )
		{
			UNIT * pControlUnit = GameGetControlUnit( pGame );
			if ( UnitGetOwnerUnit( tData.pUnit ) != pControlUnit && // maybe we should check for group here too?
				 UnitGetContainer( tData.pUnit ) != pControlUnit )
				 continue;
		}

		if ( pTypeData->bRequiresLoadedGraphics )
		{
			// EXCEPTION: In demo mode for states applied to the control unit, don't care about appearance loaded
			if ( ! ( bIsControlUnit && AppCommonGetDemoMode() ) )
			{
				// don't do this for things with no appearance! ( Town Portal warps are a good example )
				if( ! c_GetToolMode() && 
					( ! UnitTestFlag( tData.pUnit, UNITFLAG_STATE_GRAPHICS_APPLIED ) ) &&
					UnitGetAppearanceDefId( tData.pUnit ) != INVALID_ID )
				{
					// make sure that we remember we need to force reapplication of this
					// event after we come back around, so we can force it - otherwise,
					// several events ( like AddAttachment ) will note that the state
					// was already set ( this time, when we're deferring ) and will ignore
					// the attach, resulting in missing particles, etc.
					tData.pEvent->dwFlags |= STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD;
					continue; // we should/will come back when the appearance definition is loaded
				}
			}
		}

		// this event had to be deferred until after an appearance load - 
		// let's force it, to make sure it isn't ignored because the state already existed.
		// Then we'll wipe the reapplication flag, so it doesn't happen multiple times.
		if( ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD ) )
		{
			tData.pEvent->dwFlags &= ~STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD;
			tData.pEvent->dwFlags |= STATE_EVENT_FLAG_FORCE_NEW;
		}
		// get event handler for set or clear of state
		PFN_STATE_EVENT_HANDLER pfnEventHandler = pTypeData->pfnSetEventHandler;
		if (tStateEventsState.bClear)
		{
			pfnEventHandler = pTypeData->pfnClearEventHandler;
		}
	
		if ( !tStateEventsState.bClear && !tStateEventsState.bWeaponsOnly )
		{ // don't mark us as having done the event until we know that are doing it.
			// We do this BEFORE the check for a NULL pfnEventHandler, because we have some events
			// that *only* have a clear handler, and legitimately DON'T have a set handler.
			// So, we mark it as run even if it didn't have a set handler
			tStateEventsState.dwEventsPerformedMask |= dwEventsPerformedMask;
		} // this has to happen before the "on clear" check

		// run event handler
		if (pfnEventHandler == NULL)
			continue;

		// don't do events marked for "on clear" if we are not clearing the state
		if ( pTypeData->dwFlagsUsed & STATE_EVENT_FLAG_ON_CLEAR )
		{
			if ( (( tData.pEvent->dwFlags & STATE_EVENT_FLAG_ON_CLEAR ) != 0) != tStateEventsState.bClear)
				continue;
		}

		// if we are targeting weapons with this event, then iterate through the weapons
		if ((pTypeData->dwFlagsUsed & STATE_EVENT_FLAG_ON_WEAPONS) != 0 &&  
			(tData.pEvent->dwFlags & STATE_EVENT_FLAG_ON_WEAPONS ) != 0)
		{
			sStateDoEventOnWeapons( pGame, tData, tStateEventsState, pfnEventHandler );
		} 
		else if ( !tStateEventsState.bWeaponsOnly )
		{
		// otherwise, just do the event to the weapon
			pfnEventHandler( pGame, tData );
		}

		if ( pTypeData->bRefreshStatsPointer && bStatsHaveParent )
		{
			tData.pStats = sGetStatsForState( tData.pUnit, tData.nState, tData.idSource, tData.wStateIndex, FALSE, nSkill );
		}
		
	}
	
	ASSERTX_RETURN(tStateEventsState.nStatesPerformedCount < MAX_STATES_PER_STATE_EVENT_TREE, "Maximum number of parent-executed states has been reached!");
	tStateEventsState.nStatesPerformed[tStateEventsState.nStatesPerformedCount] = tData.nState;
	tStateEventsState.nStatesPerformedCount++;
}

//----------------------------------------------------------------------------
// Hammer only
//----------------------------------------------------------------------------
void StateDoEvents( 
	int nModelId,
	int nState,
	WORD wStateIndex,
	BOOL bHadState,
	BOOL bFirstPerson )
{
	ASSERT_RETURN( AppIsHammer() );

	STATE_EVENT_DATA tData;
	memclear(&tData, sizeof(STATE_EVENT_DATA));
	tData.bFirstPerson = bFirstPerson;
	tData.bHadState = bHadState;
	tData.idSource = INVALID_ID;
	tData.nModelId = nModelId;
	tData.nState = nState;
	tData.nOriginalState = nState;

	tData.wStateIndex = wStateIndex;

	STATE_EVENT_DATA_STATE tStateState;
	memclear(&tStateState, sizeof(STATE_EVENT_DATA_STATE));
	tStateState.bClear = FALSE;
	tStateState.bWeaponsOnly = FALSE;

	GAME * pGame = AppGetCltGame();
	sStateDoEvents( pGame, tData, tStateState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoEvents( 
	UNIT* pUnit,
	UNIT* pSource,
	int nState,
	WORD wStateIndex,
	BOOL bHadState )
{
	ASSERT_RETURN(pUnit);
	GAME* pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(pGame);

	int nModelId = IS_CLIENT( pUnit ) ? c_UnitGetModelIdThirdPerson( pUnit ) : INVALID_ID;

	UNITID idSource = UnitGetId( pSource );
	STATE_EVENT_DATA tData;
	memclear(&tData, sizeof(STATE_EVENT_DATA));
	tData.pStats = sGetStatsForState( pUnit, nState, idSource, wStateIndex );
	tData.bFirstPerson = FALSE;
	tData.bHadState = bHadState;
	tData.idSource = idSource;
	tData.nModelId = nModelId;
	tData.nState = nState;
	tData.nOriginalState = nState;
	tData.pUnit = pUnit;

	tData.wStateIndex = wStateIndex;

	STATE_EVENT_DATA_STATE tStateState;
	memclear(&tStateState, sizeof(STATE_EVENT_DATA_STATE));
	tStateState.bClear = FALSE;
	tStateState.bWeaponsOnly = FALSE;
	if ( tData.pStats )
	{ // just in case we have already been here before with this state
		tStateState.dwEventsPerformedMask = StatsGetStat( tData.pStats, STATS_STATE_EVENT_MASK, 0 );
	}

	sStateDoEvents( pGame, tData, tStateState );

	DWORD dwEventsPerformed = tStateState.dwEventsPerformedMask;
	if ( IS_CLIENT( pGame ) && pUnit == GameGetControlUnit( pGame ) )
	{
		tData.nModelId = c_UnitGetModelIdFirstPerson(pUnit);
		tData.bFirstPerson = TRUE;
		if ( tData.pStats )
			tStateState.dwEventsPerformedMask = StatsGetStat( tData.pStats, STATS_STATE_EVENT_MASK, 0 );
		else
			tStateState.dwEventsPerformedMask = 0;
		tStateState.nEventCount = 0;
		sStateDoEvents( pGame, tData, tStateState );
	}
	dwEventsPerformed |= tStateState.dwEventsPerformedMask;

	if ( IS_CLIENT( pGame ) && UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
	{
		tData.nModelId = c_UnitGetModelIdThirdPersonOverride(pUnit);
		tData.bFirstPerson = FALSE;
		tData.bAppearanceOverride = TRUE;
		if ( tData.pStats )
			tStateState.dwEventsPerformedMask = StatsGetStat( tData.pStats, STATS_STATE_EVENT_MASK, 0 );
		else
			tStateState.dwEventsPerformedMask = 0;
		tStateState.nEventCount = 0;
		sStateDoEvents( pGame, tData, tStateState );
	}

	if ( tData.pStats )
	{
		StatsSetStat( pGame, tData.pStats, STATS_STATE_EVENT_MASK, 0, dwEventsPerformed | tStateState.dwEventsPerformedMask );
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDoEventsOnWeapons( 
	UNIT * pUnit,
	UNIT * pSource,
	BOOL pbDoEventsOnWeapon[ MAX_WEAPONS_PER_UNIT ],
	STATS * pStats,
	int nState,
	DWORD dwStateIndex )
{
	UNITID idSource = UnitGetId( pSource );
	STATE_EVENT_DATA tData;
	memclear(&tData, sizeof(STATE_EVENT_DATA));
	tData.pStats = pStats;
	tData.bFirstPerson = FALSE;
	tData.bHadState = FALSE;
	tData.idSource = idSource;
	tData.nModelId = c_UnitGetModelId( pUnit );
	tData.nState = nState;
	tData.nOriginalState = nState;
	tData.pUnit = pUnit;
	tData.wStateIndex = (WORD) dwStateIndex;

	STATE_EVENT_DATA_STATE tStateState;
	memclear(&tStateState, sizeof(STATE_EVENT_DATA_STATE));
	tStateState.bClear = FALSE;
	tStateState.bWeaponsOnly = TRUE;
	CopyMemory( tStateState.pbApplyToWeapon, pbDoEventsOnWeapon, sizeof(BOOL) * MAX_WEAPONS_PER_UNIT );

	if ( tData.pStats )
	{ // just in case we have already been here before with this state
		tStateState.dwEventsPerformedMask = StatsGetStat( tData.pStats, STATS_STATE_EVENT_MASK, 0 );
	}

	sStateDoEvents( UnitGetGame( pUnit ), tData, tStateState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StateDoAllEventsOnWeapons( 
	UNIT* pUnit,
	BOOL pbDoEventsOnWeapon[ MAX_WEAPONS_PER_UNIT ] )
{
	ASSERT_RETURN(pUnit);
	GAME* pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pbDoEventsOnWeapon);
	ASSERT_RETURN(IS_CLIENT( pUnit ));

	STATS_ENTRY ptStatsEntries[ MAX_STATES_PER_STATSLIST ];
	STATS_ENTRY ptStatsEntries2[ 1 ];
	STATS * pStats = StatsGetModList( pUnit, SELECTOR_STATE );
	while ( pStats ) {
		STATS * pStatsNext = StatsGetModNext( pStats, SELECTOR_STATE );

		int nStateCount = 0;		
		PARAM dwParam = 0;
		do {
			nStateCount = StatsGetRange( pStats, STATS_STATE, dwParam, ptStatsEntries, MAX_STATES_PER_STATSLIST );
			int nSourceCount = StatsGetRange( pStats, STATS_STATE_SOURCE, dwParam, ptStatsEntries2, 1 );
			UNITID idSource = nSourceCount ? (UNITID)ptStatsEntries2[ 0 ].value : INVALID_ID;
			UNIT * pSource = UnitGetById( pGame, idSource );
			DWORD dwStateIndex =  ptStatsEntries2[ 0 ].GetParam();
			for ( int i = 0; i < nStateCount; i++ )
			{
				int nState = ptStatsEntries[ i ].GetParam();

				sStateDoEventsOnWeapons( pUnit, pSource, pbDoEventsOnWeapon, pStats, nState, dwStateIndex );
			}
			dwParam = ptStatsEntries[ nStateCount - 1 ].GetParam() + 1;
		} while( nStateCount  );

		pStats = pStatsNext;
	} 
}

//----------------------------------------------------------------------------
// Hammer Only
//----------------------------------------------------------------------------
void StateClearEvents( 
	int nModelId,
	int nStateId,
	WORD wStateIndex,
	BOOL bFirstPerson )
{
	ASSERT_RETURN( AppIsHammer() );

	STATE_EVENT_DATA tData;
	memclear(&tData, sizeof(STATE_EVENT_DATA));
	tData.bFirstPerson = bFirstPerson;
	tData.bHadState = TRUE;
	tData.idSource = INVALID_ID;
	tData.nModelId = nModelId;
	tData.nState = nStateId;
	tData.nOriginalState = nStateId;
	tData.wStateIndex = wStateIndex;

	STATE_EVENT_DATA_STATE tStateState;
	memclear(&tStateState, sizeof(STATE_EVENT_DATA_STATE));
	tStateState.bClear = TRUE;
	tStateState.bWeaponsOnly = FALSE;

	GAME * pGame = AppGetCltGame();
	sStateDoEvents( pGame, tData, tStateState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static 
void sStateClearEvents( 
	UNIT * pUnit,
	int nStateId,
	UNITID idSource,
	WORD wStateIndex,
	STATS * pStats,
	BOOL bFirstPerson )
{
	ASSERT_RETURN( pUnit );
 	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );

	STATE_EVENT_DATA tData;
	memclear(&tData, sizeof(STATE_EVENT_DATA));
	tData.pStats = pStats;
	tData.bFirstPerson = FALSE;
	tData.bHadState = TRUE;
	tData.idSource = idSource;
	tData.nState = nStateId;
	tData.nOriginalState = nStateId;
	tData.pUnit = pUnit;
	tData.wStateIndex = wStateIndex;
	tData.nModelId = IS_CLIENT( pUnit ) ? c_UnitGetModelIdThirdPerson( pUnit ) : INVALID_ID;

	STATE_EVENT_DATA_STATE tStateState;
	memclear(&tStateState, sizeof(STATE_EVENT_DATA_STATE));
	tStateState.bClear = TRUE;
	tStateState.bWeaponsOnly = FALSE;

	if ( tData.pStats )
	{
		tStateState.dwEventsPerformedMask = StatsGetStat( tData.pStats, STATS_STATE_EVENT_MASK, 0 );
	}

	if (IS_CLIENT(pGame) && ! UnitHasExactState(pUnit, nStateId))
	{
		c_ModelAttachmentRemoveAllByOwner( tData.nModelId, ATTACHMENT_OWNER_STATE, nStateId );
	}
	sStateDoEvents( pGame, tData, tStateState );

	if ( IS_CLIENT( pGame ) && pUnit == GameGetControlUnit( pGame ) )
	{
		tData.nModelId = c_UnitGetModelIdFirstPerson( pUnit );
		tData.bFirstPerson = TRUE;
		if (!UnitHasExactState(pUnit, nStateId))
		{
			c_ModelAttachmentRemoveAllByOwner( tData.nModelId, ATTACHMENT_OWNER_STATE, nStateId );
		}
		tStateState.nEventCount = 0;
		sStateDoEvents( pGame, tData, tStateState );
	}

	if ( IS_CLIENT( pGame ) && UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
	{
		tData.nModelId = c_UnitGetModelIdThirdPersonOverride(pUnit);
		tData.bFirstPerson = FALSE;
		tData.bAppearanceOverride = TRUE;
		if (!UnitHasExactState(pUnit, nStateId))
		{
			c_ModelAttachmentRemoveAllByOwner( tData.nModelId, ATTACHMENT_OWNER_STATE, nStateId );
		}
		tStateState.nEventCount = 0;
		sStateDoEvents( pGame, tData, tStateState );
	}
}


//----------------------------------------------------------------------------
// Client/Server Messaging
//----------------------------------------------------------------------------
static void s_sSendSetStateMessage( 
	UNIT * target,
	UNITID idSource,
	int state,
	WORD wStateIndex,
	int duration,
	int skill,
	BOOL bDontExecuteSkillStats,
	STATS * stats,
	GAMECLIENTID idClient = INVALID_GAMECLIENTID)
{
	ASSERT_RETURN(target);
	GAME * game = UnitGetGame(target);
	ASSERT_RETURN(game);

	STATE_DATA * state_data = (STATE_DATA *)ExcelGetData(game, DATATABLE_STATES, state);
	if (!state_data)
	{
		return;
	}

	if (!StateDataTestFlag( state_data, SDF_SEND_TO_ALL ) && 
		!StateDataTestFlag( state_data, SDF_SEND_TO_SELF ))
	{
#if ISVERSION(DEVELOPMENT)
		if (stats)
		{
			StatsVerifyNoSendStats(game, stats, state_data->szName);
		}
#endif
		return;
	}

	// tell the client about the nState
	MSG_SCMD_UNITSETSTATE tMsgSet; 
	int nMessageSize = 0;
	tMsgSet.idTarget = UnitGetId(target);
	tMsgSet.idSource = idSource;
	tMsgSet.wState = (WORD)state;
	tMsgSet.wStateIndex = wStateIndex;
	tMsgSet.nDuration = duration;
	tMsgSet.wSkill = (WORD)skill;
	tMsgSet.bDontExecuteSkillStats = !!bDontExecuteSkillStats;
	tMsgSet.dwStatsListId = StatsListGetId(stats);

	if (StateDataTestFlag( state_data, SDF_SEND_STATS ) && stats)
	{
		BIT_BUF tBuffer(tMsgSet.tBuffer, MAX_STATE_STATS_BUFFER);
		WriteStats(game, stats, tBuffer, STATS_WRITE_METHOD_NETWORK);
		nMessageSize += tBuffer.GetLen();
	}
	MSG_SET_BLOB_LEN(tMsgSet, tBuffer, nMessageSize);

	if (idClient != INVALID_GAMECLIENTID && 
		(StateDataTestFlag( state_data, SDF_SEND_TO_ALL ) || (UnitHasClient(target) && idClient == UnitGetClientId(target))))
	{
		s_SendUnitMessageToClient(target, idClient, SCMD_UNITSETSTATE, &tMsgSet);
	}
	else if (StateDataTestFlag( state_data, SDF_SEND_TO_ALL ))
	{
		s_SendUnitMessage(target, SCMD_UNITSETSTATE, &tMsgSet);
	} 
	else if (StateDataTestFlag( state_data, SDF_SEND_TO_SELF ) && UnitHasClient(target) && (idClient == INVALID_GAMECLIENTID || idClient == UnitGetClientId(target)))
	{
		s_SendUnitMessageToClient(target, UnitGetClientId(target), SCMD_UNITSETSTATE, &tMsgSet);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StatesSendAllToClient(
	GAME * pGame,
	UNIT * pUnit,
	GAMECLIENTID idClient )
{
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

	STATS* stats = StatsGetModList( pUnit, SELECTOR_STATE );
	for ( ; stats; stats = StatsGetModNext(stats, SELECTOR_STATE) )
	{
		PARAM dwState = 0;
		if ( ! StatsGetStatAny(stats, STATS_STATE, &dwState ) )
			continue;

		if (dwState == INVALID_PARAM)
			continue;

		DWORD dwIndex = 0;
		UNITID idSource = StatsGetStatAny(stats, STATS_STATE_SOURCE, &dwIndex);

		int nDuration = 0; // not sure how to get a hold of this... find the event?  save the end frame?
		if(StateDataTestFlag((STATE_DATA*)ExcelGetData(NULL, DATATABLE_STATES, dwState), SDF_PERSISTANT))
		{
			nDuration = UnitGetStat(pUnit, STATS_STATE_DURATION_SAVED, dwState);
		}


		DWORD nSkill = 0;
		int nSkillLevel = StatsGetStatAny( stats, STATS_STATE_SOURCE_SKILL_LEVEL, &nSkill ); // we don't save the skill either... oh well
		REF(nSkillLevel);

		s_sSendSetStateMessage( pUnit, idSource, dwState, (WORD)dwIndex, nDuration, nSkill, FALSE, stats, idClient ); 
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void s_sSendClearStateMessage( 
	UNIT * pTarget,
	UNITID idSource,
	int nState,
	WORD wStateIndex )
{
	ASSERT_RETURN( pTarget );
	GAME * pGame = UnitGetGame( pTarget );
	STATE_DATA * pStateData = (STATE_DATA *) ExcelGetData( pGame, DATATABLE_STATES, nState );
	if( !pStateData )
		return;

	if (!StateDataTestFlag( pStateData, SDF_SEND_TO_ALL ) && 
		!StateDataTestFlag( pStateData, SDF_SEND_TO_SELF ))
	{
		return;
	}

	if (UnitCanSendMessages( pTarget ))
	{
	
		// tell the client about the nState
		MSG_SCMD_UNITCLEARSTATE tMsgClear; 
		tMsgClear.idTarget		= UnitGetId( pTarget );
		tMsgClear.idSource		= idSource;
		tMsgClear.wState		= (WORD)nState;
		tMsgClear.wStateIndex	= wStateIndex;

		if (StateDataTestFlag( pStateData, SDF_SEND_TO_ALL ))
		{
			s_SendUnitMessage( pTarget, SCMD_UNITCLEARSTATE, &tMsgClear);
		} 
		else if ( StateDataTestFlag( pStateData, SDF_SEND_TO_SELF ) && UnitHasClient( pTarget ) )
		{
			s_SendMessage( pGame, UnitGetClientId( pTarget ), SCMD_UNITCLEARSTATE, &tMsgClear);
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatePulse( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// get params from event data	
	int nState = (int)tEventData.m_Data1;
	int nPulseRateInTicks = (int)tEventData.m_Data2;
	int nSkill = (int)tEventData.m_Data3;
	UNITID nSourceUnit = (UNITID)tEventData.m_Data4;
	BOOL bIsSourcePulse = tEventData.m_Data5 == 1;

	UNIT* pSourceUnit = UnitGetById( pGame, nSourceUnit );
	if( pSourceUnit == NULL )
		pSourceUnit = pUnit;
	// unit must still have this state
	if ( ( bIsSourcePulse && pSourceUnit && UnitHasExactState( pSourceUnit, nState ) == TRUE ) ||
		 ( !bIsSourcePulse && UnitHasExactState( pUnit, nState ) == TRUE ) )
	{
		
		// setup start flags
		DWORD dwSkillStartFlags = 
			SKILL_START_FLAG_NO_REPEAT_EVENT |
			SKILL_START_FLAG_DONT_RETARGET |
			SKILL_START_FLAG_DONT_SET_COOLDOWN | 
			SKILL_START_FLAG_IGNORE_COOLDOWN;
		if( IS_SERVER( pSourceUnit ) )
		{
			dwSkillStartFlags = dwSkillStartFlags | SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_SERVER_ONLY;
		}
	
		// run the skill
		ASSERTX_RETFALSE( nSkill != INVALID_LINK, "Expected skill link to pulse for state" );
		SkillStartRequest( 
			pGame, 
			pUnit, 
			nSkill, 
			nSourceUnit, 
			UnitGetPosition( pUnit ),
			dwSkillStartFlags,
			SkillGetNextSkillSeed( pGame ) );
					
		// register another pulse
		UnitRegisterEventTimed( pUnit, sStatePulse, &tEventData, nPulseRateInTicks );
		
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sStateEnableTriggerNotargetOnUnit(
	UNIT *pUnit, 
	void *pCallbackData )
{
	UNIT * pTarget = (UNIT*)pCallbackData;
	if(UnitGetMoveTargetId(pUnit) == UnitGetId(pTarget))
	{
		UnitPathAbort(pUnit);
		UnitSetMoveTarget(pUnit, INVALID_ID);
		UnitSetAITarget(pUnit, INVALID_ID, FALSE, TRUE);
		UnitSetAITarget(pUnit, INVALID_ID, TRUE);
		UnitClearStepModes(pUnit);
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sStateChanged(
	UNIT *pUnit,
	int nState)
{
	ASSERTV_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( nState != INVALID_LINK, "Expected link" );
	GAME *pGame = UnitGetGame( pUnit );
	
	if (IS_SERVER( pGame ))
	{
		const STATE_DATA *pStateData = StateGetData( pGame, nState );
		if (pStateData)
		{
		
			// some states notify the chat server of new information
			#if !ISVERSION(CLIENT_ONLY_VERSION)
				if (StateDataTestFlag( pStateData, SDF_UPDATE_CHAT_SERVER ) && UnitIsPlayer( pUnit ))
				{
					UpdateChatServerPlayerInfo( pUnit );
				}
			#endif
			
		}
		
	}
	
	
}

//----------------------------------------------------------------------------
// Enable/Disable the nState - do the events and messages given that the decision has been make about the nState
//----------------------------------------------------------------------------
static void sStateEnable( 
	UNIT* pUnit,
	UNIT* pSource,
	int nState,
	WORD wStateIndex,
	BOOL bHadState )
{
	ASSERT_RETURN(pUnit);
	GAME* pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(pGame);
	const STATE_DATA *pStateData = StateGetData( NULL, nState );
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !pStateData )
	{
		return;
	}

	sStateDoEvents( pUnit, pSource, nState, wStateIndex, bHadState );

	// setup server state pulses (note that this is the state "pulsing itself", which is different
	// than the pulsing stats from a skill when a state is enabled)
	if ( pStateData->tPulseSkill.nSkillPulse != INVALID_LINK &&
		 ( IS_SERVER( pGame ) ||
		   ( pStateData->dwFlagsScripts & STATE_SCRIPT_FLAG_PULSE_CLIENT ) ) )		   
	{
		// get the pulse rate in seconds
		int nPulseRateInMS = 0;
		int nCodeLength = 0;		

		// CML 2007.09.13 - Fix for compile error: Changing codePulseRateInMSClient back to codePulseRateInMS.
		//   Looks like mlefler forgot to check in a change to excel and states.h.
		BYTE *pCode = ExcelGetScriptCode( pGame, DATATABLE_STATES, (( IS_SERVER(pGame) || pStateData->codePulseRateInMSClient <= 0  )?pStateData->codePulseRateInMS:pStateData->codePulseRateInMSClient), &nCodeLength );

		if (pCode)
		{
			nPulseRateInMS = VMExecI( pGame, pCode, nCodeLength, NULL );
		}

		if (nPulseRateInMS > 0)
		{
			// get pulse rate in ticks - don't do this as an INT (GAME_TICKS_PER_SECOND * nPulseRateInMS/MSECS_PER_SEC )
			int nPulseRateInTicks = (int)ceil( GAME_TICKS_PER_SECOND_FLOAT * ((float)nPulseRateInMS / (float)MSECS_PER_SEC) );			

			// setup event data
			EVENT_DATA tEventData;
			tEventData.m_Data1 = nState;
			tEventData.m_Data2 = nPulseRateInTicks;
			tEventData.m_Data3 = pStateData->tPulseSkill.nSkillPulse;
			if( StateDataTestFlag( pStateData, SDF_PULSE_ON_SOURCE ))
			{
				tEventData.m_Data4 = pUnit ? UnitGetId( pUnit ) : INVALID_ID;
				tEventData.m_Data5 = 1;	// is a source pulse
				// register pulse
				UnitRegisterEventTimed( pSource, sStatePulse, &tEventData, nPulseRateInTicks );
			}
			else
			{
				tEventData.m_Data4 = pSource ? UnitGetId( pSource ) : INVALID_ID;
				tEventData.m_Data5 = 0;	// not a source pulse
				// register pulse
				UnitRegisterEventTimed( pUnit, sStatePulse, &tEventData, nPulseRateInTicks );
			}
		}
	}

	UnitTriggerEvent(pGame, EVENT_STATE_SET, pUnit, pSource, &EVENT_DATA(nState, wStateIndex, bHadState ));

	if(StateDataTestFlag( pStateData, SDF_TRIGGER_NO_TARGET_EVENT_ON_SET ))
	{
		LEVEL * pLevel = UnitGetLevel(pUnit);
		if(pLevel)
		{
			TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
			LevelIterateUnits(pLevel, eTargetTypes, sStateEnableTriggerNotargetOnUnit, (void*)pUnit);
		}
	}

	// state has changed
	if (IS_SERVER( pGame ))
	{
		s_sStateChanged( pUnit, nState );
	}
		
	// trigger a unit save on the server for states that save
	if (IS_SERVER( pGame ))
	{
		if (StateDataTestFlag( pStateData, SDF_SAVE_WITH_UNIT ))
		{
			BOOL bSaveDigest = StateDataTestFlag( pStateData, SDF_TRIGGER_DIGEST_SAVE );
			UNIT *pUltimateOwner = UnitGetUltimateOwner( pUnit );
			s_DatabaseUnitOperation( 
				pUltimateOwner, 
				pUnit, 
				DBOP_UPDATE, 
				0, 
				DBUF_FULL_UPDATE,
				NULL,
				bSaveDigest ? DOF_SAVE_OWNER_DIGEST : 0);
		}
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateDisable( 
	UNIT * pUnit,
	UNITID idSource,
	int nStateId,
	WORD wStateIndex,
	STATS * pStats )
{
	sStateClearEvents( pUnit, nStateId, idSource, wStateIndex, pStats, FALSE );

	if ( IS_SERVER( pUnit ) )
	{
		GAME * pGame = pUnit ? UnitGetGame( pUnit ) : AppGetCltGame();

		DWORD dwNow = GameGetTick( pGame );
		UnitSetStat( pUnit, STATS_STATE_CLEAR_TICK, nStateId, dwNow );

		UnitTriggerEvent(pGame, EVENT_STATE_CLEARED, pUnit, NULL, &EVENT_DATA(nStateId, idSource, wStateIndex));
		s_sSendClearStateMessage( pUnit, idSource, nStateId, wStateIndex );

		if ( pStats )
		{
			DWORD dwParam = 0;
			int nSkillLevel = StatsGetStatAny( pStats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam );
			if ( nSkillLevel )
			{
				int nSkill = StatGetParam( STATS_STATE_SOURCE_SKILL_LEVEL, dwParam, 0 );
				const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
				if (nSkill != INVALID_ID &&
					 pSkillData &&
					 !SkillDataTestFlag( pSkillData, SKILL_FLAG_ALWAYS_SELECT ))
				{
					SkillDeselect( pGame, pUnit, nSkill, TRUE );
				}
			}
		}
		
		// state has changed
		s_sStateChanged( pUnit, nStateId );	
		
	}
		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsRemoveCallback(
	GAME * pGame, 
	UNIT * pUnit, 
	STATS * pStats)
{
	ASSERT_RETURN( pStats );
	int nState;
	PARAM dwStateIndex;
	UNITID idSource;
	{
		STATS_ENTRY pStatsEntries[ 2 ];
		int nStateCount = StatsGetRange( pStats, STATS_STATE, 0, pStatsEntries, 2 );
		ASSERT_RETURN( nStateCount == 1 );
		nState		= pStatsEntries[ 0 ].GetParam();

		nStateCount = StatsGetRange( pStats, STATS_STATE_SOURCE, 0, pStatsEntries, 2 );
		if ( nStateCount )
		{
			ASSERT_RETURN( nStateCount == 1 );
			dwStateIndex = pStatsEntries[ 0 ].GetParam();
			idSource = pStatsEntries[ 0 ].value;
		} else {
			dwStateIndex = 0;
			idSource = INVALID_ID;
		}
	}
	sStateDisable( pUnit, idSource, nState, (WORD)dwStateIndex, pStats );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS * sStateCreateStatsList(
	GAME * pGame, 
	UNIT * pTarget,
	UNIT * pSource, 
	int nState,
	WORD wStateIndex,
	int nDuration,
	int nSkill,
	BOOL bDontExecuteSkillStats,
	UNIT * pTransferUnit = NULL)
{
	struct STATS * pStatsList = NULL;
	const STATE_DATA * pStateData = StateGetData( pGame, nState );
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !pStateData )
	{
		return pStatsList;
	}

	const SKILL_DATA * pSkillData = NULL;
	int nSkillLevel = 1;
	if ( nSkill != INVALID_ID )
	{
		pSkillData = SkillGetData( pGame, nSkill );
		ASSERT_RETFALSE(pSkillData);

		UNIT * pUnitForSkillLevel = SkillDataTestFlag(pSkillData, SKILL_FLAG_SKILL_LEVEL_FROM_STATE_TARGET) ? pTarget : pSource;
		if ( ! pUnitForSkillLevel )
			pUnitForSkillLevel = pSource;
		nSkillLevel = SkillGetLevel( pUnitForSkillLevel, nSkill );
		nSkillLevel = MAX(1, nSkillLevel);

		pStatsList = StatsListInit(pGame);

		// KCK: Allow both the client and server to run this code. Client will check only for codeStatsOnStateSet, server will
		// check for both, with priority to the server only code.
		// GS - server has to do both if they're both available.
		if(!bDontExecuteSkillStats)
		{
			int code_len = 0;
			BYTE * code_ptr = NULL;

			code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnStateSet, &code_len);
			if( code_ptr && code_len)
			{
				if ( pSource && UnitHasInventory( pSource ))
				{
					UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
					UnitGetWeapons( pSource, nSkill, pWeapons, FALSE );
					UnitInventorySetEquippedWeapons( pSource, pWeapons);
				}

				VMExecI(pGame, pTarget, pSource, pStatsList, pStateData->nSkillScriptParam, nSkillLevel, nSkill, nSkillLevel, pStateData->nDefinitionId, code_ptr, code_len);
			}

			if( IS_SERVER( pGame ) )
			{
				code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnStateSetServerOnly, &code_len);

				if( code_ptr && code_len)
				{
					if ( pSource && UnitHasInventory( pSource ))
					{
						UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
						UnitGetWeapons( pSource, nSkill, pWeapons, FALSE );
						UnitInventorySetEquippedWeapons( pSource, pWeapons);
					}

					VMExecI(pGame, pTarget, pSource, pStatsList, pStateData->nSkillScriptParam, nSkillLevel, nSkill, nSkillLevel, pStateData->nDefinitionId, code_ptr, code_len);
				}
			}
		}
		StatsSetStat( pGame, pStatsList, STATS_STATE_SOURCE_SKILL_LEVEL, nSkill, nSkillLevel );
	}

	if ( ! pStatsList )
		pStatsList = StatsListInit( pGame );

	StatsSetStat( pGame, pStatsList, STATS_STATE, nState, 1 );
	StatsSetStat( pGame, pStatsList, STATS_STATE_SOURCE, wStateIndex, UnitGetId( pSource ) );

	if (nDuration != 0)
	{
		StatsListAddTimed(pTarget, pStatsList, SELECTOR_STATE, 0, nDuration, sStatsRemoveCallback, FALSE);
	} 
	else 
	{
		StatsListAdd(pTarget, pStatsList, FALSE, SELECTOR_STATE);
		StatsSetRemoveCallback(pGame, pStatsList, sStatsRemoveCallback);
	}

	if ( IS_SERVER( pGame ) && pTransferUnit )
	{
		StatsTransferSet(pGame, pStatsList, pTransferUnit);
	}

	if (pSkillData && IS_SERVER(pGame))
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnStateSetServerPostProcess, &code_len);
		if (code_ptr)
		{
			VMExecI(pGame, pTarget, pSource, pStatsList, pStateData->nSkillScriptParam, nSkillLevel, nSkill, nSkillLevel, pStateData->nDefinitionId, code_ptr, code_len);
		}
	}
	if (pSkillData)
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnStateSetPostProcess, &code_len);
		if (code_ptr)
		{
			VMExecI(pGame, pTarget, pSource, pStatsList, pStateData->nSkillScriptParam, nSkillLevel, nSkill, nSkillLevel, pStateData->nDefinitionId, code_ptr, code_len);
		}
	}
	return pStatsList;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StateSetPostProcessStats(
	GAME * game, 
	UNIT * target,
	UNIT * source, 
	STATS * stats,
	int nState,
	WORD wStateIndex,
	int nSkill)
{
	ASSERT_RETURN(game);
	if (!stats)
	{
		return;
	}
	const STATE_DATA * state_data = StateGetData(game, nState);
	if (!state_data)
	{
		return;
	}

	const SKILL_DATA * skill_data = NULL;
	int nSkillLevel = 1;
	if (nSkill != INVALID_ID)
	{
		skill_data = SkillGetData(game, nSkill);
		ASSERT_RETURN(skill_data);

		UNIT * unitForSkillLevel = SkillDataTestFlag(skill_data, SKILL_FLAG_SKILL_LEVEL_FROM_STATE_TARGET) ? target : source;
		if (!unitForSkillLevel)
		{
			unitForSkillLevel = source;
		}
		nSkillLevel = SkillGetLevel(unitForSkillLevel, nSkill);
	}

	if (skill_data)
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsOnStateSetPostProcess, &code_len);
		if (code_ptr)
		{
			VMExecI(game, target, source, stats, state_data->nSkillScriptParam, nSkillLevel, nSkill, nSkillLevel, state_data->nDefinitionId, code_ptr, code_len);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetupOnPulse(
	GAME * pGame,
	UNIT * pUnit, 
	int nSkill,
	int nState,
	WORD wStateIndex,
	UNITID idSource	)
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETURN( pSkillData );
	BOOL bAddsStatsOnPulse = FALSE;

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulse, &code_len );
	if ( code_ptr )
	{
		bAddsStatsOnPulse = TRUE;
	}

	if ( !bAddsStatsOnPulse && IS_SERVER( pGame ) )
	{
		code_len = 0;
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulseServerOnly, &code_len );
		if ( code_ptr )
		{
			bAddsStatsOnPulse = TRUE;
		}
	}

	if ( bAddsStatsOnPulse )
	{
		UnitRegisterEventHandler( pGame, pUnit, EVENT_TEMPLAR_PULSE, sApplySkillStatsOnPulse, &EVENT_DATA( nState, wStateIndex, idSource, nSkill ) );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sResetStateTimer(
	GAME * pGame,
	UNIT * pTarget,
	STATS * stats,
	int nDuration,
	int nState,
	const STATE_DATA * state_data,
	UNITID idSource,
	WORD wStateIndex,
	int nSkill,
	BOOL bDontExecuteSkillStats)
{
	if (stats && nDuration)
	{
		// just update the timer on the current nState.
		StatsSetTimer( pGame, pTarget, stats, nDuration );
		StatsSetStat( pGame, stats, STATS_STATE_DURATION, 0, nDuration );

		if (state_data && StateDataTestFlag( state_data, SDF_CLIENT_NEEDS_DURATION ))
		{
			s_sSendSetStateMessage(pTarget, idSource, nState, wStateIndex, nDuration, nSkill, bDontExecuteSkillStats, stats );
		}
	}
}


//----------------------------------------------------------------------------
// Interface functions for requests to set/clear the nState
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * s_StateSet( 
	UNIT * target, 
	UNIT* source, 
	int state, 
	int duration,
	int skill, 
	UNIT *transferunit /*= NULL*/,
	UNIT * item,
	BOOL bDontExecuteSkillStats )
{
	ASSERT_RETNULL(target);
	GAME* game = UnitGetGame(target);
	ASSERT_RETNULL(game && IS_SERVER(game));

	const STATE_DATA* state_data = StateGetData(game, state);
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !state_data )
	{
		return NULL;
	}
	ASSERT_RETNULL(duration >= 0);

	if (StateDataTestFlag( state_data, SDF_CLIENT_ONLY ))
	{
		return NULL;
	}
	
	if ( AppIsHellgate() )
	{
		ASSERTX_RETNULL( StateDataTestFlag( state_data, SDF_USED_IN_HELLGATE ), state_data->szName );
	}
	if ( AppIsTugboat() )
	{
		ASSERTX_RETNULL( StateDataTestFlag( state_data, SDF_USED_IN_TUGBOAT ), state_data->szName );
	}

	if (!source)
	{
		source = target;
	}

	if(UnitIsA(source, UNITTYPE_MISSILE))
	{
		source = UnitGetUltimateOwner(source);
	}

	// some states cannot be set if other states are preventing them
	if (state_data->nStatePreventedBy != INVALID_LINK)
	{
		if (UnitHasState( game, target, state_data->nStatePreventedBy ))
		{
			return NULL;
		}
	}
	
	// save the tick this state was set on
	DWORD dwNow = GameGetTick( game );
	UnitSetStat( target, STATS_STATE_SET_TICK, state, dwNow );

	if(StateDataTestFlag( state_data, SDF_SAVE_POSITION_ON_SET ))
	{
		UnitSetStatVector(target, STATS_STATE_SET_POINT_X, state, UnitGetPosition(target));
	}

	duration = duration ? duration : state_data->nDefaultDuration;
	if(StateDataTestFlag(state_data,SDF_PERSISTANT))
	{
		//UnitSetStat();

		if(duration <= 0)
			duration = 1;
		unsigned int currentTime = (int)GameGetTick(game);
		UnitSetStat(target, STATS_STATE_START_TIME, state, currentTime); 
		UnitSetStat(target, STATS_STATE_DURATION_SAVED, state, duration);
	}
	UNITID idSource = UnitGetId( source );

	// see if the target has any states that block this nState
	BOOL bHadState = FALSE;
	WORD wStateIndex = 0;
	if (!StateDataTestFlag( state_data, SDF_STACKS ))
	{
		STATS* stats = sGetStatsForState(target, state, UnitGetId(source), 0, TRUE, skill );
		sResetStateTimer(game, target, stats, duration, state, state_data, idSource, wStateIndex, skill, bDontExecuteSkillStats);
		if( stats )
		{
			return stats;
		}
	} 
	else
	{
		if(StateDataTestFlag( state_data, SDF_STACKS_PER_SOURCE ))
		{
			STATS* stats = sGetStatsForState(target, state, UnitGetId(source), 0, FALSE, skill );
			sResetStateTimer(game, target, stats, duration, state, state_data, idSource, wStateIndex, skill, bDontExecuteSkillStats);
			if( stats )
			{
				return stats;
			}
		}
		else
		{
			// figure out a unique nState index - use a random one which is unlikely to collide
			STATS* stats = NULL;
			do 
			{
				wStateIndex = (WORD)RandGetNum(game);
				stats = sGetStatsForState(target, state, UnitGetId(source), wStateIndex, FALSE, skill );
			} while (stats);

			bHadState = UnitHasExactState(target, state);
		}
	}

	// see if there are states that must be cleared when this nState is set

	// fill the stats list with the proper data from the source
	STATS * stats = sStateCreateStatsList( game, target, source, state, wStateIndex, duration, skill, bDontExecuteSkillStats, transferunit );

	if ( stats )
	{

			StatsSetStat( game, stats, STATS_STATE_DURATION, 0, duration );
	}

	if ( skill != INVALID_ID )
	{
		sSetupOnPulse( game, target, skill, state, wStateIndex, idSource );
	}


	s_sSendSetStateMessage(target, idSource, state, wStateIndex, duration, skill, bDontExecuteSkillStats, stats );

	// enable the nState
	sStateEnable( target, source, state, wStateIndex, bHadState );

	return sGetStatsForState( target, state, idSource, wStateIndex, FALSE, skill );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sStateChanged(	
	UNIT*pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	BOOL bCleared,
	int nDuration,
	int nSkill )
{
	GAME *pGame = UnitGetGame( pTarget );
	if ( bCleared )
		UnitTriggerEvent(pGame, EVENT_STATE_CLEARED, pTarget, NULL, &EVENT_DATA(nState, idSource, wStateIndex));

	UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)GMALLOCZ(pGame, sizeof(UI_STATE_MSG_DATA));
	pData->m_pSource		= (idSource != INVALID_ID ? UnitGetById(pGame, idSource) : NULL);
	pData->m_nState			= nState; 
	pData->m_wStateIndex	= wStateIndex;
	pData->m_nDuration		= bCleared ? 0 : nDuration;
	pData->m_nSkill			= nSkill;
	pData->m_bClearing		= bCleared;

	if (pTarget == GameGetControlUnit(pGame))
	{
		UISendMessage(WM_SETCONTROLSTATE, UnitGetId(pTarget), (LPARAM)pData);
	}
	else
	{
		UISendMessage(WM_SETTARGETSTATE, UnitGetId(pTarget),	(LPARAM)pData);
	}

	UNIT *pPlayer = GameGetControlUnit( pGame );
	UNIT *pUltimateContainer = UnitGetUltimateContainer(pTarget);
	if (pUltimateContainer == pPlayer)
	{
		const STATE_DATA *pStateData = StateGetData( pGame, nState );
		if (pStateData && StateDataTestFlag( pStateData, SDF_ON_CHANGE_REPAINT_ITEM_UI ) == TRUE)
		{
			UIQueueItemWindowsRepaint();
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
STATS * c_StateSet( 
	UNIT* pTarget, 
	UNIT* pSource, 
	int nState, 
	WORD wStateIndex,
	int nDuration,
	int nSkill,
	BOOL bIgnorePreviousState,
	BOOL bDontExecuteSkillStats )
{
	ASSERT_RETNULL( pTarget );
	GAME* pGame = UnitGetGame( pTarget );
	ASSERT_RETNULL( IS_CLIENT( pGame ) );

	c_StateLoad( pGame, nState );

	const STATE_DATA* state_data = (const STATE_DATA*)ExcelGetData( pGame, DATATABLE_STATES, nState );
	ASSERT_RETNULL( state_data );

	//trace( "s_StateSet Unit ID %d State: %s\n", pTarget->unitid, state_data->szName );

	if ( AppIsHellgate() )
	{
		ASSERTX_RETNULL( StateDataTestFlag( state_data, SDF_USED_IN_HELLGATE ), state_data->szName );
	}
	if ( AppIsTugboat() )
	{
		ASSERTX_RETNULL( StateDataTestFlag( state_data, SDF_USED_IN_TUGBOAT ), state_data->szName );
	}

	BOOL bHadState = UnitHasExactState( pTarget, nState );
	if ( bHadState )
	{// the state stat acrues and causes issues here 
		bHadState = sGetStatsForState( pTarget, nState, INVALID_ID, wStateIndex, TRUE ) != NULL;
	}
	if( bIgnorePreviousState )
	{
		bHadState = FALSE;
	}
	// ignore the nDuration if you don't want it - just trust the server to clear the nState
	nDuration = StateDataTestFlag( state_data, SDF_CLIENT_NEEDS_DURATION ) ? nDuration : 0;

	STATS * pStats = NULL;
	if (bHadState && (!StateDataTestFlag( state_data, SDF_STACKS ) || nDuration) )
	{
		pStats = sGetStatsForState(pTarget, nState, INVALID_ID, wStateIndex, TRUE, nSkill );
		if ( nDuration )
			StatsSetTimer(pGame, pTarget, pStats, nDuration);
	}
	else
	{
		pStats = sStateCreateStatsList( pGame, pTarget, pSource, nState, wStateIndex, nDuration, nSkill, bDontExecuteSkillStats );
	}

	if ( pStats )
	{
		StatsSetStat( pGame, pStats, STATS_STATE_DURATION, 0, nDuration );
	}

	if ( nSkill != INVALID_ID )
	{
		sSetupOnPulse( pGame, pTarget, nSkill, nState, wStateIndex, UnitGetId( pSource ) );
	}

	sStateEnable( pTarget, pSource, nState, wStateIndex, bHadState );

	// we need to get the state change for states with duration too -
	// if the duration of a malady/buff changes, we have to know to update it in the statesdisplay
	if (bHadState == FALSE || nDuration)
	{	
		c_sStateChanged(pTarget, UnitGetId(pSource), nState, wStateIndex, FALSE, nDuration, nSkill );
	}

	
	return sGetStatsForState( pTarget, nState, UnitGetId(pSource), wStateIndex, FALSE, nSkill );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StateClear( 
	UNIT * pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	BOOL bAnySource )
{
	ASSERT_RETURN( pTarget );
	ASSERT_RETURN( IS_CLIENT( pTarget ) );
	// see if the nState is currently set
	BOOL bHadState = UnitHasExactState( pTarget, nState );
	
	STATS * pStats = sGetStatsForState( pTarget, nState, idSource, wStateIndex, bAnySource );
	GAME * pGame = UnitGetGame( pTarget );
	if ( pStats )
	{
		StatsListRemove( pGame, pStats );
		StatsListFree( pGame, pStats );
	}

	if (bHadState == TRUE)
	{
		c_sStateChanged(pTarget, idSource, nState, wStateIndex, TRUE, 0, INVALID_ID );
	}

}

#else //SERVER_VERSION
STATS * c_StateSet( 
	UNIT* pTarget, 
	UNIT* pSource, 
	int nState, 
	WORD wStateIndex,
	int nDuration,
	int nSkill,
	BOOL bIgnorePreviousState,
	BOOL bDontExecuteSkillStats )
{
    ASSERT(FALSE);
    return NULL;
}
void c_StateClear( 
	UNIT * pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	BOOL bAnySource )
{
    ASSERT(FALSE);
}
#endif// ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sStateCleared(
	UNIT *pUnit,
	int nState,
	UNITID idSource,
	WORD wStateIndex,
	int nSkillID )
{
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETURN( nState != INVALID_LINK, "s_sStateCleared() - Expected state link" );
	const STATE_DATA *pStateData = StateGetData( pGame, nState );	
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !pStateData )
	{
		return;
	}
	DWORD dwNow = GameGetTick( pGame );
	UnitSetStat( pUnit, STATS_STATE_CLEAR_TICK, nState, dwNow );

	UnitTriggerEvent(pGame, EVENT_STATE_CLEARED, pUnit, NULL, &EVENT_DATA(nState, idSource, wStateIndex));
	
	// trigger a unit save on the server for states that save
	if (pStateData && StateDataTestFlag( pStateData, SDF_SAVE_WITH_UNIT ))
	{
		BOOL bSaveDigest = StateDataTestFlag( pStateData, SDF_TRIGGER_DIGEST_SAVE );
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pUnit );
		s_DatabaseUnitOperation( 
			pUltimateOwner, 
			pUnit, 
			DBOP_UPDATE, 
			0, 
			DBUF_FULL_UPDATE,
			NULL,
			bSaveDigest ? DOF_SAVE_OWNER_DIGEST : 0);
	}
	//see if the state needs to execute a script on removall
	if( nSkillID != INVALID_ID &&
		pStateData->dwFlagsScripts & STATE_SCRIPT_FLAG_ON_REMOVE )
	{
		UNIT *pSource = UnitGetById( pGame, idSource );		
		if( pStateData->dwFlagsScripts & STATE_SCRIPT_FLAG_ON_SOURCE )
		{
			if( pSource )
			{
				SkillExecuteScript( SKILL_SCRIPT_STATE_REMOVED, pSource, nSkillID, SkillGetLevel( pSource, nSkillID ), pUnit );
			}
		}
		else
		{		
			SkillExecuteScript( SKILL_SCRIPT_STATE_REMOVED, pUnit, nSkillID, SkillGetLevel( pUnit, nSkillID ), pSource );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StateClear( 
	UNIT * pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	int nSkill )
{
	ASSERT_RETURN( pTarget );
	GAME * pGame = UnitGetGame( pTarget );
	ASSERT_RETURN( IS_SERVER( pTarget ) );
	STATS * pStats = sGetStatsForState( pTarget, nState, idSource, wStateIndex, FALSE, nSkill );
	if ( pStats )
	{
		StatsListRemove( pGame, pStats );
		StatsListFree( pGame, pStats );
	}
	s_sStateCleared( pTarget, nState, idSource, wStateIndex, nSkill );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StateClearAllOfType( 
	UNIT * pUnit, 
	int nStateType )
{
	ASSERT_RETFALSE( pUnit );
	GAME * pGame = UnitGetGame( pUnit );

	BOOL bRemovedState = FALSE;
	STATS_ENTRY ptStatsEntries[ MAX_STATES_PER_STATSLIST ];
	STATS * pStats = StatsGetModList( pUnit, SELECTOR_STATE );
	while ( pStats ) {
		STATS * pStatsNext = StatsGetModNext( pStats, SELECTOR_STATE );

		int nState = INVALID_ID;
		WORD wStateIndex = 0;
		int nStateCount = 0;		
		BOOL bRemove = FALSE;
		UNITID idSource = INVALID_ID;
		PARAM dwParam = 0;
		do {
			nStateCount = StatsGetRange( pStats, STATS_STATE, dwParam, ptStatsEntries, MAX_STATES_PER_STATSLIST );
			for ( int i = 0; i < nStateCount; i++ )
			{
				nState = ptStatsEntries[ i ].GetParam();
				if ( nStateType == INVALID_ID || StateIsA( nState, nStateType ) )
				{
					idSource = (UNITID)StatsGetStat(pStats, STATS_STATE_SOURCE, wStateIndex);
					bRemove = TRUE;
					break;
				}
			}
			dwParam = ptStatsEntries[ nStateCount - 1 ].GetParam() + 1;
		} while( nStateCount && ! bRemove );

		if ( bRemove )
		{			
			StatsListRemove( pGame, pStats );
			StatsListFree( pGame, pStats );
			bRemovedState = TRUE;
			if ( IS_SERVER( pGame ) )
			{
				s_sStateCleared( pUnit, nState, idSource, wStateIndex, INVALID_ID );
			}
			else
			{
#if !ISVERSION(SERVER_VERSION)
				c_sStateChanged( pUnit, idSource, nState, wStateIndex, TRUE, 0, INVALID_ID );
#endif
			}
		}
		pStats = pStatsNext;

	} 
	return bRemovedState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_StatesApplyAfterGraphicsLoad( 
	UNIT * pUnit )
{
	ASSERT_RETURN( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( IS_CLIENT( pUnit ) );

	if ( AppCommonGetDemoMode() && GameGetControlUnit( pGame ) == pUnit )
	{
		// In Demo mode, don't apply states to the control unit.
		return;
	}

	STATS_ENTRY ptStatsEntries[ MAX_STATES_PER_STATSLIST ];
	STATS_ENTRY ptStatsEntries2[ 1 ];
	STATS * pStats = StatsGetModList( pUnit, SELECTOR_STATE );
	while ( pStats ) {
		STATS * pStatsNext = StatsGetModNext( pStats, SELECTOR_STATE );

		int nStateCount = 0;		
		PARAM dwParam = 0;
		do {
			nStateCount = StatsGetRange( pStats, STATS_STATE, dwParam, ptStatsEntries, MAX_STATES_PER_STATSLIST );
			int nSourceCount = StatsGetRange( pStats, STATS_STATE_SOURCE, dwParam, ptStatsEntries2, 1 );
			UNITID idSource = nSourceCount ? (UNITID)ptStatsEntries2[ 0 ].value : INVALID_ID;
			UNIT * pSource = UnitGetById( pGame, idSource );
			DWORD dwStateIndex =  ptStatsEntries2[ 0 ].GetParam();
			for ( int i = 0; i < nStateCount; i++ )
			{
				int nState = ptStatsEntries[ i ].GetParam();

				// just do the events again, it will apply the events that it missed the first time around
				sStateDoEvents( pUnit, pSource, nState, (WORD)dwStateIndex, TRUE );
			}
			dwParam = ptStatsEntries[ nStateCount - 1 ].GetParam() + 1;
		} while( nStateCount  );

		pStats = pStatsNext;
	} 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatesShareModStates( 
	UNIT * pDest,
	UNIT * pSource )
{
	ASSERT_RETURN( pDest );
	ASSERT_RETURN( pSource );
	GAME * pGame = UnitGetGame( pDest );

	UNIT_ITERATE_STATS_RANGE( pSource, STATS_STATE, pStatsEntries, ii, 32)
	{
		int nState = pStatsEntries[ ii ].GetParam();
		if ( nState == INVALID_PARAM ) 
			continue;

		const STATE_DATA * pStateData = StateGetData( pGame, nState );
		if ( !pStateData || !StateDataTestFlag( pStateData, SDF_SHARING_MOD_STATE ))
			continue;
		
		s_StateSet( pDest, pSource, nState, 0 );
	}
	STATS_ITERATE_STATS_END; 

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StateGetFirstOfType( 
	UNIT * pUnit, 
	int nStateType )
{
	ASSERT_RETINVALID( pUnit );

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_STATE, pStatsEntries, ii, 32)
	{
		int nState = pStatsEntries[ ii ].GetParam();
		if ( nStateType == INVALID_ID || StateIsA( nState, nStateType ) )
		{
			return nState;
		}
	}
	STATS_ITERATE_STATS_END; 

	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
void c_StateLoad(
	GAME * pGame,
	int nStateId )
{
	STATE_DATA * pStateData = (STATE_DATA *)ExcelGetData(pGame, DATATABLE_STATES, nStateId );
	if (!pStateData)
	{
		return;
	}
	if (StateDataTestFlag( pStateData, SDF_LOADED ))
	{
		return;
	}

	const STATE_DEFINITION * pStateDef = StateGetDefinition( nStateId );

	if ( ! pStateDef )
		return;
	for ( int i = 0; i < pStateDef->nEventCount; i++ )
	{
		STATE_EVENT * pEvent = & pStateDef->pEvents[ i ];

		STATE_EVENT_TYPE_DATA * pEventType = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
		ASSERTV_CONTINUE( pEventType, "state \"%s\" has an event that isn't found in the state_event_types table", pStateDef->tHeader.pszName );

		if ( pEventType->bUsesAttachments )
		{
			c_AttachmentDefinitionLoad( pGame, pEvent->tAttachmentDef, INVALID_ID, "" );
		} 
		if ( pEventType->bUsesScreenEffects )
		{
			// load the screen effect here
			int nScreenEffect = DefinitionGetIdByName( DEFINITION_GROUP_SCREENEFFECT, pEvent->pszData );
			REF( nScreenEffect );
		}
	}

	SETBIT( pStateData->pdwFlags, SDF_LOADED, TRUE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StateFlagSoundsForLoad(
	GAME * pGame,
	int nStateId,
	BOOL bLoadNow)
{
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if(nStateId < 0)
		return;

	STATE_DATA* pStateData = (STATE_DATA*)ExcelGetData(pGame, DATATABLE_STATES, nStateId );
	ASSERT_RETURN(pStateData);
	
	const STATE_DEFINITION * pStateDef = StateGetDefinition( nStateId );

	if ( ! pStateDef )
		return;
	for ( int i = 0; i < pStateDef->nEventCount; i++ )
	{
		STATE_EVENT * pEvent = & pStateDef->pEvents[ i ];

		STATE_EVENT_TYPE_DATA * pEventType = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
		if ( pEventType && pEventType->bUsesAttachments )
		{
			c_AttachmentDefinitionFlagSoundsForLoad( pGame, pEvent->tAttachmentDef, bLoadNow );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StatesFlagSoundsForLoad(
	GAME * pGame)
{
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	int nStateCount = ExcelGetNumRows(EXCEL_CONTEXT(pGame), DATATABLE_STATES);
	for(int i=0; i<nStateCount; i++)
	{
		STATE_DATA* pStateData = (STATE_DATA*)ExcelGetData(pGame, DATATABLE_STATES, i );
		if(!pStateData)
			continue;

		if(StateDataTestFlag( pStateData, SDF_FLAG_FOR_LOAD ))
		{
			c_StateFlagSoundsForLoad(pGame, i, FALSE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StatesLoadAll( 
	GAME * pGame )
{
	// changed to call fillpak load state code
	FillPak_LoadStatesAll();
}

#endif  !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatesInit( 
	GAME * pGame )
{
	int nStateCount = ExcelGetNumRows( pGame, DATATABLE_STATES );
	for (int ii = 0; ii < nStateCount; ii++)
	{
		STATE_DATA* state_data = (STATE_DATA*)ExcelGetData( pGame, DATATABLE_STATES, ii );
		if ( !state_data || state_data->nDefinitionId == INVALID_ID )
			continue;

		STATE_DEFINITION * pDef = (STATE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_STATES, state_data->nDefinitionId );
		if(pDef)
		{
			for ( int i = 0; i < pDef->nEventCount; i++ )
			{
				STATE_EVENT * pEvent = & pDef->pEvents[ i ];
				ConditionDefinitionPostLoad( pEvent->tCondition );

				ASSERT_CONTINUE( pEvent->eType != INVALID_ID );

				STATE_EVENT_TYPE_DATA * pEventTypeData = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
				for ( int j = 0; j < NUM_STATE_TABLE_REFERENCES; j++ )
				{
					if ( pEventTypeData->pnUsesTable[ j ] == INVALID_ID )
						continue;

					int * pnIndex;
					const char * pszString = sEventGetStringAndIndexForExcel( pEvent, j, &pnIndex );
					if ( ! pszString )
						continue;

					*pnIndex = ExcelGetLineByStringIndex( pGame, (EXCELTABLE)pEventTypeData->pnUsesTable[ j ], pszString );
				}
			}
		}
		else
		{
			ShowDataWarning("State %s references nonexistent state events file %s\n", state_data->szName, state_data->szEventFile);
			state_data->nDefinitionId = INVALID_ID;
		}
#ifdef SAVING_STATES
		CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_STATES )->Save( pDef );
#endif
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
static void c_StateAddRope( 
	UNIT * pSource,
	UNIT * pTarget,
	const VECTOR & vTarget,
	WORD wTargetIndex,
	int nState,
	STATS * pStats,
	STATE_EVENT * pEvent )
{
	int nSourceModelId = c_UnitGetModelId( pSource );
	int nTargetModelId = pTarget ? c_UnitGetModelId( pTarget ) : INVALID_ID;
	if ( nSourceModelId == INVALID_ID || (pTarget && nTargetModelId == INVALID_ID) )
		return;

	GAME * pGame = AppGetCltGame();

	VECTOR vPosition;
	if (pEvent->tAttachmentDef.pszBone[0] != '\0')
	{
		if (pEvent->tAttachmentDef.nBoneId == INVALID_ID)
		{
			int nSourceAppearanceDefId = c_AppearanceGetDefinition(nSourceModelId);
			pEvent->tAttachmentDef.nBoneId = c_AppearanceDefinitionGetBoneNumber(nSourceAppearanceDefId, pEvent->tAttachmentDef.pszBone);
		}
		MATRIX tBoneMatrix;
		if (c_AppearanceGetBoneMatrix(nSourceModelId, &tBoneMatrix, pEvent->tAttachmentDef.nBoneId))
		{
			MatrixMultiply(&vPosition, &cgvNone, &tBoneMatrix);
		}
	}
	else
	{
		vPosition = UnitGetAimPoint( pSource ) - UnitGetPosition( pSource );
	}

	VECTOR vTargetAimPoint = pTarget ? UnitGetAimPoint( pTarget ) : vTarget;
	VECTOR vTargetPosition = pTarget ? UnitGetPosition( pTarget ) : vTarget;

	ATTACHMENT_DEFINITION tAttachmentDef;
	tAttachmentDef.vPosition = vPosition;
	tAttachmentDef.vNormal   = vTargetAimPoint - UnitGetAimPoint( pSource );
	const MATRIX * pmWorldInverse = e_ModelGetWorldScaledInverse( nSourceModelId );
	MatrixMultiplyNormal( &tAttachmentDef.vNormal, &tAttachmentDef.vNormal, pmWorldInverse );
	VectorNormalize( tAttachmentDef.vNormal );

	int nMissileClass = sEventGetIndexForExcel( pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nParticleSystem = c_MissileEffectAttachToUnit( pGame, pSource, NULL, nMissileClass, MISSILE_EFFECT_PROJECTILE,
		tAttachmentDef, ATTACHMENT_OWNER_STATE, nState, 0.0f, TRUE );

	if ( nParticleSystem == INVALID_ID )
		return;

	VECTOR vAimOffset;
	tAttachmentDef.eType = ATTACHMENT_PARTICLE_ROPE_END;
	tAttachmentDef.nBoneId = c_AppearanceGetAimBone( nTargetModelId, vAimOffset );
	if ( tAttachmentDef.nBoneId == INVALID_ID )
		tAttachmentDef.vPosition = vTargetAimPoint - vTargetPosition;
	else
		tAttachmentDef.vPosition = VECTOR( 0 );

	tAttachmentDef.vNormal   = UnitGetAimPoint( pSource ) - vTargetAimPoint;
	if(nTargetModelId >= 0)
	{
		pmWorldInverse = e_ModelGetWorldScaledInverse( nTargetModelId );
		MatrixMultiplyNormal( &tAttachmentDef.vNormal, &tAttachmentDef.vNormal, pmWorldInverse );
		VectorNormalize( tAttachmentDef.vNormal );
	}

	c_ModelAttachmentAdd( nTargetModelId, tAttachmentDef, "", TRUE, ATTACHMENT_OWNER_STATE, nState, nParticleSystem );

	StatsSetStat( pGame, pStats, STATS_STATE_PARTICLE_SYSTEM, wTargetIndex, nParticleSystem );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_StateRemoveRope( 
	UNIT * pSource,
	UNIT * pTarget,
	PARAM dwTargetIndex,
	int nState,
	STATS * pStats,
	STATE_EVENT * pEvent )
{
	ASSERT_RETURN( pStats );
	if ( pStats )
	{
		int nParticleSystem = StatsGetStat( pStats, STATS_STATE_PARTICLE_SYSTEM, dwTargetIndex );
		if ( nParticleSystem != INVALID_ID )
			c_ParticleSystemDestroy( nParticleSystem ); // the attachments will take care of themselves...
	}
}

#endif //  !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORD sStateGetTargetIndex(
	STATS * pStats)
{
	STATS_ENTRY statsEntries[MAX_TARGETS_PER_STATE];
	int nTargets = StatsGetRange(pStats, STATS_STATE_TARGET, 0, statsEntries, arrsize(statsEntries));
	ASSERT(nTargets < USHRT_MAX);
	WORD wTargetIndex = DOWNCAST_INT_TO_WORD(-1);
	for (WORD ii = 0; ii < nTargets; ii++)
	{
		if (statsEntries[ii].GetParam() != (PARAM)ii)
		{
			wTargetIndex = ii;
			break;
		}
	}
	if (wTargetIndex == -1)
	{
		wTargetIndex = DOWNCAST_INT_TO_WORD(nTargets);
	}
	return wTargetIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateAddRope(
	GAME * pGame,
	int nState,
	UNIT * pUnit,
	UNIT * pTarget,
	const VECTOR & vTarget,
	WORD wTargetIndex,
	STATS * pStats)
{
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( pGame ) )
	{
		const STATE_DEFINITION * pStateDef = StateGetDefinition( nState );
		if(!pStateDef)
			return;

		for ( int i = 0; i < pStateDef->nEventCount; i++ )
		{
			STATE_EVENT * pEvent = & pStateDef->pEvents[ i ];
			STATE_EVENT_TYPE_DATA * pEventType = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
			if ( pEventType->pnUsesTable[ STATE_TABLE_REFERENCE_ATTACHMENT ] == DATATABLE_MISSILES &&
				pEventType->bAddsRopes )
			{
				c_StateAddRope( pUnit, pTarget, vTarget, wTargetIndex, nState, pStats, pEvent );
				break;
			}
		}
	}
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateAddTarget( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	UNIT * pTarget, 
	int nState,
	WORD wStateIndex )
{
	STATS * pStats = sGetStatsForState( pUnit, nState, idStateSource, wStateIndex );
	ASSERT_RETURN( pStats );

	WORD wTargetIndex = sStateGetTargetIndex(pStats);
	StatsSetStat( pGame, pStats, STATS_STATE_TARGET, wTargetIndex, UnitGetId( pTarget ) );

	sStateAddRope(pGame, nState, pUnit, pTarget, VECTOR(0), wTargetIndex, pStats);

	if ( IS_SERVER( pGame ) )
	{
		s_sSendAddTargetMsg( pGame, idStateSource, UnitGetId( pUnit ), UnitGetId( pTarget ), nState, wStateIndex );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStateAddTargetLocation( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	const VECTOR & vTarget, 
	int nState,
	WORD wStateIndex )
{
	STATS * pStats = sGetStatsForState( pUnit, nState, idStateSource, wStateIndex );
	ASSERT_RETURN( pStats );

	WORD wTargetIndex = sStateGetTargetIndex(pStats);
	StatsSetStatVector( pGame, pStats, STATS_STATE_TARGET_X, wTargetIndex, vTarget );

	sStateAddRope(pGame, nState, pUnit, NULL, vTarget, wTargetIndex, pStats);

	if ( IS_SERVER( pGame ) )
	{
		s_sSendAddTargetLocationMsg( pGame, idStateSource, UnitGetId( pUnit ), vTarget, nState, wStateIndex );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
void c_StateAddTarget( 
	GAME * pGame,
	MSG_SCMD_STATEADDTARGET * pMsg )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	UNIT * pStateHolder = UnitGetById( pGame, pMsg->idStateHolder );
	UNIT * pTarget		= UnitGetById( pGame, pMsg->idTarget );
	if ( ! pStateHolder )
		return;
	if ( pTarget )
		sStateAddTarget( pGame, pMsg->idStateSource, pStateHolder, pTarget, pMsg->wState, pMsg->wStateIndex );
}
#endif//  !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
void c_StateAddTargetLocation( 
	GAME * pGame,
	MSG_SCMD_STATEADDTARGETLOCATION * pMsg )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	UNIT * pStateHolder = UnitGetById( pGame, pMsg->idStateHolder );
	if ( ! pStateHolder )
		return;

	sStateAddTargetLocation( pGame, pMsg->idStateSource, pStateHolder, pMsg->vTarget, pMsg->wState, pMsg->wStateIndex );
}
#endif//  !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StateAddTargetLocation( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	const VECTOR & vTarget, 
	int nState,
	WORD wStateIndex )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	sStateAddTargetLocation( pGame, idStateSource, pUnit, vTarget, nState, wStateIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StateAddTarget( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	UNIT * pTarget, 
	int nState,
	WORD wStateIndex )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	sStateAddTarget( pGame, idStateSource, pUnit, pTarget, nState, wStateIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
void c_StateRemoveTarget( 
	GAME * pGame,
	MSG_SCMD_STATEREMOVETARGET * pMsg )
{
	UNIT * pStateHolder = UnitGetById( pGame, pMsg->idStateHolder );
	if ( ! pStateHolder )
		return;
	WORD wStateIndex = pMsg->wStateIndex;
	int nState = pMsg->wState;
	UNITID idStateSource = pMsg->idStateSource;

	const STATE_DEFINITION * pStateDef = StateGetDefinition( nState );

	STATS * pStats = sGetStatsForState( pStateHolder, nState, idStateSource, wStateIndex );
	if ( ! pStats )
		return;

	STATS_ENTRY pStatsEntries[MAX_TARGETS_PER_STATE];
	int nMaxTargets = MIN(ARC_LIGHTNING_MAX_TARGETS, MAX_TARGETS_PER_STATE);
	int nTargets = StatsGetRange(pStats, STATS_STATE_TARGET, 0, pStatsEntries, nMaxTargets);
	PARAM dwTargetIndex = INVALID_PARAM;
	for (int ii = 0; ii < nTargets; ii++)
	{
		if ((UNITID)pStatsEntries[ii].value == pMsg->idTarget)
		{
			dwTargetIndex = pStatsEntries[ii].GetParam();
			break;
		}
	}
	if (dwTargetIndex == INVALID_PARAM)
	{
		return; // we didn't have that pTarget
	}

	StatsSetStat( pGame, pStats, STATS_STATE_TARGET, dwTargetIndex, INVALID_ID );

	for ( int i = 0; i < pStateDef->nEventCount; i++ )
	{
		STATE_EVENT * pEvent = & pStateDef->pEvents[ i ];
		STATE_EVENT_TYPE_DATA * pEventType = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
		if ( pEventType->pnUsesTable[ STATE_TABLE_REFERENCE_ATTACHMENT ] == DATATABLE_MISSILES &&
			pEventType->bAddsRopes )
		{
			UNIT * pTarget = UnitGetById( pGame, pMsg->idTarget );
			c_StateRemoveRope( pStateHolder, pTarget, dwTargetIndex, nState, pStats, pEvent );
			break;
		}
	}
}
#endif//  !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAttachment(
	GAME * pGame,
	const STATE_EVENT_DATA & tData,
	ATTACHMENT_DEFINITION & tAttachDef )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW ) == 0 && tData.pUnit && tData.bHadState )
		return;

	if ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_ADD_TO_CENTER )
	{
		int nModelDefinition = e_ModelGetDefinition( tData.nModelId );
		if ( nModelDefinition != INVALID_ID )
		{
			const BOUNDING_BOX * pBoundingBox = e_ModelDefinitionGetBoundingBox( nModelDefinition, LOD_ANY );
			if ( pBoundingBox )
				tAttachDef.vPosition = BoundingBoxGetCenter( pBoundingBox );
		}
	}

	if(tData.pEvent->dwFlags & STATE_EVENT_FLAG_IGNORE_CAMERA)
	{
		tAttachDef.dwFlags |= ATTACHMENT_FLAGS_DONT_MUTE_SOUND;
	}

	BOOL bForceNew = tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW;
	int nAttachmentId = c_ModelAttachmentAdd( tData.nModelId, tAttachDef, FILE_PATH_DATA, bForceNew, 
		ATTACHMENT_OWNER_STATE, tData.nOriginalState, INVALID_ID );

	if ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_FLOAT )
	{
		c_ModelAttachmentFloat( tData.nModelId, nAttachmentId );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachment(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ATTACHMENT_DEFINITION tAttachDefCopy;
	MemoryCopy( &tAttachDefCopy, sizeof( ATTACHMENT_DEFINITION ), &tData.pEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );
	sAddAttachment( pGame, tData, tAttachDefCopy );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventPlayTriggerSound(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN(tData.pUnit);
	ATTACHMENT_DEFINITION tAttachDefCopy;
	MemoryCopy( &tAttachDefCopy, sizeof( ATTACHMENT_DEFINITION ), &tData.pEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );
	tAttachDefCopy.eType = ATTACHMENT_SOUND;
	const UNIT_DATA * pUnitData = UnitGetData(tData.pUnit);
	ASSERT_RETURN(pUnitData);
	tAttachDefCopy.nAttachedDefId = pUnitData->m_nTriggerSound;
	sAddAttachment( pGame, tData, tAttachDefCopy );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentBBox(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	ASSERT_RETURN( tData.pEvent->fParam > 0.0f );
	if ( ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW ) == 0 && tData.pUnit && tData.bHadState )
		return;

	c_ModelAttachmentAddBBox( tData.nModelId, (int)tData.pEvent->fParam, tData.pEvent->tAttachmentDef, FILE_PATH_DATA, FALSE, 
		ATTACHMENT_OWNER_STATE, tData.nOriginalState, INVALID_ID, 
		(tData.pEvent->dwFlags & STATE_EVENT_FLAG_FLOAT) != 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void (*FN_ADD_ATTACHMENT_ASYNCH)(GAME * pGame, const STATE_EVENT_DATA * pStateEventData);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoDelayedAddAttachmentAsynch(
	void *pDefinition,
	EVENT_DATA *pEventData)
{
	STATE_EVENT_DATA *pStateEventData = (STATE_EVENT_DATA *)pEventData->m_Data1;
	UNITID idUnit = (UNITID)pEventData->m_Data2;
	GAME *pGame = (GAME *)pEventData->m_Data3;
	FN_ADD_ATTACHMENT_ASYNCH pfnCallback = (FN_ADD_ATTACHMENT_ASYNCH)pEventData->m_Data4;
	ASSERTX_RETURN( pStateEventData, "Expected state event data" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

	if (idUnit != INVALID_ID)
	{
		UNIT *pUnit = UnitGetById( pGame, idUnit );
		if (pUnit)
		{
			// we only care if the unit still has this state
			if (UnitHasExactState( pUnit, pStateEventData->nState ))
			{
				pfnCallback(pGame, pStateEventData);
			}
		}
	}

	// free callback data
	GFREE( pGame, pStateEventData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentAsynchAware(
	GAME * pGame,
	const STATE_EVENT_DATA & tData,
	FN_ADD_ATTACHMENT_ASYNCH pfnCallback)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	int nAppearanceDefId = e_ModelGetAppearanceDefinition( tData.nModelId );
	if (nAppearanceDefId != INVALID_ID)
	{
		// if definition is not loaded, set a callback to do this event when it is
		APPEARANCE_DEFINITION *pDefAppearance = (APPEARANCE_DEFINITION *)DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
		if (pDefAppearance == NULL)
		{
			STATE_EVENT_DATA *pStateEventData = (STATE_EVENT_DATA *)GMALLOC( pGame, sizeof( STATE_EVENT_DATA ) );
			*pStateEventData = tData;

			UNITID idUnit = INVALID_ID;
			if (tData.pUnit)
			{
				idUnit = UnitGetId( tData.pUnit );
			}

			EVENT_DATA eventData((DWORD_PTR)pStateEventData, (DWORD_PTR)idUnit, (DWORD_PTR)pGame, (DWORD_PTR)pfnCallback);
			DefinitionAddProcessCallback( 
				DEFINITION_GROUP_APPEARANCE, 
				nAppearanceDefId, 
				sDoDelayedAddAttachmentAsynch, 
				&eventData);

			return;
		}
	}

	pfnCallback(pGame, &tData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAddAttachmentToAim(
	GAME * pGame,
	const STATE_EVENT_DATA *pStateEventData)
{
	ATTACHMENT_DEFINITION tAttachDefCopy;
	MemoryCopy( &tAttachDefCopy, sizeof( ATTACHMENT_DEFINITION ), &pStateEventData->pEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );
	VECTOR vOffset;
	tAttachDefCopy.nBoneId = c_AppearanceGetAimBone( pStateEventData->nModelId, vOffset );
	if ( tAttachDefCopy.nBoneId != INVALID_ID )
	{
		if ( ( tAttachDefCopy.dwFlags & ATTACHMENT_FLAGS_ABSOLUTE_POSITION ) == 0 )
		{
			tAttachDefCopy.vPosition += vOffset;
		}
	}
	sAddAttachment( pGame, *pStateEventData, tAttachDefCopy );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentToAim(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sHandleEventAddAttachmentAsynchAware(pGame, tData, sDoAddAttachmentToAim);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAddAttachmentToHeight(
	GAME * pGame,
	const STATE_EVENT_DATA *pStateEventData)
{
	ATTACHMENT_DEFINITION tAttachDefCopy;
	MemoryCopy( &tAttachDefCopy, sizeof( ATTACHMENT_DEFINITION ), &pStateEventData->pEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );
	if ( pStateEventData->pUnit )
		tAttachDefCopy.vPosition.fZ += UnitGetCollisionHeight(pStateEventData->pUnit);
	tAttachDefCopy.dwFlags |= ATTACHMENT_FLAGS_ABSOLUTE_POSITION; // we don't want the unit's scale to be factored in again
	sAddAttachment( pGame, *pStateEventData, tAttachDefCopy );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentToHeight(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sHandleEventAddAttachmentAsynchAware(pGame, tData, sDoAddAttachmentToHeight);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentToRandom(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	ASSERT_RETURN( tData.pEvent->fParam > 0.0f );
	if ( ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW ) == 0 && tData.pUnit && tData.bHadState )
		return;

	c_ModelAttachmentAddToRandomBones( tData.nModelId, (int)tData.pEvent->fParam, tData.pEvent->tAttachmentDef, FILE_PATH_DATA, FALSE, 
												ATTACHMENT_OWNER_STATE, tData.nOriginalState, INVALID_ID, 
												(tData.pEvent->dwFlags & STATE_EVENT_FLAG_FLOAT) != 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachmentBoneWeights(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	c_ModelAttachmentAddAtBoneWeight(tData.nModelId, tData.pEvent->tAttachmentDef, FALSE, ATTACHMENT_OWNER_STATE,
										tData.nOriginalState, (tData.pEvent->dwFlags & STATE_EVENT_FLAG_FLOAT) != 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRemoveAttachment(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	c_ModelAttachmentRemove( tData.nModelId, tData.pEvent->tAttachmentDef, FILE_PATH_DATA, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventHolyRadius(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	int nEnemyCount = UnitGetStat( tData.pUnit, STATS_HOLY_RADIUS_ENEMY_COUNT );
	if ( nEnemyCount <= 0 )
		return;

	// don't draw a holy radius for everyone, but we need to see them on bad guys
	if ( tData.pUnit != GameGetControlUnit( pGame ) && UnitGetTeam( tData.pUnit ) != TEAM_BAD )
		return;

	float fHolyRadius = UnitGetHolyRadius( tData.pUnit );
	if ( fHolyRadius == 0.0f )
		fHolyRadius = 1.0f;

	if ( tData.nModelId != INVALID_ID )
		HolyRadiusAdd( tData.nModelId, tData.pEvent->tAttachmentDef.nAttachedDefId, tData.nOriginalState, fHolyRadius, nEnemyCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventHolyRadiusClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	int nAttachmentId = c_ModelAttachmentFind( tData.nModelId, ATTACHMENT_OWNER_HOLYRADIUS, tData.nOriginalState );
	if ( nAttachmentId != INVALID_ID )
		c_ModelAttachmentRemove( tData.nModelId, nAttachmentId, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum 
{
	STATE_APPEARANCE_CHANGE_SCALE = 0,
	STATE_APPEARANCE_CHANGE_HEIGHT,
	STATE_APPEARANCE_CHANGE_WEIGHT,
};

static BOOL sEventMakeAppearanceChange(
	GAME* pGame, 
	UNIT* pUnit, 
	const EVENT_DATA& tEventdata)
{
	int nAppearanceChange = (int)tEventdata.m_Data1;
	float fFinal = EventParamToFloat(tEventdata.m_Data2);
	float fOriginal = EventParamToFloat(tEventdata.m_Data3);
	int nTotalTicks = (int)tEventdata.m_Data4;
	int nRemainingTicks = (int)tEventdata.m_Data5;

	float fChange = (fFinal - fOriginal) / float(nTotalTicks);

	float fCurrent;
	switch ( nAppearanceChange )
	{
	case STATE_APPEARANCE_CHANGE_SCALE:
	default:
		fCurrent = UnitGetScale(pUnit);
		break;

	case STATE_APPEARANCE_CHANGE_HEIGHT:
		{
			float fWeight = 0.0f;
			UnitGetShape( pUnit, fCurrent, fWeight );
		}
		break;

	case STATE_APPEARANCE_CHANGE_WEIGHT:
		{
			float fHeight = 0.0f;
			UnitGetShape( pUnit, fHeight, fCurrent );
		}
		break;
	}

	fCurrent += fChange;

	BOOL bDone = FALSE;
	if((fChange > 0.0f && fCurrent > fFinal) ||
		(fChange < 0.0f && fCurrent < fFinal))
	{
		bDone = TRUE;
		fCurrent = fFinal;
	}

	nRemainingTicks--;
	if(nRemainingTicks <= 0)
	{
		bDone = TRUE;
		fCurrent = fFinal;
	}

	switch ( nAppearanceChange )
	{
	case STATE_APPEARANCE_CHANGE_SCALE:
		UnitSetScale(pUnit, fCurrent);
		break;

	case STATE_APPEARANCE_CHANGE_HEIGHT:
		{
			float fHeight = 0.0f;
			float fWeight = 0.0f;
			UnitGetShape( pUnit, fHeight, fWeight );
			UnitSetShape( pUnit, fCurrent, fWeight );
		}
		break;

	case STATE_APPEARANCE_CHANGE_WEIGHT:
		{
			float fHeight = 0.0f;
			float fWeight = 0.0f;
			UnitGetShape( pUnit, fHeight, fWeight );
			UnitSetShape( pUnit, fHeight, fCurrent );
		}
		break;
	}

	if(!bDone && !IsUnitDeadOrDying(pUnit) && !UnitTestFlag(pUnit, UNITFLAG_JUSTFREED))
	{
		UnitRegisterEventTimed(pUnit, sEventMakeAppearanceChange, &EVENT_DATA(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fOriginal), nTotalTicks, nRemainingTicks), 1);
	}
	return TRUE;
}

void StateRegisterUnitScale( UNIT *pUnit,
							 int nAppearanceChange,
							 float fCurrent,
							 float fFinal,
							 int nTicks,
							 DWORD dwFlags )
{
	if(UnitHasTimedEvent(pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) ))
	{
		UnitUnregisterTimedEvent(pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) );
	}
	if( dwFlags & STATE_EVENT_FLAG_SET_IMMEDIATELY ||
		UnitTestFlag(pUnit, UNITFLAG_JUSTFREED))
	{
		EVENT_DATA tEventData(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), 1, 1);
		sEventMakeAppearanceChange(UnitGetGame( pUnit ), pUnit, tEventData);
	}
	else
	{		
		UnitRegisterEventTimed(pUnit, sEventMakeAppearanceChange, &EVENT_DATA(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), nTicks, nTicks), 1);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMakeAppearanceChange(
	GAME * pGame,
	const STATE_EVENT_DATA & tData,
	int nAppearanceChange,
	int nStatDesired,
	int nStatOriginal )
{
	ASSERT_RETURN(tData.pUnit);

	UnitSetStatFloat(tData.pUnit, nStatDesired, tData.nOriginalState, tData.pEvent->fParam);

	float fFinal = UnitGetStatFloat(tData.pUnit, nStatOriginal, 0);
	UNIT_ITERATE_STATS_RANGE( tData.pUnit, nStatDesired, pStatsEntry, jj, 64 )
	{
		DWORD dwValue = pStatsEntry[jj].value;
		float fValue = *(float*)&dwValue;
		fFinal *= fValue;
	}
	UNIT_ITERATE_STATS_END;

	float fCurrent = 1.0f;
	switch ( nAppearanceChange )
	{
	case STATE_APPEARANCE_CHANGE_SCALE:
		fCurrent = UnitGetScale(tData.pUnit);
		break;

	case STATE_APPEARANCE_CHANGE_HEIGHT:
		{
			float fWeight = 0.0f;
			UnitGetShape( tData.pUnit, fCurrent, fWeight );
		}
		break;

	case STATE_APPEARANCE_CHANGE_WEIGHT:
		{
			float fHeight = 0.0f;
			UnitGetShape( tData.pUnit, fHeight, fCurrent );
		}
		break;
	default:
		ASSERT(0);
		break;
	}

	if(fFinal != fCurrent)
	{
		StateRegisterUnitScale( tData.pUnit, nAppearanceChange, fCurrent, fFinal, tData.pEvent->tAttachmentDef.nVolume, tData.pEvent->dwFlags );
		/*
		if(UnitHasTimedEvent(tData.pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) ))
		{
			UnitUnregisterTimedEvent(tData.pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) );
		}
		if(tData.pEvent->dwFlags & STATE_EVENT_FLAG_SET_IMMEDIATELY ||
			UnitTestFlag(tData.pUnit, UNITFLAG_JUSTFREED))
		{
			EVENT_DATA tEventData(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), 1, 1);
			sEventMakeAppearanceChange(pGame, tData.pUnit, tEventData);
		}
		else
		{
			int nTicks = tData.pEvent->tAttachmentDef.nVolume;
			UnitRegisterEventTimed(tData.pUnit, sEventMakeAppearanceChange, &EVENT_DATA(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), nTicks, nTicks), 1);
		}
		*/
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearAppearanceChange(
	GAME * pGame,
	const STATE_EVENT_DATA & tData,
	int nAppearanceChange,
	int nStatDesired,
	int nStatOriginal )
{
	ASSERT_RETURN(tData.pUnit);

	UnitClearStat(tData.pUnit, nStatDesired, tData.nOriginalState);

	float fFinal = UnitGetStatFloat(tData.pUnit, nStatOriginal);
	UNIT_ITERATE_STATS_RANGE( tData.pUnit, nStatDesired, pStatsEntry, jj, 64 )
	{
		DWORD dwValue = pStatsEntry[jj].value;
		float fValue = *(float*)&dwValue;
		fFinal *= fValue;
	}
	UNIT_ITERATE_STATS_END;

	float fCurrent = 1.0f;
	switch ( nAppearanceChange )
	{
	case STATE_APPEARANCE_CHANGE_SCALE:
		fCurrent = UnitGetScale(tData.pUnit);
		break;

	case STATE_APPEARANCE_CHANGE_HEIGHT:
		{
			float fWeight = 0.0f;
			UnitGetShape( tData.pUnit, fCurrent, fWeight );
		}
		break;

	case STATE_APPEARANCE_CHANGE_WEIGHT:
		{
			float fHeight = 0.0f;
			UnitGetShape( tData.pUnit, fHeight, fCurrent );
		}
		break;
	default:
		ASSERT(0);
		break;
	}

	if(fFinal != fCurrent)
	{
		if(UnitHasTimedEvent(tData.pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) ))
		{
			UnitUnregisterTimedEvent(tData.pUnit, sEventMakeAppearanceChange, CheckEventParam1, EVENT_DATA(nAppearanceChange) );
		}
		if(tData.pEvent->dwFlags & STATE_EVENT_FLAG_CLEAR_IMMEDIATELY ||
			UnitTestFlag(tData.pUnit, UNITFLAG_JUSTFREED))
		{
			EVENT_DATA tEventData(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), 1, 1);
			sEventMakeAppearanceChange(pGame, tData.pUnit, tEventData);
		}
		else
		{
			int nTicks = tData.pEvent->tAttachmentDef.nVolume;
			UnitRegisterEventTimed(tData.pUnit, sEventMakeAppearanceChange, &EVENT_DATA(nAppearanceChange, EventFloatToParam(fFinal), EventFloatToParam(fCurrent), nTicks, nTicks), 1);
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetScaleToSize(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sMakeAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_SCALE, STATS_UNIT_SCALE_DESIRED, STATS_UNIT_SCALE_ORIGINAL );
}
static void sHandleEventClearSetScaleToSize(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sClearAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_SCALE, STATS_UNIT_SCALE_DESIRED, STATS_UNIT_SCALE_ORIGINAL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetHeight(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sMakeAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_HEIGHT, STATS_UNIT_HEIGHT_DESIRED, STATS_UNIT_HEIGHT_ORIGINAL );
}
static void sHandleEventClearHeight(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sClearAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_HEIGHT, STATS_UNIT_HEIGHT_DESIRED, STATS_UNIT_HEIGHT_ORIGINAL );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetWeight(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sMakeAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_WEIGHT, STATS_UNIT_WEIGHT_DESIRED, STATS_UNIT_WEIGHT_ORIGINAL );
}
static void sHandleEventClearWeight(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sClearAppearanceChange( pGame, tData, STATE_APPEARANCE_CHANGE_WEIGHT, STATS_UNIT_WEIGHT_DESIRED, STATS_UNIT_WEIGHT_ORIGINAL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoStateStartSkill( 
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	int nSkill = (int)tEventData.m_Data1;
	DWORD dwStartFlags = 0;
	if ( IS_SERVER( pGame ) )
	{
		dwStartFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
		dwStartFlags |= SKILL_START_FLAG_SERVER_ONLY;
	}
	else
		dwStartFlags |= SKILL_START_FLAG_DONT_SEND_TO_SERVER;

	return SkillStartRequest( 
		pGame, 
		pUnit, 
		nSkill, 
		INVALID_ID, 
		VECTOR(0), 
		dwStartFlags, 
		SkillGetNextSkillSeed( pGame ));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSkillStart(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNIT *pUnit = tData.pUnit;
	float flDelayInSeconds = tData.pEvent->fParam;
	DWORD dwDelayInTicks = (DWORD)(flDelayInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	int nSkill = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	
	// setup event data
	EVENT_DATA tEventData;
	tEventData.m_Data1 = nSkill;
	
	// register event or do now
	if (dwDelayInTicks > 0)
	{
	
		// register event
		UnitRegisterEventTimed( pUnit, sDoStateStartSkill, &tEventData, dwDelayInTicks );		
		
	}
	else
	{

		// do the skill now
		sDoStateStartSkill( pGame, pUnit, tEventData );
								
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSkillStop(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNIT *pUnit = tData.pUnit;
	float flDelayInSeconds = tData.pEvent->fParam;
	DWORD dwDelayInTicks = (DWORD)(flDelayInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	int nSkill = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );

	// see if we have an event to cancel
	BOOL bRemovedEvent = FALSE;
	if (dwDelayInTicks > 0)
	{
		EVENT_DATA tEventData;
		tEventData.m_Data1 = nSkill;	
		if (UnitHasTimedEvent( pUnit, sDoStateStartSkill, CheckEventParam1, &tEventData ))
		{
			UnitUnregisterTimedEvent( pUnit, sDoStateStartSkill, CheckEventParam1, &tEventData );	
			bRemovedEvent = TRUE;
		}
	}
	
	// do stop request if event was not removed
	if (bRemovedEvent == FALSE)
	{
		SkillStopRequest( tData.pUnit, nSkill );

		if ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_CLEAR_IMMEDIATELY )
		{
			SkillStop( pGame, tData.pUnit, nSkill, INVALID_ID );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sHandleEventRagdollStartPulling(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
#ifdef HAVOK_ENABLED
	c_RagdollStartPulling( tData.nModelId );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRagdollStopPulling(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
#ifdef HAVOK_ENABLED
	c_RagdollStopPulling( tData.nModelId );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRagdollStartCollapsing(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
#ifdef HAVOK_ENABLED
	c_RagdollFallApart( tData.nModelId, FALSE, TRUE, TRUE, TRUE, 10.0f, FALSE );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRagdollStopCollapsing(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetStateOnStat(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	int nStateToSet  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nStatToWatch = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );
	ASSERT_RETURN( nStateToSet != INVALID_ID );
	ASSERT_RETURN( nStatToWatch != INVALID_ID );

	if ( UnitGetStat( tData.pUnit, nStatToWatch ) && ! UnitHasState( pGame, tData.pUnit, nStateToSet ) )
	{
		if ( IS_SERVER( pGame ) )
			s_StateSet( tData.pUnit, tData.pUnit, nStateToSet, 0, INVALID_ID );
		if ( IS_CLIENT( pGame ) )
        {
#if  !ISVERSION(SERVER_VERSION)
            c_StateSet( tData.pUnit, tData.pUnit, nStateToSet, 0, 0, INVALID_ID );
#endif // !ISVERSION(SERVER_VERSION)
        }
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventStateClearAllOfType(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	int nStateToClear  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nStateToClear != INVALID_ID );

	if ( IS_SERVER( pGame ) )
		StateClearAllOfType( tData.pUnit, nStateToClear );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearSetStateOnStat(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	int nStateToSet  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nStatToWatch = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );

	if ( UnitHasState( pGame, tData.pUnit, nStateToSet ) )
	{
		if ( IS_SERVER( pGame ) )
			s_StateClear( tData.pUnit, UnitGetId( tData.pUnit ), nStateToSet, 0 );
		if ( IS_CLIENT( pGame ) )
#if  !ISVERSION(SERVER_VERSION)
			c_StateClear( tData.pUnit, UnitGetId( tData.pUnit ), nStateToSet, 0 );
#endif//  !ISVERSION(SERVER_VERSION)
		ASSERT_RETURN( UnitGetStat( tData.pUnit, nStatToWatch ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventStateSet(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
#if  !ISVERSION(CLIENT_ONLY_VERSION)
	if ( ! tData.pUnit )
		return;

	BOOL bForceNew = tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW;
	ASSERT_RETURN( bForceNew || tData.pStats );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	int nStateToSet  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );

	UNIT * pSource = UnitGetById( pGame, tData.idSource );
	int nSkill = INVALID_ID;
	int nDuration = 0;
	if(!bForceNew)
	{
		DWORD dwParam = 0;
		if ( StatsGetStatAny( tData.pStats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam ) )
		{
			nSkill = dwParam;
		}

		if ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_SHARE_DURATION )
		{
			nDuration = StatsGetStat( tData.pStats, STATS_STATE_DURATION );
		}
	}

	s_StateSet( tData.pUnit, pSource, nStateToSet, nDuration, nSkill, NULL, NULL );
#endif//  !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventStateSetOnClient(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( ! tData.pUnit )
		return;

	BOOL bForceNew = tData.pEvent->dwFlags & STATE_EVENT_FLAG_FORCE_NEW;
	ASSERT_RETURN( bForceNew || tData.pStats );
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	int nStateToSet  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );

	UNIT * pSource = UnitGetById( pGame, tData.idSource );
	int nSkill = INVALID_ID;
	int nDuration = 0;
	if(!bForceNew)
	{
		DWORD dwParam = 0;
		if ( StatsGetStatAny( tData.pStats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam ) )
		{
			nSkill = dwParam;
		}

		if ( tData.pEvent->dwFlags & STATE_EVENT_FLAG_SHARE_DURATION )
		{
			nDuration = StatsGetStat( tData.pStats, STATS_STATE_DURATION );
		}
	}

	c_StateSet( tData.pUnit, pSource, nStateToSet, 0, nDuration, nSkill, bForceNew );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventFullyHeal(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	s_UnitRestoreVitals( tData.pUnit, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateMonitorStat(
	UNIT * pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	if ( nOldValue && nNewValue )
		return;

	GAME * pGame = UnitGetGame( pUnit );

	if ( ! UnitHasState( pGame, pUnit, STATE_MONITORS_STAT ) )
		return;

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_STATE, pStatsEntries, ii, 32)
	{
		int nState = pStatsEntries[ ii ].GetParam();
		if ( StateIsA( nState, STATE_MONITORS_STAT ) )
		{
			const STATE_DEFINITION * pStateDef = StateGetDefinition( nState );

			for ( int i = 0; i < pStateDef->nEventCount; i++ )
			{
				STATE_EVENT * pEvent = & pStateDef->pEvents[ i ];
				STATE_EVENT_TYPE_DATA * pEventType = (STATE_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_STATE_EVENT_TYPES, pEvent->eType );
				if ( pEventType->pfnSetEventHandler != sHandleEventSetStateOnStat )
					continue;

				int nStateToSet  = sEventGetIndexForExcel( pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
				int nStatToWatch = sEventGetIndexForExcel( pEvent, STATE_TABLE_REFERENCE_EVENT );
				if ( nStatToWatch == INVALID_ID || nStateToSet == INVALID_ID )
					continue;

				if ( nStatToWatch != wStat )
					continue;

				BOOL bHasState = UnitHasState( pGame, pUnit, nStateToSet );
				if ( nNewValue && bHasState )
					continue;
				if ( !nNewValue && !bHasState )
					continue;

				if ( IS_SERVER( pGame ) && pEventType->bServerOnly )
				{
					if ( nNewValue )
						s_StateSet( pUnit, pUnit, nStateToSet, 0, INVALID_ID );
					else
						s_StateClear( pUnit, UnitGetId( pUnit ), nStateToSet, 0 );
				}
				else if ( IS_CLIENT( pGame ) && pEventType->bClientOnly )
				{
#if  !ISVERSION(SERVER_VERSION)
					if ( nNewValue )
						c_StateSet( pUnit, pUnit, nStateToSet, 0, 0, INVALID_ID );
					else
						c_StateClear( pUnit, UnitGetId( pUnit ), nStateToSet, 0 );
#endif//  !ISVERSION(SERVER_VERSION)
				}
			}
		}
	}
	STATS_ITERATE_STATS_END; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventEnableDrawGroup(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	int nDrawGroup = tData.pEvent->tAttachmentDef.eType;
	e_ModelSetNoDrawGroup( tData.nModelId, nDrawGroup, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDisableDrawGroup(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	int nDrawGroup = tData.pEvent->tAttachmentDef.eType;
	e_ModelSetNoDrawGroup( tData.nModelId, nDrawGroup, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSpin(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	sHandleSpin( pGame, tData.pUnit, EVENT_DATA((DWORD_PTR)tData.pEvent, tData.nModelId) );
}
//----------------------------------------------------------------------------
static void sHandleEventSpinClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
//#ifdef HAMMER
	if (AppIsHammer())
	{
		GameEventUnregisterAll( pGame, sHandleSpin, &sgtHammerEventList );
	}
//#else
	else
	{
		UnitUnregisterTimedEvent( tData.pUnit, sHandleSpin );
	}
//#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventStartArcLightning(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( tData.pUnit && pGame );
	UNIT * pSourceUnit = UnitGetById(pGame, tData.idSource);
	UNIT * pWeapons[MAX_WEAPONS_PER_UNIT];
	memclear(pWeapons, MAX_WEAPONS_PER_UNIT * sizeof(UNIT*));
	if(pSourceUnit->inventory)
	{
		UnitInventoryGetEquippedWeapons(pSourceUnit, pWeapons);
	}
	UnitRegisterEventTimed( tData.pUnit, sHandleArcLightning, &EVENT_DATA((DWORD_PTR)tData.pEvent, tData.nOriginalState, tData.idSource, UnitGetId(pWeapons[0]) ), 1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetMode(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNITMODE eMode = (UNITMODE) sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if (IS_SERVER( pGame ))
	{
		s_UnitSetMode( tData.pUnit, eMode, 0, 0.0f );
	}
	else
	{
		c_UnitSetMode( tData.pUnit, eMode );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetModeClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
//	ASSERT_RETURN( IS_SERVER( pGame ) );
	UNITMODE eMode = (UNITMODE) sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if ( !UnitTestFlag( tData.pUnit, UNITFLAG_JUSTFREED) )
		UnitEndMode( tData.pUnit, eMode );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetModeClientDuration(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNITMODE eMode = (UNITMODE) sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if (IS_SERVER( pGame ))
		return;

	BOOL bHasMode;
	int nAppearanceDefId = UnitGetAppearanceDefId( tData.pUnit, TRUE );
	if ( nAppearanceDefId == INVALID_ID)
		return;

	float duration = AppearanceDefinitionGetAnimationDuration( pGame, nAppearanceDefId, 0, eMode, bHasMode );
	c_UnitSetMode( tData.pUnit, eMode, 0, duration );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventLevelUp(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if (!AppIsHammer())
	{
		ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
		if ( ! tData.pUnit )
			return;

		// It's perfectly legitimate for this to happen in multiplayer.
		if( tData.pUnit != GameGetControlUnit( pGame ) )
			return;

		int nLevel = UnitGetExperienceLevel( tData.pUnit );
		UIShowLevelUpMessage( nLevel );

#endif// !ISVERSION(SERVER_VERSION)
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRankUp(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if (!AppIsHammer())
	{
		ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
		if ( ! tData.pUnit )
			return;

		// It's perfectly legitimate for this to happen in multiplayer.
		if( tData.pUnit != GameGetControlUnit( pGame ) )
			return;

		int nLevel = UnitGetPlayerRank( tData.pUnit );
		UIShowRankUpMessage( nLevel );

#endif// !ISVERSION(SERVER_VERSION)
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDisableDraw(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	if( AppIsHellgate() )
	{
		if ( tData.pUnit )
		{ // If I set the STATS_FADE_TIME_IN_TICKS in the statslist, then it doesn't get applied until after the fade has begun
			UnitSetStat( tData.pUnit, STATS_FADE_TIME_IN_TICKS, MAX((int)(tData.pEvent->fParam * GAME_TICKS_PER_SECOND), 1) );
			UnitChangeStat( tData.pUnit, STATS_NO_DRAW, 0, 1 ); 
		} else {
			e_ModelSetDrawAndShadow( tData.nModelId, FALSE );
		}
	}
	else
	{
		if ( tData.pUnit && tData.pEvent->fParam != 0 )
		{ // If I set the STATS_FADE_TIME_IN_TICKS in the statslist, then it doesn't get applied until after the fade has begun
			UnitSetStat( tData.pUnit, STATS_FADE_TIME_IN_TICKS, MAX((int)(tData.pEvent->fParam * GAME_TICKS_PER_SECOND), 1) );
			UnitSetStat( tData.pUnit, STATS_NO_DRAW, 1 ); 
		} else {
			e_ModelSetDrawAndShadow( tData.nModelId, FALSE );
		}
	}
	
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDisableDrawClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	if ( AppIsHellgate()  )
	{// Yeah, we've hardcoded things to fade in over half a second...
		if( tData.pUnit )
		{
			UnitSetStat( tData.pUnit, STATS_FADE_TIME_IN_TICKS, 10 );
			UnitChangeStat( tData.pUnit, STATS_NO_DRAW, 0, -1 ); 
			if ( tData.pUnit == GameGetControlUnit( pGame ) )
				c_UnitUpdateViewModel( tData.pUnit, TRUE, FALSE );

		}
		else
		{
			e_ModelSetDrawAndShadow( tData.nModelId, TRUE );
		}
	}
	else
	{
		if ( tData.pUnit && tData.pEvent->fParam != 0 )
		{ 
			UnitSetStat( tData.pUnit, STATS_FADE_TIME_IN_TICKS, 10 );
			UnitSetStat( tData.pUnit, STATS_NO_DRAW, 0 ); 
		} 
		else 
		{
			e_ModelSetDrawAndShadow( tData.nModelId, TRUE );
		}

	}
#endif//  !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontDrawWeapons(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	ASSERT_RETURN( tData.pStats );
	StatsSetStat(pGame, tData.pStats, STATS_DONT_DRAW_WEAPONS, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDisableSound(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	if ( tData.pUnit )
	{
		UnitChangeStat( tData.pUnit, STATS_NO_SOUND, 1);
		int nModelId = c_UnitGetModelIdFirstPerson(tData.pUnit);
		if(nModelId >= 0)
		{
			e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, TRUE);
		}
		nModelId = c_UnitGetModelIdThirdPerson(tData.pUnit);
		if(nModelId >= 0)
		{
			e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, TRUE);
		}
		nModelId = c_UnitGetModelIdThirdPersonOverride(tData.pUnit);
		if(nModelId >= 0)
		{
			e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, TRUE);
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
// this is not friendly with other states using the same flags - current use case doesn't make me worry about it
//----------------------------------------------------------------------------
static void sHandleEventDisableQuests(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
	{
		UnitSetFlag( tData.pUnit, UNITFLAG_DISABLE_QUESTS );
	}
}

//----------------------------------------------------------------------------
// this is not friendly with other states using the same flags - current use case doesn't make me worry about it
//----------------------------------------------------------------------------
static void sHandleEventDisableQuestsClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
	{
		UnitClearFlag( tData.pUnit, UNITFLAG_DISABLE_QUESTS );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDisableSoundClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	if ( tData.pUnit )
	{
		UnitChangeStat( tData.pUnit, STATS_NO_SOUND, -1);
		int nCurrentValue = UnitGetStat(tData.pUnit, STATS_NO_SOUND);
		if(nCurrentValue == 0)
		{
			int nModelId = c_UnitGetModelIdFirstPerson(tData.pUnit);
			if(nModelId >= 0)
			{
				e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, FALSE);
			}
			nModelId = c_UnitGetModelIdThirdPerson(tData.pUnit);
			if(nModelId >= 0)
			{
				e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, FALSE);
			}
			nModelId = c_UnitGetModelIdThirdPersonOverride(tData.pUnit);
			if(nModelId >= 0)
			{
				e_ModelSetFlagbit(nModelId, MODEL_FLAGBIT_NOSOUND, FALSE);
			}
		}
	}
#endif//  !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetOpacity(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	float fOpacity = tData.pEvent->fParam;
	if ( tData.pUnit )
	{
		float fOpacityPrev = UnitGetStatFloat( tData.pUnit, STATS_OPACITY_OVERRIDE );
		if ( fOpacityPrev == 0.0f || fOpacityPrev > fOpacity )
			UnitSetStatFloat( tData.pUnit, STATS_OPACITY_OVERRIDE, 0, fOpacity );
	}
	else
		e_ModelSetAlpha( tData.nModelId, fOpacity );
#endif//  !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearOpacity(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	ASSERT_RETURN ( IS_CLIENT( pGame ) );
#if  !ISVERSION(SERVER_VERSION)
	if ( tData.pUnit )
	{
		float fOpacity = tData.pEvent->fParam;
		float fOpacityPrev = UnitGetStatFloat( tData.pUnit, STATS_OPACITY_OVERRIDE );
		if ( fOpacityPrev == fOpacity )
			UnitSetStatFloat( tData.pUnit, STATS_OPACITY_OVERRIDE, 0, 0.0f );
	}
	else
		e_ModelSetAlpha( tData.nModelId, 1.0f );
#endif//  !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontTarget(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
		UnitSetDontTarget( tData.pUnit, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontTargetClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
		UnitSetDontTarget( tData.pUnit, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontAttack(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNIT *pUnit = tData.pUnit;
	if (pUnit)
	{
		UnitSetDontAttack(pUnit, TRUE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontAttackClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	UNIT *pUnit = tData.pUnit;
	if (pUnit)
	{
		UnitSetDontAttack(pUnit, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontInterrupt(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
		UnitSetDontInterrupt( tData.pUnit, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDontInterruptClear(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
	if ( tData.pUnit )
		UnitSetDontInterrupt( tData.pUnit, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAddAttachemntShrinkBone(
	GAME * pGame,
	const STATE_EVENT_DATA & tData )
{
#ifdef _DEBUG
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_NO_SHRINKING_BONES) != 0 )
	{
		return;
	}
#endif

	ASSERT( IS_CLIENT( pGame ) );
	if ( tData.nModelId == INVALID_ID )
		return;

	if ( tData.pUnit && UnitDataTestFlag( UnitGetData( tData.pUnit), UNIT_DATA_FLAG_DONT_SHRINK_BONES ) )
		return;

	int nAppearanceDef = c_AppearanceGetDefinition( tData.nModelId );

	c_AttachmentDefinitionLoad( pGame, tData.pEvent->tAttachmentDef, nAppearanceDef, "" );

	ATTACHMENT_DEFINITION tAttachDef = tData.pEvent->tAttachmentDef;

	// use the bone that was shrunk
	tAttachDef.nBoneId = c_AppearanceShrunkBoneAdd( tData.nModelId, tData.pEvent->tAttachmentDef.nBoneId );

	sAddAttachment( pGame, tData, tAttachDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventFireMissileGibs(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Client only" );

	if ( AppTestFlag( AF_CENSOR_PARTICLES ) )
		return;

	UNIT *pUnit = tData.pUnit;
	ASSERTX_RETURN( pUnit, "Expected source for state event type" );

	// don't do this for destructables
	if (UnitIsA( pUnit, UNITTYPE_DESTRUCTIBLE ))
	{
		return;
	}
	
	// get missile parameters
	int nMissileClass = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nMissileCount = (int)tData.pEvent->fParam;
	ASSERTX_RETURN( nMissileCount > 0, "Missile count is for state event is zero!" );

	// what will the elemental effects be for the missiles fired
	int nState = tData.nOriginalState;
	const STATE_DATA *pStateData = StateGetData( pGame, nState );
	BOOL bElementEffectsExist = FALSE;
	BOOL bElementEffectBuffer[MAX_DAMAGE_TYPES];
	int num_damage_types = GetNumDamageTypes(AppGetCltGame());

	// clear buffer for elemental effects
	memclear(bElementEffectBuffer, arrsize(bElementEffectBuffer) * sizeof(BOOL));

	// set the element of this state to on
	if (pStateData && pStateData->nDamageTypeElement != INVALID_LINK)
	{
		ASSERTX_RETURN( pStateData->nDamageTypeElement >= 0 && pStateData->nDamageTypeElement < num_damage_types, "Invalid state element" );
		bElementEffectBuffer[ pStateData->nDamageTypeElement ] = TRUE;
		bElementEffectsExist = TRUE;
	}
				
	// get source position	
	VECTOR vUnitPosition = UnitGetPosition( pUnit );
	float flRadius = UnitGetCollisionRadius( pUnit );
	float flHeight = UnitGetCollisionHeight( pUnit );
	
	// create all missile gibs	
	for ( int i = 0; i < nMissileCount; i++ )
	{
		
		// pick source location randomly offset in the collision area of the unit
		VECTOR vPosition = vUnitPosition;
		vPosition.fX += (flRadius * RandGetFloat( pGame, 0.0, 1.0f ));
		vPosition.fY += (flRadius * RandGetFloat( pGame, 0.0, 1.0f ));
		vPosition.fZ += (flHeight * RandGetFloat( pGame, 0.0, 1.0f ));
		
		// pick direction vector
		VECTOR vDirection = RandGetVector(pGame);
		if ( vDirection.fZ < 0.2f )
		{
			vDirection.fZ += 0.5f;
		}
		if (vDirection.fZ <= 0.0f)
		{
			vDirection.fZ = RandGetFloat(pGame);
		}
		VectorNormalize( vDirection, vDirection );
		VECTOR vTarget = vPosition + vDirection;
		MissileFire(
			pGame, 
			nMissileClass, 
			pUnit, 
			NULL,			// weapon
			INVALID_LINK,	// skill
			NULL,			// target unit
			vTarget,
			vPosition, 
			vDirection, 
			0,				// random seed
			0,
			0,
			bElementEffectsExist ? bElementEffectBuffer : NULL);
			
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventCreatePaperdollAppearance(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( IS_CLIENT( tData.pUnit ) );

	c_UnitCreatePaperdollModel(tData.pUnit, APPEARANCE_NEW_DONT_DO_INIT_ANIMATION | APPEARANCE_NEW_FORCE_ANIMATE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDestroyPaperdollAppearance(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( IS_CLIENT( tData.pUnit ) );

	if (tData.pUnit->pGfx->nModelIdPaperdoll != INVALID_ID)
	{
		c_AppearanceDestroy( tData.pUnit->pGfx->nModelIdPaperdoll );
		tData.pUnit->pGfx->nModelIdPaperdoll = INVALID_ID;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventDrawLoadingScreen(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	return;

#if !ISVERSION(SERVER_VERSION)
	//if ( !UIShowingLoadingScreen() )
	//{
	//	UIShowLoadingScreen( 0 );
	//	AppLoadingScreenUpdate( TRUE );
	//}
#endif// !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearLoadingScreen(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	return;

#if !ISVERSION(SERVER_VERSION)
	//if(UIShowingLoadingScreen())
	//{
	//	UIHideLoadingScreen();
	//}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetMaterialOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nExcelIndex = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nType = tData.pEvent->nData;
	int nMaterial = e_GetGlobalMaterial( nExcelIndex );

	if ( nMaterial == INVALID_ID )
	{
		// cmarch: The only way I see this happening is if material set in hammer is not resolved to a valid link
		V( e_ModelRemoveAllMaterialOverrides( tData.nModelId ) );
		return;
	}

	V( e_MaterialRestore( nMaterial ) );
	if ( nType == MATERIAL_OVERRIDE_NORMAL )
	{
		V( e_ModelAddMaterialOverride( tData.nModelId, nMaterial ) );
	} else if ( nType == MATERIAL_OVERRIDE_CLONE )
	{
		V( e_ModelActivateClone( tData.nModelId, nMaterial ) );
	}
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearMaterialOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nExcelIndex = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nType = tData.pEvent->nData;
	int nMaterial = e_GetGlobalMaterial( nExcelIndex );

	if ( nMaterial == INVALID_ID )
	{
		// cmarch: The only way I see this happening is if material set in hammer is not resolved to a valid link
		V( e_ModelRemoveAllMaterialOverrides( tData.nModelId ) );
		return;
	}

	if ( nType == MATERIAL_OVERRIDE_CLONE )
	{
		V( e_ModelDeactivateClone( tData.nModelId, nMaterial ) );
	} else
	{
		V( e_ModelRemoveMaterialOverride( tData.nModelId, nMaterial ) );
	}
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetModelEnvironmentOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nEnvDefId = e_EnvironmentLoad( tData.pEvent->pszData, TRUE );
	ASSERT( nEnvDefId != INVALID_ID );
	V( e_ModelSetEnvironmentOverride( tData.nModelId, nEnvDefId ) );
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearModelEnvironmentOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	V( e_ModelSetEnvironmentOverride( tData.nModelId, INVALID_ID ) );
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetGlobalEnvironmentOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	REGION* pRegion = e_GetCurrentRegion();
	ASSERT( pRegion );
	pRegion->bSavePrevEnvironmentDef = TRUE;
	e_SetEnvironmentDef( pRegion->nId, tData.pEvent->pszData, "", TRUE );
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearGlobalEnvironmentOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	REGION* pRegion = e_GetCurrentRegion();
	ASSERT( pRegion );
	ASSERT( pRegion->nPrevEnvironmentDef != INVALID_ID );
	pRegion->bSavePrevEnvironmentDef = FALSE;
	e_SetEnvironmentDef( pRegion->nId, 
		DefinitionGetNameById( DEFINITION_GROUP_ENVIRONMENT, 
		pRegion->nPrevEnvironmentDef ), "", TRUE );
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventShareWardrobe(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.idSource != UnitGetId( tData.pUnit ) );
	UNIT * pSource = UnitGetById( pGame, tData.idSource );
	if ( ! pSource )
		return;

	int nModelSource = c_UnitGetModelIdThirdPerson( pSource );
	ASSERT_RETURN( nModelSource != INVALID_ID );

	int nModelDefSource = e_ModelGetDefinition( nModelSource );
	ASSERT_RETURN( nModelDefSource != INVALID_ID );

	int nWardrobe = UnitGetWardrobe( tData.pUnit );

	c_WardrobeSetModelDefinitionOverride( nWardrobe, nModelDefSource );
	V( e_ModelSetDefinition( c_UnitGetModelIdThirdPerson( tData.pUnit ), nModelDefSource, LOD_ANY ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventShareWardrobeClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	int nWardrobe = UnitGetWardrobe( tData.pUnit );

	c_WardrobeSetModelDefinitionOverride( nWardrobe, INVALID_ID );

	int nModelDef = c_WardrobeGetModelDefinition( nWardrobe ); 

	V( e_ModelSetDefinition( c_UnitGetModelIdThirdPerson( tData.pUnit ), nModelDef, LOD_ANY ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetOverrideAppearance(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	int nMonsterClass = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nMonsterClass != INVALID_ID );
	UnitOverrideAppearanceSet( pGame, tData.pUnit, DATATABLE_MONSTERS, nMonsterClass );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearOverrideAppearance(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	UnitOverrideAppearanceClear( pGame, tData.pUnit );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetOverrideAppearanceObject(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	int nObjectClass = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nObjectClass != INVALID_ID );
	UnitOverrideAppearanceSet( pGame, tData.pUnit, DATATABLE_OBJECTS, nObjectClass );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearOverrideAppearanceObject(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	UnitOverrideAppearanceClear( pGame, tData.pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventScreenEffect(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	int nEffect = DefinitionGetIdByName( DEFINITION_GROUP_SCREENEFFECT, tData.pEvent->pszData );
	ASSERTV( nEffect != INVALID_ID, "Invalid screen effect definition \"%s\" in state \"%s\" event!",
		tData.pEvent->pszData,
		DefinitionGetNameById( DEFINITION_GROUP_STATES, tData.nState ) );

	if ( nEffect == INVALID_ID )
	{
		return;
	}

	// screen effect definition is restored by this function:
	e_ScreenEffectSetActive( nEffect, TRUE );

#endif //#if !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventScreenEffectClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( tData.pUnit != GameGetControlUnit( pGame ) )
		return;

	int nEffect = DefinitionGetIdByName( DEFINITION_GROUP_SCREENEFFECT, tData.pEvent->pszData );
	ASSERTV( nEffect != INVALID_ID, "Invalid screen effect definition \"%s\" in state \"%s\" event!",
		tData.pEvent->pszData,
		DefinitionGetNameById( DEFINITION_GROUP_STATES, tData.nState ) );

	if ( nEffect == INVALID_ID )
	{
		return;
	}

	e_ScreenEffectSetActive( nEffect, FALSE );

#endif //#if !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventCharacterLightingSet(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( tData.nModelId == INVALID_ID )
		return;

	int nExcelIndex = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );

	STATE_LIGHTING_DATA * pLighting = (STATE_LIGHTING_DATA*)ExcelGetData( pGame, DATATABLE_STATE_LIGHTING, nExcelIndex );

	if ( ! pLighting || ! pLighting->szSHCubemap[ 0 ] )
	{
		V( e_ModelClearBaseSHCoefs( tData.nModelId ) );
		return;
	}

	float fIntensity = MAX( tData.pEvent->fParam, 0.f );
	V( e_ModelSetBaseSHCoefs(
		tData.nModelId,
		fIntensity,
		pLighting->tBaseSHCoefs_O2,
		pLighting->tBaseSHCoefs_O3,
		&pLighting->tBaseSHCoefsLin_O2,
		&pLighting->tBaseSHCoefsLin_O3 ) );

#endif //#if !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventCharacterLightingClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( tData.nModelId == INVALID_ID )
		return;

	V( e_ModelClearBaseSHCoefs( tData.nModelId ) );

#endif //#if !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUICinematicMode(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	UISetCinematicMode( TRUE );

#endif //#if !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUICinematicModeClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	UISetCinematicMode( FALSE );

#endif //#if !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventPreventAllSkills(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( c_GetToolMode() && ! tData.pUnit )
		return;
	UnitSetPreventAllSkills( tData.pUnit, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventPreventAllSkillsClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( c_GetToolMode() && ! tData.pUnit )
		return;
	UnitSetPreventAllSkills( tData.pUnit, FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRemoveUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	//ASSERT_RETURN( IS_SERVER( pGame ) );  // there is a client version now
	if ( ! tData.pUnit )
		return;
	const UNIT_DATA * pUnitData = UnitGetData( tData.pUnit );
	if ( ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
	{
		UnitFree( tData.pUnit, UFF_DELAYED | UFF_SEND_TO_CLIENTS );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRemoveUnitIndestructible(
								   GAME *pGame,
								   const STATE_EVENT_DATA &tData)
{
	//ASSERT_RETURN( IS_SERVER( pGame ) );  // there is a client version now
	if ( ! tData.pUnit )
		return;

	UnitFree( tData.pUnit, UFF_DELAYED | UFF_SEND_TO_CLIENTS );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDelayedFree(
	 GAME * game, 
	 UNIT * unit, 
	 const EVENT_DATA & )
{
	UnitFree( unit, UFF_DELAYED | UFF_SEND_TO_CLIENTS );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventRemoveUnitAfterDelay(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	if ( ! tData.pUnit )
		return;
	const UNIT_DATA * pUnitData = UnitGetData( tData.pUnit );
	if ( ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
	{
		float fDelay = tData.pEvent->fParam;
		int nTicks = (int)(fDelay * GAME_TICKS_PER_SECOND) - 1;
		if ( nTicks > 0 )
		{
			UnitRegisterEventTimed( tData.pUnit, sDelayedFree, EVENT_DATA(), nTicks );
		}
		else
			sDelayedFree( pGame, tData.pUnit, EVENT_DATA() );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventHitFlashUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	c_AppearanceSetHit( c_UnitGetModelId(tData.pUnit) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleQuest_RTS_Start(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	// only change the UI and camera modes for the player who is commanding the RTS quest
	if (tData.pUnit && 
		tData.pUnit == GameGetControlUnit(pGame))
	{
		CLT_VERSION_ONLY(UIEnterRTSMode());
		c_CameraSetViewType( VIEW_RTS, TRUE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleQuest_RTS_End(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	// only change the UI and camera modes for the player who is commanding the RTS quest
	if (tData.pUnit && 
		tData.pUnit == GameGetControlUnit(pGame))
	{
		CLT_VERSION_ONLY(UIExitRTSMode());
		c_CameraRestoreViewType();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleShowExtraHotkeys(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
//	CLT_VERSION_ONLY(UISwitchComponents( UICOMP_ACTIVEBAR, UICOMP_RTS_ACTIVEBAR ));
	CLT_VERSION_ONLY(UIComponentActivate( UICOMP_RTS_ACTIVEBAR, TRUE ));

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleHideExtraHotkeys(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

#if !ISVERSION(SERVER_VERSION)
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

//	UISwitchComponents( UICOMP_RTS_ACTIVEBAR, UICOMP_ACTIVEBAR );
	UIComponentActivate( UICOMP_ACTIVEBAR, TRUE );

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleClearExtraHotkeys(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	for( int t = TAG_SELECTOR_SPELLHOTKEY1; t <= TAG_SELECTOR_SPELLHOTKEY12; t++ )
	{
		UNIT_TAG_HOTKEY * pTag = UnitGetHotkeyTag( tData.pUnit, t );
		if ( pTag )
		{
			UnitRemoveTag( tData.pUnit, pTag );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSetFirstPersonFOV(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
	int nDegrees = (int)tData.pEvent->fParam;
	c_CameraSetVerticalFov( nDegrees, VIEW_1STPERSON );

	// If this state is setting the FOV to something narrow enough (zoomed in), enter sniper distance mode
	if ( nDegrees < 60 )
	{
		e_ModelDistanceCullSetOverride( MONSTER_TARGET_MAXIMUM_DISTANCE );
	}

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSetThirdPersonFOV(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
	int nDegrees = (int)tData.pEvent->fParam;
	c_CameraSetVerticalFov( nDegrees, VIEW_3RDPERSON );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleRestoreFOV(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
	c_CameraRestoreFov();

	// In case we were in sniper distance mode, clear the override
	e_ModelDistanceCullClearOverride();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMessage(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
	float flFadeOutMultiplier = tData.pEvent->fParam;
	
	// get the string to display
	GLOBAL_STRING eString = (GLOBAL_STRING)sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	const WCHAR *puszMessage = GlobalStringGet( eString );
	UIShowQuickMessage( puszMessage, flFadeOutMultiplier );
	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetStat(
	 GAME *pGame,
	 const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
#if !ISVERSION(CLIENT_ONLY_VERSION)
	float fValue = tData.pEvent->fParam;

	int nStat = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	const STATS_DATA * pStatsData = StatsGetData( pGame, nStat );
	if (! pStatsData )
		return;
	if (! tData.pStats )
		return;

	if ( StatsDataTestFlag(pStatsData, STATS_DATA_FLOAT) )
	{
		StatsSetStatFloat( pGame, tData.pStats, nStat, 0, fValue );
	} else {
		StatsSetStat( pGame, tData.pStats, nStat, (int)fValue );
	}

	if (fValue > 0.0f &&
		pStatsData->m_nStatsType == STATSTYPE_MAX_PURE &&
		tData.pUnit)
	{
		// up the current to match the boost to the max (BOOST TO THE MAX!)
		ASSERT(pStatsData->m_nAssociatedStat[0] != INVALID_LINK);
		if (pStatsData->m_nAssociatedStat[0] != INVALID_LINK)
		{
			UnitChangeStat(tData.pUnit, pStatsData->m_nAssociatedStat[0], (int)fValue);
		}
	}

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sHandleEventIncStat(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
#if !ISVERSION(CLIENT_ONLY_VERSION)
	float fValue = tData.pEvent->fParam;

	int nStat = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	const STATS_DATA * pStatsData = StatsGetData( pGame, nStat );
	if (! pStatsData )
		return;
	if (! tData.pStats )
		return;

	if ( StatsDataTestFlag(pStatsData, STATS_DATA_FLOAT) )
	{
		float val =  (float)StatsGetStat(tData.pStats,nStat, 0);
		StatsSetStatFloat( pGame, tData.pStats, nStat, 0, val+fValue);
	} else {
		int val =  StatsGetStat(tData.pStats,nStat, 0);
		StatsSetStat( pGame, tData.pStats, nStat, val + (int)fValue );
	}

	if (fValue > 0.0f &&
		pStatsData->m_nStatsType == STATSTYPE_MAX_PURE &&
		tData.pUnit)
	{
		// up the current to match the boost to the max (BOOST TO THE MAX!)
		ASSERT(pStatsData->m_nAssociatedStat[0] != INVALID_LINK);
		if (pStatsData->m_nAssociatedStat[0] != INVALID_LINK)
		{
			UnitChangeStat(tData.pUnit, pStatsData->m_nAssociatedStat[0], (int)fValue);
		}
	}

#endif
}

static void sHandleEventDecStat(
								GAME *pGame,
								const STATE_EVENT_DATA &tData)
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
#if !ISVERSION(CLIENT_ONLY_VERSION)
	float fValue = tData.pEvent->fParam;

	int nStat = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	const STATS_DATA * pStatsData = StatsGetData( pGame, nStat );
	if (! pStatsData )
		return;
	if (! tData.pStats )
		return;

	if ( StatsDataTestFlag(pStatsData, STATS_DATA_FLOAT) )
	{
		float val =  (float)StatsGetStat(tData.pStats,nStat, 0);
		StatsSetStatFloat( pGame, tData.pStats, nStat, 0, val-fValue);
	} else {
		int val =  StatsGetStat(tData.pStats,nStat, 0);
		StatsSetStat( pGame, tData.pStats, nStat, val - (int)fValue );
	}

	if (fValue > 0.0f &&
		pStatsData->m_nStatsType == STATSTYPE_MAX_PURE &&
		tData.pUnit)
	{
		// up the current to match the boost to the max (BOOST TO THE MAX!)
		ASSERT(pStatsData->m_nAssociatedStat[0] != INVALID_LINK);
		if (pStatsData->m_nAssociatedStat[0] != INVALID_LINK)
		{
			UnitChangeStat(tData.pUnit, pStatsData->m_nAssociatedStat[0], -(int)fValue);
		}
	}

#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleInformQuestSet(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN( IS_SERVER( pGame ) );
	s_QuestEventUnitStateChange(tData.pUnit, tData.nState, TRUE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleInformQuestClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN( IS_SERVER( pGame ) );
	s_QuestEventUnitStateChange(tData.pUnit, tData.nState, FALSE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleQuestStatesUpdate(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN( IS_SERVER( pGame ) );
	s_QuestsUpdateAvailability( tData.pUnit );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSetStateOnPet( 
	UNIT *pPet, 
	void *pUserData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nState = *(int *)pUserData;
	s_StateSet( pPet, pPet, nState, 0 );
#endif	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sClearStateOnPet( 
	UNIT *pPet, 
	void *pUserData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nState = *(int *)pUserData;
	s_StateClear( pPet, UnitGetId( pPet ), nState, 0 );
#endif	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSetStateOnPets(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pUnit = tData.pUnit;
	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nState = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERTX_RETURN( nState != INVALID_LINK, "Expected state" )
	PetIteratePets( pUnit, sSetStateOnPet, &nState );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleClearStateOnPets(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pUnit = tData.pUnit;
	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nState = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERTX_RETURN( nState != INVALID_LINK, "Expected state" )
	PetIteratePets( pUnit, sClearStateOnPet, &nState );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetOnDeathState( 
	GAME * pGame,
	UNIT * pUnit )
{
	// this returns the first one that it finds... we only want one
	STATS* pStats = StatsGetModList( pUnit, SELECTOR_STATE );
	for ( ; pStats; pStats = StatsGetModNext(pStats, SELECTOR_STATE) )
	{
		PARAM dwState = 0;
		if ( ! StatsGetStatAny(pStats, STATS_STATE, &dwState ) )
			continue;

		if ( dwState == INVALID_PARAM )
			continue;

		const STATE_DATA * pStateData = StateGetData( pGame, dwState );
		if ( ! pStateData )
			continue;

		if ( pStateData->nOnDeathState != INVALID_ID )
			return pStateData->nOnDeathState;
	}
	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearStateFromEvent(
	GAME* pGame,
	UNIT* pUnit, 
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	if( !pHandlerData )	
		return;
	int nState = (int)pHandlerData->m_Data1;
	int nUnit = (int)pHandlerData->m_Data2;
	UNIT *unitExecuteOn = UnitGetById( pGame, nUnit );
	ASSERT_RETURN( unitExecuteOn );

	if(nState >= 0)
	{
		StateClearAllOfType(unitExecuteOn, nState);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventStateClearOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN( tData.pUnit );	//we need a parent to execute the skill on
	int nEventId = tData.pEvent->tAttachmentDef.nAttachedDefId;
	int nStateId =  tData.pEvent->nExcelIndex;

	ASSERT_RETURN( nEventId != INVALID_ID );
	ASSERT_RETURN( nStateId != INVALID_ID );

	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}
	const STATE_DATA *pOtherStateData = StateGetData( pGame, nStateId );
	if( !pOtherStateData ) //we need a skill
	{
		return;
	}
	const STATE_DATA *pThisStateData = StateGetData( pGame, tData.nState );
	if( !pThisStateData ) //we need the state
	{
		return;
	}

	DWORD dwId;
	UnitRegisterEventHandlerRet( dwId, pGame,  tData.pUnit, (UNIT_EVENT)nEventId, sClearStateFromEvent, &EVENT_DATA( nStateId, UnitGetId( tData.pUnit ), pThisStateData->nDefinitionId ) ); //store the skill id, and unit ID
	StatsSetStat( pGame, pStats, STATS_EVENT_REF, nEventId, dwId );	//save the id	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearStateClearOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	int nEventId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nStateId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );

	ASSERT_RETURN( nEventId != INVALID_ID );
	ASSERT_RETURN( nStateId != INVALID_ID );

	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}

	DWORD dwId = StatsGetStat( pStats, STATS_EVENT_REF, nEventId );	//save the id	
	if( dwId > 0 )
		UnitUnregisterEventHandler( pGame,  tData.pUnit, (UNIT_EVENT)nEventId, dwId );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobeAddLayer(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;
#if !ISVERSION( SERVER_VERSION )
	ASSERT( IS_CLIENT( pGame ) );
	int nWardrobeLayer = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if ( nWardrobeLayer == INVALID_ID )
		return;

	c_WardrobeForceLayerOntoUnit( tData.pUnit, nWardrobeLayer );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobeRemoveLayer(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;
#if !ISVERSION( SERVER_VERSION )
	ASSERT( IS_CLIENT( pGame ) );
	int nWardrobeLayer = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if ( nWardrobeLayer == INVALID_ID )
		return;

	c_WardrobeRemoveLayerFromUnit( tData.pUnit, nWardrobeLayer );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExecuteSkillFromEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	if( !pHandlerData )	
		return;
	int nSkill = (int)pHandlerData->m_Data1;
	int nUnit = (int)pHandlerData->m_Data2;
	BOOL bUseUnitAsTarget = (BOOL)pHandlerData->m_Data4;
	int nSkillLevel = (int)pHandlerData->m_Data5;
	UNIT *unitExecuteOn = UnitGetById( pGame, nUnit );
	ASSERT_RETURN( unitExecuteOn );
	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
	ASSERT_RETURN(pSkillData);

	STATS * pStatsList = NULL;
	if(SkillDataTestFlag(pSkillData, SKILL_FLAG_UNIT_EVENT_TRIGGER_SKILL_NEEDS_DAMAGE_INCREMENT))
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		const struct COMBAT * pCombat = (const struct COMBAT*)pData;
		int nCombatDamageIncrement = 0;
		if(pCombat)
		{
			nCombatDamageIncrement = s_CombatGetDamageIncrement(pCombat);
		}
		if(nCombatDamageIncrement > 0)
		{
			pStatsList = StatsListInit(pGame);
			if(pStatsList)
			{
				StatsSetStat(pGame, pStatsList, STATS_STATE_EVENT_DAMAGE_INCREMENT, nCombatDamageIncrement);
				StatsListAdd(unitExecuteOn, pStatsList, TRUE);
			}
		}
#endif
	}

	UNIT * pTarget = bUseUnitAsTarget ? pUnit : pOtherUnit;

	SkillStartRequest( 
		pGame, 
		unitExecuteOn, 
		nSkill, 
		UnitGetId( pTarget ), 
		UnitGetPosition( unitExecuteOn ), 
		dwFlags, 
		SkillGetNextSkillSeed( pGame ),
		nSkillLevel);

	if(pStatsList)
	{
		StatsListFree(pGame, pStatsList);
	}
}

//----------------------------------------------------------------------------
//This will add an event call back for the state that will then execute a skill
//----------------------------------------------------------------------------
static void sDoEventExecuteSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData,
	UNITID nUnitToExecuteOn,
	BOOL bUseUnitAsTarget)
{
	ASSERT_RETURN( tData.pUnit );	//we need a parent to execute the skill on
	ASSERT_RETURN( nUnitToExecuteOn != INVALID_ID );

	int nEventId = tData.pEvent->tAttachmentDef.nAttachedDefId;
	int nSkillId =  tData.pEvent->nExcelIndex;

	ASSERT_RETURN( nEventId != INVALID_ID );
	ASSERT_RETURN( nSkillId != INVALID_ID );

	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}
	const SKILL_DATA *skillData = SkillGetData( pGame, nSkillId );
	if( !skillData ) //we need a skill
	{
		return;
	}
	const STATE_DATA *stateData = StateGetData( pGame, tData.nState );
	if( !stateData ) //we need the state
	{
		return;
	}
	int nSkillLevel = 0;
	if ( tData.pStats )
	{
		PARAM dwParam = 0;
		nSkillLevel = StatsGetStatAny( tData.pStats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam );
	}

	DWORD dwId;
	UnitRegisterEventHandlerRet( dwId, pGame, tData.pUnit, (UNIT_EVENT)nEventId, sExecuteSkillFromEvent, &EVENT_DATA( skillData->nId, nUnitToExecuteOn, stateData->nDefinitionId, bUseUnitAsTarget, nSkillLevel ) ); //store the skill id, and unit ID
	StatsSetStat( pGame, pStats, STATS_EVENT_REF, nEventId, dwId );	//save the id	
}

//----------------------------------------------------------------------------
//This will add an event call back for the state that will then execute a skill
//----------------------------------------------------------------------------
static void sHandleEventExecuteSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	sDoEventExecuteSkillOnEvent(pGame, tData, UnitGetId( tData.pUnit ), FALSE);
}

//----------------------------------------------------------------------------
//This will add an event call back for the state that will then execute a skill
//----------------------------------------------------------------------------
static void sHandleEventExecuteSourceSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	sDoEventExecuteSkillOnEvent(pGame, tData, tData.idSource, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define			MAX_DISTANCE_AWAY_ALLOWED 30

static void sHandleEventAwardAttackerLuckOnUnitDeath(
		GAME *pGame,
		const STATE_EVENT_DATA &tData )
{

#define			NUM_LUCK 100.0f
#define			NUM_SMALL_LUCK_IN_BIG 7
#define			NUM_LUCK_BIG_MAX 10

	if( IS_SERVER( pGame ) )
		return;			//client only	
	if( AppIsHellgate() )
		return;			//Hellgate doesn't use this right now
	/*UNIT *pUnitGiving = tData.pUnit;
	UNIT *pUnitRecieving = UnitGetById( pGame, tData.idSource );
	if( !( pUnitGiving &&
		   pUnitRecieving ) )
	{
		return;			//we only do this for Monsters and players
	}

	if( UnitGetGenus( pUnitRecieving ) == GENUS_OBJECT )
	{
		UNIT *pUnit = pUnitGiving;
		pUnitGiving = pUnitRecieving;
		pUnitRecieving = pUnit;		
	}
	//we need some way of setting these values.
	if( VectorDistanceXY( UnitGetPosition( pUnitGiving ), UnitGetPosition( pUnitRecieving ) ) > MAX_DISTANCE_AWAY_ALLOWED )
	{
		return; //player is to far away
	}

	int LuckAward = UnitGetLuckToAward( pUnitGiving );	
	BOOL isPlayer = UnitIsPlayer( pUnitGiving );
	if( isPlayer )
		LuckAward = (int)(NUM_LUCK * NUM_SMALL_LUCK_IN_BIG * 2.0f + NUM_SMALL_LUCK_IN_BIG - 1.0f); //the player is giving away luck.

	if( LuckAward == 0 )
		return;			// have to have luck and XP
	int LuckAwardBig( 0 );
	LuckAward = (int)CEIL( (float)LuckAward/NUM_LUCK);
	if( LuckAward > 0 )
	{
		LuckAwardBig = LuckAward/NUM_SMALL_LUCK_IN_BIG;
		LuckAward -= LuckAwardBig * NUM_SMALL_LUCK_IN_BIG;
		if( LuckAwardBig > NUM_LUCK_BIG_MAX )
			LuckAwardBig = NUM_LUCK_BIG_MAX;
	}

	for( int t = 0; t < LuckAward; t++ )
	{
		c_StateSet( pUnitGiving, pUnitRecieving, (isPlayer)?STATE_REMOVEVISUALLUCKSMALL:STATE_AWARDVISUALLUCKSMALL, 0, 0, INVALID_ID );
	}
	for( int t = 0; t < LuckAwardBig; t++ )
	{
		c_StateSet( pUnitGiving, pUnitRecieving, (isPlayer)?STATE_REMOVEVISUALLUCKBIG:STATE_AWARDVISUALLUCKBIG, 0, 0, INVALID_ID );
	}

*/
}


//----------------------------------------------------------------------------
//This handles when a unit dies that it gives a visual feed back
//----------------------------------------------------------------------------

static void sHandleEventAwardAttackerXPOnUnitDeath(
		GAME *pGame,
		const STATE_EVENT_DATA &tData )
{


#define			NUM_SMALL_XP_IN_BIG 5
#define			NUM_XP_BIG_MAX 5


	if( gDisableXPVisuals ||
		IS_SERVER( pGame ) )
		return;			//client only	
	if( AppIsHellgate() )
		return;			//Hellgate doesn't use this right now
	UNIT *pUnitGiving = tData.pUnit;
	UNIT *pUnitRecieving = UnitGetById( pGame, tData.idSource );
	if( !( pUnitGiving &&
		   pUnitRecieving  ) )
	{
		return;			//we only do this for Monsters and players
	}

	//we need some way of setting these values.
	if( VectorDistanceXY( UnitGetPosition( pUnitGiving ), UnitGetPosition( pUnitRecieving ) ) > MAX_DISTANCE_AWAY_ALLOWED )
	{
		return; //player is to far away
	}

	int XPAward = pUnitGiving->pUnitData->nExperiencePct; //it's all relative for us. So we can use the Pct as a ref for how much visual XP they are getting
	int XPAwardBig( 0 );	
	int nLevel = UnitGetExperienceLevel( pUnitRecieving );
	//int nRecieverLevel = UnitGetExperienceLevel( pUnitRecieving );
	//if( nLevel < nRecieverLevel )
	//	nLevel = nRecieverLevel;	
	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETURN( pMonLevel );
	int nMaxExperience = (int)max( 1, (int)(((float)pMonLevel->nExperience * .50f) * 100.0f) );		
	
	if( XPAward > 0 )
	{
		
		XPAward = (int)max( 1, (XPAward /nMaxExperience) );
		XPAwardBig = XPAward/NUM_SMALL_XP_IN_BIG;
		XPAward -= XPAwardBig * NUM_SMALL_XP_IN_BIG;
		if( XPAwardBig > NUM_XP_BIG_MAX )
			XPAwardBig = NUM_XP_BIG_MAX;


	}
	else
	{
		XPAward = 0; 
	}	 
	
	//we are removing this for play test now
	
	
	for( int t = 0; t < XPAward; t++ )
	{
		c_StateSet( pUnitGiving, pUnitRecieving, STATE_AWARDVISUALXPSMALL, 0, 0, INVALID_ID );
	}
	for( int t = 0; t < XPAwardBig; t++ )
	{
		c_StateSet( pUnitGiving, pUnitRecieving, STATE_AWARDVISUALXPBIG, 0, 0, INVALID_ID );
	}
	

}




//----------------------------------------------------------------------------
//This fires missiles on on the CLIENT
//----------------------------------------------------------------------------
static void sHandleEventFireVisualHomingMissile(
		GAME *pGame,
		const STATE_EVENT_DATA &tData )
{
//cone of release is angle/180.  So .5 would be a cone of 90%, 1.0f would be 180 and 2 would be a circle 
#define CONE_RELEASE 2.0
	UNIT *pUnitFireFrom = tData.pUnit;
	UNIT *pTarget = UnitGetById( pGame, tData.idSource );
	if( !( pUnitFireFrom &&
		   pTarget ) )
	{
		return;			
	}	

	int nMissileClass = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );

	if( nMissileClass == -1 )
	{
		return;	//we need a valid missile class.
	}	

	// get source position	
	VECTOR vUnitPosition = UnitGetPosition( pUnitFireFrom );
	VECTOR vPlayerPosition = UnitGetPosition( pTarget );
	float flRadiusMax = UnitGetCollisionRadius( pUnitFireFrom );
	float flRadius = 0.0f;//flRadiusMax * .5f;	

	VECTOR vForward = vUnitPosition - vPlayerPosition;
	VectorNormalize( vForward, vForward );	
		
	int count = (int)tData.pEvent->fParam;
	if( count <= 0 )
		count = 1;
	for( int t = 0; t < count; t++ )
	{

			
		// pick source location randomly offset in the collision area of the unit
		VECTOR vPosition = vUnitPosition;
		vPosition.fX = (flRadius * RandGetFloat( pGame, -1.0, 1.0f )) + vUnitPosition.fX;
		vPosition.fY = (flRadius * RandGetFloat( pGame, -1.0, 1.0f )) + vUnitPosition.fY;
		vPosition.fZ = ( flRadius * RandGetFloat( pGame, 0.0, 1.0f ) ) + vUnitPosition.fZ;
		

		// pick direction vector
		VECTOR vDirection;	
		VectorScale( vDirection, vForward, -1 ); //opposite vector of the forward		
		vDirection.fX *= RandGetFloat( pGame, 0, CONE_RELEASE );	//Cone of release
		vDirection.fY *= RandGetFloat( pGame, 0, CONE_RELEASE );	//Cone of release
		vDirection = vDirection + vForward;
		VectorNormalize( vDirection, vDirection );
		UNIT *pMissile = MissileFire(	pGame, 
										nMissileClass,  //missile XP
										pUnitFireFrom, 
										NULL,			// weapon
										INVALID_LINK,	// skill
										pTarget,		// target unit
										vPlayerPosition,
										vPosition, 
										vDirection, 
										0,				// random seed
										0,
										0,
										NULL );

		if( pMissile )
		{
			if( pTarget && pMissile->nTeam == INVALID_ID )
			{
				if( UnitGetTeam( pTarget ) == TEAM_PLAYER_START )
				{
					UnitSetTeam( pMissile, TEAM_BAD );
				}
				else
				{
					UnitSetTeam( pMissile, TEAM_PLAYER_START );
				}
			}
			else
			{
				UnitSetTeam( pMissile, TEAM_BAD );
			}
			UnitSetOwner( pMissile, NULL );
			if( pMissile->pUnitData->flHomingAfterXSeconds == 0 )
			{
				MissileSetHoming( pMissile, 0, flRadiusMax );
				UnitSetStat( pMissile, STATS_HOMING_TARGET_ID, tData.idSource );
			}
		}
	}





}

//----------------------------------------------------------------------------
//This removes the attached missiles that are stored in the state's stats
//----------------------------------------------------------------------------
static void sHandleRemoveMissiles(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	UNITID idMissile = StatsGetStat( tData.pStats, STATS_STATE_MISSILE_ID );
	if ( idMissile != INVALID_ID )
	{
		UNIT * pMissile = UnitGetById( pGame, idMissile );
		if ( pMissile )
		{
			MissileConditionalFree( pMissile, UFF_DELAYED );
		}
	}
}

//----------------------------------------------------------------------------
//This will add an event call back for the state that will then execute a skill
//----------------------------------------------------------------------------
static void sHandleEventClearExecuteSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	int nEventId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nSkillId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );

	ASSERT_RETURN( nEventId != INVALID_ID );
	ASSERT_RETURN( nSkillId != INVALID_ID );
	
	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}
	DWORD dwId = StatsGetStat( pStats, STATS_EVENT_REF, nEventId );	//save the id	
	if( dwId > 0 )
		UnitUnregisterEventHandler( pGame,  tData.pUnit, (UNIT_EVENT)nEventId, dwId );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSetHotkey(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	if ( ! tData.pUnit )
		return;

	int nSkill = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nHotkey = (int) tData.pEvent->fParam;

	UNIT_TAG_HOTKEY * pTag = UnitGetHotkeyTag( tData.pUnit, TAG_SELECTOR_HOTKEY1 + nHotkey );
	if ( ! pTag )
	{
		pTag = UnitAddHotkeyTag( tData.pUnit, TAG_SELECTOR_HOTKEY1 + nHotkey );
	}
	pTag->m_nSkillID[ 0 ] = nSkill;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSetMouseSkills(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pStats || ! tData.pUnit )
		return;

	int nSkillRight = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nSkillLeft  = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );

	ASSERT_RETURN( nSkillRight != INVALID_ID );
	ASSERT_RETURN( nSkillLeft  != INVALID_ID );

	StatsSetStat( pGame, tData.pStats, STATS_SKILL_RIGHT, UnitGetStat( tData.pUnit, STATS_SKILL_RIGHT ) );
	StatsSetStat( pGame, tData.pStats, STATS_SKILL_LEFT,  UnitGetStat( tData.pUnit, STATS_SKILL_LEFT ) );

	UnitSetStat( tData.pUnit, STATS_SKILL_RIGHT, nSkillRight );
	UnitSetStat( tData.pUnit, STATS_SKILL_LEFT,  nSkillLeft );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleRestoreMouseSkills(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pStats || ! tData.pUnit )
		return;

	int nSkillRight = StatsGetStat( tData.pStats, STATS_SKILL_RIGHT );
	int nSkillLeft  = StatsGetStat( tData.pStats, STATS_SKILL_LEFT );

	if ( nSkillRight != INVALID_ID )
		UnitSetStat( tData.pUnit, STATS_SKILL_RIGHT, nSkillRight );
	if ( nSkillLeft != INVALID_ID )
		UnitSetStat( tData.pUnit, STATS_SKILL_LEFT,  nSkillLeft );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventCooldownSkill(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	int nSkill = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * tData.pEvent->fParam);

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if(nTicks < 0)
	{
		SkillStartCooldown( pGame, tData.pUnit, NULL, nSkill, 0, pSkillData, 0, FALSE );
	}
	else
	{
		SkillStartCooldown( pGame, tData.pUnit, NULL, nSkill, 0, pSkillData, nTicks, TRUE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCooldownOwnerSkillOnEvent(
	GAME* pGame,
	UNIT* pUnit, 
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	if( !pHandlerData )	
		return;
	int idOwner = (int)pHandlerData->m_Data1;
	int nSkill = (int)pHandlerData->m_Data2;
	int nTicks = (int)pHandlerData->m_Data3;
	UNIT *pOwner = UnitGetById( pGame, idOwner );
	if ( pOwner )
	{
		const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );
		if ( nTicks <= 0 )
		{
			SkillStartCooldown( pGame, pOwner, NULL, nSkill, 0, pSkillData, 0, FALSE );
		}
		else
		{
			SkillStartCooldown( pGame, pOwner, NULL, nSkill, 0, pSkillData, nTicks, TRUE );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetEventHandler_CooldownOwnerSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	UNIT *pOwner = UnitGetUltimateOwner( tData.pUnit );

	if ( ! pOwner )
		return;

	int nSkill =  tData.pEvent->nExcelIndex;
	ASSERT_RETURN( nSkill != INVALID_LINK );
	int nTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * tData.pEvent->fParam);

	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}

	int nEventId = tData.pEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETURN( nEventId != INVALID_ID );

	DWORD dwId;
	UnitRegisterEventHandlerRet( dwId, pGame, tData.pUnit, (UNIT_EVENT)nEventId, sCooldownOwnerSkillOnEvent, &EVENT_DATA( UnitGetId( pOwner ), nSkill, nTicks ) ); //store the skill id, and the override cooldown if any
	StatsSetStat( pGame, pStats, STATS_EVENT_REF, nEventId, dwId );	//save the id	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearEventHandler_CooldownOwnerSkillOnEvent(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	int nEventId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	int nStateId = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_EVENT );

	ASSERT_RETURN( nEventId != INVALID_ID );
	ASSERT_RETURN( nStateId != INVALID_ID );

	STATS * pStats = tData.pStats;
	if (!pStats)	//we need the stats
	{
		return;
	}

	DWORD dwId = StatsGetStat( pStats, STATS_EVENT_REF, nEventId );	//save the id	
	if( dwId > 0 )
		UnitUnregisterEventHandler( pGame,  tData.pUnit, (UNIT_EVENT)nEventId, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetAddToOwnerTeam(
	UNIT *pPet,
	void *pUserData )
{
	UNIT * pOwner = (UNIT*)pUserData;
	ASSERT_RETURN( pOwner );
	ASSERT_RETURN( pPet );
	TeamAddUnit( pPet, pOwner->nTeam );
}

//----------------------------------------------------------------------------
//Switches the player to a temporary team just for them and the units they control.
//----------------------------------------------------------------------------
static void sHandleSwitchTeamToTempPlayerTeam(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERTX_RETURN( tData.pUnit, "No Unit Found.");
	ASSERTX_RETURN( UnitIsPlayer( tData.pUnit ), "Expected unit to be a player.");
	ASSERTX_RETURN( tData.pStats, "Stats were not passed in.");

	// free for all, make a new (temporary) team for each player and their pets
	int nTeam = TeamsAdd( pGame, NULL, NULL, TEAM_PLAYER_PVP_FFA, RELATION_HOSTILE ); // team system handles cleanup when the team is empty after the player leaves the game
	ASSERTX_RETURN( nTeam != INVALID_TEAM, "state event failed to create teams" );

	ASSERT_RETURN( TeamAddUnit( tData.pUnit, nTeam ) );

	PetIteratePets( tData.pUnit , sPetAddToOwnerTeam, tData.pUnit );
}

//----------------------------------------------------------------------------
//Removes the team override
//----------------------------------------------------------------------------
static void sHandleRemoveTempPlayerTeam(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERTX_RETURN( tData.pUnit, "No Unit Found.");
	ASSERTX_RETURN( UnitIsPlayer( tData.pUnit ), "Expected unit to be a player.");
	ASSERTX_RETURN( tData.pStats, "Stats were not passed in.");

	int nOldTeam = UnitGetTeam( tData.pUnit );
	ASSERTX_RETURN( nOldTeam != INVALID_TEAM, "state event expected a team on the player" );
	ASSERTX_RETURN( nOldTeam >= 0, "state event expected a positive team index on the player" );
	int nNewTeam = UnitGetDefaultTeam( pGame, tData.pUnit );
	ASSERTX_RETURN( nNewTeam != INVALID_TEAM, "state event expected a default team for the player" );
	ASSERTX_RETURN( nNewTeam >= 0, "state event expected a positive default team index for the player" );
	TeamAddUnit( tData.pUnit, nNewTeam );
	PetIteratePets( tData.pUnit , sPetAddToOwnerTeam, tData.pUnit );
}

//----------------------------------------------------------------------------
//Switches the unit to the source unit's team. Can override by stat being passed...
//0(blank) - use SourceUnit
//1 - make evil
//2 - make good
//----------------------------------------------------------------------------
static void sHandleSwitchTeamToSourceUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERTX_RETURN( tData.pUnit, "No Unit Found.");
	ASSERTX_RETURN( tData.pStats, "Stats were not passed in.");
	int changeToTeam = TEAM_BAD;
	TARGET_TYPE targetType = TARGET_BAD;

	switch( (int)tData.pEvent->fParam )
	{
	case 0:
		{
			UNIT *pSourceUnit = UnitGetById(  pGame, tData.idSource );
			ASSERTX_RETURN( pSourceUnit, "Trying to change team of target but no source unit is available." );
			changeToTeam = UnitGetTeam( pSourceUnit ); //get the team
			targetType = UnitGetTargetType( pSourceUnit );
		}
		break;
	case 1:
		{
			changeToTeam = TEAM_BAD; //bad
			targetType = TARGET_GOOD;
		}
		break;
	case 2:
		{
			changeToTeam = TEAM_PLAYER_START; //player
			targetType = TARGET_BAD;
		}
		break;
	}

	//int nCurrentTeam = UnitGetTeam(tData.pUnit);
	//TARGET_TYPE eCurrentTargeType = UnitGetTargetType(tData.pUnit);
	int nOverrideIndex = UnitOverrideTeam(pGame, tData.pUnit, changeToTeam, targetType);
	StatsSetStat( pGame, tData.pStats, STATS_TEAM_OVERRIDE_INDEX, nOverrideIndex );
	// Invalidate their last target
	// because of the way Mythos uses lastattacker, we need to clear it out during
	// an alignment switch.
	if( AppIsTugboat() )
	{
		UnitSetAITarget(tData.pUnit, INVALID_ID);
		UnitSetStat( tData.pUnit, STATS_AI_LAST_ATTACKER_ID, INVALID_ID );
	}
}

//----------------------------------------------------------------------------
//Removes the team override
//----------------------------------------------------------------------------
static void sHandleRemoveTeamSetBySourceUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERTX_RETURN( tData.pUnit, "No Unit Found.");
	ASSERTX_RETURN( tData.pStats, "Stats were not passed in.");

	int nOverrideIndex = StatsGetStat( tData.pStats, STATS_TEAM_OVERRIDE_INDEX );
	if(nOverrideIndex >= 0)
	{
		UnitCancelOverrideTeam(pGame, tData.pUnit, nOverrideIndex);
	}
}

//----------------------------------------------------------------------------
// this event doesn't handle stacking... if it happens twice, it will blow up
//----------------------------------------------------------------------------
static void sHandleChangeCameraUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	int nInventoryLocation = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	if ( nInventoryLocation == INVALID_ID )
		return;

	UNIT * pCameraUnit = UnitInventoryGetByLocation( tData.pUnit, nInventoryLocation );
	if ( ! pCameraUnit )
		return;

	GameSetCameraUnit( pGame, UnitGetId( pCameraUnit ));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleClearCameraUnit(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	GameSetCameraUnit( pGame, UnitGetId( GameGetControlUnit( pGame ) ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleFirstPersonCamera(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	c_CameraSetViewType( VIEW_1STPERSON, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleThirdPersonCamera(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	c_CameraSetViewType( VIEW_3RDPERSON, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleRestoreCamera(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	c_CameraRestoreViewType();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetBGSoundOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	int nOverrideID = c_BGSoundsSetOverride(tData.pEvent->tAttachmentDef.nAttachedDefId);
	StatsSetStat( pGame, tData.pStats, STATS_BACKGROUND_SOUND_OVERRIDE, nOverrideID );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearBGSoundOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( tData.pUnit == GameGetControlUnit( pGame ) );

	int nOverrideID = StatsGetStat(tData.pStats, STATS_BACKGROUND_SOUND_OVERRIDE);
	if(nOverrideID < 0)
	{
		return;
	}

	c_BGSoundsClearOverride(nOverrideID);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSetSoundMixState(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	if( tData.pUnit != GameGetControlUnit( pGame ) )
		return;

	// If we already had this state set, then don't do anything
	if(tData.bHadState)
		return;

	// If this state already has a mix state set, then don't do anything else
	if(!tData.pStats || StatsGetStat(tData.pStats, STATS_SOUND_MIX_STATE) >= 0)
		return;

	int nTicks = int(tData.pEvent->fParam);

	int nMixStateID = c_SoundSetMixState(tData.pEvent->tAttachmentDef.nAttachedDefId, nTicks);
	StatsSetStat( pGame, tData.pStats, STATS_SOUND_MIX_STATE, nMixStateID );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearSoundMixState(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	if( tData.pUnit != GameGetControlUnit( pGame ) )
		return;

	int nMixStateID = StatsGetStat(tData.pStats, STATS_SOUND_MIX_STATE);
	if(nMixStateID < 0)
	{
		return;
	}

	c_SoundReleaseMixState(nMixStateID);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventClearPlayerMovement(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	if( tData.pUnit != GameGetControlUnit( pGame ) )
		return;

	c_PlayerClearAllMovement( pGame );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUpdateAnimationContext(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	if ( ! tData.pUnit )
		return;

	if ( tData.nModelId == INVALID_ID )
		return;

	c_AppearanceUpdateContext( tData.nModelId, TRUE );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAnimationGroupSet(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN(pGame && IS_SERVER( pGame ));

	if ( ! tData.pUnit )
		return;

	ASSERT_RETURN( UnitGetStat( tData.pUnit, STATS_ANIMATION_GROUP_OVERRIDE ) == INVALID_ID );

	int nAnimGroup = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	UnitSetStat( tData.pUnit, STATS_ANIMATION_GROUP_OVERRIDE, nAnimGroup );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventAnimationGroupClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
	ASSERT_RETURN(pGame && IS_SERVER( pGame ));

	if ( ! tData.pUnit )
		return;

	UnitSetStat( tData.pUnit, STATS_ANIMATION_GROUP_OVERRIDE, INVALID_ID );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSourceTakeResponsibility(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(pGame && IS_SERVER(pGame));

	if ( ! tData.pUnit )
		return;

	UnitSetUnitIdTag(tData.pUnit, TAG_SELECTOR_RESPONSIBLE_UNIT, tData.idSource);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventSourceTakeResponsibilityClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(pGame && IS_SERVER(pGame));

	if ( ! tData.pUnit )
		return;

	UnitSetUnitIdTag(tData.pUnit, TAG_SELECTOR_RESPONSIBLE_UNIT, INVALID_ID);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUIComponentActivate(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nUIComp = tData.pEvent->tAttachmentDef.nAttachedDefId;
	if (nUIComp != INVALID_LINK)
	{
		UI_COMPONENT_ENUM eComp = (UI_COMPONENT_ENUM)nUIComp;
		UIComponentActivate( eComp, TRUE );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUIComponentDeactivate(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nUIComp = tData.pEvent->tAttachmentDef.nAttachedDefId;
	if (nUIComp != INVALID_LINK)
	{
		UI_COMPONENT_ENUM eComp = (UI_COMPONENT_ENUM)nUIComp;
		UIComponentActivate( eComp, FALSE );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUIComponentShow(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nUIComp = tData.pEvent->tAttachmentDef.nAttachedDefId;
	if (nUIComp != INVALID_LINK)
	{
		UI_COMPONENT_ENUM eComp = (UI_COMPONENT_ENUM)nUIComp;
		UIComponentSetVisibleByEnum( eComp, TRUE );
		if (tData.pEvent->fParam > 0.0f)
		{
			UIComponentActivate( eComp, TRUE );
		}		
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventUIComponentHide(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nUIComp = tData.pEvent->tAttachmentDef.nAttachedDefId;
	if (nUIComp != INVALID_LINK)
	{
		UI_COMPONENT_ENUM eComp = (UI_COMPONENT_ENUM)nUIComp;
		UIComponentSetVisibleByEnum( eComp, FALSE );
		if (tData.pEvent->fParam > 0.0f)
		{
			UIComponentActivate( eComp, FALSE );
		}				
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobePrimaryReplacesSkin(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)

	if ( tData.nModelId == INVALID_ID )
		return;

	int nWardrobe = c_AppearanceGetWardrobe( tData.nModelId );
	if ( nWardrobe != INVALID_ID )
		c_WardrobeSetPrimaryColorReplacesSkin( pGame, nWardrobe, TRUE );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobePrimaryReplacesSkinClear(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)

	if ( tData.nModelId == INVALID_ID )
		return;

	int nWardrobe = c_AppearanceGetWardrobe( tData.nModelId );
	if ( nWardrobe != INVALID_ID )
		c_WardrobeSetPrimaryColorReplacesSkin( pGame, nWardrobe, FALSE );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobeSetColorsetOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{
	if ( ! tData.pUnit )
		return;

	int nColorset = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nColorset != INVALID_ID );
 
	ASSERT_RETURN( UnitGetStat( tData.pUnit, STATS_COLORSET_OVERRIDE, nColorset ) == 0 );
	UnitSetStat( tData.pUnit, STATS_COLORSET_OVERRIDE, nColorset, 1 );

	UnitSetFlag( tData.pUnit, UNITFLAG_WARDROBE_CHANGED );
	WardrobeUpdateFromInventory( tData.pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEventWardrobeClearColorsetOverride(
	GAME *pGame,
	const STATE_EVENT_DATA &tData)
{

	if ( ! tData.pUnit )
		return;

	int nColorset = sEventGetIndexForExcel( tData.pEvent, STATE_TABLE_REFERENCE_ATTACHMENT );
	ASSERT_RETURN( nColorset != INVALID_ID );

	UnitSetStat( tData.pUnit, STATS_COLORSET_OVERRIDE, nColorset, 0 );

	UnitSetFlag( tData.pUnit, UNITFLAG_WARDROBE_CHANGED );
	WardrobeUpdateFromInventory( tData.pUnit );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateRunScriptsOnUnitStates( UNIT *pUnit,
								  UNIT *pTarget,
								  int ScriptFlags, /*STATE_SCRIPTS_ENUM*/
								  STATS *pStats )
{
	ASSERT_RETURN(pUnit);
	if( pTarget == NULL )
		pTarget = pUnit;
	GAME *pGame = UnitGetGame( pUnit );	
	UNIT *pUltimateOwner = UnitGetUltimateOwner( pUnit );
	STATS* stats = StatsGetModList( pUltimateOwner, SELECTOR_STATE);
	STATS_ENTRY stats_entries[1];
	while (stats)
	{
		{
			int nStateCount = StatsGetRange(stats, STATS_STATE, 0, stats_entries, 1);
			if( nStateCount <= 0 )
			{
				stats = StatsGetModNext(stats, SELECTOR_STATE);
				continue;
			}
			int nState = (int)stats_entries[0].GetParam();
			if( nState == INVALID_ID ) //invalid state
			{
				stats = StatsGetModNext(stats, SELECTOR_STATE);
				continue;
			}
			const STATE_DATA *pStateDef = StateGetData( pGame, nState );
			if( !pStateDef ||
				(pStateDef->dwFlagsScripts & ScriptFlags ) == 0 )	//state doesn't have script flags just continue
			{
				stats = StatsGetModNext(stats, SELECTOR_STATE);
				continue;
			}
			PARAM dwParam = 0;
			StatsGetStatAny( stats, STATS_STATE_SOURCE_SKILL_LEVEL, &dwParam );
			int nSkill = dwParam;			
			if( nSkill == INVALID_ID )	//bad skill
			{
				stats = StatsGetModNext(stats, SELECTOR_STATE);
				continue;
			}
			const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );
			if( !pSkillData )	//bad skill
			{
				stats = StatsGetModNext(stats, SELECTOR_STATE);
				continue;
			}
			int nSkillLevel = StatsGetStat( stats, STATS_STATE_SOURCE_SKILL_LEVEL, nSkill ); //skill level
			UNIT *pUnitSource( pUnit );		
			if (StatsGetRange(stats, STATS_STATE_SOURCE, 0, stats_entries, 1) != 0)
			{
				UNITID nUnitSource = (UNITID)stats_entries[0].value;
				if( nUnitSource == INVALID_ID )
				{
					stats = StatsGetModNext(stats, SELECTOR_STATE);
					continue;
				}
				pUnitSource = UnitGetById( pGame, nUnitSource );
				if( !pUnitSource )
				{
					stats = StatsGetModNext(stats, SELECTOR_STATE);
					continue;
				}
			}
			//ok we are ready to rock...
			SKILL_SCRIPT_ENUM scriptValue = SKILL_SCRIPT_TRANSFER_ON_ATTACK;
			/*
			don't need right now; but if we add the functionality for states to run multiple scripts at different times...
			if( pStateDef->dwFlagsScripts & STATE_SCRIPT_FLAG_ATTACK_MELEE ||
				pStateDef->dwFlagsScripts & STATE_SCRIPT_FLAG_ATTACK_RANGED )
			{
				scriptEnum = SKILL_SCRIPT_ENUM::SKILL_SCRIPT_TRANSFER_ON_ATTACK; 
			}
			*/
			SkillExecuteScript( scriptValue,
								pUnitSource,
								nSkill,
								nSkillLevel,
								pUnit,
								pStats,
								INVALID_ID,
								INVALID_ID,
								nState );
		}

		stats = StatsGetModNext(stats, SELECTOR_STATE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void StateClearAttackState( UNIT *pUnit )
{	
	if( AppIsHellgate() )
		return;
	ASSERTX( pUnit, "Need attacker" );
	
	if (IS_SERVER(pUnit))
	{
		s_StateClear( pUnit, UnitGetId( UnitGetUltimateOwner( pUnit ) ), STATE_ATTACKING, 0, INVALID_ID );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		// this goes all crazy when the server sends the stat changes in conjunction!
		//c_StateClear( pUnit, UnitGetId( UnitGetUltimateOwner( pUnit ) ), STATE_ATTACKING, 0 );
#endif
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

STATS * StateSetAttackStart( UNIT *pAttacker,
							 UNIT *pTarget,
							 UNIT *pWeapon,
							 BOOL bMelee )
{
	if( AppIsHellgate() )
		return NULL;
	if( pAttacker == NULL )
	{
		if( pWeapon != NULL )
		{
			pAttacker = UnitGetOwnerUnit( pWeapon );
		}
	}
	ASSERTX_RETNULL( pAttacker, "Need Attacker" );
	if( pTarget == NULL )
	{
		pTarget = pAttacker;
	}
	GAME *pGame = UnitGetGame( pAttacker );
	STATS *stats( NULL );
	if( pWeapon == NULL )
	{
		if( UnitGetGenus( pAttacker ) == GENUS_PLAYER  ||
			UnitGetGenus( pAttacker ) == GENUS_MONSTER )
			return NULL; //state can only be set on attacker weapon or missile
		pWeapon = pAttacker; //attacker is a weapon or missile
		pAttacker = UnitGetUltimateOwner( pWeapon ); //now get monster or player
	}

	//clear the states if need be on Items
	if( UnitGetGenus( pAttacker ) == GENUS_ITEM )
	{
		//Why do this? Well some weapons shoot a projectile. If that is the case, the stats to transfer don't matter
		StateClearAttackState( pAttacker );
	}		
	StateClearAttackState( pWeapon ); //clear the state to make sure it doesn't stack the stats
	//Ultimate owner
	UNIT *pUltimateOwner = UnitGetUltimateOwner( pAttacker );

	//must not set state twice on weapon.
	ASSERTX( !UnitHasState( pGame, pWeapon, STATE_ATTACKING ), "Trying to set state twice. Can't do that.");	
	
	if( IS_SERVER( pGame ) )
	{		
		stats = s_StateSet( pWeapon, pUltimateOwner, STATE_ATTACKING, 0 );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		// this goes all crazy when the server sends the stat changes in conjunction!
		//stats = c_StateSet( pWeapon, pUltimateOwner, STATE_ATTACKING, 0, 0, -1 );
#endif
	}
	if( stats )
	{
		
		StateRunScriptsOnUnitStates( pWeapon, 
									 pTarget,
									 (bMelee)?STATE_SCRIPT_FLAG_ATTACK_MELEE:STATE_SCRIPT_FLAG_ATTACK_RANGED,
									 stats );
	}
	return stats;
}


//----------------------------------------------------------------------------
struct STATE_EVENT_TYPE_HANDLER_LOOKUP
{
	const char *pszName;
	PFN_STATE_EVENT_HANDLER pfnHandler;
};

//----------------------------------------------------------------------------
static STATE_EVENT_TYPE_HANDLER_LOOKUP sgtStateEventHandlerLookupTable[] = 
{
	{ "AddAttachment",				sHandleEventAddAttachment },
	{ "AddAttachmentBBox",			sHandleEventAddAttachmentBBox },
	{ "RemoveAttachment",			sHandleEventRemoveAttachment },
	{ "AddAttachmentToAim",			sHandleEventAddAttachmentToAim },
	{ "AddAttachmentToHeight",		sHandleEventAddAttachmentToHeight },
	{ "EnableDrawGroup",			sHandleEventEnableDrawGroup },
	{ "DisableDrawGroup",			sHandleEventDisableDrawGroup },
	{ "AddAttachmentToRandom",		sHandleEventAddAttachmentToRandom },
	{ "Spin",						sHandleEventSpin },
	{ "SpinClear",					sHandleEventSpinClear },
	{ "StartArcLightning",			sHandleEventStartArcLightning },
	{ "ArcLightningClear",			sHandleEventArcLightningClear },
	{ "ModifyAI",					sHandleEventModifyAI },
	{ "ModifyAIClear",				sHandleEventModifyAIClear },	
	{ "SetTargetToSource",			sHandleEventSetTargetToSource },
	{ "ClearTargetToSource",		sHandleEventClearTargetToSource },
	{ "SetMode",					sHandleEventSetMode },
	{ "SetModeClear",				sHandleEventSetModeClear },
	{ "SetModeClientDuration",		sHandleEventSetModeClientDuration },
	{ "LevelUp",					sHandleEventLevelUp },
	{ "RankUp",						sHandleEventRankUp },
	{ "DisableDraw",				sHandleEventDisableDraw },
	{ "DisableDrawClear",			sHandleEventDisableDrawClear },
	{ "DontDrawWeapons",			sHandleEventDontDrawWeapons },
	{ "SetOpacity",					sHandleEventSetOpacity },
	{ "ClearOpacity",				sHandleEventClearOpacity },
	{ "DontTarget",					sHandleEventDontTarget },
	{ "DontTargetClear",			sHandleEventDontTargetClear },
	{ "DontAttack",					sHandleEventDontAttack },
	{ "DontAttackClear",			sHandleEventDontAttackClear },
	{ "HolyRadius",					sHandleEventHolyRadius },
	{ "HolyRadiusClear",			sHandleEventHolyRadiusClear },
	{ "AddAttachmentShrinkBone",	sHandleEventAddAttachemntShrinkBone },
	{ "FireMissileGibs",			sHandleEventFireMissileGibs },
	{ "CreatePaperdollAppearance",	sHandleEventCreatePaperdollAppearance },
	{ "DestroyPaperdollAppearance",	sHandleEventDestroyPaperdollAppearance },
	{ "DrawLoadingScreen",			sHandleEventDrawLoadingScreen },
	{ "ClearLoadingScreen",			sHandleEventClearLoadingScreen },
	{ "SetMaterialOverride",		sHandleEventSetMaterialOverride },
	{ "ClearMaterialOverride",		sHandleEventClearMaterialOverride },
	{ "SetModelEnvironmentOverride",	sHandleEventSetModelEnvironmentOverride },
	{ "ClearModelEnvironmentOverride",	sHandleEventClearModelEnvironmentOverride },
	{ "SetGlobalEnvironmentOverride",	sHandleEventSetGlobalEnvironmentOverride },
	{ "ClearGlobalEnvironmentOverride",	sHandleEventClearGlobalEnvironmentOverride },
	{ "AddAttachmentBoneWeights",	sHandleEventAddAttachmentBoneWeights },
	{ "SetScaleToSize",				sHandleEventSetScaleToSize },
	{ "ClearSetScaleToSize",		sHandleEventClearSetScaleToSize },
	{ "SetHeight",					sHandleEventSetHeight },
	{ "ClearHeight",				sHandleEventClearHeight },
	{ "SetWeight",					sHandleEventSetWeight },
	{ "ClearWeight",				sHandleEventClearWeight },
	{ "SkillStart",					sHandleEventSkillStart },
	{ "SkillStop",					sHandleEventSkillStop },
	{ "RagdollStartPulling",		sHandleEventRagdollStartPulling },
	{ "RagdollStopPulling",			sHandleEventRagdollStopPulling },
	{ "RagdollStartCollapsing",		sHandleEventRagdollStartCollapsing },
	{ "RagdollStopCollapsing",		sHandleEventRagdollStopCollapsing },
	{ "SetStateOnStat",				sHandleEventSetStateOnStat },
	{ "StateClearAllOfType",		sHandleEventStateClearAllOfType },
	{ "ClearSetStateOnStat",		sHandleEventClearSetStateOnStat },
	{ "StateSet",					sHandleEventStateSet },	
	{ "StateSetOnClient",			sHandleEventStateSetOnClient },	
	{ "SetStateOnPets",				sHandleSetStateOnPets },	
	{ "ClearStateOnPets",			sHandleClearStateOnPets },	
	{ "FullyHeal",					sHandleEventFullyHeal },
	{ "DontInterrupt",				sHandleEventDontInterrupt },
	{ "DontInterruptClear",			sHandleEventDontInterruptClear },
	{ "ShareWardrobe",				sHandleEventShareWardrobe },
	{ "ShareWardrobeClear",			sHandleEventShareWardrobeClear },
	{ "SetOverrideAppearance",		sHandleEventSetOverrideAppearance },
	{ "ClearOverrideAppearance",	sHandleEventClearOverrideAppearance },
	{ "SetOverrideAppearanceObject",sHandleEventSetOverrideAppearanceObject },
	{ "ClearOverrideAppearanceObject",sHandleEventClearOverrideAppearanceObject },
	{ "SetScreenEffect",			sHandleEventScreenEffect },
	{ "ClearScreenEffect",			sHandleEventScreenEffectClear },
	{ "UICinematicMode",			sHandleEventUICinematicMode },
	{ "UICinematicModeClear",		sHandleEventUICinematicModeClear },
	{ "PreventAllSkills",			sHandleEventPreventAllSkills },
	{ "PreventAllSkillsClear",		sHandleEventPreventAllSkillsClear },
	{ "SetCharacterLighting",		sHandleEventCharacterLightingSet },
	{ "ClearCharacterLighting",		sHandleEventCharacterLightingClear },
	{ "RemoveUnit",					sHandleEventRemoveUnit },
	{ "RemoveUnitIndestructible",	sHandleEventRemoveUnitIndestructible },
	{ "RemoveUnitWithDelay",		sHandleEventRemoveUnitAfterDelay },
	{ "ExecuteSkillOnEvent",		sHandleEventExecuteSkillOnEvent },
	{ "ExecuteSourceSkillOnEvent",	sHandleEventExecuteSourceSkillOnEvent },
	{ "ClearExecuteSkillOnEvent",	sHandleEventClearExecuteSkillOnEvent },
	{ "AwardAttackXPOnUnitDeath",	sHandleEventAwardAttackerXPOnUnitDeath },
	{ "FireVisualHomingMissile",	sHandleEventFireVisualHomingMissile },
	{ "AwardAttackLuckOnUnitDeath",	sHandleEventAwardAttackerLuckOnUnitDeath },
	{ "RemoveMissiles",				sHandleRemoveMissiles },
	{ "SetHotkey",					sHandleSetHotkey },
	{ "SetMouseSkills",				sHandleSetMouseSkills },
	{ "RestoreMouseSkills",			sHandleRestoreMouseSkills },
	{ "HitFlashUnit",				sHandleEventHitFlashUnit },
	{ "CooldownSkill",				sHandleEventCooldownSkill },
	{ "CooldownOwnerSkillOnEvent",	sSetEventHandler_CooldownOwnerSkillOnEvent },
	{ "ClearCooldownOwnerSkillOnEvent",	sClearEventHandler_CooldownOwnerSkillOnEvent },
	{ "SwitchTeamToSourceUnit",		sHandleSwitchTeamToSourceUnit },
	{ "RemoveTeamSetBySourceUnit",	sHandleRemoveTeamSetBySourceUnit },
	{ "SwitchTeamToTempPlayerTeam",	sHandleSwitchTeamToTempPlayerTeam },
	{ "RemoveTempPlayerTeam",		sHandleRemoveTempPlayerTeam },
	{ "ChangeCameraUnit",			sHandleChangeCameraUnit },
	{ "ClearCameraUnit",			sHandleClearCameraUnit },
	{ "FirstPersonCamera",			sHandleFirstPersonCamera },
	{ "ThirdPersonCamera",			sHandleThirdPersonCamera },
	{ "RestoreCamera",				sHandleRestoreCamera },
	{ "ShowExtraHotkeys",			sHandleShowExtraHotkeys },
	{ "HideExtraHotkeys",			sHandleHideExtraHotkeys },
	{ "ClearExtraHotkeys",			sHandleClearExtraHotkeys },
	{ "SetFirstPersonFOV",			sHandleSetFirstPersonFOV },
	{ "SetThirdPersonFOV",			sHandleSetThirdPersonFOV },
	{ "RestoreFOV",					sHandleRestoreFOV },
	{ "Message",					sMessage },
	{ "SetBGSoundOverride",			sHandleEventSetBGSoundOverride },
	{ "ClearBGSoundOverride",		sHandleEventClearBGSoundOverride },
	{ "SetStat",					sHandleEventSetStat },
	{ "IncStat",					sHandleEventIncStat },
	{ "DecStat",					sHandleEventDecStat },
	{ "StateClearOnEvent",			sHandleEventStateClearOnEvent },
	{ "ClearStateClearOnEvent",		sHandleEventClearStateClearOnEvent },
	{ "WardrobeAddLayer",			sHandleEventWardrobeAddLayer },
	{ "WardrobeRemoveLayer",		sHandleEventWardrobeRemoveLayer },
	{ "SetSoundMixState",			sHandleEventSetSoundMixState },
	{ "ClearSoundMixState",			sHandleEventClearSoundMixState },
	{ "ClearPlayerMovement",		sHandleEventClearPlayerMovement },
	{ "UpdateAnimationContext",		sHandleEventUpdateAnimationContext },
	{ "AnimationGroupSet",			sHandleEventAnimationGroupSet },
	{ "AnimationGroupClear",		sHandleEventAnimationGroupClear },
	{ "Quest_RTS_Start",			sHandleQuest_RTS_Start },
	{ "Quest_RTS_End",				sHandleQuest_RTS_End },
	{ "Inform Quest Set",			sHandleInformQuestSet },
	{ "Inform Quest Clear",			sHandleInformQuestClear },
	{ "Quest States Update",		sHandleQuestStatesUpdate },
	{ "DisableSound",				sHandleEventDisableSound },
	{ "DisableSoundClear",			sHandleEventDisableSoundClear },
	{ "DisableQuests",				sHandleEventDisableQuests },
	{ "DisableQuestsClear",			sHandleEventDisableQuestsClear },
	{ "SourceTakeResponsibility",	sHandleEventSourceTakeResponsibility },
	{ "SourceTakeResponsibilityClear",sHandleEventSourceTakeResponsibilityClear },
	{ "UIComponentActivate",		sHandleEventUIComponentActivate },
	{ "UIComponentDeactivate",		sHandleEventUIComponentDeactivate },
	{ "UIComponentShow",			sHandleEventUIComponentShow }, 
	{ "UIComponentHide",			sHandleEventUIComponentHide },
	{ "WardrobePrimaryReplacesSkin",sHandleEventWardrobePrimaryReplacesSkin },
	{ "WardrobePrimaryReplacesSkinClear",sHandleEventWardrobePrimaryReplacesSkinClear },
	{ "WardrobeSetColorsetOverride",sHandleEventWardrobeSetColorsetOverride },
	{ "WardrobeClearColorsetOverride",sHandleEventWardrobeClearColorsetOverride },
	{ "PlayTriggerSound",			sHandleEventPlayTriggerSound },

	{ NULL,	NULL }		// please keep this last
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_STATE_EVENT_HANDLER sLookupStateEventHandler( 
	const char *pszEventHandler)
{
	for (unsigned int ii = 0; sgtStateEventHandlerLookupTable[ii].pfnHandler != NULL; ++ii)
	{
		const STATE_EVENT_TYPE_HANDLER_LOOKUP *pLookup = &sgtStateEventHandlerLookupTable[ii];
		if (PStrICmp(pLookup->pszName, pszEventHandler) == 0)
		{
			return pLookup->pfnHandler;
		}
	}

	ASSERTV_RETNULL(0, "Unrecognized set state event handler '%s'", pszEventHandler);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelStateEventTypePostProcess(
	struct EXCEL_TABLE * table)
{
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		STATE_EVENT_TYPE_DATA * state_event_type_data = (STATE_EVENT_TYPE_DATA *)ExcelGetDataPrivate(table, ii);
				
		// translate set event handler
		const char * pszEventHandler = state_event_type_data->szSetEventHandler;
		if (pszEventHandler[ 0 ] != 0)
		{
			state_event_type_data->pfnSetEventHandler = sLookupStateEventHandler(pszEventHandler);
		}

		// translate clear event handler
		pszEventHandler = state_event_type_data->szClearEventHandler;		
		if (pszEventHandler[ 0 ] != 0)
		{
			state_event_type_data->pfnClearEventHandler = sLookupStateEventHandler(pszEventHandler);			
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_StateLightingDataEngineLoad()
{
#if !ISVERSION(SERVER_VERSION)
	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_STATES, "textures\\" );
	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];

	// go through all rows
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_STATE_LIGHTING );
	for (int i = 0; i < nNumRows; ++i)
	{
		STATE_LIGHTING_DATA *pStateLightingData = (STATE_LIGHTING_DATA *)ExcelGetData( NULL, DATATABLE_STATE_LIGHTING, i );  // cast away const
		if ( pStateLightingData->szSHCubemap[ 0 ] )
		{
			// NOTE: currently forcing sync load on these since they happen at start-of-day
			PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, pStateLightingData->szSHCubemap );
			V( e_CubeTextureNewFromFile( &pStateLightingData->nSHCubemap, szFilePath, TEXTURE_GROUP_UNITS, TRUE ) );
			e_CubeTextureAddRef( pStateLightingData->nSHCubemap );
			ASSERT_CONTINUE( pStateLightingData->nSHCubemap != INVALID_ID );

			if ( ! e_IsNoRender() )
			{
				V( e_GetSHCoefsFromCubeMap( pStateLightingData->nSHCubemap, pStateLightingData->tBaseSHCoefs_O2, pStateLightingData->tBaseSHCoefs_O3, FALSE ) );
				V( e_GetSHCoefsFromCubeMap( pStateLightingData->nSHCubemap, pStateLightingData->tBaseSHCoefsLin_O2, pStateLightingData->tBaseSHCoefsLin_O3, TRUE ) );
			}

			e_CubeTextureReleaseRef( pStateLightingData->nSHCubemap );
			pStateLightingData->nSHCubemap = INVALID_ID;
		}
	}
#endif	//	!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StateIsBad(
	int nState)
{
	const STATE_DATA *pStateData = StateGetData( NULL, nState );
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if( !pStateData )
	{
		return FALSE;
	}
	return StateDataTestFlag( pStateData, SDF_IS_BAD );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_StateClearBadStates( 
	UNIT *pUnit)
{
	BOOL bClearedStates = FALSE;

	int nNumStates = ExcelGetNumRows( NULL, DATATABLE_STATES );
	for (int nState = 0; nState < nNumStates; ++nState)
	{
		if (StateIsBad( nState ))
		{
			if (StateClearAllOfType( pUnit, nState ) == TRUE)
			{
				bClearedStates = TRUE;
			}
		}
	}
		
	return bClearedStates;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_StateEventIsHolyRadius(
	const STATE_EVENT_TYPE_DATA *pStateEventTypeData)
{
	return pStateEventTypeData->pfnSetEventHandler == sHandleEventHolyRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateArrayEnable(
	UNIT *pUnit,
	const int *pnStateArray,
	int nArraySize,
	int nDuration,
	BOOL bIsClientOnly)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pnStateArray , "Expected state array" );
	ASSERTX_RETURN( nArraySize > 0, "Invalid state array entry count" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// go through each state array entry
	for ( int i = 0; i < nArraySize; i++ )
	{
		
		int nState = pnStateArray[ i ];
		if (nState != INVALID_LINK)
		{
			const STATE_DATA *pStateData = StateGetData( NULL, nState );
			if (pStateData)
			{
				if (IS_SERVER( pGame ))
				{
					s_StateSet( pUnit, pUnit, nState, nDuration );
				}
				else if (bIsClientOnly || StateDataTestFlag( pStateData, SDF_CLIENT_ONLY ))
				{
					// only set state for client only objects because the server will not
					// be informing clients about the state of course.
					c_StateSet( pUnit, pUnit, nState, 0, nDuration, INVALID_ID );
				}
			}
						
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME_VARIANT_TYPE StateGetGameVariantType(
	GAME *pGame,
	int nState)
{
	ASSERTV_RETVAL( nState != INVALID_LINK, GV_INVALID, "Expected state link" );
	
	// get state data
	const STATE_DATA *pStateData = StateGetData( pGame, nState );
	if (pStateData)
	{
		return pStateData->eGameVariant;
	}
	
	return GV_INVALID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateArrayGetGameVariantFlags( 
	int *pStates,
	int nMaxStates,
	DWORD *pdwGameVariantFlags)
{
	ASSERTV_RETURN( pStates, "Expected state array" );
	ASSERTV_RETURN( pdwGameVariantFlags, "Expected game variant flags" );
	
	// go through array
	for (int i = 0; i < nMaxStates; ++i)
	{
		int nState = pStates[ i ];
		if (nState != INVALID_LINK)
		{
			GAME_VARIANT_TYPE eType = StateGetGameVariantType( NULL, nState );
			if (eType != GV_INVALID)
			{
				SETBIT( *pdwGameVariantFlags, eType );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StateIterateSaved(
	UNIT *pUnit,
	FN_SAVE_STATE_ITERATOR *pfnCallback,
	void *pCallbackData)
{
	ASSERTV_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( pfnCallback, "Expected callback" );

	for (STATS *pStats = StatsGetModList( pUnit, SELECTOR_STATE ); 
		 pStats; 
		 pStats = StatsGetModNext( pStats, SELECTOR_STATE ) )
	{
		PARAM dwState = 0;
		if ( ! StatsGetStatAny(pStats, STATS_STATE, &dwState ) )
			continue;

		if ( dwState == INVALID_PARAM )
			continue;

		const STATE_DATA * pStateData = StateGetData( NULL, dwState );
		if ( ! pStateData )
			continue;

		if ( StateDataTestFlag( pStateData, SDF_SAVE_WITH_UNIT ))
		{
			pfnCallback( pUnit, dwState, pCallbackData );
		}
			
	}
	
}
