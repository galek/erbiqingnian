//----------------------------------------------------------------------------
//	condition.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _CONDITION_H_
#define _CONDITION_H_

struct CONDITION_PARAM
{
	int nValue;
	float fValue;
};

#define MAX_CONDITION_PARAMS 2

enum
{
	CONDITION_BIT_CHECK_OWNER,
	CONDITION_BIT_CHECK_WEAPON,
	CONDITION_BIT_CHECK_TARGET,
	CONDITION_BIT_NOT_DEAD_OR_DYING,
	CONDITION_BIT_IS_YOUR_PLAYER,
	CONDITION_BIT_OWNER_IS_YOUR_PLAYER,
	CONDITION_BIT_CHECK_STATE_SOURCE,

	NUM_CONDITION_FLAGS,
};

struct CONDITION_DEFINITION
{
	int nType;
	int nState;
	int nUnitType;
	int nSkill;
	int nMonsterClass;
	int nObjectClass;
	int nStat;
	DWORD dwFlags[DWORD_FLAG_SIZE(NUM_CONDITION_FLAGS)];
	CONDITION_PARAM tParams[ MAX_CONDITION_PARAMS ];
};

typedef BOOL (* CONDITION_FUNCTION)( struct CONDITION_INPUT & tInput );

#define MAX_CONDITION_FUNCTION_NAME_LEN 32
#define MAX_CONDITION_PARAM_STRING_LENGTH 32
struct CONDITION_FUNCTION_DATA
{
	char szName[ DEFAULT_INDEX_SIZE	];
	char szFunction[ MAX_CONDITION_FUNCTION_NAME_LEN ];
	SAFE_FUNC_PTR(CONDITION_FUNCTION, pfnHandler);
	BOOL bNot;
	BOOL bUsesState;
	BOOL bUsesUnitType;
	BOOL bUsesSkill;
	BOOL bUsesMonsterClass;
	BOOL bUsesObjectClass;
	BOOL bUsesStat;
	char pszParamText[ MAX_CONDITION_PARAMS ][ MAX_CONDITION_PARAM_STRING_LENGTH ];
};

BOOL ConditionFunctionsExcelPostProcess( 
	struct EXCEL_TABLE * table);

BOOL ConditionEvalute(
	struct UNIT * pUnit,
	const CONDITION_DEFINITION & tDef,
	int nSkill = INVALID_ID,
	UNITID idWeapon = INVALID_ID,
	struct STATS * pStatsForState = NULL,
	UNITTYPE * pnUnitType = NULL );

void ConditionDefinitionInit(
	CONDITION_DEFINITION & tDef );

void ConditionDefinitionPostLoad(
	CONDITION_DEFINITION & tDef );



#endif // _CONDITION_H_

