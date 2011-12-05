//----------------------------------------------------------------------------
// e_texture_priv.h
//
// - Header for texture functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_TEXTURE_PRIV_
#define __E_TEXTURE_PRIV_

#include "config.h"
#include "DxC/dxC_texturesamplerconstants.h"

// lots of things rely on this numbering/ordering
typedef enum {
	TEXTURE_GROUP_NONE = -1,
	TEXTURE_GROUP_BACKGROUND = 0,
	TEXTURE_GROUP_UNITS,
	TEXTURE_GROUP_PARTICLE,
	TEXTURE_GROUP_UI,
	TEXTURE_GROUP_WARDROBE,
	NUM_TEXTURE_GROUPS,
}TEXTURE_GROUP;

typedef enum 
{
	TEXTURE_NONE = -1,
	TEXTURE_DIFFUSE = 0,
	TEXTURE_NORMAL,
	TEXTURE_SELFILLUM,
	TEXTURE_DIFFUSE2,
	TEXTURE_SPECULAR,
	TEXTURE_ENVMAP,
	TEXTURE_LIGHTMAP,
	NUM_TEXTURE_TYPES,
}TEXTURE_TYPE;

typedef enum
{
	TEXTURE_RGBA_FF = 0,		// white solid alpha
	TEXTURE_RGBA_00,			// black clear alpha
	TEXTURE_RGB_00_A_FF,		// black solid alpha
	TEXTURE_RG_7F_BA_FF,		// neutral biased normal
	TEXTURE_RGB_7F_A_FF,		// grey solid alpha
	TEXTURE_RGBA_7F,			// grey half alpha
	TEXTURE_OCCLUDED,			// texture for occluded objects
	TEXTURE_FLASHLIGHTMAP,		// override lightmap for flashlight levels
	// count
	NUM_UTILITY_TEXTURES,
} UTILITY_TEXTURE_TYPE;

typedef enum
{
	TEXTURE_SAVE_DDS = 0,
	TEXTURE_SAVE_TGA,
	TEXTURE_SAVE_PNG
} TEXTURE_SAVE_FORMAT;


struct UTILITY_TEXTURE
{
	int			nTextureID;
	DWORD		dwColor;
};

struct TEXTURE_TYPE_DATA
{
	char		pszName[ DEFAULT_INDEX_SIZE	]; 
	int			pnPriority[ NUM_TEXTURE_GROUPS ];
};

PRESULT e_TextureSave (
	int nTextureID,
	const char * pszFileNameWithPath,
	DWORD dwLoadFlags = 0,
	TEXTURE_SAVE_FORMAT eSaveFormat = TEXTURE_SAVE_DDS,
	BOOL bUpdatePak = FALSE );

PRESULT e_TextureSaveRect (
	int nTextureID,
	OS_PATH_CHAR * poszFileNameWithPath,
	struct E_RECT * pRect,
	TEXTURE_SAVE_FORMAT eSaveFormat = TEXTURE_SAVE_DDS,
	BOOL bUpdatePak = FALSE );

int e_GetUtilityTexture( UTILITY_TEXTURE_TYPE eType );

#endif