#pragma once
using namespace std;
#define PETS_EVENT_TYPE_ExperienceSavedEvent	11
#define PETS_EVENT_TYPE_LevelUpEvent	12
#define PETS_EVENT_TYPE_SkillKnownEvent	13
#define PETS_EVENT_TYPE_KilledByMonsterEvent	14
#define PETS_EVENT_TYPE_KilledByPlayerEvent	15

void TrackEvent_ExperienceSavedEvent(
	INT32 experience,
	string playername,
	ODBCINT64 guid);
void TrackEvent_LevelUpEvent(
	ODBCINT64 guid,
	INT32 level,
	INT32 rank);
void TrackEvent_SkillKnownEvent(
	ODBCINT64 guid_player,
	INT32 code_skill,
	string name_skill,
	INT32 level_player,
	INT32 rank_player,
	INT32 level_skill);
void TrackEvent_KilledByMonsterEvent(
	ODBCINT64 guid_player,
	INT32 level_player,
	INT32 rank_player,
	INT32 code_monster,
	string name_monster,
	INT32 level_monster,
	INT32 quality_monster);
void TrackEvent_KilledByPlayerEvent(
	ODBCINT64 guid_victim,
	INT32 level_victim,
	INT32 rank_victim,
	ODBCINT64 guid_attacker,
	INT32 level_attacker,
	INT32 rank_attacker,
	INT32 context);

