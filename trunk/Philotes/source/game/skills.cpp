//----------------------------------------------------------------------------
// skills.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "console.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "excel_private.h"
#include "boundingbox.h"
#include "level.h"
#include "skills.h"
#include "skills_priv.h"
#include "district.h"
#include "ai.h"
#include "weapons.h"
#include "gameunits.h"
#include "unit_priv.h"
#include "missiles.h"
#include "s_message.h"
#include "maths.h"
#include "combat.h"
#include "inventory.h"
#include "items.h"
#include "clients.h"
#include "c_message.h"
#include "movement.h"
#include "c_particles.h"
#include "c_appearance.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "c_camera.h"
#include "states.h"
#include "weapons_priv.h"
#include "unitmodes.h"
#include "monsters.h"
#include "proc.h"
#include "c_sound.h"
#include "uix.h"
#include "uix_priv.h"
#include "ai_priv.h"
#include "teams.h"
#include "unittag.h"
#include "filepaths.h"
#include "script.h"
#include "e_model.h"
#include "treasure.h"
#include "spawnclass.h"
#include "windowsmessages.h"
#include "quests.h"
#include "c_animation.h"
#include "player.h"
#include "pets.h"
#include "crc.h"
#include "waypoint.h"
#include "skills_shift.h"
#include "colors.h"
#include "objects.h"
#include "room_path.h"
#include "wardrobe_priv.h"
#include "c_sound_util.h"
#include "c_ui.h"
#include "achievements.h"
#include "stringtable.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
BOOL gbDoSkillCooldown = TRUE;
BOOL gInfinitePower = FALSE;
#endif
#define LEFT_SWING_MULTIPLIER	.7f	//tugboat
#define RIGHT_SWING_MULTIPLIER	.9f //tugboat


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct SKILL_REQUEST
{
	int nSkill;
	int nRequestedSkill;
	DWORD dwSeed;
};

struct SKILL_IS_ON
{
	int nSkill;
	int nRequestedSkill;
	UNITID idWeapon;
	DWORD dwSeed;
	GAME_TICK tiStartTick;
	DWORD dwFlags;
};

struct SKILL_TARGET
{
	int		nSkill;
	UNITID	idWeapon;
	UNITID	idTarget;
	int		nIndex;
	DWORD	dwFlags;
};

struct SKILL_INFO
{
	SIMPLE_DYNAMIC_ARRAY<SKILL_REQUEST> pRequests;
	SIMPLE_DYNAMIC_ARRAY<SKILL_IS_ON>	pSkillsOn; 
	SIMPLE_DYNAMIC_ARRAY<SKILL_TARGET>	pTargets;
};

struct SKILL_CONTEXT
{
	GAME *								game;
	UNIT *								unit;
	UNIT *								ultimateOwner;
	int									skill;
	int									skillLevel;
	const SKILL_DATA *					skillData;
	int									skillRequested;
	int									skillRequestedLevel;
	UNITID								idWeapon;
	UNIT *								weapon;
	DWORD								dwFlags;
	DWORD								dwSeed;

	SKILL_CONTEXT(GAME * in_game, UNIT * in_unit, int in_skill, int in_skillLevel, int in_skillRequested, int in_skillRequestedLevel, UNITID in_idWeapon, DWORD in_dwFlags, DWORD in_dwSeed) :
		game(in_game), unit(in_unit), ultimateOwner(NULL), skill(in_skill), skillLevel(in_skillLevel), skillData(NULL), skillRequested(in_skillRequested), skillRequestedLevel(in_skillRequestedLevel), 
			idWeapon(in_idWeapon), weapon(NULL), dwFlags(in_dwFlags), dwSeed(in_dwSeed) { skillData = SkillGetData(game, skill); };

	SKILL_CONTEXT(GAME * in_game, UNIT * in_unit, int in_skill, int in_skillLevel, UNITID in_idWeapon, DWORD in_dwFlags, DWORD in_dwSeed) :
		game(in_game), unit(in_unit), ultimateOwner(NULL), skill(in_skill), skillLevel(in_skillLevel), skillData(NULL), skillRequested(in_skill), skillRequestedLevel(in_skillLevel), 
			idWeapon(in_idWeapon), weapon(NULL), dwFlags(in_dwFlags), dwSeed(in_dwSeed) { skillData = SkillGetData(game, skill); };

	SKILL_CONTEXT(GAME * in_game, UNIT * in_unit, int in_skill, int in_skillLevel, const SKILL_DATA * in_skillData, UNITID in_idWeapon, DWORD in_dwFlags, DWORD in_dwSeed) :
		game(in_game), unit(in_unit), ultimateOwner(NULL), skill(in_skill), skillLevel(in_skillLevel), skillData(in_skillData), skillRequested(in_skill), skillRequestedLevel(in_skillLevel), 
			idWeapon(in_idWeapon), weapon(NULL), dwFlags(in_dwFlags), dwSeed(in_dwSeed) { if (!skillData) skillData = SkillGetData(game, skill); };

	SKILL_CONTEXT(GAME * in_game, UNIT * in_unit, int in_skill, int in_skillLevel, const SKILL_DATA * in_skillData, UNIT * in_pWeapon, DWORD in_dwFlags, DWORD in_dwSeed) :
		game(in_game), unit(in_unit), ultimateOwner(NULL), skill(in_skill), skillLevel(in_skillLevel), skillData(in_skillData), skillRequested(in_skill), skillRequestedLevel(in_skillLevel), 
			idWeapon(INVALID_ID), weapon(in_pWeapon), dwFlags(in_dwFlags), dwSeed(in_dwSeed) { if (weapon) idWeapon = UnitGetId(weapon); if (!skillData) skillData = SkillGetData(game, skill); };
};

#define LEAP_IMPACT_FORCE	30


//tugboat added
static BOOL sSkillEventAddMuzzleFlash( 
	const SKILL_EVENT_FUNCTION_DATA & tData );

static BOOL sSkillEventResurrect(
	const SKILL_EVENT_FUNCTION_DATA & tData );   //Marsh added - for resurrecting creatures.
//end tugboat added

static BOOL sSkillExecute(
	SKILL_CONTEXT & context);

static BOOL sSkillStart(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	int nRequestedSkill,
	UNITID idWeapon,
	DWORD dwStartFlags,
	DWORD dwRandSeed,
	int nSkillLevel = 0);

static BOOL sUpdateActiveSkills(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData);

static BOOL sHandleSkillRepeat( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData);

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelSkillsFuncPostProcess(
	struct EXCEL_TABLE * table)
{
	REF(table);
	// This is to determine the frames needed to draw the dependencies on the skills UI page.
	//   It only needs to be run every now and then so it's just commented out, but
	//   please don't delete 'cause we're going to need it again.
	// #define RUN_SKILLS_FUNC_POSTPROCESS
#ifdef RUN_SKILLS_FUNC_POSTPROCESS
	static const int MAX_DIST = 20;
	static const int ARRSIZE = (MAX_DIST*2)+1;
	int nDistances[ARRSIZE][ARRSIZE];
	memclear(&nDistances, ARRSIZE * ARRSIZE * sizeof(int));

	// record all of the distances of dependencies on the skill tree
	for (unsigned int iSkill = 0; iSkill < ExcelGetCountPrivate(table); iSkill++)
	{
		const SKILL_DATA * pSkillData = SkillGetDataPrivate(table, iSkill);
		for( int iReq = 0; iReq < MAX_PREREQUISITE_SKILLS; iReq++ )
		{
			if( pSkillData->pnRequiredSkills[iReq] != INVALID_ID &&
				pSkillData->pnRequiredSkillsLevels[iReq] > 0 )
			{
				const SKILL_DATA * pReqSkillData = SkillGetDataPrivate(table, pSkillData->pnRequiredSkills[iReq]);	
				ASSERTX_CONTINUE(pReqSkillData->nSkillTab == pSkillData->nSkillTab, "A SKill's requirement is on a different UI tab");

				int nDistX = pSkillData->nSkillPageX - pReqSkillData->nSkillPageX;
				ASSERTX_CONTINUE(abs(nDistX) < MAX_DIST, "Skill too far away from its requirement");
				int nDistY = pSkillData->nSkillPageY - pReqSkillData->nSkillPageY;
				ASSERTX_CONTINUE(abs(nDistY) < MAX_DIST, "Skill too far away from its requirement");
				ASSERTX_CONTINUE(nDistX != 0 || nDistY != 0, "Skill is in the same place as its requirement");

				nDistances[nDistX + MAX_DIST][nDistY + MAX_DIST]++;
			}
		}
	}

	for (int x = 0; x < ARRSIZE; x++)
	{
		for (int y = 0; y < ARRSIZE; y++)
		{
			if (nDistances[x][y] > 0)
			{
				trace( "x = %2d y = %2d number = %d\n", x-MAX_DIST, y-MAX_DIST, nDistances[x][y]);
			}
		}

	}
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sWeaponUsesFiringError(
	UNIT * pWeapon )
{
	return pWeapon ? UnitGetStat( pWeapon, STATS_FIRING_ERROR_INCREASE_SOURCE_WEAPON ) != 0 : FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUsesEnergy(
	UNIT * pWeapon )
{
	return pWeapon ? UnitGetStat( pWeapon, STATS_ENERGY_MAX ) != 0 : FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsInitSkillInfo(
	UNIT * unit)
{
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
	unit->pSkillInfo = (SKILL_INFO *)GMALLOC(game, sizeof(SKILL_INFO));
	MEMORYPOOL * pool = GameGetMemPool(game);
	ArrayInit(unit->pSkillInfo->pSkillsOn, pool, 1);
	ArrayInit(unit->pSkillInfo->pRequests, pool, 3);
	ArrayInit(unit->pSkillInfo->pTargets, pool, 3);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsFreeSkillInfo(
	UNIT * pUnit )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );

	pUnit->pSkillInfo->pSkillsOn.Destroy();
	pUnit->pSkillInfo->pRequests.Destroy();
	pUnit->pSkillInfo->pTargets.Destroy();

	GFREE( pGame, pUnit->pSkillInfo );
	pUnit->pSkillInfo = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillsGetSkillInfoArrayLength(
	UNIT * pUnit,
	SKILLS_INFO_ARRAY eArray)
{
	if(!pUnit || !pUnit->pSkillInfo)
		return 0;
	SKILL_INFO * pSkillInfo = pUnit->pSkillInfo;

	switch(eArray)
	{
	case SIA_REQUEST:
		return pSkillInfo->pRequests.Count();
	case SIA_IS_ON:
		return pSkillInfo->pSkillsOn.Count();
	case SIA_TARGETS:
		return pSkillInfo->pTargets.Count();
	default:
		return 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsGetSkillInfoItemText(
	UNIT * pUnit,
	SKILLS_INFO_ARRAY eArray,
	unsigned int nIndex,
	int nTextLength,
	WCHAR * pwszText)
{
	if(!pUnit || !pUnit->pSkillInfo)
		return;
	SKILL_INFO * pSkillInfo = pUnit->pSkillInfo;

	switch(eArray)
	{
	case SIA_REQUEST:
		{
			ASSERT_RETURN(nIndex < pSkillInfo->pRequests.Count());
			const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pSkillInfo->pRequests[nIndex].nSkill);
			PStrPrintf(pwszText, nTextLength, L"Skill %d [%S] Seed %u",
				pSkillInfo->pRequests[nIndex].nSkill, pSkillData->szName, pSkillInfo->pRequests[nIndex].dwSeed);
			return;
		}
		break;
	case SIA_IS_ON:
		{
			ASSERT_RETURN(nIndex < pSkillInfo->pSkillsOn.Count());
			const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pSkillInfo->pSkillsOn[nIndex].nSkill);
			UNIT * pWeapon = UnitGetById(UnitGetGame(pUnit), pSkillInfo->pSkillsOn[nIndex].idWeapon);
			PStrPrintf(pwszText, nTextLength, L"Skill %d [%S] Weapon %d [%S] Seed %u",
				pSkillInfo->pSkillsOn[nIndex].nSkill, pSkillData->szName,
				pSkillInfo->pSkillsOn[nIndex].idWeapon, pWeapon ? UnitGetDevName(pWeapon) : "",
				pSkillInfo->pSkillsOn[nIndex].dwSeed);
			return;
		}
		break;
	case SIA_TARGETS:
		{
			ASSERT_RETURN(nIndex < pSkillInfo->pTargets.Count());
			const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), pSkillInfo->pTargets[nIndex].nSkill);
			UNIT * pWeapon = UnitGetById(UnitGetGame(pUnit), pSkillInfo->pTargets[nIndex].idWeapon);
			UNIT * pTarget = UnitGetById(UnitGetGame(pUnit), pSkillInfo->pTargets[nIndex].idTarget);
			PStrPrintf(pwszText, nTextLength, L"Skill %d [%S] Weapon %d [%S] Target %d [%S]",
				pSkillInfo->pTargets[nIndex].nSkill, pSkillData->szName,
				pSkillInfo->pTargets[nIndex].idWeapon, pWeapon ? UnitGetDevName(pWeapon) : "",
				pSkillInfo->pTargets[nIndex].idTarget, pTarget ? UnitGetDevName(pTarget) : "");
			return;
		}
		break;
	default:
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillIsOn(
	SKILL_IS_ON * pSkillIsOn,
	int nCount,
	int nSkill,
	UNITID idWeapon,
	BOOL bIgnoreWeapon)
{
	if(nCount <= 0 || !pSkillIsOn)
	{
		return INVALID_ID;
	}
	for ( int i = 0; i < nCount; i++, pSkillIsOn++ )
	{
		if ( pSkillIsOn->nSkill == nSkill &&
			(bIgnoreWeapon || pSkillIsOn->idWeapon == idWeapon) )
			return i;
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SKILL_IS_ON * SkillIsOn( 
	UNIT * pUnit, 
	int nSkill,
	UNITID idWeapon,
	BOOL bIgnoreWeapon )
{
	ASSERT_RETNULL( pUnit );
	ASSERT_RETNULL( pUnit->pSkillInfo );
	int nCount = pUnit->pSkillInfo->pSkillsOn.Count();
	SKILL_IS_ON * pSkillIsOn = nCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;
	int nIndex = sSkillIsOn(pSkillIsOn, nCount, nSkill, idWeapon, bIgnoreWeapon);
	if(nIndex >= 0)
	{
		return pUnit->pSkillInfo->pSkillsOn.GetPointer( nIndex );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsOnWithFlag(
	UNIT * pUnit,
	int nSkillFlag )
{
	ASSERT_RETFALSE( pUnit );
	SKILL_INFO * pSkillInfo = pUnit->pSkillInfo;
	ASSERT_RETFALSE(pSkillInfo);

	int nCount = pSkillInfo->pSkillsOn.Count();
	SKILL_IS_ON * pSkillIsOn = nCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pSkillIsOn++ )
	{
		const SKILL_DATA * pSkillData = SkillGetData( NULL, pSkillIsOn->nSkill );
		if ( SkillDataTestFlag( pSkillData, nSkillFlag ) )
			return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsGetSkillsOn(
	UNIT * pUnit, 
	int *pnSkills,
	int nBufLen)
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );
	ASSERT_RETURN( nBufLen > 0 );

	memset(pnSkills, INVALID_LINK, sizeof(int) * nBufLen );
	int nCount = pUnit->pSkillInfo->pSkillsOn.Count();
	nCount = MIN(nCount, nBufLen);
	if (nCount <= 0)
		return;

	SKILL_IS_ON * pSkillIsOn = pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 );
	for ( int i = 0; i < nCount; i++, pSkillIsOn++ )
	{
		pnSkills[i] = pSkillIsOn->nSkill;
	}
};

//----------------------------------------------------------------------------
void SkillClearEventHandler(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon,
	UNIT_EVENT eEvent )
{
#define MAX_EVENT_HANDLERS_PER_UNIT 10
	UNIT * pUnitWithId = pWeapon ? pWeapon : pUnit;
	if ( UnitHasEventHandler( pGame, pUnit, eEvent ) )
	{
		UNIT_ITERATE_STATS_RANGE( pUnitWithId, STATS_SKILL_EVENT_ID, pStatsEntry, jj, MAX_EVENT_HANDLERS_PER_UNIT )
		{
			DWORD dwEventId = pStatsEntry[ jj ].value;
			if ( dwEventId != INVALID_ID && UnitUnregisterEventHandler( pGame, pUnit, eEvent, dwEventId ) )
			{
				UnitSetStat( pUnitWithId, STATS_SKILL_EVENT_ID, pStatsEntry[ jj ].GetParam(), INVALID_ID );
			}
		}
		UNIT_ITERATE_STATS_END;

	}
}

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
static BOOL sSkillsInitFixupMeleeFlag(
	GAME* pGame,
	SKILL_DATA * pSkillData,
	SKILL_EVENT * pSkillEvent,
	SKILL_EVENT_TYPE_DATA * pSkillEventTypeData )
{

	if ( SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_MELEE) ||
		 ( AppIsTugboat() && 
           SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_RANGED) ||
		   SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_SPELL) ) )
	{
		return TRUE;
	}
	if ( AppIsTugboat() && pSkillEventTypeData->bIsSpell )
	{
		SkillDataSetFlag( pSkillData, SKILL_FLAG_IS_SPELL, TRUE );
	}
	if (pSkillEventTypeData->bIsMelee)
	{
		SkillDataSetFlag( pSkillData, SKILL_FLAG_IS_MELEE, TRUE );
		SETBIT( pSkillData->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
		return TRUE;
	}
	else
	{
		if( AppIsTugboat() && pSkillEventTypeData->bIsRanged)
		{
			SkillDataSetFlag( pSkillData, SKILL_FLAG_IS_RANGED, TRUE );
		}
		if (pSkillEventTypeData->bSubSkillInherit && pSkillEventTypeData->eAttachedTable == DATATABLE_SKILLS)
		{
			SKILL_DATA * pSubSkillData = (SKILL_DATA *)ExcelGetDataByStringIndex(pGame, DATATABLE_SKILLS, pSkillEvent->tAttachmentDef.pszAttached);
			ASSERT_RETFALSE( pSubSkillData );

			if ( pSubSkillData->szEvents[ 0 ] != 0 )
			{
				pSubSkillData->nEventsId = DefinitionGetIdByName( DEFINITION_GROUP_SKILL_EVENTS, pSubSkillData->szEvents );
			}
			else
			{
				pSubSkillData->nEventsId = INVALID_ID;
			}

			if ( pSubSkillData->nEventsId == INVALID_ID )
			{
				return FALSE;
			}

			SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSubSkillData->nEventsId );
			if ( !pEvents || !pEvents->pEventHolders )
			{
				return FALSE;
			}

			for( int i = 0; i < pEvents->nEventHolderCount; i++ )
			{
				SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
				for( int j = 0; j < pHolder->nEventCount; j++ )
				{
					SKILL_EVENT * pSubSkillEvent = &pHolder->pEvents[ j ];
					SKILL_EVENT_TYPE_DATA * pSubSkillEventTypeData = SkillGetEventTypeData( pGame, pSubSkillEvent->nType );
					ASSERT_CONTINUE(pSubSkillEventTypeData);
					if ( pSubSkillEvent == pSkillEvent )
						continue;
					if (sSkillsInitFixupMeleeFlag(pGame, pSkillData, pSubSkillEvent, pSubSkillEventTypeData))
					{
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

#if   !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitBeginTrackingMeleeBones( 
	GAME * pGame, 
	UNIT * pUnit )
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );

	int nModelId = c_UnitGetModelIdThirdPerson( pUnit );
	if ( nModelId == INVALID_ID )
		return;

	int nAppearanceDef = UnitGetAppearanceDefId( pUnit, TRUE );
	if ( nAppearanceDef == INVALID_ID )
		return;

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		int nBoneIndex = c_WeaponGetAttachmentBoneIndex( nAppearanceDef, i );
		if ( nBoneIndex == INVALID_ID )
			continue;

		c_AppearanceTrackBone( nModelId, nBoneIndex );
	}
}
#endif   !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillGetSkillLevel(
	UNIT * unit,
	int skill,
	int skillLevel)
{
	if (skillLevel > 0)
	{
		return skillLevel;
	}
	return SkillGetLevel(unit, skill);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillGetSkillLevel(
	SKILL_CONTEXT & context)
{
	if (context.skillLevel <= 0)
	{
		context.skillLevel = SkillGetLevel(context.unit, context.skill);
	}
	return context.skillLevel;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSkillGetWeapon(
	SKILL_CONTEXT & context)
{
	if (context.idWeapon == INVALID_ID)
	{
		return NULL;
	}
	context.weapon = UnitGetById(context.game, context.idWeapon);
	return context.weapon;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventGetParamDoScript(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParam,
	SKILL_EVENT_PARAM * pParam)
{
	*pParam = pSkillEvent->tParam[nParam];

	if(!pUnit)
		return FALSE;
	
	const SKILL_DATA * pSkillData = SkillGetData(NULL, nSkill);
	ASSERT_RETFALSE(pSkillData);

	DWORD dwFlagToCheck = INVALID_ID;
	PCODE codeToExecute = NULL_CODE;
	switch(nParam)
	{
	case 0:
		dwFlagToCheck = SKILL_EVENT_FLAG2_USE_PARAM0_PCODE;
		codeToExecute = pSkillData->codeEventParam0;
		break;
	case 1:
		dwFlagToCheck = SKILL_EVENT_FLAG2_USE_PARAM1_PCODE;
		codeToExecute = pSkillData->codeEventParam1;
		break;
	case 2:
		dwFlagToCheck = SKILL_EVENT_FLAG2_USE_PARAM2_PCODE;
		codeToExecute = pSkillData->codeEventParam2;
		break;
	case 3:
		dwFlagToCheck = SKILL_EVENT_FLAG2_USE_PARAM3_PCODE;
		codeToExecute = pSkillData->codeEventParam3;
		break;
	default:
		ASSERTX_RETFALSE(FALSE, "Invalid Skill Event Param Index");
	}

	if(pSkillEvent->dwFlags2 & dwFlagToCheck)
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(NULL, DATATABLE_SKILLS, codeToExecute, &code_len);
		if (code_ptr)
		{
			nSkillLevel = sSkillGetSkillLevel(pUnit, nSkill, nSkillLevel);
			UNIT *pTarget( NULL );
			if( UnitGetGenus( pUnit ) == GENUS_MONSTER )
			{
				pTarget = UnitGetAITarget( pUnit );			
			}
			pParam->nValue = VMExecI( UnitGetGame(pUnit), pUnit, pTarget, NULL, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
			pParam->flValue = float(pParam->nValue);
			
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventGetParam(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParam,
	SKILL_EVENT_PARAM * pParam)
{
	ASSERTX_RETFALSE( pSkillEvent, "Expected skill event" );
	ASSERTX_RETFALSE( nParam >= 0 && nParam < MAX_SKILL_EVENT_PARAM, "Invalid skill event param index" );
	return sSkillEventGetParamDoScript(pUnit, nSkill, nSkillLevel, pSkillEvent, nParam, pParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SKILL_EVENT_PARAM *SkillEventGetParam(
	const SKILL_EVENT *pSkillEvent,
	int nParam)
{
	ASSERTX_RETFALSE( pSkillEvent, "Expected skill event" );
	ASSERTX_RETFALSE( nParam >= 0 && nParam < MAX_SKILL_EVENT_PARAM, "Invalid skill event param index" );
	return &pSkillEvent->tParam[ nParam ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillEventGetParamInt(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript )
{
	SKILL_EVENT_PARAM tParam;
	BOOL bHadScript = sSkillEventGetParam( pUnit, nSkill, nSkillLevel, pSkillEvent, nParamIndex, &tParam );
	if ( pbHadScript )
		*pbHadScript = bHadScript;
	return tParam.nValue;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillEventGetParamFloat(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript )
{
	SKILL_EVENT_PARAM tParam;
	BOOL bHadScript = sSkillEventGetParam( pUnit, nSkill, nSkillLevel, pSkillEvent, nParamIndex, &tParam );
	if ( pbHadScript )
		*pbHadScript = bHadScript;
	return tParam.flValue;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillEventGetParamBool(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript )
{
	SKILL_EVENT_PARAM tParam;
	BOOL bHadScript = sSkillEventGetParam( pUnit, nSkill, nSkillLevel, pSkillEvent, nParamIndex, &tParam );
	if ( pbHadScript )
		*pbHadScript = bHadScript;
	return (BOOL)tParam.nValue;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *SkillGetDevName(
	GAME *pGame,
	int nSkill)
{
	const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );
	return pSkillData->szName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *SkillGetDisplayName(
	GAME *pGame,
	int nSkill)
{
	const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );
	int nDisplayName = pSkillData->nDisplayName;
	return StringTableGetStringByIndex( nDisplayName );
}

//----------------------------------------------------------------------------
// things that you need to do to a skill event, but you don't need a SKILL_DATA
//----------------------------------------------------------------------------
static void sSkillEventLoadCommon(
	SKILL_EVENT * skill_event,
	SKILL_EVENT_TYPE_DATA * skill_event_type_data,
	const char * pszName,
	BOOL bDisabled )
{
	if (skill_event_type_data->eAttachedTable != DATATABLE_NONE)
	{
		skill_event->tAttachmentDef.nAttachedDefId = ExcelGetLineByStringIndex(NULL, skill_event_type_data->eAttachedTable, skill_event->tAttachmentDef.pszAttached);
		if (!bDisabled && !skill_event_type_data->bDoesNotRequireTableEntry)
		{
//			ASSERTX(skill_event->tAttachmentDef.nAttachedDefId != INVALID_ID, pszName);
		}
	}

	if (skill_event_type_data->bConvertParamFromDegreesToDot)
	{
		// This will be broken if use param script is used
		SKILL_EVENT_PARAM *pParam0 = (SKILL_EVENT_PARAM *)SkillEventGetParam(skill_event, 0);
		float fDegrees = pParam0->flValue;
		float fRadians = DegreesToRadians(fDegrees);
		pParam0->flValue = cos(fRadians / 2.0f);
	}

	for (unsigned int ii = 0; ii < MAX_SKILL_EVENT_PARAM; ++ii)	// this keeps us from having to do the conversion all of the time
	{ 
		skill_event->tParam[ii].nValue = (int)skill_event->tParam[ii].flValue;
	}

	ConditionDefinitionPostLoad(skill_event->tCondition);
}


//-------------------------------------------------------------------------------------------------
// weapon target blending vars
void MeleeWeaponsExcelPostProcess(
	void)
{
	EXCEL_TABLE * table = ExcelGetTableNotThreadSafe(DATATABLE_MELEEWEAPONS);
	ASSERT_RETURN(table);

	unsigned int count = ExcelGetCountPrivate(table);

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		MELEE_WEAPON_DATA * weapon_data = (MELEE_WEAPON_DATA *)ExcelGetDataPrivate(table, ii);
		if (!AppIsHammer())
		{
			if (weapon_data->szSwingEvents[0] != 0)
			{
				weapon_data->nSwingEventsId = DefinitionGetIdByName(DEFINITION_GROUP_SKILL_EVENTS, weapon_data->szSwingEvents);
			} 
			else 
			{
				weapon_data->nSwingEventsId = INVALID_ID;
			}
		}
		else
		{
			weapon_data->nSwingEventsId = INVALID_ID;
		}

		if ( weapon_data->nSwingEventsId == INVALID_ID )
			continue;

		SKILL_EVENTS_DEFINITION * skill_events_def = (SKILL_EVENTS_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_SKILL_EVENTS, weapon_data->nSwingEventsId);

		ASSERT_CONTINUE( skill_events_def->nEventHolderCount == 1 );
		SKILL_EVENT_HOLDER * event_holder = &skill_events_def->pEventHolders[0];
		for (int jj = 0; jj < event_holder->nEventCount; ++jj)
		{
			SKILL_EVENT * skill_event = &event_holder->pEvents[jj];
			ASSERTX_CONTINUE(skill_event->nType != INVALID_ID, weapon_data->szSwingEvents);

			SKILL_EVENT_TYPE_DATA * skill_event_type_data = SkillGetEventTypeData(NULL, skill_event->nType);
			ASSERT_CONTINUE(skill_event_type_data);

			sSkillEventLoadCommon( skill_event, skill_event_type_data, weapon_data->szSwingEvents, FALSE );
		}

	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//#define DEFINITION_PROCESSING_ENABLED
#ifdef DEFINITION_PROCESSING_ENABLED
#include "definition_priv.h"
#endif

BOOL ExcelSkillsPostProcessAll( 
	struct EXCEL_TABLE * table)
{
	if ( AppIsHammer() ) // this really slows down Hammer's start up.  I don't think that we need to do it in Hammer - Tyler
		return TRUE;
	ASSERT_RETFALSE(table);

#if ISVERSION(CHEATS_ENABLED)
	GLOBAL_DEFINITION * pGlobalDefinition = DefinitionGetGlobal();
	if(pGlobalDefinition)
	{
		gbDoSkillCooldown = !(pGlobalDefinition->dwFlags & GLOBAL_FLAG_SKILL_COOLDOWN);
		gInfinitePower = pGlobalDefinition->dwFlags & GLOBAL_FLAG_MAX_POWER;
	}
#endif

	MeleeWeaponsExcelPostProcess();

	EXCEL_TABLE * tableSkillEvent = ExcelGetTableNotThreadSafe(DATATABLE_SKILLS);
	{
		EXCEL_TABLE * tableMissiles = ExcelGetTableNotThreadSafe(DATATABLE_MISSILES);
		ASSERT_RETFALSE(tableSkillEvent && tableMissiles);
	}

#if !ISVERSION(SERVER_VERSION)
	SkillsActivatorsInitExcelPostProcess(table);
#endif //!ISVERSION(SERVER_VERSION)

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		SKILL_DATA * skill_data = (SKILL_DATA *)ExcelGetDataPrivate(table, ii);
		if (! skill_data )
			continue;

		if (SkillDataTestFlag(skill_data, SKILL_FLAG_INITIALIZED))
		{
			continue;
		}
		SkillDataSetFlag(skill_data, SKILL_FLAG_INITIALIZED, TRUE);

		skill_data->nId = ii;

		ZeroMemory( skill_data->nSkillsInSameTab, sizeof( int ) * MAX_SKILLS_IN_TAB );
		//figure out what skills are in the same tab as us...
		if( skill_data->nSkillTab > 0 )
		{
			for( unsigned int t = 0; t < ExcelGetCountPrivate(table); t++ )
			{
				if( t != ii )
				{
					const SKILL_DATA * skill_data_check = (const SKILL_DATA *)ExcelGetDataPrivate(table, t);
					if (!skill_data_check )
						continue;				
					if( skill_data_check->nSkillTab == skill_data->nSkillTab )
					{					
						ASSERT_CONTINUE( skill_data->nSkillsInSameTabCount < MAX_SKILLS_IN_TAB );
						skill_data->nSkillsInSameTab[ skill_data->nSkillsInSameTabCount++ ] = t;					
					}
				}
			}
		}

		if (skill_data->szSummonedAI && skill_data->szSummonedAI[0] != 0)
		{
			skill_data->nSummonedAI = DefinitionGetIdByName(DEFINITION_GROUP_AI, skill_data->szSummonedAI);
		}
		//sets the string for the effect accumulation. Usually it's going to be the same as the effect....
		if( skill_data->pnStrings[ SKILL_STRING_EFFECT_ACCUMULATION ] == INVALID_ID )
		{
			skill_data->pnStrings[ SKILL_STRING_EFFECT_ACCUMULATION ] = skill_data->pnStrings[ SKILL_STRING_EFFECT ];
		}

		for (unsigned int jj = 0; jj < MAX_MISSILES_PER_SKILL; ++jj)
		{
			skill_data->pnMissileIds[jj] = INVALID_ID;
		}
		for (unsigned int jj = 0; jj < MAX_SKILLS_PER_SKILL; ++jj)
		{
			skill_data->pnSkillIds[jj] = INVALID_ID;
		}

		skill_data->nPowerCost = StatsGetAsShiftedInt( STATS_POWER_CUR, skill_data->fPowerCost );
		skill_data->nPowerCostPerLevel = StatsGetAsShiftedInt( STATS_POWER_CUR, skill_data->fPowerCostPerLevel );

		skill_data->nAimHolderIndex = INVALID_ID;
		skill_data->nAimEventIndex = INVALID_ID;

		skill_data->fFiringCone = DegreesToRadians(skill_data->fFiringCone);

		if (skill_data->tTargetQueryFilter.nUnitType == INVALID_ID || skill_data->tTargetQueryFilter.nUnitType == UNITTYPE_NONE)
		{
			skill_data->tTargetQueryFilter.nUnitType = UNITTYPE_ANY;
		}

		if (skill_data->nRequiredWeaponUnittype == INVALID_ID || skill_data->nRequiredWeaponUnittype == UNITTYPE_NONE)
		{
			skill_data->nRequiredWeaponUnittype = UNITTYPE_ANY;
		}
		
		if( skill_data->nUnitEventTrigger[0] != INVALID_ID )
		{
			SkillDataSetFlag(skill_data, SKILL_FLAG_HAS_EVENT_TRIGGER, TRUE);				
		}

		if (skill_data->nStateOnSelect != INVALID_ID ||
			 SkillDataTestFlag(skill_data, SKILL_FLAG_START_ON_SELECT) ||
			 SkillDataTestFlag(skill_data, SKILL_FLAG_HAS_EVENT_TRIGGER) )
		{
			SkillDataSetFlag(skill_data, SKILL_FLAG_CAN_BE_SELECTED, TRUE);
		}

		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DEAD_AND_ALIVE))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_BOTH_DEAD_AND_ALIVE );
		}		
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DONT_IGNORE_OWNED_STATE))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_DONT_IGNORE_OWNED_STATE );
		}		
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DYING_ON_START))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DYING_AFTER_START))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_ONLY_DYING))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_DYING );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DEAD))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DEAD );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_ENEMY))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_FRIEND))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_CONTAINER))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_CONTAINER );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_PETS))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_PETS );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_DONT_TARGET_PETS))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_NO_PETS );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_IGNORE_TEAM))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_IGNORE_CHAMPIONS))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_CHAMPIONS );
		}
		if (AppIsHellgate() && SkillDataTestFlag(skill_data, SKILL_FLAG_CHECK_LOS))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_TARGET_DESTRUCTABLES))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_DONT_TARGET_DESTRUCTABLES))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_NO_DESTRUCTABLES );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_ALLOW_UNTARGETABLES))
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES );
		}
		if (UnitIsA(skill_data->tTargetQueryFilter.nUnitType, UNITTYPE_OBJECT) ||
			SkillDataTestFlag(skill_data, SKILL_FLAG_ALLOW_OBJECTS) )
		{
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
			SETBIT( skill_data->tTargetQueryFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
		}
		if (SkillDataTestFlag(skill_data, SKILL_FLAG_CHECK_INVENTORY_SPACE))
		{ 
			ASSERT(skill_data->pnSummonedInventoryLocations[0] != INVALID_ID);
			ASSERT(skill_data->pnSummonedInventoryLocations[1] == INVALID_ID);
		}

		for (unsigned int jj = 0; jj < MAX_WEAPON_LOCATIONS_PER_SKILL; ++jj)
		{	// we should look this up in excel data, but it seems like doesn't really work for inventory locations
			if (skill_data->pnWeaponInventoryLocations[jj] == INVLOC_RHAND)
			{
				skill_data->pnWeaponIndices[jj] = WEAPON_INDEX_RIGHT_HAND;
			}
			else if (skill_data->pnWeaponInventoryLocations[jj] == INVLOC_LHAND)
			{
				skill_data->pnWeaponIndices[jj] = WEAPON_INDEX_LEFT_HAND;
			}
			else
			{
				skill_data->pnWeaponIndices[jj] = INVALID_ID;
			}
		}

		if (skill_data->szEvents && skill_data->szEvents[0] != 0)
		{
			skill_data->nEventsId = DefinitionGetIdByName(DEFINITION_GROUP_SKILL_EVENTS, skill_data->szEvents);
		}
		else
		{
			skill_data->nEventsId = INVALID_ID;
		}

		if (skill_data->nEventsId == INVALID_ID)
		{
			continue;
		}

		SKILL_EVENTS_DEFINITION * skill_events_def = (SKILL_EVENTS_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_SKILL_EVENTS, skill_data->nEventsId);
		if (!skill_events_def)
		{
			ShowDataWarning("Skill %s references nonexistent skill events file %s\n", skill_data->szName, skill_data->szEvents);
			skill_data->nEventsId = INVALID_ID;
		}

		if (!skill_events_def->pEventHolders)
		{
			continue;
		}

#ifdef DEFINITION_PROCESSING_ENABLED
		BOOL bSaveDefinition = FALSE;
#endif
		for (int ii = 0; ii < skill_events_def->nEventHolderCount; ++ii)
		{
			SKILL_EVENT_HOLDER * event_holder = &skill_events_def->pEventHolders[ii];
			for (int jj = 0; jj < event_holder->nEventCount; ++jj)
			{
				SKILL_EVENT * skill_event = &event_holder->pEvents[jj];
				ASSERTX_CONTINUE(skill_event->nType != INVALID_ID, skill_data->szEvents);

				SKILL_EVENT_TYPE_DATA * skill_event_type_data = SkillGetEventTypeData(NULL, skill_event->nType);
				ASSERT_CONTINUE(skill_event_type_data);

				sSkillEventLoadCommon( skill_event, skill_event_type_data, skill_data->szEvents, SkillDataTestFlag(skill_data, SKILL_FLAG_DISABLED) );

				if (skill_event_type_data->bAimingEvent)
				{
					skill_data->nAimHolderIndex = ii;
					skill_data->nAimEventIndex = jj;
				}
				if (skill_event_type_data->bUsesTargetIndex)
				{
					SkillDataSetFlag(skill_data, SKILL_FLAG_USES_TARGET_INDEX, TRUE);
				}

#ifdef DEFINITION_PROCESSING_ENABLED
				// !!! PUT YOUR CODE HERE !!!
				if (PStrICmp(skill_event_type_data->szEventHandler, "FireLaser") == 0)
				{
					// copy all params to param0
					SKILL_EVENT_PARAM *pParam0 = (SKILL_EVENT_PARAM *)SkillEventGetParam(skill_event, 0);
					pParam0->flValue = 0;
					bSaveDefinition = TRUE;
				}
#endif
				sSkillsInitFixupMeleeFlag(NULL, skill_data, skill_event, skill_event_type_data);

				if (skill_event_type_data->bUsesLasers)
				{
					SkillDataSetFlag(skill_data, SKILL_FLAG_USES_LASERS, TRUE);

					if(skill_event->dwFlags2 & SKILL_EVENT_FLAG2_LASER_INCLUDE_IN_UI)
					{
						skill_data->nLaserCount++;
					}
				}
				if (skill_event_type_data->bCanMultiBullets)
				{
					SkillDataSetFlag(skill_data, SKILL_FLAG_CAN_MULTI_BULLETS, TRUE);
				}
				if (skill_event_type_data->bUsesMissiles)
				{
					SkillDataSetFlag(skill_data, SKILL_FLAG_USES_MISSILES, TRUE);
					// GS - ALERT!  This will be broken if "use param script" is used!
					int nMissileCount = skill_event->tParam[0].nValue;
					nMissileCount = MAX(0, nMissileCount);
					skill_data->nMissileCount += nMissileCount;
					if ( nMissileCount > 1 )
						SkillDataSetFlag( skill_data, SKILL_FLAG_USES_HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD, TRUE );
				}
				if (skill_event_type_data->bStartCoolingAndPowerCost || skill_event->dwFlags2 & SKILL_EVENT_FLAG2_CHARGE_POWER_AND_COOLDOWN)
				{
					// TRAVIS: Yeah, I know this is a little dangerous, but
					// we have cascading conditional events that need to charge, and each one needs to have the flag ( for the one that is successful )
					
					if( AppIsHellgate() )
					{
						ASSERTV(!SkillDataTestFlag(skill_data, SKILL_FLAG_DONT_COOLDOWN_ON_START), "Skill %s has more than one skill event that wants to set the cooldown and power cost", skill_data->szName);
						ASSERTV(!SkillDataTestFlag(skill_data, SKILL_FLAG_POWER_ON_EVENT), "Skill %s has more than one skill event that wants to set the cooldown and power cost", skill_data->szName);
					}
					SkillDataSetFlag(skill_data, SKILL_FLAG_DONT_COOLDOWN_ON_START, TRUE);
					SkillDataSetFlag(skill_data, SKILL_FLAG_POWER_ON_EVENT, TRUE);
				}
				if (skill_event_type_data->bConsumeItem)
				{
					ASSERTV(!SkillDataTestFlag(skill_data, SKILL_FLAG_CONSUME_ITEM_ON_EVENT), "Skill %s has more than one skill event that wants to consume the target item", skill_data->szName);
					SkillDataSetFlag(skill_data, SKILL_FLAG_CONSUME_ITEM_ON_EVENT, TRUE);
				}

				if (skill_event_type_data->eAttachedTable == DATATABLE_MISSILES &&
					skill_event->tAttachmentDef.nAttachedDefId != INVALID_ID)
				{
					BOOL bFound = FALSE;
					int nMissileClass = skill_event->tAttachmentDef.nAttachedDefId;
					for (unsigned int ii = 0; ii < MAX_MISSILES_PER_SKILL; ++ii)
					{
						if (skill_data->pnMissileIds[ii] == nMissileClass)
						{
							bFound = TRUE;
							break;
						}
						if (skill_data->pnMissileIds[ii] == INVALID_ID)
						{
							skill_data->pnMissileIds[ii] = nMissileClass;
							bFound = TRUE;
							break;
						}
					}
					ASSERTX_CONTINUE(bFound, "Skills need to handle more missiles");
				}

				if (skill_event_type_data->eAttachedTable == DATATABLE_SKILLS &&
					skill_event->tAttachmentDef.nAttachedDefId != INVALID_ID)
				{
					BOOL bFound = FALSE;
					int nSkillId = skill_event->tAttachmentDef.nAttachedDefId;
					for (unsigned int ii = 0; ii < MAX_SKILLS_PER_SKILL; ++ii)
					{
						if (skill_data->pnSkillIds[ii] == nSkillId)
						{
							bFound = TRUE;
							break;
						}
						if (skill_data->pnSkillIds[ii] == INVALID_ID)
						{
							skill_data->pnSkillIds[ii] = nSkillId;
							bFound = TRUE;
							break;
						}
					}
					ASSERTX_CONTINUE(bFound, "Skills need to handle more subskills");
				}
			}
		}

#ifdef DEFINITION_PROCESSING_ENABLED
		if (bSaveDefinition)
		{
			CDefinitionContainer * container = CDefinitionContainer::GetContainerByType(DEFINITION_GROUP_SKILL_EVENTS);
			container->Save(skill_events_def);
		}
#endif
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetCooldown( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nSkill,
	const SKILL_DATA * pSkillData,
	int nMinTicks,
	int nSkillLevel /*=-1*/ )
{
	int nCooldown = 0;
	int nPercent = 0;

	ASSERT_RETZERO(pSkillData);
	ASSERT_RETZERO(pGame && pUnit);

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_UNIT_COOLDOWN ) )
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		nCooldown = pUnitData->nCooldown;
		if ( ! nCooldown ) // just in case the unit didn't specify a cooldown
			nCooldown = pSkillData->nCooldownTicks;
		nCooldown <<= StatsGetShift(pGame, STATS_SKILL_COOLDOWN);
	}
	else if ( pWeapon && SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_COOLDOWN ) )
	{
		nCooldown = UnitGetStatAny(pWeapon, STATS_SKILL_COOLDOWN, NULL);
		if ( ! nCooldown )
		{
			int nWeaponSkill = ItemGetPrimarySkill( pWeapon );
			const SKILL_DATA * pSkillDataWeapon = SkillGetData( pGame, nWeaponSkill );
			if ( pSkillDataWeapon )
				nCooldown = pSkillDataWeapon->nCooldownTicks << StatsGetShift(pGame, STATS_SKILL_COOLDOWN);
		}
	} 
	else
	{
		if( pSkillData->codeCoolDown != NULL_CODE )
		{
			int code_len = 0;
			BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeCoolDown, &code_len);
			if (code_ptr)
			{
				if( UnitIsA( pUnit, UNITTYPE_ITEM ) )
				{
					UnitSetStat( pUnit, STATS_SKILL_EXECUTED_BY_ITEMCLASS, UnitGetClass( pUnit ) );
				}
				nCooldown = VMExecI(pGame, pUnit, NULL, pSkillData->nId, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len) << StatsGetShift( NULL, STATS_SKILL_COOLDOWN );
			}

		}
		else
		{
			nCooldown = pSkillData->nCooldownTicks << StatsGetShift(pGame, STATS_SKILL_COOLDOWN);
		}
	} 

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_ADD_UNIT_COOLDOWN ) )
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );		
		nCooldown += pUnitData->nCooldown << StatsGetShift(pGame, STATS_SKILL_COOLDOWN);					
		nCooldown += UnitGetStat( pUnit, STATS_SKILL_COOLDOWN, nSkill ) + UnitGetStat( pUnit, STATS_SKILL_COOLDOWN );
	}
	

	if ( ! nCooldown )
		return 0; // it only gets faster from here...

	if (!SkillDataTestFlag(pSkillData, SKILL_FLAG_CONSTANT_COOLDOWN))		// Don't adjust the cooldown of this skill if this skill says not to
	{
		nPercent += UnitGetStat(pUnit, STATS_OFFWEAPON_COOLDOWN_PCT);
	}
	for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnSkillGroup[ i ] != INVALID_ID )
			nPercent += UnitGetStat( pUnit, STATS_COOLDOWN_PCT_SKILLGROUP, pSkillData->pnSkillGroup[ i ] );
	}
	
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeCooldownPercentChange, &code_len);
	if (code_ptr )
	{
		if ( nSkillLevel <= 0 )
			nSkillLevel = SkillGetLevel( pUnit, nSkill );
		int nSkillLevelForCooldown = MAX( nSkillLevel - 1, 0 );
		nPercent += VMExecI(pGame, pUnit, NULL, nPercent, nSkillLevelForCooldown, nSkill, nSkillLevelForCooldown, INVALID_ID, code_ptr, code_len);
	}

	//this will be zero if not set
	nPercent += UnitGetStat( pUnit, STATS_COOL_DOWN_PCT_SKILL, nSkill );

	if ( nPercent <= -100 )
		nCooldown = 100 * nCooldown;
	else if ( nPercent )
		nCooldown = (nCooldown * 100) / (100 + nPercent);

	nCooldown = nCooldown >> StatsGetShift(pGame, STATS_SKILL_COOLDOWN);

	nCooldown = MAX( nCooldown, nMinTicks );

	return nCooldown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS * sSkillGetCooldownStats( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nSkill )
{
	STATS* stats = StatsGetModList(pUnit, SELECTOR_SKILL_COOLDOWN);
	while (stats)
	{
		if (StatsGetStat(stats, STATS_SKILL_COOLING, nSkill) > 0)
		{
			return stats;
		}
		stats = StatsGetModNext(stats, SELECTOR_SKILL_COOLDOWN);
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS * sSkillGroupGetCooldownStats( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nSkillGroup )
{
	STATS* stats = StatsGetModList(pUnit, SELECTOR_SKILL_COOLDOWN);
	while (stats)
	{
		if (StatsGetStat(stats, STATS_SKILL_GROUP_COOLING, nSkillGroup) > 0)
		{
			return stats;
		}
		stats = StatsGetModNext(stats, SELECTOR_SKILL_COOLDOWN);
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSkillStartCooldown( 
	SKILL_CONTEXT & context,
	int nMinTicks,
	BOOL bForceMinTicks = FALSE,
	BOOL bSendMessage = FALSE)
{
	ASSERT_RETURN(context.game);
	ASSERT_RETURN(context.skillData);

#if ISVERSION(CHEATS_ENABLED)
	if (gbDoSkillCooldown == FALSE)
	{
		return;
	}
#endif
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_NEVER_SET_COOLDOWN))
	{
		return;
	}
	
	UNIT * pUnitToCool = context.unit;
	if (context.weapon && !SkillDataTestFlag(context.skillData, SKILL_FLAG_COOLDOWN_UNIT_INSTEAD_OF_WEAPON))		
	{
		pUnitToCool = context.weapon;
	}
	if (!pUnitToCool)
	{
		return;
	}
	int nCooldown = bForceMinTicks ? nMinTicks : SkillGetCooldown(context.game, context.unit, context.weapon, context.skill, context.skillData, nMinTicks, context.skillLevel);

	int nTick = (int)GameGetTick(context.game);	// it's gonna get converted to an int anyway

	// cooldown the skill
	if ( nCooldown )
	{
		STATS * pStats = NULL;
		int nCurrentCooling = UnitGetStat(pUnitToCool, STATS_SKILL_COOLING, context.skill);
		if (nCurrentCooling > nTick + nCooldown)
		{
			pStats = NULL;
		}
		else if (nCurrentCooling)
		{
			pStats = sSkillGetCooldownStats(context.game, pUnitToCool, context.skill);
			ASSERT_RETURN(pStats);
		} 
		else 
		{
			pStats = StatsListInit(context.game);
			ASSERT_RETURN(pStats);
		}

		if (pStats)
		{
			StatsSetStat(context.game, pStats, STATS_SKILL_COOLING, context.skill, nTick + nCooldown);
			if (!nCurrentCooling)
			{
				StatsListAdd(pUnitToCool, pStats, TRUE, SELECTOR_SKILL_COOLDOWN);
			}
			StatsSetTimer(context.game, pUnitToCool, pStats, nCooldown);
		}
	}

	// cooldown the skill group
	if (context.skillData->nCooldownSkillGroup != INVALID_ID &&
		context.skillData->nCooldownTicksForSkillGroup )
	{
		STATS * pStats = NULL;
		int nCurrentCooling = UnitGetStat(context.unit, STATS_SKILL_GROUP_COOLING, context.skillData->nCooldownSkillGroup);
		if (nCurrentCooling > nTick + context.skillData->nCooldownTicksForSkillGroup)
		{ 
			// it already has a cooldown that will end later than this one
			pStats = NULL;
		}
		else if (nCurrentCooling)
		{
			pStats = sSkillGroupGetCooldownStats(context.game, pUnitToCool, context.skillData->nCooldownSkillGroup);
			ASSERT_RETURN( pStats );
		} 
		else 
		{
			pStats = StatsListInit(context.game);
			ASSERT_RETURN( pStats );
		}

		if (pStats)
		{
			StatsSetStat(context.game, pStats, STATS_SKILL_GROUP_COOLING, context.skillData->nCooldownSkillGroup, nTick + context.skillData->nCooldownTicksForSkillGroup);
			StatsSetStat(context.game, pStats, STATS_SKILL_GROUP_COOLING_TICKS, context.skillData->nCooldownSkillGroup, context.skillData->nCooldownTicksForSkillGroup);
			if (!nCurrentCooling)
			{
				StatsListAdd(pUnitToCool, pStats, TRUE, SELECTOR_SKILL_COOLDOWN);
			}
			StatsSetTimer(context.game, pUnitToCool, pStats, context.skillData->nCooldownTicksForSkillGroup);
		}
	}

	if ( bSendMessage && IS_SERVER( context.game ) )
	{
		MSG_SCMD_SKILLCOOLDOWN tMsg;
		tMsg.id = UnitGetId( context.unit );
		tMsg.idWeapon = UnitGetId( context.weapon );
		tMsg.wSkillId = DOWNCAST_INT_TO_WORD( context.skill );
		tMsg.wTicks = DOWNCAST_INT_TO_WORD(nCooldown);
		SkillSendMessage( context.game, context.unit, SCMD_SKILLCOOLDOWN, &tMsg, TRUE );
	}

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(context.game) && GameGetControlUnit(context.game) == context.unit)
	{
		UISkillShowCooldown(context.game, context.unit, context.skill);
	}
#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillStartCooldown( 
	GAME * game, 
	UNIT * unit, 
	UNIT * weapon, 
	int skill,
	int nSkillLevel,
	const SKILL_DATA * skillData,
	int nMinTicks,
	BOOL bForceMinTicks,
	BOOL bSendMessage )
{
	SKILL_CONTEXT context(game, unit, skill, nSkillLevel, skillData, weapon, 0, 0);
	sSkillStartCooldown(context, nMinTicks, bForceMinTicks, bSendMessage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearAllSkillCooldowns(
	GAME * pGame,
	UNIT * pUnit)
{
	STATS* stats = StatsGetModList(pUnit, SELECTOR_SKILL_COOLDOWN);
	while (stats)
	{
		PARAM nSkill = 0;
		PARAM nSkillGroup = 0;
		if (StatsGetStatAny(stats, STATS_SKILL_COOLING, &nSkill) > 0)
		{
			const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
			if(pSkillData && !SkillDataTestFlag(pSkillData, SKILL_FLAG_DONT_CLEAR_COOLDOWN_ON_DEATH))
			{
				StatsSetTimer(pGame, pUnit, stats, 1);
			}
		}
		else if (StatsGetStatAny(stats, STATS_SKILL_GROUP_COOLING, &nSkillGroup) > 0)
		{
			const SKILLGROUP_DATA * pSkillGroupData = (const SKILLGROUP_DATA *) ExcelGetData( EXCEL_CONTEXT(pGame), DATATABLE_SKILLGROUPS, nSkillGroup);
			if(pSkillGroupData && !pSkillGroupData->bDontClearCooldownOnDeath)
			{
				StatsSetTimer(pGame, pUnit, stats, 1);
			}
		}
		stats = StatsGetModNext(stats, SELECTOR_SKILL_COOLDOWN);
	}
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe != INVALID_ID )
	{
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
			if ( pWeapon )
				UnitClearAllSkillCooldowns( pGame, pWeapon );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeAllSkillCooldowns(
	GAME * pGame,
	UNIT * pUnit,
	int	   nDelta)
{
	// assumes the stat timers are already set for the new value
	STATS* stats = StatsGetModList(pUnit, SELECTOR_SKILL_COOLDOWN);
	while (stats)
	{
		PARAM nSkill = 0;
		PARAM nSkillGroup = 0;
		if (StatsGetStatAny(stats, STATS_SKILL_COOLING, &nSkill) > 0)
		{
			StatsChangeStat(pGame, stats, STATS_SKILL_COOLING, nSkill, nDelta);
		}
		else if (StatsGetStatAny(stats, STATS_SKILL_GROUP_COOLING, &nSkillGroup) > 0)
		{
			StatsChangeStat(pGame, stats, STATS_SKILL_GROUP_COOLING, nSkillGroup, nDelta);
		}
		stats = StatsGetModNext(stats, SELECTOR_SKILL_COOLDOWN);
	}
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe != INVALID_ID )
	{
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
			if ( pWeapon )
				UnitChangeAllSkillCooldowns( pGame, pWeapon, nDelta );
		}
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsCooling(
	UNIT * pUnit, 
	int nSkill,
	UNITID idWeapon )
{
	if (! pUnit )
		return FALSE;
	if ( idWeapon != INVALID_ID )
	{
		UNIT * pWeapon = UnitGetById( UnitGetGame( pUnit ), idWeapon );
		if ( pWeapon )
			pUnit = pWeapon;
	}
	return pUnit ? UnitGetStat( pUnit, STATS_SKILL_COOLING, nSkill ) != 0 : FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillClearCooldown(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill)
{
	ASSERT_RETURN(pGame);

	UNIT * pUnitToCool = pWeapon ? pWeapon : pUnit;
	ASSERT_RETURN(pUnitToCool);

	STATS * pStats = sSkillGetCooldownStats( pGame, pUnitToCool, nSkill );
	if(!pStats)
		return;

	StatsSetTimer(pGame, pUnitToCool, pStats, 1);
}

//----------------------------------------------------------------------------
static void sDecreaseWeaponFiringError(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon);
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleDecreaseWeaponFiringError(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	UNIT * pContainer = UnitGetContainer( pUnit );
	sDecreaseWeaponFiringError( pGame, pContainer, pUnit );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDecreaseWeaponFiringError(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon)
{
	if ( UnitTestFlag(pWeapon, UNITFLAG_JUSTFREED) )
		return;

	int nCooldown = UnitGetStatAny( pWeapon, STATS_SKILL_COOLING, NULL );
	int nGameTick = (int) GameGetTick( pGame );
	if ( nCooldown > 0 && nCooldown - nGameTick > 0 )
	{
		UnitUnregisterTimedEvent( pWeapon, sHandleDecreaseWeaponFiringError );
		UnitRegisterEventTimed( pWeapon, sHandleDecreaseWeaponFiringError, EVENT_DATA(), nCooldown - nGameTick );		
	} else {

		StateClearAllOfType( pWeapon, STATE_FIRING_ERROR_INCREASING_WEAPON );

		STATS * pStats = NULL;
		if ( IS_SERVER( pWeapon ) )
			pStats = s_StateSet( pWeapon, pUnit, STATE_FIRING_ERROR_DECREASING_WEAPON, 0 );
		else
			pStats = c_StateSet( pWeapon, pUnit, STATE_FIRING_ERROR_DECREASING_WEAPON, 0, 0, INVALID_ID );
		if ( pStats )
		{
			int nSource = UnitGetStat( pWeapon, STATS_FIRING_ERROR_DECREASE_SOURCE_WEAPON );
			StatsSetStat( pGame, pStats, STATS_FIRING_ERROR_DECREASE_WEAPON, nSource );
		}

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsUpdateMovingFiringError(
	UNIT * unit,
	float fDistanceMoved )
{
	ASSERT_RETURN( AppIsHellgate() );
	ASSERT_RETURN( unit );
	GAME * game = UnitGetGame( unit );
	ASSERT_RETURN( game );
	int nStopMovingState = UnitGetFirstStateOfType(game, unit, STATE_STOP_MOVING);
#define MOVEMENT_CHECK_EPSILON 0.01f
	BOOL bIsMoving = fabs( fDistanceMoved ) > MOVEMENT_CHECK_EPSILON && nStopMovingState < 0;

	if ( bIsMoving )
	{
		if ( ! SkillIsOnWithFlag( unit, SKILL_FLAG_DO_NOT_CLEAR_REMOVE_ON_MOVE_STATES ) )
			StateClearAllOfType( unit, STATE_REMOVE_ON_MOVE );

		if ( !UnitHasState( game, unit, STATE_FIRING_ERROR_INCREASING ))
		{
			StateClearAllOfType( unit, STATE_FIRING_ERROR_DECREASING );

			STATS * pStats;
			if ( IS_SERVER( unit ) )
				pStats = s_StateSet( unit, unit, STATE_FIRING_ERROR_INCREASING, 0 );
			else
				pStats = c_StateSet( unit, unit, STATE_FIRING_ERROR_INCREASING, 0, 0, INVALID_ID );

			if ( pStats )
			{
				int nRate = UnitGetStat( unit, STATS_FIRING_ERROR_INCREASE_SOURCE );
				StatsSetStat( game, pStats, STATS_FIRING_ERROR_INCREASE, nRate );
			}
		}
	} else {
		if ( !UnitHasState( game, unit, STATE_FIRING_ERROR_DECREASING ))
		{
			StateClearAllOfType( unit, STATE_FIRING_ERROR_INCREASING );

			STATS * pStats;
			if ( IS_SERVER( unit ) )
				pStats = s_StateSet( unit, unit, STATE_FIRING_ERROR_DECREASING, 0 );
			else
				pStats = c_StateSet( unit, unit, STATE_FIRING_ERROR_DECREASING, 0, 0, INVALID_ID );
			if ( pStats )
			{
				int nRate = UnitGetStat( unit, STATS_FIRING_ERROR_DECREASE_SOURCE );
				StatsSetStat( game, pStats, STATS_FIRING_ERROR_DECREASE, nRate );
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sIncreaseWeaponEnergy(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon)
{
	StateClearAllOfType( pWeapon, STATE_ENERGY_DECREASING );

	STATS * pStats = NULL;
	if ( IS_SERVER( pWeapon ) )
		pStats = s_StateSet(pWeapon, pUnit, STATE_ENERGY_INCREASING, 0);
	else
		pStats = c_StateSet(pWeapon, pUnit, STATE_ENERGY_INCREASING, 0, 0, INVALID_ID );

	if ( pStats )
	{
		int nSource = UnitGetStat( pWeapon, STATS_ENERGY_INCREASE_SOURCE );
		StatsSetStat( pGame, pStats, STATS_ENERGY_INCREASE, nSource );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillNewWeaponIsEquipped(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon )
{
	if ( ! pWeapon || ! pUnit )
	{
		if( AppIsTugboat() && ( UnitIsA( pUnit, UNITTYPE_PLAYER ) ||
			UnitIsA( pUnit, UNITTYPE_HIRELING ) ) )
		{
			UnitCalculateVisibleDamage( pUnit );
		}
		return;
	}

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = StatGetParam( STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0 );
		if (nSkill == INVALID_ID)
		{
			continue;
		}

		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_COOLDOWN ) )
			continue;

		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );
		for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
		{
			if ( pWeapons[ i ] != pWeapon )
				continue;

			// make weapons cooldown when then are first equipped
			if( AppIsHellgate() )
			{
				const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
				SkillStartCooldown( pGame, pUnit, pWeapon, nSkill, 0, pSkillData, GAME_TICKS_PER_SECOND );
			}
		}
	}
	UNIT_ITERATE_STATS_END;

	if( AppIsTugboat() && ( UnitIsA( pUnit, UNITTYPE_PLAYER ) ||
		UnitIsA( pUnit, UNITTYPE_HIRELING ) ) )
	{
		UnitCalculateVisibleDamage( pUnit );
	}

	if ( pWeapon && sWeaponUsesFiringError( pWeapon ) )
	{
		sDecreaseWeaponFiringError( pGame, pUnit, pWeapon );
	}
	if ( pWeapon && UnitUsesEnergy( pWeapon ) )
	{
		sIncreaseWeaponEnergy( pGame, pUnit, pWeapon );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillSetSeed( GAME * pGame, UNIT * pUnit, UNIT * pWeapon, int nSkill, DWORD dwSeed, BOOL bSetChildren = TRUE )
{
	UnitSetStat( pWeapon ? pWeapon : pUnit, STATS_SKILL_SEED, nSkill, dwSeed );
	if(bSetChildren)
	{
		const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
		if(pSkillData)
		{
			for(int i=0; i<MAX_SKILLS_PER_SKILL; i++)
			{
				if(pSkillData->pnSkillIds[i] != INVALID_ID)
				{
					UnitSetStat( pWeapon ? pWeapon : pUnit, STATS_SKILL_SEED, pSkillData->pnSkillIds[i], dwSeed );
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD SkillGetSeed( GAME * pGame, UNIT * pUnit, UNIT * pWeapon, int nSkill )
{
	RAND tRand;
	DWORD dwSeedCurr = UnitGetStat( pWeapon ? pWeapon : pUnit, STATS_SKILL_SEED, nSkill );;
	RandInit(tRand, dwSeedCurr);
	sSkillSetSeed(pGame, pUnit, pWeapon, nSkill, RandGetNum(tRand), FALSE);
	return dwSeedCurr;
}
void SkillInitRand( RAND &tRand, GAME * pGame, UNIT * pUnit, UNIT * pWeapon, int nSkill )
{
	DWORD dwSeedCurr = UnitGetStat( pWeapon ? pWeapon : pUnit, STATS_SKILL_SEED, nSkill );
	RandInit(tRand, dwSeedCurr);
	sSkillSetSeed(pGame, pUnit, pWeapon, nSkill, RandGetNum(tRand), FALSE);	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareSkillIsOn( const void * p1, const void * p2 )
{
	SKILL_IS_ON * pFirst  = (SKILL_IS_ON *)p1;
	SKILL_IS_ON * pSecond = (SKILL_IS_ON *)p2;
	if ( pFirst->nSkill > pSecond->nSkill )
		return 1;
	if ( pFirst->nSkill < pSecond->nSkill )
		return -1;
	if ( pFirst->idWeapon > pSecond->idWeapon )
		return 1;
	if ( pFirst->idWeapon < pSecond->idWeapon )
		return -1;
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillGetWarmUpTicks( 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	const SKILL_DATA * pSkillData )
{
	return pSkillData->nWarmupTicks;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillNeedsToHold(
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill,
	BOOL bIgnoreWeapon)
{
	ASSERT_RETZERO(pUnit);
	ASSERT_RETZERO(pUnit->pSkillInfo);
	GAME * pGame = UnitGetGame( pUnit );

	UNITID idWeapon = pWeapon ? UnitGetId(pWeapon) : INVALID_ID;
	SKILL_IS_ON * pSkillIsOn = SkillIsOn(pUnit, nSkill, idWeapon, bIgnoreWeapon);
	if(!pSkillIsOn)
		return 0;

	const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );

	int nHoldTicks = pSkillData->nMinHoldTicks;

	if ( nHoldTicks == 0 )
	{
		float fDuration = SkillGetDuration( pGame, pUnit, nSkill, TRUE, pWeapon );
		if( AppIsTugboat() )
		{
			if( SkillDataTestFlag( pSkillData, SKILL_FLAG_NO_BLEND ) != 0 )
			{
			}
			else
			{
				if( pWeapon && UnitTestFlag(pUnit, UNITFLAG_NOMELEEBLEND) == 0 &&
					UnitIsDualWielding( pUnit, nSkill ) )
				{
					if ( WEAPON_INDEX_RIGHT_HAND == WardrobeGetWeaponIndex( pUnit, UnitGetId( pWeapon ) ) )
					{
						fDuration *= RIGHT_SWING_MULTIPLIER;
					}
					else
					{
						fDuration *= LEFT_SWING_MULTIPLIER;
					}
				}
				else
				{
					fDuration *= RIGHT_SWING_MULTIPLIER;
				}
			}
		}

		nHoldTicks = PrimeFloat2Int(GAME_TICKS_PER_SECOND_FLOAT * fDuration);		
		//if ( nHoldTicks > 2 )  // this was cutting off events at the end of the skill
		//	nHoldTicks -= 1; // we don't need to hold the entire skill.  Just most of it.
	} 

	nHoldTicks += sSkillGetWarmUpTicks( pUnit, pWeapon, pSkillData );

	if ( nHoldTicks <= 0 ) 
		return 0;

	GAME_TICK tiStartTime = pSkillIsOn->tiStartTick;
	GAME_TICK tiCurrentTime = GameGetTick(pGame);

	if ( tiStartTime > tiCurrentTime )
		return (int)tiStartTime + nHoldTicks - (int)tiCurrentTime;// skill hasn't executed yet.
	return max( 0, (int)tiStartTime + nHoldTicks - (int)GameGetTick(pGame) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillDoCanTakePowerCost(
	SKILL_CONTEXT & context,
	BOOL bPlaySounds)
{
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
	if (GameGetDebugFlag(context.game, DEBUGFLAG_INFINITE_POWER))
		return TRUE;
#endif

	const UNIT_DATA * pUnitData = UnitGetData(context.unit);
	ASSERT_RETFALSE(pUnitData);

	// check power
	int nPowerCost = SkillGetPowerCost(context.unit, context.skillData, context.skillLevel );
	if (nPowerCost > 0)
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_KILL_PETS_FOR_POWER_COST))
		{
			// if there are pets consuming too much max power for this power cost to be applied normally,
			// enough of the other pets will be killed,
			// and their max power consumption will be turned into power for summoning this pet
			int nPowerMax = UnitGetStat(context.unit, STATS_POWER_MAX);
			if (nPowerCost > nPowerMax)
			{
				int nPowerMaxPenaltyPets = UnitGetStatShift(context.unit, STATS_POWER_MAX_PENALTY_PETS);
				if( UnitGetStat(context.unit, STATS_POWER_CUR) == nPowerMax &&
					nPowerMax - nPowerMaxPenaltyPets >= nPowerCost )
				{

					return TRUE;
				}
				else
				{

#if !ISVERSION(SERVER_VERSION)
					if (bPlaySounds && pUnitData->m_nOutOfManaSound != INVALID_ID)
					{
						c_SoundPlay(pUnitData->m_nOutOfManaSound, &c_UnitGetPosition(context.unit), NULL);
					}
#endif
					return FALSE;
				}
			}
		}

		if (UnitGetStat(context.unit, STATS_POWER_CUR) < nPowerCost)
		{
#if !ISVERSION(SERVER_VERSION)
			if (bPlaySounds && pUnitData->m_nOutOfManaSound != INVALID_ID)
			{
				c_SoundPlay(pUnitData->m_nOutOfManaSound, &c_UnitGetPosition(context.unit), NULL);
			}
#endif
			return FALSE;
		}	
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST))
	{
		const SKILL_EVENTS_DEFINITION * pEvents = (const SKILL_EVENTS_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_SKILL_EVENTS, context.skillData->nEventsId);
		if (pEvents)
		{
			// Find a skill event that checks the pet power cost
			for (int i = 0; i < pEvents->nEventHolderCount; i++)
			{
				const SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[i];
				ASSERT_CONTINUE(pHolder);
				for (int j = 0; j < pHolder->nEventCount; j++)
				{
					const SKILL_EVENT * pSkillEvent = &pHolder->pEvents[j];
					ASSERT_CONTINUE(pSkillEvent);
					SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData(context.game, pSkillEvent->nType);
					ASSERT_CONTINUE(pSkillEventTypeData);
					if (pSkillEventTypeData->bCheckPetPowerCost)
					{
						ASSERT_CONTINUE(pSkillEvent->tAttachmentDef.nAttachedDefId >= 0);
						if (!PetCanAdd(context.unit, nPowerCost, context.skillData->pnSummonedInventoryLocations[0]))
						{
							return FALSE;
						}
					}
				}
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillCanTakePowerCost(
	UNIT * unit,
	int skill,
	int skilllevel,
	const SKILL_DATA * pSkillData,
	BOOL bPlaySounds)
{
	SKILL_CONTEXT context(UnitGetGame(unit), unit, skill, skilllevel, pSkillData, INVALID_ID, 0, 0);
	return sSkillDoCanTakePowerCost(context, bPlaySounds);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillCanRequest(
	SKILL_CONTEXT & context)
{
	ASSERT_RETFALSE(context.skillData);

	if( UnitIsInPVPTown(context.unit) )
	{
		if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_START_IN_PVP_TOWN))
		{
			return FALSE;
		}
	}
	else if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_START_IN_TOWN) && UnitIsInTown(context.unit))
	{
		return FALSE;
	}

	if (SkillDataTestFlag( context.skillData, SKILL_FLAG_MUST_START_IN_PORTAL_SAFE_LEVEL ) && 
		!UnitIsInPortalSafeLevel(context.unit))
	{

		return FALSE;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_USE) && !PlayerIsSubscriber(context.unit) )
	{
		return FALSE;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CANT_START_IN_PVP) && UnitPvPIsEnabled(context.unit))
	{
		return FALSE;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_NOT_DO_IN_HELLRIFT) && UnitIsInHellrift(context.unit))
	{
		return FALSE;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_DECOY_CANNOT_USE) && UnitIsDecoy(context.unit))
	{
		return FALSE;
	}

	if (AppIsHellgate())
	{
		if (UnitHasState(context.game, context.unit, STATE_QUEST_RTS) && !SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_START_IN_RTS_MODE))
		{
			return FALSE;
		}

		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_NOT_START_IN_RTS_LEVEL) && UnitIsInRTSLevel(context.unit))
		{
			return FALSE;
		}
	}

	if (UnitIsGhost(context.unit) && !SkillDataTestFlag(context.skillData, SKILL_FLAG_GHOST_CAN_DO))
	{
		return FALSE;
	}

	//in some cases the context.unit will be a monster, but the skill has been initiated by the player. In that case
	//the skill needs to have the requiredUnitType set to any ... or player/monster - since both are executing the skill.
	if( AppIsHellgate() )
	{
		if (context.skillData->nRequiredUnittype >= 0 && !UnitIsA(context.unit, context.skillData->nRequiredUnittype))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillCanStart(
	SKILL_CONTEXT & context,
	UNIT * target,
	BOOL bCheckIsOn,
	BOOL bPlaySounds = FALSE )
{
	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);
	ASSERT_RETFALSE(context.skill != INVALID_ID);

	if (UnitGetRoom(context.unit) && RoomIsActive(UnitGetRoom(context.unit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(context.unit, "Attempt to start skill in Inactive Room"))
		{
			return FALSE;
		}
	}

	const UNIT_DATA* pUnitData = UnitGetData( context.unit );
	if (!context.skillData)
	{
		context.skillData = SkillGetData(context.game, context.skill);
		ASSERT_RETFALSE(context.skillData);
	}

	// check that it can be done by 
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_SERVER_ONLY) && !IS_SERVER(context.game))
	{
		return FALSE;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CLIENT_ONLY) && !IS_CLIENT(context.game))
	{
		return FALSE;
	}

	if (!sSkillCanRequest(context))
	{
		return FALSE;
	}

	if (UnitGetStat(context.unit, STATS_PREVENT_ALL_SKILLS) > 0 && !SkillDataTestFlag(context.skillData, SKILL_FLAG_IGNORE_PREVENT_ALL_SKILLS))
	{
		return FALSE;
	}

	// can the skill be done while dead?
	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_DEAD_CAN_DO) && IsUnitDeadOrDying(context.unit))
	{
		return FALSE;
	}

	// server doesn't set on ground for players
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_ON_GROUND_ONLY) && !UnitIsOnGround(context.unit))
	{
		return FALSE;
	}

	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_GETHIT_CAN_DO) && UnitTestModeGroup(context.unit, MODEGROUP_GETHIT))
	{
		return FALSE;
	}
	
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_MOVING_CANT_DO) && UnitTestModeGroup(context.unit, MODEGROUP_MOVE))
	{
		return FALSE;
	}

	if (context.skillData->nRequiredState != INVALID_ID && !UnitHasState(context.game, context.unit, context.skillData->nRequiredState))
	{
		return FALSE;
	}

	for(int i=0; i<MAX_PROHIBITING_STATES; i++)
	{
		if (context.skillData->nProhibitingStates[i] != INVALID_ID && UnitHasState(context.game, context.unit, context.skillData->nProhibitingStates[i]))
		{
			return FALSE;
		}
	}

	if (bCheckIsOn &&
		SkillDataTestFlag(context.skillData, SKILL_FLAG_SKILL_IS_ON) && 
		SkillIsOn(context.unit, context.skill, context.idWeapon) &&
		SkillDataTestFlag(context.skillData, SKILL_FLAG_HAS_EVENT_TRIGGER ) == FALSE )	//skills can be triggered multiple times if they have Event Triggers.
	{
		return FALSE;
	}

	if (context.skillData->nExtraSkillToTurnOn != INVALID_ID &&
		context.skillData->nExtraSkillToTurnOn != context.skill )
	{
		SKILL_CONTEXT t_context(context.game, context.unit, context.skillData->nExtraSkillToTurnOn, 0, INVALID_ID, (context.dwFlags & ~SKILL_START_FLAG_IGNORE_COOLDOWN), 0);
		if (!sSkillCanStart(t_context, target, bCheckIsOn))
		{
			return FALSE;
		}
	}

	sSkillGetSkillLevel(context);

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(context.game, DATATABLE_SKILLS, context.skillData->codeStartCondition, &code_len);
	if (code_ptr && !VMExecI(context.game, context.unit, target, NULL, context.skill, context.skillLevel, context.skill, context.skillLevel, INVALID_ID, code_ptr, code_len))
	{
		return FALSE;
	}

	sSkillGetWeapon(context);

	// does this skill need a weapon?
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON))
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_WEAPON_IS_REQUIRED))
		{
			if (context.idWeapon == INVALID_ID)
			{
				UNIT * weapons[MAX_WEAPON_LOCATIONS_PER_SKILL];
				UnitGetWeapons(context.unit, context.skill, weapons, (context.dwFlags & SKILL_START_FLAG_IGNORE_COOLDOWN) == 0);
				if (!weapons[0])
				{
					return FALSE;
				}
				if (SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON_SKILL) && ItemGetPrimarySkill(weapons[0]) == INVALID_ID)
				{
					return FALSE;
				}
			}
			else if (!context.weapon)
			{
				return FALSE;
			}
		}

		if (AppIsHellgate() && UnitTestFlag(context.unit, UNITFLAG_CAN_REQUEST_LEFT_HAND) == FALSE)
		{
			// when you have two swords, you swing them both at once.  The left one doesn't start a skill.
			int nAnimationGroup = UnitGetAnimationGroup(context.unit);
			if (nAnimationGroup != INVALID_ID && 
				context.skillData->pnWeaponIndices[0] == WEAPON_INDEX_LEFT_HAND &&
				context.skillData->pnWeaponIndices[1] == INVALID_ID)
			{
				const ANIMATION_GROUP_DATA * animationGroupData = (const ANIMATION_GROUP_DATA *)ExcelGetData(context.game, DATATABLE_ANIMATION_GROUP, nAnimationGroup);
				if (!animationGroupData->bCanStartSkillWithLeftWeapon)
				{
					return FALSE;
				}
			}
		}
	} 

	// check cooldown
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_IGNORE_COOLDOWN_ON_START) == FALSE &&
		(context.dwFlags & SKILL_START_FLAG_IGNORE_COOLDOWN) == 0 &&
		(IS_SERVER(context.game) || (context.dwFlags & SKILL_START_FLAG_INITIATED_BY_SERVER) == 0))
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON_COOLDOWN))
		{
			// is the weapon cooling if they have one?
			if (context.weapon && 
				(UnitGetStatAny(context.weapon, STATS_SKILL_COOLING, NULL) || UnitGetStatAny(context.weapon, STATS_SKILL_GROUP_COOLING, NULL)))
			{
				return FALSE;
			}
		} 
		else if (UnitGetStat(context.unit, STATS_SKILL_COOLING, context.skill))
		{

#if !ISVERSION(SERVER_VERSION)
			if (bPlaySounds && pUnitData->m_nCantCastYetSound != INVALID_ID)
			{
				c_SoundPlay(pUnitData->m_nCantCastYetSound, &c_UnitGetPosition(context.unit), NULL);
			}
#endif
			return FALSE;
		}
		else if (context.skillData->nCooldownSkillGroup != INVALID_ID && UnitGetStat(context.unit, STATS_SKILL_GROUP_COOLING, context.skillData->nCooldownSkillGroup))
		{
			return FALSE;
		}
	}

	// check power
	if(!sSkillDoCanTakePowerCost(context, bPlaySounds))
	{
		return FALSE;
	}
	
	target = target ? target : SkillGetTarget(context.unit, context.skill, context.idWeapon);
	BOOL bIsInRange = TRUE; // this time we check the range
	if (target && SkillDataTestFlag(context.skillData, SKILL_FLAG_VERIFY_TARGET) &&
		!SkillIsValidTarget(context.game, context.unit, target, NULL, context.skill, TRUE, &bIsInRange ))
	{		
		target = NULL;
	}
	// in tug, we want to verify it is in range too.
	if( !bIsInRange && AppIsTugboat() )
	{
		target = NULL;
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT) && !target && UnitIsA(context.unit, UNITTYPE_MONSTER))
	{
		return FALSE;
	}

	// this is hellgate only so that shift-skills work in tugboat
	if (AppIsHellgate() && SkillDataTestFlag(context.skillData, SKILL_FLAG_MUST_TARGET_UNIT))
	{
		if (!target)
		{
			return FALSE;
		}

		float fRange = SkillGetRange(context.unit, context.skill, context.weapon, FALSE);
		if(context.skillData->fRangeMin != 0 || fRange != 0)
		{ //Only danger here is if both min and max range are 0; in that case...well, account for float margin of error in the range...>_>
			if (!UnitsAreInRange(context.unit, target, context.skillData->fRangeMin, fRange, NULL))
			{
				return FALSE;
			}
		}
	}

	// check we have room for summons
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CHECK_INVENTORY_SPACE))
	{ 
		int nSummonedInvLoc = context.skillData->pnSummonedInventoryLocations[0];
		if (nSummonedInvLoc != INVALID_LINK)
		{
		
			// this only makes sense with one inventory location
			int nMaxCount = UnitInventoryGetArea(context.unit, nSummonedInvLoc);
			if (nMaxCount > 0)
			{
				// count up the creatures that are alive and in the world to get the current count
				// note: we will have squires or turrets in our inventory if we had them in a previous
				// game and they died or the player left the game.  We do this because we want to
				// be able to resurrect the exact same pet in later games
				int nCurrentAliveCount = UnitInventoryLocCountAliveInWorld( context.unit, nSummonedInvLoc );				
				if (nCurrentAliveCount >= nMaxCount)
				{
					return FALSE;
				}
			}
		}
	}

	if(SkillDataTestFlag(context.skillData, SKILL_FLAG_REQUIRE_PATH_TO_TARGET))
	{
		if(IS_SERVER(context.game))
		{
			DWORD dwPathFlags = 0;
			SETBIT(dwPathFlags, PCF_TEST_ONLY_BIT);
			UNITID idOldMoveTarget = UnitGetMoveTargetId(context.unit);
			UnitSetMoveTarget(context.unit, UnitGetId(target));
			BOOL bSuccess = UnitCalculatePath(context.game, context.unit, PATH_FULL, dwPathFlags);
			UnitSetMoveTarget(context.unit, idOldMoveTarget);
			if(!bSuccess)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillCanStart(
	GAME * game,
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	UNIT * target,
	BOOL bCheckIsOn,
	BOOL bPlaySounds,
	DWORD dwSkillStartFlags)
{
	SKILL_CONTEXT context(game, unit, skill, 0, idWeapon, dwSkillStartFlags, 0);
	return sSkillCanStart(context, target, bCheckIsOn, bPlaySounds );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL c_SkillGetIconInfo( 
	UNIT * pUnit, 
	int nSkill,
	BOOL & bIconIsRed,
	float & fCooldownScale,
	int & nTicksRemaining,
	BOOL bCheckFallbacks )
{
	if ( ! pUnit )
		return FALSE;

	nTicksRemaining = 0;
	bIconIsRed = TRUE;
	fCooldownScale = 1.0f;

	GAME * pGame = UnitGetGame( pUnit );
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if (pSkillData == NULL)
	{
		return FALSE;
	}
	
	int nEndTime = 0;
	if ( pSkillData->nExtraSkillToTurnOn != INVALID_ID )
	{
		nEndTime = UnitGetStat(pUnit, STATS_SKILL_COOLING, pSkillData->nExtraSkillToTurnOn );
	}

	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

	if ( nEndTime <= 0 )
	{
		if ( pWeapons[ 0 ] && SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_COOLDOWN ) ) 
			nEndTime = UnitGetStatAny(pWeapons[ 0 ], STATS_SKILL_COOLING, NULL);
		else 
			nEndTime = UnitGetStat(pUnit, STATS_SKILL_COOLING, nSkill);
	} 

	if ( nEndTime <= 0 )
	{
		for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
		{
			nEndTime = max( nEndTime, UnitGetStat(pUnit, STATS_SKILL_GROUP_COOLING, pSkillData->pnSkillGroup[ i ] ));
		}
	} 

	int nCurTime = GameGetTick(UnitGetGame(pUnit));
	if ( nEndTime && nEndTime > nCurTime )
	{
		nTicksRemaining = nEndTime - nCurTime;
		//float fIconGrowTime = 2.0f;
		//if( AppIsHellgate() )
		//{
		//	fCooldownScale = PIN( ((GAME_TICKS_PER_SECOND_FLOAT * fIconGrowTime) - nTicksRemaining) / (GAME_TICKS_PER_SECOND_FLOAT * fIconGrowTime), 0.3f, 1.0f );
		//}
		//else
		{
			int nLevel = AppIsTugboat() ? SkillGetLevel( pUnit, nSkill ) : INVALID_SKILL_LEVEL;
			int coolDownTicks = SkillGetCooldown( pGame, pUnit, pWeapons[ 0 ], nSkill, pSkillData, 0, nLevel );
			fCooldownScale = PIN( (float)(coolDownTicks - nTicksRemaining) / (float)coolDownTicks, 0.0f, 1.0f );
		}
	}

	if ((fCooldownScale < 1.0f || nTicksRemaining >= 6) )
	{
		bIconIsRed = TRUE;
	}
	else
	{
		bIconIsRed = FALSE;
	}

	UNIT * pTarget = UIGetTargetUnit();

	if ( pTarget &&
		SkillDataTestFlag(pSkillData, SKILL_FLAG_VERIFY_TARGET) &&
		! SkillIsValidTarget(pGame, pUnit, pTarget, NULL, nSkill, TRUE ) )
	{
		pTarget = NULL;
	}

	if ( ! pTarget && SkillDataTestFlag( pSkillData, SKILL_FLAG_FIND_TARGET_UNIT ))
	{
		float fRangeMax = SkillGetRange( pUnit, nSkill, NULL );

		DWORD pdwSearchFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
		ZeroMemory( pdwSearchFlags, sizeof(DWORD) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
		if ( ! AppIsHellgate() )
		{
			SETBIT( pdwSearchFlags, SKILL_TARGETING_BIT_CHECK_DIRECTION );
		}
		//check direction for tugboat
		pTarget = SkillGetNearestTarget( pGame, pUnit, pSkillData, fRangeMax, NULL, pdwSearchFlags );
	}

	if ( ! SkillCanStart( pGame, pUnit, nSkill, INVALID_ID, pTarget, FALSE, FALSE, SKILL_START_FLAG_IGNORE_COOLDOWN | SKILL_START_FLAG_IGNORE_POWER_COST ) )
	{
		bIconIsRed = TRUE;
		if ( ! bCheckFallbacks || SkillDataTestFlag( pSkillData, SKILL_FLAG_UI_IS_RED_ON_FALLBACK ) )
		{
			return FALSE;
		}

		for ( int i = 0; i < NUM_FALLBACK_SKILLS; i++ )
		{
			if ( pSkillData->pnFallbackSkills[ i ] == INVALID_ID )
				continue;
			int nTicksRemainingOnFallback = 0;
			float fCooldownScaleOnFallback = 0.0f;
			BOOL bIconIsRedOnFallback = FALSE;
			if ( c_SkillGetIconInfo( pUnit, pSkillData->pnFallbackSkills[ i ], bIconIsRedOnFallback, fCooldownScaleOnFallback, nTicksRemainingOnFallback, FALSE ) )
			{
				if ( !bIconIsRedOnFallback )
					bIconIsRed = FALSE;
			}
		}
	} 
	else if ( ! SkillCanStart( pGame, pUnit, nSkill, INVALID_ID, pTarget, FALSE, FALSE, 0 ) )
	{
		bIconIsRed = TRUE;
	}

	return TRUE;
}
#endif // !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillSelectMeleeSkill( 
	UNIT * pUnit, 
	int nSkill, 
	UNITID idTarget )
{
	ASSERT_RETURN( IS_CLIENT( pUnit ) );
#if !ISVERSION(SERVER_VERSION)
	if ( pUnit != GameGetControlUnit( UnitGetGame( pUnit ) ) )
		return;

	int nMeleeSkill = c_SkillsFindActivatorSkill( SKILL_ACTIVATOR_KEY_MELEE );

	UnitSetStat( pUnit, STATS_SKILL_MELEE_SKILL, nMeleeSkill );

	MSG_CCMD_SKILLPICKMELEESKILL tMsg;
	tMsg.wSkill = (WORD) nMeleeSkill;
	c_SendMessage( CCMD_SKILLPICKMELEESKILL, &tMsg );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillFindFallbackSkill(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTarget,
	const SKILL_DATA * pSkillData)
{
	int nFallbackSkill = INVALID_ID;
	for ( int jj = 0; jj < NUM_FALLBACK_SKILLS; jj++ )
	{
		if ( pSkillData->pnFallbackSkills[ jj ] == INVALID_ID )
			continue;
		if ( SkillCanStart( pGame, pUnit, pSkillData->pnFallbackSkills[ jj ], INVALID_ID, pTarget, FALSE, FALSE, SKILL_START_FLAG_IGNORE_COOLDOWN ) )
		{
			nFallbackSkill = pSkillData->pnFallbackSkills[ jj ];
			break;
		}
	}
	return nFallbackSkill;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sCheckSkillIsOnWantedList(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& tEventData, 
	const struct EVENT_DATA* check_data)
{
	int nSkill = (int) tEventData.m_Data1;
	UNITID idWeapon = (UNITID)tEventData.m_Data2;

	SKILL_IS_ON * pSkillsWanted = (SKILL_IS_ON *) check_data->m_Data1;
	int nWantedCount = (int) check_data->m_Data2;

	BOOL bIsWanted = FALSE;
	for ( int i = 0; i < nWantedCount; i++ )
	{
		if ( pSkillsWanted[ i ].nSkill == nSkill )
		{
			bIsWanted = TRUE;
			break;
		}
	}

	if ( !bIsWanted && SkillGetIsOn( unit, nSkill, idWeapon ) )
		return FALSE;

	return !bIsWanted;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillUpdateActiveSkills( 
	UNIT * pUnit )
{
	
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );
	GAME * pGame = UnitGetGame( pUnit );

	// find out what skills we are doing - make a copy so that we can iterate this list and change the unit's list
	// this list is kept sorted on the unit
	SKILL_IS_ON pSkillsOn[ MAX_SKILLS_ON ];
	int nSkillsOnCount = pUnit->pSkillInfo->pSkillsOn.Count();
	ASSERT_RETURN( nSkillsOnCount < MAX_SKILLS_ON );
	if ( nSkillsOnCount )
	{
		MemoryCopy( pSkillsOn, MAX_SKILLS_ON * sizeof(SKILL_IS_ON), pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ), sizeof( SKILL_IS_ON ) * nSkillsOnCount );
		qsort( pSkillsOn, nSkillsOnCount, sizeof(SKILL_IS_ON), sCompareSkillIsOn );
	}

	BOOL bPreventOtherSkills = FALSE;
	BOOL bHoldOtherSkills = FALSE;
	int nPreventPriority = -1;
	for ( int i = 0; i < nSkillsOnCount; i++ )
	{
		const SKILL_DATA * pSkillData = SkillGetData( pGame, pSkillsOn[ i ].nSkill );
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_HOLD_OTHER_SKILLS ) )
		{
			bHoldOtherSkills = TRUE;
		}

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_PREVENT_OTHER_SKILLS_BY_PRIORITY ) &&
			 pSkillData->nPriority > nPreventPriority )
		{
			UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
			UnitGetWeapons( pUnit, pSkillsOn[ i ].nSkill, pWeapons, FALSE );
			for ( int j = 0; j < MAX_WEAPON_LOCATIONS_PER_SKILL; j++ )
			{ // this should work even if the skill doesn't use weapons
				int nSkillToUse = pSkillsOn[ i ].nSkill;
				if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_SKILL ) )
				{
					nSkillToUse = ItemGetPrimarySkill( pWeapons[ j ] );
					if ( nSkillToUse == INVALID_ID )
						continue;
				} else if ( j > 0 )
					break;

				if( SkillNeedsToHold( pUnit, pWeapons[j], nSkillToUse ) )
				{
					nPreventPriority = pSkillData->nPriority;
				}
			}
		}

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_PREVENT_OTHER_SKILLS ) )
		{
			if( AppIsHellgate() )
			{
				bPreventOtherSkills = TRUE;
			}
			else
			{ 
				// OLD - Hey Marsh or Travis, why is this here?  if the skill that prevents other skills stops, it triggers another SkillUpdateActiveSkills() on the next frame.
				// Marsh - I'm going to remove this for now and see if anything breaks. In theory it shouldn't.
				// TRAVIS - Apparently this is what made dual-wield actually WORK, so taking it out == BAD.
				UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
				UnitGetWeapons( pUnit, pSkillsOn[ i ].nSkill, pWeapons, FALSE );
				for ( int j = 0; j < MAX_WEAPON_LOCATIONS_PER_SKILL; j++ )
				{ // this should work even if the skill doesn't use weapons
					int nSkillToUse = pSkillsOn[ i ].nSkill;
					if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_SKILL ) )
					{
						nSkillToUse = ItemGetPrimarySkill( pWeapons[ j ] );
						if ( nSkillToUse == INVALID_ID )
							continue;
					} else if ( j > 0 )
						break;
	
					if( SkillNeedsToHold( pUnit, pWeapons[j], nSkillToUse ) )
					{
						bPreventOtherSkills = TRUE;
					}
				}
			}
		}
	}


	// find out what skills we are requesting
	SKILL_REQUEST pRequestedSkills[ MAX_SKILLS_ON ];
	int nRequestedSkillsCount = 0;
	{
		int nMaxPriority = nPreventPriority + 1; // we want to prevent skills that are at or below the prevent priority
		int nCount = pUnit->pSkillInfo->pRequests.Count();
		SKILL_REQUEST * pCurr = nCount ? pUnit->pSkillInfo->pRequests.GetPointer( 0 ) : NULL;
		for ( int i = 0; i < nCount; i++, pCurr++ )
		{
			int nSkill = pCurr->nSkill;
			int nRequestedSkill = pCurr->nSkill;
			const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

			UNIT * pTarget = SkillGetTarget( pUnit, pCurr->nSkill, INVALID_ID );
			DWORD dwStartFlagsToCheck = 0;
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_ALL ) )
				dwStartFlagsToCheck = SKILL_START_FLAG_IGNORE_COOLDOWN;

			if ( ! SkillCanStart( pGame, pUnit, pCurr->nSkill, INVALID_ID, pTarget, FALSE, FALSE, dwStartFlagsToCheck ) )
			{
				int nFallbackSkill = sSkillFindFallbackSkill(pGame, pUnit, pTarget, pSkillData);
				if ( nFallbackSkill != INVALID_ID )
				{
					nSkill = nFallbackSkill;
					pSkillData = SkillGetData( pGame, nSkill );
					
					// update the target for this fallback skill ... I
					// don't think this is right to go stomping on what targets might 
					// already be set for a fallback skill, but also it is not right
					// to set a fallback target and not have it be cleared -Colin
					//if ( pTarget )// && ! SkillGetTarget( pUnit, nSkill, INVALID_ID )  )
					{
						UNITID idTarget = UnitGetId( pTarget );
						SkillSetTarget( pUnit, nSkill, INVALID_ID, idTarget );
						if ( IS_CLIENT( pUnit ) && SkillDataTestFlag( pSkillData, SKILL_FLAG_SELECTS_A_MELEE_SKILL ) )
							sSkillSelectMeleeSkill( pUnit, nSkill, idTarget );
					}

				} else {
					continue;
				}
			}
			if ( pSkillData->nPriority == nMaxPriority )
			{
				ASSERT_CONTINUE( nRequestedSkillsCount < MAX_SKILLS_ON );
				pRequestedSkills[ nRequestedSkillsCount ].nSkill = nSkill;
				pRequestedSkills[ nRequestedSkillsCount ].nRequestedSkill = nRequestedSkill;
				pRequestedSkills[ nRequestedSkillsCount ].dwSeed = pCurr->dwSeed;
				nRequestedSkillsCount++;
			}
			else if ( pSkillData->nPriority > nMaxPriority )
			{
				pRequestedSkills[ 0 ].nSkill = nSkill;
				pRequestedSkills[ 0 ].nRequestedSkill = nRequestedSkill;
				pRequestedSkills[ 0 ].dwSeed = pCurr->dwSeed;
				nRequestedSkillsCount = 1;
				nMaxPriority = pSkillData->nPriority;
			}
		}
	}

	DWORD dwSkillStartFlags = 0;

	// find out what skills that really means that we want to do - there might be repeats in the array
	SKILL_IS_ON pSkillsWanted[ MAX_SKILLS_ON ];
	int nSkillsWantedCount = 0;
	BOOL bStopHoldingSkills = FALSE;
	BOOL bSkillHolding = FALSE;
	for ( int i = 0; i < nRequestedSkillsCount; i++ )
	{
		const SKILL_DATA * pSkillData = SkillGetData( pGame, pRequestedSkills[ i ].nSkill );
		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		UnitGetWeapons( pUnit, pRequestedSkills[ i ].nSkill, pWeapons, FALSE );

		BOOL bWantPreventOthers = FALSE;
		for ( int j = -1; j < MAX_WEAPON_LOCATIONS_PER_SKILL; j++ )
		{ // this should work even if the skill doesn't use weapons
			UNIT * pWeapon = NULL;
			const SKILL_DATA * pSkillDataToUse = NULL;
			int nSkillToUse = pRequestedSkills[ i ].nSkill;
			if ( j == -1 )
			{
				if ( pSkillData->nExtraSkillToTurnOn == INVALID_ID )
					continue;
				nSkillToUse = pSkillData->nExtraSkillToTurnOn;
			}
			else if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_SKILL ) )
			{
				pWeapon = pWeapons[ j ];
				nSkillToUse = ItemGetPrimarySkill( pWeapon );
				if ( nSkillToUse == INVALID_ID )
					continue;

				pSkillDataToUse = SkillGetData( pGame, nSkillToUse );

				if( AppIsHellgate() )
				{
					if ( SkillDataTestFlag( pSkillDataToUse, SKILL_FLAG_USES_WEAPON ) )
					{ // this is used by focus items in Hellgate
						UNIT * pWeaponsForcedByWeapon[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
						UnitGetWeapons( pUnit, nSkillToUse, pWeaponsForcedByWeapon, FALSE );
						pWeapon = pWeaponsForcedByWeapon[ 0 ];
					}
				}

			}
			else if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON ) )
			{
				pWeapon = pWeapons[ j ];
				if ( ! pWeapon ) 
				{
					if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) )
						break;
					else if ( j > 0 )
						break;
				}
			}
			else if ( j > 0 )
				break;

			pSkillDataToUse = pSkillDataToUse ? pSkillDataToUse : SkillGetData( pGame, nSkillToUse );
			if ( ! pSkillDataToUse )
				continue;

			if ( SkillDataTestFlag( pSkillDataToUse, SKILL_FLAG_SELECTS_A_MELEE_SKILL ) )
			{
				int nMeleeSkill = UnitGetStat( pUnit, STATS_SKILL_MELEE_SKILL );
				if ( nMeleeSkill != INVALID_ID )
				{
					nSkillToUse = nMeleeSkill;
					pSkillDataToUse = SkillGetData( pGame, nSkillToUse );
				}
			}
#if (0)
			// TODO cmarch: fix field weapon support for the Engineer's drone
			if ( AppIsHellgate() && SkillDataTestFlag( pSkillDataToUse, SKILL_FLAG_USES_WEAPON ) && !pWeapon )
			{
				for (unsigned int ii = 0; ii < MAX_WEAPON_LOCATIONS_PER_SKILL; ++ii)
				{
					for (unsigned int jj = 0; jj < MAX_WEAPON_LOCATIONS_PER_SKILL; ++jj)
					{	
						if (pSkillData->pnWeaponInventoryLocations[ii] != INVALID_ID &&
						    pSkillData->pnWeaponInventoryLocations[ii] == pSkillDataToUse->pnWeaponInventoryLocations[jj] &&
							pWeapons[ii])
						{
							pWeapon = pWeapons[ii];
						}
					}
				}
			}
#endif
			UNITID idWeapon = UnitGetId( pWeapon );
			if( AppIsTugboat() &&
				SkillNeedsToHold( pUnit, pWeapon, nSkillToUse ) )
			{
				bSkillHolding = TRUE;
			}
			if(bPreventOtherSkills && !SkillIsOn(pUnit, nSkillToUse, idWeapon))
			{
				continue;
			}

			//This is not included for Tugboat, I'm going to keep it in.
			if(SkillDataTestFlag(pSkillDataToUse, SKILL_FLAG_DISALLOW_SAME_SKILL) && 
				sSkillIsOn(pSkillsWanted, nSkillsWantedCount, nSkillToUse, INVALID_ID, TRUE) != INVALID_ID)
			{
				continue;
			}

			bWantPreventOthers = SkillDataTestFlag(pSkillDataToUse, SKILL_FLAG_PREVENT_OTHER_SKILLS);			
			bWantPreventOthers |= SkillDataTestFlag(pSkillDataToUse, SKILL_FLAG_PREVENT_OTHER_SKILLS_BY_PRIORITY);			
			bWantPreventOthers = bWantPreventOthers && ( AppIsHellgate() || bSkillHolding ); 
			if ( ! bWantPreventOthers )
			{
				if ( SkillDataTestFlag( pSkillDataToUse, SKILL_FLAG_USE_ALL_WEAPONS ) &&
					 SkillGetLeftHandWeapon( pGame, pUnit, nSkillToUse, pWeapon ) ) // if we are using both weapons, we can only do one skill
					 bWantPreventOthers = TRUE;
			}
			if(bWantPreventOthers)
			{
				nSkillsWantedCount = 0;
			}

			ASSERT_CONTINUE( nSkillsWantedCount < MAX_SKILLS_ON );
			pSkillsWanted[ nSkillsWantedCount ].nSkill = nSkillToUse;
			pSkillsWanted[ nSkillsWantedCount ].nRequestedSkill = pRequestedSkills[ i ].nRequestedSkill;
			pSkillsWanted[ nSkillsWantedCount ].idWeapon = idWeapon;
			pSkillsWanted[ nSkillsWantedCount ].dwSeed   = pRequestedSkills[ i ].dwSeed;

			UNIT * pTarget = SkillGetTarget( pUnit, pRequestedSkills[ i ].nSkill, INVALID_ID );
			// no target and need one? Let's find it.

			if( AppIsTugboat() && UnitGetGenus( pUnit ) == GENUS_PLAYER &&
				SkillDataTestFlag( pSkillData, SKILL_FLAG_FIND_TARGET_UNIT ) &&
				( !pTarget ||
				  !SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon,
									   nSkillToUse, TRUE ) ) )
			{
				SkillClearTargets( pUnit, nSkillToUse, idWeapon );
				VECTOR vTarget;
				SkillFindTarget( pGame,
								 pUnit,
								 nSkillToUse,
								 pWeapon,
								 &pTarget,
								 vTarget );
				if( pTarget )
				{
					SkillSetTarget( pUnit, nSkillToUse, idWeapon, UnitGetId( pTarget ) );
				}
			}

			if ( pTarget && SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon,
								pSkillsWanted[ nSkillsWantedCount ].nSkill, TRUE ) )
				SkillSetTarget( pUnit, pSkillsWanted[ nSkillsWantedCount ].nSkill,
					pSkillsWanted[ nSkillsWantedCount ].idWeapon, UnitGetId( pTarget ) );

			nSkillsWantedCount++;

			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_DELAY_MELEE ) )
				dwSkillStartFlags |= SKILL_START_FLAG_DELAY_MELEE;
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_STOP_HOLDING_SKILLS ) )
				bStopHoldingSkills = TRUE;

			if(bWantPreventOthers)
			{
				break;
			}
		}
		if(bWantPreventOthers)
		{
			break;
		}
	}
	qsort( pSkillsWanted, nSkillsWantedCount, sizeof(SKILL_IS_ON), sCompareSkillIsOn );

	///////////////////////////////////////////////
	// Begin Tugboat Only Code
	// do Tugboat's alternating weapon thing
	///////////////////////////////////////////////
	BOOL bFlipWeaponHandednessOnSkillStart = FALSE;
	if ( AppIsTugboat() )
	{
		if( bSkillHolding )
		{
			nSkillsWantedCount = 0;
		}
		if ( nSkillsWantedCount > 1 )
		{
			// see if we are dual melee and need to flip the weapon handedness no matter what
			BOOL bCheckForValidTargets = TRUE;
			{
				int nMeleeWeapons = 0;
				int nRangedWeapons = 0;
				for ( int i = 0; i < nSkillsWantedCount; i++ )
				{
					UNIT * pWeapon = UnitGetById( pGame, pSkillsWanted[ i ].idWeapon );
					int nWeaponSkill = ItemGetPrimarySkill( pWeapon );
					if ( nWeaponSkill == INVALID_ID )
						continue;
					const SKILL_DATA * pWeaponSkill = SkillGetData( NULL, nWeaponSkill );
					if ( SkillDataTestFlag( pWeaponSkill, SKILL_FLAG_IS_MELEE ) )
						nMeleeWeapons++;
					else
						nRangedWeapons++;
				}
				if ( nMeleeWeapons == nSkillsWantedCount )
				{
					bFlipWeaponHandednessOnSkillStart = TRUE;
					bCheckForValidTargets = FALSE;
				}
			}

			// remove skills that you shouldn't start - this assumes that ranged skills come later in the skills xls
			if ( bCheckForValidTargets )
			{
				BOOL bInMeleeRange = FALSE;
				for ( int i = 0; i < nSkillsWantedCount; i++ )
				{
					UNIT * pTarget = SkillGetTarget( pUnit, pSkillsWanted[ i ].nSkill, pSkillsWanted[ i ].idWeapon );
					UNIT * pWeapon = UnitGetById( pGame, pSkillsWanted[ i ].idWeapon );

					BOOL bIsInRange = TRUE; // this time we check the range
					const SKILL_DATA * pSkillData = SkillGetData( pGame, pSkillsWanted[ i ].nSkill );

					BOOL bTargetValid = SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, pSkillsWanted[ i ].nSkill, TRUE, &bIsInRange );
					if ( //!SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, pSkillsWanted[ i ].nSkill, TRUE, &bIsInRange ) ||
						( !bTargetValid && SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE  ) ) ||
						!bIsInRange || ( bInMeleeRange && SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_RANGED  ) ) )

					{// remove the skill from the list since you don't currently have a valid target
						if ( i < nSkillsWantedCount - 1 )
							MemoryMove( &pSkillsWanted[ i ], (nSkillsWantedCount - i) * sizeof(SKILL_IS_ON), &pSkillsWanted[ i + 1 ], (nSkillsWantedCount - i - 1) * sizeof(SKILL_IS_ON));
						i--;
						nSkillsWantedCount--;
						if ( nSkillsWantedCount < 2 )
							break;
					}
					else
					{
						// if we get here, then we have a melee weapon in range, and don't need to do any ranged attacks

						if ( bTargetValid && SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE ) )
						{
							bInMeleeRange = TRUE;
						}
					}
				}
			}
			// now make sure that you pick the weapon for the hand that you want
			if ( nSkillsWantedCount > 1 )
			{
				bFlipWeaponHandednessOnSkillStart = TRUE;

				int nDesiredWeaponIndex = UnitTestFlag( pUnit, UNITFLAG_OFFHAND ) ? WEAPON_INDEX_RIGHT_HAND : WEAPON_INDEX_LEFT_HAND;
				for ( int i = 0; i < nSkillsWantedCount; i++ )
				{
					UNIT * pWeapon = UnitGetById( pGame, pSkillsWanted[ i ].idWeapon );
					if ( ! pWeapon )
						continue;
					if ( nDesiredWeaponIndex != WardrobeGetWeaponIndex( pUnit, pSkillsWanted[ i ].idWeapon ) )
					{
						if ( i < nSkillsWantedCount - 1 )
							MemoryMove( &pSkillsWanted[ i ], (nSkillsWantedCount - i) * sizeof(SKILL_IS_ON), &pSkillsWanted[ i + 1 ], (nSkillsWantedCount - i - 1) * sizeof(SKILL_IS_ON));
						i--;
						nSkillsWantedCount--;
					}	
				}

			}
		}
	}
	///////////////////////////////////////////////
	// End Tugboat Only Code
	///////////////////////////////////////////////

	int nSkillsStarted = 0;
	int nSkillsStopped = 0;
	int nSkillsContinued = 0;

	int nTicksToNextUpdate = INVALID_ID;

	// first stop skills that we don't want anymore
	for ( int nWantedCurr = 0, nOnCurr = 0; 
		nOnCurr < nSkillsOnCount || nWantedCurr < nSkillsWantedCount; )
	{
		while ( nWantedCurr < nSkillsWantedCount &&
			( nOnCurr >= nSkillsOnCount || 
				sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) < 0 ) )
		{
			nWantedCurr++;
		}

		while ( nOnCurr < nSkillsOnCount &&
			( nWantedCurr >= nSkillsWantedCount || 
						sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) > 0 ) )
		{// stop this skill - there should be no duplicates to check
			// first check and see if we need to hold it

			UNIT * pWeapon = UnitGetById( pGame, pSkillsOn[ nOnCurr ].idWeapon );
			int nTicksToHold = bStopHoldingSkills ? 0 : SkillNeedsToHold( pUnit, pWeapon, pSkillsOn[ nOnCurr ].nSkill );
			ASSERT( nTicksToHold >= 0 );

			const SKILL_DATA * pSkillData = SkillGetData( pGame, pSkillsOn[ nOnCurr ].nSkill );

			if ( bHoldOtherSkills && ! SkillDataTestFlag( pSkillData, SKILL_FLAG_HOLD_OTHER_SKILLS ) )
			{
				// Do nothing
			}
			else if(pSkillData->nHoldWithMode >= 0 && UnitTestMode(pUnit, (UNITMODE)pSkillData->nHoldWithMode))
			{
				// Do nothing
			}
			else if ( ! nTicksToHold )
			{
				nSkillsStopped++;
				SkillStop( pGame, pUnit, pSkillsOn[ nOnCurr ].nSkill, pSkillsOn[ nOnCurr ].idWeapon, FALSE, FALSE );
				if ( bPreventOtherSkills && SkillDataTestFlag( pSkillData, SKILL_FLAG_PREVENT_OTHER_SKILLS ) ) // we need to update again so that we can turn on other skills
					nTicksToNextUpdate = 1; 
				if ( bHoldOtherSkills && SkillDataTestFlag( pSkillData, SKILL_FLAG_HOLD_OTHER_SKILLS ) ) // we need to update again in case we need to turn off other skills
					nTicksToNextUpdate = 1; 
			} 
			else if ( nTicksToNextUpdate == INVALID_ID || nTicksToHold < nTicksToNextUpdate )
			{
				nTicksToNextUpdate = nTicksToHold;
			}
			nOnCurr++;
		}

		if ( nOnCurr < nSkillsOnCount && nWantedCurr < nSkillsWantedCount && 
			sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) == 0 ) 
		{ // keep doing this skill
			nSkillsContinued++;
			nOnCurr++;
			nWantedCurr++;
		}
	}

	// remove repeating skills that aren't wanted
	UnitUnregisterTimedEvent( pUnit, sHandleSkillRepeat, sCheckSkillIsOnWantedList, EVENT_DATA( DWORD_PTR(&pSkillsWanted[ 0 ]), nSkillsWantedCount) );

	// now check the remaining skills for double-use of weapons
	for ( int i = 0; i < nSkillsWantedCount; i++ )
	{
		UNITID idWeapon = pSkillsWanted[i].idWeapon;
		int nSkill = pSkillsWanted[i].nSkill;
		BOOL bAllowWant = TRUE;
		for(int k=0; k<nSkillsWantedCount; k++)
		{
			if(pSkillsWanted[k].idWeapon == idWeapon &&
				pSkillsWanted[k].nSkill != nSkill)
			{
				bAllowWant = FALSE;
				break;
			}
		}
		if(bAllowWant)
		{
			for(int k=0; k<nSkillsOnCount; k++)
			{
				if(SkillIsOn(pUnit, pSkillsOn[k].nSkill, pSkillsOn[k].idWeapon) &&
					pSkillsOn[k].idWeapon == idWeapon &&
					pSkillsOn[k].nSkill != nSkill)
				{
					bAllowWant = FALSE;
					break;
				}
			}
		}
		if(bAllowWant)
		{
			continue;
		}

		if(i < nSkillsWantedCount-1)
		{
			MemoryMove(&pSkillsWanted[i], (MAX_SKILLS_ON - i) * sizeof(SKILL_IS_ON), &pSkillsWanted[i+1], (nSkillsWantedCount-i) * sizeof(SKILL_IS_ON));
		}
		nSkillsWantedCount--;
		i--;
	}

	// start skills that we want to start
	for ( int nWantedCurr = 0, nOnCurr = 0; 
		nOnCurr < nSkillsOnCount || nWantedCurr < nSkillsWantedCount; )
	{
		while ( nWantedCurr < nSkillsWantedCount &&
			( nOnCurr >= nSkillsOnCount || 
				sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) < 0 ) )
		{
			BOOL bSameAsPrevious = FALSE;
			if ( nWantedCurr != 0 )
			{// there can be duplicates in this sorted list
				if ( pSkillsWanted[ nWantedCurr ].idWeapon == pSkillsWanted[ nWantedCurr - 1 ].idWeapon &&
					 pSkillsWanted[ nWantedCurr ].nSkill   == pSkillsWanted[ nWantedCurr - 1 ].nSkill)
					 bSameAsPrevious = TRUE;
			}
			if ( ! bSameAsPrevious && ! bPreventOtherSkills )
			{
				nSkillsStarted++;
				if ( sSkillStart( pGame, pUnit, pSkillsWanted[ nWantedCurr ].nSkill, pSkillsWanted[ nWantedCurr ].nRequestedSkill, 
					pSkillsWanted[ nWantedCurr ].idWeapon, dwSkillStartFlags, pSkillsWanted[ nWantedCurr ].dwSeed ))
				{
					const SKILL_DATA * pSkillData = SkillGetData( pGame, pSkillsWanted[ nWantedCurr ].nSkill );

					if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_DONT_STAGGER ) )
					{
						dwSkillStartFlags |= SKILL_START_FLAG_STAGGER;
					}
					///////////////////////////////////////////////
					// begin Tugboat Only code for weapon alternating
					///////////////////////////////////////////////
					if ( AppIsTugboat() && bFlipWeaponHandednessOnSkillStart )
						UnitSetFlag( pUnit, UNITFLAG_OFFHAND, ! UnitTestFlag( pUnit, UNITFLAG_OFFHAND ) );
					///////////////////////////////////////////////
					// end Tugboat Only code for weapon alternating
					///////////////////////////////////////////////
				}
			}
			nWantedCurr++;
		}

		while ( nOnCurr < nSkillsOnCount &&
			( nWantedCurr >= nSkillsWantedCount || 
						sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) > 0 ) )
		{
			nOnCurr++;
		}

		if ( nOnCurr < nSkillsOnCount && nWantedCurr < nSkillsWantedCount && 
			sCompareSkillIsOn( &pSkillsWanted[ nWantedCurr ], &pSkillsOn[ nOnCurr ] ) == 0 ) 
		{ // keep doing this skill
			SKILL_IS_ON * pContinueSkillIsOn = SkillIsOn(pUnit, pSkillsWanted[ nWantedCurr ].nSkill, pSkillsWanted[ nWantedCurr ].idWeapon, FALSE);
			if(pContinueSkillIsOn)
			{
				pContinueSkillIsOn->nRequestedSkill = pSkillsWanted[ nWantedCurr ].nRequestedSkill;
			}
			nSkillsContinued++;
			nOnCurr++;
			nWantedCurr++;
		}
	}

	UnitUnregisterTimedEvent( pUnit, sUpdateActiveSkills );

	if ( nTicksToNextUpdate == INVALID_ID && SkillGetNumOn( pUnit, FALSE ) > 0 )
	{
		nTicksToNextUpdate = GAME_TICKS_PER_SECOND; // just to make sure that nothing falls through the cracks.  We were seeing skills stuck on for a while.
	}

	if ( nTicksToNextUpdate != INVALID_ID )
	{
		UnitRegisterEventTimed( pUnit, sUpdateActiveSkills, NULL, nTicksToNextUpdate );
	}

	//trace("SkillUpdateActiveSkills[%c]: UNIT:" ID_PRINTFIELD "Requested:%d On:%d->%d Start:%d Stop:%d Continue:%d TIME:%d\n",
	//	IS_SERVER(pGame) ? 'S' : 'C',
	//	ID_PRINT(UnitGetId(pUnit)),
	//	pUnit->pSkillInfo->pRequests.Count(),
	//	nSkillsOnCount, pUnit->pSkillInfo->pSkillsOn.Count(),
	//	nSkillsStarted, nSkillsStopped, nSkillsContinued,
		//GameGetTick(UnitGetGame(pUnit)));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUpdateActiveSkills(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	SkillUpdateActiveSkills( pUnit );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillUpdateActiveSkillsDelayed(
	UNIT *pUnit)
{
	UnitUnregisterTimedEvent( pUnit, sUpdateActiveSkills );
	UnitRegisterEventTimed( pUnit, sUpdateActiveSkills, NULL, 1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillSetOnFlag( 
	UNIT * pUnit, 
	int nSkill,
	int nRequestedSkill,
	UNITID idWeapon,
	DWORD dwSeed,
	GAME_TICK tiTick,
	BOOL bSet )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );
	int nCount = pUnit->pSkillInfo->pSkillsOn.Count();

	SKILL_IS_ON * pSkillIsOn = nCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pSkillIsOn++ )
	{
		if ( pSkillIsOn->nSkill == nSkill &&
			pSkillIsOn->idWeapon == idWeapon )
		{
			if ( ! bSet )
			{
				ArrayRemoveByIndex(pUnit->pSkillInfo->pSkillsOn, i); // this will hose the pSkillIsOn pointer
			}
			return;
		}
	}
	if ( ! bSet )
		return;

	SKILL_IS_ON tSkillIsOn;
	tSkillIsOn.dwSeed = dwSeed;
	tSkillIsOn.nSkill = nSkill;
	tSkillIsOn.nRequestedSkill = nRequestedSkill;
	tSkillIsOn.idWeapon = idWeapon;
	tSkillIsOn.tiStartTick = tiTick;
	tSkillIsOn.dwFlags = 0;
	ArrayAddItem(pUnit->pSkillInfo->pSkillsOn, tSkillIsOn);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsRequested( 
	UNIT * pUnit, 
	int nSkill )
{
	ASSERT_RETFALSE( pUnit );
	ASSERT_RETFALSE( pUnit->pSkillInfo );
	int nCount = pUnit->pSkillInfo->pRequests.Count();
	SKILL_REQUEST * pRequest = nCount ? pUnit->pSkillInfo->pRequests.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pRequest++ )
	{
		if ( pRequest->nSkill == nSkill )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillRequestAdd( 
	UNIT * pUnit, 
	int nSkill,
	DWORD dwSeed)
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );

	// we are allowing multiple requests for the same skill
	// TRAVIS: why not do this?
	if ( AppIsTugboat() && SkillIsRequested( pUnit, nSkill ) )
		return;

	SKILL_REQUEST tRequest;
	tRequest.nSkill = nSkill;
	tRequest.dwSeed = dwSeed;

	ArrayAddItem(pUnit->pSkillInfo->pRequests, tRequest);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillRequestRemove( 
	UNIT * pUnit, 
	int nSkill )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pRequests.Count();

	SKILL_REQUEST * pRequest = nCount ? pUnit->pSkillInfo->pRequests.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pRequest++ )
	{
		if ( pRequest->nSkill == nSkill )
		{
			ArrayRemoveByIndex(pUnit->pSkillInfo->pRequests, i);
			return;
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillSetRequested( 
	UNIT * pUnit, 
	int nSkill,
	DWORD dwSeed,
	BOOL bOn )
{
	if ( bOn )
		sSkillRequestAdd   ( pUnit, nSkill, dwSeed );
	else
		sSkillRequestRemove( pUnit, nSkill );

	SkillUpdateActiveSkills( pUnit );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SKILL_TARGET * sSkillFindTarget(
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	int index)
{
	ASSERT_RETNULL(unit);
	SKILL_INFO * skillinfo = unit->pSkillInfo;
	ASSERT_RETNULL(skillinfo);

	int nCount = skillinfo->pTargets.Count();

	SKILL_TARGET * target = nCount ? skillinfo->pTargets.GetPointer(0) : NULL;
	for (int i = 0; i < nCount; i++, target++)
	{
		if (target->nSkill == skill &&
			target->idWeapon == idWeapon &&
			target->nIndex == index)
		{
			return target;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSkillGetTarget(
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	int index,
	DWORD * pdwFlags )
{
	SKILL_TARGET * target = sSkillFindTarget(unit, skill, idWeapon, index);
	if (target)
	{
		if ( pdwFlags )
			*pdwFlags = target->dwFlags;
		return UnitGetById(UnitGetGame(unit), target->idTarget);
	}

	if ( pdwFlags )
		*pdwFlags = 0;
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITID sSkillGetTargetId(
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	int index )
{
	SKILL_TARGET * target = sSkillFindTarget(unit, skill, idWeapon, index);
	if (target)
		return  target->idTarget;

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSkillGetTarget(
	SKILL_CONTEXT & context,
	int index = 0,
	DWORD * pdwFlags = NULL )
{
	return sSkillGetTarget(context.unit, context.skill, context.idWeapon, index, pdwFlags);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * SkillGetTarget(
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	int index, // = 0
	DWORD * pdwFlags )		// = NULL
{
	return sSkillGetTarget(unit, skill, idWeapon, index, pdwFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID SkillGetTargetId(
	UNIT * unit,
	int skill,
	UNITID idWeapon,
	int index )
{
	return sSkillGetTargetId(unit, skill, idWeapon, index);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int SkillGetTargetIndex(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	UNITID idTarget )
{
	ASSERT_RETINVALID( pUnit );
	ASSERT_RETINVALID( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pTargets.Count();

	SKILL_TARGET * pTarget = nCount ? pUnit->pSkillInfo->pTargets.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++ )
	{
		if ( pTarget->nSkill == nSkill &&
			 pTarget->idWeapon == idWeapon &&
			 pTarget->idTarget == idTarget )
		{
			return pTarget->nIndex;
		}
		pTarget++;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNIT * SkillGetAnyTarget(
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon,
	BOOL bCheckValid )
{	
	ASSERT_RETNULL( pUnit );
	ASSERT_RETNULL( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pTargets.Count();

	GAME * pGame = UnitGetGame( pUnit );

	SKILL_TARGET * pSkillTarget = nCount ? pUnit->pSkillInfo->pTargets.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pSkillTarget++ )
	{
		if ( pWeapon && pSkillTarget->idWeapon != UnitGetId( pWeapon ) )
			continue;

		if ( nSkill == INVALID_ID || pSkillTarget->nSkill == nSkill )
		{
			if ( bCheckValid )
			{
				UNIT * pTarget		= UnitGetById( pGame, pSkillTarget->idTarget );
				UNIT * pWeapon		= UnitGetById( pGame, pSkillTarget->idWeapon );
				if ( pTarget && SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, pSkillTarget->nSkill, FALSE ))
					return pTarget;
			} else {
				return UnitGetById( pGame, pSkillTarget->idTarget );
			}
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNIT * SkillGetAnyValidTarget(
	UNIT * pUnit,
	int nSkill )
{	
	ASSERT_RETNULL( pUnit );
	ASSERT_RETNULL( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pTargets.Count();

	GAME * pGame = UnitGetGame( pUnit );

	SKILL_TARGET * pSkillTarget = nCount ? pUnit->pSkillInfo->pTargets.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pSkillTarget++ )
	{
		UNIT * pTarget = UnitGetById( pGame, pSkillTarget->idTarget );
		if ( pTarget && 
			 ( nSkill == INVALID_ID ||
			   SkillIsValidTarget( pGame, pUnit, pTarget, NULL, nSkill, FALSE ) ) )
			return pTarget;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SkillSetTarget(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	UNITID idTarget,
	int nIndex )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->pSkillInfo );
 
	//GAME * pGame = UnitGetGame( pUnit );
	//trace( 
	//	"%s SkillSetTarget - Skill:%s[%d] Weapon:%s[%d] Target:%s[%d]\n", 
	//	IS_SERVER(pUnit) ? "SERVER" : "CLIENT", 
	//	SkillGetDevName( pGame, nSkill ),
	//	nSkill, 
	//	UnitGetDevName( pGame, idWeapon ),
	//	idWeapon,
	//	UnitGetDevName( pGame, idTarget ), 
	//	idTarget)

	int nCount = pUnit->pSkillInfo->pTargets.Count();

	// some skills require that you stay with one target.  You can go from no target to a target, but not from target to target or target to no target
	BOOL bCanSwitch = TRUE;
	const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CANNOT_RETARGET ) &&
		SkillGetIsOn( pUnit, nSkill, idWeapon, FALSE ) )
		bCanSwitch = FALSE;
	if ( ! bCanSwitch && idTarget == INVALID_ID )
		return;

	BOOL bAdd = TRUE;
	BOOL bChanged = TRUE;

	SKILL_TARGET * pSkillTarget = nCount ? pUnit->pSkillInfo->pTargets.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nCount; i++, pSkillTarget++ )
	{
		if ( pSkillTarget->nSkill == nSkill &&
			 pSkillTarget->idWeapon == idWeapon &&  //tugboat had an OR checking if idWeapon was invalid. I don't see why you can't set a targt wth an invalid weapon ID. So I'm leaving it out
			 pSkillTarget->nIndex == nIndex )
		{
			if ( ! bCanSwitch )
			{
				// KCK: We should allow a retarget if the item no longer exists
				// KCK NOTE: This code being here seems really wrong, as it will allow the target lists on our
				// client and server to get out of sync. It seems like we'd be better of keeping them in sync
				// and making the decision to retarget or not within sSkillEventAimLaser(), however that's a
				// much larger change to make.
				if ( pSkillTarget->idTarget != idTarget && pSkillTarget->idTarget != INVALID_ID &&
					 UnitGetById(UnitGetGame(pUnit), pSkillTarget->idTarget))
					return;
			}

			if ( idTarget == INVALID_ID )
			{
				ArrayRemoveByIndex(pUnit->pSkillInfo->pTargets, i);
				pSkillTarget = NULL;  // this has been freed by the removal and is now invalid
			}
			else if ( pSkillTarget->idTarget != idTarget )
			{
				pSkillTarget->idTarget = idTarget;
				pSkillTarget->dwFlags = 0;
			}
			else
			{
				bChanged = FALSE;
			}
			bAdd = FALSE;
			break;
		}
	}

	if ( ! bAdd && idTarget == INVALID_ID )
		bChanged = FALSE;

	if ( idTarget == INVALID_ID )
		bAdd = FALSE;

	if ( bAdd )
	{
		SKILL_TARGET tSkillTarget;
		tSkillTarget.idTarget = idTarget;
		tSkillTarget.idWeapon = idWeapon;
		tSkillTarget.nSkill   = nSkill;
		tSkillTarget.nIndex   = nIndex;
		tSkillTarget.dwFlags  = 0;

		ArrayAddItem(pUnit->pSkillInfo->pTargets, tSkillTarget);
	}

	GAME * pGame = UnitGetGame( pUnit );
	if ( IS_CLIENT( pUnit ) && bChanged && pUnit == GameGetControlUnit( pGame ) )
	{
		CLT_VERSION_ONLY(c_SendSkillChangeWeaponTarget( (WORD) nSkill, idWeapon, idTarget, nIndex ));
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SkillClearTargets(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon )
{
	ASSERT_RETURN( pUnit );
	if( pUnit->pSkillInfo == NULL ) //some skills have pSkillInfo as null
		return;
	//ASSERT_RETURN( pUnit->pSkillInfo );

	//GAME * pGame = UnitGetGame( pUnit );
	//trace( 
	//	"%s SkillClearTargets - Skill:%s[%d] Weapon:%s[%d]\n", 
	//	IS_SERVER(pUnit) ? "SERVER" : "CLIENT", 
	//	SkillGetDevName( pGame, nSkill ),
	//	nSkill, 
	//	UnitGetDevName( pGame, idWeapon ),
	//	idWeapon);
		
	for ( UINT i = 0; i < pUnit->pSkillInfo->pTargets.Count(); i++ )
	{
		SKILL_TARGET * pSkillTarget = pUnit->pSkillInfo->pTargets.GetPointer( i );
		if ( pSkillTarget->nSkill == nSkill &&
			 pSkillTarget->idWeapon == idWeapon )
		{
			ArrayRemoveByIndex(pUnit->pSkillInfo->pTargets, i); // this hoses our pSkillTarget - so don't use it after this
			i--;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_SkillControlUnitChangeTarget( 
	UNIT * pTarget )
{
	GAME * pGame = AppGetCltGame();
	if ( !pGame )
		return;
	UNIT * pUnit = GameGetControlUnit( pGame );
	if ( ! pUnit )
		return;

	ASSERT_RETURN( pUnit->pSkillInfo );
	int nSkillIsOnCount = pUnit->pSkillInfo->pSkillsOn.Count();
	const SKILL_IS_ON * pSkillIsOn = nSkillIsOnCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;
	for ( int i = 0; i < nSkillIsOnCount; i++, pSkillIsOn++)
	{
		int nSkill = pSkillIsOn->nSkill;
		SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
		if ( SkillGetTarget( pUnit, nSkill, pSkillIsOn->idWeapon ) != pTarget &&
			SkillIsValidTarget( pGame, pUnit, pTarget, UnitGetById( pGame, pSkillIsOn->idWeapon), 
				nSkill, AppIsHellgate(), NULL ) ) //tugboat passes false in.
		{
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_TARGET_INDEX ) && pTarget &&
				 SkillGetTargetIndex( pUnit, nSkill, pSkillIsOn->idWeapon, UnitGetId( pTarget ) ) > 0 )
			{
				continue;
			}
			SkillSetTarget( pUnit, nSkill, pSkillIsOn->idWeapon, UnitGetId( pTarget ) );
		}
	}
	//TODO: The below code looks like it was added pretty recently before tugboat and hellgate merge. This might be needed for tugboat. Need to ask Tyler
	if( AppIsHellgate() )
	{
		int nRequestCount = pUnit->pSkillInfo->pRequests.Count();
		const SKILL_REQUEST * pRequest = nRequestCount ? pUnit->pSkillInfo->pRequests.GetPointer( 0 ) : NULL;
		for ( int i = 0; i < nRequestCount; i++, pRequest++)
		{
			int nSkill = pRequest->nSkill;
			SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
	
			BOOL bValidTarget = FALSE;
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON ))
			{
				UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ]; 
				UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );
				for ( int j = 0; j < MAX_WEAPON_LOCATIONS_PER_SKILL; j++ )
				{
					if ( ! pWeapons[ j ] )
						continue;
					if ( SkillIsValidTarget( pGame, pUnit, pTarget, pWeapons[ j ], nSkill, TRUE, NULL ) )
						bValidTarget = TRUE;
				}
				if ( ! bValidTarget && ! pWeapons[ 0 ] )
				{
					if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) )
						bValidTarget = SkillIsValidTarget( pGame, pUnit, pTarget, NULL, nSkill, TRUE, NULL );
					else 
					{ // if you have a punch or kick fallback skill, go check it.
						for ( int i = 0; i < NUM_FALLBACK_SKILLS; i++ )
						{
							int nFallbackSkill = pSkillData->pnFallbackSkills[ i ];
							if ( nFallbackSkill == INVALID_ID )
								continue;
							const SKILL_DATA * pFallbackSkillData = SkillGetData( pGame, nFallbackSkill );
							if ( ! pFallbackSkillData )
								continue;
							if ( SkillDataTestFlag( pFallbackSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) )
								continue;
							bValidTarget = SkillIsValidTarget( pGame, pUnit, pTarget, NULL, nFallbackSkill, TRUE, NULL );
							break;
						}
					}
				}
			} else {
				bValidTarget = SkillIsValidTarget( pGame, pUnit, pTarget, NULL, nSkill, TRUE, NULL );
			}
	
			if ( SkillGetTarget( pUnit, nSkill, INVALID_ID ) != pTarget && bValidTarget)
			{
				if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_TARGET_INDEX ) && pTarget &&
					SkillGetTargetIndex( pUnit, nSkill, INVALID_ID, UnitGetId( pTarget ) ) > 0 )
				{
					continue;
				}
				SkillSetTarget( pUnit, nSkill, INVALID_ID, UnitGetId( pTarget ) );
			}
		}
	}

}
#endif// !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGetWeapons(
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ],
	BOOL bCheckCooldown )
{
	ASSERT_RETURN(pWeaponArray);
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
		pWeaponArray[ i ] = NULL;

	if ( ! pUnit )
		return;

	if ( nSkill == INVALID_ID )
		return;

	GAME * pGame = UnitGetGame( pUnit );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if ( ! pSkillData )
		return;

	if ( pSkillData->pnWeaponIndices[ 0 ] == INVALID_ID )
		return;

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return;

	int nArrayCurr = 0;
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnWeaponIndices[ i ] != INVALID_ID )
		{
			UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, pSkillData->pnWeaponIndices[ i ] );
			if ( ! pWeapon )
				continue;
			if ( bCheckCooldown )
			{
				if ( UnitGetStatAny( pWeapon, STATS_SKILL_COOLING, NULL ) )
					continue;
				int nWeaponSkill = ItemGetPrimarySkill( pWeapon );
				if ( nWeaponSkill != INVALID_ID )
				{
					const SKILL_DATA * pWeaponSkillData = SkillGetData( pGame, nWeaponSkill );
					if ( pWeaponSkillData->nCooldownSkillGroup != INVALID_ID &&
						 UnitGetStat( pWeapon, STATS_SKILL_GROUP_COOLING, pWeaponSkillData->nCooldownSkillGroup ))
						continue;
				}
			}
			
			if ( ! UnitIsA( pWeapon, pSkillData->nRequiredWeaponUnittype ) )
				continue;
			pWeaponArray[ nArrayCurr ] = pWeapon;
			nArrayCurr++;
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsSortWeapons(
	 GAME * pGame,
	 UNIT * pUnit,
	 UNITID piWeapons[ MAX_WEAPONS_PER_UNIT ] )
{
	const UNIT_DATA * pUnitData = pUnit ? UnitGetData( pUnit ) : NULL;
	if ( pUnitData && UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_DONT_SORT_WEAPONS ) )
		return;

	if( AppIsTugboat() ) //Appears tugboat sorts the weapons a bit different.
	{
		if ( piWeapons[ WEAPON_INDEX_LEFT_HAND ] != INVALID_ID )
		{
			UNIT * pLeftWeapon = UnitGetById( pGame, piWeapons[ WEAPON_INDEX_LEFT_HAND ] );
			int nLeftSkill = ItemGetPrimarySkill( pLeftWeapon );
			if ( nLeftSkill != INVALID_ID )
			{
				const SKILL_DATA * pLeftSkillData = SkillGetData( pGame, nLeftSkill );
				if ( SkillDataTestFlag( pLeftSkillData, SKILL_FLAG_IS_MELEE ) )
				{
					UNIT * pRightWeapon = UnitGetById( pGame, piWeapons[ WEAPON_INDEX_RIGHT_HAND ] );
					int nRightSkill = ItemGetPrimarySkill( pRightWeapon );
					BOOL bSwitchWeapons = TRUE;
					if ( nRightSkill != INVALID_ID )
					{
						const SKILL_DATA * pRightSkillData = SkillGetData( pGame, nRightSkill );
						bSwitchWeapons = ! SkillDataTestFlag( pRightSkillData, SKILL_FLAG_IS_MELEE );
					}
	
					if ( bSwitchWeapons )
					{
						UNITID idTemp = piWeapons[ WEAPON_INDEX_RIGHT_HAND ];
						piWeapons[ WEAPON_INDEX_RIGHT_HAND ] = piWeapons[ WEAPON_INDEX_LEFT_HAND ];
						piWeapons[ WEAPON_INDEX_LEFT_HAND ] = idTemp;
					}
				}
			}
		}
		return;
	}
	//Hellgate only
	if ( piWeapons[ WEAPON_INDEX_LEFT_HAND ] == INVALID_ID )
		return;

	UNIT * pLeftWeapon = UnitGetById( pGame, piWeapons[ WEAPON_INDEX_LEFT_HAND ] );
	if ( ! pLeftWeapon )
		return;

	if ( UnitIsA( pLeftWeapon, UNITTYPE_MELEE ) || UnitIsA( pLeftWeapon, UNITTYPE_CABALIST_FOCUS ))
	{
		UNIT * pRightWeapon = UnitGetById( pGame, piWeapons[ WEAPON_INDEX_RIGHT_HAND ] );

		BOOL bSwitchWeapons = TRUE;
		if ( pRightWeapon )
		{
			bSwitchWeapons = !UnitIsA( pRightWeapon, UNITTYPE_MELEE ) && !UnitIsA( pRightWeapon, UNITTYPE_CABALIST_FOCUS );
		}

		if ( bSwitchWeapons )
		{
			UNITID idTemp = piWeapons[ WEAPON_INDEX_RIGHT_HAND ];
			piWeapons[ WEAPON_INDEX_RIGHT_HAND ] = piWeapons[ WEAPON_INDEX_LEFT_HAND ];
			piWeapons[ WEAPON_INDEX_LEFT_HAND ] = idTemp;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITMODE SkillChangeModeByWeapon(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITMODE eMode,
	UNITID idWeapon )
{
	// check to see if we need to use the left hand mode
	if ( idWeapon != INVALID_ID )
	{
		int nWeaponIndex = WardrobeGetWeaponIndex( pUnit, idWeapon );
		if ( nWeaponIndex == WEAPON_INDEX_LEFT_HAND )
		{
			const UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, eMode );
			if ( pModeData->eOtherHandMode != INVALID_ID )
				eMode = pModeData->eOtherHandMode;
		}
	}
	return eMode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillGetBonusLevels(
	UNIT * pUnit,
	const SKILL_DATA * pSkillData,
	int nSkill,
	int nBaseLevel )
{
	int nLevel = 0;

	for ( int i = 0; i < MAX_LINKED_SKILLS; i++ )
	{
		if ( pSkillData->pnLinkedLevelSkill[ i ] != INVALID_ID )
			nLevel += UnitGetStat( pUnit, STATS_SKILL_LEVEL, pSkillData->pnLinkedLevelSkill[ i ] );
	}

	if ( ! nLevel && !nBaseLevel )
		return 0;

	for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnSkillGroup[ i ] != INVALID_ID )
			nLevel += UnitGetStat( pUnit, STATS_SKILL_GROUP_LEVEL, pSkillData->pnSkillGroup[ i ] );
	}
	
	return nLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetLevel(
	UNIT * pUnit,
	int nSkill,
	BOOL ignoreBonus /* = FALSE */)
{
	const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
	if ( ! pSkillData )
		return 0;

	ASSERT_RETZERO( pUnit );
	int nLevel = UnitGetStat( pUnit, STATS_SKILL_LEVEL, nSkill );	
	if( !ignoreBonus )
	{
		nLevel += sSkillGetBonusLevels( pUnit, pSkillData, nSkill, nLevel );
	}

	for( int i = 0; i < MAX_SKILL_LEVEL_INCLUDE_SKILLS; i++ )
	{
		if( pSkillData->nSkillLevelIncludeSkills[ i ] == INVALID_ID )
			break;
		nLevel += SkillGetLevel( pUnit, pSkillData->nSkillLevelIncludeSkills[ i ] );
	}

	if ( AppIsTugboat() )
	{
		for ( int i = 0; i < MAX_PREREQUISITE_STATS; i++ )
		{
			if ( pSkillData->pnRequiredStats[ i ] == INVALID_ID )
				continue;

			int nStatValue = UnitGetStat( pUnit, pSkillData->pnRequiredStats[ i ] );
			for ( int j = nLevel - 1; j >= 0; j-- )
			{
				if ( nStatValue < pSkillData->pnRequiredStatValues[ i ][ j ] )
					nLevel--;
				else
					break;
			}
		}
		if ( nLevel <= 0 )
			return 0;
	}

	return MIN( pSkillData->nMaxLevel, nLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillLevelChanged(
	UNIT * unit,
	int nSkill,
	BOOL bHadSkill )
{
	GAME * game = UnitGetGame( unit );
	const SKILL_DATA * skill_data = SkillGetData(game, nSkill);
	ASSERT_RETURN(skill_data);
	int codelen = 0;
	int nLevel = SkillGetLevel( unit, nSkill );
	
	// don't apply stats to items which grant skill levels, as they will duplicate
	// the stats added to the player when the item is equipped -cmarch
	if ( AppIsTugboat() ||
		 UnitIsA( unit, skill_data->nRequiredUnittype ) ||  
		 !UnitIsA( unit, UNITTYPE_ITEM ) )
	{
		BYTE * code = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsOnLevelChange, &codelen);
		if (code)
		{
			VMExecI( game, unit, 0, 0, nSkill, nLevel, INVALID_ID, code, codelen);
		}
	}

	if ( IS_SERVER( unit ) )
	{
		if ( UnitIsA( unit, skill_data->nRequiredUnittype ) && 
			SkillDataTestFlag( skill_data, SKILL_FLAG_CAN_BE_SELECTED ) )
		{
			BOOL bWasSelected = UnitGetStat( unit, STATS_SKILL_IS_SELECTED, nSkill ) != 0;
			if ( bWasSelected )
			{

				SkillDeselect( game, unit, nSkill, TRUE );
			}

			if ( (nLevel && bWasSelected) ||
				 (!bHadSkill && nLevel && ! SkillDataTestFlag( skill_data, SKILL_FLAG_DONT_SELECT_ON_PURCHASE ) ))
			{
				SkillSelect( game, unit, nSkill );
			}

		}

		if ( ! bHadSkill && nLevel && 
			SkillDataTestFlag( skill_data, SKILL_FLAG_DEFAULT_SHIFT_SKILL_ENABLED ) && 
			! UnitTestFlag( unit, UNITFLAG_JUSTCREATED ) && 
			UnitIsA( unit, skill_data->nRequiredUnittype ) )
		{
			UnitSetStat( unit, STATS_SKILL_SHIFT_ENABLED, nSkill, 1 );
		}

		if ( bHadSkill && ! nLevel )
		{
			UnitTriggerEvent( game, EVENT_SKILL_FORGOTTEN, unit, NULL, &nSkill );
		}
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		if( nLevel > 0)
		{
			c_SkillLoad(game, nSkill);
		}
	}
#endif

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillHasLevel(
	int nSkill,
	int nLevel)
{
	const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	int nNumLevels = 0;
		
	for (int i = 0; i < MAX_LEVELS_PER_SKILL; ++i)
	{
		if (pSkillData->pnSkillLevelToMinLevel[ i ] > 0)
		{
			nNumLevels++;
		}
	}
	
	return nLevel <= nNumLevels;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillNextLevelAvailableAt( 
	UNIT * pUnit,
	int nSkill)
{
	ASSERT_RETZERO( pUnit );

	const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	ASSERT_RETZERO( pSkillData );

	int nSkillLevel = UnitGetBaseStat( pUnit, STATS_SKILL_LEVEL, nSkill ); // don't call SkillGetLevel or UnitGetStat - we need the base skill level without items
	if ( nSkillLevel >= MAX_LEVELS_PER_SKILL )
		return 0;

	return pSkillData->pnSkillLevelToMinLevel[ nSkillLevel ];
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetMaxLevelForUI ( 
	UNIT * pUnit,
	int nSkill)
{
	const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	ASSERT_RETZERO( pSkillData );

	int nMaxLevel = MAX_LEVELS_PER_SKILL;
	for ( int i = 0; i < MAX_LEVELS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnSkillLevelToMinLevel[ i ] == INVALID_ID )
		{
			nMaxLevel = i;
			break;
		}
	}
	int nLevel = UnitGetStat( pUnit, STATS_SKILL_LEVEL, nSkill );

	nMaxLevel += sSkillGetBonusLevels( pUnit, pSkillData, nSkill, nLevel );
	int nBaseLevel = UnitGetBaseStat( pUnit, STATS_SKILL_LEVEL, nSkill );
	nMaxLevel += nLevel - nBaseLevel; // find out how many levels have been added by items

	return MIN( pSkillData->nMaxLevel, nMaxLevel );
}	

inline int GetPointsInvestedInSkillTab( UNIT *pUnit, const SKILL_DATA * pSkillData )
{
	ASSERT_RETZERO( pUnit );
	ASSERT_RETZERO( pSkillData );
	int nPointsInvested( 0 );//UnitGetStat( pUnit, STATS_SKILL_POINTS_TIER_SPENT, pSkillData->nSkillTab ) );
	for( int iSkillIndex = 0; iSkillIndex < pSkillData->nSkillsInSameTabCount; iSkillIndex ++ )
	{
		if( pSkillData->nSkillsInSameTab[ iSkillIndex ] != INVALID_ID )
		{
			nPointsInvested += SkillGetLevel(pUnit, pSkillData->nSkillsInSameTab[ iSkillIndex ]);
		}
	}
	return nPointsInvested;
}


inline int GetPointsInvestedInSkillTab( UNIT *pUnit,
											int nSkillTab )
{
	ASSERT_RETZERO( pUnit );
	return UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab );
}

BOOL SkillUsesPerkPoints(
	const SKILL_DATA * pSkillData)
{
	ASSERT_RETFALSE(pSkillData);
	return pSkillData->pnPerkPointCost[0] > 0;
}

int SkillGetPerkPointsRequiredForNextLevel(
	const SKILL_DATA * pSkillData,
	int nCurrentLevel)
{
	ASSERT_RETINVALID(pSkillData);
	ASSERT_RETINVALID(nCurrentLevel >= 0);
	ASSERT_RETINVALID(nCurrentLevel <= MAX_LEVELS_PER_SKILL);

	if(nCurrentLevel == MAX_LEVELS_PER_SKILL)
	{
		return INVALID_ID;
	}

	return pSkillData->pnPerkPointCost[nCurrentLevel];
}

int SkillGetTotalPerkPointCostForLevel(
	const SKILL_DATA * pSkillData,
	int nLevel)
{
	ASSERT_RETINVALID(pSkillData);
	ASSERT_RETINVALID(nLevel >= 0);
	ASSERT_RETINVALID(nLevel <= MAX_LEVELS_PER_SKILL);

	int nTotalCost = 0;
	for(int i=0; i<nLevel; i++)
	{
		if(pSkillData->pnPerkPointCost[i] > 0)
		{
			nTotalCost += pSkillData->pnPerkPointCost[i];
		}
	}
	return nTotalCost;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD SkillGetNextLevelFlags(
	UNIT * pUnit,
	int nSkill )
{
	ASSERT_RETFALSE( pUnit );
	GAME * pGame = UnitGetGame( pUnit );

	DWORD dwFlags = 0;
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if (!pSkillData)
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_ERROR );
		return dwFlags;
	}

	if ( ! UnitIsA( pUnit, pSkillData->nRequiredUnittype ) )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_WRONG_UNITTYPE );
	}

	// disabled for GStar
	//if ( pSkillData->nRequiredStyle != INVALID_ID &&
	//	PlayerGetStyle( pUnit ) != pSkillData->nRequiredStyle )
	//{
	//	SETBIT( dwFlags, SKILLLEARN_BIT_WRONG_STYLE );
	//}

	int nBaseLevel = UnitGetBaseStat(pUnit, STATS_SKILL_LEVEL, nSkill);
	BOOL bMissingPrereq = FALSE;
	BOOL bHasPrereq = FALSE;
	for( int i = 0; i < MAX_PREREQUISITE_SKILLS; i++ )
	{
		if( pSkillData->pnRequiredSkills[i] != INVALID_ID &&
			pSkillData->pnRequiredSkillsLevels[i] > 0 )
		{
			if( UnitGetBaseStat( pUnit, STATS_SKILL_LEVEL, pSkillData->pnRequiredSkills[i] ) < // we have to check the base level of the skill
				pSkillData->pnRequiredSkillsLevels[ i ] )
			{
				bMissingPrereq = TRUE;
			}
			else
			{
				bHasPrereq = TRUE;
			}
		}
		else
		{
			break;
		}

	}
	if( bMissingPrereq && !( pSkillData->bOnlyRequireOneSkill && bHasPrereq ) )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL );
	}

	int nUnitLevel = UnitGetExperienceLevel(pUnit);
	REF(nUnitLevel);
	if(SkillUsesPerkPoints(pSkillData))
	{
		int nPerkPointsAvailable = UnitGetStat(pUnit, STATS_PERK_POINTS);
		int nPerkPointCost = SkillGetPerkPointsRequiredForNextLevel(pSkillData, nBaseLevel);
		if(nPerkPointCost > 0 && nPerkPointsAvailable < nPerkPointCost)
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_INSUFFICIENT_PERK_POINTS );
		}
	}
	else
	{
		int nUnitSkillPoints = pSkillData->bUsesCraftingPoints ? UnitGetStat(pUnit, STATS_CRAFTING_POINTS) : UnitGetStat(pUnit, STATS_SKILL_POINTS);
		if (nUnitSkillPoints <= 0)
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_NO_SKILL_POINTS );
		}
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_LEARN ) && ! PlayerIsSubscriber(pUnit) )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_SUBSCRIPTION_REQUIRED );
	}

	for ( int i = 0; i < MAX_PREREQUISITE_STATS; i++ )
	{
		if ( pSkillData->pnRequiredStats[ i ] == INVALID_ID )
			continue;

		if ( UnitGetBaseStat( pUnit, pSkillData->pnRequiredStats[ i ] ) < pSkillData->pnRequiredStatValues[ i ][ nBaseLevel ] )
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_STATS_ARE_TOO_LOW );
		}
	}

	//figure out how many points invested in this tier
	int nTiersInvestedIn = 0;
	if( AppIsTugboat() )
	{
		if( pSkillData->bUsesCraftingPoints )
		{
			nTiersInvestedIn = GetPointsInvestedInSkillTab( pUnit, pSkillData->nSkillTab );
		}
		else
		{
			nTiersInvestedIn = GetPointsInvestedInSkillTab( pUnit, 0 );
		}

	}
	int nTier = 0;
	if( AppIsTugboat() )
	{
		if( pSkillData->bUsesCraftingPoints )
		{
			nTier = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[ 0 ] != -1 ? pSkillData->pnSkillLevelToMinLevel[ 0 ] : 0, KSkillLevel_CraftingTier );
		}
		else
		{
			nTier = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[ 0 ] != -1 ? pSkillData->pnSkillLevelToMinLevel[ 0 ] : 0, KSkillLevel_Tier );
		}
	}


	// See if the player already has the skill (there will be a skill unit)
	BOOL bSkillIsKnown = FALSE;
	if ( nBaseLevel )
	{

		if ( nBaseLevel >= pSkillData->nMaxLevel ||
			pSkillData->pnSkillLevelToMinLevel[ nBaseLevel ] == INVALID_ID )
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_SKILL_IS_MAX_LEVEL );
		}
		else if ( AppIsHellgate() &&
				  pSkillData->pnSkillLevelToMinLevel[ nBaseLevel ] > nUnitLevel )
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW );
		}
		bSkillIsKnown = TRUE;
	}

	if ( AppIsHellgate() &&
		!bSkillIsKnown && pSkillData->pnSkillLevelToMinLevel[ 0 ] > nUnitLevel)
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW );
	}
	else if( AppIsTugboat() &&	
			nTier > nTiersInvestedIn )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW );	
	}

	if(SkillDataTestFlag(pSkillData, SKILL_FLAG_REQUIRES_STAT_UNLOCK_TO_PURCHASE))
	{
		if(UnitGetStat(pUnit, STATS_SKILL_UNLOCKED, nSkill) == 0)
		{
			SETBIT( dwFlags, SKILLLEARN_BIT_SKILL_IS_LOCKED );
		}
	}

	// There isn't any reason why we can't learn it...
	if ( !dwFlags && SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE ) )
	{
		//find out how many points we have invested in this tier...
		SETBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL );

	}

	if ( SkillGetLevel( pUnit, nSkill ) > nBaseLevel )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_SKILL_LEVEL_ENHANCED_BY_ITEMS );
	}

#if ISVERSION(CHEATS_ENABLED)
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();

	if ( pGlobals->dwFlags & GLOBAL_FLAG_SKILL_LEVEL_CHEAT )
	//if(GameGetDebugFlag(pUnit->pGame, DEBUGFLAG_ALWAYS_KILL))
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL );
		CLEARBIT( dwFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW );
		CLEARBIT( dwFlags, SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL );
		CLEARBIT( dwFlags, SKILLLEARN_BIT_STATS_ARE_TOO_LOW );
		CLEARBIT( dwFlags, SKILLLEARN_BIT_SKILL_IS_LOCKED );
	}
#endif

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_DISABLED ) )
	{
		SETBIT( dwFlags, SKILLLEARN_BIT_SKILL_IS_DISABLED );
	}

	return dwFlags;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillLevelUp(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bCostsSkillPoints,
	BOOL bForce )
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETFALSE(pSkillData);

	int nBaseSkillLevel = UnitGetBaseStat( pUnit, STATS_SKILL_LEVEL, nSkill );
	int nUnitLevel = UnitGetExperienceLevel(pUnit);
	int nUnitSkillPoints = pSkillData->bUsesCraftingPoints ? UnitGetStat(pUnit, STATS_CRAFTING_POINTS) : UnitGetStat(pUnit, STATS_SKILL_POINTS);
	int nUnitPerkPoints = UnitGetStat(pUnit, STATS_PERK_POINTS);
	int nPerkPointsRequired = SkillGetPerkPointsRequiredForNextLevel(pSkillData, nBaseSkillLevel);
	if(SkillUsesPerkPoints(pSkillData))
	{
		if( !bForce && ( bCostsSkillPoints && nUnitPerkPoints < nPerkPointsRequired ) )
		{
			return FALSE;
		}
	}
	else
	{
		if ( !bForce && ( bCostsSkillPoints && nUnitSkillPoints <= 0 ) )
		{
			return FALSE;
		}
	}

#if ISVERSION(CHEATS_ENABLED)
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( pGlobals->dwFlags & GLOBAL_FLAG_SKILL_LEVEL_CHEAT )
	{
		bForce = TRUE;
		bCostsSkillPoints = FALSE;
	}
#endif

	// See if the player already has the skill (there will be a skill unit)
	if( AppIsTugboat() )
	{
		if( pSkillData->pnSkillLevelToMinLevel[ 0 ] != INVALID_ID )
		{
			int nTiersInvestedIn = 0;
			int nTier = 0;

			if( pSkillData->bUsesCraftingPoints )
			{
				nTiersInvestedIn = SkillGetSkillLevelValue( GetPointsInvestedInSkillTab( pUnit, pSkillData->nSkillTab ), KSkillLevel_CraftingTier );
				nTier = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[ 0 ], KSkillLevel_CraftingTier );  //returns 0 if invalid
			}
			else
			{
				nTiersInvestedIn = SkillGetSkillLevelValue( GetPointsInvestedInSkillTab( pUnit, 0/*pSkillData->nSkillTab*/ ), KSkillLevel_Tier );
				nTier = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[ 0 ], KSkillLevel_Tier );  //returns 0 if invalid
			}

			bForce = bForce | ( (nTiersInvestedIn >= nTier )?TRUE:FALSE);
		}

	}
	if ( bForce ||
		 (pSkillData->pnSkillLevelToMinLevel[ nBaseSkillLevel ] != INVALID_ID && 
		  pSkillData->pnSkillLevelToMinLevel[ nBaseSkillLevel ] <= nUnitLevel) &&
		  !(AppIsTugboat() && nBaseSkillLevel + 1 > pSkillData->nMaxLevel ) )
	{
		UnitSetStat( pUnit, STATS_SKILL_LEVEL, nSkill, nBaseSkillLevel + 1 );		
		if ( bCostsSkillPoints )
		{
			if(SkillUsesPerkPoints(pSkillData))
			{
				UnitSetStat(pUnit, STATS_PERK_POINTS, nUnitPerkPoints-nPerkPointsRequired);
			}
			else
			{
				if( pSkillData->bUsesCraftingPoints )
				{
					UnitSetStat(pUnit, STATS_CRAFTING_POINTS, nUnitSkillPoints-1);
				}
				else
				{
					UnitSetStat(pUnit, STATS_SKILL_POINTS, nUnitSkillPoints-1);
				}
			}
		}
	}

	if ( !nBaseSkillLevel && pSkillData->nGivesSkill != INVALID_ID )
	{
		if ( ! UnitGetStat( pUnit, STATS_SKILL_LEVEL, pSkillData->nGivesSkill ) )
			SkillPurchaseLevel( pUnit, pSkillData->nGivesSkill, FALSE );
	}

	// tell the achievements system
	s_AchievementsSendSkillLevelUp( pUnit, nSkill );

	return TRUE;
}

BOOL SkillTierCanBePurchased( UNIT *pUnit,
							  int nSkillTab,
							  BOOL bCostsSkillPoints,
							  BOOL bForce )
{
	int nUnitSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS);
	int nSpentSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab );
	if ( !bForce && 
		( bCostsSkillPoints && nUnitSkillPoints <= 0 ||
		  nSpentSkillPoints >= MAX_SKILL_POINTS_IN_TIER ))
	{
		return FALSE;
	}

	return TRUE;
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL CraftingTierCanBePurchased( UNIT *pUnit,
							     int nSkillTab,
							     BOOL bCostsSkillPoints,
							     BOOL bForce )
{
	int nUnitSkillPoints = UnitGetStat(pUnit, STATS_CRAFTING_POINTS);
	int nSpentSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab );
	if ( !bForce && 
		( bCostsSkillPoints && nUnitSkillPoints <= 0 ||
		nSpentSkillPoints >= MAX_SKILL_POINTS_IN_CRAFTING_TIER ))
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillTierUp(
						  GAME * pGame,
						  UNIT * pUnit,
						  int nSkillTab,
						  BOOL bCostsSkillPoints = TRUE,
						  BOOL bForce = FALSE )
{
	if( SkillTierCanBePurchased( pUnit, nSkillTab, bCostsSkillPoints, bForce ) == FALSE )
	{
		return FALSE;
	}
	int nUnitSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS);
	int nSpentSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab );

#if ISVERSION(CHEATS_ENABLED)
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( pGlobals->dwFlags & GLOBAL_FLAG_SKILL_LEVEL_CHEAT )
	{
		bForce = TRUE;
		bCostsSkillPoints = FALSE;
	}
#endif



	UnitSetStat( pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab, nSpentSkillPoints+1 );
	UnitSetStat( pUnit, STATS_SKILL_POINTS, nUnitSkillPoints-1);
	UnitTriggerEvent( pGame, EVENT_TIER_POINT_SPENT, pUnit, pUnit, NULL );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCraftingTierUp(
						 GAME * pGame,
						 UNIT * pUnit,
						 int nSkillTab,
						 BOOL bCostsSkillPoints = TRUE,
						 BOOL bForce = FALSE )
{
	if( CraftingTierCanBePurchased( pUnit, nSkillTab, bCostsSkillPoints, bForce ) == FALSE )
	{
		return FALSE;
	}
	int nUnitSkillPoints = UnitGetStat(pUnit, STATS_CRAFTING_POINTS);
	int nSpentSkillPoints = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab );

#if ISVERSION(CHEATS_ENABLED)
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( pGlobals->dwFlags & GLOBAL_FLAG_SKILL_LEVEL_CHEAT )
	{
		bForce = TRUE;
		bCostsSkillPoints = FALSE;
	}
#endif



	UnitSetStat( pUnit, STATS_SKILL_POINTS_TIER_SPENT, nSkillTab, nSpentSkillPoints+1 );
	UnitSetStat( pUnit, STATS_CRAFTING_POINTS, nUnitSkillPoints-1);
	UnitTriggerEvent( pGame, EVENT_TIER_POINT_SPENT, pUnit, pUnit, NULL );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillPurchaseTier(
						UNIT * pUnit,
						int nSkillTab )
{
	ASSERT_RETFALSE( pUnit );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(IS_SERVER(pGame));


	return sSkillTierUp( pGame, pUnit, nSkillTab );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CraftingPurchaseTier(
					   UNIT * pUnit,
					   int nSkillTab )
{
	ASSERT_RETFALSE( pUnit );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(IS_SERVER(pGame));


	return sCraftingTierUp( pGame, pUnit, nSkillTab );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillShouldRespec(
	const SKILL_DATA * pSkillData,
	DWORD dwFlags)
{
	ASSERT_RETFALSE(pSkillData);

	if(TEST_MASK(dwFlags, SRF_CRAFTING_MASK))
	{
		return pSkillData->bUsesCraftingPoints;
	}
	else
	{
		if(pSkillData->bUsesCraftingPoints)
		{
			return FALSE;
		}
	}

	BOOL bUsesPerkPoints = SkillUsesPerkPoints(pSkillData);
	if(TEST_MASK(dwFlags, SRF_PERKS_MASK))
	{
		return bUsesPerkPoints;
	}
	else
	{
		if(bUsesPerkPoints)
		{
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillRespec( 
	UNIT * pUnit,
	DWORD dwFlags)
{

	ASSERT_RETFALSE( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE( pGame );

	DWORD dwModifiedFlags = dwFlags;

	if ( AppIsTugboat() )
	{// Elite and Hardcore players are not allowed to Re-Spec
		ASSERT_RETFALSE( !PlayerIsElite( pUnit ) );
		ASSERT_RETFALSE( !PlayerIsHardcore( pUnit ) );

		if ( UnitGetStat( pUnit, STATS_LEVEL ) <= 5 )
		{
			CLEARBIT(dwModifiedFlags, SRF_CHARGE_CURRENCY_BIT);
			CLEARBIT(dwModifiedFlags, SRF_USE_RESPEC_STAT_BIT);
		}
	}

	cCurrency cost = PlayerGetRespecCost( pUnit, TEST_MASK(dwModifiedFlags, SRF_CRAFTING_MASK) );
	if ( TEST_MASK(dwModifiedFlags, SRF_CHARGE_CURRENCY_MASK) && UnitGetCurrency( pUnit ) < cost )
	{
		return FALSE;
	}

	if ( TEST_MASK(dwModifiedFlags, SRF_USE_RESPEC_STAT_MASK) && MAX_RESPECS != -1 )
	{
		int nRespecs = TEST_MASK(dwModifiedFlags, SRF_CRAFTING_MASK) ? UnitGetStat( pUnit, STATS_RESPECSCRAFTING ) : UnitGetStat( pUnit, STATS_RESPECS );
		if( nRespecs >= MAX_RESPECS )
		{
			return FALSE;
		}
	}

	BOOL bUnitChanged = FALSE;

	const UNIT_DATA *pUnitData = UnitGetData(pUnit);
	ASSERT_RETINVALID( pUnitData );
	
	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkillId = (int) pStatsEntry[ jj ].GetParam();
		if ( nSkillId != INVALID_ID )
		{
			const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkillId );
			int nBasePoints = UnitGetBaseStat( pUnit, STATS_SKILL_LEVEL, nSkillId );
			if( sSkillShouldRespec( pSkillData, dwModifiedFlags ) )
			{
				if ( nBasePoints )
				{
					if( pSkillData && SkillDataTestFlag( pSkillData, SKILL_FLAG_DONT_ALLOW_RESPEC ) == FALSE  ) //some skills such as crafting can't be respected
					{
						SkillStop( pGame, pUnit, nSkillId, INVALID_ID, TRUE, TRUE ); //make sure to stop the skill

						
						if ( pSkillData && SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE ) )
						{
							for ( int i = 0; i < NUM_UNIT_START_SKILLS; i++ )
							{
								if ( pUnitData->nStartingSkills[ i ] == INVALID_ID )
									break;
								if ( pUnitData->nStartingSkills[ i ] == nSkillId )
								{
									nBasePoints--;
								}
							}
							if ( nBasePoints )
							{
								UnitChangeStat( pUnit, STATS_SKILL_LEVEL, nSkillId, -nBasePoints );
								if(SkillUsesPerkPoints(pSkillData))
								{
									int nTotalPerkPointsRefunded = 0;
									for(int i=0; i<nBasePoints; i++)
									{
										int nPerkPointsForNextSkillLevel = SkillGetPerkPointsRequiredForNextLevel(pSkillData, i);
										if(nPerkPointsForNextSkillLevel < 0)
										{
											break;
										}
										nTotalPerkPointsRefunded += nPerkPointsForNextSkillLevel;
									}
									UnitChangeStat( pUnit, STATS_PERK_POINTS, nTotalPerkPointsRefunded);
								}
								else
								{
									if( pSkillData->bUsesCraftingPoints )
									{
										if( AppIsTugboat() ) //tugboat never allows for you to respec your crafting points
										{
											UnitChangeStat( pUnit, STATS_CRAFTING_POINTS, nBasePoints );
										}
									}
									else
									{
										UnitChangeStat( pUnit, STATS_SKILL_POINTS, nBasePoints );
									}
								}
								bUnitChanged = TRUE;
							}
						}
					}
				}
			}
		}
	}
	UNIT_ITERATE_STATS_END;

	if( !TEST_MASK(dwModifiedFlags, SRF_CRAFTING_MASK) )
	{
		int nSkillPointsTierSpend = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, 0 );
		if ( nSkillPointsTierSpend )
		{
			UnitChangeStat( pUnit, STATS_SKILL_POINTS, nSkillPointsTierSpend );
			UnitSetStat( pUnit, STATS_SKILL_POINTS_TIER_SPENT, 0 );
			bUnitChanged = TRUE;
		}
	}

	// these are for crafting skill tiers
	
	if( AppIsTugboat() && TEST_MASK(dwModifiedFlags, SRF_CRAFTING_MASK)) //tugboat never allows for you to respec your crafting points
	{
		for( int i = 4; i < 7; i++ )
		{
			int nSkillPointsTierSpend = UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, pUnitData->nSkillTab[i] );
			if ( nSkillPointsTierSpend )
			{
				UnitChangeStat( pUnit, STATS_CRAFTING_POINTS, nSkillPointsTierSpend );
				UnitSetStat( pUnit, STATS_SKILL_POINTS_TIER_SPENT, pUnitData->nSkillTab[i], 0 );
				bUnitChanged = TRUE;
			}
		}
	}
	

	// OK, no points changed hands, let's not charge you.
	if( bUnitChanged )
	{
		//Take money
		if( TEST_MASK(dwModifiedFlags, SRF_CHARGE_CURRENCY_MASK) )
		{
			ASSERT_RETFALSE( UnitRemoveCurrencyValidated( pUnit, cost ) );
		}

		if ( TEST_MASK(dwModifiedFlags, SRF_USE_RESPEC_STAT_MASK) )
		{
			if( TEST_MASK(dwModifiedFlags, SRF_CRAFTING_MASK) ) 
				UnitChangeStat( pUnit, STATS_RESPECSCRAFTING, 1 );
			else
				UnitChangeStat( pUnit, STATS_RESPECS, 1 );
		}
	}
	if( AppIsTugboat() )
	{
		UnitTriggerEvent( pGame, EVENT_TIER_POINT_SPENT, pUnit, pUnit, NULL );
	}

	// this will verify equip requirements and remove items if necessary
	s_InventoryChanged(pUnit);

	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillPurchaseLevel(
	UNIT * pUnit,
	int nSkill,
	BOOL bCostsSkillPoints,
	BOOL bForce )
{
	ASSERT_RETFALSE( pUnit );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(IS_SERVER(pGame));


	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if (!pSkillData)
		return FALSE;


	if ( ! UnitIsA( pUnit, pSkillData->nRequiredUnittype ) )
		return FALSE;

	// OK, the skill doesn't have level data and it's just gonna assert later, so ... no
	if (pSkillData->pnSkillLevelToMinLevel[0] == INVALID_ID)
	{
		return FALSE;
	}

	if(SkillDataTestFlag(pSkillData, SKILL_FLAG_REQUIRES_STAT_UNLOCK_TO_PURCHASE))
	{
		if(UnitGetStat(pUnit, STATS_SKILL_UNLOCKED, nSkill) == 0)
		{
			return FALSE;
		}
	}

	if ( ! sSkillLevelUp( pGame, pUnit, nSkill, bCostsSkillPoints, bForce ) )
		return FALSE;

	UnitTriggerEvent( pGame, EVENT_SKILL_LEARNED, pUnit, pUnit, &EVENT_DATA( nSkill, INVALID_ID ) );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillUnlearn(
	UNIT * pUnit,
	int nSkill )
{
	ASSERT_RETFALSE( pUnit );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(IS_SERVER(pGame));

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if (!pSkillData)
		return FALSE;
	
	UnitSetStat(pUnit, STATS_SKILL_LEVEL, nSkill, 0);		//clear out skill level

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float sSkillGetRange( 
	SKILL_CONTEXT & context,
	BOOL bGetDesired = FALSE,
	BOOL bGetMeleeRange = FALSE)
{
	ASSERT_RETVAL(context.game, 0.0f);
	ASSERT_RETVAL(context.unit, 0.0f);
	if (context.skillData == NULL)
	{
		context.skillData = SkillGetData(context.game, context.skill);
		ASSERT_RETVAL(context.skillData, 0.0f);
	}

	float fRangeMax = 0.0f;
	float fRangeMultiplier = 1.0f;

	SKILL_CONTEXT t_context = context;

	// find the max range from any source
	if (t_context.idWeapon != INVALID_ID && SkillDataTestFlag(t_context.skillData, SKILL_FLAG_USES_WEAPON) && 
		!SkillDataTestFlag(t_context.skillData, SKILL_FLAG_DONT_USE_WEAPON_RANGE))
	{
		ASSERT(t_context.weapon);
		int weaponSkill = ItemGetPrimarySkill(t_context.weapon);
		if (weaponSkill != INVALID_ID)
		{
			const SKILL_DATA * weaponSkillData = SkillGetData(t_context.game, weaponSkill);
			ASSERT(weaponSkillData);
			if (weaponSkillData && SkillDataTestFlag(weaponSkillData, SKILL_FLAG_ALLOW_REQUEST))
			{
				fRangeMultiplier = t_context.skillData->fWeaponRangeMultiplier;
				ASSERT(fRangeMultiplier > 0.0f);
				if (fRangeMultiplier <= 0.0f)
				{
					fRangeMultiplier = 1.0f;
				}
				t_context.skill = weaponSkill;
				t_context.skillData = weaponSkillData;
				t_context.skillLevel = 0;
			}
		}
	}

	if( !bGetDesired && context.skillData->codeSkillRangeScript != NULL_CODE )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( context.game, DATATABLE_SKILLS, context.skillData->codeSkillRangeScript, &code_len);
		if (code_ptr)
		{		
			int nSkillLevel = (context.skillRequestedLevel > 0 )?context.skillRequestedLevel:context.skillLevel;
			if( nSkillLevel == 0 )
				nSkillLevel = SkillGetLevel( context.unit, context.skill );
			if( nSkillLevel == 0 && context.ultimateOwner )
				nSkillLevel = SkillGetLevel( context.ultimateOwner, context.skill );

			float fRangeMult = ((float)VMExecI( context.game, context.unit, NULL, NULL, context.skill,  nSkillLevel, context.skill, nSkillLevel, INVALID_ID, code_ptr, code_len)/100.0f) + 1.0f;
			fRangeMultiplier = MAX(fRangeMultiplier, fRangeMult);
		}
	}

	if (t_context.skillData->fRangePercentPerLevel != 0.0f)
	{
		fRangeMultiplier += (sSkillGetSkillLevel(t_context) * t_context.skillData->fRangePercentPerLevel) / 100.0f;
	}

	BOOL bCheckWithoutWeapons = TRUE;
	if (!t_context.weapon && SkillDataTestFlag(t_context.skillData, SKILL_FLAG_USES_WEAPON) && ! SkillDataTestFlag(t_context.skillData, SKILL_FLAG_DONT_USE_WEAPON_RANGE))
	{
		UNIT * weapons[MAX_WEAPON_LOCATIONS_PER_SKILL];
		UnitGetWeapons(t_context.unit, t_context.skill, weapons, FALSE);
		if (SkillDataTestFlag(t_context.skillData, SKILL_FLAG_WEAPON_IS_REQUIRED))
		{
			for (int kk = 0; kk < NUM_FALLBACK_SKILLS && !weapons[0]; ++kk)
			{
				if (t_context.skillData->pnFallbackSkills[kk] != INVALID_ID)
				{
					UnitGetWeapons(t_context.unit, t_context.skillData->pnFallbackSkills[kk], weapons, FALSE);
				}
			}
		}
		bCheckWithoutWeapons = !weapons[0] && ! SkillDataTestFlag(t_context.skillData, SKILL_FLAG_WEAPON_IS_REQUIRED);
		for (int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++)
		{
			int weaponSkill = ItemGetPrimarySkill(weapons[i]);
			// Travis - gotta do this for Tugboat - recursion if you don't
			if (AppIsTugboat() && weaponSkill == INVALID_ID)
			{
				continue;
			}
			else if (AppIsHellgate() && (weaponSkill == INVALID_ID || weaponSkill == t_context.skill))
			{
				continue;
			}
			SKILL_CONTEXT w_context(t_context.game, t_context.unit, weaponSkill, 0, NULL, weapons[i], 0, 0);
			float fRange = sSkillGetRange(w_context, bGetDesired, bGetMeleeRange);
			fRangeMax = MAX(fRangeMax, fRange);
		}
	} 
	
	if (bCheckWithoutWeapons)
	{
		for (int i = 0; i < MAX_MISSILES_PER_SKILL; i++)
		{
			if (t_context.skillData->pnMissileIds[ i ] == INVALID_ID)
			{
				continue;
			}

			const UNIT_DATA * missile_data = MissileGetData(t_context.game, t_context.skillData->pnMissileIds[i]);
			ASSERT_CONTINUE(missile_data);
			if (UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_DONT_USE_RANGE_FOR_SKILL))
			{
				continue;
			}

			float fRangePercent = MissileGetRangePercent(t_context.unit, t_context.weapon, missile_data);
			float fMissileRange = missile_data->fRangeBase * fRangePercent;

			if (bGetDesired && missile_data->fDesiredRangeMultiplier > 0.0f)
			{
				fMissileRange *= missile_data->fDesiredRangeMultiplier;
			}
			fRangeMax = MAX(fRangeMax, fMissileRange);
		}
	}

	if (SkillDataTestFlag(t_context.skillData, SKILL_FLAG_USE_RANGE) && 
		(!bGetMeleeRange || !SkillDataTestFlag(t_context.skillData, SKILL_FLAG_DONT_USE_RANGE_FOR_MELEE)))
	{
		float fRange = bGetDesired ? t_context.skillData->fRangeDesired : t_context.skillData->fRangeMax;
		fRangeMax = MAX(fRangeMax, fRange);
	}

	if (SkillDataTestFlag(t_context.skillData, SKILL_FLAG_USE_HOLY_AURA_FOR_RANGE))
	{
		float fRange = UnitGetHolyRadius(t_context.unit);
		fRangeMax = MAX(fRangeMax, fRange);
	}

	if ((SkillDataTestFlag(t_context.skillData, SKILL_FLAG_IS_MELEE) || bGetMeleeRange) &&
		 !SkillDataTestFlag(t_context.skillData, SKILL_FLAG_FORCE_SKILL_RANGE_FOR_MELEE))
	{
		switch (UnitGetGenus(t_context.unit))
		{
		case GENUS_MONSTER:
		case GENUS_PLAYER:		
			{
				if (AppIsTugboat() && UnitGetGenus(t_context.unit) == GENUS_PLAYER)
				{
					// ranges in tugboat come from the skill
					float fRange = bGetDesired ? t_context.skillData->fRangeDesired : t_context.skillData->fRangeMax;
					fRangeMax = MAX(fRangeMax, fRange);
					break;
				}
				const UNIT_DATA * unit_data = UnitGetData(t_context.unit);
				if (unit_data)
				{
					float fMeleeRange = bGetDesired ? unit_data->fMeleeRangeDesired : unit_data->fMeleeRangeMax;
					float fRange = fMeleeRange * UnitGetScale(t_context.unit);
					fRangeMax = MAX(fRangeMax, fRange);
				}
			}
			break;
		}
	}

	return fRangeMax * fRangeMultiplier;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillGetRange(
	UNIT * unit,
	int skill,
	UNIT * weapon,
	BOOL bGetMeleeRange,
	int skillLevel)
{	
	SKILL_CONTEXT context(UnitGetGame(unit), unit, skill, skillLevel, NULL, weapon, 0, 0);
	return sSkillGetRange(context, FALSE, bGetMeleeRange);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillGetDesiredRange(
	UNIT * unit,
	int skill,
	UNIT * weapon,
	BOOL bGetMeleeRange,
	int skillLevel)
{
	SKILL_CONTEXT context(UnitGetGame(unit), unit, skill, skillLevel, NULL, weapon, 0, 0);
	return sSkillGetRange(context, TRUE, bGetMeleeRange);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITMODE sSkillEventHolderGetMode(
	const SKILL_DATA * pSkillData,
	const SKILL_EVENT_HOLDER * pHolder )
{
	if ( pSkillData->nModeOverride != INVALID_ID )
		return (UNITMODE) pSkillData->nModeOverride;
	return (UNITMODE) pHolder->nUnitMode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetMode(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon )
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETINVALID( pSkillData );

	SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSkillData->nEventsId );
	ASSERT_RETINVALID( pEvents );

	if ( pEvents->nEventHolderCount <= 0 )
		return INVALID_ID;

	ASSERT_RETINVALID( pEvents->pEventHolders );

	UNITMODE eMode = sSkillEventHolderGetMode( pSkillData, &pEvents->pEventHolders[ 0 ] );

	return SkillChangeModeByWeapon( pGame, pUnit, nSkill, eMode, idWeapon );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define HELLGATE_PLAYER_MELEE_SPEEDUP 25
int UnitGetMeleeSpeed( 
	UNIT * pUnit )
{
	ASSERT_RETZERO( AppIsHellgate() );
	int nPercent = UnitGetStat( pUnit, STATS_OFFWEAPON_MELEE_SPEED );
	// use the slower melee weapon's speed
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return nPercent;

	GAME * pGame = UnitGetGame( pUnit );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pWeapons[ i ] = WardrobeGetWeapon( pGame, nWardrobe, i );
	}

	int nSlowestIndex = INVALID_ID;
	int nSlowestVal = 0;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! pWeapons[ i ] )
			continue;
		if ( ! UnitIsA( pWeapons[ i ], UNITTYPE_MELEE ) )
			continue;
		int nSpeed = UnitGetStat( pWeapons[ i ], STATS_MELEE_SPEED );
		if ( nSlowestIndex == INVALID_ID ||
			nSpeed < nSlowestVal )
		{
			nSlowestIndex = i;
			nSlowestVal = nSpeed;
		}
	}
	nPercent += nSlowestVal;

	nPercent = MIN( nPercent, 70 );

	return nPercent;
}



static float sSkillGetModeDuration( 
	GAME * pGame, 
	UNIT * pUnit,
	const SKILL_DATA * pSkillData,
	UNITMODE eMode,
	UNIT * pWeapon )
{
	BOOL bHasMode;
	float fModeDuration = UnitGetModeDuration( pGame, pUnit, eMode, bHasMode );
	if( AppIsHellgate() ) 
	{
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE ) )
		{
			int nPercent = UnitGetMeleeSpeed( pUnit );
			if ( nPercent )
				fModeDuration -= fModeDuration * ((float)(min(nPercent,75)) / 100.0f);
		}
		return fModeDuration;
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE ) )
	{
		int nPercent = UnitGetStat( pUnit, STATS_OFFWEAPON_MELEE_SPEED );
		nPercent += UnitGetStatMatchingUnittype( pUnit, &pWeapon, 1, STATS_ATTACK_SPEED );		
		nPercent += UnitGetStat( pUnit, STATS_MELEE_SPEED );
		// We do this because on the client it isn't accruing to the player the same way
		// but we still need to know about it. If we do it on the server too, it does it twice
		if ( IS_CLIENT( pGame ) && pWeapon )
		{
			nPercent += UnitGetStat( pWeapon, STATS_MELEE_SPEED );
		}
		//Removed because we couldn't tell skills were breaking.
		//nPercent = MIN( nPercent, 150 );
		if ( nPercent )
			fModeDuration /= ( 1 + ( (float)nPercent / 100.0f ) );
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_RANGED ) )
	{		
		int nPercent = UnitGetStatMatchingUnittype( pUnit, &pWeapon, 1, STATS_ATTACK_SPEED );
		nPercent += UnitGetStat( pUnit, STATS_RANGED_SPEED );
		// We do this because on the client it isn't accruing to the player the same way
		// but we still need to know about it. If we do it on the server too, it does it twice
		if ( IS_CLIENT( pGame ) && pWeapon )
		{
			nPercent += UnitGetStat( pWeapon, STATS_RANGED_SPEED );
		}
		//Removed because we couldn't tell skills were breaking.
		//nPercent = MIN( nPercent, 150 );
		if ( nPercent )
			fModeDuration /= ( 1 + ( (float)nPercent / 100.0f ) );
	}
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_SPELL ) )
	{
		int nPercent = UnitGetStat( pUnit, STATS_SPELL_SPEED );


		//Removed because we couldn't tell skills were breaking.
		//nPercent = MIN( nPercent, 150 );
		if ( nPercent )
			fModeDuration /= ( 1 + ( (float)nPercent / 100.0f ) );
	}
	fModeDuration *= pSkillData->fModeSpeedMultiplier;
	return fModeDuration;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillGetDuration( 
	GAME * pGame, 
	UNIT * pUnit,
	int nSkill,
	BOOL bSkipWarmup,
	UNIT * pWeapon )
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETZERO( pSkillData );

	// skills with no events have no duration
	if( pSkillData->nEventsId == INVALID_ID )
	{
		return 0.0f;
	}
	SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSkillData->nEventsId );
	ASSERT_RETZERO( pEvents );

	float fDuration = 0.0f;
	for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
	{
		SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
		if ( (pHolder->nUnitMode == MODE_FIRE_RIGHT_WARMUP || pHolder->nUnitMode == MODE_FIRE_RIGHT_HOLD_WARMUP) && bSkipWarmup )
			continue;

// TRAVIS: TODO: FIXME - ?IS THIS RIGHT?
		if( AppIsTugboat() && pHolder->fDuration != 0 )
		{
			return pHolder->fDuration;
		}

		UNITMODE eMode = sSkillEventHolderGetMode( pSkillData, pHolder );

		eMode = SkillChangeModeByWeapon( pGame, pUnit,  nSkill, eMode, UnitGetId( pWeapon ) );

		fDuration += (pHolder->fDuration > 0.0f) ? pHolder->fDuration : sSkillGetModeDuration( pGame, pUnit, pSkillData, eMode, pWeapon );
	}
	return fDuration;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillGetFiringPositionAndDirection( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nSkill,
	UNITID idWeapon,
	const SKILL_EVENT * pSkillEvent,
	const UNIT_DATA * missile_data,
	VECTOR & vPositionOut,
	VECTOR * pvDirectionOut,
	UNIT * pTarget,
	VECTOR * pvTarget,
	DWORD dwSeed,
	BOOL bDontAimAtCrosshairs,
	float flHorizSpread,
	BOOL bClearSkillTargetsOutOfCone ) // = false
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	int nWeaponIndex = WardrobeGetWeaponIndex( pUnit, idWeapon );
	if ( pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_AIM_WITH_WEAPON_ZERO )
		nWeaponIndex = 0;

	if ( IS_CLIENT( pGame ) && UnitGetGenus( pUnit ) == GENUS_PLAYER )
	{
		c_UnitUpdateGfx( pUnit );
	} 

	if( AppIsHellgate() )
	{
		if ( nWeaponIndex != INVALID_ID && !UnitIsA( pUnit, UNITTYPE_PLAYER ) )
			UnitUpdateWeaponPosAndDirecton( pGame, pUnit, nWeaponIndex, pTarget, 
				UnitTestFlag( pUnit, UNITFLAG_ALWAYS_AIM_AT_TARGET ), bDontAimAtCrosshairs );
	}else{
		BOOL bInvalidTarget = FALSE;
		if ( nWeaponIndex != INVALID_ID )
			UnitUpdateWeaponPosAndDirecton( pGame, pUnit, nWeaponIndex, pTarget, FALSE, TRUE, &bInvalidTarget );//bDontAimAtCrosshairs );

		// target is not in valid firing cone - kill it from the skill targets!
		if( IS_SERVER( pGame ) && bInvalidTarget && bClearSkillTargetsOutOfCone )
		{

			if( pUnit )
			{
				pTarget = NULL;
				SkillSetTarget( pUnit, nSkill, idWeapon, INVALID_ID );
			}
		}

	}

	if ( pTarget && ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_PLACE_ON_TARGET ) != 0 )
	{
		if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
		{
			if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE )
			{
				vPositionOut = pSkillEvent->tAttachmentDef.vPosition + UnitGetPosition( pTarget );
				if ( pvDirectionOut )
					*pvDirectionOut = pSkillEvent->tAttachmentDef.vNormal;
			} else {
				MATRIX matWorld;
				UnitGetWorldMatrix( pTarget, matWorld );
				MatrixMultiply( &vPositionOut, &pSkillEvent->tAttachmentDef.vPosition, &matWorld );
				if ( pvDirectionOut )
				{
					MatrixMultiplyNormal( pvDirectionOut, &pSkillEvent->tAttachmentDef.vNormal, &matWorld );
					VectorNormalize( *pvDirectionOut );
				}
			}
		} else {
			vPositionOut = UnitGetAimPoint( pTarget );
			if( AppIsTugboat() && UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured && pUnit )
			{
				vPositionOut = UnitGetPosition( pTarget ) + ( UnitGetAimPoint( pUnit ) - UnitGetPosition( pUnit ) );
			}

			if ( pvDirectionOut )
			{
				
				if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
				{
					VECTOR vShooter;
					if ( pUnit )
					{
						vShooter = UnitGetAimPoint( pUnit );
					}
					else
					{						
						vShooter = VECTOR( 0 );
					}

					VECTOR vDelta = vShooter - vPositionOut;
					if( AppIsTugboat() && !UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured ) //tugboat doesn't consider Z
					{
						vDelta.fZ = 0.0f;
					}

					VectorNormalize( *pvDirectionOut, vDelta );
				}
				else
				{
					*pvDirectionOut = UnitGetFaceDirection( pTarget, FALSE );
					if( UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured && pTarget )
					{
						VECTOR vTarget = UnitGetPosition( pTarget ) + ( vPositionOut - UnitGetPosition( pUnit ) );

						VECTOR vDelta = vTarget - vPositionOut;
						VectorNormalize( vDelta );
						(*pvDirectionOut).fZ += vDelta.fZ;

					}
				}
			}
		}
	}
	else if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
	{
		if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE )
		{
			vPositionOut = pSkillEvent->tAttachmentDef.vPosition + UnitGetPosition( pUnit );
			if ( pvDirectionOut )
				*pvDirectionOut = pSkillEvent->tAttachmentDef.vNormal;
		} else {
			MATRIX matWorld;
			if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_AIM_WITH_WEAPON )
			{
				VECTOR vPositionForAiming( 0 );
				VECTOR vDirectionForAiming( 0, 1, 0 );
				UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, &vPositionForAiming, &vDirectionForAiming );		
				MatrixFromVectors( matWorld, vPositionForAiming, VECTOR( 0, 0, 1 ), -vDirectionForAiming );
			} else {
				UnitGetWorldMatrix( pUnit, matWorld );
			}
			MatrixMultiply( &vPositionOut, &pSkillEvent->tAttachmentDef.vPosition, &matWorld );
			const UNIT_DATA * pUnitData = UnitGetData( pUnit );
			if ( pUnitData->fOffsetUp )
				vPositionOut.fZ += pUnitData->fOffsetUp;
			if ( pvDirectionOut )
			{
				if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
				{
					VECTOR vTarget;
					if ( pTarget )
					{
						vTarget = UnitGetAimPoint( pTarget );
					}
					else if ( pvTarget ) 
					{
						vTarget = *pvTarget;
					}
					else
					{
						//WARN( pTarget || pvTarget );
						vTarget = VECTOR( 0 );
					}

					VECTOR vDelta = vTarget - vPositionOut;
 					if( AppIsTugboat() && !UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured ) //tugboat doesn't consider Z
					{
						vDelta.fZ = 0.0f;
					}
					if ( VectorNormalize( *pvDirectionOut, vDelta ) < 0.001f )
						*pvDirectionOut = UnitGetFaceDirection( pUnit, FALSE );
				}
				else
				{
					MatrixMultiplyNormal( pvDirectionOut, &pSkillEvent->tAttachmentDef.vNormal, &matWorld );
					VectorNormalize( *pvDirectionOut );

					if( UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured && pTarget )
					{
						VECTOR vTarget = UnitGetPosition( pTarget ) + ( vPositionOut - UnitGetPosition( pUnit ) );
						VECTOR vDelta = vTarget - vPositionOut;
						VectorNormalize( vDelta );
						(*pvDirectionOut).fZ += vDelta.fZ;
	
					}
				}
			}
		}
	}
	else
	{
		// KCK: An issue exists where in some cases monster missiles are not being placed correctly.
		// This is because we've been relying on the Event Offset to place them, which if set correctly
		// in Hammer will work UNLESS the animation is modified (such as having the head turn towards
		// the player). The actual bone has not been used in this case because the server has no concept
		// of bones and the server needs to track the missile as well.
		// The code here tries to fix that problem by creating the missile on the client at the bone
		// location, if set. This only applies to missiles that have a target (as opposed to direction),
		// as otherwise the sync errors would be extreme and not recoverable.
		// The "use bone for missile position" field in skills_monster.xls must be set to use this
		bool		bPositionFromBone = false;
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_BONE_FOR_MISSILE_POSITION )  &&
			 IS_CLIENT( pGame ) && 
			 pSkillEvent->tAttachmentDef.pszBone[0] != '\0' &&
			 pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		{
			int nSourceModelId = c_UnitGetModelId( pUnit );
			if (nSourceModelId != INVALID_ID)
			{
				int nBoneId = pSkillEvent->tAttachmentDef.nBoneId;
				if (pSkillEvent->tAttachmentDef.nBoneId == INVALID_ID)
				{
					int nSourceAppearanceDefId = c_AppearanceGetDefinition(nSourceModelId);
					nBoneId = c_AppearanceDefinitionGetBoneNumber(nSourceAppearanceDefId, pSkillEvent->tAttachmentDef.pszBone);
				}
				MATRIX tBoneMatrix;
				if (c_AppearanceGetBoneMatrix(nSourceModelId, &tBoneMatrix, nBoneId))
				{
					const MATRIX * pmWorld = e_ModelGetWorldScaled( nSourceModelId );
					MatrixMultiply( &tBoneMatrix, &tBoneMatrix, pmWorld );
					MatrixMultiply( &vPositionOut, &cgvNone, &tBoneMatrix );
					bPositionFromBone = true;
				}
			}
		} 
		if ( !bPositionFromBone )
		// End KCK Code
			UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, &vPositionOut, pvDirectionOut );		

		if( AppIsTugboat() )
		{
			//I looked in the Hellgate revisions and didn't see
			//this code, so I'm guessing it's unique to Tugboat.....marsh			
			if ( pvDirectionOut )
			{

				if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
				{
					VECTOR vTarget;
					if ( pTarget )
					{
						vTarget = UnitGetAimPoint( pTarget );
					} else if ( pvTarget ) 
					{
						vTarget = *pvTarget;
					} else {
						//WARN( pTarget || pvTarget );
						vTarget = VECTOR( 0 );
					}
					VECTOR vDelta = vTarget - vPositionOut;
					if( !UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured ) //tugboat doesn't consider Z
					{
						vDelta.fZ = 0;
					}
					VectorNormalize( *pvDirectionOut, vDelta );
				}
				else
				{
					if( UnitGetLevel( pUnit )->m_pLevelDefinition->bContoured && pTarget )
					{
						VECTOR vTarget = UnitGetPosition( pTarget ) + ( vPositionOut - UnitGetPosition( pUnit ) );
						VECTOR vDelta = vTarget - vPositionOut;
						VectorNormalize( vDelta );
						(*pvDirectionOut).fZ += vDelta.fZ;

					}
				}
			}
		}
	}

	if(pSkillEvent->dwFlags & SKILL_EVENT_FLAG_RANDOM_FIRING_DIRECTION && pvDirectionOut)
	{
		RAND tRand;
		RandInit( tRand, dwSeed );
		*pvDirectionOut = RandGetVector(tRand);
		if(pSkillEvent->tAttachmentDef.vNormal.fX == 0.0f)
		{
			pvDirectionOut->fX = 0.0f;
		}
		if(pSkillEvent->tAttachmentDef.vNormal.fY == 0.0f)
		{
			pvDirectionOut->fY = 0.0f;
		}
		if(pSkillEvent->tAttachmentDef.vNormal.fZ == 0.0f)
		{
			pvDirectionOut->fZ = 0.0f;
		}
		VectorNormalize(*pvDirectionOut);
		dwSeed = RandGetNum(tRand);
	}

	/*
	if(pSkillEvent->dwFlags & SKILL_EVENT_FLAG_AUTOAIM_PROJECTILE && pvDirectionOut)
	{
		if(missile_data->nHavokShape != HAVOK_SHAPE_NONE)
		{
			ONCE
			{
				VECTOR vTarget;
				if ( pTarget )
				{
					vTarget = UnitGetPosition( pTarget );
				}
				else if ( pvTarget ) 
				{
					vTarget = *pvTarget;
				}
				else
				{
					break;
				}
				if(VectorIsZeroEpsilon(vTarget))
				{
					break;
				}

				VECTOR vDelta = vTarget - vPositionOut;

				// Auto-aim
				float fZFinal = vTarget.fZ - vPositionOut.fZ;
				float fXYFinal = VectorDistanceXY(vTarget, vPositionOut);
				float fGravity = HavokGetGravity().fZ;
				float fVelocity = MissileGetFireSpeed(pGame, missile_data, pUnit, *pvDirectionOut);

				float VSquared = fVelocity * fVelocity;
				float XSquared = fXYFinal * fXYFinal;
				float VSquaredMinusGY = VSquared - fGravity*fZFinal;
				float XSquaredPlusYSquared = XSquared + fZFinal*fZFinal;

				// Are there any real solutions?
				float fDiscriminant = VSquaredMinusGY * VSquaredMinusGY - fGravity * fGravity * XSquaredPlusYSquared;
				if(fDiscriminant >= 0)
				{
					fDiscriminant = sqrtf(fDiscriminant);
					float fDenominator = 2.0f * VSquared * XSquaredPlusYSquared;
					float fSolution1Squared = (XSquared * (VSquaredMinusGY + fDiscriminant)) / fDenominator;
					float fSolution2Squared = (XSquared * (VSquaredMinusGY - fDiscriminant)) / fDenominator;

					//float fTheta[4] = { INFINITY, INFINITY, INFINITY, INFINITY };
					float fTheta[2] = { INFINITY, INFINITY };
					if(fSolution1Squared >= 0)
					{
						float fCosineTheta = sqrtf(fSolution1Squared);
						fTheta[0] = acosf(fCosineTheta);
						fTheta[1] = -fTheta[0];
					}
					//if(fSolution2Squared >= 0)
					//{
					//	float fCosineTheta = sqrtf(fSolution2Squared);
					//	fTheta[2] = acosf(fCosineTheta);
					//	fTheta[3] = -fTheta[2];
					//}
					//float fActualTheta = MIN(fTheta[0], MIN(fTheta[1], MIN(fTheta[2], fTheta[3])));
					//float fActualTheta = MIN(fTheta[0], fTheta[1]);
					static volatile int foo = 0;
					float fActualTheta = fTheta[foo];
					VECTOR vBase = vDelta;
					vBase.fZ = 0.0f;
					VectorNormalize(vBase);
					VECTOR vRotationAxis;
					VectorCross(vRotationAxis, vBase, VECTOR(0, 0, 1));
					MATRIX matRotation;
					MatrixRotationAxis(matRotation, vRotationAxis, fActualTheta);
					MatrixMultiply(pvDirectionOut, &vBase, &matRotation);
				}
			}
		}
	}
	// */

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_TARGETS_POSITION ) && pvDirectionOut && pvTarget )
	{
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) && pTarget)
		{
			*pvTarget = UnitGetAimPoint( pTarget );
		} else {
			*pvTarget = UnitGetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill );
			if ( VectorIsZero( *pvTarget ))  //I'm not sure if this is a good place for this. Does the SKILL_TARGET_X get reset at any point - should it? What about a skill that spawns multiple monsters or other skills that might touch this stat - marsh
			{
				LEVEL* pLevel = UnitGetLevel( pUnit );
				float fCollideDist = LevelLineCollideLen(pGame, pLevel, vPositionOut, *pvDirectionOut, 30.0f);
				*pvTarget = *pvDirectionOut;
				*pvTarget *= fCollideDist;
				*pvTarget += vPositionOut;
			}
		}
	} 
	else if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_AIM_AT_TARGET ) && pTarget )
	{
		VECTOR vAimPoint = UnitGetAimPoint( pTarget );
		if ( pvTarget )
			*pvTarget = vAimPoint;

		if ( pvDirectionOut )
		{
			VECTOR vDelta = vAimPoint - vPositionOut;
			VectorNormalize( *pvDirectionOut, vDelta );
		}
	}

	float fVertRange;
	float fHorizRange;
	UNIT * pWeapon = UnitGetById( pGame, idWeapon );
	SkillGetAccuracy( pUnit, pWeapon, nSkill, missile_data, flHorizSpread, fVertRange, fHorizRange );

	if (missile_data && pvDirectionOut && ( fVertRange != 0.0f || fHorizRange != 0.0f ))
	{

		RAND tRand;
		RandInit( tRand, dwSeed );
		float fVertChange  = RandGetFloat( tRand ) * 2.0f * fVertRange  - fVertRange;
		float fHorizChange( fHorizRange );
		if( AppIsHellgate() ) //Tugboat doesn't want it to be random.
			fHorizChange = RandGetFloat( tRand ) * 2.0f * fHorizRange - fHorizRange;
		//fVertChange = fVertRange; // these help when testing the accuracy crosshairs
		//fHorizChange = fHorizRange;

		VECTOR vUpVector = UnitGetUpDirection( pUnit );
		VECTOR vSideVector;
		if ( vUpVector == *pvDirectionOut || vUpVector == -*pvDirectionOut )
		{
			float fTemp = vUpVector.fX;
			vUpVector.fX = vUpVector.fY;
			vUpVector.fY = vUpVector.fZ;
			vUpVector.fZ = fTemp;
			VectorCross( vSideVector, vUpVector, *pvDirectionOut );
		} else {
			VectorCross( vSideVector, vUpVector, *pvDirectionOut );
		}
		VectorNormalize( vSideVector );
		
		*pvDirectionOut = VectorLerp( *pvDirectionOut, fHorizChange < 0 ? -vSideVector : vSideVector, -fabsf(fHorizChange) + 1.0f );
		VectorNormalize( *pvDirectionOut );

		*pvDirectionOut = VectorLerp( *pvDirectionOut, fVertChange < 0 ? -vUpVector : vUpVector, -fabsf(fVertChange) + 1.0f );
		VectorNormalize( *pvDirectionOut );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillGetAccuracy(
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill,
	const UNIT_DATA * pMissileData,
	float fHorizontalSpread,
	float & fVerticalRange,
	float & fHorizontalRange )
{
	fVerticalRange = 0.0f;
	fHorizontalRange = 0.0f;

	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pMissileData );

	{
		UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
		pWeapons[ 0 ] = pWeapon;
		for ( int i = 1; i < MAX_WEAPONS_PER_UNIT; i++ )
			pWeapons[ i ] = NULL;
		UnitInventorySetEquippedWeapons( pUnit, pWeapons ); // this is so that stats like, STATS_FIRING_ERROR_BONUS, are set properly
	}
	int nHorizontalErrorOverride = UnitGetStat( pUnit, STATS_FIRING_ERROR_OVERRIDE_HORIZONTAL );
	int nVerticalErrorOverride   = UnitGetStat( pUnit, STATS_FIRING_ERROR_OVERRIDE_VERTICAL );

	if (( pMissileData->fHorizontalAccuracy != 0.0f || 
		pMissileData->fVerticleAccuracy != 0.0f ||
		fHorizontalSpread != 0.0f ))
	{
		fVerticalRange		= pMissileData->fVerticleAccuracy;
		fHorizontalRange	= pMissileData->fHorizontalAccuracy + fHorizontalSpread;

		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );

		int nFiringError = 0;
		if ( pSkillData && SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_UNIT_FIRING_ERROR ))
			nFiringError = UnitGetStat( pUnit, STATS_FIRING_ERROR );

		if ( pWeapon && sWeaponUsesFiringError( pWeapon ) )
			nFiringError = MAX( nFiringError, UnitGetStat( pWeapon, STATS_FIRING_ERROR_WEAPON ) );

		nFiringError += UnitGetStat( pUnit, STATS_FIRING_ERROR_BONUS );

		float fErrorPercent = 0.0f;
		if ( nFiringError != 0 )
		{
			fErrorPercent = (float)(nFiringError) / 100.0f;
		}
		fErrorPercent = 1.0f + MAX( fErrorPercent, -0.95f );

		if ( AppIsHellgate() )
		{
			int nAccuracy = UnitGetStat( pUnit, STATS_ACCURACY );
			if ( nAccuracy > 0 )
			{
				const UNIT_DATA * pUnitData = UnitGetData( pUnit );
				ASSERT_RETURN( pUnitData );

				nAccuracy -= pUnitData->nAccuracyBase;
				fErrorPercent -= ( nAccuracy * fErrorPercent ) / ( nAccuracy + 100 );
			}
		}

		fVerticalRange   *= fErrorPercent;
		fHorizontalRange *= fErrorPercent;
	}

	if ( nVerticalErrorOverride != 0 )
	{
		fVerticalRange = (float)nVerticalErrorOverride / 10.0f;
	}
	if ( nHorizontalErrorOverride != 0 )
	{
		fHorizontalRange = (float)nHorizontalErrorOverride / 10.0f;
	}
}

//----------------------------------------------------------------------------
// extract stat-agnostic cost from unit
//----------------------------------------------------------------------------
static BOOL sSkillTakeCost(
	SKILL_CONTEXT & context,
	int stat,
	int cost)
{
	ASSERT_RETFALSE(context.unit);
	cost = MAX(cost, 0);

	if (cost > 0 && IS_SERVER(context.unit))
	{
		int cur = UnitGetStat(context.unit, stat);
		if (cur < cost)
		{
			return FALSE;
		}
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
		if (!GameGetDebugFlag(context.game, DEBUGFLAG_INFINITE_POWER))
#endif
			UnitChangeStat(context.unit, stat, -cost);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// extract power cost from unit
//----------------------------------------------------------------------------
static BOOL sSkillTakePowerCost(
	SKILL_CONTEXT & context)
{
	// if the skill reduces power max, that is the cost, don't double the cost by reducing power cur (applied later)
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST))
		return TRUE;

	ASSERT_RETFALSE(context.unit);
	int nPowerCost = SkillGetPowerCost(context.unit, context.skillData, sSkillGetSkillLevel(context));

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_KILL_PETS_FOR_POWER_COST))
	{
		// if there are pets consuming too much max power, they will be killed
		// and the max power consumption will be turned into power for activating this skill
		int nPowerCur = UnitGetStat(context.unit, STATS_POWER_CUR);
		if (nPowerCur < nPowerCost)
		{
			nPowerCost = nPowerCur;
		}
	}

	return sSkillTakeCost(context, STATS_POWER_CUR, nPowerCost);
}


//----------------------------------------------------------------------------
// extract power cost from unit
//----------------------------------------------------------------------------
BOOL SkillTakePowerCost(
	UNIT * unit,
	int skill,
	const SKILL_DATA * pSkillData)
{
	SKILL_CONTEXT context(UnitGetGame(unit), unit, skill, 0, pSkillData, INVALID_ID, 0, 0);
	return sSkillTakePowerCost(context);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillTestLineOfSight(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData,
	UNIT * pWeapon,
	UNIT * pUnitTarget,
	VECTOR * pvTarget )
{
	VECTOR vPosition;
	if ( pSkillData->nAimHolderIndex == INVALID_ID )
	{
		int nWeaponIndex = WardrobeGetWeaponIndex( pUnit, UnitGetId( pWeapon ) );
		UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, &vPosition, NULL );
	} else {
		SKILL_EVENTS_DEFINITION * pEvents = ( pSkillData->nEventsId != INVALID_ID ) ? (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSkillData->nEventsId ) : NULL;
		ASSERT_RETFALSE( pSkillData->nAimHolderIndex < pEvents->nEventHolderCount );
		SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ pSkillData->nAimHolderIndex ];
		ASSERT_RETFALSE( pSkillData->nAimEventIndex < pHolder->nEventCount );
		SKILL_EVENT * pSkillEvent = & pHolder->pEvents[ pSkillData->nAimEventIndex ];

		DWORD dwSeed = SkillGetSeed( pGame, pUnit, pWeapon, nSkill );

		const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
		const UNIT_DATA * missile_data = NULL;
		if ( pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES )
		{
			missile_data = MissileGetData(pGame, pSkillEvent->tAttachmentDef.nAttachedDefId);
		}

		SkillGetFiringPositionAndDirection( pGame, pUnit, nSkill, UnitGetId( pWeapon ), pSkillEvent, missile_data, 
			vPosition, NULL, pUnitTarget, pvTarget, dwSeed );
	}

	VECTOR vTargetPos = pvTarget ? *pvTarget : UnitGetAimPoint( pUnitTarget );

	VECTOR vDelta = vTargetPos - vPosition;
	float fDistance = VectorLength( vDelta );
	VECTOR vDirection;
	if ( fDistance != 0.0f )
		VectorScale( vDirection, vDelta, 1.0f / fDistance );
	else
		vDirection = VECTOR( 0, 1, 0 );

	LEVEL* pLevel = UnitGetLevel( pUnit );
	float fDistToCollide = LevelLineCollideLen( pGame, pLevel, vPosition, vDirection, fDistance );
	return ( fDistToCollide >= fDistance );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillShouldStart(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pUnitTarget,
	VECTOR * pvTarget )
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeaponArray, TRUE );

	// MONSTERS are the only things that call this function, so this flag is OK.
	if ( AppIsTugboat() &&
		 SkillDataTestFlag( pSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) &&
		 !pUnitTarget )
	{
		return FALSE;
	}
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		UNIT * pWeapon = pWeaponArray[ i ];
		if ( ! pWeapon && i > 0 )
			continue;

		if( AppIsTugboat() && pWeapon && UnitIsA( pWeapon, UNITTYPE_WEAPON ) )
		{
			int nWeaponSkill = ItemGetPrimarySkill( pWeapon);
			const SKILL_DATA * pWeaponSkillData = SkillGetData( pGame, nWeaponSkill );
			// MONSTERS are the only things that call this function, so this flag is OK.
			if ( SkillDataTestFlag( pWeaponSkillData, SKILL_FLAG_MONSTER_MUST_TARGET_UNIT ) &&
				!pUnitTarget )
			{
				return FALSE;
			}
			// check LOS
			if ( SkillDataTestFlag( pWeaponSkillData, SKILL_FLAG_CHECK_LOS ) && pUnitTarget )
			{
				//if ( ! SkillTestLineOfSight( pGame, pUnit, nSkill, pSkillData, pWeapon, pUnitTarget, pvTarget ) )
				if( pUnitTarget &&
					!UnitInLineOfSight( pGame, pUnit, pUnitTarget ) )
					return FALSE;
			}
		}
		// check LOS
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) && pUnitTarget )
		{
			if ( AppIsHellgate() && !SkillTestLineOfSight( pGame, pUnit, nSkill, pSkillData, pWeapon, pUnitTarget, pvTarget ) )			
			{
				return FALSE;
			}
			if( AppIsTugboat() &&
				pUnitTarget &&
				!UnitInLineOfSight( pGame, pUnit, pUnitTarget ) )
			{
				return FALSE;
			}
		}

		UNITID idWeapon    = pWeapon    ? UnitGetId( pWeapon ) : INVALID_ID;
		if ( ! SkillCanStart( pGame, pUnit, nSkill, idWeapon, pUnitTarget, FALSE, FALSE, 0 ) )
			return FALSE;

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) )
		{
			BOOL bIsInRange = FALSE;
			if ( ! SkillIsValidTarget( pGame, pUnit, pUnitTarget, pWeapon, nSkill,  TRUE, &bIsInRange ) ||
				! bIsInRange )
				return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// Skill Event Handlers
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleStartMode( 
	GAME * pGame,
	UNIT * unit,
	const struct EVENT_DATA& tEventData)
{
	ASSERT_RETFALSE(pGame && unit);
/*
	if (RoomIsActive(UnitGetRoom(unit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(unit, "Attempt to start mode in inactive Room"))
		{
			return FALSE;
		}
	}
*/
	int nSkill = (int)tEventData.m_Data1;
	REF(nSkill);
	UNITID idWeapon = (UNITID)tEventData.m_Data2;
	REF(idWeapon);
	UNITMODE eMode = (UNITMODE)((int)tEventData.m_Data3);
	float fModeDuration = EventParamToFloat( tEventData.m_Data4 );
	ASSERT_RETFALSE( eMode != INVALID_ID );

	if ( IS_SERVER( pGame ) )
		s_UnitSetMode( unit, eMode, 0, fModeDuration, 0, FALSE );
	else if ( IS_CLIENT( pGame ) )
		c_UnitSetMode( unit, eMode, 0, fModeDuration );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sHandleEndMode( 
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& tEventData)
{
	ASSERT_RETFALSE(pGame && unit);
	//ASSERT(IS_SERVER(pGame));

	int nSkill = (UNITMODE)((int)tEventData.m_Data1);
	UNITID idWeapon = (UNITID)tEventData.m_Data2;
	UNITMODE eMode = (UNITMODE)((int)tEventData.m_Data3);

	eMode = SkillChangeModeByWeapon( pGame, unit, nSkill, eMode, idWeapon );

	UnitEndMode( unit, eMode, FALSE );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sSkillEventGetTime( 
	UNIT * pUnit, 
	UNITMODE eMode,
	const SKILL_EVENT * pSkillEvent )
{
	if ( ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_ANIM_CONTACT_POINT ) == 0 )
		return pSkillEvent->fTime;

	int nAppearanceDef = UnitGetAppearanceDefId( pUnit, TRUE );
	int nAnimationGroup = UnitGetAnimationGroup( pUnit );

	return AppearanceDefinitionGetContactPoint( nAppearanceDef, nAnimationGroup, eMode, pSkillEvent->fTime );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * SkillGetLeftHandWeapon( 
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon )
{
	UNIT * pWeaponOther = WardrobeGetWeapon( pGame, UnitGetWardrobe( pUnit ), WEAPON_INDEX_LEFT_HAND );
	if ( ! pWeaponOther )
		return NULL;
	if ( pWeaponOther == pWeapon )
		return NULL;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	if ( UnitIsA( pWeaponOther, pSkillData->nRequiredWeaponUnittype ) )
		return pWeaponOther;
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillSendMessage(
	GAME* game,
	UNIT* unit,
	BYTE bCommand,
	MSG_STRUCT * msg,
	BOOL bForceSendToKnown)
{
	bool bExcludeUnit = UnitIsA(unit, UNITTYPE_PLAYER) && !bForceSendToKnown;
	GAMECLIENTID idClientExclude = bExcludeUnit ? UnitGetClientId(unit) : INVALID_GAMECLIENTID;
	s_SendUnitMessage(unit, bCommand, msg, idClientExclude);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetHolyRadius( 
	UNIT * pUnit )
{
	int nRadius = UnitGetStat( pUnit, STATS_HOLY_RADIUS );
	int nPercent = UnitGetStat( pUnit, STATS_HOLY_RADIUS_PERCENT );
	if ( nPercent )
	{
		return (float)( nRadius * (100 + nPercent) ) / 100.0f;
	}
	return (float) nRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleSkillStop( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData)
{
	int nSkill = (int) tEventData.m_Data1;
	UNITID idWeapon = (UNITID)tEventData.m_Data2;
	ASSERT_RETFALSE( nSkill != INVALID_ID );
	SkillStop( pGame, pUnit, nSkill, idWeapon );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS * sSkillEventAddDamagePercentFromSkillGroups(
	UNIT * pUnit,
	const SKILL_DATA * pSkillData,
	STATS * pStats)
{
	ASSERT_RETVAL(pUnit, pStats);
	ASSERT_RETVAL(pSkillData, pStats);

	if(!SkillDataTestFlag(pSkillData, SKILL_FLAG_APPLY_SKILLGROUP_DAMAGE_PERCENT))
	{
		return pStats;
	}

	int nDamagePercentSkill = pStats ? StatsGetStat(pStats, STATS_DAMAGE_PERCENT_SKILL) : 0;
	for(int i=0; i<MAX_SKILL_GROUPS_PER_SKILL; i++)
	{
		if(pSkillData->pnSkillGroup[i] != INVALID_ID)
		{
			int nDamagePercentSkillGroup = UnitGetStat(pUnit, STATS_DAMAGE_PERCENT_SKILLGROUP, pSkillData->pnSkillGroup[i]);
			nDamagePercentSkill += nDamagePercentSkillGroup;
		}
	}

	if (!pStats)
	{
		pStats = StatsListInit(UnitGetGame(pUnit));
	}
	StatsSetStat(UnitGetGame(pUnit), pStats, STATS_DAMAGE_PERCENT_SKILL, nDamagePercentSkill);
	return pStats;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * SkillCreateSkillEventStats(
	GAME * game,
	UNIT * unit,
	int skill,
	const SKILL_DATA * skill_data,
	int skillLevel,
	UNIT *pObjectTarget)
{
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(unit);
	if (!skill_data)
	{
		skill_data = SkillGetData(game, skill);
		ASSERT_RETNULL(skill_data);
	}

	STATS * stats = NULL;
#if !ISVERSION(CLIENT_ONLY_VERSION)

	// apply codeStatsSkillEventServer
	if ( IS_SERVER(game) )
	{
		int codeLen = 0;
		BYTE * codePtr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsSkillEventServer, &codeLen);
		if (codePtr)
		{
			if (!stats)
			{
				stats = StatsListInit(game);
			}
			if (skillLevel <= 0)
			{
				skillLevel = SkillGetLevel(unit, skill);
			}

			VMExecI(game, unit, pObjectTarget, stats, skill, skillLevel, skill, skillLevel, INVALID_ID, codePtr, codeLen);
		}
	}
	// now apply codeStatsSkillEventServerPostProcess
	if ( IS_SERVER(game) )
	{
		int codeLen = 0;
		BYTE * codePtr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsSkillEventServerPostProcess, &codeLen);
		if (codePtr)
		{
			if (!stats)
			{
				stats = StatsListInit(game);
			}
			if (skillLevel <= 0)
			{
				skillLevel = SkillGetLevel(unit, skill);
			}

			VMExecI(game, unit, pObjectTarget, stats, skill, skillLevel, skill, skillLevel, INVALID_ID, codePtr, codeLen);
		}
	}

	if ( IS_SERVER(game) )
	{
		stats = sSkillEventAddDamagePercentFromSkillGroups(unit, skill_data, stats);
	}

#endif

	// there are some stats that both client and server need for an event to work
	{
		int codeLen = 0;
		BYTE * codePtr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsSkillEvent, &codeLen);
		if (codePtr)
		{
			if ( ! stats )
				stats = StatsListInit(game);

			if (skillLevel <= 0)
			{
				skillLevel = SkillGetLevel(unit, skill);
			}

			VMExecI(game, unit, pObjectTarget, stats, skill, skillLevel, skill, skillLevel, INVALID_ID, codePtr, codeLen);
		}

	}
	return stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillHandleSkillEvent( 
	GAME * pGame,
	UNIT * unit,
	const struct EVENT_DATA & tEventData)
{
	ASSERT_RETFALSE(pGame && unit);
	if ( AppIsHellgate() && 
		 !UnitGetRoom(unit))
	{
		return FALSE;
	}

	int nSkill = (int)tEventData.m_Data1;
	int nSkillLevel	= (int)tEventData.m_Data6;
	UNITID idWeapon	= (int)tEventData.m_Data2;
	const SKILL_EVENT * pSkillEvent = (const SKILL_EVENT *)tEventData.m_Data3;
	float fDuration	= EventParamToFloat(tEventData.m_Data4);
	DWORD_PTR pParam = tEventData.m_Data5;

	SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
	//tugboat doesn't have this but it seems like a cool feature. LEaving in.
	UNIT * pUnitForCondition = unit;
	if(pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_WEAPON_FOR_CONDITION)
	{
		pUnitForCondition = UnitGetById(pGame, idWeapon);
	}

	if ( (! pSkillEventTypeData->bTestsItsCondition || pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_CONDITION_ON_EVENT ) && 
		pSkillEvent->tCondition.nType != INVALID_ID && 
		! ConditionEvalute( pUnitForCondition, pSkillEvent->tCondition, nSkill, idWeapon ) )
	{
		return FALSE;
	}

	if((pSkillEvent->dwFlags & SKILL_EVENT_FLAG_SERVER_ONLY && !IS_SERVER(pGame)) ||
		(pSkillEvent->dwFlags & SKILL_EVENT_FLAG_CLIENT_ONLY && !IS_CLIENT(pGame)))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(pSkillEvent);
	ASSERT_RETFALSE(pSkillEventTypeData);

	PFN_SKILL_EVENT pfnEventHandler = pSkillEventTypeData->pfnEventHandler;
	if ( ! pfnEventHandler )
		return FALSE;
	ASSERT_RETFALSE( pfnEventHandler );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	if ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_CHANCE_PCODE )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeEventChance, &code_len);
		if (code_ptr)
		{
			int nChance = VMExecI(pGame, unit, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);

			if ( (int) RandGetNum( pGame, 100 ) > nChance )
				return FALSE;
		}
	}
	else if ( pSkillEvent->fRandChance < 1.0f )
	{
		// make some things happen randomly
		if (RandGetFloat(pGame) > pSkillEvent->fRandChance)
		{
			return FALSE;
		}
	}

	if(pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_CHARGE_POWER_AND_COOLDOWN && !sSkillCanTakePowerCost(unit, nSkill, nSkillLevel, pSkillData, FALSE))
	{
		return FALSE;
	}

	struct STATS * pStatsList = NULL;
	if (pSkillEventTypeData->bApplySkillStats)
	{
		pStatsList = SkillCreateSkillEventStats(pGame, unit, nSkill, pSkillData, nSkillLevel);
		if (pStatsList)
		{
			StatsListAdd(unit, pStatsList, FALSE);
		}
	}

	SKILL_EVENT_FUNCTION_DATA tFunctionData(pGame, unit, pSkillEvent, nSkill, nSkillLevel, pSkillData, idWeapon);
	tFunctionData.fDuration = fDuration;
	tFunctionData.pParam = pParam;
	BOOL bEventSucceeded = pfnEventHandler( tFunctionData );

	if(bEventSucceeded)
	{
		if(pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_CHARGE_POWER_AND_COOLDOWN)
		{
			if(!SkillTakePowerCost(unit, nSkill, pSkillData))
			{
				if ( pStatsList )
				{
					StatsListRemove( pGame, pStatsList );

					StatsListFree( pGame, pStatsList );
				}

				SkillStop( pGame, unit, nSkill, idWeapon, TRUE, FALSE);
				return FALSE;
			}

			SkillStartCooldown(pGame, unit, UnitGetById(pGame, idWeapon), nSkill, nSkillLevel, pSkillData, 0, FALSE, TRUE );
		}

		if(pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_MARK_SKILL_AS_SUCCESSFUL)
		{
			SkillIsOnSetFlag(unit, SIO_SUCCESSFUL_BIT, TRUE, nSkill, idWeapon);
		}

		if((pSkillEventTypeData->bCausesAttackEvent || (pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_CAUSE_ATTACK_EVENT)) &&
			!(pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_CAUSE_ATTACK_EVENT))
		{
			UNIT * pWeapons[MAX_WEAPONS_PER_UNIT] = { NULL, NULL };
			SkillGetWeaponsForAttack(tFunctionData, pWeapons);
			for(int i=0; i<MAX_WEAPONS_PER_UNIT; i++)
			{
				if( pWeapons[i] )
				{
					UnitTriggerEvent(pGame, EVENT_ATTACK, unit, pWeapons[i], NULL);
					UnitTriggerEvent(pGame, EVENT_ATTACK, pWeapons[i], unit, NULL);
				}
			}
		}
	}

	if ( pStatsList )
	{
		StatsListRemove( pGame, pStatsList );

		StatsListFree( pGame, pStatsList );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillRegisterEvent( 
	GAME* pGame, 
	UNIT* pUnit, 
	const SKILL_EVENT* pSkillEvent,
	int nSkill,
	int nSkillLevel,
	UNITID idWeapon,
	float fDeltaTime,
	float fDuration,
	float fTimeUntilSkillStop,
	DWORD_PTR pData )
{
	BOOL bDoEvent = FALSE;
	const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );

	if ( pSkillEventTypeData->bClientOnly )
	{
		if ( IS_CLIENT( pGame ) )
		{
			bDoEvent = TRUE;
		}
	} 
	else if (pSkillEventTypeData->bServerOnly)
	{
		if ( IS_SERVER( pGame ) )
		{
			bDoEvent = TRUE;
		}
	} 
	else 
	{
		bDoEvent = TRUE;
	}

	if ( bDoEvent )
	{
		EVENT_DATA tEventData(nSkill, idWeapon, (DWORD_PTR)pSkillEvent, EventFloatToParam( fDuration ), pData, nSkillLevel);
		int nTicks = PrimeFloat2Int(fDeltaTime * GAME_TICKS_PER_SECOND_FLOAT);
		if ( fTimeUntilSkillStop )
		{
			int nTicksToSkillStop = PrimeFloat2Int(fTimeUntilSkillStop * GAME_TICKS_PER_SECOND_FLOAT);
			if ( nTicksToSkillStop && nTicksToSkillStop == nTicks )
				nTicks--;
		}
		BOOL bDoNow = (nTicks <= 0);

		if ( bDoNow )
			return SkillHandleSkillEvent( pGame, pUnit, tEventData);
		else
		{
			UnitRegisterEventTimed(pUnit, SkillHandleSkillEvent, &tEventData, nTicks);
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sShouldDelayMeleeAttack( 
	GAME* pGame,
	UNIT* pUnit,
	int nSkill,
	UNIT * pWeapon )
{
	int nWeaponSkill = ItemGetPrimarySkill( pWeapon );

	if ( nWeaponSkill == INVALID_ID )
		return FALSE;

	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );
	if ( pWeapons[ 0 ] == NULL || pWeapons[ 1 ] == NULL ||
		( AppIsTugboat() && pWeapons[ 1 ] && UnitIsA( pWeapons[1], UNITTYPE_SHIELD ) ) )
		return FALSE;

	const SKILL_DATA * pWeaponSkillData = SkillGetData( pGame, nWeaponSkill );
	if ( !SkillDataTestFlag( pWeaponSkillData, SKILL_FLAG_IS_MELEE ) )
		return FALSE;

	{// check for dual melee
		int nLeftWeaponSkill = ItemGetPrimarySkill( pWeapons[ WEAPON_INDEX_LEFT_HAND ] );
		if ( nLeftWeaponSkill != INVALID_ID )
		{
			if( AppIsTugboat() )
				return FALSE;
			const SKILL_DATA * pSkillData = SkillGetData( pGame, nLeftWeaponSkill );
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE ) )
				return FALSE;
		}
	}

	UNITID idWeapon		= UnitGetId( pWeapon );

	UNIT * pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );

	if ( pTarget && ! SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, nSkill, TRUE ) )
		pTarget = NULL;

	if ( ! pTarget )
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleSkillRepeat( 
	GAME * pGame,
	UNIT * pUnit,
	const struct EVENT_DATA & tEventData)
{
//	trace("sHandleSkillRepeat called (%c) for skill %d. Tick: %d, power: %d, cool: %d\n", (IS_CLIENT(pGame))? 'c':'s', tEventData.m_Data1, GameGetTick(pGame), UnitGetStat(pUnit, STATS_POWER_CUR), UnitGetStat(pUnit, STATS_SKILL_COOLING, (DWORD)tEventData.m_Data1));

	if (RoomIsActive(UnitGetRoom(pUnit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(pUnit, "Attempt to repeat skill in Inactive Room"))
		{
			return FALSE;
		}
	}
	int nSkill = (int) tEventData.m_Data1;
	UNITID idWeapon = (UNITID)tEventData.m_Data2;
	BOOL bSetRepeatFlag = (BOOL)tEventData.m_Data3;
	int nRequestedSkill = (int) tEventData.m_Data4;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	UNIT * pWeapon = UnitGetById( pGame, idWeapon );

	BOOL bUpdateActiveSkills = FALSE;
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SELECTS_A_MELEE_SKILL ) )
	{
		if ( IS_CLIENT( pGame ) )
		{
			UNIT * pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );
			sSkillSelectMeleeSkill( pUnit, nSkill, UnitGetId( pTarget ) );
		}
		bUpdateActiveSkills = TRUE;
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_TRACK_REQUEST ) )
	{
		bUpdateActiveSkills = TRUE;
	}

	if ( bUpdateActiveSkills )
	{
		SkillUpdateActiveSkills( pUnit );

		if ( ! SkillIsOn( pUnit, nSkill, idWeapon, FALSE ) )
			return FALSE;
	}

	UNIT * pUnitToCheckForCooling = pWeapon;
	if ( ! pUnitToCheckForCooling || SkillDataTestFlag( pSkillData, SKILL_FLAG_COOLDOWN_UNIT_INSTEAD_OF_WEAPON ))
		pUnitToCheckForCooling = pUnit;

	if ( UnitGetStat( pUnitToCheckForCooling, STATS_SKILL_COOLING, nSkill ) ||
		( pSkillData->nCooldownSkillGroup != INVALID_ID && UnitGetStat( pUnitToCheckForCooling, STATS_SKILL_GROUP_COOLING, pSkillData->nCooldownSkillGroup )))
	{
		UnitRegisterEventTimed( pUnit, sHandleSkillRepeat, &EVENT_DATA( nSkill, idWeapon, bSetRepeatFlag, nRequestedSkill ), 1);
		return FALSE; 
	}

	DWORD dwSeed = SkillGetSeed( pGame, pUnit, pWeapon, nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );

	sSkillSetSeed( pGame, pUnit, pWeapon, nSkill, RandGetNum( tRand ) );

	SKILL_CONTEXT skill_context(pGame, pUnit, nSkill, 0, nRequestedSkill, 0, idWeapon, bSetRepeatFlag ? SKILL_START_FLAG_IS_REPEATING : 0, dwSeed);
	if ( ! SkillCanStart( pGame, pUnit, nSkill, idWeapon, NULL, FALSE, FALSE, 0 ) || 
		!sSkillExecute(skill_context))
	{
		SkillStop( pGame, pUnit, nSkill, idWeapon );
		SkillUpdateActiveSkills( pUnit );
		return FALSE;

	} else if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SKILL_IS_ON ) )
		sSkillSetOnFlag( pUnit, nSkill, nRequestedSkill, idWeapon, dwSeed, GameGetTick(pGame), TRUE );
 
	return TRUE;
}


//----------------------------------------------------------------------------
// Start skill functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float sSkillScheduleEvents( 
	SKILL_CONTEXT & context,
	SKILL_EVENT_HOLDER * holder,
	float fTimeOffset,
	float fModeDuration,
	BOOL bNoModeEvent,
	BOOL bForceLoopMode)
{
	ASSERT_RETVAL(context.game, 0.0f);
	ASSERT_RETVAL(context.unit, 0.0f);
	ASSERT_RETVAL(context.skillData, 0.0f);

	UNITMODE eMode = sSkillEventHolderGetMode(context.skillData, holder);
	eMode = SkillChangeModeByWeapon(context.game, context.unit, context.skill, eMode, context.idWeapon);

	if (fModeDuration == 0.0f)
	{
		fModeDuration = holder->fDuration;
	}

	if (fModeDuration == 0.0f)
	{
		context.weapon = UnitGetById(context.game, context.idWeapon);
		fModeDuration = sSkillGetModeDuration(context.game, context.unit, context.skillData, eMode, context.weapon);
	}

	if (!bNoModeEvent)
	{
		BOOL bLoopMode = bForceLoopMode ? TRUE : SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE);
		if (!bLoopMode || !UnitTestMode(context.unit, eMode))
		{
			int nTicksToMode = PrimeFloat2Int(fTimeOffset / GAME_TICK_TIME);
			float fDuration = bLoopMode ? 0.0f : fModeDuration;
			UnitRegisterEventTimed(context.unit, sHandleStartMode, &EVENT_DATA(context.skill, context.idWeapon, eMode, EventFloatToParam(fDuration)), nTicksToMode);
		}
	}

	for (int j = 0; j < holder->nEventCount; j++)
	{
		SKILL_EVENT * pSkillEvent = &holder->pEvents[j];

		const SKILL_EVENT_TYPE_DATA * skillEventTypeData = (const SKILL_EVENT_TYPE_DATA *)ExcelGetData(context.game, DATATABLE_SKILLEVENTTYPES, pSkillEvent->nType);
		ASSERTONCE(skillEventTypeData);
		if (!skillEventTypeData)
		{
			continue;
		}
		if (skillEventTypeData->bSubSkillInherit)
		{
			UNIT * target = SkillGetTarget(context.unit, context.skill, context.idWeapon);
			SkillSetTarget(context.unit, pSkillEvent->tAttachmentDef.nAttachedDefId, context.idWeapon, UnitGetId(target));

			VECTOR vTarget = UnitGetStatVector(context.unit, STATS_SKILL_TARGET_X, context.skill, 0);
			UnitSetStatVector(context.unit, STATS_SKILL_TARGET_X, pSkillEvent->tAttachmentDef.nAttachedDefId, 0, vTarget);
		}

		UNITMODE eMode = sSkillEventHolderGetMode(context.skillData, holder);
		float fEventTime = sSkillEventGetTime(context.unit, eMode, pSkillEvent);
		if (fEventTime < 1.0f)
		{
			float fTime = fEventTime * fModeDuration;
			if (fEventTime < 0.0f)
			{
				// these are events that happen only on the first time through a still - it doesn't happen on repeats
				if (context.dwFlags & SKILL_START_FLAG_IS_REPEATING)
				{
					continue;
				}
				fTime = -fTime;
			}
			SkillRegisterEvent(context.game, context.unit, pSkillEvent, context.skill, context.skillLevel, context.idWeapon, fTimeOffset + fTime, fModeDuration - fTime, fModeDuration + fTimeOffset );
		}
	}

	return fModeDuration;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillScheduleEvents( 
	GAME * game, 
	UNIT * unit, 
	int skill,
	int skillLevel,
	UNITID idWeapon,
	const SKILL_DATA * skillData,
	SKILL_EVENT_HOLDER * holder,
	float fTimeOffset,
	float fModeDuration,
	BOOL bNoModeEvent,
	BOOL bForceLoopMode,
	DWORD dwFlags)
{
	SKILL_CONTEXT context(game, unit, skill, skillLevel, skillData, idWeapon, dwFlags, 0);
	return sSkillScheduleEvents(context, holder, fTimeOffset, fModeDuration, bNoModeEvent, bForceLoopMode);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SKILL_EVENTS_DEFINITION * SkillGetWeaponMeleeSkillEvents( 
	GAME * pGame, 
	UNIT * pWeapon )
{
	SKILL_EVENTS_DEFINITION * pWeaponEvents = NULL;
	const MELEE_WEAPON_DATA * pMeleeData = WeaponGetMeleeData( pGame, pWeapon );
	if ( pMeleeData )
	{
		ASSERT_RETNULL ( pMeleeData->szSwingEvents[ 0 ] == 0 || pMeleeData->nSwingEventsId != INVALID_ID );
		if ( pMeleeData->nSwingEventsId != INVALID_ID )
		{
			pWeaponEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pMeleeData->nSwingEventsId );
		}
	}
	return pWeaponEvents;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillFilterTarget( 
	SKILL_CONTEXT & context,
	UNIT * target,
	const TARGET_QUERY_FILTER & tFilter,
	BOOL bStartingSkill,
	BOOL bDontCheckAttackerUnittype)
{
	ASSERT_RETTRUE(context.unit);
	ASSERT_RETTRUE(target);

	if (tFilter.nUnitType != UNITTYPE_ANY && !UnitIsA(target, tFilter.nUnitType))
	{
		return TRUE;
	}

	if (tFilter.nIgnoreUnitType > UNITTYPE_NONE && UnitIsA(target, tFilter.nIgnoreUnitType))
	{
		return TRUE;
	}

	const UNIT_DATA * pTargetUnitData = UnitGetData(target);
	ASSERT_RETTRUE(pTargetUnitData);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(!bDontCheckAttackerUnittype)
	{
		if(!s_UnitCanAttackUnit(context.unit, target))
		{
			return TRUE;
		}
	}
#endif

	BOOL bUnitIsDead = UnitTestMode(target, MODE_DEAD);
	BOOL bUnitIsDying = UnitTestMode(target, MODE_DYING);
	if( AppIsTugboat() &&
		UnitIsA( target, UNITTYPE_HEADSTONE ) )
	{
		bUnitIsDead = TRUE;
	}

	if (!UnitGetCanTarget(target) && ! TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES) )
	{
		return TRUE;
	}

	if( TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_BOTH_DEAD_AND_ALIVE ) == FALSE )
	{

		// only allow you to target dying if you are not starting a skill
		if (!((bUnitIsDying || bUnitIsDead) && UnitDataTestFlag(pTargetUnitData, UNIT_DATA_FLAG_SELECTABLE_DEAD) && context.skillData && SkillDataTestFlag(context.skillData, SKILL_FLAG_TARGET_SELECTABLE_DEAD)))
		{ 
			if (!TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_TARGET_DEAD_OR_DYING))
			{
				BOOL bTargetDying;
				if (bStartingSkill)
				{
					bTargetDying = TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START);
				}
				else
				{
					bTargetDying = TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START);
				}
				if (!bTargetDying && bUnitIsDying) 
				{
					return TRUE;
				}

				if (TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_DYING) && bTargetDying && !bUnitIsDying)
				{
					return TRUE;
				}

				BOOL bTargetDead = TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DEAD);

				if (!(bTargetDead && bUnitIsDying))
				{
					if ((bTargetDead  && !bUnitIsDead) || 
						(!bTargetDead &&  bUnitIsDead)) 
					{
						return TRUE;
					}
				}
			}
		}
	}

	// note, this line is replicated from a nearly identical line at the start of the function, do we really need to do this?
	if (!UnitIsA(target, tFilter.nUnitType))
	{
		return TRUE;
	}
	if ( TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ONLY_CHAMPIONS) && !MonsterIsChampion(target))
	{
		return TRUE;
	}

	if ( TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_CHAMPIONS) && MonsterIsChampion(target))
	{
		return TRUE;
	}

	if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_FLYERS ) )
	{
		if ( UnitGetStat( target, STATS_CAN_FLY ) )
			return TRUE;
	}

	if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_MERCHANTS ) )
	{
		if ( UnitIsMerchant( target ) ) //TODO: Tugboat needs to redefine what a merchant is. Merchant in tugboat is a person who sells items. But we also don't want to target quest givers or hireling sellers. ect ect...
			return TRUE;
	}

	//this use to be just a single state for tugboat but I like the multiple states...leaving in.
	for (int i = 0; i < TARGET_QUERY_FILTER::IGNORE_STATE_COUNT; i++)
	{
		if (tFilter.pnIgnoreUnitsWithState[i] != INVALID_ID && UnitHasState(context.game, target, tFilter.pnIgnoreUnitsWithState[i]))
		{
			if ( TESTBIT(tFilter.pdwFlags, SKILL_TARGETING_BIT_DONT_IGNORE_OWNED_STATE) )
			{
				STATS * pStats = GetStatsForStateGivenByUnit( tFilter.pnIgnoreUnitsWithState[i], target, UnitGetId( context.unit ) );
				if ( ! pStats )
					return TRUE;
			}
			else
			{
				return TRUE;
			}

		}
	}
	
	for (int i = 0; i < TARGET_QUERY_FILTER::ALLOW_ONLY_STATES_COUNT; i++)
	{
		if (tFilter.pnAllowUnitsOnlyWithState[i] == INVALID_ID)
		{
			continue;
		}
		if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_STATE_GIVEN_BY_OWNER ) &&				
			GetStatsForStateGivenByUnit(tFilter.pnAllowUnitsOnlyWithState[i], target, UnitGetId(context.unit)) == NULL)
		{
			return TRUE;
		}
		else if(!UnitHasState(context.game, target, tFilter.pnAllowUnitsOnlyWithState[i]))
		{
			return TRUE;
		}
	}

	if (UnitGetTeam(target) == TEAM_DESTRUCTIBLE)
	{
		if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES) || UnitDataTestFlag(UnitGetData(target), UNIT_DATA_FLAG_EVERYONE_CAN_TARGET) )
		{
			return FALSE;
		}

		if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_DESTRUCTABLES) )
		{
			return TRUE;
		}

		if (context.ultimateOwner == NULL)
		{
			context.ultimateOwner = UnitGetUltimateOwner(context.unit);
		}
		if (context.ultimateOwner && UnitIsA(context.ultimateOwner, UNITTYPE_PLAYER))
		{
			return FALSE;
		}
		return TRUE;
	}

	if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_CONTAINER ) )
	{
		return !UnitIsContainedBy(context.unit, target);
	}

	if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_PETS ) )
	{
		if (!PetIsPet(target) || PetGetOwner(target) != UnitGetId(context.unit))
		{
			return TRUE; // do not target nonpets that aren't owned by me
		}
	}

	if ( TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_PETS) )
	{
		if (PetIsPet(target))
		{
			return TRUE;  // do not target
		}
	}
	
	if ( ! TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM ) )
	{
		if (TestFriendly(context.game, context.unit, target))
		{
            if ( ! TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND ) )
			{
				return TRUE;
			}
		} 
		else if (TestHostile(context.game, context.unit, target)) 
		{
			if ( ! TESTBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY ) ) 
			{
				return TRUE;
			}
		}
		else
		{
			return TRUE;
		}
	}
	if (target && context.skillData)
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(context.game, DATATABLE_SKILLS, context.skillData->codeStartConditionOnTarget, &code_len);
		if (code_ptr && ! VMExecI(context.game, target, NULL, context.skillData->nId, sSkillGetSkillLevel(context), context.skillData->nId, context.skillLevel, INVALID_ID, code_ptr, code_len))
		{
			return TRUE;
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillIsValidTarget( 
	SKILL_CONTEXT & context,
	UNIT * target,
	BOOL bStartingSkill,
	BOOL * pbIsInRange = NULL,
	BOOL bIgnoreWeaponTargeting = FALSE,
	BOOL bDontCheckAttackerUnittype = FALSE)
{
	ASSERT_RETFALSE(context.skill != INVALID_ID);
	ASSERT_RETFALSE(context.skillData);

	if (!target)
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CAN_TARGET_UNIT) &&
			!SkillDataTestFlag(context.skillData, SKILL_FLAG_MUST_TARGET_UNIT))
		{
			if (pbIsInRange)
			{
				*pbIsInRange = TRUE;
			}
			return TRUE;
		}
		return FALSE;
	} 
	else if (SkillDataTestFlag(context.skillData, SKILL_FLAG_MUST_NOT_TARGET_UNIT))
	{
		return FALSE;
	}

	SKILL_CONTEXT t_context = context;

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON_TARGETING) && !bIgnoreWeaponTargeting)
	{
		int nWeaponSkill = ItemGetPrimarySkill(context.weapon);
		if (nWeaponSkill != INVALID_ID)
		{
			t_context.skill = (AppIsHellgate() ? nWeaponSkill : context.skill);
			t_context.skillData = SkillGetData(context.game, t_context.skill);
			t_context.skillLevel = 0;
		}
		else
		{
			if (SkillDataTestFlag(t_context.skillData, SKILL_FLAG_FORCE_USE_WEAPON_TARGETING))
			{
				return FALSE;
			}
		}
	}

	ROOM * unitRoom = UnitGetRoom(t_context.unit);
	ROOM * targetRoom = UnitGetRoom(target);
	if (unitRoom && targetRoom && unitRoom->pLevel != targetRoom->pLevel)
	{
		return FALSE;
	}

	if (sSkillFilterTarget(t_context, target, t_context.skillData->tTargetQueryFilter, bStartingSkill, bDontCheckAttackerUnittype))
	{
		return FALSE;
	}

	if (pbIsInRange)
	{
		float fSkillRangeMax = sSkillGetRange(t_context);

		if ( SkillDataTestFlag( t_context.skillData, SKILL_FLAG_IS_MELEE ))
		{
			*pbIsInRange = UnitCanMeleeAttackUnit( t_context.unit, target, t_context.weapon, 0, fSkillRangeMax, FALSE, NULL, FALSE );
		} else {
			float fSkillRangeMin = t_context.skillData->fRangeMin;

			*pbIsInRange = UnitsAreInRange(t_context.unit, target, fSkillRangeMin, fSkillRangeMax, NULL);
		}
		if ( SkillDataTestFlag( t_context.skillData, SKILL_FLAG_TEST_FIRING_CONE_ON_START ))
		{
			*pbIsInRange = UnitInFiringCone(t_context.unit, target, NULL, t_context.skillData->fFiringCone, NULL);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillCheckMeleeTargetRange(
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData,
	UNITID idWeapon,
	int nSkillLevel,
	BOOL bForce )
{
	ASSERT_RETURN( pUnit );

	if ( ! pUnit->pSkillInfo )
		return;

	GAME * pGame = UnitGetGame( pUnit );
	// right now, this is the only thing that we are updating...
	if ( !bForce && !SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_TARGET_IN_MELEE_RANGE_ON_START ) )
		return;

	UNIT * pWeapon = UnitGetById( pGame, idWeapon );
	float fMeleeRange = SkillGetRange( pUnit, nSkill, pWeapon, TRUE, nSkillLevel );
	int nTargetCount = pUnit->pSkillInfo->pTargets.Count();
	for ( int i = 0; i < nTargetCount; i++ )
	{
		SKILL_TARGET * pTarget = pUnit->pSkillInfo->pTargets.GetPointer( i );
		if ( pTarget &&
			 pTarget->nSkill == nSkill )
		{
			UNIT * pTargetUnit = UnitGetById( pGame, pTarget->idTarget );

			if ( pTargetUnit && UnitCanMeleeAttackUnit( pUnit, pTargetUnit, pWeapon, 0, fMeleeRange, FALSE, NULL, FALSE, 0.0f ) )
				pTarget->dwFlags |= SKILL_TARGET_FLAG_IN_MELEE_RANGE_ON_START;
			else
				pTarget->dwFlags &= ~SKILL_TARGET_FLAG_IN_MELEE_RANGE_ON_START;
		}
	}
}

//----------------------------------------------------------------------------
// this function might end with context.weapon freed!
//----------------------------------------------------------------------------
static BOOL sStartSkillGeneric( 
	SKILL_CONTEXT & context,
	const SKILL_EVENTS_DEFINITION * events)
{
	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);
	ASSERT_RETFALSE(context.skillData);
	if (!events)
	{
		return FALSE;
	}

	if (!context.weapon)
	{
		context.weapon = UnitGetById(context.game, context.idWeapon);
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CHECK_RANGE_ON_START))
	{
		UNIT * target = sSkillGetTarget(context);
		BOOL bIsInRange = FALSE;
		if (!sSkillIsValidTarget(context, target, TRUE, &bIsInRange) || !bIsInRange)
		{
			return FALSE;
		}
	}

	SKILL_EVENTS_DEFINITION * pWeaponEvents = NULL;
	SKILL_EVENTS_DEFINITION * pWeaponEventsOther = NULL;
	UNIT * pWeaponOther = NULL;
	if (context.weapon)
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_DO_MELEE_ITEM_EVENTS))
		{
			pWeaponEvents = SkillGetWeaponMeleeSkillEvents(context.game, context.weapon);
			// TRAVIS:This keeps us from getting doubled weapon-swings - we allow overlapping weapon skill events, and don't work this way
			if (AppIsHellgate() && SkillDataTestFlag(context.skillData, SKILL_FLAG_DO_MELEE_ITEM_EVENTS))
			{
				pWeaponOther = SkillGetLeftHandWeapon(context.game, context.unit, context.skill, context.weapon);

				if (pWeaponOther)
				{
					pWeaponEventsOther = SkillGetWeaponMeleeSkillEvents(context.game, pWeaponOther);
				}
			}
		}
	} 

	float fSkillDurationCurrent = 0.0f;
	for (int i = 0; i < events->nEventHolderCount; i++)
	{
		SKILL_EVENT_HOLDER * pHolder = &events->pEventHolders[i];
		float fModeDuration = sSkillScheduleEvents(context, pHolder, fSkillDurationCurrent, 0.0f, FALSE, FALSE);
		if (pWeaponEvents && pWeaponEvents->pEventHolders)
		{
			pHolder = &pWeaponEvents->pEventHolders[0];
			sSkillScheduleEvents(context, pHolder, fSkillDurationCurrent, fModeDuration, TRUE, FALSE);
		}
		// for dual melee - this helps the other weapon do its effects
		if (pWeaponEventsOther && pWeaponEventsOther->pEventHolders)
		{ 
			pHolder = &pWeaponEventsOther->pEventHolders[0];
			UNITID idWeaponOld = context.idWeapon;
			context.idWeapon = UnitGetId( pWeaponOther );
			sSkillScheduleEvents(context, pHolder, fSkillDurationCurrent, fModeDuration, TRUE, FALSE);
			context.idWeapon = idWeaponOld;
		}
		fSkillDurationCurrent += fModeDuration;
	}

	int nTicksToSkillEnd = PrimeFloat2Int(fSkillDurationCurrent / GAME_TICK_TIME);
	nTicksToSkillEnd = MAX(1, nTicksToSkillEnd);
	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE) && !SkillDataTestFlag(context.skillData, SKILL_FLAG_NO_IDLE_ON_STOP))
	{
		UNITMODE eIdleMode = (AppGetState() == APP_STATE_CHARACTER_CREATE) ? MODE_IDLE_BADASS : MODE_IDLE;
		UnitRegisterEventTimed(context.unit, sHandleStartMode, &EVENT_DATA(context.skill, context.idWeapon, eIdleMode, EventFloatToParam(0.0f)), nTicksToSkillEnd);
	}
	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_HOLD) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_ALL) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE))
	{
		UnitRegisterEventTimed(context.unit, sHandleSkillStop, &EVENT_DATA(context.skill, context.idWeapon), nTicksToSkillEnd);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStartSkillNoModes( 
	SKILL_CONTEXT & context,
	const SKILL_EVENTS_DEFINITION * events)
{
	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);
	ASSERT_RETFALSE(context.skillData);
	if (!events)
	{
		return TRUE;
	}

	for (int i = 0; i < events->nEventHolderCount; i++)
	{
		SKILL_EVENT_HOLDER * pHolder = &events->pEventHolders[i];

		for (int j = 0; j < pHolder->nEventCount; j++)
		{
			SKILL_EVENT * pSkillEvent = &pHolder->pEvents[j];
			UNITMODE eMode = sSkillEventHolderGetMode(context.skillData, pHolder);
			float fEventTime = sSkillEventGetTime(context.unit, eMode, pSkillEvent);
			// these are events that happen only on the first time through a still - it doesn't happen on repeats
			if (fEventTime < 0.0f)
			{
				if (context.dwFlags & SKILL_START_FLAG_IS_REPEATING)
				{
					continue;
				}
			}
			if (fEventTime < 1.0f)
			{
				SkillRegisterEvent(context.game, context.unit, pSkillEvent, context.skill, context.skillLevel, context.idWeapon, 0.0f, 0.0f);
			}
		}
	}
	if(!SkillDataTestFlag(context.skillData, SKILL_FLAG_DONT_STOP_REQUEST_AFTER_EXECUTE))
	{
		SkillStopRequest(context.unit, context.skill, TRUE, TRUE); 
	}
	// TRAVIS - do this so that they can kick off any stat drains or >1.0f events. OTherwise how do they happen?
	if (AppIsTugboat())
	{
		float fSkillDurationCurrent = 0.0f;
		for ( int i = 0; i < events->nEventHolderCount; i++)
		{
			SKILL_EVENT_HOLDER * pHolder = &events->pEventHolders[ i ];
			float fModeDuration = 0;
			UNITMODE eMode = sSkillEventHolderGetMode(context.skillData, pHolder );
			eMode = SkillChangeModeByWeapon(context.game, context.unit, context.skill, eMode, context.idWeapon);

			if (fModeDuration == 0.0f)
			{
				fModeDuration = pHolder->fDuration;
			}

			if (fModeDuration == 0.0f)
			{
				context.weapon = UnitGetById(context.game, context.idWeapon);
				fModeDuration = sSkillGetModeDuration(context.game, context.unit, context.skillData, eMode, context.weapon);
			}	

			fSkillDurationCurrent += fModeDuration;
		}

		int nTicksToSkillEnd = PrimeFloat2Int(fSkillDurationCurrent / GAME_TICK_TIME);

		if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_HOLD) && 
			!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_ALL) && 
			!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE) && 
			!SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE))
		{
			UnitRegisterEventTimed(context.unit, sHandleSkillStop, &EVENT_DATA(context.skill, context.idWeapon), nTicksToSkillEnd);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStartSkillWarmUp( 
	SKILL_CONTEXT & context,
	const SKILL_EVENTS_DEFINITION * events)
{
	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);
	ASSERT_RETFALSE(context.skillData);
	ASSERT_RETFALSE(events);
	ASSERT_RETFALSE(events->nEventHolderCount >= 2);

	SKILL_EVENT_HOLDER * pWarmupHolder = &events->pEventHolders[0];
	if(AppIsHellgate())
	{
		ASSERT_RETFALSE(pWarmupHolder->nUnitMode == MODE_FIRE_RIGHT_WARMUP || pWarmupHolder->nUnitMode == MODE_FIRE_RIGHT_HOLD_WARMUP);
	}
	ASSERT_RETFALSE(context.skillData->nWarmupTicks > 0);

	BOOL bDoWarmup = TRUE;
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE) && (context.dwFlags & SKILL_START_FLAG_IS_REPEATING) != 0)
	{
		bDoWarmup = FALSE;
	}

	context.weapon = UnitGetById(context.game, context.idWeapon);

	float fSkillDurationCurrent = 0.0f;
	if (bDoWarmup)
	{
		int nWarmUpTicks = sSkillGetWarmUpTicks(context.unit, context.weapon, context.skillData);

		float fWarmupDuration = (float)nWarmUpTicks * GAME_TICK_TIME;

		float fModeDuration = sSkillScheduleEvents(context, pWarmupHolder, fSkillDurationCurrent, fWarmupDuration, FALSE, FALSE);
		REF(fModeDuration);

		fSkillDurationCurrent += fWarmupDuration;

		UnitRegisterEventTimed(context.unit, sHandleEndMode, &EVENT_DATA(context.skill, context.idWeapon, pWarmupHolder->nUnitMode), nWarmUpTicks);
	}

	for (int i = 1; i < events->nEventHolderCount; i++)
	{
		SKILL_EVENT_HOLDER * pHolder = &events->pEventHolders[ i ];
		BOOL bForceLoop = SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE);
		float fModeDuration = sSkillScheduleEvents(context, pHolder, fSkillDurationCurrent, 0.0f, FALSE, bForceLoop);
		fSkillDurationCurrent += fModeDuration;
	}

	int nTicksToSkillEnd = PrimeFloat2Int(fSkillDurationCurrent / GAME_TICK_TIME);
	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE) && !SkillDataTestFlag(context.skillData, SKILL_FLAG_NO_IDLE_ON_STOP))
	{
		UNITMODE eIdleMode = (AppGetState() == APP_STATE_CHARACTER_CREATE) ? MODE_IDLE_BADASS : MODE_IDLE;
		UnitRegisterEventTimed(context.unit, sHandleStartMode, &EVENT_DATA(context.skill, context.idWeapon, eIdleMode, EventFloatToParam(0.0f)), nTicksToSkillEnd);
	}
	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_HOLD) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_ALL) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE) && 
		!SkillDataTestFlag(context.skillData, SKILL_FLAG_LOOP_MODE))
	{
		UnitRegisterEventTimed(context.unit, sHandleSkillStop, &EVENT_DATA(context.skill, context.idWeapon), nTicksToSkillEnd);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//typedef BOOL FP_SKILL_START(GAME * pGame, UNIT * pUnit, int nSkill, UNITID idWeapon, 
//							const SKILL_DATA * pSkillData, SKILL_EVENTS_DEFINITION * pEvents, DWORD dwFlags );
typedef BOOL FP_SKILL_START(SKILL_CONTEXT & context, const SKILL_EVENTS_DEFINITION * events);

struct SKILL_START_FUNC_CHART
{
	FP_SKILL_START *	pfnStartFunction;
	BOOL				bUsesWeaponSkill;
	BOOL				bCanRepeat;
	BOOL				bSetSkillOn;
	BOOL				bAllAtOnce;
};

static SKILL_START_FUNC_CHART sgpSkillStartTbl[] =
{//		function						uses weapon		can repeat	skill on	All at once
	{	NULL,							FALSE,			FALSE,		FALSE,		FALSE,			}, // SKILL_STARTFUNCTION_NULL
	{	sStartSkillGeneric,				FALSE,			TRUE,		TRUE,		FALSE,			}, // SKILL_STARTFUNCTION_GENERIC
	{	sStartSkillNoModes,				FALSE,			FALSE,		FALSE,		TRUE,			}, // SKILL_STARTFUNCTION_NOMODES
	{	sStartSkillWarmUp,				FALSE,			TRUE,		TRUE,		FALSE,			}, // SKILL_STARTFUNCTION_WARMUP
};
static const int sgnSkillStartTblCount = arrsize(sgpSkillStartTbl);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSkillSendStartMessage(
									 GAME * pGame,
									 UNIT * pUnit,
									 int nSkill,
									 UNITID idTarget,
									 const VECTOR & vTarget,
									 DWORD dwStartFlags,
									 DWORD dwSeed)
{
	ASSERT_RETURN ( IS_SERVER( pGame ) );

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	// do not start if this skill was started as server only
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SERVER_ONLY ) ||
		(dwStartFlags & SKILL_START_FLAG_SERVER_ONLY) != 0 )
	{
		return;
	}

	BOOL bSendToAll = ((dwStartFlags & SKILL_START_FLAG_INITIATED_BY_SERVER) != 0);

	// which start flags are we supposed to send to the client
	DWORD dwStartFlagsToSend = 0;
	if (dwStartFlags & SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT)
	{
		dwStartFlagsToSend = dwStartFlags;
	}
	else if(dwStartFlags & SKILL_START_FLAG_INITIATED_BY_SERVER)
	{
		dwStartFlagsToSend |= SKILL_START_FLAG_INITIATED_BY_SERVER;
	}

	if ( idTarget == INVALID_ID )
	{
		MSG_SCMD_SKILLSTARTXYZ tMsg;
		tMsg.id = UnitGetId( pUnit );
		tMsg.wSkillId = DOWNCAST_INT_TO_WORD(nSkill);
		tMsg.vTarget = vTarget;
		tMsg.dwStartFlags = dwStartFlagsToSend;
		tMsg.dwSkillSeed = dwSeed;
		tMsg.vUnitPosition = UnitGetPosition(pUnit);
		SkillSendMessage( pGame, pUnit, SCMD_SKILLSTARTXYZ, &tMsg, bSendToAll );
	} else if ( VectorIsZero(vTarget) ) 
	{
		MSG_SCMD_SKILLSTARTID tMsg;
		tMsg.id = UnitGetId( pUnit );
		tMsg.wSkillId = DOWNCAST_INT_TO_WORD(nSkill);
		tMsg.idTarget = idTarget;
		tMsg.dwStartFlags = dwStartFlagsToSend;
		tMsg.dwSkillSeed = dwSeed;
		tMsg.vUnitPosition = UnitGetPosition(pUnit);
		SkillSendMessage( pGame, pUnit, SCMD_SKILLSTARTID, &tMsg, bSendToAll );
	} else {
		MSG_SCMD_SKILLSTART tMsg;
		tMsg.id = UnitGetId( pUnit );
		tMsg.wSkillId = DOWNCAST_INT_TO_WORD(nSkill);
		tMsg.idTarget = idTarget;
		tMsg.vTarget  = vTarget;
		tMsg.dwStartFlags = dwStartFlagsToSend;
		tMsg.dwSkillSeed = dwSeed;
		tMsg.vUnitPosition = UnitGetPosition(pUnit);
		SkillSendMessage( pGame, pUnit, SCMD_SKILLSTART, &tMsg, bSendToAll );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillExecute(
	SKILL_CONTEXT & context)
{
//	trace("sSkillExecute called (%c) for skill %d. Tick: %d, power: %d, cool: %d\n", (IS_CLIENT(context.game))? 'c':'s', context.skill, GameGetTick(context.game), UnitGetStat(context.unit, STATS_POWER_CUR), UnitGetStat(context.unit, STATS_SKILL_COOLING, context.skill));

	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);

	context.skillData = SkillGetData(context.game, context.skill);
	ASSERT_RETFALSE(context.skillData);
	ASSERT_RETFALSE(context.skillData->nStartFunc >= 0 && context.skillData->nStartFunc < sgnSkillStartTblCount);

	if (context.skillRequested == INVALID_ID)
	{
		context.skillRequested = context.skill;
	}

	ASSERT_RETFALSE(!SkillDataTestFlag(context.skillData, SKILL_FLAG_WEAPON_IS_REQUIRED) || context.idWeapon != INVALID_ID);
	context.weapon = UnitGetById(context.game, context.idWeapon);

	SKILL_START_FUNC_CHART * pFunctionChart = &sgpSkillStartTbl[context.skillData->nStartFunc];
	ASSERT_RETFALSE(pFunctionChart);
	if (!pFunctionChart->pfnStartFunction)
	{
		return FALSE;
	}

	if (context.weapon && sWeaponUsesFiringError(context.weapon))
	{
		StateClearAllOfType( context.weapon, STATE_FIRING_ERROR_DECREASING_WEAPON );

		STATS * pStats = NULL;
		if ( IS_SERVER( context.weapon ) )
			pStats = s_StateSet(context.weapon, context.unit, STATE_FIRING_ERROR_INCREASING_WEAPON, 0);
		else if ( ! UnitHasState( context.game, context.weapon, STATE_FIRING_ERROR_INCREASING_WEAPON ))
			pStats = c_StateSet(context.weapon, context.unit, STATE_FIRING_ERROR_INCREASING_WEAPON, 0, 0, INVALID_ID );

		if (pStats)
		{// since this stat acrues to the unit and the unit might want to modify the weapon value, grab the value from the unit
			int nSource = UnitGetStat(context.unit, STATS_FIRING_ERROR_INCREASE_SOURCE_WEAPON);
			StatsSetStat(context.game, pStats, STATS_FIRING_ERROR_INCREASE_WEAPON, nSource);
		}
	}

	if ( context.weapon && UnitUsesEnergy(context.weapon))
	{
		StateClearAllOfType( context.weapon, STATE_ENERGY_INCREASING );

		STATS * pStats = NULL;
		if ( IS_SERVER( context.weapon ) )
			pStats = s_StateSet(context.weapon, context.unit, STATE_ENERGY_DECREASING, 0);
		else if ( ! UnitHasState( context.game, context.weapon, STATE_ENERGY_DECREASING ))
			pStats = c_StateSet(context.weapon, context.unit, STATE_ENERGY_DECREASING, 0, 0, INVALID_ID );
		if (pStats)
		{
			int nSource = UnitGetStat(context.weapon, STATS_ENERGY_DECREASE_SOURCE);
			int nSourcePct = UnitGetStat(context.unit, STATS_ENERGY_DECREASE_SOURCE_PCT);
			nSource = PCT( nSource, 100 + nSourcePct );
			nSource = MAX( nSource, 1 );
			StatsSetStat(context.game, pStats, STATS_ENERGY_DECREASE, nSource);
		}
	}

	// determine skill level
	sSkillGetSkillLevel(context);

	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_POWER_ON_EVENT) && 
		(context.dwFlags & SKILL_START_FLAG_IGNORE_POWER_COST) == 0)
	{
		if (!sSkillTakePowerCost(context))
		{
			return FALSE;
		}

		// KCK: An issue exists where when power is low, the server will start a skill, decrement the power and send the new power
		// value to the client before the client is able to start the skill. When the client processes its loop, it will then not
		// have enough power to start the skill and the two sides will get out of sync. What this code does is force the client
		// to start the skill in this case. If the skill is already running on the client this should just be ignored.
		// KCK: This isn't working quite right yet and I have other priorities. I'm leaving this code in but commented becsause I'd
		// like to come back to this.
/*		int	nCost = SkillGetPowerCost( context.unit, context.skillData, sSkillGetSkillLevel(context) );
		int nCur = UnitGetStat( context.unit, STATS_POWER_CUR );
		int	nRegen = UnitGetStatRegen( context.unit, STATS_POWER_CUR );
		trace("sSkillExecute: %s starting skill %d. Cost: %d, remaining: %d, regen: %d\n", (IS_SERVER( context.game ))? "Server":"Client", context.skill, nCost, nCur, nRegen);
		if ( IS_SERVER( context.game ) && nCost > 0 && nCur <= nCost )
		{
			trace("sSkillExecute: Server forcing client to start skill #%d\n", context.skill);
			DWORD dwFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER | SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_IGNORE_COOLDOWN | SKILL_START_FLAG_IGNORE_POWER_COST;
			s_sSkillSendStartMessage(context.game, context.unit, context.skill, INVALID_ID, VECTOR(0), dwFlags, context.dwSeed);
		}
*/
	}

	const SKILL_EVENTS_DEFINITION * pEvents = (context.skillData->nEventsId != INVALID_ID) ? (const SKILL_EVENTS_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_SKILL_EVENTS, context.skillData->nEventsId ) : NULL;

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_IS_AGGRESSIVE))
	{
		if (pFunctionChart->bAllAtOnce)
		{
			if (UnitGetStat(context.unit, STATS_SKILL_LAST_AGGRESSIVE_TIME) != -1) // another skill is probably on
			{
				UnitSetStat(context.unit, STATS_SKILL_LAST_AGGRESSIVE_TIME, GameGetTick(context.game));
			}
		} 
		else 
		{
			UnitSetStat(context.unit, STATS_SKILL_LAST_AGGRESSIVE_TIME, -1);
		}
	}

	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_DONT_COOLDOWN_ON_START))
	{
		if ((context.dwFlags & SKILL_START_FLAG_DONT_SET_COOLDOWN) == 0)
		{
			sSkillStartCooldown(context, 0);
		}
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_CHECK_TARGET_IN_MELEE_RANGE_ON_START))
	{
		SkillCheckMeleeTargetRange( context.unit, context.skill, context.skillData, context.idWeapon, context.skillLevel, FALSE );
	}

	if (!pFunctionChart->pfnStartFunction(context, pEvents))
	{
		if (SkillDataTestFlag(context.skillData, SKILL_FLAG_SKILL_IS_ON))
		{
			sSkillSetOnFlag(context.unit, context.skill, context.skillRequested, context.idWeapon, 0, 0, FALSE);
		}
		return FALSE;
	} 
	else if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE) && context.skillData->nMinHoldTicks == 0)
	{ 
		// we restarted the skill, so set the time accordingly
		SKILL_IS_ON * pSkillIsOn = SkillIsOn(context.unit, context.skill, context.idWeapon);
		if (pSkillIsOn)
		{
			pSkillIsOn->tiStartTick = GameGetTick(context.game);
		}
	}

	// the weapon could have been freed by the start function
	context.weapon = UnitGetById(context.game, context.idWeapon);

	// At this point our skill has been started, so trigger an aggressive event if necessary
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_IS_AGGRESSIVE))
	{
		UnitTriggerEvent(context.game, EVENT_AGGRESSIVE_SKILL_STARTED, context.unit, NULL, NULL);
	}
	UnitTriggerEvent(context.game, EVENT_SKILL_START, context.unit, NULL, &EVENT_DATA(context.skill, context.idWeapon));

	// we are just repeating skills that use weapons since we use the cooldown time
	if (pFunctionChart->bCanRepeat &&
		(context.weapon || !SkillDataTestFlag(context.skillData, SKILL_FLAG_WEAPON_IS_REQUIRED)) && 
		(context.dwFlags & SKILL_START_FLAG_NO_REPEAT_EVENT) == 0 &&
		(SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_ALL) || SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE)))
	{
		int nTicksBetweenRepeats = 0;
		if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_REPEAT_FIRE))
		{
			int nFireTime = sSkillGetWarmUpTicks(context.unit, context.weapon, context.skillData);

			float fDuration = SkillGetDuration(context.game, context.unit, context.skill, TRUE, context.weapon);
			if (AppIsTugboat() && UnitTestFlag(context.unit, UNITFLAG_NOMELEEBLEND) == 0 &&
				SkillDataTestFlag(context.skillData, SKILL_FLAG_NO_BLEND) == 0)
			{
				if (context.weapon && UnitIsDualWielding(context.unit, context.skill))
				{
					if (WEAPON_INDEX_RIGHT_HAND == WardrobeGetWeaponIndex(context.unit, UnitGetId(context.weapon)))
					{
						fDuration *= RIGHT_SWING_MULTIPLIER;
					}
					else
					{
						fDuration *= LEFT_SWING_MULTIPLIER;
					}
				}
				else
				{
					fDuration *= RIGHT_SWING_MULTIPLIER;
				}
			}			
			nFireTime += (int) (fDuration * GAME_TICKS_PER_SECOND_FLOAT) + 1; 
			nTicksBetweenRepeats = MAX(nTicksBetweenRepeats, nFireTime);
		} 
		else 
		{
			if ((context.dwFlags & SKILL_START_FLAG_IS_REPEATING) == 0)
			{
				nTicksBetweenRepeats = sSkillGetWarmUpTicks(context.unit, context.weapon, context.skillData);
			}
			nTicksBetweenRepeats += SkillGetCooldown(context.game, context.unit, context.weapon, context.skill, context.skillData, 0, context.skillLevel);
		}

		nTicksBetweenRepeats = MAX(nTicksBetweenRepeats, 1);

		UnitRegisterEventTimed(context.unit, sHandleSkillRepeat, &EVENT_DATA(context.skill, context.idWeapon, TRUE, context.skillRequested), nTicksBetweenRepeats);
	}

	// add the stats list for this skill
	if (IS_SERVER(context.game))
	{
		UNIT *skillTarget = SkillGetTarget(context.unit, context.skill, context.idWeapon); //sets the target of the skill

		if (skillTarget && SkillDataTestFlag(context.skillData, SKILL_FLAG_ANGERS_TARGET_ON_EXECUTE))
		{
			BOOL bInRange = FALSE;
			if (sSkillIsValidTarget(context, skillTarget, FALSE, &bInRange) && bInRange)
			{
				// well, they tried to attack...
				//   And apparently an AI can be angry but have no attacker so they don't do anything.
				UnitSetStat(skillTarget, STATS_AI_LAST_ATTACKER_ID, UnitGetId(context.unit));
				AI_AngerOthers(context.game, skillTarget, context.unit);
			}
		}

		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(context.game, DATATABLE_SKILLS, context.skillData->codeStatsOnSkillStart, &code_len);
		struct STATS * pStatsList = NULL;
		if (code_ptr)
		{

			// TRAVIS: if we're doing the same skill and they're overlapping we REALLY need to
			// pull out the stats applied by the previous version of same - otherwise they'll stack - but only one will
			// get removed on the stop! Bad!
			STATS * pStats = StatsGetModList( context.unit, SELECTOR_SKILL );
			while ( pStats )
			{ // we don't want to remove other skills' stats lists
				DWORD dwParam = INVALID_ID;
				UNITID idStatsWeapon = StatsGetStatAny( pStats, STATS_SKILL_STATSLIST, &dwParam );
				if ( dwParam == (DWORD)context.skill && idStatsWeapon == context.idWeapon )
				{
					StatsListRemove( context.game, pStats );
					StatsListFree( context.game, pStats );
					break;
				}
				pStats = StatsGetModNext( pStats, SELECTOR_SKILL );
			}

			pStatsList = StatsListInit(context.game);
			VMExecI(context.game, context.unit, skillTarget, pStatsList, context.skill, context.skillLevel, context.skill, context.skillLevel, INVALID_ID, code_ptr, code_len);
			StatsSetStat(context.game, pStatsList, STATS_SKILL_STATSLIST, context.skill, context.idWeapon); // so that we can tell this skill from other ones
			StatsListAdd(context.unit, pStatsList, TRUE, SELECTOR_SKILL);
		}

		if (AppIsTugboat() && context.skillData->eUsage != USAGE_PASSIVE)
		{
			UNIT *pWeaponUnit = UnitGetById( context.game, context.idWeapon );
			StateSetAttackStart( context.unit, skillTarget, pWeaponUnit, TRUE);			
		}	
	}

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(context.game) && SkillDataTestFlag(context.skillData, SKILL_FLAG_IS_MELEE))
	{
		c_sUnitBeginTrackingMeleeBones(context.game, context.unit);
	}
#endif



	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillIsValidRequest(
	SKILL_CONTEXT & context,
	UNIT * target,
	const VECTOR & vTarget)
{
	if ((context.dwFlags & SKILL_START_FLAG_INITIATED_BY_SERVER) != 0)
	{
		return TRUE;
	}

	if (IS_SERVER(context.game) && UnitIsPlayer(context.unit))
	{
		if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_ALLOW_REQUEST))
		{
			return FALSE;
		}
	}

	ASSERT_RETFALSE((context.dwFlags & SKILL_START_FLAG_IGNORE_COOLDOWN) == 0);
	ASSERT_RETFALSE((context.dwFlags & SKILL_START_FLAG_IGNORE_POWER_COST) == 0);
	ASSERT_RETFALSE((context.dwFlags & SKILL_START_FLAG_DONT_SET_COOLDOWN) == 0);

	// do we need to verify the targets?
	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_VERIFY_TARGETS_ON_REQUEST))
	{
		if (!SkillIsValidTarget(context.game, context.unit, target, context.weapon, context.skill, TRUE, NULL))
		{
			return FALSE;
		}
		if (!target && vTarget != INVALID_POSITION) 
		{	
			// check range for positional target.
			float fSkillRangeMin = context.skillData->fRangeMin;
			float fSkillRangeMax = SkillGetRange(context.unit, context.skill, context.weapon);
			ONCE
			{
				if (fSkillRangeMin == 0.0f && fSkillRangeMax == 0.0f) 
				{
					break;
				}
				// no range limit data.
				ASSERTV_BREAK(fSkillRangeMax > fSkillRangeMin, "No valid range of skill ranges for skill %d", context.skill);
				if (!UnitIsInRange(context.unit, vTarget, fSkillRangeMin, fSkillRangeMax, NULL))
				{
					return FALSE;
				}
			}
		}
	}

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_REQUIRES_SKILL_LEVEL))
	{
		if (sSkillGetSkillLevel(context) <= 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillStartRequest(
	SKILL_CONTEXT & context,
	UNITID idTarget,
	const VECTOR & vTarget,
	DWORD dwRandSeed)
{
	ASSERT_RETFALSE(context.game);
	ASSERT_RETFALSE(context.unit);
	ASSERT_RETFALSE(context.skill != INVALID_ID);
	ASSERT_RETFALSE(context.skillData);

	UNITLOG_ASSERTX_RETFALSE(sSkillCanRequest(context), context.skillData->szName, context.unit);

	UNITLOG_ASSERTX_RETFALSE(context.unit->pSkillInfo && context.unit->pSkillInfo->pRequests.Count() < MAX_SKILLS_ON, 
		"too many skills requested", pUnit);
	
	UNIT * target = UnitGetById(context.game, idTarget);

	if (SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON))
	{
		UNIT * weapons[MAX_WEAPON_LOCATIONS_PER_SKILL];
		UnitGetWeapons(context.unit, context.skill, weapons, FALSE);
		context.weapon = weapons[0];
		context.idWeapon = UnitGetId(weapons[0]);
	}

	if (IS_SERVER(context.game)) 
	{
		UNITLOG_ASSERT_RETFALSE(sSkillIsValidRequest(context, target, vTarget), context.unit);
	}

	SkillSetTarget(context.unit, context.skill, INVALID_ID, idTarget);
	SkillSetTarget(context.unit, context.skill, context.idWeapon, idTarget);

	if (!SkillDataTestFlag(context.skillData, SKILL_FLAG_KEEP_TARGET_POSITION_ON_REQUEST))
	{
		UnitSetStatVector(context.unit, STATS_SKILL_TARGET_X, context.skill, 0, vTarget);
	}



	// pick a melee skill now - before we need to know which skill to use in UpdateActiveSkills()
	if (IS_CLIENT(context.unit) &&
		 SkillDataTestFlag(context.skillData, SKILL_FLAG_TRACK_REQUEST) &&
		 SkillDataTestFlag(context.skillData, SKILL_FLAG_USES_WEAPON))
	{
		UNIT * weapons[MAX_WEAPON_LOCATIONS_PER_SKILL];
		UnitGetWeapons(context.unit, context.skill, weapons, FALSE);
		int nWeaponSkill = ItemGetPrimarySkill(weapons[0]);
		if (nWeaponSkill == INVALID_ID)
		{
			int nFallbackSkill = sSkillFindFallbackSkill(context.game, context.unit, target, context.skillData);
			UnitGetWeapons(context.unit, nFallbackSkill, weapons, FALSE);
			nWeaponSkill = ItemGetPrimarySkill(weapons[0]);
		}
		if (nWeaponSkill != INVALID_ID)
		{
			const SKILL_DATA * weaponSkillData = SkillGetData(context.game, nWeaponSkill);
			if (SkillDataTestFlag(weaponSkillData, SKILL_FLAG_SELECTS_A_MELEE_SKILL))
			{
				sSkillSelectMeleeSkill(context.unit, context.skill, idTarget);
			}
		}
	}

	BOOL retval = TRUE;
	// KCK: If the server is forcing us to start the skill, do so instead of starting the request process,
	// even if this is normally a requested skill
	// KCK: Removed for the time being while I finish this
//	if ( SkillDataTestFlag(context.skillData, SKILL_FLAG_TRACK_REQUEST) &&
//		!(context.dwFlags & SKILL_START_FLAG_INITIATED_BY_SERVER ) )
	if ( SkillDataTestFlag(context.skillData, SKILL_FLAG_TRACK_REQUEST))
	{
		SkillSetRequested(context.unit, context.skill, dwRandSeed, TRUE);
	} 
	else 
	{
		BOOL bTestCanStartSkill = !(context.dwFlags & SKILL_START_FLAG_INITIATED_BY_SERVER ) != 0;
		if ( !bTestCanStartSkill && SkillDataTestFlag( context.skillData, SKILL_FLAG_ALWAYS_TEST_CAN_START_SKILL ) )
			bTestCanStartSkill = TRUE;

		if ( bTestCanStartSkill &&
			 !sSkillCanStart(context, NULL, FALSE))
		{
			retval = FALSE;
			for ( int i = 0; i < NUM_FALLBACK_SKILLS; i++ )
			{
				if ( context.skillData->pnFallbackSkills[ i ] == INVALID_ID )
					continue;
				int nSkillPrev = context.skill;
				const SKILL_DATA * pSkillDataPrev = context.skillData;
				context.skill = context.skillData->pnFallbackSkills[ i ];
				context.skillData = SkillGetData( context.game, context.skill );
				if (sSkillCanStart(context, NULL, FALSE))
				{
					retval = sSkillStart(context.game, context.unit, context.skill, context.skill, context.idWeapon, context.dwFlags, dwRandSeed, context.skillLevel);
					break;
				}
				context.skill = nSkillPrev;
				context.skillData = pSkillDataPrev;
			}
		}
		else
		{
			retval = sSkillStart(context.game, context.unit, context.skill, context.skill, context.idWeapon, context.dwFlags, dwRandSeed, context.skillLevel);
		}
	}

	if (IS_SERVER(context.game) && ! SkillDataTestFlag( context.skillData, SKILL_FLAG_SERVER_ONLY ) && retval )
	{
		s_sSkillSendStartMessage(context.game, context.unit, context.skill, idTarget, vTarget, context.dwFlags, dwRandSeed);
	}
#if !ISVERSION(SERVER_VERSION)
	else if (IS_CLIENT(context.game) && context.unit == GameGetControlUnit(context.game) && 
		(context.dwFlags & (SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_DONT_SEND_TO_SERVER)) == 0 && 
		retval)	// why send a non-startable skill? That's just wasted traffic!
	{
		c_SendSkillStart(UnitGetPosition(context.unit), DOWNCAST_INT_TO_WORD(context.skill), idTarget, vTarget);
	}
#endif


	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillStartRequest(
	GAME * game,
	UNIT * unit,
	int skill,
	UNITID idTarget,
	const VECTOR & vTarget,
	DWORD dwStartFlags,
	DWORD dwRandSeed,
	int skillLevel)	// = 0
{
//	trace("SkillStartRequest called (%c) for skill %d. Tick: %d, power: %d, cool: %d\n", (IS_CLIENT(game))? 'c':'s', skill, GameGetTick(game), UnitGetStat(unit, STATS_POWER_CUR), UnitGetStat(unit, STATS_SKILL_COOLING, skill));

	SKILL_CONTEXT context(game, unit, skill, skillLevel, INVALID_ID, dwStartFlags, dwRandSeed);
	return sSkillStartRequest(context, idTarget, vTarget, dwRandSeed);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillStart(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	int nRequestedSkill,
	UNITID idWeapon,
	DWORD dwStartFlags,
	DWORD dwRandSeed,
	int nSkillLevel)
{
//	trace("sSkillStart called (%c) for skill %d. Tick: %d, power: %d, cool: %d\n", (IS_CLIENT(pGame))? 'c':'s', nSkill, GameGetTick(pGame), UnitGetStat(pUnit, STATS_POWER_CUR), UnitGetStat(pUnit, STATS_SKILL_COOLING, nSkill));

	ASSERT_RETFALSE( pGame );
	ASSERT_RETFALSE( pUnit );

	StateClearAllOfType(pUnit, STATE_REMOVE_ON_DO_SKILL);
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	BOOL bResult = TRUE;
	UNIT * pWeapon = UnitGetById( pGame, idWeapon );

	int nStaggerTime = 0;
	if ( dwStartFlags & SKILL_START_FLAG_STAGGER )
		nStaggerTime = pWeapon ? SkillGetCooldown( pGame, pUnit, pWeapon, nSkill, pSkillData, 0 ) / 2 : 0;

	BOOL bDelayStart = FALSE;

	if ( nStaggerTime )
	{
		bDelayStart = TRUE;
	}

	if ( !bDelayStart && 
		(dwStartFlags & (SKILL_START_FLAG_INITIATED_BY_SERVER )) == 0 &&
		! SkillCanStart( pGame, pUnit, nSkill, idWeapon, NULL, FALSE, FALSE, dwStartFlags ) )
	{
		bDelayStart = TRUE;
	}

	if ( (dwStartFlags & SKILL_START_FLAG_DELAY_MELEE) != 0 &&
		sShouldDelayMeleeAttack( pGame, pUnit, nSkill, pWeapon ) )
	{
		bDelayStart = TRUE;
	}

	// TRAVIS - why would we set a delayed skill to be 'on'? It'll get set to on
	// on the delayed repeat event
	if ( !bDelayStart &&
		SkillDataTestFlag( pSkillData, SKILL_FLAG_SKILL_IS_ON ) &&
		!SkillIsOn( pUnit, nSkill, idWeapon ) )
	{
		GAME_TICK tiGameTick = GameGetTick( pGame );
		if ( bDelayStart )
			tiGameTick += nStaggerTime;
		sSkillSetOnFlag( pUnit, nSkill, nRequestedSkill, idWeapon, dwRandSeed, tiGameTick, TRUE );
	}

	EVENT_DATA tEventData( nSkill, idWeapon, FALSE, nRequestedSkill );
	BOOL bRepeatExists = UnitHasTimedEvent( pUnit, sHandleSkillRepeat, CheckEventParam12, &tEventData );

	if ( !bDelayStart && ! bRepeatExists )
	{
		sSkillSetSeed( pGame, pUnit, pWeapon, nSkill, dwRandSeed );		

		SKILL_CONTEXT skill_context(pGame, pUnit, nSkill, nSkillLevel, nRequestedSkill, 0, idWeapon, dwStartFlags, dwRandSeed);
		if (!sSkillExecute(skill_context))
		{
			bDelayStart = TRUE;
		} 
	}

	if ( bDelayStart && ! bRepeatExists )
	{
		bResult = FALSE;

		UnitRegisterEventTimed( pUnit, sHandleSkillRepeat, &tEventData, nStaggerTime ? nStaggerTime : 1 );
	} 

	if(SkillDataTestFlag(pSkillData, SKILL_FLAG_SERVER_ONLY))
	{
		dwStartFlags |= SKILL_START_FLAG_SERVER_ONLY;
	}


#ifdef TRACE_SYNCH
	const VECTOR* tgt = UnitGetMoveTarget(pUnit);

	trace("SkillStartXYZ[%c]: UNIT:" ID_PRINTFIELD "  POS:(%4.4f,%4.4f,%4.4f)  TGT:(%4.4f,%4.4f,%4.4f) SKILL:%d  TIME:%d\n",
		IS_SERVER(pGame) ? 'S' : 'C',
		ID_PRINT(UnitGetId(pUnit)),
		pUnit->vPosition.fX, pUnit->vPosition.fY, pUnit->vPosition.fZ,
		tgt ? tgt->fX : 0.0f, tgt ? tgt->fY : 0.0f, tgt ? tgt->fZ : 0.0f,
		nSkill,
		GameGetTick(UnitGetGame(pUnit)));
#endif
		
	return bResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STAT_CHART 
{
	WORD	wStat;
};
static const STAT_CHART sgptSkillStats[] = 
{
	{ STATS_SKILL_TARGET_X	},
	{ STATS_SKILL_TARGET_Y	},
	{ STATS_SKILL_TARGET_Z	},
};
static const int sgnSkillStats = arrsize(sgptSkillStats);


//----------------------------------------------------------------------------
// Get hit, death or something has interrupted the skill.  Stop skill processing
//----------------------------------------------------------------------------
static void sSkillsStopAllSkillsWithFlag(
	GAME * pGame,
	UNIT * pUnit,
	int nSkillFlag )
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);

	if ( ! pUnit->pSkillInfo )
		return;

	int nSkillsOnCount = pUnit->pSkillInfo->pSkillsOn.Count();
	if ( ! nSkillsOnCount )
		return;

	ASSERT_RETURN( nSkillsOnCount < MAX_SKILLS_ON );

	SKILL_IS_ON pSkillsOn[ MAX_SKILLS_ON ];
	MemoryCopy( pSkillsOn, MAX_SKILLS_ON * sizeof(SKILL_IS_ON), pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ), nSkillsOnCount * sizeof( SKILL_IS_ON ) );

	SKILL_IS_ON * pCurr = &pSkillsOn[ 0 ];
	for ( int i = 0; i < nSkillsOnCount; i++, pCurr++ )
	{
		SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, pCurr->nSkill );
		if ( SkillDataTestFlag( pSkillData, nSkillFlag ) )
			SkillStop( pGame, pUnit, pCurr->nSkill, pCurr->idWeapon );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillStopAll(
	GAME * pGame,
	UNIT * pUnit)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);

	if ( ! pUnit->pSkillInfo )
		return;

	ArrayClear(pUnit->pSkillInfo->pRequests);

	sSkillsStopAllSkillsWithFlag( pGame, pUnit, SKILL_FLAG_USE_STOP_ALL );

#if !ISVERSION(SERVER_VERSION)
	if(IS_CLIENT(pGame) && pUnit == GameGetControlUnit(pGame))
	{
		c_SendSkillStop((WORD)-1 );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillsEnteredDead(
	GAME * pGame,
	UNIT * pUnit)
{
	sSkillsStopAllSkillsWithFlag( pGame, pUnit, SKILL_FLAG_STOP_ON_DEAD );

	return TRUE;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetTicksSinceLastAggressiveSkill(
	GAME * pGame,
	UNIT * pUnit)
{
	int nLastAggressiveMove = UnitGetStat( pUnit, STATS_SKILL_LAST_AGGRESSIVE_TIME );
	if ( nLastAggressiveMove == -1 )
		return 0;
	return (int) GameGetTick( pGame ) - nLastAggressiveMove;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillStop(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	BOOL bSend,
	BOOL bForceSendToAll)
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pUnit);
	if (GameGetState(pGame) != GAMESTATE_RUNNING)
	{
		if(AppGetState() != APP_STATE_CHARACTER_CREATE)
		{
			return FALSE;
		}
	}
	SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
	ASSERT_RETFALSE( pSkillData->nStartFunc >= 0 || pSkillData->nStartFunc < sgnSkillStartTblCount );

	if (SkillDataTestFlag( pSkillData, SKILL_FLAG_SKILL_IS_ON ) )
	{
		sSkillSetOnFlag( pUnit, nSkill, INVALID_ID, idWeapon, 0, 0, FALSE );
	}
	//tugboat does not have this in the merge...but leaving in
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_AGGRESSIVE ) )
	{
		if ( ! SkillIsOnWithFlag( pUnit, SKILL_FLAG_IS_AGGRESSIVE ) )
			UnitSetStat( pUnit, STATS_SKILL_LAST_AGGRESSIVE_TIME, GameGetTick( pGame ) );
	}

	EVENT_DATA tCheckEventData( nSkill, idWeapon );
	UnitUnregisterTimedEvent( pUnit, SkillHandleSkillEvent,  CheckEventParam12,  &tCheckEventData );
	UnitUnregisterTimedEvent( pUnit, sHandleStartMode,	 CheckEventParam12,  &tCheckEventData );
	UnitUnregisterTimedEvent( pUnit, sHandleEndMode,	 CheckEventParam12,  &tCheckEventData );
	UnitUnregisterTimedEvent( pUnit, sHandleSkillRepeat, CheckEventParam12,  &tCheckEventData );
	UnitUnregisterTimedEvent( pUnit, sHandleSkillStop,	 CheckEventParam12,  &tCheckEventData );

	UNIT * pWeapon = UnitGetById( pGame, idWeapon );

	if ( IS_CLIENT( pGame ))
	{
		SkillClearEventHandler( pGame, pUnit, pWeapon, EVENT_AIMUPDATE );
	}

	SkillClearEventHandler( pGame, pUnit, pWeapon, EVENT_DIDCOLLIDE );

	STATS * pStats = StatsGetModList( pUnit, SELECTOR_SKILL );
	while ( pStats )
	{ // we don't want to remove other skills' stats lists
		DWORD dwParam = INVALID_ID;
		UNITID idStatsWeapon = StatsGetStatAny( pStats, STATS_SKILL_STATSLIST, &dwParam );
		if ( dwParam == (DWORD)nSkill && idStatsWeapon == idWeapon )
		{
			StatsListRemove( pGame, pStats );
			StatsListFree( pGame, pStats );
			break;
		}
		pStats = StatsGetModNext( pStats, SELECTOR_SKILL );
	}

	UnitTriggerEvent( pGame, EVENT_SKILL_STOPPED, pUnit, pWeapon, &EVENT_DATA( nSkill, UnitGetId( pWeapon ) ) );

	if (AppIsTugboat() && pSkillData->eUsage != USAGE_PASSIVE)
	{
		UNIT * weapons[MAX_WEAPON_LOCATIONS_PER_SKILL];
		UnitGetWeapons(pUnit, nSkill, weapons, FALSE);
		for( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			if( weapons[i] )
			{
				StateClearAttackState( weapons[i] );
			}
		}
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_REMOVE_MISSILES_ON_STOP ) )
		UnitTriggerEvent( pGame, EVENT_SKILL_DESTROYING_MISSILES, pUnit, pWeapon, &EVENT_DATA( nSkill, UnitGetId( pWeapon ) ) );

	if ( pWeapon && sWeaponUsesFiringError( pWeapon ))
	{
		sDecreaseWeaponFiringError( pGame, pUnit, pWeapon );
	}

	if ( pWeapon && UnitUsesEnergy( pWeapon ) )
	{
		sIncreaseWeaponEnergy( pGame, pUnit, pWeapon );
	}

	if ( IS_CLIENT( pGame ) )
	{
		if ( pWeapon )
		{
			int nModelId = c_UnitGetModelId( pWeapon );
			c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_SKILL, nSkill );
		}
		if ( pUnit == GameGetControlUnit( pGame ) )
		{
			c_ModelAttachmentRemoveAllByOwner( c_UnitGetModelIdFirstPerson( pUnit ), ATTACHMENT_OWNER_SKILL, nSkill );
		} 
		c_ModelAttachmentRemoveAllByOwner( c_UnitGetModelIdThirdPerson( pUnit ),		 ATTACHMENT_OWNER_SKILL, nSkill );
		c_ModelAttachmentRemoveAllByOwner( c_UnitGetModelIdThirdPersonOverride( pUnit ), ATTACHMENT_OWNER_SKILL, nSkill );

		if ( pWeapon && SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_ALL_WEAPONS ) )
		{
			UNIT * pOtherWeapon = SkillGetLeftHandWeapon( pGame, pUnit, nSkill, pWeapon );
			if ( pOtherWeapon )
			{
				int nModelId = c_UnitGetModelId( pOtherWeapon );
				c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_SKILL, nSkill );
			}
		}
	}

	// do end skill events - which have a time > 1.0f and are in the last set of events
	if ( pSkillData->nEventsId != INVALID_ID && !UnitTestFlag(pUnit, UNITFLAG_JUSTFREED) )
	{
		SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSkillData->nEventsId );
		if ( pEvents && pEvents->nEventHolderCount > 0 )
		{
			SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ pEvents->nEventHolderCount - 1 ];
			for ( int i = 0; i < pHolder->nEventCount; i++ )
			{
				SKILL_EVENT * pSkillEvent = &pHolder->pEvents[ i ];
				UNITMODE eMode = sSkillEventHolderGetMode( pSkillData, pHolder );
				float fEventTime = sSkillEventGetTime( pUnit, eMode, pSkillEvent );
				if ( fEventTime >= 1.0f )
				{
					SkillRegisterEvent( pGame, pUnit, pSkillEvent, nSkill, 0, idWeapon, fEventTime - 1.0f, 0.0f );
				}
			}
			if( pSkillData->nStartFunc != SKILL_STARTFUNCTION_NOMODES )
			{
				for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
				{ 
					pHolder = &pEvents->pEventHolders[ i ];
					UNITMODE eMode = sSkillEventHolderGetMode( pSkillData, pHolder );
					eMode = (UNITMODE) SkillChangeModeByWeapon( pGame, pUnit, nSkill, eMode, idWeapon );
					UnitEndMode( pUnit, eMode, 0, FALSE );
				}
			}
			
		}

		if ( IS_SERVER( pGame ))
		{
			s_UnitSetMode( pUnit, MODE_IDLE, TRUE );
		}
	}

	for (int ii = 0; ii < sgnSkillStats; ii++)
	{
		UnitClearStat(pUnit, sgptSkillStats[ii].wStat, nSkill);
	}
	SkillClearTargets( pUnit, nSkill, idWeapon );
	SkillClearTargets( pUnit, nSkill, INVALID_ID );

	if ( IS_SERVER( pGame ) )
	{
		if (bSend)
		{
			MSG_SCMD_SKILLSTOP tMsg;
			tMsg.id = UnitGetId( pUnit );
			tMsg.wSkillId = DOWNCAST_INT_TO_WORD(nSkill);
			tMsg.bRequest = FALSE;
			SkillSendMessage( pGame, pUnit, SCMD_SKILLSTOP, &tMsg, bForceSendToAll );
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleSkillStopRequest( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData)
{
	SkillStopRequest( pUnit, (int)tEventData.m_Data1, (BOOL)tEventData.m_Data2, FALSE );
	return TRUE;
}

//----------------------------------------------------------------------------
#define GLOBAL_MIN_HOLD 10
void SkillStopRequest(
	UNIT * pUnit,
	int nSkill,
	BOOL bSend /*= FALSE*/,
	BOOL bCheckIsRequested /*= TRUE*/,
	BOOL bForceSendToAll /*= FALSE*/ )
{
	ASSERT_RETURN(pUnit);

	if ( UnitTestFlag(pUnit, UNITFLAG_JUSTFREED) )
		return;

	GAME * pGame = UnitGetGame( pUnit );

	if ( bCheckIsRequested && ! SkillIsRequested( pUnit, nSkill ) )
		return;

	UnitUnregisterTimedEvent( pUnit, sHandleSkillStopRequest, CheckEventParam12, &EVENT_DATA( nSkill, bSend ) );

#if   !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( pGame ) && bSend && pUnit == GameGetControlUnit( pGame ) )
		c_SendSkillStop( nSkill );
#endif//   !ISVERSION(SERVER_VERSION)

	if ( IS_SERVER( pGame ) && bSend )
	{
		MSG_SCMD_SKILLSTOP tMsg;
		tMsg.id = UnitGetId( pUnit );
		tMsg.wSkillId = DOWNCAST_INT_TO_WORD(nSkill);
		tMsg.bRequest = TRUE;
		SkillSendMessage( pGame, pUnit, SCMD_SKILLSTOP, &tMsg, bForceSendToAll );
	}

	SkillSetRequested( pUnit, nSkill, 0, FALSE );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillStopAllRequests(
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);

	if ( UnitTestFlag(pUnit, UNITFLAG_JUSTFREED) )
		return;

	if ( ! pUnit->pSkillInfo )
		return;

	int nCount = pUnit->pSkillInfo->pRequests.Count();
	if ( ! nCount )
		return;

	ASSERT( nCount < MAX_SKILLS_ON );
	nCount = MIN( nCount, MAX_SKILLS_ON );

	SKILL_REQUEST pRequestedSkills[ MAX_SKILLS_ON ];
	SKILL_REQUEST * pFirst = pUnit->pSkillInfo->pRequests.GetPointer( 0 );

	MemoryCopy( pRequestedSkills, sizeof( SKILL_REQUEST ) * MAX_SKILLS_ON, pFirst, nCount * sizeof( SKILL_REQUEST ) );

	for ( int i = 0; i < nCount; i++ )
	{
		SKILL_REQUEST * pRequest = &pRequestedSkills[ i ];
		SkillStopRequest( pUnit, pRequest->nSkill, TRUE );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillsIgnoreUnitInput(
	GAME * pGame,
	UNIT * pUnit,
	BOOL & bStopMoving )
{
	ASSERT_RETTRUE( pUnit );
	ASSERT_RETTRUE( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pSkillsOn.Count();

	SKILL_IS_ON * pCurr = nCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;
	BOOL bIgnoreInput = FALSE;
	for ( int i = 0; i < nCount; i++, pCurr++ )
	{
		const SKILL_DATA * pSkillData = SkillGetData( pGame, pCurr->nSkill );

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
			bStopMoving = TRUE;

		if( SkillDataTestFlag( pSkillData, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT ) )
			bIgnoreInput = TRUE;
	}

	if ( UnitTestModeGroup( pUnit, MODEGROUP_GETHIT ) )
		return TRUE;

	return bIgnoreInput;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillRequestSelect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill )
{
#if   !ISVERSION(SERVER_VERSION)
	if ( !IS_CLIENT( pGame ) || GameGetControlUnit( pGame ) != pUnit )
		return;

	MSG_CCMD_SKILLSELECT tMsg;
	tMsg.wSkill = (WORD) nSkill;
	tMsg.dwSeed = SkillGetNextSkillSeed( pGame );
	c_SendMessage( CCMD_SKILLSELECT, &tMsg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillRequestDeselect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill )
{
#if   !ISVERSION(SERVER_VERSION)
	if ( !IS_CLIENT( pGame ) || GameGetControlUnit( pGame ) != pUnit )
		return;

	MSG_CCMD_SKILLDESELECT tMsg;
	tMsg.wSkill = (WORD) nSkill;
	c_SendMessage( CCMD_SKILLDESELECT, &tMsg);
#endif   !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
BOOL c_SkillControlUnitRequestStartSkill(
	GAME * pGame,
	int nSkill,
	UNIT * pTargetOverride,
	VECTOR vTargetPosOverride )  //tugboat added
{
	ASSERT(IS_CLIENT(pGame));

	UNIT * pUnit = GameGetControlUnit( pGame );
	if ( ! pUnit )
		return FALSE;

	if ( SkillIsOnWithFlag( pUnit, SKILL_FLAG_NO_PLAYER_SKILL_INPUT ) )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETFALSE(pSkillData);
	if ( UnitTestModeGroup( pUnit, MODEGROUP_GETHIT ) && !SkillDataTestFlag( pSkillData, SKILL_FLAG_GETHIT_CAN_DO ) )
		return TRUE;

	if( AppIsTugboat() && SkillIsRequested( pUnit, nSkill ) )
	{
		return FALSE;
	}

	if ( AppIsHellgate() && UnitHasState( pGame, pUnit, STATE_QUEST_RTS ) )
	{
		c_UI_RTS_Selection( pGame, &vTargetPosOverride, &pTargetOverride );
	}
	
	SKILL_CONTEXT context(pGame, pUnit, nSkill, 0, INVALID_ID, 0, 0);
	if ( ! sSkillCanRequest( context ) )
	{
		BOOL bStoppedInTown = FALSE;
		if( UnitIsInPVPTown(pUnit) )
		{
			if (!SkillDataTestFlag(pSkillData, SKILL_FLAG_CAN_START_IN_PVP_TOWN))
			{
				bStoppedInTown = TRUE;
			}
		}
		else if (!SkillDataTestFlag(pSkillData, SKILL_FLAG_CAN_START_IN_TOWN) && UnitIsInTown(pUnit))
		{
			bStoppedInTown = TRUE;
		}

		if ( ( !bStoppedInTown ) ||
			( SkillDataTestFlag( pSkillData, SKILL_FLAG_MUST_START_IN_PORTAL_SAFE_LEVEL ) && 
			  !UnitIsInPortalSafeLevel( pUnit) ) ||
			  ( SkillDataTestFlag(pSkillData, SKILL_FLAG_CANT_START_IN_PVP) && 
			    UnitPvPIsEnabled( pUnit ) ) )
		{
			const UNIT_DATA* unit_data = UnitGetData( pUnit );

			if (unit_data && unit_data->m_nCantCastHereSound != INVALID_ID)
			{
				c_SoundPlay(unit_data->m_nCantCastHereSound, &c_UnitGetPosition(pUnit), NULL);
			}
		}

		return FALSE;
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_REQUIRES_SKILL_LEVEL ) )
	{
		if ( SkillGetLevel( pUnit, nSkill ) <= 0 )
			return FALSE;
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_BE_SELECTED ) )
	{
		if (!SkillIsSelected(pGame, pUnit, nSkill))
		{
			// we have some toggles in Tug that we want to check starting ability on

			if( AppIsTugboat() && pSkillData->eUsage == USAGE_TOGGLE )
			{
				BOOL bCanStart = SkillCanStart( pGame, pUnit, nSkill, INVALID_ID, NULL, FALSE, TRUE, 0 );
				if( !bCanStart )
				{
					return FALSE;
				}
			}

			c_SkillRequestSelect( pGame, pUnit, nSkill );
		}
		else
		{
			c_SkillRequestDeselect( pGame, pUnit, nSkill );
		}

		if ( pSkillData->nEventsId == INVALID_ID )
			return TRUE;
	}

	if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_REQUEST ) )
		return FALSE;

	int nWeaponIndex = 0;
	UNIT * pWeapon = WardrobeGetWeapon(pGame, UnitGetWardrobe(pUnit), nWeaponIndex);
	if(!pWeapon)
	{
		nWeaponIndex++;
	}
	VECTOR vWeaponPosition, vWeaponDirection;
	UnitGetWeaponPositionAndDirection(pGame, pUnit, nWeaponIndex, &vWeaponPosition, &vWeaponDirection);
	VectorNormalize(vWeaponDirection);

	UNIT * pTargetUnit( NULL );
	VECTOR vTarget( 0 );
	if( AppIsHellgate() )
	{
 		pTargetUnit = pTargetOverride ? pTargetOverride : UIGetTargetUnit();

		SkillFindTarget( pGame, pUnit, nSkill, NULL, &pTargetUnit, vTarget );
		if(UnitHasState(pGame, pUnit, STATE_USE_TARGET_POSITION_OVERRIDE) && !VectorIsZero(vTargetPosOverride))
		{
			vTarget = vTargetPosOverride;
		}

		UnitUnregisterTimedEvent( pUnit, sHandleSkillStopRequest, CheckEventParam13, &EVENT_DATA( nSkill, TRUE ) );	
		UNITID idTarget = pTargetUnit ? UnitGetId( pTargetUnit ) : INVALID_ID;
		
		DWORD dwSkillSeed = SkillGetCRC( UnitGetPosition( pUnit ), vWeaponPosition, vWeaponDirection );
		SkillStartRequest( pGame, pUnit, nSkill, idTarget, vTarget, 0, dwSkillSeed );
	}
	else	//tugboat
	{
		vTarget = vTargetPosOverride;
		pTargetUnit = pTargetOverride ? pTargetOverride : UIGetClickedTargetUnit();		
/*#ifdef DRB_3RD_PERSON
		if ( pTargetUnit && c_CameraGetViewType() == VIEW_3RDPERSON )
		{
			PlayerFaceTarget( pGame, pTargetUnit );
			c_PlayerSendMove( pGame );
		}
#endif*/
		if ( pSkillData &&
			SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
		{
			c_PlayerStopPath( pGame );
		}
		// TRAVIS:
		// we need to force a directional update when firing skills on the server - they
		// need to point the right direction
		c_SendUnitSetFaceDirection( pUnit, UnitGetFaceDirection( pUnit, FALSE ) );
	
		UnitUnregisterTimedEvent( pUnit, sHandleSkillStopRequest, CheckEventParam13, &EVENT_DATA( nSkill, TRUE ) );
	
		UNITID idTarget = pTargetUnit ? UnitGetId( pTargetUnit ) : INVALID_ID;
		DWORD dwSkillSeed = SkillGetCRC( UnitGetPosition( pUnit ), vWeaponPosition, vWeaponDirection );
		BOOL bCanStart = SkillCanStart( pGame, pUnit, nSkill, INVALID_ID, pTargetUnit, FALSE, TRUE, 0 );
		if( !SkillStartRequest( pGame, pUnit, nSkill, idTarget, vTarget, 0, dwSkillSeed ) ||
			!bCanStart )
		{
			const UNIT_DATA* unit_data = UnitGetData( pUnit );


			// we don't play this if canstart is false, because skillcanstart takes care
			// of all the specific can't cast messages
			if (unit_data && unit_data->m_nCantCastSound != INVALID_ID && bCanStart)
			{
				c_SoundPlay(unit_data->m_nCantCastSound, &c_UnitGetPosition(pUnit), NULL);
			}
		}
	}
	return TRUE;
}

#endif//  !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetPowerCost( 
	UNIT * pUnit, 
	const SKILL_DATA * pSkillData,
	int nSkillLevel,
	BOOL bCanCapByPowerMax )
{
	ASSERTX_RETZERO(pSkillData, "Getting the skill power cost requires skill data");
	nSkillLevel = MAX(1, nSkillLevel);
	
	GAME * pGame = UnitGetGame( pUnit );
	const UNIT_DATA * pUnitData = pUnit ? UnitGetData(pUnit) : NULL;
	if ( ! pUnitData || UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IGNORES_POWER_COST_FOR_SKILLS) )
		return 0;

	
	int nCost( 0 ); 	
	if( pSkillData->codePowerCost != NULL_CODE )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codePowerCost, &code_len);
		if (code_ptr)
		{
			nCost = VMExecI(pGame, pUnit, NULL, pSkillData->nId, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len) << StatsGetShift( NULL, STATS_POWER_CUR );
			//We can have other skills modify the cost of the skill to do by a percent.
			if( pSkillData->codePowerCostMultPCT != NULL_CODE )
			{
				code_len = 0;
				code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codePowerCostMultPCT, &code_len);
				if (code_ptr)
				{
					int nPCTIncreaes = VMExecI(pGame, pUnit, NULL, pSkillData->nId, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len);
					nCost += PCT( nCost, nPCTIncreaes );
				}
			}
			//Because the Power Cost of a skill has to be an Int and some skills repeat per second we going to make the cost be
			//per second based off of skill.
			if( pSkillData->nWarmupTicks > 0 &&
				SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_FIRE ) )
			{
				int percent = (int)( 100.0f * ((float)pSkillData->nWarmupTicks/GAME_TICKS_PER_SECOND_FLOAT ) );
				nCost = PCT( nCost, MAX( percent, 1 )  );
			}
		}
	}
	else
	{
		nCost = pSkillData->nPowerCost + pSkillData->nPowerCostPerLevel * (nSkillLevel - 1);
	}

	int nPercentChange = 0;
	for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnSkillGroup[ i ] == INVALID_ID )
			continue;

		nPercentChange += UnitGetStat( pUnit, STATS_POWER_COST_PCT_SKILLGROUP, pSkillData->pnSkillGroup[ i ] );
	}

	//this stat will be zero if not set
	nPercentChange += UnitGetStat( pUnit, STATS_POWER_COST_PCT_SKILL, pSkillData->nId );


	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_ADJUST_POWER_BY_LEVEL ) )
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		int nUnitType = pUnitData->nUnitTypeForLeveling;
		const PLAYERLEVEL_DATA * pPlayerLevelData = PlayerLevelGetData( pGame, nUnitType, UnitGetStat( pUnit, STATS_LEVEL ) );
		if ( pPlayerLevelData )
			nPercentChange -= pPlayerLevelData->nPowerCostPercent - 100;
	}
	if ( nPercentChange )
	{
		nCost -= PCT( nCost, nPercentChange );
		nCost = MAX( nCost, 0 );
	}

	if ( bCanCapByPowerMax && SkillDataTestFlag( pSkillData, SKILL_FLAG_POWER_COST_BOUNDED_BY_MAX_POWER ) )
	{
		int nPowerMax = UnitGetStat( pUnit, STATS_POWER_MAX );
		if ( nCost > nPowerMax )
			nCost = nPowerMax;
	}

	return nCost;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetSelectCost( 
	UNIT * pUnit, 
	int nSkill,
	const SKILL_DATA * pSkillData,
	int nSkillLevel )
{
	ASSERT_RETZERO( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeSelectCost, &code_len);
	if (!code_ptr)
	{
		return 0;
	}
	return VMExecI(pGame, pUnit, NULL, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSkillDeselect(
	GAME * game,
	UNIT * unit,
	int skill,
	const SKILL_DATA * skill_data)
{
	ASSERT_RETURN(IS_SERVER(game));
	ASSERT_RETURN(unit);
	ASSERT_RETURN((skill_data = (!skill_data ? SkillGetData(game, skill) : skill_data)) != NULL);

	if (skill_data->nStateOnSelect != INVALID_ID)
	{
		s_StateClear(unit, UnitGetId(unit), skill_data->nStateOnSelect, 0, skill);
	}
	if ( IS_SERVER( game ) && !UnitTestFlag(unit, UNITFLAG_FREED) )
	{
		if (skill_data->nClearStateOnDeselect != INVALID_ID)
		{
			StateClearAllOfType( unit, skill_data->nClearStateOnDeselect );
		}
	}
	for( int t = 0; t < MAX_SKILL_EVENT_TRIGGERS; t++ )
	{
		if (skill_data->nUnitEventTrigger[t] != INVALID_ID)
		{
			DWORD dwEventId = UnitGetStat(unit, STATS_SKILL_ACTIVATE_TRIGGER_EVENTID, skill, t);
			if ( dwEventId != INVALID_ID )
			{
				UnitUnregisterEventHandler(game, unit, UNIT_EVENT(skill_data->nUnitEventTrigger[t]), dwEventId);				
				UnitSetStat( unit, STATS_SKILL_ACTIVATE_TRIGGER_EVENTID, skill, t, INVALID_ID );
			}
		}
		//UnitClearStat(unit, STATS_SKILL_ACTIVATE_TRIGGER_EVENTID, skill);
	}
	UnitSetStat(unit, STATS_SKILL_IS_SELECTED, skill, 0);
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStatsOnDeselectServerOnly, &code_len);		
	if (code_ptr)
	{
		int nSkillLevel = SkillGetLevel( unit, skill );
		VMExecI(game, unit, skill, nSkillLevel, skill, nSkillLevel, INVALID_ID, code_ptr, code_len);
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillDeselect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bForce )
{
	ASSERT_RETURN(pUnit);

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_BE_SELECTED ) )
		return;

	//tugboat didn't have this, but it seems ok to keep in.
	if ( ! bForce && SkillDataTestFlag( pSkillData, SKILL_FLAG_ALWAYS_SELECT ) )
		return;

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_START_ON_SELECT ) )
	{
		SkillStop( pGame, pUnit, nSkill, INVALID_ID );
	}

	if ( IS_SERVER( pGame ) )
	{
		s_sSkillDeselect(pGame, pUnit, nSkill, pSkillData);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sSkillEventTriggeredDoSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	const EVENT_DATA & event_data)
{
	int nSkill = int(event_data.m_Data1);
	UNITID idOtherUnit = UNITID(event_data.m_Data2);

	BOOL bSuccess = SkillStartRequest( pGame, pUnit, nSkill, idOtherUnit, VECTOR(0), SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT, SkillGetNextSkillSeed(pGame));

	// print execute
	extern BOOL gbProcTrace;
	if (bSuccess && gbProcTrace)
	{
		ConsoleString( 
			CONSOLE_SYSTEM_COLOR, 
			L"(%d) '%S' executed skill proc '%S'", 
			GameGetTick( pGame ),
			UnitGetDevName( pUnit ),
			SkillGetDevName( pGame, nSkill));
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSkillSelectEventTriggered(
	GAME* pGame, 
	UNIT* pUnit, 
	UNIT* pOtherUnit, 
	EVENT_DATA* pEventData,
	void* pData,
	DWORD dwId )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nSkill = int(pEventData->m_Data1);
	UNIT_EVENT eEventType = UNIT_EVENT(pEventData->m_Data2);
	if(nSkill < 0)
	{
		goto unregisterandquit;
	}

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if(!pSkillData)
	{
		goto unregisterandquit;
	}

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeUnitEventTriggerChance, &code_len);
	if (!code_ptr)
	{
		goto unregisterandquit;
	}

	if(!SkillCanStart(pGame, pUnit, nSkill, INVALID_ID, pOtherUnit, TRUE, FALSE, 0))
	{
		return;
	}

	int nSkillLevel = SkillGetLevel(pUnit, nSkill);
	float fChance = VMExecI(pGame, pUnit, NULL, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len) / 100.0f;

	int nDamageIncrement = DAMAGE_INCREMENT_FULL;
	struct COMBAT * pCombat = GamePeekCombat(pGame);
	if(pCombat)
	{
		nDamageIncrement = s_CombatGetDamageIncrement(pCombat);
	}

	float fModifiedChance = fChance;
	if(nDamageIncrement > 0 && nDamageIncrement != DAMAGE_INCREMENT_FULL)
	{
		fModifiedChance = CombatModifyChanceByIncrement(fChance, PctToFloat(nDamageIncrement));
	}

#if ISVERSION(CHEATS_ENABLED)
	if (ProcGetProbabilityOverride() != 0)
	{
		fModifiedChance = float(ProcGetProbabilityOverride()) / 100.0f;
	}
#endif

	float fRoll = RandGetFloat(pGame);

	// print attempt
	extern BOOL gbProcTrace;
	if (gbProcTrace)
	{
		ConsoleString( 
			CONSOLE_SYSTEM_COLOR, 
			L"(%d) '%S' attempt skill proc '%S', chance = %d%%, increment = %d, modified chance = %d%%, roll = %d%%", 
			GameGetTick( pGame ),
			UnitGetDevName( pUnit ),
			SkillGetDevName( pGame, nSkill ),
			int(fChance*100.0f),
			nDamageIncrement,
			int(fModifiedChance*100.0f),
			int(fRoll*100.0f));
	}

	if(fModifiedChance > 0.0f && fRoll <= fModifiedChance)
	{
		UnitRegisterEventTimed(pUnit, s_sSkillEventTriggeredDoSkill, EVENT_DATA(nSkill, UnitGetId( pOtherUnit )), 1);
	}
	
	return;

unregisterandquit:
	UnitUnregisterEventHandler(pGame, pUnit, eEventType, dwId);
	return;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillCanBeSelected(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bCheckStats )
{
	// do can select code
	ASSERT_RETFALSE(nSkill != INVALID_ID);
	const SKILL_DATA * skill_data = SkillGetData(pGame, nSkill);
	ASSERT_RETFALSE(skill_data);

	if ( ! SkillDataTestFlag( skill_data, SKILL_FLAG_CAN_BE_SELECTED ) )
		return FALSE;

	if(IS_SERVER(pGame))
	{
		int code_len = 0;
		BYTE * code_ptr = NULL;
		code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, skill_data->codeSelectCondition, &code_len);
		if(code_ptr && code_len)
		{
			int nSkillLevel = SkillGetLevel(pUnit, nSkill);
			if(!VMExecI(pGame, pUnit, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len))
			{
				return FALSE;
			}
		}
	}

	if (skill_data->nSelectCheckStat == INVALID_ID)
	{
		return TRUE;
	}

	if ( bCheckStats )
	{
		int nSkillLevel = SkillGetLevel(pUnit, nSkill);

		int need = 0;
		{
			if ( skill_data->nSelectCheckStat == INVALID_ID )
				return TRUE;

			need = UnitGetStatShift( pUnit, skill_data->nSelectCheckStat );
		}

		int cost = SkillGetSelectCost( pUnit, nSkill, skill_data, nSkillLevel );

		if (need < cost)
		{
			return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillSelect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bForce,
	BOOL bUseSeed,
	DWORD dwSkillSeed)
{
	ASSERT_RETURN(pUnit);

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	ASSERT_RETURN( pSkillData );

	if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_BE_SELECTED ) )
		return;

	UNITLOG_ASSERTX_RETURN( UnitIsA( pUnit, pSkillData->nRequiredUnittype ), pSkillData->szName, pUnit );

	if ( ! sSkillCanBeSelected( pGame, pUnit, nSkill, TRUE ))
	{
		if (IS_CLIENT(pGame))
		{
			if( AppIsHellgate() )
			{
				// Play a "denied!" sound
				int nSound = GlobalIndexGet( GI_SOUND_SKILL_CANNOT_SELECT );
				if (nSound != -1)
				{
					CLT_VERSION_ONLY( c_SoundPlay( nSound, &c_UnitGetPosition( pUnit ), NULL ) );
				}
			}
			else
			{
				const UNIT_DATA* unit_data = UnitGetData( pUnit );
				if (!unit_data)
				{
					return;
				}

				if (unit_data->m_nCantSound != INVALID_ID)
				{
					c_SoundPlay(unit_data->m_nCantSound, &c_UnitGetPosition(pUnit), NULL);
				}
			}
			
		}
		UnitSetStat( pUnit, STATS_SKILL_IS_SELECTED, nSkill, FALSE);
		//UNITLOG_ASSERT(FALSE, pUnit);//This might occur more often than I think - rdonald.
		// Actually, it can happen quite often.  If a player doesn't have enough power and the skill requires power to select...
		return;
	}

	// Make sure the user doesn't already have a skill of this type selected
	if ( ! bForce )
	{
		if (UnitGetStat( pUnit, STATS_SKILL_IS_SELECTED, nSkill ))
		{
			return;
		}
	}

	if ( IS_SERVER( pGame ) )
	{
		if ( pSkillData->nClearStateOnSelect != INVALID_ID &&
			( pSkillData->nPreventClearStateOnSelect == INVALID_ID || !UnitHasState( pGame, pUnit, pSkillData->nPreventClearStateOnSelect ) ) )
		{
			StateClearAllOfType( pUnit, pSkillData->nClearStateOnSelect );
		}
	}

	if ( IS_SERVER( pGame ) )
	{
		STATS * pStats = NULL;
		if (pSkillData->nStateOnSelect != INVALID_ID)
		{
			pStats = s_StateSet( pUnit, pUnit, pSkillData->nStateOnSelect, 0, nSkill );
		}

		for( int t = 0; t < MAX_SKILL_EVENT_TRIGGERS; t++ )
		{
			if(pSkillData->nUnitEventTrigger[t] != INVALID_ID)
			{
				DWORD dwEventId = UnitGetStat(pUnit, STATS_SKILL_ACTIVATE_TRIGGER_EVENTID, nSkill, t);
				if(dwEventId == INVALID_ID)
				{
					UnitRegisterEventHandlerRet(dwEventId, pGame, pUnit, UNIT_EVENT(pSkillData->nUnitEventTrigger[t]), s_sSkillSelectEventTriggered, EVENT_DATA(nSkill, pSkillData->nUnitEventTrigger[t]));
					UnitSetStat( pUnit, STATS_SKILL_ACTIVATE_TRIGGER_EVENTID, nSkill, t, dwEventId );
				}
			}
		}

		UnitSetStat( pUnit, STATS_SKILL_IS_SELECTED, nSkill, TRUE);
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_START_ON_SELECT ) )
	{
		if(!bUseSeed)
		{
			dwSkillSeed = SkillGetNextSkillSeed( pGame );
		}
		sSkillStart( pGame, pUnit, nSkill, nSkill, INVALID_ID, 0, dwSkillSeed );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SkillsSelectAll(
	GAME * pGame,
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);

	if ( IS_CLIENT( pGame ) )
		return;  // the client waits until the server tells it that skills are selected.

	// clear out selected stat for skills that have been removed
	{ 
		UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_IS_SELECTED, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
		{
			int nSkill = StatGetParam( STATS_SKILL_IS_SELECTED, pStatsEntry[ jj ].GetParam(), 0 );
			if (nSkill == INVALID_ID)
			{	
				// clear out the stat just in case
				s_sSkillDeselect(pGame, pUnit, nSkill, NULL);
				continue;
			}
		}
		UNIT_ITERATE_STATS_END;
	}

	{
		UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
		{
			int nSkill = StatGetParam( STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0 );
			if (nSkill == INVALID_ID)
			{
				continue;
			}

			const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
			if (!pSkillData)
			{
				continue;
			}
			if (!UnitIsA(pUnit, pSkillData->nRequiredUnittype))
			{
				continue;
			}
			if ( UnitGetStat( pUnit, STATS_SKILL_IS_SELECTED, nSkill ) || SkillDataTestFlag( pSkillData, SKILL_FLAG_ALWAYS_SELECT )	)
			{
				s_sSkillDeselect(pGame, pUnit, nSkill, pSkillData);
				SkillSelect( pGame, pUnit, nSkill, TRUE );
			}
			else
			{
				SkillDeselect( pGame, pUnit, nSkill, TRUE );				
			}
		}
		UNIT_ITERATE_STATS_END;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsSelected(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill )
{
	return UnitGetStat( pUnit, STATS_SKILL_IS_SELECTED, nSkill );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanMeleeAttackUnit(
	UNIT* attacker,
	UNIT* defender,
	UNIT* weapon,
	int attack_index,
	float fRange,
	BOOL bForceIsInRange,
	const SKILL_EVENT * pSkillEvent,
	BOOL bAllowDead,
	float fFiringCone,
	VECTOR * pvProposedAttackerLocation)
{
	ASSERT_RETFALSE(attacker && defender);
	ROOM* attacker_room = UnitGetRoom(attacker);
	ROOM* defender_room = UnitGetRoom(defender);
	if (!attacker_room || !defender_room || RoomGetLevel(attacker_room) != RoomGetLevel(defender_room))
	{
		return FALSE;
	}

	// check to see if they overlap in the Z
	if( AppIsHellgate() )// tugboat doesn't care anymore - no height to speak of
	{
		float fBonusAttackerHeight = 0.0f;
		if ( UnitDataTestFlag( UnitGetData( attacker ), UNIT_DATA_FLAG_CAN_MELEE_ABOVE_HEIGHT ) )
			fBonusAttackerHeight = 3.0f;
		if (attacker->vPosition.fZ + UnitGetCollisionHeight( attacker ) + fRange + fBonusAttackerHeight < defender->vPosition.fZ ||
			defender->vPosition.fZ + UnitGetCollisionHeight( defender ) + fRange						< attacker->vPosition.fZ)
		{
			return FALSE;
		}
	}

	if (!bAllowDead && IsUnitDeadOrDying(defender))
	{
		return FALSE;
	}

	if ( ! bForceIsInRange )
	{
		VECTOR vAttackPosition;
		VECTOR vAttackDirection;
		if(pvProposedAttackerLocation)
		{
			vAttackPosition = *pvProposedAttackerLocation;
			vAttackDirection = UnitGetPosition(defender) - vAttackPosition;
			VectorNormalize(vAttackDirection);
		}
		else if ( pSkillEvent && (pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET) != 0 )
		{
			MATRIX mWorld;
			UnitGetWorldMatrix( attacker, mWorld );
			MatrixMultiply		( &vAttackPosition,  &pSkillEvent->tAttachmentDef.vPosition, &mWorld );
			MatrixMultiplyNormal( &vAttackDirection, &pSkillEvent->tAttachmentDef.vNormal, &mWorld );
			VectorNormalize( vAttackDirection );
		} else {
			vAttackPosition = UnitGetPosition( attacker );
			vAttackDirection= UnitGetFaceDirection( attacker, FALSE );
		}

		VECTOR vDelta = UnitGetPosition( defender ) - vAttackPosition;
		vDelta.fZ = 0.0f;
		float fDistance = AppIsHellgate() ? VectorLength( vDelta ) : VectorLengthXY( vDelta );
		float fDistanceSquared = fDistance * fDistance;

		//float fMinRange = UnitGetCollisionRadius( attacker ) + UnitGetCollisionRadius( defender ) + fRange;
		// TRAVIS: UnitsAreInRange already factors in the collision radii of the 2 units - why do it twice?
		if ( ! UnitsAreInRange( attacker, defender, 0.0f, fRange, &fDistanceSquared ) )
			return FALSE;
	
		if( AppIsHellgate() )
		{
			const UNIT_DATA * pAttackerData = UnitGetData( attacker );
			if ( UnitDataTestFlag(pAttackerData, UNIT_DATA_FLAG_MUST_FACE_MELEE_TARGET) && IS_CLIENT( attacker ) )
			{
				VECTOR vDirection;
				if ( fDistance )
					VectorScale( vDirection, vDelta, 1.0f / fDistance );
				else
					vDirection = VECTOR(0,1,0);
				float fDot = VectorDot( vDirection, vAttackDirection );
				if ( pSkillEvent )
				{
					float fParam = SkillEventGetParamFloat( NULL, INVALID_ID, 0, pSkillEvent, 0 );
					if ( fDot < fParam )
						return FALSE;
				}
				else if ( fDot < 0.0f )
					return FALSE;
			}
		}else{ //must be tugboat
			// TRAVIS: might have to remove - server doesn't necessarily look like client
			vAttackPosition -= UnitGetFaceDirection( attacker, FALSE ) * .5f;
			vDelta = UnitGetPosition( defender ) - vAttackPosition;
			if( fFiringCone > 0 )
			{
				vDelta.z=0; //don't check z
				VectorNormalize( vDelta, vDelta );
				float fDot = acos( PIN( VectorDot( vAttackDirection, vDelta ), -1.0f, 1.0f ) );
				if ( fDot > fFiringCone )
				{
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInFiringCone(
	UNIT* attacker,
	UNIT* defender,
	const SKILL_EVENT * pSkillEvent,
	float fFiringCone,
	VECTOR * pvProposedAttackerLocation)
{
	ASSERT_RETFALSE(attacker && defender);
	ROOM* attacker_room = UnitGetRoom(attacker);
	ROOM* defender_room = UnitGetRoom(defender);
	if (!attacker_room || !defender_room || RoomGetLevel(attacker_room) != RoomGetLevel(defender_room))
	{
		return FALSE;
	}

	VECTOR vAttackPosition;
	VECTOR vAttackDirection;
	if(pvProposedAttackerLocation)
	{
		vAttackPosition = *pvProposedAttackerLocation;
		vAttackDirection = UnitGetPosition(defender) - vAttackPosition;
		VectorNormalize(vAttackDirection);
	}
	else if ( pSkillEvent && (pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET) != 0 )
	{
		MATRIX mWorld;
		UnitGetWorldMatrix( attacker, mWorld );
		MatrixMultiply		( &vAttackPosition,  &pSkillEvent->tAttachmentDef.vPosition, &mWorld );
		MatrixMultiplyNormal( &vAttackDirection, &pSkillEvent->tAttachmentDef.vNormal, &mWorld );
		VectorNormalize( vAttackDirection );
	} else {
		vAttackPosition = UnitGetPosition( attacker );
		vAttackDirection= UnitGetFaceDirection( attacker, FALSE );
	}
	

	vAttackPosition -= UnitGetFaceDirection( attacker, FALSE ) * .5f;
	VECTOR vDelta = UnitGetPosition( defender ) - vAttackPosition;
	if( fFiringCone > 0 )
	{
		vDelta.z=0; //don't check z
		VectorNormalize( vDelta, vDelta );
		float fDot = acos( PIN( VectorDot( vAttackDirection, vDelta ), -1.0f, 1.0f ) );
		if ( fDot > fFiringCone )
		{
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillGetMeleeBone( 
	UNIT * pUnit,
	int nSkill, 
	UNIT * pWeapon )
{
	int nIndex = WardrobeGetWeaponIndex( pUnit, UnitGetId( pWeapon ) );
	if ( nIndex == INVALID_ID )
		return INVALID_ID;

	int nAppearanceDef = UnitGetAppearanceDefId( pUnit, TRUE );
	if ( nAppearanceDef == INVALID_ID )
		return INVALID_ID;

	return c_WeaponGetAttachmentBoneIndex( nAppearanceDef, nIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sSkillAddMeleeImpact( 
	UNIT * pAttacker,
	UNIT * pDefender,
	int nSkill, 
	UNIT * pWeapon,
	float fForce )
{
	if ( ! AppIsHellgate() )
		return FALSE;

	if ( ! pAttacker )
		return FALSE;

#ifdef HAVOK_ENABLED

	GAME * pGame = UnitGetGame( pAttacker );
	// add impacts on the target looking at where the bones are and were
	const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
	HAVOK_IMPACT tImpact;
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IMPACT_USES_AIM_BONE ) )
	{
		VECTOR vAimPosition = UnitGetAimPoint( pAttacker );
		HavokImpactInit( pGame, tImpact, fForce, vAimPosition, NULL );
	} 
	else 
	{
		MATRIX mBonePrev;
		MATRIX mBoneCurr;
		{
			int nModelId = c_UnitGetModelIdThirdPerson( pAttacker );
			if ( nModelId == INVALID_ID )
				return FALSE;

			int nBone = sSkillGetMeleeBone( pAttacker, nSkill, pWeapon );
			if ( nBone == INVALID_ID )
				return FALSE;

			if ( ! c_AppearanceGetBoneMatrix( nModelId, &mBonePrev, nBone, TRUE ) )
				return FALSE;

			c_AppearanceGetBoneMatrix( nModelId, &mBoneCurr, nBone );

			MATRIX mWorld;
			UnitGetWorldMatrix( pAttacker, mWorld );

			MatrixMultiply( &mBonePrev, &mBonePrev, &mWorld );
			MatrixMultiply( &mBoneCurr, &mBoneCurr, &mWorld );
		}

		VECTOR vWeaponOffset(0);
		if ( pWeapon )
		{// simulates a 1 meter long sword
			vWeaponOffset.z = 1.0f; 
		}
		VECTOR vPositionPrev;
		MatrixMultiply( &vPositionPrev, &vWeaponOffset, &mBonePrev );
		VECTOR vPositionCurr;
		MatrixMultiply( &vPositionCurr, &vWeaponOffset, &mBoneCurr );

		VECTOR vPushDirection = vPositionCurr - vPositionPrev;
		vPushDirection.fZ /= 16.0f; // we don't care much about the z factor of the swing
		float fDistance = VectorNormalize( vPushDirection );

		VECTOR vDefenderFromAttacker = UnitGetPosition( pDefender ) - UnitGetPosition( pAttacker );
		VectorNormalize( vDefenderFromAttacker );

		if ( fDistance == 0.0f )
			vPushDirection = vDefenderFromAttacker;

		if ( VectorDot( vDefenderFromAttacker, vPushDirection ) < -0.3f )
		{
			VECTOR vSide;
			VectorCross( vSide, vDefenderFromAttacker, vPushDirection );
			VectorNormalize( vSide );
			VectorCross( vPushDirection, vSide, vDefenderFromAttacker );
			VectorNormalize( vPushDirection );
		}

		//if ( pSkillData->fImpactForwardBias != 0.0f )
		//{ // some skills should always push along with the attacking unit.
		//	VECTOR vAttackerFaceDirection = UnitGetFaceDirection( pAttacker, FALSE );
		//	vPushDirection = VectorLerp( vAttackerFaceDirection, vPushDirection, pSkillData->fImpactForwardBias );
		//	VectorNormalize( vPushDirection );
		//}

		HavokImpactInit( pGame, tImpact, fForce, vPositionPrev, &vPushDirection );
	}
	UnitAddImpact( pGame, pDefender, &tImpact );
#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL UnitMeleeAttackUnit(
	UNIT* attacker,
	UNIT* defender,
	UNIT* weapon,
	UNIT* weapon_other,
	int nSkill,
	int attack_index,
	float fRange,
	BOOL bForceIsInRange,
	const SKILL_EVENT * pSkillEvent,	
	float fDamageMultiplier,
	int nDamageTypeOverride,
	float fImpactForce,
	DWORD dwUnitAttackUnitFlags)
{
	if ( ! defender )
		return FALSE;

	if ( IS_CLIENT( defender ) )
	{
#if   !ISVERSION(SERVER_VERSION)
		if ( AppIsTugboat() ) //Tugboat doesn't do AddMeleeImpact
		{
			// ok, we're just going to make destructibles explode on the client -
			// way more satisfying. The real action will take place on the server
			// and do the drops if necessary
			if( attacker && attacker == GameGetControlUnit( UnitGetGame( attacker ) )  )
			{
				float fFiringCone = 0;
				GAME * pGame = UnitGetGame( attacker );
				const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
				if( attacker )
				{
					fFiringCone = pSkillData->fFiringCone;
				}

				if (!UnitCanMeleeAttackUnit(attacker, defender, weapon, 0, fRange, bForceIsInRange, pSkillEvent, FALSE, fFiringCone ))
				{
					if( AppIsTugboat() && attacker )
					{
						SkillSetTarget( attacker, nSkill, weapon ? UnitGetId( weapon ) : INVALID_ID, INVALID_ID );
					}
					return FALSE;
				}
				const DAMAGE_TYPE_DATA * damage_type_data = DamageTypeGetData(pGame, DAMAGE_TYPE_SLASHING );

				c_UnitHitUnit( pGame, weapon ? UnitGetId( weapon ) : UnitGetId( attacker ), UnitGetId( defender ), TRUE, damage_type_data->nSoftHitState, TRUE );
				
				if( UnitIsA( defender, UNITTYPE_DESTRUCTIBLE ) )
				{
					BOOL bHasMode;
					float fDyingDurationInTicks = UnitGetModeDuration(pGame, defender, MODE_DYING, bHasMode);
					if( bHasMode )
					{
						c_UnitSetMode(defender, MODE_DYING, 0, fDyingDurationInTicks);
						UnitSetFlag(defender, UNITFLAG_DEAD_OR_DYING);
						UnitChangeTargetType(defender, TARGET_DEAD);
					}
				}

			}
			return TRUE;
		}
		return c_sSkillAddMeleeImpact( attacker, defender, nSkill, weapon, fImpactForce );
#endif//  !ISVERSION(SERVER_VERSION)
	}
#if !ISVERSION(CLIENT_ONLY_VERSION)
	else
	{
		float fFiringCone = 0;
		GAME * pGame = UnitGetGame( attacker );
		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
		if( AppIsTugboat() && attacker )
		{
			fFiringCone = pSkillData->fFiringCone;
		}
		if (!UnitCanMeleeAttackUnit(attacker, defender, weapon, 0, fRange, bForceIsInRange, pSkillEvent, FALSE, fFiringCone ))
		{
			if( AppIsTugboat() && attacker )
			{
				SkillSetTarget( attacker, nSkill, weapon ? UnitGetId( weapon ) : INVALID_ID, INVALID_ID );
			}
			return FALSE;
		}

		UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
		ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
		pWeapons[ 0 ] = weapon;
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_COMBINE_WEAPON_DAMAGE ) )
		{
			pWeapons[ 1 ] = SkillGetLeftHandWeapon( pGame, attacker, nSkill, weapon );
		} else {
			pWeapons[ 1 ] = NULL;
		}
		DWORD dwFlags = dwUnitAttackUnitFlags;
		SETBIT(dwFlags, UAU_MELEE_BIT);
		// For now, we're leaving dual-wield damage alone in Mythos
		if(IsUnitDualWielding(pGame, attacker, UNITTYPE_MELEE) && AppIsHellgate())
		{
			SETBIT(dwFlags, UAU_APPLY_DUALWEAPON_MELEE_SCALING_BIT);
		}
		//Marsh needs to add Damage Multiplier and DamageOverride
		s_UnitAttackUnit( attacker, defender, pWeapons, 0,
#ifdef HAVOK_ENABLED	// CHB 2007.02.01
			NULL,
#endif
			dwFlags, fDamageMultiplier, nDamageTypeOverride);

		SKILL_IS_ON * pSkillIsOn = SkillIsOn(attacker, nSkill, UnitGetId(weapon), FALSE);
		if(!pSkillIsOn && weapon_other)
		{
			pSkillIsOn = SkillIsOn(attacker, nSkill, UnitGetId(weapon_other), FALSE);
		}
		if(pSkillIsOn &&
			pSkillIsOn->nRequestedSkill >= 0 &&
			pSkillIsOn->nRequestedSkill != pSkillIsOn->nSkill)
		{
			const SKILL_DATA * pRequestedSkillData = SkillGetData(pGame, pSkillIsOn->nRequestedSkill);
			if(SkillDataTestFlag(pSkillData, SKILL_FLAG_EXECUTE_REQUESTED_SKILL_ON_MELEE_ATTACK) && 
				SkillDataTestFlag(pRequestedSkillData, SKILL_FLAG_CAN_BE_EXECUTED_FROM_MELEE_ATTACK))
			{
				if(SkillIsValidTarget(pGame, attacker, defender, weapon, pSkillIsOn->nRequestedSkill, TRUE, NULL, TRUE) &&
					SkillCanStart( pGame, attacker, pSkillIsOn->nRequestedSkill, UnitGetId(weapon), defender, FALSE, FALSE, 0 ) )
				{
					SKILL_CONTEXT skill_context(pGame, attacker, pSkillIsOn->nRequestedSkill, 0, UnitGetId(weapon), SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT, 0);
					sSkillExecute(skill_context);
				}
			}
		}
	}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
// Maybe the skill seed should use an RNG to be less predictable? - GS
//----------------------------------------------------------------------------
DWORD SkillGetNextSkillSeed(
	GAME * pGame )
{
	return pGame->m_dwSkillSeed++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsValidTarget( 
	GAME * game, 
	UNIT * unit, 
	UNIT * target,
	UNIT * weapon,
	int skill,
	BOOL bStartingSkill,
	BOOL * pbIsInRange,
	BOOL bIgnoreWeaponTargeting,
	BOOL bDontCheckAttackerUnittype )
{
	SKILL_CONTEXT context( game, unit, skill, 0, NULL, weapon, 0, 0);
	return sSkillIsValidTarget(context, target, bStartingSkill, pbIsInRange, bIgnoreWeaponTargeting, bDontCheckAttackerUnittype);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct SkillTargetQuerySortContext
{
	GAME * pGame;
	VECTOR * pCenter;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillTargetQuerySortCompareByUnitId(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{
	UNITID idLeft = *(UNITID*)pLeft;
	UNITID idRight = *(UNITID*)pRight;
	if(idLeft < idRight)
	{
		return -1;
	}
	else if(idLeft > idRight)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillTargetQuerySortCompareByDistance(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{
	SkillTargetQuerySortContext * pQueryContext = (SkillTargetQuerySortContext *)pContext;
	UNITID idLeft = *(UNITID*)pLeft;
	UNITID idRight = *(UNITID*)pRight;
	UNIT * pUnitLeft = UnitGetById(pQueryContext->pGame, idLeft);
	UNIT * pUnitRight = UnitGetById(pQueryContext->pGame, idRight);
	float fDistanceLeft = VectorDistanceSquared(UnitGetPosition(pUnitLeft), *pQueryContext->pCenter);
	float fDistanceRight = VectorDistanceSquared(UnitGetPosition(pUnitRight), *pQueryContext->pCenter);

	if(fDistanceLeft < fDistanceRight)
	{
		return -1;
	}
	else if(fDistanceLeft > fDistanceRight)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillTargetQuery( 
	GAME * pGame, 
	SKILL_TARGETING_QUERY & tTargetingQuery )
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(tTargetingQuery.pSeekerUnit);
	if (!tTargetingQuery.pSeekerUnit->pRoom && !tTargetingQuery.pCenterRoom)
	{
		return;
	}

	tTargetingQuery.nUnitIdCount = 0;
	if ( ! tTargetingQuery.pSeekerUnit )
		return;
	if ( tTargetingQuery.nUnitIdMax <= 0 )
		return;
	if ( ! tTargetingQuery.pnUnitIds )
		return;
	ASSERT_RETURN( tTargetingQuery.nUnitIdMax <= MAX_TARGETS_PER_QUERY ||
					! TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT ) );

	//ASSERT_RETURN( tTargetingQuery.tFilter.dwFlags ); // we use the flags to say what type of query to run.  You must set one.

	// We must do these before checking the max range, because they are range-independent
	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_CONTAINER ) )
	{ // since this can just target the unit's container, go ahead and grab it
		UNIT * pTarget = UnitGetContainer( tTargetingQuery.pSeekerUnit );
		if ( pTarget )
		{
			tTargetingQuery.pnUnitIds[ 0 ] = UnitGetId( pTarget );
			tTargetingQuery.nUnitIdCount++;
		}
		return;
	}

	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_PETS ) )
	{
		for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
		{
			if( tTargetingQuery.pnSummonedInventoryLocations[ i ] == INVALID_ID )
				continue;
			int nInventoryCount = UnitInventoryGetCount(tTargetingQuery.pSeekerUnit, tTargetingQuery.pnSummonedInventoryLocations[ i ] );
			nInventoryCount = MIN(nInventoryCount, tTargetingQuery.nUnitIdMax);
			UNIT * pTarget = UnitInventoryLocationIterate(tTargetingQuery.pSeekerUnit, tTargetingQuery.pnSummonedInventoryLocations[ i ], NULL);
			while(nInventoryCount && pTarget && tTargetingQuery.nUnitIdCount < tTargetingQuery.nUnitIdMax )
			{
				tTargetingQuery.pnUnitIds[ tTargetingQuery.nUnitIdCount ] = UnitGetId( pTarget );
				tTargetingQuery.nUnitIdCount++;

				pTarget = UnitInventoryLocationIterate(tTargetingQuery.pSeekerUnit, tTargetingQuery.pnSummonedInventoryLocations[ i ], pTarget);
			}
		}
		return;
	}

	if ( tTargetingQuery.fMaxRange == 0.0f )
		return;

	VECTOR vTargetingLocation;
	VECTOR vTargetingLocationEnd( 0 );  // used for Distance to Line check
	VECTOR vLOSPosition( 0 );
	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION ) &&
		tTargetingQuery.pvLocation )
	{
		vTargetingLocation = *tTargetingQuery.pvLocation;
		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS ) ||
			 TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_SIMPLE_PATH_EXISTS ))
		{
			vLOSPosition = *tTargetingQuery.pvLocation;
		}
		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE ) )
		{
			vTargetingLocationEnd = UnitGetPosition( tTargetingQuery.pSeekerUnit );
			vLOSPosition = UnitGetAimPoint( tTargetingQuery.pSeekerUnit );
		}
	} else {
		ASSERT ( ! TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE) );
		vTargetingLocation = UnitGetPosition( tTargetingQuery.pSeekerUnit );
		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS ) )
		{
			// Yeah, I could pick a better place to aim from, but this isn't too bad
			vLOSPosition = UnitGetAimPoint( tTargetingQuery.pSeekerUnit );
		}
	}

	VECTOR vLineEdge( 0 );  // used for Distance to Line check
	float fLineLengthSquared = 0.0f;
	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE ) )
	{
		vLineEdge = vTargetingLocationEnd - vTargetingLocation;
		fLineLengthSquared = AppIsHellgate() ? VectorLengthSquared( vLineEdge ) : VectorLengthSquaredXY( vLineEdge );
	}

	int pnTargetTypes[ NUM_TARGET_TYPES ];
	int nTargetTypeCount = 0;

	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DEAD ) ||
		TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_TARGET_DEAD_OR_DYING ) ||
		TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START ) ||
		TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START ) ||
		TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_FORCE ) )//force appears to be a tugboat only thing so it shouldn't effect hellgate
		pnTargetTypes[ nTargetTypeCount++ ] = TARGET_DEAD;
	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT ) )
		pnTargetTypes[ nTargetTypeCount++ ] = TARGET_OBJECT;
	if ( UnitIsA( tTargetingQuery.tFilter.nUnitType, UNITTYPE_MISSILE ) )
		pnTargetTypes[ nTargetTypeCount++ ] = TARGET_MISSILE;
	else
	{
		int nSeekerType = UnitGetTargetType( tTargetingQuery.pSeekerUnit ) ;
		if(UnitIsA(tTargetingQuery.pSeekerUnit, UNITTYPE_MISSILE))
		{
			UNIT * pOwner = UnitGetUltimateOwner(tTargetingQuery.pSeekerUnit);
			if(pOwner)
			{
				nSeekerType = UnitGetTargetType( pOwner ) ;
			}
		}
		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND ) )
		{
			if( nSeekerType == TARGET_DEAD &&
				AppIsTugboat() ) //not sure if this is such a good thing for hellgate.
			{
				UNIT *pOwner = UnitGetUltimateOwner(tTargetingQuery.pSeekerUnit);
				if(pOwner)
				{
					if( UnitGetGenus( pOwner ) == GENUS_MONSTER )
					{
						pnTargetTypes[ nTargetTypeCount++ ] = TARGET_BAD;
					}
					else
					{
						pnTargetTypes[ nTargetTypeCount++ ] = TARGET_GOOD;
					}
				}
			}
			pnTargetTypes[ nTargetTypeCount++ ] = nSeekerType;


		}
		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY ) )
		{
			int nEnemyType;
			switch( nSeekerType )
			{
			case TARGET_GOOD: nEnemyType = TARGET_BAD;  break;
			case TARGET_BAD:  nEnemyType = TARGET_GOOD; break;
			case TARGET_DEAD:
				{
					switch(UnitGetTeam(tTargetingQuery.pSeekerUnit))
					{
					case TEAM_BAD:			nEnemyType = TARGET_GOOD;	break;
					case TEAM_PLAYER_START:	nEnemyType = TARGET_BAD;	break;
					default:				nEnemyType = TARGET_BAD;	break;
					}
				}
				break;
			default:		  nEnemyType = TARGET_BAD;	 break;
			}
			pnTargetTypes[ nTargetTypeCount++ ] = nEnemyType;

			// hack to add good list to target types if in pvp
			if (nEnemyType == TARGET_BAD && UnitPvPIsEnabled(tTargetingQuery.pSeekerUnit))
			{
				BOOL bFound = FALSE;
				for (int ii = 0; ii < nTargetTypeCount; ++ii)
				{
					if (pnTargetTypes[ii] == TARGET_GOOD)
					{
						bFound = TRUE;
					}
				}
				if (!bFound)
				{
					pnTargetTypes[nTargetTypeCount++] = TARGET_GOOD;
				}
			}
		} 
	}

	ROOM* pRoom = tTargetingQuery.pCenterRoom ? tTargetingQuery.pCenterRoom : tTargetingQuery.pSeekerUnit->pRoom;
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETURN( pLevel );

	float pfRangeBest[ MAX_TARGETS_PER_QUERY ];
	BOOL bDone = FALSE;
	float fLOSDistanceSquared = tTargetingQuery.fLOSDistance * tTargetingQuery.fLOSDistance;
#define ROOM_COLL_FUDGE_FACTOR (1.5f)
	float fRoomCullDistanceSquared = (tTargetingQuery.fMaxRange + ROOM_COLL_FUDGE_FACTOR) * (tTargetingQuery.fMaxRange + ROOM_COLL_FUDGE_FACTOR);


	SKILL_CONTEXT skill_context(pGame, tTargetingQuery.pSeekerUnit, tTargetingQuery.pSkillData ? tTargetingQuery.pSkillData->nId : INVALID_ID, 0, tTargetingQuery.pSkillData, INVALID_ID, 0, 0);

	// what type of neighboring room will we use
	int nNumRoomNeighbors = RoomGetVisibleRoomCount(pRoom);
	for (int ii = -1; ii < nNumRoomNeighbors && !bDone; ii++)
	{
		ROOM * pRoomTest( NULL );
		pRoomTest = (ii == -1) ? pRoom : RoomGetVisibleRoom(pRoom, ii);
		// connected rooms can be NULL
		if( !pRoomTest )
		{
			continue;
		}

		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE ) )
		{
			if ( RoomGetDistanceSquaredToPoint(pRoomTest, vTargetingLocation) > fRoomCullDistanceSquared &&
				 RoomGetDistanceSquaredToPoint(pRoomTest, vTargetingLocationEnd) > fRoomCullDistanceSquared )
				 continue;
		}
		else if(RoomGetDistanceSquaredToPoint(pRoomTest, vTargetingLocation) > fRoomCullDistanceSquared)
		{
			continue;
		}
		for ( int jj = 0; jj < nTargetTypeCount; jj++ )
		{
			UNIT * pUnitCurr = pRoomTest->tUnitList.ppFirst[ pnTargetTypes[ jj ] ];

			for ( ; pUnitCurr && ! bDone; pUnitCurr = pUnitCurr->tRoomNode.pNext )
			{
				if ( pUnitCurr == tTargetingQuery.pSeekerUnit &&
					! TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_SEEKER ))
					continue;

				if (sSkillFilterTarget(skill_context, pUnitCurr, tTargetingQuery.tFilter, TRUE, FALSE))
				{
					continue;
				}

				float fMaxRangeToUnit = tTargetingQuery.fMaxRange;
				float fMaxRangeToUnitSquared = fMaxRangeToUnit * fMaxRangeToUnit;
				if ( tTargetingQuery.nRangeModifierStat_Min != INVALID_ID || tTargetingQuery.nRangeModifierStat_Percent != INVALID_ID &&
					 !TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE ) )
				{
					int nMin		= tTargetingQuery.nRangeModifierStat_Min		!= INVALID_ID ? UnitGetStat( pUnitCurr, tTargetingQuery.nRangeModifierStat_Min )	 : 0;
					int nPercent	= tTargetingQuery.nRangeModifierStat_Percent	!= INVALID_ID ? UnitGetStat( pUnitCurr, tTargetingQuery.nRangeModifierStat_Percent ) : 0;

					if ( nPercent != 0 )
					{
						fMaxRangeToUnit = fMaxRangeToUnit * (100.0f - (float)nPercent) / 100.0f;
					}
					if ( nMin != 0 )
					{
						float fMin = (float)nMin;
						if ( fMaxRangeToUnit < fMin )
							fMaxRangeToUnit = fMin;
					}
					fMaxRangeToUnitSquared = fMaxRangeToUnit * fMaxRangeToUnit;
				}

				float fRangeSquared;
				if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE ) )
				{
					ASSERT_RETURN( fLineLengthSquared != 0.0f );
					VECTOR vUnitCurrPos = UnitGetPosition( pUnitCurr );
					float fU =  ( ( ( vUnitCurrPos.fX - vTargetingLocation.fX ) * vLineEdge.fX ) +
						( ( vUnitCurrPos.fY - vTargetingLocation.fY ) * vLineEdge.fY ) +
						( ( vUnitCurrPos.fZ - vTargetingLocation.fZ ) * vLineEdge.fZ ) ) /
						fLineLengthSquared;

					// Check if point falls within the line segment
					if ( fU < 0.0f || fU > 1.0f )
						continue;

					VECTOR Intersection;
					Intersection.fX = vTargetingLocation.fX + ( fU * vLineEdge.fX );
					Intersection.fY = vTargetingLocation.fY + ( fU * vLineEdge.fY );
					Intersection.fZ = vTargetingLocation.fZ + ( fU * vLineEdge.fZ );

					Intersection = vUnitCurrPos - Intersection;
					fRangeSquared = AppIsHellgate() ? VectorLengthSquared( Intersection ) : VectorLengthSquaredXY( Intersection );
					if ( ! UnitIsInRange( pUnitCurr, vTargetingLocation, fMaxRangeToUnit, &fRangeSquared ) )
						continue;
				}
				else if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE ) )
				{
					BOOL bAllowDead = TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DEAD ) ||
						TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_TARGET_DEAD_OR_DYING ) ||
						TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START ) ||
						TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START ) ||
						TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_FORCE ); //force is not used by Hellgate. Should be ok just to leave it
					if ( ! UnitCanMeleeAttackUnit( tTargetingQuery.pSeekerUnit, pUnitCurr, NULL, 0, fMaxRangeToUnit, FALSE, NULL, bAllowDead ) )
						continue;
					fRangeSquared = 0.0f; // if we can melee, we don't care about the range anymore
				}
				else if ( ! TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION ) )
				{
					fRangeSquared = AppIsHellgate() ? UnitsGetDistanceSquared( tTargetingQuery.pSeekerUnit, pUnitCurr ) :
									UnitsGetDistanceSquaredXY( tTargetingQuery.pSeekerUnit, pUnitCurr );

					if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_UNIT_RADIUS ) )
					{
						if ( ! UnitsAreInRange( tTargetingQuery.pSeekerUnit, pUnitCurr, 0.0f, fMaxRangeToUnit, &fRangeSquared ) )
							continue;
					} else {
						if ( fRangeSquared >= fMaxRangeToUnitSquared )
							continue;
					}
				}
				else
				{
					VECTOR vUnitCurrPosition = UnitGetPosition( pUnitCurr );
					//tugboat doesn't use Z
					vTargetingLocation.z = (AppIsTugboat()) ? 0 : vTargetingLocation.z;
					vUnitCurrPosition.z = (AppIsTugboat()) ? 0 : vTargetingLocation.z;
					fRangeSquared = VectorDistanceSquared( vTargetingLocation, vUnitCurrPosition );
					if ( ! UnitIsInRange( pUnitCurr, vTargetingLocation, fMaxRangeToUnit, &fRangeSquared ) )
						continue;
				}


				if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_DIRECTION ) &&
					 ( AppIsHellgate() && fRangeSquared > 0.0f ) )
				{
					VECTOR vDelta = UnitGetPosition( pUnitCurr );
					if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION ) &&
						 tTargetingQuery.pvLocation)
					{
						vDelta -= *tTargetingQuery.pvLocation;
					}
					else
					{
						vDelta -= UnitGetPosition( tTargetingQuery.pSeekerUnit );
					}
					VectorNormalize( vDelta, vDelta );
					VECTOR vUnitDirection = UnitGetFaceDirection( tTargetingQuery.pSeekerUnit, FALSE );					
					if( AppIsHellgate() )
					{
						float fDot = VectorDot( vUnitDirection, vDelta );
						if ( fDot < tTargetingQuery.fDirectionTolerance )
							continue;
					}
					else
					{
						//Need to ask Travis why he's doing it this way versus hellgate way
						float fDot = acos( PIN( VectorDot( vUnitDirection, vDelta ), -1.0f, 1.0f ) );
						if ( fDot > tTargetingQuery.fDirectionTolerance )
							continue;
					}
				}

				if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS ) )
				{
					if( !( AppIsTugboat() && UnitGetLevel( pUnitCurr )->m_pLevelDefinition->bContoured ) )
					{
						if(tTargetingQuery.fLOSDistance <= 0.0f || fRangeSquared > fLOSDistanceSquared)
						{
							VECTOR vTargetPos = UnitGetAimPoint( pUnitCurr );

							VECTOR vDelta = vTargetPos - vLOSPosition;
							float fDistance = VectorLength( vDelta );
							VECTOR vDirection;
							if ( fDistance != 0.0f )
								VectorScale( vDirection, vDelta, 1.0f / fDistance );
							else
								vDirection = VECTOR( 0, 1, 0 );

							fDistance = PIN( fDistance, 0.0f, tTargetingQuery.fMaxRange );

							if ( fDistance )
							{
								float fDistToCollide = LevelLineCollideLen( pGame, pLevel, vLOSPosition, vDirection, fDistance );
								if ( fDistToCollide < fDistance )
									continue;
							}

							UNITTYPE eBlockingUnitType = UNITTYPE_BLOCKING;
							UNIT * pBlockingUnit = SelectTarget(pGame, tTargetingQuery.pSeekerUnit, 0, vLOSPosition, vDirection, fDistance, NULL, NULL, NULL, INVALID_ID, NULL, INVALID_ID, &eBlockingUnitType);
							if ( pBlockingUnit && pUnitCurr != pBlockingUnit)
								continue;
						}
					}
				}

				if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_SIMPLE_PATH_EXISTS ) && pUnitCurr->m_pPathing )
				{
					ROOM * pUnitCurrRoom = NULL;
					int nUnitPathNodeIndex = INVALID_ID;
					UnitGetPathNodeOccupied(pUnitCurr, &pUnitCurrRoom, &nUnitPathNodeIndex);
					if(pUnitCurrRoom != NULL && nUnitPathNodeIndex >= 0)
					{
						ROOM_PATH_NODE * pUnitCurrPathNode = RoomGetRoomPathNode(pUnitCurrRoom, nUnitPathNodeIndex);
						if(!RoomPathNodeDoesSimplePathExistFromNodeToPoint(pUnitCurrRoom, pUnitCurrPathNode, vLOSPosition))
							continue;
					}
				}

				if ( tTargetingQuery.codeSkillFilter )
				{
					int code_len = 0;
					BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tTargetingQuery.codeSkillFilter, &code_len);
					if ( code_ptr && ! VMExecI( pGame, pUnitCurr, NULL, code_ptr, code_len) )
						continue;
				}

				if(tTargetingQuery.pfnFilter)
				{
					if(!(tTargetingQuery.pfnFilter)(pUnitCurr, tTargetingQuery.pFilterEventData))
						continue;
				}

				if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT ) )
				{
					tTargetingQuery.nUnitIdCount++;
				}
				else if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST ) )
				{// see if it is closer than the other ones
					BOOL bAddedToList = FALSE;
					for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
					{
						if ( pfRangeBest[ i ] > fRangeSquared )
						{// shift the other targets to make room
							if ( tTargetingQuery.nUnitIdCount < tTargetingQuery.nUnitIdMax )
								tTargetingQuery.nUnitIdCount++;

							if ( tTargetingQuery.nUnitIdCount - i > 0 )
							{
								for ( int j = tTargetingQuery.nUnitIdCount - 1; j > i; j-- )
								{
									pfRangeBest[ j ] = pfRangeBest[ j - 1 ];
									tTargetingQuery.pnUnitIds[ j ] = tTargetingQuery.pnUnitIds[ j - 1 ];
								}
							}
							pfRangeBest[ i ] = fRangeSquared;
							tTargetingQuery.pnUnitIds[ i ] = UnitGetId( pUnitCurr );
							bAddedToList = TRUE;
							break;
						}
					}
					if ( ! bAddedToList && tTargetingQuery.nUnitIdCount < tTargetingQuery.nUnitIdMax )
					{
						tTargetingQuery.pnUnitIds[ tTargetingQuery.nUnitIdCount ] = UnitGetId( pUnitCurr );
						pfRangeBest[ tTargetingQuery.nUnitIdCount ] = fRangeSquared;
						tTargetingQuery.nUnitIdCount++;
					}
				} 
				else if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND ) )
				{ // just add it to the end
					if ( tTargetingQuery.nUnitIdCount < tTargetingQuery.nUnitIdMax )
					{
						tTargetingQuery.pnUnitIds[ tTargetingQuery.nUnitIdCount ] = UnitGetId( pUnitCurr );
						tTargetingQuery.nUnitIdCount++;
					}
					if ( tTargetingQuery.nUnitIdCount == tTargetingQuery.nUnitIdMax )
					{
						bDone = TRUE;
					}
				} 
			}
		}
	}

	if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_RANDOM ) )
	{ // mix up the results
		for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
		{
			int nSwap = RandGetNum(pGame, tTargetingQuery.nUnitIdCount);
			int nTemp = tTargetingQuery.pnUnitIds[ nSwap ];
			tTargetingQuery.pnUnitIds[ nSwap ] = tTargetingQuery.pnUnitIds[ i ];
			tTargetingQuery.pnUnitIds[ i ] = nTemp;
		}
	}
	else if( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_SORT_BY_UNITID ) || 
		TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_SORT_BY_DISTANCE ) )
	{
		SkillTargetQuerySortContext tSortContext;
		tSortContext.pGame = pGame;
		tSortContext.pCenter = &vTargetingLocation;
		if( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_SORT_BY_UNITID ) )
		{
			qsort_s(tTargetingQuery.pnUnitIds, tTargetingQuery.nUnitIdCount, sizeof(UNITID), sSkillTargetQuerySortCompareByUnitId, &tSortContext);
		}
		else
		{
			qsort_s(tTargetingQuery.pnUnitIds, tTargetingQuery.nUnitIdCount, sizeof(UNITID), sSkillTargetQuerySortCompareByDistance, &tSortContext);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * SkillGetNearestTarget( 
	GAME * pGame, 
	UNIT * pUnit, 
	const SKILL_DATA * pSkillData,
	float fMaxRange, 
	VECTOR * pvLocation,
	DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ] )		//tugboat added
{
	UNITID pnNearest[ 1 ];

	ASSERT_RETNULL( pSkillData );

	SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pnUnitIds = pnNearest;
	tTargetingQuery.nUnitIdMax = 1;
	tTargetingQuery.fMaxRange = fMaxRange;
	tTargetingQuery.pvLocation = pvLocation;
	if ( pdwFlags )
	{
		for ( int i = 0; i < NUM_TARGET_QUERY_FILTER_FLAG_DWORDS; i++ )
			tTargetingQuery.tFilter.pdwFlags[ i ] |= pdwFlags[ i ];
	}
	if ( pvLocation )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	}
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE ) )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
	}
	//tugboat stuff but should be ok to leave in for hellgate
	tTargetingQuery.fDirectionTolerance = pSkillData->fFiringCone;
	//tugboat stuff
	SkillTargetQuery( pGame, tTargetingQuery );

	if ( tTargetingQuery.nUnitIdCount == 0 )
		return NULL;
	return UnitGetById( pGame, tTargetingQuery.pnUnitIds[ 0 ] );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void SkillGetNearbyTargets( 
//	GAME * pGame, 
//	UNIT * pUnit, 
//	SKILL_DATA * pSkillData,
//	float fMaxRange, 
//	VECTOR * pvLocation,
//	UNITID * pnUnitIds,
//	int nUnitIdMax,
//	int & nUnitIdCount,
//	DWORD dwSkillSeed )
//{
//	nUnitIdCount = 0;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTargetPeacemaker(
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon,
	SKILL_DATA * pSkillData, 
	UNIT ** ppTargetUnit,
	VECTOR & vTarget )
{
	ASSERT_RETFALSE( ppTargetUnit );

	int nWeaponIndex = WardrobeGetWeaponIndex( pUnit, UnitGetId( pWeapon ) );

	LEVEL* pLevel = UnitGetLevel( pUnit );

	VECTOR vPosition;
	VECTOR vDirection;
	UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, &vPosition, &vDirection );

	float fCollideDist = LevelLineCollideLen(pGame, pLevel, vPosition, vDirection, pSkillData->fParam1);

	if ( *ppTargetUnit )
	{
		vTarget = UnitGetAimPoint( *ppTargetUnit );
	}
	else if ( fCollideDist >= pSkillData->fParam1 )
	{
		VECTOR vDelta = vDirection;
		vDelta *= pSkillData->fParam1;
		vTarget = vPosition + vDelta;

		*ppTargetUnit = SkillGetNearestTarget( pGame, pUnit, pSkillData, pSkillData->fParam2, &vTarget );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef BOOL FP_SKILL_TARGET( GAME * pGame, UNIT * pUnit, UNIT * pWeapon, SKILL_DATA * pSkillData, UNIT ** ppTargetUnit, VECTOR & vTarget );

static FP_SKILL_TARGET * sgpSkillTargetFunc[] =
{
	NULL,
	sTargetPeacemaker,
};
static int sgnSkillTargetFuncCount = sizeof( sgpSkillTargetFunc ) / sizeof( FP_SKILL_TARGET * );
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillFindTarget(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon,
	UNIT ** ppTargetUnit,
	VECTOR & vTarget )
{
	ASSERT_RETFALSE( ppTargetUnit );
	int nWeaponSkill = ItemGetPrimarySkill( pWeapon );
	int nSkillTargeting = nSkill;

	SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
	ASSERT( pSkillData );
	SKILL_DATA * pSkillDataTargeting = pSkillData;
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_TARGETING ) )
	{
		if ( nWeaponSkill != INVALID_ID )
		{
			pSkillDataTargeting = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nWeaponSkill );
			nSkillTargeting = nWeaponSkill;
		}
		else 
			return FALSE;
	}

	if ( !SkillIsValidTarget( pGame, pUnit, *ppTargetUnit, pWeapon, nSkillTargeting,  TRUE ) )
	{
		*ppTargetUnit = NULL;
	}

	if ( pSkillDataTargeting->nTargetingFunc > 0 && pSkillDataTargeting->nTargetingFunc < sgnSkillTargetFuncCount )
	{
		if (! sgpSkillTargetFunc[ pSkillDataTargeting->nTargetingFunc ]( pGame, pUnit, pWeapon, pSkillDataTargeting, ppTargetUnit, vTarget ) )
			return FALSE;
	}

	if ( !*ppTargetUnit && SkillDataTestFlag( pSkillDataTargeting, SKILL_FLAG_FIND_TARGET_UNIT ) )
	{
		float fRangeMax = SkillGetRange( pUnit, nSkillTargeting, pWeapon );
		//check direction for tugboat
		DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
		ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
		if ( ! AppIsHellgate() )
		{
			SETBIT( pdwFlags, SKILL_TARGETING_BIT_CHECK_DIRECTION );
		}
		*ppTargetUnit = SkillGetNearestTarget( pGame, pUnit, pSkillDataTargeting, fRangeMax, NULL, pdwFlags );
	}

	if ( ! *ppTargetUnit && SkillDataTestFlag( pSkillDataTargeting, SKILL_FLAG_MUST_TARGET_UNIT ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if   !ISVERSION(SERVER_VERSION)
void c_SkillEventsLoad(
	GAME * pGame,
	int nSkillEventsId)
{
	if ( nSkillEventsId != INVALID_ID )
	{
		SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, nSkillEventsId );
		for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
		{
			SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
			for ( int j = 0; j < pHolder->nEventCount; j++ )
			{
				SKILL_EVENT * pSkillEvent = &pHolder->pEvents[ j ];
				const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
				ASSERT_CONTINUE(pSkillEventTypeData);
				//Tugboat only had missiles but Looking at source control I'm guessing this just hasn't
				//been merged yet. So leaving it in.
				if ( pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES ||
					pSkillEventTypeData->eAttachedTable == DATATABLE_MONSTERS ||
					pSkillEventTypeData->eAttachedTable == DATATABLE_ITEMS ||
					pSkillEventTypeData->eAttachedTable == DATATABLE_OBJECTS )
				{
					int nUnitClass = pSkillEvent->tAttachmentDef.nAttachedDefId;
					if ( nUnitClass != INVALID_ID)
						UnitDataLoad( pGame, pSkillEventTypeData->eAttachedTable, nUnitClass );
				}
				if ( pSkillEventTypeData->eAttachedTable == DATATABLE_STATES )
				{
					if(pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID)
					{
						int nCount = 1;
						if(pSkillEventTypeData->nParamContainsCount > 0)
						{
							const SKILL_EVENT_PARAM * pParam = SkillEventGetParam(pSkillEvent, pSkillEventTypeData->nParamContainsCount);
							if(pParam)
							{
								nCount = pParam->nValue;
							}
						}
						for(int k=0; k<nCount; k++)
						{
							c_StateLoad(pGame, pSkillEvent->tAttachmentDef.nAttachedDefId + k);
						}
					}
				}
				if ( pSkillEventTypeData->bUsesAttachment )
				{
					ASSERT_RETURN( pSkillEventTypeData->eAttachedTable == DATATABLE_NONE );
					c_AttachmentDefinitionLoad( pGame, pSkillEvent->tAttachmentDef, INVALID_ID, "" );
				} 
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillLoad(
	GAME * pGame,
	int nSkill)
{
	if ( nSkill == INVALID_ID )
		return;
 
//	if ( !pGame || !IS_CLIENT( pGame ) )
//		return;

	SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
	if (! pSkillData )
		return;

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_LOADED ) )
		return;

	SkillDataSetFlag( pSkillData, SKILL_FLAG_LOADED, TRUE );

	if ( pSkillData->nCooldownFinishedSound != INVALID_ID )
		c_SoundLoad( pSkillData->nCooldownFinishedSound );

	c_SkillEventsLoad(pGame, pSkillData->nEventsId);
}

#endif//   !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if   !ISVERSION(SERVER_VERSION)
void c_SkillEventsFlagSoundsForLoad(
	GAME * pGame,
	SKILL_EVENTS_DEFINITION * pEvents,
	BOOL bLoadNow)
{
	for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
	{
		SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
		for ( int j = 0; j < pHolder->nEventCount; j++ )
		{
			SKILL_EVENT * pSkillEvent = &pHolder->pEvents[ j ];
			const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
			ASSERT_CONTINUE(pSkillEventTypeData);
			if ( pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES ||
				pSkillEventTypeData->eAttachedTable == DATATABLE_MONSTERS ||
				pSkillEventTypeData->eAttachedTable == DATATABLE_ITEMS ||
				pSkillEventTypeData->eAttachedTable == DATATABLE_OBJECTS )
			{
				int nUnitClass = pSkillEvent->tAttachmentDef.nAttachedDefId;
				if ( nUnitClass != INVALID_ID)
					c_UnitDataFlagSoundsForLoad( pGame, pSkillEventTypeData->eAttachedTable, nUnitClass, bLoadNow );
			}
			if ( pSkillEventTypeData->eAttachedTable == DATATABLE_STATES )
			{
				if(pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID)
				{
					int nCount = 1;
					if(pSkillEventTypeData->nParamContainsCount > 0)
					{
						const SKILL_EVENT_PARAM * pParam = SkillEventGetParam(pSkillEvent, pSkillEventTypeData->nParamContainsCount);
						if(pParam)
						{
							nCount = pParam->nValue;
						}
					}
					for(int k=0; k<nCount; k++)
					{
						c_StateFlagSoundsForLoad(pGame, pSkillEvent->tAttachmentDef.nAttachedDefId + k, bLoadNow);
					}
				}
			}
			if ( pSkillEventTypeData->bUsesAttachment )
			{
				ASSERT_RETURN( pSkillEventTypeData->eAttachedTable == DATATABLE_NONE );
				c_AttachmentDefinitionFlagSoundsForLoad( pGame, pSkillEvent->tAttachmentDef, bLoadNow );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillFlagSoundsForLoad(
	GAME * pGame,
	int nSkill,
	BOOL bLoadNow)
{
	if ( nSkill == INVALID_ID )
		return;

	if ( !pGame || !IS_CLIENT( pGame ) )
		return;

	SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( pGame, DATATABLE_SKILLS, nSkill );
	if (!pSkillData)
	{
		return;
	}

	if ( pSkillData->nCooldownFinishedSound != INVALID_ID )
		c_SoundFlagForLoad( pSkillData->nCooldownFinishedSound, bLoadNow );

	if ( pSkillData->nEventsId != INVALID_ID )
	{
		SKILL_EVENTS_DEFINITION * pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pSkillData->nEventsId );
		c_SkillEventsFlagSoundsForLoad(pGame, pEvents, bLoadNow);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_RUN_TO_DISTANCE_HELLGATE 3.8f
#define MAX_RUN_TO_DISTANCE_TUGBOAT 1.8f
BOOL c_SkillUnitShouldRunForward(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon,
	const VECTOR & vForward,
	BOOL bCheckRunToRangeFlag )
{
	if ( UnitHasState( pGame, pUnit, STATE_REMOVE_ON_MOVE ) )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );

	if ( bCheckRunToRangeFlag && ! SkillDataTestFlag( pSkillData, SKILL_FLAG_RUN_TO_RANGE ) )
		return FALSE;

	UNIT * pTarget = SkillGetAnyTarget( pUnit, nSkill, pWeapon, TRUE );
	if ( ! pTarget )
		pTarget = SkillGetAnyTarget( pUnit, INVALID_ID, pWeapon, TRUE );
	if ( ! pTarget )
	{
		
		pTarget = (AppIsHellgate())?UIGetTargetUnit():UIGetClickedTargetUnit();
		
	}

	if ( ! pTarget )
		return FALSE;

	float fRangeMax		= SkillGetRange( pUnit, nSkill, pWeapon );
	if ( UnitsAreInRange( pUnit, pTarget, 0.0f, fRangeMax, NULL ) && ! UnitTestMode( pUnit, MODE_RUN ) )
		return FALSE; // we are in range.  keep standing still

	float fRangeDesired = SkillGetDesiredRange( pUnit, nSkill, pWeapon );
	if ( UnitsAreInRange( pUnit, pTarget, 0.0f, fRangeDesired, NULL ) )
		return FALSE; // we are close enough
	
	if ( ! UnitsAreInRange( pUnit, pTarget, 0.0f, ((AppIsHellgate())?MAX_RUN_TO_DISTANCE_HELLGATE:MAX_RUN_TO_DISTANCE_TUGBOAT), NULL ) )
		return FALSE; // we are close enough

	VECTOR vDelta = UnitGetPosition( pTarget ) - UnitGetPosition( pUnit );
	VECTOR vDirection;
	VectorNormalize( vDirection, vDelta );
	if ( VectorDot( vDirection, vForward ) > 0.75f )
		return TRUE; // we are pointing in the right direction

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD c_SkillGetPlayerInputOverride(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vForward,
	BOOL & bSetModesToIdle )
{
	ASSERT_RETZERO( pUnit );
	ASSERT_RETZERO( pUnit->pSkillInfo );

	int nCount = pUnit->pSkillInfo->pSkillsOn.Count();

	SKILL_IS_ON * pCurr = nCount ? pUnit->pSkillInfo->pSkillsOn.GetPointer( 0 ) : NULL;

	for ( int i = 0; i < nCount; i++, pCurr++ )
	{
		int nSkill = pCurr->nSkill;
		UNIT * pWeapon = UnitGetById( pGame, pCurr->idWeapon );

		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
		if ( pSkillData->nPlayerInputOverride )
		{
			bSetModesToIdle = TRUE;
			return pSkillData->nPlayerInputOverride;
		}

		if ( c_SkillUnitShouldRunForward( pGame, pUnit, nSkill, pWeapon, vForward, TRUE ) )
			return PLAYER_MOVE_FORWARD;
	}

	return 0;
}

#endif//   !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillUnitSetInitialStats(
	UNIT *pUnit, 
	int nSkillID, 
	int nLevel)
{
	GAME* pGame = UnitGetGame(pUnit);
	
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkillID );

	ASSERT_RETURN(pSkillData);
	// Set the skill level
	int nSkillLevel = INVALID_ID;

	for ( int jj = 0; jj < MAX_LEVELS_PER_SKILL; jj++ )
	{
		if ( pSkillData->pnSkillLevelToMinLevel[ jj ] < 0 )
			break;
		if ( pSkillData->pnSkillLevelToMinLevel[ jj ] <= nLevel )
		{
			nSkillLevel = jj + 1;
		}
	}

	ASSERT_RETURN( nSkillLevel != INVALID_ID );

	UnitSetStat(pUnit, STATS_SKILL_LEVEL, nSkillID, nSkillLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillTestFlag(
	GAME *pGame,
	UNIT *pSkillUnit,
	int nFlag )
{
	int nSkillID = UnitGetSkillID(pSkillUnit);
	if (nSkillID != INVALID_ID)
	{
		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkillID );
		if ( pSkillData )
			return SkillDataTestFlag( pSkillData, nFlag );
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanLearnSkill( 
	UNIT * pUnit, 
	int nSkillId )
{
	if ( ! pUnit || nSkillId == INVALID_ID )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( UnitGetGame( pUnit ), nSkillId );
	if ( ! pSkillData )
		return FALSE;

	if ( ! UnitIsA( pUnit, pSkillData->nRequiredUnittype ) )
	{
		return FALSE;
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_LEARN ) && ! PlayerIsSubscriber(pUnit) )
	{
		return FALSE;
	}

	// we might want level or stat requirements here
	// in Tugboat, you must have available slots of the appropriate skillgroup type unoccupied
	// before you can learn the skill
	if( AppIsTugboat() &&
		UnitIsA( pUnit, UNITTYPE_PLAYER ) )
	{
		if( pSkillData->pnSkillGroup[ 0 ] != -1 )
		{
			int SkillsOpen = PlayerGetAvailableSkillsOfGroup( UnitGetGame( pUnit ), pUnit, pSkillData->pnSkillGroup[ 0 ] ) -
								PlayerGetKnownSkillsOfGroup( UnitGetGame( pUnit ), pUnit, pSkillData->pnSkillGroup[ 0 ] );
			if( SkillsOpen <= 0 )
			{
				return FALSE;
			}
		}
	}
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetNumOn(
	UNIT * pUnit,
	BOOL bCheckAI) 
{
	ASSERT_RETZERO(pUnit);

	SKILL_INFO * pSkillInfo = pUnit->pSkillInfo;
	ASSERT_RETZERO(pSkillInfo);

	if( !bCheckAI )
	{
		return pSkillInfo->pSkillsOn.Count();
	}

	int count = 0;
	for( unsigned int t = 0; t < pSkillInfo->pSkillsOn.Count(); t++ )
	{		
		if( pSkillInfo->pSkillsOn[ t ].nSkill != NULL )
		{
			const SKILL_DATA *pSkillData = SkillGetData( UnitGetGame( pUnit ), pSkillInfo->pSkillsOn[ t ].nSkill );
			if(!pSkillData)
				continue;

			if(SkillDataTestFlag(pSkillData, SKILL_FLAG_AI_IS_BUSY_WHEN_ON))
			{
				count++;
			}
		}
	}
	return count;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillGetIsOn(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	BOOL bIgnoreWeapon)
{
	return SkillIsOn(pUnit, nSkill, idWeapon, bIgnoreWeapon) != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillIsOnSetFlag(
	UNIT * pUnit,
	int nFlag,
	BOOL bSet,
	int nSkill,
	UNITID idWeapon)
{
	SKILL_IS_ON * pSkillIsOn = SkillIsOn(pUnit, nSkill, idWeapon, FALSE);
	if(!pSkillIsOn)
		return;

	SETBIT(pSkillIsOn->dwFlags, nFlag, bSet);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsOnTestFlag(
	UNIT * pUnit,
	int nFlag,
	int nSkill,
	UNITID idWeapon)
{
	SKILL_IS_ON * pSkillIsOn = SkillIsOn(pUnit, nSkill, idWeapon, FALSE);
	if(!pSkillIsOn)
		return FALSE;

	return TESTBIT(pSkillIsOn->dwFlags, nFlag);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD SkillGetCRC(
	const VECTOR & vPosition,
	const VECTOR & vWeaponPosition,
	const VECTOR & vWeaponDirection)
{
	DWORD dwCRC = 0;
	dwCRC = CRC(dwCRC, (const BYTE*)&vPosition.fX, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vPosition.fY, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vPosition.fZ, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponPosition.fX, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponPosition.fY, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponPosition.fZ, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponDirection.fX, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponDirection.fY, sizeof(float));
	dwCRC = CRC(dwCRC, (const BYTE*)&vWeaponDirection.fZ, sizeof(float));
	return dwCRC;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillItemGetSkill(
	UNIT * pSkillItem)
{
	STATS_ENTRY statsEntry[1];
	if (UnitGetStatsRange(pSkillItem, STATS_SKILL_LEVEL, 0, statsEntry, arrsize(statsEntry)) > 0)
	{
		//left in for tugboat
		int nSkill = StatGetParam( STATS_SKILL_LEVEL, statsEntry[ 0 ].GetParam(), 0 );
		if (nSkill == INVALID_ID)
		{
			return INVALID_ID;
		}

		return statsEntry[0].GetParam();
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillGetSkillCurrentlyUsingWeapon(
	UNIT * pUnit,
	UNIT * pWeapon)
{
	ASSERT_RETINVALID( pUnit );
	if ( ! pWeapon )
		return INVALID_ID;
	SKILL_INFO * pSkillInfo = pUnit->pSkillInfo;
	ASSERT_RETINVALID(pSkillInfo);

	for ( int i = 0; i < (int) pSkillInfo->pSkillsOn.Count(); i++ )
	{
		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		int nSkill = pSkillInfo->pSkillsOn[ i ].nSkill;
		const SKILL_DATA * pSkillData = SkillGetData(UnitGetGame(pUnit), nSkill);
		if(pSkillData && SkillDataTestFlag(pSkillData, SKILL_FLAG_DOES_NOT_ACTIVELY_USE_WEAPON))
			continue;

		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

		for ( int j = 0; j < MAX_WEAPON_LOCATIONS_PER_SKILL; j++ )
		{
			if ( pWeapons[ j ] == pWeapon )
				return nSkill;
		}
	}

	return ItemGetPrimarySkill( pWeapon );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SkillDataGetStateDuration(
	GAME * game,
	UNIT * unit,
	int skill,
	int sklvl,
	BOOL bApplyBonus)
{
	ASSERT_RETINVALID(game);
	ASSERT_RETINVALID(unit);
	ASSERT_RETINVALID(skill != INVALID_ID);
	
	if (sklvl <= 0)
	{
		sklvl = SkillGetLevel(unit, skill);
		sklvl = MAX(1, sklvl);
	}

	const SKILL_DATA * skill_data = SkillGetData(game, skill);
	ASSERT_RETINVALID(skill_data);

	int duration = 0;
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStateDuration, &code_len);
		if (code_ptr)
		{
			//UnitSetStat( unit, STATS_SKILL_EXECUTED_BY_ITEMCLASS, UnitGetClass( unit ) );
			duration += VMExecI(game, unit, skill, sklvl, skill, sklvl, INVALID_ID, code_ptr, code_len);

			if (bApplyBonus && duration > 0)
			{
				int code_len = 0;
				BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_SKILLS, skill_data->codeStateDurationBonus, &code_len);
				if (code_ptr)
				{
					int duration_bonus = VMExecI(game, unit, skill, sklvl, skill, sklvl, INVALID_ID, code_ptr, code_len);
					if (duration_bonus != 0)
					{
						duration += PCT(duration, duration_bonus);
					}
				}
			}
		}
	}

	return duration;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//the following two functions return the variables for a skill by index.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SkillGetVariable( UNIT *pUnit,
					  UNIT *pObject,
					  const SKILL_DATA * pSkillData,
					  SKILL_VARIABLES skillVariable,
					  int nSkillLevelOverride /* = -1 */ )
{
	ASSERTX_RETINVALID( pUnit, "Unit needed for SkillGetVariable" );
	GAME *pGame = UnitGetGame( pUnit );
	int nSkillLevel( nSkillLevelOverride );
	if( nSkillLevelOverride <= 0 )			
		nSkillLevel = SkillGetLevel( UnitGetUltimateOwner( pUnit ), pSkillData->nId );	
	ASSERTX_RETINVALID( pSkillData, "Skill data not found for variable" );
	int code_len( 0 );	
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeVariables[ skillVariable ], &code_len);
	int returnValue( 0 );	
	if (code_ptr)
	{				
		returnValue = VMExecI( pGame, pUnit, pObject, NULL, pSkillData->nId, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len);		
	}
	return returnValue;
}

int SkillGetVariable( UNIT *pUnit,
					  UNIT *pObject,
					  int nSkill,
					  SKILL_VARIABLES skillVariable,
					  int nSkillLevelOverride /* = -1 */  )
{
	ASSERTX_RETINVALID( pUnit, "Unit needed for SkillGetVariable" );
	ASSERTX_RETINVALID( nSkill != INVALID_LINK, "Skill data not found for variable" );
	return SkillGetVariable( pUnit,
		                     pObject,
							 (SKILL_DATA *)ExcelGetData( UnitGetGame( pUnit ), DATATABLE_SKILLS, nSkill ),
							 skillVariable,
							 nSkillLevelOverride );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//end of Varaible get
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sGetSkillForStatTransfer(
	int nSkill)
{
	const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	if (pSkillData->nSkillParent != INVALID_LINK)
	{
		return sGetSkillForStatTransfer( pSkillData->nSkillParent );
	}
	return nSkill;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillTransferStatsToUnit(
	UNIT *pUnit,
	UNIT *pGiver,
	int nSkill,
	int nSkillLevel,
	int nSelector)
{	
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// apply stats from the skill
	//if (IS_SERVER( pGame ))
	{
		int nSkillForTransfer = sGetSkillForStatTransfer(nSkill);

		STATS * stats = SkillCreateSkillEventStats(pGame, pGiver, nSkillForTransfer, NULL, nSkillLevel, pUnit );
		if (stats)
		{
			StatsListAdd(pUnit, stats, TRUE, nSelector);
		}
	}
}


//----------------------------------------------------------------------------
//returns the bonus skill value
//----------------------------------------------------------------------------
int SkillGetBonusSkillValue( UNIT *pUnit, const SKILL_DATA *pSkillData, int nBonusIndex )
{
	ASSERTX_RETZERO( pSkillData, "Expected skill data" );
	ASSERTX_RETZERO( pUnit, "Expected Unit" );
	ASSERTX_RETZERO( nBonusIndex >= 0 && nBonusIndex < MAX_SKILL_BONUS_COUNT, "INvalid index" );			
	GAME *pGame = UnitGetGame( pUnit );
	if( pSkillData->tSkillBonus.nSkillBonusByValueByScript[ nBonusIndex ] != NULL )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->tSkillBonus.nSkillBonusByValueByScript[ nBonusIndex ], &code_len);		
		int returnValue = VMExecI( pGame, UnitGetUltimateOwner( pUnit ), NULL, NULL, pSkillData->nId, INVALID_LINK, pSkillData->nId, INVALID_SKILL_LEVEL, INVALID_LINK, code_ptr, code_len);
		return returnValue; //debuging 
	}
	return 0;
}


//----------------------------------------------------------------------------
// executes the attack script from a skill on stats to get transfered
//----------------------------------------------------------------------------
void SkillExecuteScript( SKILL_SCRIPT_ENUM scriptToExecute, 
						 UNIT *pUnit, 
						 int nSkill, 
						 int nSkillLevel,// = INVALID_SKILL_LEVEL,
						 UNIT *pObject,// = NULL,  
						 STATS *stats,// = NULL, 
						 int param1,// = INVALID_ID, 
						 int param2,// = INVALID_ID, 								 								  
						 int nStateToPass, // = INVALID_ID
						 int nItemClassExecutedSkill )// = INVALID_ID )
{
	
	ASSERTX_RETURN( nSkill != INVALID_ID, "INVALID SKILL" );	
	ASSERTX_RETURN( pUnit, "invalid unit" );	
	GAME *pGame = UnitGetGame( pUnit );
	const SKILL_DATA *skillData = SkillGetData( pGame, nSkill );
	ASSERTX_RETURN( skillData, "invalid skill" );
	if( nSkillLevel == INVALID_SKILL_LEVEL && pUnit )
	{
		nSkillLevel = SkillGetLevel( pUnit, nSkill, FALSE );
	}
	UINT nTableRef = DATATABLE_SKILLS;
	PCODE pCode( NULL_CODE );
	switch( scriptToExecute )
	{
	case SKILL_SCRIPT_CRAFTING_PROPERTIES:
		pCode = skillData->codeCraftingProperties;
		break;
	case SKILL_SCRIPT_CRAFTING:
		pCode = skillData->codeCrafting;
		break;
	case SKILL_SCRIPT_TRANSFER_ON_ATTACK:
		pCode = skillData->codeStatsSkillTransferOnAttack;
		break;
	case SKILL_SCRIPT_STATE_REMOVED:
		pCode = skillData->codeStateRemoved;
		break;
	//this will basically run an item's skill script data as if it was it's own script data
	case SKILL_SCRIPT_ITEM_SKILL_SCRIPT1:
	case SKILL_SCRIPT_ITEM_SKILL_SCRIPT2:
	case SKILL_SCRIPT_ITEM_SKILL_SCRIPT3:
		{
			ASSERTX_RETURN( nItemClassExecutedSkill != INVALID_ID, "invalid item Data" );
			const UNIT_DATA *pItemData = UnitGetData( pGame, GENUS_ITEM, nItemClassExecutedSkill );
			ASSERTX_RETURN( pItemData != NULL, "invalid item Data" );
			pCode = pItemData->codeSkillExecute[ scriptToExecute - SKILL_SCRIPT_ITEM_SKILL_SCRIPT1 ];
			nTableRef = DATATABLE_ITEMS;
		}
		break;
	default:
		return; //script not supported
	}
	if( pCode != NULL_CODE )
	{
		int code_len( 0 );
		BYTE * code_ptr = ExcelGetScriptCode( pGame, nTableRef, pCode, &code_len);
		ASSERTX_RETURN( code_ptr, "NO stats in skill attack script" );	
		VMExecI( pGame, pUnit, pObject, stats, param1, param2, nSkill, nSkillLevel, nStateToPass, code_ptr, code_len);		
	}
}

inline const SKILL_STATS * GetSkillStatData( int nSkillStatID, int &nSkillLvl )
{
	const SKILL_STATS *pSkillStats = (const SKILL_STATS*)ExcelGetData( NULL, DATATABLE_SKILL_STATS, nSkillStatID );
	while( pSkillStats->nUsingXColumns != 0 && 
		   nSkillLvl > pSkillStats->nUsingXColumns )
	{
		ASSERT_BREAK( pSkillStats->nLinksTo != INVALID_ID );
		nSkillLvl -= pSkillStats->nUsingXColumns;
		pSkillStats = (const SKILL_STATS*)ExcelGetData( NULL, DATATABLE_SKILL_STATS, pSkillStats->nLinksTo );
	}
	return pSkillStats;
}

int GetSkillStat( int nSkillStatID, UNIT *pUnit,  int nSkillLvl )
{

	ASSERT_RETINVALID( nSkillStatID != INVALID_ID );
	//bit ungly but this changes nSkillLvl
	const SKILL_STATS *pSkillStats = GetSkillStatData( nSkillStatID, nSkillLvl );
	ASSERT_RETINVALID( pSkillStats );
	if( nSkillLvl == INVALID_SKILL_LEVEL )
	{
		ASSERT_RETZERO( pUnit );
		ASSERT_RETZERO( pSkillStats->nSkillId != INVALID_ID );
		nSkillLvl = SkillGetLevel( pUnit, pSkillStats->nSkillId );
	}
	if( nSkillLvl <= 0 )
		return 0;	//default stat value.
	--nSkillLvl;	//levels start at 1, so we need to subtract one
	int nAdditionalBonus( 0 );
	if( pUnit != NULL && pSkillStats->nSkillModValueByTreeInvestment >= 0 && pSkillStats->nSkillId != INVALID_ID)
	{
		const SKILL_DATA *pSkillData = SkillGetData( UnitGetGame( pUnit ), pSkillStats->nSkillId );		
		float fPointsInvestedInTree = (float)GetPointsInvestedInSkillTab( pUnit, pSkillData );
		float fPCT = MIN( 100.0f, fPointsInvestedInTree )/(float)MAX_SKILL_POINTS_PER_TREE;
		float fPCTOfValue = ((float)pSkillStats->nSkillModValueByTreeInvestment * fPCT)/1000.0f;	//we made 1000 the max value or 1000 equals 100%
		nAdditionalBonus += (int)((float)pSkillStats->nSkillStatValues[ nSkillLvl ] * fPCTOfValue);				
	}
	int nValueToReturn = pSkillStats->nSkillStatValues[ nSkillLvl ] + nAdditionalBonus;
	//lets do any PVP stat changes here...
	if( pUnit != NULL && pSkillStats->nSkillModValueForPVP != -1 )
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );

		if(pLevel)
		{
			switch( MYTHOS_LEVELAREAS::LevelAreaGetLevelType( LevelGetLevelAreaID( pLevel ) ) )
			{
			case MYTHOS_LEVELAREAS::KLEVELTYPE_PVP:
				nValueToReturn = (int)MAX( 1, PCT( nValueToReturn, pSkillStats->nSkillModValueForPVP ) );
				break;
			}	
		}
	}
	return nValueToReturn;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillDataFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	SKILL_DATA * skill_data = (SKILL_DATA *)rowdata;

	if (skill_data->pnAchievements)
	{
		FREE(NULL, skill_data->pnAchievements);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillIsInSkillGroup(
	const SKILL_DATA * pSkillData,
	int nSkillGroup)
{
	ASSERT_RETFALSE(pSkillData);

	if(nSkillGroup < 0)
		return FALSE;

	for(int i=0; i<MAX_SKILL_GROUPS_PER_SKILL; i++)
	{
		if(pSkillData->pnSkillGroup[i] == nSkillGroup)
		{
			return TRUE;
		}
	}
	return FALSE;
}
