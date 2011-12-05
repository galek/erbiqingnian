//----------------------------------------------------------------------------
// debugbars_def.h
//
// - Header for debug ui bar sets
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef INCLUDE_DEBUGBARS_ENUM
#define DEFINE_BAR_SET(codename,szName,pfnUpdate) codename,
#else
#define DEFINE_BAR_SET(codename,szName,pfnUpdate) { szName, pfnUpdate },
#endif

DEFINE_BAR_SET( DEBUG_BARS_GENERAL,					"general",					sDebugBarUpdateGeneral )
DEFINE_BAR_SET( DEBUG_BARS_PARTICLE,				"particle",					sDebugBarUpdateParticle )
DEFINE_BAR_SET( DEBUG_BARS_BACKGROUND,				"background",				sDebugBarUpdateBackground )
DEFINE_BAR_SET( DEBUG_BARS_TIMING,					"timing",					sDebugBarUpdateTiming )		// CHB 2006.08.14

#undef DEFINE_BAR_SET
