//----------------------------------------------------------------------------
// skills_shift.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SKILLS_SHIFT_H_
#define _SKILLS_SHIFT_H_

typedef enum SKILL_ACTIVATOR_KEY {
	SKILL_ACTIVATOR_KEY_INVALID = -1,
	SKILL_ACTIVATOR_KEY_SHIFT = 0,
	SKILL_ACTIVATOR_KEY_POTION,
	SKILL_ACTIVATOR_KEY_MELEE,

	NUM_SKILL_ACTIVATOR_KEYS,
};


void c_SkillsActivatorKeyDown( 
	SKILL_ACTIVATOR_KEY eKey );
void c_SkillsActivatorKeyUp(
	SKILL_ACTIVATOR_KEY eKey );

void SkillsActivatorsInitExcelPostProcess(
	struct EXCEL_TABLE * table);

int c_SkillsFindActivatorSkill(
	SKILL_ACTIVATOR_KEY eKey );

UNIT * c_SkillsFindActivatorItem(
	SKILL_ACTIVATOR_KEY eKey );

#endif