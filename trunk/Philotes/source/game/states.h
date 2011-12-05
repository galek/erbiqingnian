//----------------------------------------------------------------------------
//	states.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _STATES_H_
#define _STATES_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _DEFINITION_HEADER_INCLUDED_
#include "definition.h"
#endif

#ifndef _UNITTYPES_H_
#include "unittypes.h"
#endif

#ifndef _CONDITION_H_
#include "condition.h"
#endif

#ifndef _STATS_H_
#include "stats.h"
#endif

#include "c_attachment.h"
#include "e_irradiance.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum STATE_EVENT_FLAG
{
	STATE_EVENT_FLAG_FORCE_NEW_BIT,
	STATE_EVENT_FLAG_FIRST_PERSON_BIT,
	STATE_EVENT_FLAG_ADD_TO_CENTER_BIT,
	STATE_EVENT_FLAG_CONTROL_UNIT_ONLY_BIT,
	STATE_EVENT_FLAG_FLOAT_BIT,
	STATE_EVENT_FLAG_OWNED_BY_CONTROL_BIT,
	STATE_EVENT_FLAG_SET_IMMEDIATELY_BIT,
	STATE_EVENT_FLAG_CLEAR_IMMEDIATELY_BIT,
	STATE_EVENT_FLAG_NOT_CONTROL_UNIT_BIT,
	STATE_EVENT_FLAG_ON_WEAPONS_BIT,
	STATE_EVENT_FLAG_IGNORE_CAMERA_BIT,
	STATE_EVENT_FLAG_ON_CLEAR_BIT,
	STATE_EVENT_FLAG_CHECK_CONDITION_ON_CLEAR_BIT,
	STATE_EVENT_FLAG_SHARE_DURATION_BIT,
	STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD_BIT,
	STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS_BIT,
};

#define STATE_EVENT_FLAG_FORCE_NEW					MAKE_MASK( STATE_EVENT_FLAG_FORCE_NEW_BIT )
#define STATE_EVENT_FLAG_FIRST_PERSON				MAKE_MASK( STATE_EVENT_FLAG_FIRST_PERSON_BIT )
#define STATE_EVENT_FLAG_ADD_TO_CENTER				MAKE_MASK( STATE_EVENT_FLAG_ADD_TO_CENTER_BIT )
#define STATE_EVENT_FLAG_CONTROL_UNIT_ONLY			MAKE_MASK( STATE_EVENT_FLAG_CONTROL_UNIT_ONLY_BIT )
#define STATE_EVENT_FLAG_FLOAT						MAKE_MASK( STATE_EVENT_FLAG_FLOAT_BIT )
#define STATE_EVENT_FLAG_OWNED_BY_CONTROL			MAKE_MASK( STATE_EVENT_FLAG_OWNED_BY_CONTROL_BIT )
#define STATE_EVENT_FLAG_SET_IMMEDIATELY			MAKE_MASK( STATE_EVENT_FLAG_SET_IMMEDIATELY_BIT )
#define STATE_EVENT_FLAG_CLEAR_IMMEDIATELY			MAKE_MASK( STATE_EVENT_FLAG_CLEAR_IMMEDIATELY_BIT )
#define STATE_EVENT_FLAG_NOT_CONTROL_UNIT			MAKE_MASK( STATE_EVENT_FLAG_NOT_CONTROL_UNIT_BIT )
#define STATE_EVENT_FLAG_ON_WEAPONS					MAKE_MASK( STATE_EVENT_FLAG_ON_WEAPONS_BIT )
#define STATE_EVENT_FLAG_IGNORE_CAMERA				MAKE_MASK( STATE_EVENT_FLAG_IGNORE_CAMERA_BIT )
#define STATE_EVENT_FLAG_ON_CLEAR					MAKE_MASK( STATE_EVENT_FLAG_ON_CLEAR_BIT )
#define STATE_EVENT_FLAG_CHECK_CONDITION_ON_CLEAR	MAKE_MASK( STATE_EVENT_FLAG_CHECK_CONDITION_ON_CLEAR_BIT )
#define STATE_EVENT_FLAG_SHARE_DURATION				MAKE_MASK( STATE_EVENT_FLAG_SHARE_DURATION_BIT )
#define STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD	MAKE_MASK( STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD_BIT )
#define STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS	MAKE_MASK( STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS_BIT )

#define MAX_STATE_EQUIV						10
#define MAX_STATE_ASSSOC					3


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct STATE_EVENT_DATA;
enum GAMEFLAG;

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
typedef void (*PFN_STATE_EVENT_HANDLER)( GAME *pGame, const STATE_EVENT_DATA &tData );

//----------------------------------------------------------------------------
#define STATE_EVENT_PARAM_STRING_SIZE 32
#define STATE_EVENT_INDEX_SIZE 64
enum 
{
	STATE_TABLE_REFERENCE_ATTACHMENT,
	STATE_TABLE_REFERENCE_EVENT,
	NUM_STATE_TABLE_REFERENCES,
};

enum STATE_SCRIPTS_FLAGS_EXCEL
{
	STATE_SCRIPT_FLAG_EXCEL_ATTACK_MELEE,
	STATE_SCRIPT_FLAG_EXCEL_ATTACK_RANGED,
	STATE_SCRIPT_FLAG_EXCEL_ON_REMOVE,
	STATE_SCRIPT_FLAG_EXCEL_EXECUTE_ON_SOURCE,
	STATE_SCRIPT_FLAG_EXCEL_PULSE_CLIENT
};
enum STATE_SCRIPTS_FLAGS
{
	STATE_SCRIPT_FLAG_ATTACK_MELEE = MAKE_MASK(STATE_SCRIPT_FLAG_EXCEL_ATTACK_MELEE),
	STATE_SCRIPT_FLAG_ATTACK_RANGED  = MAKE_MASK(STATE_SCRIPT_FLAG_EXCEL_ATTACK_RANGED),
	STATE_SCRIPT_FLAG_ON_REMOVE  = MAKE_MASK(STATE_SCRIPT_FLAG_EXCEL_ON_REMOVE),
	STATE_SCRIPT_FLAG_ON_SOURCE  = MAKE_MASK(STATE_SCRIPT_FLAG_EXCEL_EXECUTE_ON_SOURCE),
	STATE_SCRIPT_FLAG_PULSE_CLIENT  = MAKE_MASK(STATE_SCRIPT_FLAG_EXCEL_PULSE_CLIENT)
};

struct STATE_EVENT_TYPE_DATA
{
	char szName[ STATE_EVENT_INDEX_SIZE ];
	char szParamText[ STATE_EVENT_PARAM_STRING_SIZE ];
	char szSetEventHandler[ MAX_FUNCTION_STRING_LENGTH ];
	SAFE_FUNC_PTR(PFN_STATE_EVENT_HANDLER, pfnSetEventHandler);
	char szClearEventHandler[ MAX_FUNCTION_STRING_LENGTH ];	
	SAFE_FUNC_PTR(PFN_STATE_EVENT_HANDLER, pfnClearEventHandler);
	int pnUsesTable[ NUM_STATE_TABLE_REFERENCES ];
	int pnTableFilter[ NUM_STATE_TABLE_REFERENCES ];
	BOOL bClientOnly;
	BOOL bServerOnly;
	DWORD dwFlagsUsed;
	BOOL bUsesBone;
	BOOL bUsesBoneWeight;
	BOOL bUsesAttachments;
	BOOL bAddsRopes;
	BOOL bUsesMatOverrideType;
	BOOL bRefreshStatsPointer;
	BOOL bUsesEnvironmentOverride;
	BOOL bUsesScreenEffects;
	BOOL bCanClearOnFree;
	BOOL bControlUnitOnly;
	BOOL bRequiresLoadedGraphics;
	BOOL bApplyToAppearanceOverride;
};

struct STATE_LIGHTING_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	char szSHCubemap[ MAX_XML_STRING_LENGTH ];
	int nSHCubemap;
	SH_COEFS_RGB<2>	tBaseSHCoefs_O2;		// sRGB
	SH_COEFS_RGB<3>	tBaseSHCoefs_O3;
	SH_COEFS_RGB<2>	tBaseSHCoefsLin_O2;		// linear
	SH_COEFS_RGB<3>	tBaseSHCoefsLin_O3;
};

//----------------------------------------------------------------------------
#include "..\data_common\excel\states_hdr.h"	// auto-generated by states.xls
struct STATE_EVENT
{
	int						eType;
	DWORD					dwFlags;
	float					fParam;
	int						nData;				// misc data-holder
	char					pszData[ MAX_XML_STRING_LENGTH ];
	ATTACHMENT_DEFINITION	tAttachmentDef;
	CONDITION_DEFINITION	tCondition;
	char					pszExcelString[ MAX_XML_STRING_LENGTH ];
	int						nExcelIndex;
};

//----------------------------------------------------------------------------
struct STATE_DEFINITION
{
	DEFINITION_HEADER					tHeader;
	STATE_EVENT *						pEvents;
	int									nEventCount;
};

//----------------------------------------------------------------------------
struct STATE_PULSE_SKILL
{
	EXCEL_STR							(szSkillPulse);							// string name of skill to pulse
	int									nSkillPulse;							// id of skill this state runs each pulse
};

//----------------------------------------------------------------------------
enum STATE_DATA_FLAG
{
	SDF_INVALID = -1,

	SDF_LOADED,
	SDF_STACKS,
	SDF_STACKS_PER_SOURCE,
	SDF_SEND_TO_ALL,
	SDF_SEND_TO_SELF,
	SDF_SEND_STATS,
	SDF_CLIENT_NEEDS_DURATION,
	SDF_CLIENT_ONLY,
	SDF_EXECUTE_PARENT_EVENTS,
	SDF_TRIGGER_NO_TARGET_EVENT_ON_SET,
	SDF_SAVE_POSITION_ON_SET,
	SDF_SAVE_WITH_UNIT,
	SDF_FLAG_FOR_LOAD,
	SDF_SHARING_MOD_STATE,
	SDF_USED_IN_HELLGATE,							// TRAVIS:-right now, since states are shared in data_common,
	SDF_USED_IN_TUGBOAT,							// we need a way to determine which is used in which - since they load media
													// and dump it in the pak, as well as reference specific skills.
													// this may not be the ideal way to do it - 
	SDF_IS_BAD,
	SDF_PULSE_ON_SOURCE,
	SDF_ON_CHANGE_REPAINT_ITEM_UI,
	SDF_SAVE_IN_UNITFILE_HEADER,
	SDF_PERSISTANT,
	SDF_UPDATE_CHAT_SERVER,							// when this state changes, update the chat server with new player data
	SDF_ONLY_STATE,									// set during postprocess if no other states isa this one
	SDF_TRIGGER_DIGEST_SAVE,						// trigger a character digest save when this state changes
			
	SDF_NUM_FLAGS				// keep this last please
};

//----------------------------------------------------------------------------
struct STATE_DATA
{
	EXCEL_STR							(szName);
	WORD								wCode;
	EXCEL_STR							(szEventFile);
	int									pnTypeEquiv[MAX_STATE_EQUIV];
	int									nStatePreventedBy;
	int									nDefaultDuration;
	int									nOnDeathState;
	int									nSkillScriptParam;
	int									nDefinitionId;
	int									nDamageTypeElement;
	PCODE								codePulseRateInMS;						// this state will "pulse" on the attached unit at this rate
	PCODE								codePulseRateInMSClient;
	STATE_PULSE_SKILL					tPulseSkill;							// this state runs this skill each pulse
	int									nIconOrder;
	EXCEL_STR							(szUIIconFrame);
	EXCEL_STR							(szUIIconTexture);
	int									nIconColor;
	EXCEL_STR							(szUIIconBackFrame);
	int									nIconBackColor;
	int									nIconTooltipStr[APP_GAME_NUM_GAMES];
	int									nStateAssoc[MAX_STATE_ASSSOC];			// associated states
	DWORD								dwFlagsScripts;
	enum GAME_VARIANT_TYPE				eGameVariant;
	DWORD								pdwFlags[DWORD_FLAG_SIZE(SDF_NUM_FLAGS)];
	SAFE_PTR							(DWORD *, pdwIsAStateMask);				// flag array of states which isa this one
};


#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
//this is a quick Hack(temp) for our Particle guy to test some stuff - should be removed. Put in june 5.2007
extern BOOL				gDisableXPVisuals;
#endif


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL ExcelStatesPostProcess(
	struct EXCEL_TABLE * table);

void ExcelStatesRowFree(
	EXCEL_TABLE * table,
	BYTE * rowdata);

BOOL ExcelStateEventTypePostProcess(
	struct EXCEL_TABLE * table);

void c_StateLightingDataEngineLoad();

BOOL StateDefinitionXMLPostLoad(
	void *pData,
	BOOL bForceSyncLoad);

void StateEventInit( 
	STATE_EVENT *pStateEvent);

void UnitInitStateFlags(
	UNIT * unit);

void UnitFreeStateFlags(
	UNIT * unit);

void UnitStatsChangeState(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const struct STATS_CALLBACK_DATA & data,
	BOOL bSend);

struct STATS * s_StateSet( 
	UNIT * pTarget, 
	UNIT * pSource, 
	int nState, 
	int nDuration,
	int nSkill = INVALID_ID,
	UNIT *pTransferUnitStats = NULL,
	UNIT * pItem = NULL,
	BOOL bDontExecuteSkillStats = FALSE);

struct STATS * c_StateSet( 
	UNIT * pTarget, 
	UNIT * pSource, 
	int nState, 
	WORD wStateIndex,
	int nDuration,
	int nSkill = INVALID_ID,
	BOOL bIgnorePreviousState = FALSE,
	BOOL bDontExecuteSkillStats = FALSE);

void c_StateSetPostProcessStats(
	GAME * game, 
	UNIT * target,
	UNIT * source, 
	STATS * stats,
	int nState,
	WORD wStateIndex,
	int nSkill);

void c_StateClear( 
	UNIT * pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	BOOL bAnySource = FALSE );

BOOL StateClearAllOfType( 
	UNIT * pUnit, 
	int nStateType );

void s_StateClear( 
	UNIT * pTarget, 
	UNITID idSource, 
	int nState, 
	WORD wStateIndex,
	int nSkill = INVALID_ID );

void c_StatesApplyAfterGraphicsLoad( 
	UNIT * pUnit );

void StatesShareModStates( 
	UNIT * pDest,
	UNIT * pSource );

int StateGetFirstOfType( 
	UNIT * pUnit, 
	int nStateType );

void c_StatesLoadAll( 
	GAME * pGame );

void c_StateLoad(
	GAME * pGame,
	int nStateId);

void c_StateFlagSoundsForLoad(
	GAME * pGame,
	int nState,
	BOOL bLoadNow);

void c_StatesFlagSoundsForLoad(
	GAME * pGame);

void StatesInit( 
	GAME * pGame );

void c_StateAddTarget( 
	GAME * pGame,
	struct MSG_SCMD_STATEADDTARGET * pMsg );

void c_StateRemoveTarget( 
	GAME * pGame,
	struct MSG_SCMD_STATEREMOVETARGET * pMsg );

void c_StateAddTargetLocation( 
	GAME * pGame,
	struct MSG_SCMD_STATEADDTARGETLOCATION * pMsg );

STATS* GetStatsForStateGivenByUnit( int nState, 
								   UNIT *unit,
								   UNITID idSource,
								   WORD wIndex = 0,
								   BOOL bAnySource = FALSE,
								   int nSkill = INVALID_ID );

void StateRunScriptsOnUnitStates( UNIT *pUnit,
								  UNIT *pTarget,
								  int ScriptFlags, /*STATE_SCRIPTS_FLAGS*/
								  STATS *pStats );
							  
						
STATS * StateSetAttackStart( UNIT *pAttacker,
							 UNIT *pTarget,
							 UNIT *pWeapon,
							 BOOL bMelee );

void StateClearAttackState( UNIT *pUnit );

BOOL UnitHasState( 
	GAME * pGame,
	UNIT * pUnit,
	int nState );

int UnitGetFirstStateOfType( 
	GAME * pGame,
	UNIT * pUnit,
	int nState );

int UnitGetOnDeathState( 
	GAME * pGame,
	UNIT * pUnit );

void s_StateAddTarget( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	UNIT * pTarget, 
	int nState,
	WORD wStateIndex );

void s_StateAddTargetLocation( 
	GAME * pGame,
	UNITID idStateSource, 
	UNIT * pUnit, 
	const VECTOR & vTarget, 
	int nState,
	WORD wStateIndex );

void s_StatesSendAllToClient(
	GAME * pGame,
	UNIT * pUnit,
	GAMECLIENTID idClient );

void c_StateDoAllEventsOnWeapons( 
	UNIT* pUnit,
	BOOL pbDoEventsOnWeapon[ MAX_WEAPONS_PER_UNIT ] );

// Hammer Only
void StateDoEvents( 
	int nModelId,
	int nState,
	WORD wStateIndex,
	BOOL bHadState,
	BOOL bFirstPerson );

// Hammer Only
void StateClearEvents( 
	int nModelId,
	int nStateId,
	WORD wStateIndex,
	BOOL bFirstPerson );

BOOL UnitHasExactState( 
	UNIT * unit,
	int state);

void StateMonitorStat(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend);

BOOL StateIsBad(
	int nState);

BOOL s_StateClearBadStates( 
	UNIT *pUnit);

BOOL c_StateEventIsHolyRadius(
	const STATE_EVENT_TYPE_DATA *pStateEventTypeData);

void StateArrayEnable(
	UNIT *pUnit,
	const int *pnStateArray,
	int nArraySize,
	int nDuration,
	BOOL bIsClientOnly);

GAME_VARIANT_TYPE StateGetGameVariantType(
	GAME *pGame,
	int nState);

void StateArrayGetGameVariantFlags( 
	int *pStates,
	int nMaxStates,
	DWORD *pdwGameVariantFlags);

typedef void FN_SAVE_STATE_ITERATOR( UNIT *pUnit, int nState, void *pCallbackData );

void StateIterateSaved(
	UNIT *pUnit,
	FN_SAVE_STATE_ITERATOR *pfnCallback,
	void *pCallbackData);
	

void StateRegisterUnitScale( UNIT *pUnit,
							 int nAppearanceChange,
							 float fCurrent,
							 float fFinal,
							 int nTicks,
							 DWORD dwFlags );
//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
inline const STATE_DATA * StateGetData(
	GAME * game,
	int state)
{
	const STATE_DATA * state_data = (const STATE_DATA *)ExcelGetData(game, DATATABLE_STATES, state);
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	return state_data;
}

//----------------------------------------------------------------------------
inline BOOL StateDataTestFlag(
	const STATE_DATA *pStateData,
	STATE_DATA_FLAG eFlag)
{
	ASSERTV_RETFALSE( pStateData, "Expected state data" );
	return TESTBIT( pStateData->pdwFlags, eFlag );
}

//----------------------------------------------------------------------------
inline unsigned int StatesGetCount(
	GAME * game)
{
	return ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_STATES);
}


//----------------------------------------------------------------------------
inline BOOL StateIsA( 
	int stateA,
	int stateB)
{
	return ExcelTestIsA(EXCEL_CONTEXT(), DATATABLE_STATES, stateA, stateB);
}


//----------------------------------------------------------------------------
inline const STATE_DEFINITION * StateGetDefinition( 
	int state)
{
	const STATE_DATA * state_data = StateGetData(NULL, state);
	// TRAVIS: now that we use VERSION_APP - we can have NULL state data
	if (!state_data || state_data->nDefinitionId == INVALID_ID)
	{
		return NULL;
	}

	return (const STATE_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_STATES, state_data->nDefinitionId);
}


//----------------------------------------------------------------------------
inline const STATE_EVENT_TYPE_DATA * StateEventTypeGetData(
	GAME * game,
	int stateEventType)
{
	return (const STATE_EVENT_TYPE_DATA *)ExcelGetData(game, DATATABLE_STATE_EVENT_TYPES, stateEventType);
}


#endif // _STATES_H_