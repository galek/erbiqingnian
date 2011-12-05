//-----------------------------------------------------------------------------
//
// End-user configurable and persistent graphics and performance settings.
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _GFXOPTIONS_H_
#define _GFXOPTIONS_H_

#if !ISVERSION(SERVER_VERSION)

class SettingsExchange;
class OptionState;
class FeatureLineMap;

void GfxOptions_Load(void);
void GfxOptions_Save(void);
void GfxOptions_Save(const OptionState * pState, const FeatureLineMap * pMap);
void GfxOptions_SaveDefaults(void);		// CHB 2007.10.22
void GfxOptions_GetDefaults(OptionState * * ppStateOut, FeatureLineMap * * ppMapOut);	// CHB 2007.10.22
void GfxOptions_Deinit(void);	// CHB 2007.10.22 - needed to free defaults

#else

#define GfxOptions_Load()				((void)0)
#define GfxOptions_Save()				((void)0)

#endif

#endif
