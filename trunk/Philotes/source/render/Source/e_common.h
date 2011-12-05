//----------------------------------------------------------------------------
// e_common.h
//
// - Header for common engine code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_COMMON__
#define __E_COMMON__

//#include "interpolationpath.h"
#include "presult.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9
#	undef ENGINE_TARGET_NAME
#	define ENGINE_TARGET_NAME				_T("DX9")
#endif

#ifdef ENGINE_TARGET_DX10
#	undef ENGINE_TARGET_NAME
#	define ENGINE_TARGET_NAME				_T("DX10")
#endif

#ifndef ENGINE_TARGET_NAME
#	define ENGINE_TARGET_NAME				_T("")
#endif

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define VECTOR_MAKE_FROM_ARGB(dwCol)		VECTOR4( \
	(float)( ( dwCol >> 16 ) & 0xff ) / 255.0f, \
	(float)( ( dwCol >> 8  ) & 0xff ) / 255.0f, \
	(float)( ( dwCol       ) & 0xff ) / 255.0f, \
	(float)( ( dwCol >> 24 ) & 0xff ) / 255.0f )
#define ARGB_MAKE_FROM_INT(r, g, b, a)		(DWORD ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b)))
#define ARGB_MAKE_FROM_FLOAT(r, g, b, a)	ARGB_MAKE_FROM_INT(float(r) * 255.0f, float(g) * 255.0f, float(b) * 255.0f, float(a) * 255.0f)
#define ARGB_MAKE_FROM_VECTOR4(v4)			ARGB_MAKE_FROM_FLOAT(v4.x, v4.y, v4.z, v4.w)
#define ARGB_MAKE_FROM_VECTOR3(v3, a)		ARGB_MAKE_FROM_FLOAT(v3.x, v3.y, v3.z, a)
#define MAKE_565( r, g, b )					( ( ( (b) & 0xf8 ) << 8 ) + ( ( (g) & 0xfc ) << 3 ) + ( ( (r) & 0xf8 ) >> 3 ) )
#define FLOAT_AS_DWORD( f )					(*((DWORD*)(&(f))))

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum DRAW_LAYER
{
	DRAW_LAYER_DEFAULT = -1,
	DRAW_LAYER_GENERAL = 0,
	DRAW_LAYER_ENV,
	DRAW_LAYER_UI,
	DRAW_LAYER_SCREEN_EFFECT,
	//DRAW_LAYER_UI_TOP,
	NUM_DRAW_LAYERS
};

enum
{
	OBJECT_SPACE = 0,
	WORLD_SPACE,
	//
	NUM_COORDINATE_SPACES
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// generic RECT struct, matching exactly to Windows' RECT
struct E_RECT
{
	LONG    left;
	LONG    top;
	LONG    right;
	LONG    bottom;

	void Set( LONG l, LONG t, LONG r, LONG b )		{ left = l; top = t; right = r; bottom = b; }
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

// From Knuth, The Art of Computer Programming, Volume 1: Fundamental Algorithms, Third Edition (1997).
const float gfInvLogn2 = static_cast<float>(1.4426950408889634073599246810018921374266);

#if ISVERSION(DEVELOPMENT)
#define DRAW_LAYER_NAME_MAX_LEN		64
extern const char gszDrawLayerNames[ NUM_DRAW_LAYERS ][ DRAW_LAYER_NAME_MAX_LEN ];
#endif // DEVELOPMENT

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_CommenceInit(void);
PRESULT e_ConcludeInit(void);
PRESULT e_Deinit(void);

bool e_IsNoRender(void);

PRESULT e_InitializeStorage();
PRESULT e_DestroyStorage();

struct RAND & e_GetEngineOnlyRand();

int e_GetResolutionDownsamples(
	int nFromWidth,
	int nFromHeight,
	int nToWidth,
	int nToHeight );

const TCHAR* e_GetResultString( PRESULT pr );

BOOL e_IsDX10Enabled();
const TCHAR * e_GetTargetName();

#endif // __E_COMMON__