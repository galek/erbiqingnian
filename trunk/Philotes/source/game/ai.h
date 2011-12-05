//----------------------------------------------------------------------------
// ai.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _AI_H_
#define _AI_H_

#include "../data_common/excel/aicommon_state_hdr.h"

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL AI_Update(
	struct GAME* game,
	struct UNIT* unit,
	const struct EVENT_DATA& event_data);

void AI_Init(
	struct GAME* game,
	struct UNIT* unit);

void AI_Free(
	struct GAME* pGame,
	struct UNIT* pUnit);

BOOL AI_ToggleOutputToConsole(
	void);

BOOL AI_Register(
	struct UNIT* pUnit,
	BOOL bRandomPeriod,
	int nForcePeriod = -1 );

BOOL AI_Unregister(
	struct UNIT* pUnit );

BOOL AI_InsertTable(
	UNIT * pUnit,
	int nAIDefinition,
	struct STATS * pStatsForIds = NULL,
	int nTimeout = INVALID_ID);

BOOL AI_InsertTable(
	UNIT * pUnit,
	const struct AI_BEHAVIOR_DEFINITION_TABLE * pTable,
	struct STATS * pStatsForIds = NULL,
	int nTimeout = INVALID_ID);

void AI_RemoveTableByStats(
	UNIT * pUnit,
	struct STATS * pStatsWithIds );

void AI_AngerOthers(
	GAME* pGame,
	UNIT* defender,
	UNIT* attacker,
	int nRange = -1);

enum GET_AI_TARGET_FLAGS
{
	GET_AI_TARGET_IGNORE_OVERRIDE_BIT,
	GET_AI_TARGET_ONLY_OVERRIDE_BIT,
	GET_AI_TARGET_FORCE_CLIENT_BIT,
};

UNIT * UnitGetAITarget(
	UNIT * pUnit,
	DWORD dwFlags = 0);

UNITID UnitGetAITargetId(
	UNIT * pUnit,
	DWORD dwFlags = 0);

void UnitSetAITarget(
	UNIT * pUnit,
	UNITID idTarget,
	BOOL bOverrideTarget = FALSE,
	BOOL bSendToClient = FALSE);

void UnitSetAITarget(
	UNIT * pUnit,
	UNIT * pTarget,
	BOOL bOverrideTarget = FALSE,
	BOOL bSendToClient = FALSE);

void c_AIDataLoad(
	GAME * pGame,
	int nAI);

void c_AIFlagSoundsForLoad(
	GAME * pGame,
	int nAI,
	BOOL bLoadNow);

#endif // _AI_H_