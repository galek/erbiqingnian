//----------------------------------------------------------------------------
// e_caps_def.h
//
// - Header for capabilities definitions/specifications
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

// NOTE: These caps are a generalized superset of platform capabilities.
// The idea is that platforms detect and fill these in as is most appropriate to
// that platform's needs.


#ifdef INCLUDE_CAPS_ENUM
#define DEFINE_CAP(name,fourcc,desc,wantsmaller) name,
#else
// { code name, string name, smaller value is better }
#define DEFINE_CAP(name,fourcc,desc,wantsmaller) { MAKEFOURCC(fourcc[0],fourcc[1],fourcc[2],fourcc[3]), desc, wantsmaller },
#endif

DEFINE_CAP( CAP_PHYSICAL_VIDEO_MEMORY_MB,				"PHVM",	"Physical video memory (MB)",					FALSE )
DEFINE_CAP( CAP_TOTAL_ESTIMATED_VIDEO_MEMORY_MB,		"TOVM",	"Total est. video memory incl. AGP (MB)",		FALSE )
DEFINE_CAP( CAP_SYSTEM_MEMORY_MB,						"SYSM",	"System memory (MB)",							FALSE )
DEFINE_CAP( CAP_SYSTEM_CLOCK_SPEED_MHZ,					"CLCK",	"System clock speed (MHz)",						FALSE )
DEFINE_CAP( CAP_CPU_SPEED_MHZ,							"CPUS",	"CPU speed (MHz)",								FALSE )
DEFINE_CAP( CAP_NATIVE_RESOLUTION_WIDTH,				"DSPW",	"Native display resolution",					FALSE )
DEFINE_CAP( CAP_NATIVE_RESOLUTION_HEIGHT,				"DSPH",	"Native display resolution",					FALSE )

#undef DEFINE_CAP