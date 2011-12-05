//----------------------------------------------------------------------------
// imports.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "imports.h"
#include "random.h"
#include "game.h"
#include "unit_priv.h"
#include "picker.h"
#include "skills.h"
#include "inventory.h"
#include "script.h"
#include "script_priv.h"
#include "fontcolor.h"
#include "uix.h"
#include "uix_components.h"
#include "items.h"
#include "monsters.h"
#include "unittag.h"
#include "level.h"
#include "c_tasks.h"
#include "states.h"
#include "player.h"
#include "uix_priv.h"
#include "wardrobe.h"
#include "gameunits.h"
#include "ai.h"
#include "accountbadges.h"
#include "combat.h"
#include "global_themes.h"
#include "Economy.h"
#include "Currency.h"
#include "s_townportal.h"
#include "skills_priv.h"
#include "missiles.h"
#include "pets.h"
#include "achievements.h"
#include "globalindex.h"
#include "gameglobals.h"
#include "s_recipe.h"
#include "crafting_properties.h"
#include "recipe.h"
#include "gamemail.h"

#define MAKE_COLOR_CODE(use, clr)		((BOOL)use << 16 | (BYTE)clr)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMABS(
	int a)
{
	return abs(a);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMin(
	int a,
	int b)
{
	return (min(a, b));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMax(
	int a,
	int b)
{
	return (max(a, b));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPin(
	int value,
	int a,
	int b)
{
	return PIN(value, a, b);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPercent(
	int a,
	int b)
{
	return PCT(a, b);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPercentFloat(
	int a,
	int b,
	int c )
{
	float p = ((float)b/(float)c)/100.0f;
	return (int)( (float)a * p ); 
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPercentOfHundred(
	SCRIPT_CONTEXT * pContext,
	int a,
	int b,
	int c )
{
	float p = (float)a/(float)b;
	return (int)FLOOR( p * (float)c );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRandSkillSeed(
	SCRIPT_CONTEXT * pContext,
	int a,
	int b)
{
	ASSERT_RETVAL(pContext, a);
	ASSERT_RETVAL(pContext->nSkill!=INVALID_ID, a );
	if (b <= a)
	{
		return a;
	}
	RAND tRand;
	SkillInitRand( tRand, pContext->game, UnitGetUltimateOwner( pContext->unit ), NULL, pContext->nSkill );		
	return RandGetNum( tRand, a, b);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRand(
	GAME * game,
	int a,
	int b)
{
	ASSERT_RETVAL(game, a);
	if (b < a)
	{
		return RandGetNum(game, b, a);
	}
	else
	{
		return RandGetNum(game, a, b);
	}
}

int VMRandByUnitId(
	SCRIPT_CONTEXT * pContext,
	int a,
	int b)
{
	ASSERT_RETVAL(pContext, a);
	ASSERT_RETVAL(pContext->unit, a);
	if (b <= a)
	{
		return a;
	}
	RAND tRand;
	DWORD dwSeedCurr = (DWORD)UnitGetId(pContext->unit);
	RandInit(tRand, dwSeedCurr);
				
	return RandGetNum( tRand, a, b);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMChance(
	GAME * game,
	int nChance,
	int nChanceOutOf)
{
	ASSERT_RETZERO(game);
	return ((int)RandGetNum(game, 0, nChanceOutOf) <= nChance)?1:0; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMChanceByStateMod(
	SCRIPT_CONTEXT * pContext,
	int nChance,
	int nChanceOutOf,
	int nState,
	int nModBy )
{
	ASSERT_RETZERO(nState != INVALID_ID);
	ASSERT_RETZERO(pContext->unit);
	nChance += PCT( nChance, nModBy );
	return ( (int)RandGetNum(pContext->game, 0, nChanceOutOf) <= nChance )?1:0; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRoll(
	GAME * game,
	int a,
	int b)
{
	ASSERT_RETZERO(game);
	if (b < 0)
	{
		return 0;
	}
	int result = a;
	for (int ii = 0; ii < a; ii++)
	{
		result += RandGetNum(game, b);
	}
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDistribute(
	GAME * game,
	int numdie,
	int diesize,
	int start)
{
	ASSERT_RETZERO(game);

	int roll = VMRoll(game, numdie, diesize);
	int min = numdie;
	return roll - (min + numdie * diesize) / 2 + start;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRoundStat(
	GAME * game,
	int stat,
	int value)
{
	return StatsRoundStat(game, stat, value) >> StatsGetShift(game, stat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillGetLevel_Object(
	SCRIPT_CONTEXT * pContext,
	int nSkill )
{
	return SkillGetLevel( pContext->object, nSkill );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetEventIDByName(
	SCRIPT_CONTEXT * pContext,
	int nEventID )
{
	ASSERT_RETZERO( nEventID != INVALID_ID );
	return nEventID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetAffixIDByName(
	SCRIPT_CONTEXT * pContext,
	int nAffixID )
{
	ASSERT_RETZERO( nAffixID != INVALID_ID );
	return nAffixID;
}
					   
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillGetLevel(
	UNIT * pUnit,
	int nSkill )
{
	return SkillGetLevel( pUnit, nSkill );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPickSkill(
	UNIT * unit,
	STATS * stats,
	int nUnitType,
	int nSkillGroup,
	int nSkillLevel )
{
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	int ilvl = UnitGetExperienceLevel(unit);

	PickerStart(game, picker);
	int skill_count = ExcelGetNumRows(game, DATATABLE_SKILLS);
	for (int ii = 0; ii < skill_count; ii++)
	{
		const SKILL_DATA* skill_data = SkillGetData(game, ii);
		if ( ! skill_data )
			continue;
		if (!SkillDataTestFlag(skill_data, SKILL_FLAG_LEARNABLE))
		{
			continue;
		}
		if (! UnitIsA( skill_data->nRequiredUnittype, nUnitType ) )
		{
			continue;
		}
		if ( nSkillGroup != INVALID_ID )
		{
			BOOL bIsInSkillGroup = FALSE;
			for ( int n = 0; n < MAX_SKILL_GROUPS_PER_SKILL; n++ )
			{
				if ( skill_data->pnSkillGroup[ n ] == nSkillGroup )
					bIsInSkillGroup = TRUE;
			}
			if ( ! bIsInSkillGroup )
				continue;
		}

		if ( skill_data->pnSkillLevelToMinLevel[ 0 ] <= ilvl )
		{
			PickerAdd(MEMORY_FUNC_FILELINE(game, ii, 1));
		}
	}

	int skill = PickerChoose(game);
	if (skill < 0)
	{
		return 0;
	}

	nSkillLevel = MAX(1, nSkillLevel);

	if (stats)
	{
		StatsSetStat(game, stats, STATS_SKILL_LEVEL, skill, nSkillLevel);
	}
	else if (unit)
	{
		SkillUnitSetInitialStats(unit, skill, nSkillLevel);
		//UnitSetStat(unit, stat, skill, level);
	}


	return skill;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPickSkill(
	UNIT * unit,
	STATS * stats,
	int nSkillLevel )
{
	return sPickSkill( unit, stats, UNITTYPE_ANY, INVALID_ID, nSkillLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPickSkillByUnitType(
	UNIT * unit,
	STATS * stats,
	int nUnitType,
	int nSkillLevel )
{
	return sPickSkill( unit, stats, nUnitType, INVALID_ID, nSkillLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPickSkillBySkillGroup(
	UNIT * unit,
	STATS * stats,
	int nSkillGroup,
	int nSkillLevel )
{
	return sPickSkill( unit, stats, UNITTYPE_ANY, nSkillGroup, nSkillLevel );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatParent(
	UNIT * unit,
	int stat )
{
	if( stat == INVALID_LINK ||
		unit == NULL )
		return 0;
	UNIT *pOwner = UnitGetUltimateOwner( unit );
	if( pOwner == NULL )
		return 0;
	return UnitGetStat( pOwner, stat );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatFromOwner(
	UNIT * unit,
	int stat )
{
	if( stat == INVALID_LINK ||
		unit == NULL )
		return 0;
	UNIT *pOwner = UnitGetUltimateOwner( unit );
	if( pOwner == NULL )
		return 0;
	int statVal = UnitGetStat( pOwner, stat ) >> StatsGetShift( UnitGetGame( unit ), stat );
	return statVal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatFromOwnerDivBySkillVar(
	SCRIPT_CONTEXT * pContext,
	int stat,
	int nVariable)
{
	ASSERTX_RETZERO( pContext->nSkill != INVALID_ID, "Skill was not passed into script. Unable to execute script function getVar." );
	if( stat == INVALID_LINK ||
		pContext->unit == NULL ||		
		nVariable < 0 ) 
		return -1;	
	UNIT *pOwner = UnitGetUltimateOwner( pContext->unit );
	if( pOwner == NULL )
		return -1;
	int skillVarValue = VMGetSkillVariableFromSkill( pContext, pContext->nSkill, (SKILL_VARIABLES)nVariable );
	if( skillVarValue != 0 )
	{
		skillVarValue = VMGetStatFromOwnerDivBy( pOwner, stat, skillVarValue );
	}
	return skillVarValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatFromOwnerDivBy(
	UNIT * unit,
	int stat,
	int nDivBy )
{
	if( nDivBy == 0 )
		return 0;
	float flStateValue = (float)VMGetStatFromOwner( unit, stat );
	return (int)FLOOR( (flStateValue/(float)nDivBy) + .5f );
}

//----------------------------------------------------------------------------
// this is dumb
//----------------------------------------------------------------------------
int VMSwitchUnitAndObject(
	SCRIPT_CONTEXT * pContext )
{
	UNIT *pObject = pContext->object;
	pContext->object = pContext->unit;
	pContext->unit = pObject;
	return 1;
}

int VMGetAchievementCompleteCount(
	SCRIPT_CONTEXT * pContext,
	int nAchievementID )
{
	ASSERT_RETZERO( nAchievementID != INVALID_ID );	
	const ACHIEVEMENT_DATA * pAchievementData = (ACHIEVEMENT_DATA *)ExcelGetData(NULL, DATATABLE_ACHIEVEMENTS, nAchievementID );//skill_data->nAchievementLink );
	ASSERT_RETZERO( pAchievementData );	
	return pAchievementData->nCompleteNumber;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillVariable(
	SCRIPT_CONTEXT * pContext,	
	int nVariable )
{
	ASSERTX_RETZERO( pContext->nSkill != INVALID_ID, "Skill was not passed into script. Unable to execute script function getVar." );
	if( !pContext->unit ||	
		( nVariable < SKILL_VARIABLE_ONE || nVariable >= SKILL_VARIABLE_COUNT ) )
		return 0;
	return SkillGetVariable( UnitGetUltimateOwner( pContext->unit ), pContext->object, pContext->nSkill, (SKILL_VARIABLES)nVariable, pContext->nSkillLevel ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetVarRange(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->nSkill != INVALID_ID );
	ASSERT_RETZERO( pContext->nSkillLevel != INVALID_ID );
	const SKILL_DATA *pSkillData = (SKILL_DATA *)ExcelGetData( pContext->game, DATATABLE_SKILLS, pContext->nSkill );
	ASSERT_RETZERO( pSkillData);
	int code_len( 0 );	
	BYTE * code_ptr = ExcelGetScriptCode( pContext->game, DATATABLE_SKILLS, pSkillData->codeSkillRangeScript, &code_len);		
	if (code_ptr)
	{				
		return VMExecI( pContext->game, pContext->unit, NULL, NULL, pContext->nSkill, pContext->nSkillLevel, pContext->nSkill, pContext->nSkillLevel, INVALID_ID, code_ptr, code_len);		
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetAttackerSkillVarBySkill(
	SCRIPT_CONTEXT * pContext,	
	int nSkill,
	int nVariable )
{
	ASSERTX_RETZERO( pContext->unit, "No unit passed in." );
	int attackerID = UnitGetStat(pContext->unit, STATS_AI_LAST_ATTACKER_ID );	
	ASSERTX_RETZERO( attackerID != INVALID_ID, "Unit has no attacker." );
	UNIT *pUnit = UnitGetById( pContext->game, attackerID );	
	if( !pUnit ||
		nSkill == INVALID_ID ||
		( nVariable < SKILL_VARIABLE_ONE || nVariable >= SKILL_VARIABLE_COUNT ) )
		return 0; //attacker could of died and left the instance.....
	return SkillGetVariable( UnitGetUltimateOwner( pUnit ), pContext->object, nSkill, (SKILL_VARIABLES)nVariable, pContext->nSkillLevel ); 
}

int VMGetSkillVariableFromSkill(
	SCRIPT_CONTEXT * pContext,
	int nSkill,
	int nVariable )
{
	if( !pContext->unit ||
		nSkill == INVALID_ID ||
		( nVariable < SKILL_VARIABLE_ONE || nVariable >= SKILL_VARIABLE_COUNT ) )
		return 0;
	return SkillGetVariable( UnitGetUltimateOwner( pContext->unit ), pContext->object, nSkill, (SKILL_VARIABLES)nVariable ); 
}

int VMGetSkillVariableFromSkillWithLvl(
								SCRIPT_CONTEXT * pContext,
								int nSkill,
								int nVariable,
								int nLevel )
{
	if( !pContext->unit ||
		nSkill == INVALID_ID ||
		( nVariable < SKILL_VARIABLE_ONE || nVariable >= SKILL_VARIABLE_COUNT ) )
		return 0;
	return SkillGetVariable( UnitGetUltimateOwner( pContext->unit ), pContext->object, nSkill, (SKILL_VARIABLES)nVariable, nLevel ); 
}



int VMGetSkillVariableFromSkillFromObject(
	SCRIPT_CONTEXT * pContext,
	int nSkill,
	int nVariable )
{
	if( !pContext->object ||
		nSkill == INVALID_ID ||
		( nVariable < SKILL_VARIABLE_ONE || nVariable >= SKILL_VARIABLE_COUNT ) )
		return 0;
	return SkillGetVariable( UnitGetUltimateOwner( pContext->object ), pContext->unit, nSkill, (SKILL_VARIABLES)nVariable, pContext->nSkillLevel ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasState(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	GAME * game = UnitGetGame(pContext->unit);
	ASSERT_RETZERO(game);	
	//if( IS_CLIENT( game ) )
	//	return 0;
	return UnitHasState( game, pContext->unit, nState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasStateObject(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->object );
	GAME * game = UnitGetGame(pContext->object);
	ASSERT_RETZERO(game);	
	//if( IS_CLIENT( game ) )
	//	return 0;
	return UnitHasState( game, pContext->object, nState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMClearState(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	GAME * game = UnitGetGame(pContext->unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	StateClearAllOfType( pContext->unit,nState );

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMClearStateClient(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	GAME * game = UnitGetGame(pContext->unit);
	ASSERT_RETZERO(game);	
	if( !IS_CLIENT( game ) )
		return 0;
	StateClearAllOfType( pContext->unit,nState );

	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMClearStateObject(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->object );
	GAME * game = UnitGetGame(pContext->object);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	StateClearAllOfType( pContext->object,nState );

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsDualWielding(
	SCRIPT_CONTEXT * pContext )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	return ( UnitIsDualWielding( pContext->unit, pContext->nSkill )?1:0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetWieldingIsACount(
	SCRIPT_CONTEXT * pContext,
	int unittype )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->nSkill != INVALID_ID );
	ASSERT_RETZERO( unittype != INVALID_ID );
	DWORD flags( 0 );
	int Weapons( 0 );
	SETBIT( flags, IIF_EQUIP_SLOTS_ONLY_BIT );
	UNIT *pUnit = pContext->unit;
	int count( 0 );
	while( UnitGetGenus( pUnit ) != GENUS_MISSILE &&
		   UnitGetGenus( pUnit ) != GENUS_PLAYER  &&
		   count < 2 )
	{
		pUnit = UnitGetOwnerUnit( pUnit );
		count++;
	}
	if( count > 2 )
		return 0;
	UNIT * pItemCurr = UnitInventoryIterate( pUnit, NULL, flags);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pUnit, pItemCurr, flags );
		if( pItemCurr &&
			UnitIsA( pItemCurr, unittype ) )
		{
			Weapons++;
		}
		pItemCurr = pItemNext;
	}

	return Weapons;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetState(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->unit );
	GAME * game = UnitGetGame(pContext->unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
	{
		c_StateSet( pContext->unit, pContext->object, nState, 0, 0, INVALID_ID );
	}
	else
	{
		s_StateSet( pContext->unit, pContext->object, nState, 0, INVALID_ID, NULL );
	}

	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateObject(
	SCRIPT_CONTEXT * pContext,
	int nState )
{	
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO( pContext->object );
	GAME * game = UnitGetGame(pContext->object);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	s_StateSet( pContext->object, pContext->unit, nState, 0, INVALID_ID, NULL );

	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sVMSetStateWithTime(
	UNIT * pUnit,
	int nState,
	int timerMS,
	int nSkill )
{
	ASSERT_RETZERO( pUnit );
	int ticks = (int)( (float)timerMS/(float)MSECS_PER_SEC * GAME_TICKS_PER_SECOND_FLOAT );
	if( IS_CLIENT( pUnit->pGame ) )
	{
		c_StateSet( pUnit, pUnit, nState, 0, ticks, nSkill );
	}
	else
	{
		s_StateSet( pUnit, NULL, nState, ticks, nSkill, NULL );
	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddStateWithTime(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )	
{
	ASSERT_RETZERO(pContext->unit);
	if( !IS_CLIENT( pContext->unit ) )
		return 0;
	return sVMSetStateWithTime(pContext->unit, nState, timerMS, pContext->nSkill);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddStateWithTimeClient(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )	
{
	ASSERT_RETZERO(pContext->unit);
	if( IS_CLIENT( pContext->unit ) )
		return 0;
	return sVMSetStateWithTime(pContext->unit, nState, timerMS, pContext->nSkill);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTime(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )		
{			
	UNIT *unit = pContext->unit;
	return sVMSetStateWithTime(unit, nState, timerMS, pContext->nSkill);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BYTE * GetScriptParam( GAME *pGame,
								   int nParamIndex,
								   const SKILL_DATA *pSkillData,								   
								   int *code_len)
{
	switch(nParamIndex)
	{
	default:
	case 0:
		return ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeEventParam0, code_len);		
	case 1:
		return ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeEventParam1, code_len);		
	case 2:
		return ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeEventParam2, code_len);		
	case 3:
		return ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeEventParam3, code_len);		
	}
}


static int SetStateOnUnitAndExecuteScript( GAME *game,
										   UNIT *unit,
										   UNIT *object,
										   int nSkill,
										   int nState,
										   float timerMS,
										   bool clearFirst,
										   int nParamIndex = INVALID_ID)
{

	int ticks = (int)( (float)timerMS/(float)MSECS_PER_SEC * GAME_TICKS_PER_SECOND_FLOAT );
	if( timerMS <= 0 )
		ticks = 0;
	if( clearFirst )
	{
		StateClearAllOfType( unit, nState );
	//s_StateClear( unit, UnitGetId( unit ), nState, 0, -1 ); 
	}
	STATS *stats = s_StateSet( unit, unit, nState, ticks, -1, NULL );	
	if( stats == NULL ) //this is a work around for a bug
		stats = s_StateSet( unit, unit, nState, ticks, -1, NULL );	
	const SKILL_DATA *pSkillData = SkillGetData( game, nSkill );
	if( pSkillData && stats )
	{	
		int nSkillLevel = (unit)?SkillGetLevel( unit, nSkill ):INVALID_SKILL_LEVEL;
		int code_len = 0;
		BYTE * code_ptr = NULL;
		if( nParamIndex == INVALID_ID ) 
		{
			code_ptr = ExcelGetScriptCode( game, DATATABLE_SKILLS, pSkillData->codeStatsScriptOnTarget, &code_len);		
		}
		else
		{
			code_ptr = GetScriptParam( game, nParamIndex, pSkillData, &code_len );
		}
		VMExecI( game, unit, object, stats, nSkill, INVALID_LINK, nSkill, nSkillLevel, INVALID_LINK, code_ptr, code_len);
	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTimeScriptOnObject(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )		
{			
	UNIT *unit = pContext->object;
	return SetStateOnUnitAndExecuteScript( pContext->game,
											unit,
										   ((pContext->unit)?pContext->unit:unit),
											pContext->nSkill, 
											nState, 
											(float)timerMS, 
											FALSE );
											
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTimeOnObject(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )		
{			
	UNIT *unit = pContext->object;
	return sVMSetStateWithTime(unit, nState, timerMS, pContext->nSkill);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetObjectIsA(
	SCRIPT_CONTEXT * pContext,
	int unittype)
{	
	
	ASSERTX_RETZERO( pContext->object, "Object NULL" );	

	return UnitIsA(pContext->object, unittype);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMissileSourceIsA(
	SCRIPT_CONTEXT * pContext,
	int unittype)
{	
	UNIT *pMissile = ( UnitGetGenus( pContext->unit ) == GENUS_MISSILE )?pContext->unit:pContext->object;
	if( !pMissile ||
		UnitGetGenus( pMissile ) != GENUS_MISSILE )
		return 0; //might not be a missile
	UNIT *pWeapon[ MAX_WEAPONS_PER_UNIT ];	
	UnitGetWeaponsTag( pMissile, TAG_SELECTOR_MISSILE_SOURCE, pWeapon );
	if( pWeapon[ 0 ] == NULL )
		return 0;	
	return UnitIsA(pWeapon[ 0 ], unittype);
}
inline UNIT *GetItemFromContext( SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETNULL( pContext );
	UNIT *pItem( NULL );	
	if( pContext->unit &&
		UnitGetGenus( pContext->unit ) == GENUS_ITEM )
	{
		pItem = pContext->unit;
	}
	else if( pContext->object &&
		UnitGetGenus( pContext->object ) == GENUS_ITEM )
	{
		pItem = pContext->object;
	}
	else if( pContext->nSkill != INVALID_ID )
	{
		pItem = SkillGetTarget( pContext->unit, pContext->nSkill, INVALID_ID );
	}
	
	return pItem;
}

inline const UNIT_DATA *GetItemDataFromContext( SCRIPT_CONTEXT *pContext )
{
	
	if( pContext->unit )
	{
		int nItemClass = UnitGetStat( pContext->unit, STATS_SKILL_EXECUTED_BY_ITEMCLASS );
		const UNIT_DATA *pItem = UnitGetData( pContext->game, GENUS_ITEM, nItemClass );
		if( pItem )
		{
			return pItem;
		}

		UNIT *pParent = UnitGetUltimateOwner( pContext->unit );
		if( pParent != pContext->unit )
		{
			nItemClass = UnitGetStat( pParent, STATS_SKILL_EXECUTED_BY_ITEMCLASS );
			pItem = UnitGetData( pContext->game, GENUS_ITEM, nItemClass );
			if( pItem )
			{
				return pItem;
			}

		}


	}
	UNIT *pUnit = GetItemFromContext( pContext );
	if( pUnit )
	{
		return pUnit->pUnitData;
	}	
	return NULL;
}
int VMGetStatFromExecutedUnit( SCRIPT_CONTEXT * pContext,
							   int nStat)
{		
	UNIT *pUnit = GetItemFromContext( pContext );
	ASSERT_RETFAIL( pUnit );
	return ( UnitGetStat( pUnit, nStat ) >> StatsGetShift( pContext->game, nStat ) );
}


int VMGetItemDuration( SCRIPT_CONTEXT * pContext )
{	
	const UNIT_DATA *pItemData = GetItemDataFromContext( pContext );
	ASSERT_RETZERO( pItemData );
	return pItemData->nDuration;
}

int VMGetItemDurationMS( SCRIPT_CONTEXT * pContext )
{	
	const UNIT_DATA *pItemData = GetItemDataFromContext( pContext );
	ASSERT_RETZERO( pItemData );
	return (pItemData->nDuration * 1000)/20;
}

int VMGetItemCoolDown( SCRIPT_CONTEXT * pContext )
{	
	const UNIT_DATA *pItemData = GetItemDataFromContext( pContext );
	ASSERT_RETZERO( pItemData );
	return pItemData->nCooldown;
}


inline void CopyItemProps( SCRIPT_CONTEXT * pContext,
						   const UNIT_DATA *pItemData,
						   int nIndex )
{
	ASSERT_RETURN( nIndex >= 0 && nIndex < NUM_UNIT_PROPERTIES );
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(pContext->game, DATATABLE_ITEMS, pItemData->codeProperties[nIndex], &code_len);
	VMExecI(pContext->game, pContext->unit, pContext->object, pContext->stats, pContext->nParam1, pContext->nParam2, pContext->nSkill, pContext->nSkillLevel, pContext->nState, code_ptr, code_len);
}
//----------------------------------------------------------------------------
//Executes UnitData props( in excel )
//----------------------------------------------------------------------------
int VMCopyExecutedUnitProps( SCRIPT_CONTEXT * pContext,
							 int nPropIndex )
{	
	ASSERT_RETZERO( pContext->stats );
	const UNIT_DATA *pItemData = GetItemDataFromContext( pContext );
	ASSERT_RETZERO( pItemData );
	if( nPropIndex < 0 )
	{
		for( int t = 0; t < NUM_UNIT_PROPERTIES; t++ )
			CopyItemProps( pContext, pItemData, t );
	}
	else
	{
		CopyItemProps( pContext, pItemData, nPropIndex );
	}
	return 1;
}
//----------------------------------------------------------------------------
//Executes UnitData props( in excel )
//----------------------------------------------------------------------------
int VMItemSkillScript( SCRIPT_CONTEXT * pContext,
					   int nScriptIndex )
{		
	int nItemClass = UnitGetStat( pContext->unit, STATS_SKILL_EXECUTED_BY_ITEMCLASS );	
	ASSERT_RETZERO( nItemClass != INVALID_ID );
	if( nScriptIndex < 0 )
	{
		for( int t = 0; t < NUM_UNIT_SKILL_SCRIPTS; t++ )
			SkillExecuteScript( (SKILL_SCRIPT_ENUM)(SKILL_SCRIPT_ITEM_SKILL_SCRIPT1 + t ),
							    pContext->unit,
								pContext->nSkill,
								pContext->nSkillLevel,
								pContext->object,
								pContext->stats,
								pContext->nParam1,
								pContext->nParam2,
								pContext->nState,
								nItemClass );
	}
	else
	{
		SkillExecuteScript( (SKILL_SCRIPT_ENUM)(SKILL_SCRIPT_ITEM_SKILL_SCRIPT1 + nScriptIndex ),
			UnitGetUltimateOwner( pContext->unit ),
			pContext->nSkill,
			pContext->nSkillLevel,
			pContext->object,
			pContext->stats,
			pContext->nParam1,
			pContext->nParam2,
			pContext->nState,
			nItemClass );
	}
	return 1;
}

//----------------------------------------------------------------------------
//returns the objects class ID
//----------------------------------------------------------------------------
int VMGetObjectClassID( SCRIPT_CONTEXT * pContext )
{	
	ASSERT_RETINVALID( pContext->object );
	return UnitGetClass( pContext->object );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillTargetIsA(
	SCRIPT_CONTEXT * pContext,
	int unittype)
{	
	
	int nSkill = pContext->nSkill;
	UNIT *pTarget = SkillGetTarget( pContext->unit, nSkill, INVALID_ID );	
	//A skill can have a null target
	//ASSERTX_RETZERO( pTarget, "Target on skill is NULL" );	

	if (!pTarget)
	{
		return 0;
	}

	return UnitIsA(pTarget, unittype);
}


inline UNIT * GetSkillTarget( UNIT *pPlayer,
							  int nSkill )
{	
	ASSERT_RETZERO(pPlayer);
	ASSERT_RETZERO(nSkill != INVALID_ID);	
	return SkillGetTarget( pPlayer, nSkill, INVALID_ID );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPossibleTargetCount(
					 SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->nSkill != INVALID_ID );
	const SKILL_DATA *pSkillData = SkillGetData( pContext->game, pContext->nSkill );
	ASSERT_RETZERO( pSkillData );
	float fRangeMax = SkillGetRange( pContext->unit, pContext->nSkill, NULL );
	ASSERT_RETFALSE(fRangeMax != 0.0f);

	// find all nearby targets
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRangeMax;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = pContext->unit;
	SkillTargetQuery( pContext->game, tTargetingQuery );
	return tTargetingQuery.nUnitIdCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasSkillTarget(
	SCRIPT_CONTEXT * pContext )
{
	return ( GetSkillTarget( pContext->unit, pContext->nSkill ) != NULL )?1:0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetAITargetToSkillTarget(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO(pContext->unit);
	ASSERTX_RETZERO(pContext->object, "Must have initial unit with skill call this function. Meaning player must call this on another unit.");
	UNIT *pTarget  = GetSkillTarget( pContext->object, pContext->nSkill );
	if( pTarget == NULL )
		return 0;//no target							
	UnitSetAITarget(pContext->unit, UnitGetId( pTarget ), TRUE);	
	AI_Register(pContext->unit, FALSE, 1);
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetObjectAITargetToUnitAITarget(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO(pContext->unit);
	ASSERTX_RETZERO(pContext->object, "Must have initial unit with skill call this function. Meaning player must call this on another unit.");
	UNIT *pTarget  = UnitGetAITarget( pContext->unit );
	UNIT *pObjectsTarget  = UnitGetAITarget( pContext->object );
	if( pTarget == NULL ||
		pObjectsTarget != NULL )
		return 0;//no target							
	UnitSetAITarget(pContext->object, UnitGetId( pTarget ), TRUE);	
	AI_Register(pContext->object, FALSE, 1);
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetAITargetToObject(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO(pContext->unit);
	ASSERTX_RETZERO(pContext->object, "Must have initial unit with skill call this function. Meaning player must call this on another unit.");
	UnitSetAITarget(pContext->unit, UnitGetId( pContext->object ), TRUE);
	AI_Register(pContext->unit, FALSE, 1);
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMakeAIAwareOfObject(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO(pContext->unit);
	ASSERTX_RETZERO(pContext->object, "Must have initial unit with skill call this function. Meaning player must call this on another unit.");	
	UNIT *pTargetUnit = UnitGetAITarget( pContext->unit );
	if( pContext->unit != pContext->object &&
		( !pTargetUnit ||						//if my target is null or if my target is myself then set the new target
		  pTargetUnit == pContext->unit) ) 
	{
		UnitSetAITarget(pContext->unit, UnitGetId( pContext->object ), TRUE);
		AI_Register(pContext->unit, FALSE, 1);
		return 1;
	}
	return 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMBroadcastEquipEvent(
	SCRIPT_CONTEXT * pContext )
{
	//need a quick unequip and equip
	DWORD flags( 0 );
	SETBIT( flags, IIF_EQUIP_SLOTS_ONLY_BIT );
	UnitTriggerEvent( pContext->game, EVENT_EQUIPED, pContext->unit, pContext->unit, NULL );
	InvBroadcastEventOnUnitsByFlags( pContext->unit, EVENT_EQUIPED, flags ); //broadcast the event)?1:0;
	return 1;
}

//----------------------------------------------------------------------------
//Returns the count of units of a given unittype in an area
//----------------------------------------------------------------------------
int VMGetCountOfUnitTypeInArea( 
	SCRIPT_CONTEXT * pContext,	
	int nAreaSize )
{
	ASSERT_RETZERO(pContext);
	ASSERT_RETZERO(pContext->unit);
	ASSERT_RETZERO(pContext->nSkill != INVALID_ID);
	ASSERT_RETZERO(nAreaSize != 0);
	const SKILL_DATA *pSkillData = SkillGetData( pContext->game, pContext->nSkill );
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.fMaxRange	= (float)nAreaSize;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = pContext->unit;
	SkillTargetQuery( pContext->game, tTargetingQuery );
	return tTargetingQuery.nUnitIdCount;
}

//----------------------------------------------------------------------------
//Get units in area and runs a skill or script on them via targeting flags
//----------------------------------------------------------------------------
static int RunScriptAndSkillOnTargetsInAreaByPCT( 
	SCRIPT_CONTEXT * pContext,
	int nSkillToExecute,
	int nScriptParam,
	int nRadius,
	int nChance,
	int nFlag )
{
	//this is a bit of a hack
	bool bPlayer( nFlag >= 100 );
	if( bPlayer )
		nFlag -= 100;
	//end bit of a hack
	const static int pnSearchBits[][ 2 ] = 
	{ 
		{	-1,		-1	},	//0
		{	SKILL_TARGETING_BIT_TARGET_ENEMY,	-1	},	//1
		{	SKILL_TARGETING_BIT_TARGET_FRIEND,	-1	},	//2
		{	SKILL_TARGETING_BIT_TARGET_PETS,	-1	},	//3
		{	SKILL_TARGETING_BIT_TARGET_FRIEND, SKILL_TARGETING_BIT_TARGET_PETS }, //4
		{	SKILL_TARGETING_BIT_TARGET_FRIEND, SKILL_TARGETING_BIT_ALLOW_SEEKER }, //5
		{   SKILL_TARGETING_BIT_TARGET_FRIEND, SKILL_TARGETING_BIT_TARGET_DEAD } //6
	};
	ASSERT_RETZERO( nFlag >= 0 && nFlag < arrsize( pnSearchBits ) );

	UNIT *unit = pContext->unit;
	ASSERT_RETZERO(unit);	
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	//if( IS_CLIENT( game ) )
	//	return 0;
	int nSkill = pContext->nSkill;	
	ASSERT_RETZERO(nSkill != INVALID_ID);	
	const SKILL_DATA *skilldata = SkillGetData( game, nSkill );
	ASSERT_RETZERO(skilldata);
	//get the script
	int code_len(0);
	BYTE *code_ptr(NULL);
	//if below zero we are just returning count
	if( nScriptParam >= 0 )
	{
		code_ptr = GetScriptParam( pContext->game, nScriptParam, skilldata, &code_len );
		ASSERT_RETZERO(code_ptr);
	}
	int nSkillLevel = SkillGetLevel( pContext->unit, pContext->nSkill );

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY + 1]; //for player
	SKILL_TARGETING_QUERY tTargetingQuery( skilldata );
	
	*tTargetingQuery.tFilter.pdwFlags = 0;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	if ( pnSearchBits[ nFlag ][ 0 ] != -1 )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, pnSearchBits[ nFlag ][ 0 ] );
	}
	if ( pnSearchBits[ nFlag ][ 1 ] != -1 )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, pnSearchBits[ nFlag ][ 1 ] );
	}
	tTargetingQuery.fMaxRange	= (float)nRadius;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery(game, tTargetingQuery );	
	int count( 0 );
	if( bPlayer && pContext->unit )
	{
		pnUnitIds[ tTargetingQuery.nUnitIdCount++ ] = UnitGetId( pContext->unit );
	}
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( game, tTargetingQuery.pnUnitIds[ i ] );
		if ( !pTarget )
			continue;
		
		if( nChance <= 0 ||
			nChance > (int)RandGetNum( game, 1, 99 ) )
		{

			count++;		
			//run the script
			if( code_ptr && code_len)
			{
				VMExecI(pContext->game, pTarget, pContext->unit, NULL, pContext->nSkill, nSkillLevel, pContext->nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);		
			}
			//execute the skill
			if( nSkillToExecute != INVALID_ID )
			{
				DWORD dwFlags = 0;
				if ( IS_SERVER( game ) )
					dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

				SkillStartRequest( game, unit, nSkillToExecute, UnitGetId( pTarget ), UnitGetPosition( pTarget ), dwFlags, SkillGetNextSkillSeed( game ) );			
			}
		}
	}
	return count;
}
//----------------------------------------------------------------------------
//Get units in area and runs a script on them
//----------------------------------------------------------------------------
int VMRunScriptOnUnitsInAreaPCT(
	SCRIPT_CONTEXT * pContext,	
	int nScriptParam,
	int nRadius,
	int nChance,
	int nFlag )
{			
	return RunScriptAndSkillOnTargetsInAreaByPCT(pContext, INVALID_ID, nScriptParam, nRadius, nChance, nFlag );
}


//----------------------------------------------------------------------------
//Get units in area and runs a script on them
//----------------------------------------------------------------------------
int VMDoSkillAndScriptOnUnitsInAreaPCT(
	SCRIPT_CONTEXT * pContext,
	int nSkill,
	int nScriptParam,
	int nRadius,
	int nChance,
	int nFlag )
{			
	return RunScriptAndSkillOnTargetsInAreaByPCT(pContext, nSkill, nScriptParam, nRadius, nChance, nFlag );
}

//----------------------------------------------------------------------------
//Get units in area and runs a skill on them
//----------------------------------------------------------------------------
int VMDoSkillOnUnitsInAreaPCT(
	SCRIPT_CONTEXT * pContext,	
	int nSkill,
	int nRadius,
	int nChance,
	int nFlag )
{			
	return RunScriptAndSkillOnTargetsInAreaByPCT(pContext, nSkill, INVALID_ID, nRadius, nChance, nFlag );
}




static void sExecuteSkillParamEvent(
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * , 
	struct EVENT_DATA* pEventData, 
	void * data, 
	DWORD dwId )
{
	EVENT_DATA *dataEvents = (EVENT_DATA*)data;
	ASSERTX( pEventData, "NO Event data" );
	ASSERTX( dataEvents, "NO Event data" );
	int nState = (int)pEventData->m_Data4;
	if( nState != (int)dataEvents->m_Data1 )
		return;
	int nSkill = (int)pEventData->m_Data1;
	ASSERTX( nSkill != INVALID_ID, "NO SKILL to get ScriptParams from" );
	const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );	
	if ( ! pSkillData )
		return;
	int nParamIndex = (int)pEventData->m_Data2;	
	int nTarget = (int)pEventData->m_Data3;
	
	int nSkillLevel = SkillGetLevel( pUnit, nSkill );
	UNIT *pTarget = UnitGetById( pGame, nTarget );
	int code_len = 0;
	BYTE * code_ptr = GetScriptParam( pGame, nParamIndex, pSkillData, &code_len );
	
	ASSERTX( code_ptr, "NO stats in skill scriptparamX" );	
	VMExecI(pGame, pUnit, pTarget, NULL, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_STATE_CLEARED, dwId );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRunScriptParamOnStateClear(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int nParamIndex )		
{			
	ASSERTX_RETZERO( nState != INVALID_ID, "INVALID STATE" );
	ASSERTX_RETZERO( pContext->unit, "No unit passed in" );
	ASSERTX_RETZERO( pContext->nSkill, "NO Skill passed in" );
	const SKILL_DATA *pSkillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! pSkillData )
		return 0;
	int code_len = 0;
	BYTE * code_ptr = GetScriptParam( pContext->game, nParamIndex, pSkillData, &code_len );
	
	ASSERTX_RETZERO( code_ptr, "NO stats in skill scriptparamX" );	

	UnitRegisterEventHandler( pContext->game, pContext->unit, EVENT_STATE_CLEARED, sExecuteSkillParamEvent, &EVENT_DATA( pContext->nSkill, nParamIndex, (pContext->object)?UnitGetId(pContext->object):INVALID_ID, nState ) );
	return 1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateOnSkillTargetWithTimeMSScript(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS,
	int clearFirst )		
{			
	UNIT *unit = pContext->unit;
	int nSkill = pContext->nSkill;
	UNIT *pTarget = SkillGetTarget( unit, nSkill, INVALID_ID );
	ASSERT_RETZERO(unit);
	ASSERTX_RETZERO( pTarget, "Target on skill is NULL" );	
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;

	return SetStateOnUnitAndExecuteScript( game, pTarget, unit, nSkill, nState, (float)timerMS, true );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTimeAndScript(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )		
{			
	UNIT *unit = pContext->unit;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, unit, ((pContext->object != NULL )?pContext->object:unit), nSkill, nState, (float)timerMS, true );
	/*
	int ticks = (int)( (float)timerMS/(float)MSECS_PER_SEC * GAME_TICKS_PER_SECOND_FLOAT );
	STATS *stats = s_StateSet( unit, NULL, nState, ticks, nSkill, NULL );	
	const SKILL_DATA *pSkillData = SkillGetData( game, pContext->nSkill );
	if( pSkillData )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( game, DATATABLE_SKILLS, pSkillData->codeStatsScriptOnTarget, &code_len);		
		VMExecI( game, unit, stats, nSkill, INVALID_LINK, nSkill, INVALID_LINK, code_ptr, code_len);
	}
	*/	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTimeAndScriptParam(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS,
	int nParamIndex )		
{			
	UNIT *unit = pContext->unit;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, unit, ((pContext->object != NULL )?pContext->object:unit), nSkill, nState, (float)timerMS, true, nParamIndex );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateWithTimeAndScriptParamObject(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS,
	int nParamIndex )		
{			
	UNIT *object = pContext->object;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(object);
	GAME * game = pContext->game;
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, object, ((pContext->unit != NULL )?pContext->unit:object), nSkill, nState, (float)timerMS, TRUE, nParamIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddStateWithTimeAndScriptParamObject(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS,
	int nParamIndex )		
{			
	UNIT *object = pContext->object;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(object);
	GAME * game = pContext->game;
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, object, ((pContext->unit != NULL )?pContext->unit:object), nSkill, nState, (float)timerMS, FALSE, nParamIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddStateWithTimeAndScript(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS )		
{			
	UNIT *unit = pContext->unit;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, unit, ((pContext->object != NULL )?pContext->object:unit), nSkill, nState, (float)timerMS, false );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddStateWithTimeAndScriptParam(
	SCRIPT_CONTEXT * pContext,	
	int nState,
	int timerMS,
	int nParam)		
{			
	UNIT *unit = pContext->unit;
	int nSkill = pContext->nSkill;
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	return SetStateOnUnitAndExecuteScript( game, unit, ((pContext->object != NULL )?pContext->object:unit), nSkill, nState, (float)timerMS, false, nParam );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStatOnOwner(
	SCRIPT_CONTEXT * pContext,	
	int nStat,
	int nValue,
	int nParam )		
{			
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	unit = UnitGetUltimateOwner( unit );
	ASSERT_RETZERO(unit);
	UnitSetStat( unit, nStat, nParam, nValue );
	return 1;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddPCTStatOnOwner(
	SCRIPT_CONTEXT * pContext,	
	int nStat,
	int nValue,
	int nParam )		
{			
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	unit = UnitGetUltimateOwner( unit );
	ASSERT_RETZERO(unit);
	int value = UnitGetStat( unit, nStat, nParam );
	value += (int)((float)value * ( (float)nValue/100.0f ) );
	UnitSetStat( unit, nStat, nParam, nValue );
	return 1;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetDamageEffectSkill(
	SCRIPT_CONTEXT * pContext,	
	int nDmgEffect,
	int nSkill )
{
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	if( pContext->stats != NULL )
	{
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_SKILL, nDmgEffect, nSkill );
	}
	else
	{
		UnitSetStat( unit, STATS_SFX_EFFECT_SKILL, nDmgEffect, nSkill );
	}
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetDamageEffectSkillOnTarget(
	SCRIPT_CONTEXT * pContext,	
	int nDmgEffect,
	int nSkill )
{
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	if( pContext->stats != NULL )
	{
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_SKILL_ON_TARGET, nDmgEffect, nSkill );
	}
	else
	{
		UnitSetStat( unit, STATS_SFX_EFFECT_SKILL_ON_TARGET, nDmgEffect, nSkill );
	}
	return 1;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetEffectParams(
	SCRIPT_CONTEXT * pContext,	
	int nDmgEffect,
	int nParam0,
	int nParam1,
	int nParam2 )		
{			
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	if( pContext->stats != NULL )
	{
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM0, nDmgEffect, nParam0 );
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM1, nDmgEffect, nParam1 );
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM2, nDmgEffect, nParam2 );
	}
	else
	{
		UnitSetStat( unit, STATS_SFX_EFFECT_PARAM0, nDmgEffect, nParam0 );
		UnitSetStat( unit, STATS_SFX_EFFECT_PARAM1, nDmgEffect, nParam1 );
		UnitSetStat( unit, STATS_SFX_EFFECT_PARAM2, nDmgEffect, nParam2 );

	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetEffect(
	SCRIPT_CONTEXT * pContext,	
	int nDmgEffect,
	int nChance,
	int nTime,
	int nRoll )		
{			
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	if( IS_CLIENT( game ) )
		return 0;
	if( pContext->stats != NULL )
	{
		nChance += StatsGetStat( pContext->stats, STATS_SKILL_EFFECT_CHANCE, nDmgEffect );
		nTime += StatsGetStat( pContext->stats, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect );
		nRoll += StatsGetStat( pContext->stats, STATS_SFX_EFFECT_ATTACK, nDmgEffect );
		StatsSetStat( game, pContext->stats, STATS_SKILL_EFFECT_CHANCE, nDmgEffect, nChance );
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect, nTime );
		StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_ATTACK, nDmgEffect, nRoll );
	}
	else
	{
		nChance += UnitGetStat( unit, STATS_SKILL_EFFECT_CHANCE, nDmgEffect );
		nTime += UnitGetStat( unit, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect );
		nRoll += UnitGetStat( unit, STATS_SFX_EFFECT_ATTACK, nDmgEffect );
		UnitSetStat( unit, STATS_SKILL_EFFECT_CHANCE, nDmgEffect, nChance );
		UnitSetStat( unit, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect, nTime );
		UnitSetStat( unit, STATS_SFX_EFFECT_ATTACK, nDmgEffect, nRoll );

	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMLearnSkill(
	UNIT * unit,
	int skill )

{
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);
	ASSERT_RETZERO((unsigned int)skill < ExcelGetNumRows(EXCEL_CONTEXT(game), DATATABLE_SKILLS));

	if (unit)
	{
		int level = UnitGetExperienceLevel( unit );

		const SKILL_DATA * pSkillData = SkillGetData( game, skill );
		if ( ! pSkillData )
			return skill;
		if ( pSkillData->pnSkillLevelToMinLevel[ 0 ] != INVALID_ID )
			level = max( level, pSkillData->pnSkillLevelToMinLevel[ 0 ] );

		SkillUnitSetInitialStats(unit, skill, level);
	}

	return skill;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatTotal(
	VMSTATE * vm,
	int stat)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	ASSERT_RETZERO((unsigned)stat < ExcelGetNumRows(NULL, DATATABLE_STATS));

	if (vm->context.unit)
		return UnitGetStatsTotal(vm->context.unit, stat);
	else
		return StatsGetTotal(vm->context.stats, stat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetBaseStatTotal(
	VMSTATE * vm,
	int stat)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	ASSERT_RETZERO((unsigned)stat < ExcelGetNumRows(NULL, DATATABLE_STATS));

	if (vm->context.unit)
		return UnitGetBaseStatsTotal(vm->context.unit, stat);
	else
		return StatsGetTotal(vm->context.stats, stat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetBaseStat(
	VMSTATE * vm,
	int stat)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	ASSERT_RETZERO((unsigned)stat < ExcelGetNumRows(NULL, DATATABLE_STATS));

	int ret = 0;

	if (vm->context.unit)
		ret = UnitGetBaseStat(vm->context.unit, stat);
	else
		ret = StatsGetStat(vm->context.stats, stat);

	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetInvLocCount(
	UNIT * unit,
	int location)
{
	if (!unit)
	{
		return 0;
	}
	return UnitInventoryGetCount(unit, location);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsInInvLoc(
	UNIT * unit,
	int location)
{
	if (!unit)
	{
		return 0;
	}
	int itemLoc,x,y;
	if (UnitGetCurrentInventoryLocation(unit, itemLoc, x, y))
	{
		return itemLoc == location;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatCur(
	VMSTATE * vm,
	int stat,
	int param)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	switch (vm->cur_stat_source)
	{
	case 1:
		ASSERT_RETZERO(vm->context.stats);
		return StatsGetStat(vm->context.stats, stat, param);
	case 2:
		ASSERT_RETZERO(vm->context.unit);
		return UnitGetStat(vm->context.unit, stat, param);
	case 3:
		ASSERT_RETZERO(vm->context.object);
		return UnitGetStat(vm->context.object, stat, param);
	case 4:
		{
			int value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetStat(unit, stat, param);
			}
			vm->cur_stat_source = vm->pre_stat_source;
			return value;
		}
	case 5:
		ASSERT(vm->context.stats && vm->context.unit);
		return StatsGetCalculatedStat(vm->context.unit, UNITTYPE_NONE, vm->context.stats, (WORD)stat, param);
	default:
		ASSERT(0);
		break;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStatIndex(
	int stat)
{
	return stat;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeRequirement(
	SCRIPT_CONTEXT * pContext,
	int stat,
	int param)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	FONTCOLOR eDefaultColor = FONTCOLOR_WHITE;

	if (ItemMeetsStatReqsForUI(pContext->unit, stat))
		return MAKE_COLOR_CODE(TRUE, eDefaultColor);
	else
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeModUnitRequirement(
	SCRIPT_CONTEXT * pContext,
	int stat,
	int param)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!pContext->unit)
	{
		return 0;
	}

	FONTCOLOR eDefaultColor = FONTCOLOR_WHITE;

	UNITID idModUnit = UIGetModUnit();
	if (idModUnit == INVALID_ID)
		return 0;		// no color

	UNIT *pModUnit = UnitGetById(pContext->game, idModUnit);
	if (!pModUnit)
		return 0;		// no color

	if (ItemMeetsStatReqsForUI(pContext->unit, stat, pModUnit))
		return MAKE_COLOR_CODE(TRUE, eDefaultColor);
	else
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
#else
	return 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeModUnitRequirement2(
	SCRIPT_CONTEXT * pContext,
	int stat1,
	int param1,
	int stat2,
	int param2)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!pContext->unit)
	{
		return 0;
	}

	FONTCOLOR eDefaultColor = FONTCOLOR_WHITE;

	UNITID idModUnit = UIGetModUnit();
	if (idModUnit == INVALID_ID)
		return 0;		// no color

	UNIT *pModUnit = UnitGetById(pContext->game, idModUnit);
	if (!pModUnit)
		return 0;		// no color

	if (ItemMeetsStatReqsForUI(pContext->unit, stat1, pModUnit) &&
		ItemMeetsStatReqsForUI(pContext->unit, stat2, pModUnit))
		return MAKE_COLOR_CODE(TRUE, eDefaultColor);
	else
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
#else
	return 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMFeedChangeForHoverItem(
	UNIT *pUnit,
	int nStat,
	int nCheckBonus)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!pUnit)
	{
		return 0;
	}

	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = pUnit; //GameGetControlUnit(pGame);
	UNITID idItem = UIGetCursorUnit();
	if (idItem == INVALID_ID)
		idItem = UIGetHoverUnit();
	UNIT *pItem = (idItem != INVALID_ID ? UnitGetById(pGame, idItem) : NULL);
	if (!pItem || !pItem->invnode ||
//		ItemIsInEquipLocation(pPlayer, pItem))
		ItemIsEquipped(pItem, pPlayer))
	{
		return 0;
	}
	else if (!InventoryItemMeetsClassReqs(pItem, pPlayer))
	{
		return 0;
	}
	else
	{
		
		int nBonusStat = INVALID_LINK;
		if (nCheckBonus != 0)
		{
			const STATS_DATA *pStatsData = StatsGetData(pGame, nStat);
			nBonusStat = StatsGetAssociatedStat(pStatsData, 1);
		}

		int nHoverVal = UnitGetStat(pItem, nStat);
		nHoverVal -= (nBonusStat != INVALID_LINK ? UnitGetStat(pItem, nBonusStat) : 0);

		int nLocation, x, y;
		if (ItemGetEquipLocation(pPlayer, pItem, &nLocation, &x, &y) &&
			nLocation != INVLOC_NONE)
		{
			// There's an open equip slot, no need to compare against anything.
			return nHoverVal;
		}

		// Ok, time for some fancy footwork Mabel
		static const int MAX_ITEMS = 6;
		UNIT* pIgnoreItems[MAX_ITEMS];
		BOOL  pbPreventLoc[MAX_ITEMS];

		memclear(pIgnoreItems, sizeof(UNIT*) * MAX_ITEMS);
		memclear(pbPreventLoc, sizeof(BOOL) * MAX_ITEMS);

		BOOL bEmptySlot = FALSE;
		if (ItemGetItemsAlreadyInEquipLocations(pPlayer, pItem, FALSE, bEmptySlot, pIgnoreItems, MAX_ITEMS, pbPreventLoc) > 0)
		{
			int nMaxNonPreventStat = 0;
			BOOL bNonPreventFound = FALSE;
			int nTotalPreventStat = 0;
			for (int i=0; i < MAX_ITEMS; i++)
			{
				if (pIgnoreItems[i])
				{
					int nVal = UnitGetStat(pIgnoreItems[i], nStat);
					nVal -= (nBonusStat != INVALID_LINK ? UnitGetStat(pIgnoreItems[i], nBonusStat) : 0);

					if (nVal)
					{
						if (pbPreventLoc[i])
						{
							nTotalPreventStat += nVal;
						}
						else if (!bEmptySlot)		// only check a replace if there's no empty slot that will work
						{
							if (bNonPreventFound)
								nMaxNonPreventStat = MAX(nMaxNonPreventStat, nVal);
							else
								nMaxNonPreventStat = nVal;
							bNonPreventFound = TRUE;
						}
					}
				}
			}

			nHoverVal -= nMaxNonPreventStat + nTotalPreventStat;
		}

		return nHoverVal;
	}
#else
	return 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMStatChangeForHoverItem(
	UNIT *pUnit,
	int nStat)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!pUnit)
	{
		return 0;
	}

	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = pUnit; //GameGetControlUnit(pGame);
	UNITID idItem = UIGetCursorUnit();
	if (idItem == INVALID_ID)
		idItem = UIGetHoverUnit();
	UNIT *pItem = (idItem != INVALID_ID ? UnitGetById(pGame, idItem) : NULL);
	if (!pItem || !pItem->invnode ||
//		ItemIsInEquipLocation(pPlayer, pItem))
		ItemIsEquipped(pItem, pPlayer))
	{
		return 0;
	}
	else if (!InventoryItemMeetsClassReqs(pItem, pPlayer))
	{
		return 0;
	}
	else
	{
		const STATS_DATA *pStatsData = StatsGetData(pGame, nStat);
		int nBonusStat = StatsGetAssociatedStat(pStatsData, 1);

		int nHoverVal = UnitGetStat(pItem, nBonusStat);

		int nLocation, x, y;
		if (ItemGetEquipLocation(pPlayer, pItem, &nLocation, &x, &y) &&
			nLocation != INVLOC_NONE)
		{
			// There's an open equip slot, no need to compare against anything.
			return nHoverVal;
		}

		// Ok, time for some fancy footwork Mabel
		static const int MAX_ITEMS = 6;
		UNIT* pIgnoreItems[MAX_ITEMS];
		BOOL  pbPreventLoc[MAX_ITEMS];

		memclear(pIgnoreItems, sizeof(UNIT*) * MAX_ITEMS);
		memclear(pbPreventLoc, sizeof(BOOL) * MAX_ITEMS);

		BOOL bEmptySlot = FALSE;
		if (ItemGetItemsAlreadyInEquipLocations(pPlayer, pItem, FALSE, bEmptySlot, pIgnoreItems, MAX_ITEMS, pbPreventLoc) > 0)
		{
			int nMaxNonPreventStat = 0;
			int nTotalPreventStat = 0;
			for (int i=0; i < MAX_ITEMS; i++)
			{
				if (pIgnoreItems[i])
				{
					int nVal = UnitGetStat(pIgnoreItems[i], nBonusStat);

					if (nVal)
					{
						if (pbPreventLoc[i])
						{
							nTotalPreventStat += nVal;
						}
						else if (!bEmptySlot)		// only check a replace if there's no empty slot that will work
						{
							nMaxNonPreventStat = MAX(nMaxNonPreventStat, nVal);
						}
					}
				}
			}

			nHoverVal -= nMaxNonPreventStat + nTotalPreventStat;
		}

		return nHoverVal;
	}
#else
	return 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMFeedColorCode(
	SCRIPT_CONTEXT * pContext,
	int stat)
{
	ASSERT_RETZERO(pContext);
	if (pContext->unit)
	{
		int nNewFeedTotal = pContext->nParam1;			// this is what the feed will be with the new item
		int nFeedChange = pContext->nParam2;		// for color purposes the feed change is the net change considering stat change as well
													//  so if feed goes up by 10 but the stat also goes up by 12 then this will be -2

		// nFeedChange == (nFeedTotal - oldfeed) - statchange

		const STATS_DATA *pStatsData = StatsGetData(pContext->game, stat);
		int nMaxStat = StatsGetAssociatedStat(pStatsData, 0);

		//int nFeedMax = UnitGetStat(pContext->unit, nMaxStat);
		int nCurrentFeed = UnitGetStat(pContext->unit, stat);
		int nStatChange = (nNewFeedTotal - nCurrentFeed) - nFeedChange;

		int nNewFeedMax = UnitGetStat(pContext->unit, nMaxStat) + nStatChange;

		if (nNewFeedTotal > nNewFeedMax)
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}

		if (nFeedChange > 0)
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_YELLOW);
		}
		else if (nFeedChange < 0)
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_GREEN);
		}
	}

	return MAKE_COLOR_CODE(FALSE, FONTCOLOR_WHITE);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodePosNegVal2(
	SCRIPT_CONTEXT * pContext)
{
	ASSERT_RETZERO(pContext);
	if (pContext->nParam2 < 0)
	{
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_YELLOW);
	}

	if (pContext->nParam2 > 0)
	{
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_GREEN);
	}

	return MAKE_COLOR_CODE(FALSE, FONTCOLOR_WHITE);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodePosNeg(
	SCRIPT_CONTEXT *pContext)
{
	if (pContext->nParam1 > 0)
	{
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_GREEN);
	}
	if (pContext->nParam1 < 0)
	{
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
	}

	return MAKE_COLOR_CODE(FALSE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodePrice(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(UnitGetGame(unit));	
	if( UnitGetCurrency( pPlayer ) < EconomyItemBuyPrice( unit ) )
	{
		return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
	}


	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsA(
	UNIT *unit,
	int unittype)
{	
	if (!unit)
	{
		return 0;
	}

	return UnitIsA(unit, unittype);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMUnitContainsUnitType(
		  UNIT *unit,
		  int unittype)
{	
	if (!unit)
	{
		return 0;
	}
	DWORD dwInvIterateFlags = 0;
	//SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	//SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );			
	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( unit, pItem, dwInvIterateFlags );	
	while ( pItem != NULL)
	{		
		if( UnitIsA	( pItem, unittype ) )
			return TRUE;
		pItem = UnitInventoryIterate( unit, pItem, dwInvIterateFlags );
	}
	return 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsSubscriber(
	UNIT * pUnit)
{
	if (! pUnit )
		return 0;
	return PlayerIsSubscriber(UnitGetUltimateContainer(pUnit));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsElite(
	UNIT * pUnit)
{
	if (! pUnit )
		return 0;
	return PlayerIsElite( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsHardcore(
	UNIT * pUnit)
{
	if (! pUnit )
		return 0;
	return PlayerIsHardcore( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetDifficulty(
	UNIT * pUnit)
{
	if (! pUnit )
		return 0;
	return UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetStat(
	UNIT * pUnit,
	int stat,
	int param)
{
	if (! pUnit )
		return 0;
	return UnitGetStat(pUnit, stat, param);
}
//----------------------------------------------------------------------------
//returns the number of points invested in a pane
//----------------------------------------------------------------------------
int VMGetTierInvestmentForPane( SCRIPT_CONTEXT * pContext,
							    int nPaneIndex)
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( UnitGetGenus( pContext->unit ) == GENUS_PLAYER );
	return UnitGetStat( pContext->unit, STATS_SKILL_POINTS_TIER_SPENT, nPaneIndex );
}

//----------------------------------------------------------------------------
// Determine act based upon current level the unit is in.
//----------------------------------------------------------------------------
int VMGetAct(
	UNIT * pUnit)
{
	if (! pUnit )
		return 0;
	const LEVEL *pLevel = UnitGetLevel(pUnit);
	if(! pLevel )
		return 0;
	return LevelGetAct(pLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMEmailSendItemOkay(
	UNIT * pUnit,
	SCRIPT_CONTEXT * pContext)
{

	if (pUnit == NULL)
	{
		return FALSE;
	}
	
	return GameMailCanSendItem( pUnit, pContext->object );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMEmailReceiveItemOkay(
	UNIT * pUnit,
	SCRIPT_CONTEXT * pContext)
{
	if (pUnit == NULL)
	{
		return FALSE;
	}
	
	return GameMailCanReceiveItem( pUnit, pContext->object );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeSubscriber(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	// NOTE: pContext->unit is not the player, it is an item for which we are displaying a subscriber requirement
#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();

	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		if (PlayerIsSubscriber(pPlayer))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
		}
	}
#endif
	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemRequiresSubscriber(
	UNIT * pUnit)
{	
	if (!pUnit)
	{
		return 0;
	}

	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY ) )
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeVariantNormal(
	SCRIPT_CONTEXT * pContext)
{
	if (!pContext->unit)
	{
		return 0;
	}

#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();
	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		if (GameVariantFlagsGetStaticUnit(pPlayer) != 0)
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}
	}
#endif

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeVariantHardcore(
	SCRIPT_CONTEXT * pContext)
{
	if (!pContext->unit)
	{
		return 0;
	}

#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();
	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		if (GameVariantFlagsGetStaticUnit(pPlayer) != MAKE_MASK(GV_HARDCORE))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}
	}
#endif

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeVariantElite(
	SCRIPT_CONTEXT * pContext)
{
	if (!pContext->unit)
	{
		return 0;
	}

#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();
	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		if (GameVariantFlagsGetStaticUnit(pPlayer) != MAKE_MASK(GV_ELITE))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}
	}
#endif

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeVariantHardcoreElite(
	SCRIPT_CONTEXT * pContext)
{
	if (!pContext->unit)
	{
		return 0;
	}

#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();
	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		if (GameVariantFlagsGetStaticUnit(pPlayer) != (MAKE_MASK(GV_ELITE) | MAKE_MASK(GV_HARDCORE)))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}
	}
#endif

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeNightmare(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

#if(!ISVERSION(SERVER_VERSION))
	UNIT *pPlayer = UIGetControlUnit();
	if(pPlayer && UnitIsPlayer(pPlayer))
	{
		const UNIT_DATA * pUnitData = UnitGetData(pContext->unit);
		if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY) &&
			UnitGetStat(pContext->unit, STATS_ITEM_DIFFICULTY_SPAWNED) != UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
		}
	}
#endif

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsNightmareSpecific(
	UNIT * pUnit)
{	
	if (!pUnit)
	{
		return 0;
	}

 	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY) &&
		UnitGetStat(pUnit, STATS_ITEM_DIFFICULTY_SPAWNED) == DIFFICULTY_NIGHTMARE)

 	{
 		return 1;
 	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsVariantNormal(
	UNIT * pUnit)
{
	if (!pUnit)
	{
		return 0;
	}

	if (UnitGetStat(pUnit, STATS_GAME_VARIANT) == 0)
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsVariantHardcore(
	UNIT * pUnit)
{
	if (!pUnit)
	{
		return 0;
	}

	if (UnitGetStat(pUnit, STATS_GAME_VARIANT) == MAKE_MASK(GV_HARDCORE))
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsVariantElite(
	UNIT * pUnit)
{
	if (!pUnit)
	{
		return 0;
	}

	if (UnitGetStat(pUnit, STATS_GAME_VARIANT) == MAKE_MASK(GV_ELITE))
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsVariantHardcoreElite(
	UNIT * pUnit)
{
	if (!pUnit)
	{
		return 0;
	}

	if (UnitGetStat(pUnit, STATS_GAME_VARIANT) == (MAKE_MASK(GV_HARDCORE) | MAKE_MASK(GV_ELITE)))
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeClassRequirement(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(UnitGetGame(pContext->unit));
	if (pPlayer)
	{
		if (InventoryItemMeetsClassReqs(pContext->unit, pPlayer))
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_WHITE);
		}
	}

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeSkillSlots(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(UnitGetGame(pContext->unit));
	if (pPlayer)
	{
		const SKILL_DATA * pSkillData = SkillGetData( UnitGetGame(pContext->unit), UnitGetSkillID( pContext->unit ) );
		if ( ! pSkillData )
			return 0;

		int SkillsOpen = PlayerGetAvailableSkillsOfGroup( UnitGetGame(pContext->unit), pPlayer, pSkillData->pnSkillGroup[ 0 ] ) -
						 PlayerGetKnownSkillsOfGroup( UnitGetGame(pContext->unit), pPlayer, pSkillData->pnSkillGroup[ 0 ] );

		if ( SkillsOpen > 0)
		{
			return MAKE_COLOR_CODE(TRUE, FONTCOLOR_VERY_LIGHT_CYAN);
		}
	}

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeSkillUsability(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(UnitGetGame(pContext->unit));
	if (pPlayer)
	{
		int nSkill = pContext->nParam2;
		const SKILL_DATA * pSkillData = SkillGetData( pContext->game, nSkill );
		ASSERT_RETZERO( pSkillData );

		if ( UnitIsA( pPlayer, pSkillData->nRequiredUnittype ) )
			return 0;		// don't change the color
	}

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);	// mark as unusable
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMColorCodeSkillGroupUsability(
	SCRIPT_CONTEXT * pContext)
{	
	if (!pContext->unit)
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(UnitGetGame(pContext->unit));
	if (pPlayer)
	{
		int nSkillGroup = pContext->nParam2;

		int nSkillCount = ExcelGetCount(EXCEL_CONTEXT(pContext->game), DATATABLE_SKILLS);
		for (int i = 0; i < nSkillCount; i++)
		{
			const SKILL_DATA *pSkillData = SkillGetData(pContext->game, i);
			if (!pSkillData)
				continue;

			for (int j = 0; j < MAX_SKILL_GROUPS_PER_SKILL; j++)
			{
				if (pSkillData->pnSkillGroup[j] == nSkillGroup)
				{
					if (UnitIsA(pPlayer, pSkillData->nRequiredUnittype))
					{
						// the player can use one of the skills in the group, so they're ok
						return 0;		// don't change the color
					}
				}
			}
		}
	}

	return MAKE_COLOR_CODE(TRUE, FONTCOLOR_LIGHT_RED);	// mark as unusable
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMFontColor(
	SCRIPT_CONTEXT * pContext,
	int nColorIndex)
{
	return MAKE_COLOR_CODE(TRUE, nColorIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsNoDrop(
	UNIT * unit)
{
	if (!unit)
	{
		return 0;
	}
	return UnitIsNoDrop(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemIsNoTrade(
	UNIT * unit)
{
	if (!unit)
	{
		return 0;
	}
	return ItemIsNoTrade(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMObjectIsNoTrade(
	SCRIPT_CONTEXT * pContext)
{
	if (!pContext || !pContext->object)
	{
		return 0;
	}
	return ItemIsNoTrade(pContext->object);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemBuyPriceByValue(
	UNIT * unit,
	int valueType )
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && ( UIIsMerchantScreenUp() ) )
	{
		cCurrency currency = EconomyItemBuyPrice( unit );
		return currency.GetValue( (ECURRENCY_VALUES)valueType );
		//return ItemGetBuyPrice(unit);
	}

#endif// !ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemBuyPrice(
	UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && ( UIIsMerchantScreenUp() ) )
	{
		cCurrency currency = EconomyItemBuyPrice( unit );
		return currency.GetValue( KCURRENCY_VALUE_INGAME );
		//return ItemGetBuyPrice(unit);
	}

#endif// !ISVERSION(SERVER_VERSION)
	return FALSE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMBuyPriceRealWorld(
	UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && ( UIIsMerchantScreenUp() ) )
	{
		cCurrency currency = EconomyItemBuyPrice( unit );
		return currency.GetValue( KCURRENCY_VALUE_REALWORLD );
		//return ItemGetBuyPrice(unit);
	}

#endif// !ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemSellPrice(
	UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && UIIsMerchantScreenUp())
	{
		cCurrency currency = EconomyItemSellPrice( unit );
		return currency.GetValue( KCURRENCY_VALUE_INGAME );// ItemGetSellPrice(unit);
	}
#endif// !ISVERSION(SERVER_VERSION)

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemSellPriceByValue(
						  UNIT * unit,
						  int valueType )
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && ( UIIsMerchantScreenUp() ) )
	{
		cCurrency currency = EconomyItemSellPrice( unit );
		return currency.GetValue( (ECURRENCY_VALUES)valueType );
	}

#endif// !ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemSellPriceRealWorld(
	UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit && UIIsMerchantScreenUp())
	{
		cCurrency currency = EconomyItemSellPrice( unit );
		return currency.GetValue( KCURRENCY_VALUE_REALWORLD );// ItemGetSellPrice(unit);
	}
#endif// !ISVERSION(SERVER_VERSION)

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHitChance(
					UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit )
	{
		int nLevel = UnitGetStat( unit, STATS_LEVEL );
		const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(NULL, DATATABLE_MONLEVEL, nLevel);
		float fTheoreticalAC =float( monster_level->nArmor + monster_level->nDexterity / 2.0f );
		float fToHitChance = 100.0f * ( (float)UnitGetStat( unit, STATS_TOHIT ) / ( (float)UnitGetStat( unit, STATS_TOHIT ) + (float)fTheoreticalAC ) );
		if( fToHitChance < 5 )
		{
			fToHitChance = 5;
		}
		if( fToHitChance > 95 )
		{
			fToHitChance = 95;
		}
		return (int)fToHitChance;
	}
#endif// !ISVERSION(SERVER_VERSION)

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDodgeChance(
				UNIT * unit)
{	
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}

	if (unit )
	{
		int nLevel = UnitGetStat( unit, STATS_LEVEL );
		const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(NULL, DATATABLE_MONLEVEL, nLevel);
		float fTheoreticalToHit = (float)( nLevel + monster_level->nToHitBonus );
		float fToHitChance = 100.0f * ( fTheoreticalToHit / ( fTheoreticalToHit + (float)UnitGetStat( unit, STATS_AC ) ) );
		if( fToHitChance < 5 )
		{
			fToHitChance = 5;
		}
		if( fToHitChance > 95 )
		{
			fToHitChance = 95;
		}
		return (int)( 100.0f - fToHitChance );
	}
#endif// !ISVERSION(SERVER_VERSION)

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemNumAffixes(
	UNIT * unit)
{
	if (unit && UnitIsA(unit, UNITTYPE_ITEM))
	{
		return AffixGetAffixCount( unit, AF_ALL_FAMILIES );	
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemQualityPriceMult(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	if (unit)
	{
		int nQuality = ItemGetQuality( unit );
		if (nQuality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData( nQuality );
			ASSERT_RETVAL(quality_data, 100);
			if (quality_data->fPriceMultiplier > 0.0f)
			{
				return (int)(quality_data->fPriceMultiplier * 100.0f);		// we're gonna fudge this here and just always divide the price functions by 100
			}
		}
	}

	return 100;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsAlive(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	return !IsUnitDeadOrDying(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMonsterLevel(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	LEVEL * pLevel = UnitGetLevel(unit);
	if(!pLevel)
	{
		return 0;
	}

	return LevelGetMonsterExperienceLevel(pLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasActiveTask(
	UNIT * unit)
{
#if !ISVERSION(SERVER_VERSION)
	if (!unit)
	{
		return 0;
	}
	return c_TaskIsTaskActiveInThisLevel();
#else
	ASSERT(!"not server version !");
	return FALSE;
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsUsable(
	UNIT * unit)
{	
	if (!unit)
	{
		return FALSE;
	}

	UNIT *pPlayer = GameGetControlUnit(AppGetCltGame());
	if (!pPlayer || UnitGetUltimateContainer(unit) != pPlayer)
		return FALSE;

	return UnitIsUsable(pPlayer, unit) == USE_RESULT_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsExaminable(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	return UnitIsExaminable(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsOperatable(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	UNIT *pControlUnit = GameGetControlUnit(UnitGetGame(unit));
	if (!pControlUnit)
		return FALSE;

	return ItemIsQuestUsable( pControlUnit, unit ) == USE_RESULT_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemQuality(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	return ItemGetQuality( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasDomName(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	return UnitHasMagicName(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMeetsItemRequirements(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	return InventoryCheckStatRequirements(UnitGetUltimateOwner(unit), unit, NULL, NULL, INVLOC_NONE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemMeetsClassReqs(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	GAME *pGame = UnitGetGame(unit);
	ASSERT_RETZERO(IS_CLIENT(pGame));
	UNIT *pControlUnit = GameGetControlUnit(pGame);
	ASSERT_RETZERO(pControlUnit);

	return InventoryItemMeetsClassReqs(unit, pControlUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMWeaponDPS(
	UNIT * unit)
{	
	if (!unit)
	{
		return 0;
	}

	const UNIT_DATA * pUnitData = UnitGetData(unit);
	ASSERT_RETZERO(pUnitData);

	float fEstDPS = pUnitData->fEstDPS;

	// see if the damage should be adjusted based on level increase (because of rank for example)

	int nExperienceLevel = UnitGetExperienceLevel(unit);

//	if (nExperienceLevel != pUnitData->nItemExperienceLevel)
	{
		GAME *pGame = UnitGetGame(unit);
		const ITEM_LEVEL * pItemLevel1 = ItemLevelGetData( pGame, nExperienceLevel);
		const ITEM_LEVEL * pItemLevel2 = ItemLevelGetData( pGame, 1);
		fEstDPS *= ((float)pItemLevel1->nDamageMult / (float)pItemLevel2->nDamageMult);
	}

	// for each common/rare/legendary affix, add +10%/+15%/+20% to DPS - per bug #13817 [mk]

	static const int iCommonIdx = GlobalIndexGet( GI_AFFIX_TYPE_COMMON );
	static const int iRareIdx = GlobalIndexGet( GI_AFFIX_TYPE_RARE );
	static const int iLegendaryIdx = GlobalIndexGet( GI_AFFIX_TYPE_LEGENDARY );

	int iDamagePercent = 100;

	// iterate through affixes and assign an arbitrary bonus to DPS for each
	//
	UNIT_ITERATE_STATS_RANGE(unit, STATS_APPLIED_AFFIX, list, ii, 32)
	{
		int nAffix = list[ii].value;
		const AFFIX_DATA* affix_data = AffixGetData(nAffix);

		ASSERT_RETVAL(affix_data, (int)(fEstDPS + 0.5f));

		if (AffixIsType(affix_data, iLegendaryIdx))
		{
			iDamagePercent += 20;
		}
		else if (AffixIsType(affix_data, iRareIdx))
		{
			iDamagePercent += 15;
		}
		else if (AffixIsType(affix_data, iCommonIdx))
		{
			iDamagePercent += 10;
		}
	}
	UNIT_ITERATE_STATS_END;

	// also iterate through affixes from mods
	//
	DWORD flags(0);
	UNIT * pModCurr = UnitInventoryIterate(unit, NULL, flags);
	while (pModCurr)
	{
		UNIT * pModNext = UnitInventoryIterate(unit, pModCurr, flags);
		if (pModCurr)
		{
			UNIT_ITERATE_STATS_RANGE(pModCurr, STATS_APPLIED_AFFIX, list, ii, 32)
			{
				int nAffix = list[ii].value;
				const AFFIX_DATA* affix_data = AffixGetData(nAffix);

				ASSERT_RETVAL(affix_data, (int)(fEstDPS + 0.5f));

				if (AffixIsType(affix_data, iLegendaryIdx))
				{
					iDamagePercent += 20;
				}
				else if (AffixIsType(affix_data, iRareIdx))
				{
					iDamagePercent += 15;
				}
				else if (AffixIsType(affix_data, iCommonIdx))
				{
					iDamagePercent += 10;
				}
			}
			UNIT_ITERATE_STATS_END;
		}
		pModCurr = pModNext;
	}

	fEstDPS *= ((float)iDamagePercent / 100.0f);

	return (int)(fEstDPS + 0.5f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillHasReqWeapon(
	UNIT * unit,
	int skillID )
{	
	if (!unit)
	{
		return 0;
	}
	if( skillID == INVALID_ID )
		return 0;
	UNIT *weaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( unit, skillID, weaponArray, false );
	for( int t = 0; t < MAX_WEAPON_LOCATIONS_PER_SKILL; t++ )
		if( weaponArray[ t ] ) //if not NULL we are set.
			return 1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemHasSkillWhenUsed(
	UNIT * unit,
	int skillID )
{	
	if (!unit)
	{
		return 0;
	}
	if( skillID == INVALID_ID )
		return 0;

	const UNIT_DATA *pUnitData = UnitGetData(unit);

	return (pUnitData->nSkillWhenUsed == skillID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMVisibleEnemiesInRadius(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	return tTargetingQuery.nUnitIdCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMEnemiesInRadius(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	return tTargetingQuery.nUnitIdCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMChampionsInRadius(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_CHAMPIONS );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	return tTargetingQuery.nUnitIdCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sVMBossesSearchTestIsBoss(
	const UNIT * pUnit,
	const EVENT_DATA * pEventData)
{
	REF(pEventData);
	ASSERT_RETFALSE(pUnit);
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETFALSE(pUnitData);

	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_BOSS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMBossesInRadius(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	tTargetingQuery.pfnFilter = sVMBossesSearchTestIsBoss;
	SkillTargetQuery( game, tTargetingQuery );

	return tTargetingQuery.nUnitIdCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDistanceSquaredToChampion(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return radius * radius;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_CHAMPIONS );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	if(tTargetingQuery.nUnitIdCount > 0)
	{
		UNIT * pChampion = UnitGetById(game, pnUnitIds[0]);
		if(pChampion)
		{
			return int(UnitsGetDistanceSquared(unit, pChampion));
		}
	}
	return radius * radius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDistanceSquaredToBoss(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return radius * radius;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	tTargetingQuery.pfnFilter = sVMBossesSearchTestIsBoss;
	SkillTargetQuery( game, tTargetingQuery );

	if(tTargetingQuery.nUnitIdCount > 0)
	{
		UNIT * pChampion = UnitGetById(game, pnUnitIds[0]);
		if(pChampion)
		{
			return int(UnitsGetDistanceSquared(unit, pChampion));
		}
	}
	return radius * radius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMChampionHPPct(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_CHAMPIONS );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	if(tTargetingQuery.nUnitIdCount > 0)
	{
		UNIT * pChampion = UnitGetById(game, pnUnitIds[0]);
		if(pChampion)
		{
			int nHPMax = UnitGetStat(pChampion, STATS_HP_MAX);
			int nHPCur = UnitGetStat(pChampion, STATS_HP_CUR);
			return nHPCur * 100 / nHPMax;
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMBossHPPct(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	tTargetingQuery.pfnFilter = sVMBossesSearchTestIsBoss;
	SkillTargetQuery( game, tTargetingQuery );

	if(tTargetingQuery.nUnitIdCount > 0)
	{
		UNIT * pChampion = UnitGetById(game, pnUnitIds[0]);
		if(pChampion)
		{
			int nHPMax = UnitGetStat(pChampion, STATS_HP_MAX);
			int nHPCur = UnitGetStat(pChampion, STATS_HP_CUR);
			return nHPCur * 100 / nHPMax;
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMonstersPctLeft(
	UNIT * unit)
{
	if (!unit)
	{
		return 0;
	}

	if(!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return 0;
	}

	return c_PlayerGetMusicInfoMonsterPercentRemaining(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMEnemyCorpsesInRadius(
	GAME * game,
	UNIT * unit,
	int radius)
{	
	ASSERT_RETZERO(game);
	if (!unit)
	{
		return 0;
	}

	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DEAD );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= float(radius);
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( game, tTargetingQuery );

	return tTargetingQuery.nUnitIdCount;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sVMGetTagValue(
	UNIT * unit,
	int time,
	TAG_SELECTOR eTagSelector)
{
	if (!unit)
	{
		return 0;
	}
	ASSERT_RETZERO(time > 0);

	if (time > VALUE_TIME_HISTORY)
	{
		time = VALUE_TIME_HISTORY;
	}

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, eTagSelector);
	if (!pTag)
	{
		return 0;
	}

	int nCount = 0;
	int nOffset = pTag->m_nCurrent;
	do
	{
		nCount += pTag->m_nCounts[nOffset];
		time--;
		nOffset--;
		if (nOffset < 0)
		{
			nOffset = VALUE_TIME_HISTORY-1;
		}
	}
	while(time > 0);

	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMonstersKilled(
	UNIT * unit,
	int time)
{
	return sVMGetTagValue(unit, time, TAG_SELECTOR_MONSTERS_KILLED_TEAM);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMonstersKilledNonTeam(
	UNIT * unit,
	int time)
{
	return sVMGetTagValue(unit, time, TAG_SELECTOR_MONSTERS_KILLED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHPLost(
	UNIT * unit,
	int time)
{	
	return sVMGetTagValue(unit, time, TAG_SELECTOR_HP_LOST) >> StatsGetShift(UnitGetGame(unit), STATS_HP_CUR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMetersMoved(
	UNIT * unit,
	int time)
{
	// METERS_MOVED is actually in centimeters
	return sVMGetTagValue(unit, time, TAG_SELECTOR_METERS_MOVED) / 100;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAttacks(
	UNIT * unit,
	int time)
{	
	return sVMGetTagValue(unit, time, TAG_SELECTOR_AGGRESSIVE_SKILLS_STARTED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDamageDone(
	UNIT * unit,
	int time)
{	
	return sVMGetTagValue(unit, time, TAG_SELECTOR_DAMAGE_DONE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDamageRider(
	VMSTATE * vm)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	if (vm->context.stats)
	{
		vm->context.stats = StatsListAddRider(vm->context.game, vm->context.stats, SELECTOR_DAMAGE_RIDER);
	}
	else
	{
		vm->context.stats = StatsListAddRider(vm->context.game, UnitGetStats(vm->context.unit), SELECTOR_DAMAGE_RIDER);
	}
	VMInitState(vm);
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMProcRider(
	VMSTATE * vm)
{
	ASSERT_RETZERO(vm && (vm->context.stats || vm->context.unit));

	if (vm->context.stats)
	{
		vm->context.stats = StatsListAddRider(vm->context.game, vm->context.stats, SELECTOR_PROC_RIDER);
	}
	else
	{
		vm->context.stats = StatsListAddRider(vm->context.game, UnitGetStats(vm->context.unit), SELECTOR_PROC_RIDER);
	}
	VMInitState(vm);
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMKnockback(
	SCRIPT_CONTEXT * pContext,
	int nCentimeters )
{
	UNIT * pAttacker = pContext->unit;
	UNIT * pDefender = pContext->object;

	float fDistance = nCentimeters / 100.0f;

	s_UnitKnockback( pContext->game, pAttacker, pDefender, fDistance );

	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMUseStateDuration(
	UNIT * unit)
{
	if (!unit)
	{
		return 0;
	}
	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETZERO(unit_data);
	if (unit_data->nSkillWhenUsed == INVALID_ID)
	{
		return 0;
	}
	UnitSetStat( unit, STATS_SKILL_EXECUTED_BY_ITEMCLASS, UnitGetClass( unit ) );
	int ret = SkillDataGetStateDuration(UnitGetGame(unit), unit, unit_data->nSkillWhenUsed, SkillGetLevel(unit, unit_data->nSkillWhenUsed), FALSE);
	return ret ? ret : 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMUsesMissiles(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return FALSE;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if (! pSkillData )
		return 0;

	return SkillDataTestFlag(pSkillData, SKILL_FLAG_USES_MISSILES);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMUsesLasers(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return FALSE;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if ( ! pSkillData )
		return FALSE;

	return SkillDataTestFlag(pSkillData, SKILL_FLAG_USES_LASERS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMissileCount(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return 0;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if ( ! pSkillData )
		return 0;

	// If the item has some affix(es) that add missiles, add that here:
	// ...

	return pSkillData->nMissileCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetLaserCount(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return 0;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if ( ! pSkillData )
		return 0;

	// If the item has some affix(es) that add lasers, add that here:
	// ...

	return pSkillData->nLaserCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDoesFieldDamage(
	UNIT * pUnit)
{
	if (UnitGetStatAny(pUnit, STATS_DAMAGE_FIELD, NULL) > 0)
		return TRUE;

	STATS * rider = UnitGetRider(pUnit, NULL);
	while (rider)
	{
		if (StatsGetStatAny( rider, STATS_DAMAGE_FIELD, NULL ) > 0)
			return TRUE;

		rider = UnitGetRider(pUnit, rider);
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDistToPlayer(
	UNIT * pUnit)
{
	GAME *pGame = UnitGetGame(pUnit);
	if (IS_SERVER(pGame))
	{
		return 0;
	}

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
	{
		return 0;
	}

	float fDistance = sqrt(UnitsGetDistanceSquared(pPlayer, pUnit));

	return (int)fDistance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetShotsPerMinute(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return 0;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if ( ! pSkillData )
		return 0;

	int nTicksPerShot = SkillGetCooldown(UnitGetGame(pUnit), pUnit, pUnit, pUnitData->nStartingSkills[0], pSkillData, 1);
	if(nTicksPerShot <= 0)
		return 0;

	int nShotsPerMinute = (GAME_TICKS_PER_SECOND * SECONDS_PER_MINUTE) / nTicksPerShot;

	return nShotsPerMinute;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMillisecondsPerShot(
	UNIT * pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if(pUnitData->nStartingSkills[0] == INVALID_ID)
	{
		return 0;
	}

	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pUnitData->nStartingSkills[0]);
	if ( ! pSkillData )
		return 0;

	int nTicksPerShot = SkillGetCooldown(UnitGetGame(pUnit), pUnit, pUnit, pUnitData->nStartingSkills[0], pSkillData, 1);
	if(nTicksPerShot <= 0)
		return 0;

	int nMilliSecondsPerShot = (nTicksPerShot * MSECS_PER_SEC) / (GAME_TICKS_PER_SECOND);

	return nMilliSecondsPerShot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPlayerCritChance(
	UNIT * pUnit,
	int nSlot)
{
	if (!pUnit)
		return 0;
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return 0;
	if (nSlot < 0 || nSlot >= MAX_WEAPONS_PER_UNIT)
		return 0;

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	memclear(pWeapons, MAX_WEAPONS_PER_UNIT * sizeof(UNIT *));
#if !ISVERSION(SERVER_VERSION)
	UISetIgnoreStatMsgs(TRUE);
#endif
	pWeapons[nSlot] = WardrobeGetWeapon(UnitGetGame(pUnit), nWardrobe, nSlot);

	if (pWeapons[nSlot] && StatsGetParent(pWeapons[nSlot]) == pUnit)
		UnitInventorySetEquippedWeapons(pUnit, pWeapons);

	int nCritChance = UnitGetStat(pUnit, STATS_CRITICAL_CHANCE);
	if( AppIsTugboat() )
	{
		nCritChance += UnitGetStatMatchingUnittype( pUnit, pWeapons, MAX_WEAPONS_PER_UNIT, STATS_CRITICAL_CHANCE_UNITTYPE );
	}
	int nCritChancePct = UnitGetStat(pUnit, STATS_CRITICAL_CHANCE_PCT);
	nCritChance += PCT(nCritChance, nCritChancePct);
	nCritChance = PIN(nCritChance, 0, 95);

	UnitInventorySetEquippedWeapons(pUnit, NULL);
#if !ISVERSION(SERVER_VERSION)
	UISetIgnoreStatMsgs(FALSE);
#endif
	return nCritChance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPlayerCritDamage(
	UNIT * pUnit,
	int nSlot)
{
	if (!pUnit)
		return 0;
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return 0;
	if (nSlot < 0 || nSlot >= MAX_WEAPONS_PER_UNIT)
		return 0;

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	memclear(pWeapons, MAX_WEAPONS_PER_UNIT * sizeof(UNIT *));
#if !ISVERSION(SERVER_VERSION)
	UISetIgnoreStatMsgs(TRUE);
#endif
	pWeapons[nSlot] = WardrobeGetWeapon(UnitGetGame(pUnit), nWardrobe, nSlot);

	if (pWeapons[nSlot] && StatsGetParent(pWeapons[nSlot]) == pUnit)
		UnitInventorySetEquippedWeapons(pUnit, pWeapons);

	int nCritChance = UnitGetStat(pUnit, STATS_CRITICAL_MULT) + UnitGetStat(pUnit, STATS_CRITICAL_MULT_PCT);

	UnitInventorySetEquippedWeapons(pUnit, NULL);
#if !ISVERSION(SERVER_VERSION)
	UISetIgnoreStatMsgs(FALSE);
#endif
	return nCritChance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasDamageRadius(
	UNIT * pUnit)
{
	STATS * pRider = UnitGetRider(pUnit, NULL);
	while(pRider)
	{
		int nRadius = StatsGetStat(pRider, STATS_DAMAGE_RADIUS);
		if(nRadius)
		{
			return TRUE;
		}
		pRider = UnitGetRider(pUnit, pRider);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPlayerLevelSkillPowerCostPercent(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_PLAYERLEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const PLAYERLEVEL_DATA * pPlayerLevel = (PLAYERLEVEL_DATA *) ExcelGetData( NULL, DATATABLE_PLAYERLEVELS, nLevel );
	ASSERT_RETZERO( pPlayerLevel );

	return pPlayerLevel->nPowerCostPercent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemLevelDamageMult(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	return pItemLevel->nDamageMult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemLevelShieldBuffer(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	return pItemLevel->nBaseShieldBuffer;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMObjectCanUpgrade(
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext->game );
	ASSERT_RETZERO( pContext->object );
	if( UnitGetGenus( pContext->object ) != GENUS_ITEM )
		return 0;
	return ItemUpgradeCanUpgrade( pContext->object ) == IFI_UPGRADE_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMAddItemLevelArmor(
	SCRIPT_CONTEXT * pContext,
	int nLevel,
	int nPercentBonus )
{
	ASSERT_RETZERO( pContext->stats );
	ASSERT_RETZERO( pContext->game );
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	int nVal = StatsRoundStatPct( pContext->game, STATS_ARMOR, pItemLevel->nBaseArmor, nPercentBonus );
	StatsSetStat( pContext->game, pContext->stats, STATS_ARMOR, nVal );

	nVal = StatsRoundStatPct( pContext->game, STATS_ARMOR_BUFFER_MAX, pItemLevel->nBaseArmorBuffer, nPercentBonus );
	StatsSetStat( pContext->game, pContext->stats, STATS_ARMOR_BUFFER_MAX, nVal );

	nVal = StatsRoundStatPct( pContext->game, STATS_ARMOR_BUFFER_REGEN, pItemLevel->nBaseArmorRegen, nPercentBonus );
	StatsSetStat( pContext->game, pContext->stats, STATS_ARMOR_BUFFER_REGEN, nVal );
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemLevelFeed(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	return pItemLevel->nBaseFeed;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemLevelSfxAttack(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	return pItemLevel->nBaseSfxAttackAbility;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemLevelSfxDefense(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
	ASSERT_RETZERO( pItemLevel );

	return pItemLevel->nBaseSfxDefenseAbility;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelSfxDefense(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return pMonLevel->nSfxDefense;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelSfxAttack(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return pMonLevel->nSfxAttack;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelDamagePCT(
	int nLevel,
	int nPCT )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return max( 1, PCT( pMonLevel->nDamage, nPCT ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelDamage(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return pMonLevel->nDamage;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelShields(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return pMonLevel->nShield;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterLevelArmor(
	int nLevel )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
	nLevel = PIN( nLevel, 0, nNumRows - 1 );

	const MONSTER_LEVEL * pMonLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
	ASSERT_RETZERO( pMonLevel );

	return pMonLevel->nArmor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetUnitLevelAIChangerAttack(
	UNIT * pUnit)
{
	int nLevel = UnitGetExperienceLevel(pUnit);
	switch(UnitGetGenus(pUnit))
	{
	case GENUS_MONSTER:
		{
			int nNumRows = ExcelGetNumRows( NULL, DATATABLE_MONLEVEL );
			nLevel = PIN( nLevel, 0, nNumRows - 1 );

			const MONSTER_LEVEL * pMonsterLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nLevel );
			ASSERT_RETZERO( pMonsterLevel );

			return pMonsterLevel->nAIChangeAttack;
		}
		break;
	case GENUS_ITEM:
		{
			int nNumRows = ExcelGetNumRows( NULL, DATATABLE_ITEM_LEVELS );
			nLevel = PIN( nLevel, 0, nNumRows - 1 );

			const ITEM_LEVEL * pItemLevel = (ITEM_LEVEL *) ExcelGetData( NULL, DATATABLE_ITEM_LEVELS, nLevel );
			ASSERT_RETZERO( pItemLevel );

			return pItemLevel->nBaseAIChangerAttack;
		}
		break;
	case GENUS_PLAYER:
		{
			int nNumRows = ExcelGetNumRows( NULL, DATATABLE_PLAYERLEVELS );
			nLevel = PIN( nLevel, 0, nNumRows - 1 );

			const PLAYERLEVEL_DATA * pPlayerLevel = (PLAYERLEVEL_DATA *) ExcelGetData( NULL, DATATABLE_PLAYERLEVELS, nLevel );
			ASSERT_RETZERO( pPlayerLevel );

			return pPlayerLevel->nAIChangerAttack;
		}
		break;
	case GENUS_MISSILE:
		{
			UNIT * pUltimateOwner = UnitGetUltimateOwner(pUnit);
			return VMGetUnitLevelAIChangerAttack(pUltimateOwner);
		}
		break;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasContainer(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
	return UnitGetContainer( pUnit ) != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMMonsterArmor(
	UNIT * pUnit,
	int eDamageType,
	int nPercent )
{
	ASSERT_RETZERO( pUnit );
	ASSERT_RETZERO( nPercent > 0 );
	ASSERT_RETZERO( eDamageType != INVALID_ID );

	int nLevel = UnitGetExperienceLevel( pUnit );
	
	GAME * pGame = UnitGetGame( pUnit );
	const MONSTER_LEVEL * pMonLevel = MonsterLevelGetData( pGame, nLevel );
	ASSERT_RETZERO( pMonLevel );

	int armor = PCT(pMonLevel->nArmor << StatsGetShift( pGame, STATS_ARMOR), nPercent );
	int armor_buffer = PCT(pMonLevel->nArmorBuffer << StatsGetShift( pGame, STATS_ARMOR_BUFFER_MAX), nPercent );
	int armor_regen = PCT(pMonLevel->nArmorRegen << StatsGetShift( pGame, STATS_ARMOR_BUFFER_REGEN), nPercent );
	UnitSetStat( pUnit, STATS_ARMOR, eDamageType, armor);
	UnitSetStat( pUnit, STATS_ARMOR_BUFFER_MAX, eDamageType, armor_buffer);
	UnitSetStat( pUnit, STATS_ARMOR_BUFFER_CUR, eDamageType, armor_buffer);
	UnitSetStat( pUnit, STATS_ARMOR_BUFFER_REGEN, eDamageType, armor_regen);

	return 0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetDmgType( SCRIPT_CONTEXT * pContext,
				  int eDamageType )				   
{
	return eDamageType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetProcID( SCRIPT_CONTEXT * pContext,
				 int nProcID )				   
{
	return nProcID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetUnitTypeID( SCRIPT_CONTEXT * pContext,
				int nUnitType )				   
{
	return nUnitType;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetDamageEffectID( SCRIPT_CONTEXT * pContext,
				int nDamageEffectID )				   
{
	return nDamageEffectID;
}

						 
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterHPAtLevel(	
	SCRIPT_CONTEXT * pContext,
	int nLevel )
{
	ASSERTX( nLevel >= 0, "INVALID LEVEL" );
	const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(pContext->game, DATATABLE_MONLEVEL, nLevel);
	ASSERTX( monster_level, "INVALID LEVEL" );
	return monster_level->nHitPoints;
}


int VMObjectIsDestructable(
	SCRIPT_CONTEXT * pContext )
{
	if( pContext->object )
	{
		return UnitIsA( pContext->object, UNITTYPE_DESTRUCTIBLE );
	}
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetMonsterHPAtLevelPCT(	
	SCRIPT_CONTEXT * pContext,
	int nLevel,
	int nPCT )
{
	ASSERTX( nLevel >= 0, "INVALID LEVEL" );
	const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(pContext->game, DATATABLE_MONLEVEL, nLevel);
	ASSERTX( monster_level, "INVALID LEVEL" );
	return PCT( monster_level->nHitPoints, nPCT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillDmgMult(
	SCRIPT_CONTEXT * pContext,
	int nSkillLvl,
	int percent )
{
	ASSERTX( pContext->nSkill != INVALID_ID, "Skill not passed into Skill dmg mult" );
	ASSERTX( pContext->unit, "Unit not passed into Skill Dmg Mult" );
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;

	int add = ( skillData->pnSkillLevelToMinLevel[ 0 ] >= 1 )?skillData->pnSkillLevelToMinLevel[ 0 ] - 1:0; //minus one for the nSkillLvl
	return PCT( SkillGetSkillLevelValue( nSkillLvl + add, KSkillLevel_DMG ), percent );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetArmorDmgMult(
	SCRIPT_CONTEXT * pContext,
	int nSkillLvl,
	int percent )
{
	ASSERTX( pContext->nSkill != INVALID_ID, "Skill not passed into Skill dmg mult" );
	ASSERTX( pContext->unit, "Unit not passed into Skill Dmg Mult" );	
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;
	int add = ( skillData->pnSkillLevelToMinLevel[ 0 ] >= 1 )?skillData->pnSkillLevelToMinLevel[ 0 ] - 1:0; //minus one for the nSkillLvl
	return PCT( SkillGetSkillLevelValue( nSkillLvl + add, KSkillLevel_Armor ), percent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetAttackSpeedDmgMult(
	SCRIPT_CONTEXT * pContext,
	int nSkillLvl,
	int percent )
{
	ASSERTX( pContext->nSkill != INVALID_ID, "Skill not passed into Skill Attack Speed mult" );
	ASSERTX( pContext->unit, "Unit not passed into Skill Attack Speed Mult" );
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;
	int add = ( skillData->pnSkillLevelToMinLevel[ 0 ] >= 1 )?skillData->pnSkillLevelToMinLevel[ 0 ] - 1:0; //minus one for the nSkillLvl
	return PCT( SkillGetSkillLevelValue( nSkillLvl + add, KSkillLevel_AttackSpeed ), percent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetToHitMult(
	SCRIPT_CONTEXT * pContext,
	int nSkillLvl,
	int percent )
{
	ASSERTX( pContext->nSkill != INVALID_ID, "Skill not passed into Skill toHit mult" );
	ASSERTX( pContext->unit, "Unit not passed into Skill toHit Mult" );
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;
	int add = ( skillData->pnSkillLevelToMinLevel[ 0 ] >= 1 )?skillData->pnSkillLevelToMinLevel[ 0 ] - 1:0; //minus one for the nSkillLvl
	return PCT( SkillGetSkillLevelValue( nSkillLvl + add, KSkillLevel_ToHit ), percent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPctDmgMult(
	SCRIPT_CONTEXT * pContext,
	int nSkillLvl,
	int percent )
{
	ASSERTX_RETZERO( pContext->nSkill != INVALID_ID, "Skill not passed into Skill percent damage mult" );
	ASSERTX_RETZERO( pContext->unit, "Unit not passed into Skill percent damage Mult" );
	
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;
	int add = ( skillData->pnSkillLevelToMinLevel[ 0 ] >= 1 )?skillData->pnSkillLevelToMinLevel[ 0 ] - 1:0; //minus one for the nSkillLvl
	return PCT( SkillGetSkillLevelValue( nSkillLvl + add, KSkillLevel_PercentDamage ), percent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetPetCountOfType(
	SCRIPT_CONTEXT * pContext,
	int nUnitType )
{
	ASSERTX_RETZERO( pContext->nSkill != INVALID_ID, "Skill not passed in" );
	ASSERTX_RETZERO( pContext->unit, "Unit not passed");	
	const SKILL_DATA *pSkillData = SkillGetData( pContext->game, pContext->nSkill );		
	if ( ! pSkillData )
		return 0;
	ASSERTX_RETZERO( pSkillData->pnSummonedInventoryLocations[ 0 ] != INVALID_ID, "Pet inventory slot not found" );
	int count(0);
	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if( pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID )
			continue;
		int nInventoryCount = UnitInventoryGetCount(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ] );		
		UNIT * pTarget = UnitInventoryLocationIterate(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(nInventoryCount && pTarget )
		{
			if( UnitIsA( pTarget, nUnitType) )
				count++;
			pTarget = UnitInventoryLocationIterate(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ], pTarget);
		}
	}
	return count;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRunScriptParamOnPetsOfType(
	SCRIPT_CONTEXT * pContext,
	int nUnitType,
	int scriptParam)
{	
	ASSERTX_RETZERO( pContext->nSkill != INVALID_ID, "Skill not passed in" );
	ASSERTX_RETZERO( pContext->unit, "Unit not passed");	
	const SKILL_DATA *pSkillData = SkillGetData( pContext->game, pContext->nSkill );		
	if ( ! pSkillData )
		return 0;
	ASSERTX_RETZERO( pSkillData->pnSummonedInventoryLocations[ 0 ] != INVALID_ID, "Pet inventory slot not found" );
	int count(0);
	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if( pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID )
			continue;
		int nInventoryCount = UnitInventoryGetCount(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ] );		
		UNIT * pTarget = UnitInventoryLocationIterate(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(nInventoryCount && pTarget )
		{
			if( UnitIsA( pTarget, nUnitType ) )
			{
				count++;
				int code_len(0);
				BYTE * code_ptr = GetScriptParam( pContext->game, scriptParam, pSkillData, &code_len );
				int nSkillLevel = SkillGetLevel( pContext->unit, pContext->nSkill );
				ASSERTX_CONTINUE( code_ptr, "NO stats in skill scriptparamX" );	
				VMExecI(pContext->game, pTarget, pContext->unit, NULL, pContext->nSkill, nSkillLevel, pContext->nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
			}
			pTarget = UnitInventoryLocationIterate(pContext->unit, pSkillData->pnSummonedInventoryLocations[ i ], pTarget);
		}
	}
	return count;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetBonusValue(
	SCRIPT_CONTEXT * pContext,
	int nIndex )
{	
	ASSERTX_RETZERO( pContext->unit, "No unit passed in" );
	ASSERTX_RETZERO( nIndex >= 0 && nIndex < MAX_SKILL_BONUS_COUNT, "Index out of range" );
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	if ( ! skillData )
		return 0;
	ASSERTX_RETZERO( skillData, "Invalid skill index" );
	ASSERTX_RETZERO( skillData->tSkillBonus.nSkillBonusBySkills[ nIndex ] != INVALID_ID, "Invalid skill for bonus" );
	int nLevel = SkillGetLevel( UnitGetUltimateOwner( pContext->unit ), skillData->tSkillBonus.nSkillBonusBySkills[ nIndex ] );
	
	return nLevel * SkillGetBonusSkillValue( UnitGetUltimateOwner( pContext->unit ), skillData, nIndex ); 	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetBonusAll(
	SCRIPT_CONTEXT * pContext )
{
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	ASSERTX_RETZERO( skillData, "Invalid skill index" );
	int nValue( 0 );
	for( int t = 0; t < MAX_SKILL_BONUS_COUNT; t++ )
	{
		if( skillData->tSkillBonus.nSkillBonusBySkills[ t ] != INVALID_ID )
		{
			nValue += VMGetBonusValue( pContext, t );
		}
	}
	return nValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetDMGAugmentation(
	SCRIPT_CONTEXT * pContext,
	int nSkillVar,
	int nLevel,
	int nPercentOfLevel,
	int nSkillPointsInvested )
{
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	ASSERTX_RETZERO( skillData, "Invalid skill index" );	
	ASSERTX_RETZERO( skillData->nMaxLevel > 0 , "Invalid Skill Level" );		
	ASSERTX_RETZERO( nLevel >= 0, "Invalid VALUE" );	
	if( nSkillPointsInvested <= 0 )
		nSkillPointsInvested=1;	
	if( nPercentOfLevel < 0 )
		nPercentOfLevel = 100;
	//this returns the value of the varible ( should be damage at max level )
	int varValue = SkillGetVariable( pContext->unit, pContext->object, skillData, (SKILL_VARIABLES)nSkillVar, skillData->nMaxLevel );
	//now lets get the hitpoints of the monster at the level we want
	int monsterHP = VMGetMonsterHPAtLevel( pContext, nLevel );
	//get percent
	monsterHP = PCT( monsterHP, nPercentOfLevel );
	//subtract the amount of damage we are doing...
	varValue = monsterHP - varValue;		
	//devide it
	varValue=(int)CEIL((float)varValue/(float)nSkillPointsInvested);
	return varValue; //yes this could be negative but it'll show up in the UI. Which we then will need to fix.
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetDMGAugmentationPCT(
	SCRIPT_CONTEXT * pContext,
	int nSkillVar,
	int nLevel,
	int nPercentOfLevel,
	int nSkillPointsInvested )
{
	const SKILL_DATA *skillData = SkillGetData( pContext->game, pContext->nSkill );
	ASSERTX_RETZERO( skillData, "Invalid skill index" );	
	ASSERTX_RETZERO( skillData->nMaxLevel > 0 , "Invalid Skill Level" );		
	ASSERTX_RETZERO( nLevel >= 0, "Invalid VALUE" );	
	if( nSkillPointsInvested <= 0 )
		nSkillPointsInvested=1;	
	if( nPercentOfLevel < 0 )
		nPercentOfLevel = 100;
	//this returns the value of the varible ( should be damage at max level )
	int varValue = SkillGetVariable( pContext->unit, pContext->object, skillData, (SKILL_VARIABLES)nSkillVar, skillData->nMaxLevel );
	//now lets get the hitpoints of the monster at the level we want
	int monsterHP = VMGetMonsterHPAtLevel( pContext, nLevel );
	//get percent of HP
	monsterHP = PCT( monsterHP, nPercentOfLevel );
	ASSERTX_RETZERO( monsterHP > 0, "INVALID HP on monster" );
	ASSERTX_RETZERO( varValue > 0, "INVALID VALUE" );
	
	float percentDifferance = 100.0f * ( 1.0f - (float)varValue/(float)monsterHP );
	return (int)CEIL( percentDifferance/(float)nSkillPointsInvested); //yes this could be negative but it'll show up in the UI. Which we then will need to fix.
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMApplyRandomAffixOfType(
	SCRIPT_CONTEXT * context,
	int affixType)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETZERO(context);
	ASSERT_RETZERO(context->unit);
	if (AffixGetAffixCount(context->unit, AF_ALL_FAMILIES) >= AffixGetMaxAffixCount())
	{
		return 0;
	}

	DWORD dwAffixPickFlags = 0;
	SETBIT( dwAffixPickFlags, APF_ALLOW_PROCS_BIT );
	if (s_AffixPick(context->unit, dwAffixPickFlags, affixType, NULL, INVALID_LINK) == INVALID_LINK)
	{
		return 0;
	}

	return 1;
#else
	REF(context);
	REF(affixType);
	return 0;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMApplyRandomAffixOfGroup(
	SCRIPT_CONTEXT * context,
	int affixGroup)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETZERO(context);
	ASSERT_RETZERO(context->unit);
	if (AffixGetAffixCount(context->unit, AF_ALL_FAMILIES) >= AffixGetMaxAffixCount())
	{
		return 0;
	}

	DWORD dwAffixPickFlags = 0;
	SETBIT( dwAffixPickFlags, APF_ALLOW_PROCS_BIT );
	if (s_AffixPick(context->unit, dwAffixPickFlags, INVALID_LINK, NULL, affixGroup) == INVALID_LINK)
	{
		return 0;
	}

	return 1;
#else
	REF(context);
	REF(affixGroup);
	return 0;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMApplyAffix(
	SCRIPT_CONTEXT * context,
	int affix,
	int bForce)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETZERO(context);
	ASSERT_RETZERO(context->unit);
	if (AffixGetAffixCount(context->unit, AF_ALL_FAMILIES) >= AffixGetMaxAffixCount())
	{
		return 0;
	}

	if (affix == INVALID_LINK)
	{
		return 0;
	}

	ITEM_SPEC spec;
	spec.nAffixes[0] = affix;
	SETBIT(spec.dwFlags, ISF_AFFIX_SPEC_ONLY_BIT);

	DWORD dwAffixPickFlags = 0;
	if (bForce == 1)
	{
		SETBIT(dwAffixPickFlags, APF_IGNORE_LEVEL_CHECK);
		SETBIT(dwAffixPickFlags, APF_IGNORE_UNIT_TYPE_CHECK);
		SETBIT(dwAffixPickFlags, APF_IGNORE_CONTAINER_TYPE_CHECK);
		SETBIT(dwAffixPickFlags, APF_IGNORE_EXISTING_GROUP_CHECK);
		SETBIT(dwAffixPickFlags, APF_IGNORE_CONDITION_CODE);
		SETBIT(dwAffixPickFlags, APF_ALLOW_PROCS_BIT);
	}

	if (s_AffixPick(context->unit, dwAffixPickFlags, INVALID_LINK, &spec, INVALID_LINK) == INVALID_LINK)
	{
		return 0;
	}

	return 1;
#else
	REF(context);
	REF(affix);
	REF(bForce);
	return 0;
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDisplayDamageAbsorbed(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);


	// get armor (armor = armor + sfx_defense)
	int armor = UnitGetStat(pUnit, STATS_ARMOR);
	// ARMOR_PCT_BONUS is included in ARMOR due to statsfunc
	//int nArmorPercent = UnitGetStat(pUnit, STATS_ARMOR_PCT_BONUS );
	//armor += PCT(armor, nArmorPercent);

	int nLevel = UnitGetStat(pUnit, STATS_LEVEL);

	const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(NULL, DATATABLE_MONLEVEL, nLevel);
	if (!monster_level)
	{
		return 0;
	}

	int nAttackRating = monster_level->nAttackRating;
	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(pUnit), STATS_ARMOR);
	ASSERT_RETFALSE(stats_data);
	nAttackRating = nAttackRating << stats_data->m_nShift;

	float dmg_reduction_pct = (float)(armor) / (float)(armor + nAttackRating);

	return (int)(dmg_reduction_pct * 100.0f);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMDamagePctByEnergy(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);

	return CombatCalcDamageIncrementByEnergy(pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMWeaponGetRange(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);

	GAME *pGame = UnitGetGame(pUnit);
	ASSERT_RETZERO(IS_CLIENT(pGame));

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return 0;

	int nSkill = ItemGetPrimarySkill( pUnit );
	if (nSkill == INVALID_LINK)
		return 0;

	return (int)SkillGetRange( pPlayer, nSkill, pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGlobalThemeIsEnabled(
	SCRIPT_CONTEXT * context,
	int nTheme )
{
	return GlobalThemeIsEnabled( context->game, nTheme ) ? 1 : 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRemoveHPAndCheckForDeath(
	UNIT * pUnit,
	int nRemove )
{
	ASSERT_RETZERO( pUnit);
	int hpCur = UnitGetStatShift( pUnit, STATS_HP_CUR );
	hpCur -= nRemove;
	if( hpCur <= 0 )
	{
		UnitDie( pUnit, NULL);		
		return 1;
	}
	else
	{
		UnitSetStatShift( pUnit, STATS_HP_CUR, hpCur );
	}
	return 0;
}

typedef struct FIND_OLDEST_MINOR_PET_DATA
{
	int nMonsterType;
	GAME_TICK tOldestSpawnTick;
	UNIT *pOldestPet;
} FIND_OLDEST_MINOR_PET_TO_REMOVE_DATA;

void sFindOldestPetOfType( UNIT *pPet, void *pUserData )
{
	ASSERT_RETURN(pPet && pUserData);
	FIND_OLDEST_MINOR_PET_TO_REMOVE_DATA *pData = (FIND_OLDEST_MINOR_PET_TO_REMOVE_DATA*)pUserData;

	if (UnitIsA(pPet, UNITTYPE_PET) &&
		UnitIsA( pPet, pData->nMonsterType ) )
	{
		GAME_TICK tPetSpawnTick = UnitGetStat(pPet, STATS_SPAWN_TICK);
		if (pData->pOldestPet == NULL || pData->tOldestSpawnTick > tPetSpawnTick)
		{
			pData->pOldestPet = pPet;
			pData->tOldestSpawnTick = tPetSpawnTick;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRemoveOldestPetOfType(
							SCRIPT_CONTEXT * pContext,
							int nUnitType )
{
	ASSERTX_RETZERO( pContext->unit, "Unit not passed");	
	FIND_OLDEST_MINOR_PET_TO_REMOVE_DATA tOldestData;
	tOldestData.nMonsterType = nUnitType;	
	do
	{
		// kill the oldest minor pet of the specified type
		tOldestData.pOldestPet = NULL;
		PetIteratePets(pContext->unit, sFindOldestPetOfType, &tOldestData);
		if (tOldestData.pOldestPet != NULL)
		{
			UnitDie(tOldestData.pOldestPet, NULL);	
			UnitFree(tOldestData.pOldestPet, UFF_SEND_TO_CLIENTS);
			break;
		}
	} while (tOldestData.pOldestPet != NULL );


	return 1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMKillOldestPetOfType( SCRIPT_CONTEXT * pContext,
						   int nUnitType )
{
	ASSERTX_RETZERO( pContext->unit, "Unit not passed");	
	FIND_OLDEST_MINOR_PET_TO_REMOVE_DATA tOldestData;
	tOldestData.nMonsterType = nUnitType;	
	do
	{
		// kill the oldest minor pet of the specified type
		tOldestData.pOldestPet = NULL;
		PetIteratePets(pContext->unit, sFindOldestPetOfType, &tOldestData);
		if (tOldestData.pOldestPet != NULL)
		{
			UnitDie(tOldestData.pOldestPet, NULL);				
			break;
		}
	} while (tOldestData.pOldestPet != NULL );


	return 1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMScaleUnitToInMS( SCRIPT_CONTEXT * pContext,
					  int nScale,
					  int nTimeMS )
{
	ASSERT_RETZERO( pContext->unit );
	float fFinalScale = (float)nScale/100.f;
	float initialValue = UnitGetStatFloat(pContext->unit, STATS_UNIT_SCALE_ORIGINAL, 0);
	int ticks = (int)( (float)nTimeMS/(float)MSECS_PER_SEC * GAME_TICKS_PER_SECOND_FLOAT );
	StateRegisterUnitScale( pContext->unit, 0, initialValue, fFinalScale, ticks, 0 );
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillStat(
	SCRIPT_CONTEXT * context,
	int nSkillStat,
	int nSkillLvl)
{
	ASSERT_RETZERO( nSkillStat != INVALID_ID );	
	if( nSkillLvl <= 0 )
		return 0; //the default skill va
	return GetSkillStat( nSkillStat, context->unit, nSkillLvl );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetItemStatByPct( SCRIPT_CONTEXT * context,
					    int nSkillStat,
						int nPct )
{
	ASSERT_RETZERO( nSkillStat != INVALID_ID );	
	ASSERT_RETZERO( context->unit );	
	ASSERT_RETZERO( UnitGetGenus( context->unit ) == GENUS_ITEM );	
	if( context->unit->pUnitData->nBaseLevel <= 0 )
		return 0; //the default skill va
	return PCT( GetSkillStat( nSkillStat, context->unit, context->unit->pUnitData->nBaseLevel ), nPct );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMTownPortalIsAllowed(
	UNIT * pUnit )
{
	ASSERT_RETZERO( pUnit );
	return TownPortalIsAllowed( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMLowerManaCostOnSkillPct(
	SCRIPT_CONTEXT * context,
	int skillID,
	int nPctPower)
{
	ASSERT_RETZERO( skillID != INVALID_ID );
	ASSERT_RETZERO( context->game );
	ASSERT_RETZERO( context->stats );
	StatsSetStat( context->game, context->stats, STATS_POWER_COST_PCT_SKILL, skillID, nPctPower );

	//ASSERT_RETZERO( context->unit );
	//UnitSetStat( context->unit, STATS_POWER_COST_PCT_SKILL, skillID, nPctPower );
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMLowerCoolDownOnSkillPct(
	SCRIPT_CONTEXT * context,
	int skillID,
	int nPctCooldown)
{
	ASSERT_RETZERO( skillID != INVALID_ID );
	ASSERT_RETZERO( context->game );
	ASSERT_RETZERO( context->stats );
	StatsSetStat( context->game, context->stats, STATS_COOL_DOWN_PCT_SKILL, skillID, nPctCooldown );
	//ASSERT_RETZERO( context->unit );
	//UnitSetStat( context->unit, STATS_COOL_DOWN_PCT_SKILL, skillID, nPctCooldown );
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillIsOn(
	SCRIPT_CONTEXT * context,
	int skillID )
{
	ASSERT_RETZERO( skillID != INVALID_ID );
	ASSERT_RETZERO( context->unit );
	const struct SKILL_IS_ON *pSkillOn = SkillIsOn( context->unit, skillID, INVALID_ID, TRUE );
	return ( pSkillOn )?1:0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillRange( 
	SCRIPT_CONTEXT * context,
	int skillID )
{
	ASSERT_RETZERO( skillID != INVALID_ID );
	ASSERT_RETZERO( context->unit );
	int nSkillLevel = SkillGetLevel(context->unit, skillID );
	ASSERT_RETZERO(nSkillLevel != INVALID_ID );		
	return (int)SkillGetRange( context->unit, skillID, NULL, FALSE, nSkillLevel ) * 100;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillID(
	SCRIPT_CONTEXT * context,
	int skillID )
{
	ASSERT_RETZERO( skillID != INVALID_ID );		
	return skillID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMFireMissileFromObject(
	SCRIPT_CONTEXT * context,
	int missileID )
{
	ASSERT_RETZERO( missileID != INVALID_ID );		
	ASSERT_RETZERO( context->object );
	UNIT * pMissile = MissileFire( context->game, missileID, context->object, NULL,
		INVALID_ID, context->unit, VECTOR(0), UnitGetPosition( context->unit ), UnitGetMoveDirection( context->unit ), 0, 0 );
	return (pMissile)?1:0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int sCaculateGemSockets(
	SCRIPT_CONTEXT * context,
	BOOL bRare )
{	
	ASSERT_RETZERO( context->unit );
	ASSERT_RETZERO( context->game );
	if( UnitGetGenus( context->unit ) != GENUS_ITEM )
	{
		return 1;
	}
	if( ItemIsACraftedItem( context->unit ) )
	{
		if( UnitIsA( context->unit, UNITTYPE_ARMOR ) )
		{
			UnitSetStat(context->unit,  STATS_ITEM_SLOTS_MAX, INVLOC_CRAFTING_ARMOR_SOCKET, 1 );
			UnitSetStat(context->unit, STATS_ITEM_SLOTS, INVLOC_CRAFTING_ARMOR_SOCKET, 1  );		
		}
		else if( UnitIsA( context->unit, UNITTYPE_WEAPON ) )
		{
			UnitSetStat(context->unit,  STATS_ITEM_SLOTS_MAX, INVLOC_CRAFTING_WEAPON_SOCKET, 1 );
			UnitSetStat(context->unit, STATS_ITEM_SLOTS, INVLOC_CRAFTING_WEAPON_SOCKET, 1  );		
		}
		else if( UnitIsA( context->unit, UNITTYPE_TRINKET) )
		{
			UnitSetStat(context->unit,  STATS_ITEM_SLOTS_MAX, INVLOC_CRAFTING_TRINKET_SOCKET, 1 );
			UnitSetStat(context->unit, STATS_ITEM_SLOTS, INVLOC_CRAFTING_TRINKET_SOCKET, 1  );		
		}
		int nSlots = ItemGetModSlots( context->unit );
		REF(nSlots);

	}
	else
	{
		int max_slots = UnitGetStat(context->unit,  STATS_ITEM_SLOTS_MAX, INVLOC_GEMSOCKET);
		if( ( !bRare && (int)RandGetNum( context->game, 0, 1000 ) < CHANCE_FOR_SOCKET ) ||
			( bRare && (int)RandGetNum( context->game, 0, 1000 ) < CHANCE_FOR_RARE_SOCKET ) )
		{
			UnitSetStat(context->unit, STATS_ITEM_SLOTS, INVLOC_GEMSOCKET, RandGetNum( context->game, 0, max_slots )  );
		}
	}
	return 1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCaculateGemSockets(
	SCRIPT_CONTEXT * context )
{	
	return sCaculateGemSockets( context, FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCaculateRareGemSockets(
	SCRIPT_CONTEXT * context )
{	
	return sCaculateGemSockets( context, TRUE );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecuteSkill(
	SCRIPT_CONTEXT * context,
	int nSkillToExecute)
{
	ASSERT_RETZERO( nSkillToExecute != INVALID_ID );
	ASSERT_RETZERO( context->unit );
	DWORD dwFlags = 0;
	if ( IS_SERVER( context->game ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
	int nTargetID = ( context->object )?UnitGetId( context->object ):INVALID_ID;
	VECTOR nTargetPos = ( context->object )?UnitGetPosition( context->object ):VECTOR( 0 );
	if( nTargetID )
	{
		SkillSetTarget( context->unit, nSkillToExecute, INVALID_ID, nTargetID );
	}
	if( SkillStartRequest( context->game, context->unit, nSkillToExecute, nTargetID, nTargetPos, dwFlags, SkillGetNextSkillSeed( context->game ) ) )
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecuteSkillOnObject(
	SCRIPT_CONTEXT * context,
	int nSkillToExecute)
{
	ASSERT_RETZERO( nSkillToExecute != INVALID_ID );
	ASSERT_RETZERO( context->object );
	DWORD dwFlags = 0;
	if ( IS_SERVER( context->game ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
	int nTargetID = ( context->unit )?UnitGetId( context->unit ):INVALID_ID;
	VECTOR nTargetPos = ( context->unit )?UnitGetPosition( context->unit ):VECTOR( 0 );
	if( nTargetID )
	{
		SkillSetTarget( context->object, nSkillToExecute, INVALID_ID, nTargetID );
	}
	if( SkillStartRequest( context->game, context->object, nSkillToExecute, nTargetID, nTargetPos, dwFlags, SkillGetNextSkillSeed( context->game ) ) )
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMStopSkill(
	SCRIPT_CONTEXT * context,
	int nSkillToStop)
{
	ASSERT_RETZERO( nSkillToStop != INVALID_ID );
	ASSERT_RETZERO( context->unit );
	if ( IS_SERVER( context->game ) )
	{
		SkillStop( context->game, context->unit, nSkillToStop, INVALID_ID, TRUE, TRUE );
	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillPowerCost(
	UNIT * pUnit,
	int nSkill)
{
	ASSERT_RETZERO( nSkill != INVALID_ID );
	ASSERT_RETZERO( pUnit );
	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), nSkill);
	if(!pSkillData)
	{
		return 0;
	}
	int nSkillLevel = SkillGetLevel(pUnit, nSkill);
	ASSERT_RETZERO(nSkillLevel > 0);
	return SkillGetPowerCost(pUnit, pSkillData, nSkillLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSkillPowerCostAtLevel(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel)
{
	ASSERT_RETZERO( nSkill != INVALID_ID );
	ASSERT_RETZERO( pUnit );
	const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), nSkill);
	if(!pSkillData)
	{
		return 0;
	}
	ASSERT_RETZERO(nSkillLevel > 0);
	return SkillGetPowerCost(pUnit, pSkillData, nSkillLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsStashUIOpen()
{	
#if !ISVERSION(SERVER_VERSION)

	return UIIsStashScreenUp();
#else
	return FALSE;
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetCurrentGameTick(
	SCRIPT_CONTEXT * context)
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->game );

	return context->game->tiGameTick;
}

int VMSetRecipeLearned(
	SCRIPT_CONTEXT * context )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->game );
	if ( IS_CLIENT( context->game ) )
		return 0;
	ASSERT_RETZERO( context->unit );	//item/player
	ASSERT_RETZERO( context->object );	//item/player
	UNIT *pPlayer = ( UnitGetGenus( context->unit ) == GENUS_PLAYER )?context->unit:context->object;
	UNIT *pItem = ( UnitGetGenus( context->unit ) == GENUS_ITEM )?context->unit:context->object;	
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pItem );	
	int nRecipeID = ItemGetRecipeID( pItem );
	ASSERT_RETZERO( nRecipeID != INVALID_ID );
	PlayerLearnRecipe( pPlayer, nRecipeID );	
	return 1;
}


int VMGetRecipeCategoryLevel(
	SCRIPT_CONTEXT * context,
	int				 nCategory )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->unit );
	ASSERT_RETZERO( nCategory != INVALID_ID );
	return RecipeGetCategoryLevel( context->unit, nCategory, context->nParam1, context->nParam2, context->nSkill, context->nSkillLevel );	
}

int VMGetRecipeLearned(
	SCRIPT_CONTEXT * context )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->game );
	if ( IS_CLIENT( context->game ) )
		return 0;
	ASSERT_RETZERO( context->unit );	//item/player
	ASSERT_RETZERO( context->object );	//item/player
	UNIT *pPlayer = ( UnitGetGenus( context->unit ) == GENUS_PLAYER )?context->unit:context->object;
	UNIT *pItem = ( UnitGetGenus( context->unit ) == GENUS_ITEM )?context->unit:context->object;	
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pItem );	
	int nRecipeID = ItemGetRecipeID( pItem );
	ASSERT_RETZERO( nRecipeID != INVALID_ID );
	return ( UnitGetStat( pPlayer, STATS_RECIPES_LEARNED, nRecipeID ) == 1 )?TRUE:FALSE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCreateRecipe(
	SCRIPT_CONTEXT * context )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->unit );	//item
	ASSERT_RETZERO( context->object );	//player/merchant
	ASSERT_RETZERO( context->game );
	if ( IS_CLIENT( context->game ) )
		return 0;
	return s_RecipeAssignRandomRecipeID( context->object, context->unit )?1:0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCreateSpecificRecipe(
	SCRIPT_CONTEXT * context,
	int nRecipeID )
{
	ASSERT_RETZERO( nRecipeID != INVALID_ID );
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->unit );	//item
	ASSERT_RETZERO( context->game );
	if ( IS_CLIENT( context->game ) )
		return 0;
	return s_RecipeAssignSpecificRecipeID( context->unit, nRecipeID )?1:0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillMaxLevel( SCRIPT_CONTEXT * context )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->nSkill != INVALID_ID );	//item
	const SKILL_DATA *pSkillData = SkillGetData( context->game, context->nSkill );
	if( pSkillData )
	{
		return pSkillData->nMaxLevel;
	}
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGiveAllRecipes( SCRIPT_CONTEXT * context )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->unit );
	int nCount = (int)ExcelGetNumRows(context->game, DATATABLE_RECIPES);
	for( int t = 0; t < nCount; t++ )
	{
		const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( t );
		if( pRecipeDefinition )
		{
			if( pRecipeDefinition->nWeight > 0 &&
				pRecipeDefinition->nRecipeCategoriesNecessaryToCraft[0] != INVALID_ID )
			{
				PlayerLearnRecipe( context->unit, t );					
			}
		}
	}

	return 1;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillPctInvested( SCRIPT_CONTEXT * context,
						   int nSkillLevel )
{
	ASSERT_RETZERO( context );
	ASSERT_RETZERO( context->nSkill != INVALID_ID );	//item
	if( nSkillLevel <= 0 )
		return 0;
	const SKILL_DATA *pSkillData = SkillGetData( context->game, context->nSkill );
	if( pSkillData )
	{
		return (int)CEIL( 100.0f * ((float)nSkillLevel/(float)pSkillData->nMaxLevel ) );
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMHasA( SCRIPT_CONTEXT * pContext,
            int nUnitType )				   
{
	ASSERT_RETZERO( pContext );
	UNIT *pPlayer = pContext->unit;
	ASSERT_RETZERO( pPlayer );

	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate(pPlayer, pItem)) != NULL)
	{
		if (pItem->nUnitType == nUnitType)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSameGameVariant( SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext );
	UNIT *pPlayer = pContext->unit;
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( UnitIsPlayer(pPlayer) );
	UNIT *pItem = pContext->object;
	if (!pItem)
	{
		return TRUE;
	}

	DWORD dwPlayerVariant = GameVariantFlagsGetStaticUnit(pPlayer);
	DWORD dwItemVariant = UnitGetStat(pItem, STATS_GAME_VARIANT);

	ASSERT_RETTRUE(dwItemVariant != -1);

	return dwPlayerVariant == dwItemVariant;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPlayerIsInGuild( SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext );
	UNIT *pPlayer = pContext->unit;
	ASSERT_RETZERO( pPlayer );

	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	WCHAR szGuildInfo[256] = L"";
	GUILD_RANK eGuildRank = GUILD_RANK_INVALID;
	UnitGetPlayerGuildAssociationTag(pPlayer, szGuildInfo, arrsize(szGuildInfo), eGuildRank);

	return szGuildInfo[0] != 0;
}

//----------------------------------------------------------------------------
//Adds the anchor makers to the unit's level
//----------------------------------------------------------------------------
int VMAddToAnchorMakers( SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext );
	if( IS_CLIENT( pContext->game ) )
		return 1;	//only server
	UNIT *pUnit = pContext->unit;
	ASSERT_RETZERO( pUnit );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ASSERT_RETZERO( pLevel );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	VECTOR vPosition = UnitGetPosition( pUnit );	
	int nExcelIndex = ExcelGetLineByCode( EXCEL_CONTEXT( pContext->game ), DATATABLE_OBJECTS,  pUnitData->wCode );
	LevelGetAnchorMarkers( pLevel )->AddAnchor( pContext->game, nExcelIndex, vPosition.fX, vPosition.fY, pUnit );
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMItemBelongsToGambler( 
	SCRIPT_CONTEXT * pContext )
{
	ASSERT_RETZERO( pContext );
	UNIT *pItem = pContext->unit;
	ASSERT_RETZERO( pItem );

	if (!UnitIsA( pItem, UNITTYPE_ITEM ))
	{
		return FALSE;
	}

	if (!ItemBelongsToAnyMerchant(pItem))
	{
		return FALSE;
	}

	UNIT *pMerchant = ItemGetMerchant(pItem);
	if (!pMerchant)
	{
		return FALSE;
	}

	return UnitIsGambler(pMerchant);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMReevaluateDefense(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(!AppIsHellgate())
		return 0;

	GAME * pGame = UnitGetGame(pUnit);
	ITEM_INIT_CONTEXT tContext(pGame, pUnit, NULL, UnitGetClass(pUnit), UnitGetData(pUnit), NULL);
	tContext.level = UnitGetExperienceLevel(pUnit);
	tContext.item_level = ItemLevelGetData(pGame, tContext.level);
	s_ItemInitPropertiesSetDefenseStats(tContext);
#endif
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPlayerLevelAttackRating(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETZERO(pUnitData);

	int nExperienceLevel = UnitGetExperienceLevel(pUnit);
	int nAttackRatingTotal = 0;
	for(int i=0; i<=nExperienceLevel; i++)
	{
		const PLAYERLEVEL_DATA * pPlayerLevelData = PlayerLevelGetData(UnitGetGame(pUnit), pUnitData->nUnitTypeForLeveling, i);
		if(pPlayerLevelData)
		{
			nAttackRatingTotal += pPlayerLevelData->nAttackRating;
		}
	}
	return nAttackRatingTotal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPetLevelAttackRating(
	UNIT * pUnit,
	int nLevel)
{
	ASSERT_RETZERO(pUnit);
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETZERO(pUnitData);
	const MONSTER_LEVEL * pPetLevelData = (MONSTER_LEVEL *)ExcelGetData(UnitGetGame(pUnit), DATATABLE_PETLEVEL, nLevel);
	ASSERT_RETZERO(pPetLevelData);

	return PCT(pPetLevelData->nAttackRating, pUnitData->nAttackRatingPct);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMPetReevaluateDefense(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(!AppIsHellgate())
		return 0;

	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETZERO(pUnitData);

	const MONSTER_LEVEL * pPetLevelData = (MONSTER_LEVEL *)ExcelGetData(UnitGetGame(pUnit), DATATABLE_PETLEVEL, UnitGetExperienceLevel(pUnit));
	ASSERT_RETZERO(pPetLevelData);

	GAME * pGame = UnitGetGame(pUnit);
	for (int ii = 0; ii < NUM_DAMAGE_TYPES; ii++)
	{
		DAMAGE_TYPES eDamageType = (DAMAGE_TYPES)ii;
		if (pUnitData->fArmor[eDamageType] > 0.0f)
		{
			int armor = PCT(pPetLevelData->nArmor << StatsGetShift(pGame, STATS_ARMOR), pUnitData->fArmor[eDamageType]);
			int armor_buffer = PCT(pPetLevelData->nArmorBuffer << StatsGetShift(pGame, STATS_ARMOR_BUFFER_MAX), pUnitData->fArmor[eDamageType]);
			int armor_regen = PCT(pPetLevelData->nArmorRegen << StatsGetShift(pGame, STATS_ARMOR_BUFFER_REGEN), pUnitData->fArmor[eDamageType]);
			UnitSetStat(pUnit, STATS_ARMOR, eDamageType, armor);
			UnitSetStat(pUnit, STATS_ARMOR_BUFFER_MAX, eDamageType, armor_buffer);
			UnitSetStat(pUnit, STATS_ARMOR_BUFFER_CUR, eDamageType, armor_buffer);
			UnitSetStat(pUnit, STATS_ARMOR_BUFFER_REGEN, eDamageType, armor_regen);
		} 
		else 
		{
			UnitSetStat(pUnit, STATS_ELEMENTAL_VULNERABILITY, eDamageType, int(pUnitData->fArmor[eDamageType]));
		}

		// shields
		if ( pUnitData->fShields[eDamageType] > 0)
		{
			int shield = PCT(pPetLevelData->nShield << StatsGetShift(pGame, STATS_SHIELDS), pUnitData->fShields[eDamageType]);
			int shield_buffer = PCT(pPetLevelData->nShield << StatsGetShift(pGame, STATS_SHIELD_BUFFER_MAX), pUnitData->fShields[eDamageType]);
			int shield_regen = pPetLevelData->nShieldRegen << StatsGetShift(pGame, STATS_SHIELD_BUFFER_REGEN);
			UnitSetStat(pUnit, STATS_SHIELDS, shield);
			UnitSetStat(pUnit, STATS_SHIELD_BUFFER_MAX, shield_buffer);
			UnitSetStat(pUnit, STATS_SHIELD_BUFFER_CUR, shield_buffer);
			UnitSetStat(pUnit, STATS_SHIELD_BUFFER_REGEN, shield_regen);
		}
	}
#endif
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMReevaluateFeed(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(!AppIsHellgate())
		return 0;

	GAME * pGame = UnitGetGame(pUnit);
	ITEM_INIT_CONTEXT tContext(pGame, pUnit, NULL, UnitGetClass(pUnit), UnitGetData(pUnit), NULL);
	tContext.level = UnitGetExperienceLevel(pUnit);
	tContext.item_level = ItemLevelGetData(pGame, tContext.level);
	s_ItemInitPropertiesSetPerLevelStats(tContext);
#endif
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMStatGetShift(
	GAME * pGame,
	int stat )
{
	return StatsGetShift(pGame, stat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCombatHasSecondaryAttacks(
	GAME * pGame)
{
	if(!IS_SERVER(pGame))
	{
		return FALSE;
	}

	struct COMBAT * pCombat = GamePeekCombat(pGame);
	if(!pCombat)
	{
		return FALSE;
	}

	return s_CombatHasSecondaryAttacks(pCombat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCombatIsPrimaryAttack(
	GAME * pGame)
{
	if(!IS_SERVER(pGame))
	{
		return FALSE;
	}

	struct COMBAT * pCombat = GamePeekCombat(pGame);
	if(!pCombat)
	{
		return FALSE;
	}

	return s_CombatIsPrimaryAttack(pCombat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetSkillScriptParam(
	GAME * pGame,
	UNIT * pUnit,
	int nState)
{
	ASSERT_RETZERO(pUnit);

	int nActualState = UnitGetFirstStateOfType(pGame, pUnit, nState);
	if(nActualState == INVALID_ID)
	{
		return 0;
	}

	const STATE_DATA * pStateData = StateGetData(pGame, nActualState);
	ASSERT_RETZERO(pStateData);

	return pStateData->nSkillScriptParam;
}
