//----------------------------------------------------------------------------
// e_texture.cpp
//
// - Implementation for texture structures and functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_common.h"

#include "e_texture.h"
#include "e_texture_priv.h"
#include "e_settings.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

int gnDefaultTextureIDs[ NUM_TEXTURE_TYPES ]		= { INVALID_ID };
int gnDebugNeutralTextureIDs[ NUM_TEXTURE_TYPES ]	= { INVALID_ID };
int gnDefaultCubeTextureID							= INVALID_ID;

//NVTL: if you change enum UTILITY_TEXTURE_TYPE
// add the default value here also...
UTILITY_TEXTURE gnUtilityTextures[ NUM_UTILITY_TEXTURES ] = 
{
	{ INVALID_ID, 0xffffffff },
	{ INVALID_ID, 0x00000000 },
	{ INVALID_ID, 0xff000000 },
	{ INVALID_ID, 0xff7f7fff },
	{ INVALID_ID, 0xff7f7f7f },
	{ INVALID_ID, 0x7f7f7f7f },
	{ INVALID_ID, 0xff2B2B36 },
	{ INVALID_ID, 0xff000000 },
};

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

int e_GetPow2Resolution( int nResolution )
{
	if ( nResolution < 2 )
		return 2;

	int nPromote = 0;
	if ( ! IsPowerOf2( nResolution ) )
		nPromote = 1;

	int nLevels = e_GetNumMIPs( nResolution );
	return 2 << ( nLevels + nPromote - 1 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetDefaultTexture( TEXTURE_TYPE eType )
{
	if ( eType < 0 || eType >= NUM_TEXTURE_TYPES )
		return INVALID_ID;
	if ( eType == TEXTURE_ENVMAP )
		return gnDefaultCubeTextureID;

#if ISVERSION(DEVELOPMENT)
	if ( e_GetDefaultTextureOverride( eType ) )
	{
		return gnDebugNeutralTextureIDs[ eType ];
	}
#endif

	return gnDefaultTextureIDs[ eType ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetUtilityTexture( UTILITY_TEXTURE_TYPE eType )
{
	return gnUtilityTextures[ eType ].nTextureID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_TextureToggleDefaultOverride( TEXTURE_TYPE eType )
{
	switch ( eType )
	{
	case TEXTURE_DIFFUSE:
	case TEXTURE_DIFFUSE2:
		e_ToggleRenderFlag( RENDER_FLAG_NEUTRAL_DIFFUSE );
		break;
	case TEXTURE_NORMAL:
		e_ToggleRenderFlag( RENDER_FLAG_NEUTRAL_NORMAL );
		break;
	case TEXTURE_SELFILLUM:
		e_ToggleRenderFlag( RENDER_FLAG_NEUTRAL_SELFILLUM );
		break;
	case TEXTURE_SPECULAR:
		e_ToggleRenderFlag( RENDER_FLAG_NEUTRAL_SPECULAR );
		break;
	case TEXTURE_LIGHTMAP:
		e_ToggleRenderFlag( RENDER_FLAG_NEUTRAL_LIGHTMAP );
		break;
	case TEXTURE_ENVMAP:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_GetDefaultTextureOverride( TEXTURE_TYPE eType )
{
	switch ( eType )
	{
	case TEXTURE_DIFFUSE:
	case TEXTURE_DIFFUSE2:
		return e_GetRenderFlag( RENDER_FLAG_NEUTRAL_DIFFUSE );
	case TEXTURE_NORMAL:
		return e_GetRenderFlag( RENDER_FLAG_NEUTRAL_NORMAL );
	case TEXTURE_SELFILLUM:
		return e_GetRenderFlag( RENDER_FLAG_NEUTRAL_SELFILLUM );
	case TEXTURE_SPECULAR:
		return e_GetRenderFlag( RENDER_FLAG_NEUTRAL_SPECULAR );
	case TEXTURE_LIGHTMAP:
		return e_GetRenderFlag( RENDER_FLAG_NEUTRAL_LIGHTMAP );
	case TEXTURE_ENVMAP:
		break;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_ResolutionMIP( int nRes, int nMIP )
{
	int nMIPRes = nRes >> nMIP;
	if ( nMIPRes <= 0 )
		nMIPRes = 1;

	return nMIPRes;
}