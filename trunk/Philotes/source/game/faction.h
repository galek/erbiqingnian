//----------------------------------------------------------------------------
// FILE: faction.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __FACTION_H_
#define __FACTION_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct DATA_TABLE;
enum OPERATE_RESULT;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum FACTION_CONSTANTS
{
	FACTION_SCORE_NONE = -1,		// value for faction score meaning no faction score set at all
	FACTION_MAX_INIT = 4,			// arbitrary
};

//----------------------------------------------------------------------------
enum FACTION_STANDING_MOOD
{
	FSM_INVALID = -1,
	
	FSM_MOOD_BAD,			// this standing is a 'bad' standing
	FSM_MOOD_NEUTRAL,		// this standing is a 'neutral' standing
	FSM_MOOD_GOOD,			// this standing is a 'good' standing
	
};

//----------------------------------------------------------------------------
struct FACTION_INIT
{
	int nUnitType;
	int nLevelDef;
	int nFactionStanding;
};

//----------------------------------------------------------------------------
struct FACTION_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];	// name
	WORD wCode;							// code
	int	nDisplayString;					// link to string table entry
	FACTION_INIT tFactionInit[ FACTION_MAX_INIT ];	// init standings for unit types and this faction
};

//----------------------------------------------------------------------------
struct FACTION_STANDING_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];	// name
	WORD wCode;							// code
	int	nDisplayString;					// link to string table entry	
	int	nDisplayStringNumbers;			// link to string table entry that has space for the actual numbers
	int nMinScore;						// min faction point score required for this standing
	int nMaxScore;						// max faction point score in this standing 
	FACTION_STANDING_MOOD eMood;		// view of this standing (ie, is it good, bad, or neutral)
};

//----------------------------------------------------------------------------
/// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

BOOL FactionExcelPostProcess(
	EXCEL_TABLE * table);

BOOL FactionStandingExcelPostProcess(
	EXCEL_TABLE * table);

const WCHAR *FactionGetDisplayName(
	int nFaction);

const WCHAR *FactionStandingGetDisplayName(
	int nFactionStanding);

int FactionGetNumFactions(
	void);

const FACTION_DEFINITION *FactionGetDefinition(
	int nFaction);
	
int FactionGetNumStandings(
	void);

const FACTION_STANDING_DEFINITION *FactionStandingGetDefinition(
	int nFactionStanding);
			
int FactionGetStanding(
	UNIT *pUnit,
	int nFaction);

FACTION_STANDING_MOOD FactionGetMood(
	UNIT *pUnit,
	int nFaction);

BOOL FactionStandingIsBetterThan(
	int nFactionStanding,
	int nFactionStandingOther);

BOOL FactionStandingIsWorseThan(
	int nFactionStanding,
	int nFactionStandingOther);

FACTION_STANDING_MOOD FactionStandingGetMood(
	int nFactionStanding);

int FactionScoreGet(
	UNIT *pUnit,
	int nFaction);
	
void FactionScoreChange(
	UNIT *pUnit,
	int nFaction,
	int nScoreDelta);

void FactionGiveInitialScores( 
	UNIT *pUnit);

void FactionDebugString(
	int nFaction,
	int nScore,
	WCHAR *puszBuffer,
	int nBufferSize);
		
int FactionGetTitle(
	UNIT * unit );

const WCHAR * FactionScoreToDecoratedStandingName(
	int nFactionScore);

OPERATE_RESULT FactionCanOperateObject(
	UNIT * unit,
	UNIT * object );

//----------------------------------------------------------------------------
// CLIENT FUNCTIONS
//----------------------------------------------------------------------------

void c_FactionScoreChanged( 
	UNIT *pUnit,
	int nFaction, 
	int nOldValue,
	int nNewValue);
	
#endif
