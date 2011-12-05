//----------------------------------------------------------------------------
// FILE: s_questsave.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_QUEST_SAVE_H_
#define __S_QUEST_SAVE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct QUEST;
enum QUEST_STATE_VALUE;
struct QUEST_DEFINITION;
enum STAT_VERSION_RESULT;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

int QuestDataGetCurrentVersion(
	void);

BOOL s_QuestVersionData(
	QUEST *pQuest);

BOOL s_QuestsWriteToStats(
	UNIT *pPlayer);

BOOL s_QuestLoadFromStats( 
	UNIT *pPlayer,
	BOOL bFirstGameJoin);

void s_QuestRestoreToState( 
	QUEST *pQuest, 
	int nState,
	QUEST_STATE_VALUE nStateValue);

STAT_VERSION_RESULT s_VersionStatQuestSaveState(
	struct STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit);

#endif  // end __S_QUEST_SAVE_H_