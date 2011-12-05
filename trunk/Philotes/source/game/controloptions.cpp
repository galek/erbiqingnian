//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)

#include "controloptions.h"
#include "settings.h"
#include "uix_priv.h"
#include "c_input.h"
#include "config.h"

namespace
{

const char CONTROLOPTIONS_NAME[] = "Control";

CONTROLOPTIONS sOptions;

//-----------------------------------------------------------------------------

bool ControlOptions_SetNoSave(const CONTROLOPTIONS & OptsIn)
{
	CONTROLOPTIONS tOpts = OptsIn;

	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		KEY_COMMAND& key = InputGetKeyCommand(i);
		if (key.bRemappable && key.szCommandStr)
		{
			key.nCommand = OptsIn.pKeyAssignments[i].nCommand;

			for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
			{
				key.KeyAssign[j] = OptsIn.pKeyAssignments[i].KeyAssign[j];
			}
		}
	}

	tOpts.Validate();

	bool bDiffer = (sOptions != tOpts);
	sOptions = tOpts;

	return bDiffer;
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

void ControlOptions_Get(CONTROLOPTIONS & OptsOut)
{
	OptsOut = sOptions;
}

//-----------------------------------------------------------------------------

void ControlOptions_Set(const CONTROLOPTIONS & OptsIn)
{
	if (ControlOptions_SetNoSave(OptsIn))
	{
		ControlOptions_Save();
	}
}

//-----------------------------------------------------------------------------

CONTROLOPTIONS::CONTROLOPTIONS(void)
{
//	pKeyAssignments = (KEY_COMMAND_SAVE *)MALLOCZ(NULL, sizeof(KEY_COMMAND_SAVE) * InputGetKeyCommandCount());

	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		const KEY_COMMAND& key = InputGetKeyCommand(i);
//		if (key.bRemappable && key.szCommandStr)
		{
			pKeyAssignments[i].nCommand = key.nCommand;

			for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
			{
				pKeyAssignments[i].KeyAssign[j] = key.KeyAssign[j];
			}
		}
	}

	bShiftAsModifier = FALSE;
	bCtrlAsModifier = FALSE;
	bAltAsModifier = FALSE;
}

CONTROLOPTIONS::~CONTROLOPTIONS(void)
{
	//if (pKeyAssignments)
	//	FREE(NULL, pKeyAssignments);
}

static
void sExchangeKey(SettingsExchange & se, void * pContext)
{
	KEY_COMMAND *pKey = static_cast<KEY_COMMAND *>(pContext);

	for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
	{
		SettingsExchange_Do(se, pKey->KeyAssign[j].nKey, "key");
		SettingsExchange_Do(se, pKey->KeyAssign[j].nModifierKey, "mod");
	}
}


void CONTROLOPTIONS::Exchange(SettingsExchange & se)
{
	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		KEY_COMMAND& key = InputGetKeyCommand(i);
		if (key.bRemappable && key.szCommandStr)
		{
			if (SettingsExchange_Category(se, key.szCommandStr))
			{
				for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
				{
					SettingsExchange_Do(se, pKeyAssignments[i].KeyAssign[j].nKey, "key");
					SettingsExchange_Do(se, pKeyAssignments[i].KeyAssign[j].nModifierKey, "mod");
				}
				SettingsExchange_CategoryEnd(se);
			}
		}
	}

	if (SettingsExchange_Category(se, "UseAsModifier"))
	{
		SettingsExchange_Do(se, this->bShiftAsModifier, "shift");
		SettingsExchange_Do(se, this->bCtrlAsModifier, "ctrl");
		SettingsExchange_Do(se, this->bAltAsModifier, "alt");
		SettingsExchange_CategoryEnd(se);
	}
}

void CONTROLOPTIONS::Validate(void)
{
	// nothing to validate yet...
}

bool CONTROLOPTIONS::operator==(const CONTROLOPTIONS & rhs) const
{
	return (memcmp(pKeyAssignments, rhs.pKeyAssignments, sizeof(KEY_COMMAND_SAVE) * NUM_KEY_COMMANDS) == 0)
		&& (bShiftAsModifier == rhs.bShiftAsModifier)
		&& (bCtrlAsModifier == rhs.bCtrlAsModifier)
		&& (bAltAsModifier == rhs.bAltAsModifier);
}

bool CONTROLOPTIONS::operator!=(const CONTROLOPTIONS & rhs) const
{
	return !(*this == rhs);
}

void CONTROLOPTIONS::SetToDefault(void)
{
	KEY_COMMAND pDefaultKeyAssignments[] = 
	{
		#include "key_command_priv.h"
	};

	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		KEY_COMMAND& key = pDefaultKeyAssignments[i];
		pKeyAssignments[i].nCommand = key.nCommand;

		for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
		{
			pKeyAssignments[i].KeyAssign[j] = key.KeyAssign[j];
		}
	}

	bShiftAsModifier = FALSE;
	bCtrlAsModifier = FALSE;
	bAltAsModifier = FALSE;
}

//-----------------------------------------------------------------------------

static
void sExchangeFunc(SettingsExchange & se, void * pContext)
{
	static_cast<CONTROLOPTIONS *>(pContext)->Exchange(se);
}

void ControlOptions_Load(void)
{
	CONTROLOPTIONS tOpts;
	if (FAILED(Settings_Load(CONTROLOPTIONS_NAME, &sExchangeFunc, &tOpts)))
	{
		// Nothing to do.
	}
	else
	{
		ControlOptions_SetNoSave( tOpts );
	}
}

void ControlOptions_Save(void)
{
	if (FAILED(Settings_Save(CONTROLOPTIONS_NAME, &sExchangeFunc, &sOptions)))
	{
		//???
	}
}

//-----------------------------------------------------------------------------

#endif
