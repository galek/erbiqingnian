//-----------------------------------------------------------------------------
//
// System for storage and retrieval of persistent user settings.
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "settings.h"

#include "gfxoptions.h"
#include "gameoptions.h"
#include "controloptions.h"
#include "c_sound_util.h"

#if !ISVERSION(SERVER_VERSION)

static
void Settings_SaveDefaults(void)
{
	GfxOptions_SaveDefaults();
}

// This function gets its own file to isolate dependencies.
void Settings_LoadAll(void)
{
	// CHB 2007.10.22 - Before loading the user's state,
	// save the current state as the default state.
	Settings_SaveDefaults();

	GfxOptions_Load();
	GameOptions_Load();
	ControlOptions_Load();
	c_SoundLoadSoundConfigOptions();
	c_SoundLoadMusicConfigOptions();
}

#endif