//----------------------------------------------------------------------------
// uix_debug.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_DEBUG_H_
#define _UIX_DEBUG_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _COLORS_H_
#include "colors.h"
#endif


//----------------------------------------------------------------------------
// FORWARDS
//----------------------------------------------------------------------------
struct UI_COMPONENT;
enum UI_MSG_RETVAL;

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UIDEBUG_PING,
	UIDEBUG_FPS,
	UIDEBUG_BATCHES,
	UIDEBUG_STATES,
	UIDEBUG_PARTICLESTATS,
	UIDEBUG_ENGINESTATS,
	UIDEBUG_HASHMETRICS,
	UIDEBUG_SYNCH,
	UIDEBUG_MEMORY,
	UIDEBUG_MEMORY_POOL_INFO,
	UIDEBUG_ROOMDEBUG,
	UIDEBUG_MODELDEBUG,
	UIDEBUG_SOUND,
	UIDEBUG_EVENTS_SRV,
	UIDEBUG_EVENTS_CLT,
	UIDEBUG_EVENTS_UI,
	UIDEBUG_PERFORMANCE,
	UIDEBUG_HITCOUNT,
	UIDEBUG_STATS,
	UIDEBUG_VIRTUALSOUND,
	UIDEBUG_BARS,
	UIDEBUG_MUSIC,
	UIDEBUG_PARTICLES,
	UIDEBUG_BGSOUND,
	UIDEBUG_UI,
	UIDEBUG_UIGFX,
	UIDEBUG_APPEARANCE,
	UIDEBUG_DPS,
	UIDEBUG_SKILL_SRV,
	UIDEBUG_SKILL_CLT,
	UIDEBUG_SOUNDMEMORY,
	UIDEBUG_UIEDIT,
	UIDEBUG_STOPPEDSOUND,
	UIDEBUG_LAYOUTROOMDISPLAY,
	UIDEBUG_DPVS,
	UIDEBUG_MUSICSCRIPT,
	UIDEBUG_REVERB,
	UIDEBUG_SOUNDBUSES,
	UIDEBUG_SOUNDMIXSTATES,
	UIDEBUG_TAGDISPLAY,

	UIDEBUG_LAST
};

void UITestingToggle();

void UIResetDPSTimer();

UI_MSG_RETVAL UIToggleDebugDisplay(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

BOOL UIDebugDisplayGetStatus(
	DWORD eDebugDisplay);

#endif