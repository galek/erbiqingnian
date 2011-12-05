//----------------------------------------------------------------------------
// FILE: quest_tutorial.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TUTORIAL_H_
#define __QUEST_TUTORIAL_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct UNIT_DATA;
struct UNIT;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

enum
{
	TUTORIAL_INPUT_VIDEO_ENTER = 0,
	TUTORIAL_INPUT_VIDEO_CLOSE,
	TUTORIAL_INPUT_QUESTLOG,
	TUTORIAL_INPUT_QUESTLOG_CLOSE,
	TUTORIAL_INPUT_AUTOMAP,
	TUTORIAL_INPUT_MOVE_FORWARD,
	TUTORIAL_INPUT_MOVE_SIDES,
	TUTORIAL_INPUT_MOVE_BACK,
	TUTORIAL_INPUT_MOUSE_MOVE,
	TUTORIAL_INPUT_SPACEBAR,
	TUTORIAL_INPUT_LEFTBUTTON,
	TUTORIAL_INPUT_FIRST_KILL,
	TUTORIAL_INPUT_FIRST_ITEM,
	TUTORIAL_INPUT_INVENTORY,
	TUTORIAL_INPUT_INVENTORY_CLOSE,
	TUTORIAL_INPUT_SKILLS,
	TUTORIAL_INPUT_SKILLS_CLOSE,
	TUTORIAL_INPUT_APPROACH_MURMUR,
	TUTORIAL_INPUT_TALK_MURMUR,
	TUTORIAL_INPUT_APPROACH_KNIGHT,
	TUTORIAL_INPUT_OFFER_DONE,
	TUTORIAL_INPUT_WORLDMAP,
	TUTORIAL_INPUT_WORLDMAP_CLOSE,
	TUTORIAL_INPUT_APPROACH_PORTAL,

	NUM_TUTORIAL_INPUTS,

	TUTORIAL_INPUT_NONE = -1,
};

enum
{
	TUTORIAL_MSG_INPUT = 0,
	TUTORIAL_MSG_TIP_DONE,
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTutorial(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTutorial(
	const QUEST_FUNCTION_PARAM &tParam);

void c_TutorialSetClientMessage( UNIT * player, int tip_index );
void c_TutorialInputOk( UNIT * player, int input );
void s_TutorialTipsOff( UNIT * player );

void s_TutorialUpdate( UNIT * player, int type, int data );

#endif
