//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _CONTROLOPTIONS_H_
#define _CONTROLOPTIONS_H_

#if !ISVERSION(SERVER_VERSION)

#include "c_input.h"

class SettingsExchange;

struct KEY_COMMAND_SAVE
{
	int				nCommand;
	KEY_ASSIGNMENT	KeyAssign[NUM_KEYS_PER_COMMAND];
};

struct CONTROLOPTIONS
{
	KEY_COMMAND_SAVE pKeyAssignments[NUM_KEY_COMMANDS];
	BOOL bShiftAsModifier;
	BOOL bCtrlAsModifier;
	BOOL bAltAsModifier;

	CONTROLOPTIONS(void);
	~CONTROLOPTIONS(void);
	void Exchange(SettingsExchange & se);
	void Validate(void);
	void SetToDefault();
	bool operator==(const CONTROLOPTIONS & rhs) const;
	bool operator!=(const CONTROLOPTIONS & rhs) const;
};

void ControlOptions_Get(CONTROLOPTIONS & OptsOut);
void ControlOptions_Set(const CONTROLOPTIONS & Opts);
void ControlOptions_Load(void);
void ControlOptions_Save(void);

#else

#define ControlOptions_Get(x)				((void)0)
#define ControlOptions_Set(x)				((void)0)
#define ControlOptions_Load()				((void)0)
#define ControlOptions_Save()				((void)0)

#endif

#endif
