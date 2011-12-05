//----------------------------------------------------------------------------
// FILE: QuestProperties.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "QuestStructs.h"
#include "Quest.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "QuestObjectSpawn.h"
#include "QuestMonsterSpawn.h"
#include "unittag.h"
#include "script.h"
#include "script_priv.h"
#include "unittypes.h"
#include "s_QuestServer.h"
#include "c_QuestClient.h"
#include "s_quests.h"
#include "excel_private.h"
#include "skills.h"
#include "Player.h"
#include "room.h"

int MAX_ACTIVE_QUESTS_PER_UNIT = 1; //this gets changed in the function QuestCountForTugboatExcelPostProcess

//lets just make this an array
int g_QuestStatIDs[ 5 ] = { STATS_QUEST_FIRST_QUEST, 
													 STATS_QUEST_SECOND_QUEST,
													 STATS_QUEST_THIRD_QUEST,
													 STATS_QUEST_FOURTH_QUEST,
													 STATS_QUEST_FIFTH_QUEST };

int g_QuestTaskSaveIDs[ 5 ] = { STATS_QUESTTASK_FIRST_QUEST, 
														 STATS_QUESTTASK_SECOND_QUEST,
														 STATS_QUESTTASK_THIRD_QUEST,
														 STATS_QUESTTASK_FOURTH_QUEST,
														 STATS_QUESTTASK_FIFTH_QUEST };


