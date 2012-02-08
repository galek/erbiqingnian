
#ifndef RENDERER_CONFIG_H
#define RENDERER_CONFIG_H

#include <assert.h>

#define RENDERER_TEXT(_foo)   #_foo
#define RENDERER_TEXT2(_foo)  RENDERER_TEXT(_foo)

extern char gShadersDir[];

// number of lights required before it switches from forward rendering to deferred rendering.
#define RENDERER_DEFERRED_THRESHOLD  0x7FFFFFFF // set to a big number just to disable it for now...

// Enables/Disables support for dresscode in the renderer...
#define RENDERER_ENABLE_DRESSCODE 0

// If turned on, asserts get compiled in as print statements in release mode.
#define RENDERER_ENABLE_CHECKED_RELEASE 0

// maximum number of bones per-drawcall allowed.
#define RENDERER_MAX_BONES 60

#define RENDERER_TANGENT_CHANNEL            5
#define RENDERER_BONEINDEX_CHANNEL          6
#define RENDERER_BONEWEIGHT_CHANNEL         7
#define RENDERER_INSTANCE_POSITION_CHANNEL  8
#define RENDERER_INSTANCE_NORMALX_CHANNEL   9
#define RENDERER_INSTANCE_NORMALY_CHANNEL  10
#define RENDERER_INSTANCE_NORMALZ_CHANNEL  11
#define RENDERER_INSTANCE_VEL_LIFE_CHANNEL 12
#define RENDERER_INSTANCE_DENSITY_CHANNEL  13

// windows∆ΩÃ®≈‰÷√
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4100) // unreferenced formal parameter

#define RENDERER_VISUALSTUDIO
#define RENDERER_WINDOWS
#define RENDERER_ENABLE_DIRECT3D9
#define RENDERER_ENABLE_CG
#define RENDERER_ENABLE_NVPERFHUD
#define RENDERER_ENABLE_TGA_SUPPORT

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#if defined(WIN64)
#define RENDERER_64BIT
#endif

#if defined(_DEBUG)
	#define RENDERER_DEBUG
#endif

#define RENDERER_OUTPUT_MESSAGE(_rendererPtr, _msg) printf(_msg)

#if RENDERER_ENABLE_DRESSCODE
	#include <dresscode.h>
	#define RENDERER_PERFZONE(_name) dcZone(#_name);
#else

#ifdef AGPERFMON
#include "AgPerfMonEventSrcAPI.h"
	#define RENDERER_PERFZONE(_name) SAMPLE_PERF_SCOPE(_name)
#else
#define RENDERER_PERFZONE(_name)
#endif

#endif

#ifdef RENDERER_XBOX360
#define RENDERER_INSTANCING 0
#else
#define RENDERER_INSTANCING 1
#endif


#endif
