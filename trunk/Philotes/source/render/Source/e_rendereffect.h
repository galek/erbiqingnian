//----------------------------------------------------------------------------
// e_rendereffect.h
//
// - Header for render effect abstraction
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_RENDEREFFECT__
#define __E_RENDEREFFECT__

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum RENDER_EFFECT_CLASS
{
	RFX_CLASS_NONE = -1,
	RFX_CLASS_POSTPROCESS = 0,	// full-screen blur, bloom, tone map
	RFX_CLASS_RENDERTOTEXTURE,	// dynamic cube map, planar reflection
	RFX_CLASS_MODIFIER,			// space warp, force state
	RFX_CLASS_VISUAL,			// screen warp
	NUM_RFX_CLASSES
};

#define RENDER_EFFECT_ENUM
enum RENDER_EFFECT_TYPE
{
	RFX_INVALID = -1,
	#include "e_rendereffect_priv.h"
	NUM_RENDER_EFFECT_TYPES
};
#undef RENDER_EFFECT_ENUM

enum RENDER_EFFECT_COMMAND
{
	RFX_BEGIN = 0,
	RFX_END,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

struct RenderEffect;
typedef void (* PFN_EFFECT_BEGIN)( RenderEffect * pEffect );
typedef void (* PFN_EFFECT_END)  ( RenderEffect * pEffect );

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct RenderEffect
{
	RENDER_EFFECT_TYPE		eType;
	RENDER_EFFECT_CLASS		eClass;
	BOOL					bBounded;			// requires Begin/End
	BOOL					bTypeExclusive;		// only one of this type at any given time
	int						nRefCount;			// number of currently active effects of this type
	PFN_EFFECT_BEGIN		pfnBegin;
	PFN_EFFECT_END			pfnEnd;
};

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline const RenderEffect * e_RenderEffectGet( RENDER_EFFECT_TYPE eType )
{
	extern RenderEffect gtRenderEffects[];
	return & gtRenderEffects[ eType ];
}

inline DWORD e_RenderEffectMakeTypeMask( RENDER_EFFECT_TYPE eType, RENDER_EFFECT_COMMAND eCmd )
{
	return DWORD( (unsigned short)eType << 16 ) + DWORD( eCmd );
}

inline RENDER_EFFECT_TYPE e_RenderEffectGetType( DWORD dwTypeMask )
{
	return RENDER_EFFECT_TYPE( (dwTypeMask >> 16 ) & 0x0000ffff );  // to avoid the high-bit replication behavior of >>
}

inline RENDER_EFFECT_COMMAND e_RenderEffectGetCommand( DWORD dwTypeMask )
{
	return RENDER_EFFECT_COMMAND( dwTypeMask & 0x0000ffff );
}


#endif // __E_RENDEREFFECT__