//----------------------------------------------------------------------------
// e_texture.h
//
// - Header for texture functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_TEXTURE__
#define __E_TEXTURE__

#include "e_definition.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define FONT_SET_SIZE		28
#define FONT_BITMAP_SIZE	32

#define TEXTURE_FLAG_NOERRORIFMISSING			MAKE_MASK(0)
#define TEXTURE_FLAG_SYSMEM						MAKE_MASK(2)
#define TEXTURE_FLAG_FORCEFORMAT				MAKE_MASK(3)
#define TEXTURE_FLAG_POOLDEFAULT				MAKE_MASK(4)
#define TEXTURE_FLAG_NOLOD						MAKE_MASK(5)
#define TEXTURE_FLAG_WARDROBE_COLOR_COMBINING	MAKE_MASK(6)
#define _TEXTURE_FLAG_LOAD_FAILED				MAKE_MASK(7)
#define _TEXTURE_FLAG_LOAD_FAILURE_WARNED		MAKE_MASK(8)
#define TEXTURE_FLAG_HAS_ALPHA					MAKE_MASK(9)
#define TEXTURE_FLAG_USE_SCRATCHMEM				MAKE_MASK(10)

#define TEXTURE_LOAD_NO_SAVE					MAKE_MASK(0)
#define TEXTURE_LOAD_NO_CONVERT					MAKE_MASK(1)
#define TEXTURE_LOAD_SOURCE_ONLY				MAKE_MASK(2)
#define TEXTURE_LOAD_NO_ERROR_ON_SAVE			MAKE_MASK(3)
#define TEXTURE_LOAD_NO_ASYNC_TEXTURE			MAKE_MASK(4)
#define TEXTURE_LOAD_NO_ASYNC_DEFINITION		MAKE_MASK(5)
#define TEXTURE_LOAD_FORCECREATETFILE			MAKE_MASK(6)
#define TEXTURE_LOAD_ALREADY_UPDATING			MAKE_MASK(7)
#define TEXTURE_LOAD_FORCE_BC5_UPDATE			MAKE_MASK(8)
#define TEXTURE_LOAD_DONT_FREE_LOCAL			MAKE_MASK(9)
#define TEXTURE_LOAD_DONT_CHECKOUT_XML			MAKE_MASK(10)
#define TEXTURE_LOAD_ADD_TO_HIGH_PAK			MAKE_MASK(11)
#define TEXTURE_LOAD_FILLPAK_LOAD_FROM_PAK		MAKE_MASK(12)		// force load the texture during fillpak even if it's already in the pak
#define TEXTURE_LOAD_DONT_ASK_TO_CHECKOUT_XML	MAKE_MASK(13)

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum TEXCOORDS_CHANNEL
{
	TEXCOORDS_CHANNEL_LIGHTMAP = 0,
	TEXCOORDS_CHANNEL_DIFFUSE  = 1,
	TEXCOORDS_CHANNEL_DIFFUSE2 = 2,
	TEXCOORDS_MAX_CHANNELS
};

enum TEXTURE_FORMAT
{
	// These values were originally DX9 values and are being preserved since
	// so many texture definitions are already defined. Moving forward, we can
	// define additional D3D-agnostic formats for our own use.
	TEXFMT_UNKNOWN		= 0,
	TEXFMT_DXT1 		= MAKEFOURCC('D', 'X', 'T', '1'),
	TEXFMT_DXT2 		= MAKEFOURCC('D', 'X', 'T', '2'),
	TEXFMT_DXT3 		= MAKEFOURCC('D', 'X', 'T', '3'),
	TEXFMT_DXT4 		= MAKEFOURCC('D', 'X', 'T', '4'),
	TEXFMT_DXT5			= MAKEFOURCC('D', 'X', 'T', '5'),
	TEXFMT_CMP_NORMAL	= TEXFMT_DXT5,
	TEXFMT_A8R8G8B8		= 21,
	TEXFMT_A1R5G5B5		= 25,
	TEXFMT_A4R4G4B4		= 26,
	TEXFMT_L8			= 50
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

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

const char * e_GetTextureGroupName(
	TEXTURE_GROUP eGroup );

const char * e_GetTextureTypeName(
	TEXTURE_TYPE eType );

PRESULT e_TextureGetFullFileName (
	char* pszFullFileName,
	int nMaxLen,
	const char * pszBaseFolder,
	const char * pszTextureSetFolder,
	const char * pszFilename );

PRESULT e_TextureNewFromFile (
	int* pnTextureID,
	const char *pszFileNameWithPath,
	int nGroup,
	int nType, 
	DWORD dwTextureFlags = 0, 
	int nDefaultFormat = 0, 
	void * pfnRestore = NULL, 
	void * pfnPreSave = NULL,
	void * pfnPostLoad = NULL,
	TEXTURE_DEFINITION * pDefOverride = NULL,
	DWORD dwReloadFlags = 0,
	float fDistanceToCamera = 0.0f,
	int usageOverride = 0,
	PAK_ENUM ePakfile = PAK_DEFAULT);

PRESULT e_TextureNewFromFileInMemory ( 
	int* pnTextureId,
	BYTE* pbData, 
	unsigned int nDataSize,
	int nGroup,
	int nType,
	DWORD dwLoadFlags = 0 );

BOOL e_CubeTextureIsLoaded (
	int nCubeTextureId,
	BOOL bAllowFinalize = FALSE );

BOOL e_TextureIsLoaded (
	int nTextureId,
	BOOL bAllowFinalize = FALSE );

BOOL e_CubeTextureLoadFailed (
	int nCubeTextureId );

BOOL e_TextureLoadFailed (
	int nTextureId );

void e_TextureSetLoadFailureWarned (
	int nTextureId );

BOOL e_TextureLoadFailureWarned (
	int nTextureId );

PRESULT e_CubeTextureNewFromFile(
	int* pnCubeTextureId,
	const char * pszFileNameWithPath,
	TEXTURE_GROUP eGroup = TEXTURE_GROUP_NONE,
	BOOL bForceSync = FALSE );

DWORD e_GetAvailTextureMemory();
PRESULT e_TextureRestore( int id );
PRESULT e_TextureRemove( int nTextureID );
PRESULT e_CubeTextureRemove( int nTextureId );
PRESULT e_ReloadAllTextures( BOOL bCompressAndSave );
PRESULT e_TextureReload( int nTexture );
BOOL e_TextureIsValidID( int nTextureID );
PRESULT e_TextureGetOriginalResolution( int nTextureID, int & nWidth, int & nHeight );
PRESULT e_TextureGetLoadedResolution( int nTextureID, int & nWidth, int & nHeight );
int e_TextureGetDefinition( int nTextureID );
BOOL e_TextureSlotIsUsed( TEXTURE_TYPE eType );
int e_GetTextureFormat( int nSourceFormat );
int e_GetD3DTextureFormat( int nSourceFormat );
int e_ResolutionMIP( int nRes, int nMIP );
PRESULT e_TextureGetBits( int nTextureID, void ** ppBits, unsigned int& nSize, BOOL bAlloc );

PRESULT e_FontInit(
	int * pnTextureID,
	int nLetterCountWidth,
	int nLetterCountHeight,
	int nGDISize,
	BOOL bBold,
	BOOL bItalic,
	const char * pszSystemName,
	const char * pszLocalPath,
	float * pfFontWidth,
	float fTabWidth );

PRESULT e_DumpTextureReport();
int e_GetNumMIPLevels( int nDimension );
int e_GetNumMIPs( int nDimension );
int e_GetPow2Resolution( int nResolution );
PRESULT e_RestoreDefaultTextures();
int e_GetDefaultTexture( TEXTURE_TYPE eType );
void e_TextureToggleDefaultOverride( TEXTURE_TYPE eType );
BOOL e_GetDefaultTextureOverride( TEXTURE_TYPE eType );
PRESULT e_AddTextureUsedStats( int eGroup, int eType, int & nCount, int & nSize );
PRESULT e_TextureDumpList( TEXTURE_GROUP eGroup = TEXTURE_GROUP_NONE, TEXTURE_TYPE eType = TEXTURE_NONE );
PRESULT e_TexturesCleanup( TEXTURE_GROUP eOnlyGroup = TEXTURE_GROUP_NONE, TEXTURE_TYPE eOnlyType = TEXTURE_NONE );

PRESULT e_TexturesInit();
PRESULT e_TexturesDestroy();
PRESULT e_TexturesRemoveAll( DWORD dwOnlyGroup = TEXTURE_GROUP_NONE );
PRESULT e_TextureFilterRequest ( int nTextureId );
BOOL e_TextureIsFiltering   ( int nTextureId );
BOOL e_TextureIsColorCombining( int nTextureId );
PRESULT e_TexturePreload( int nTexture );
void e_TextureAddRef( int nTextureID );
void e_TextureReleaseRef( int nTextureID, BOOL bCleanupNow = FALSE );
int e_TextureGetRefCount( int nTexture );
void e_CubeTextureAddRef( int nCubeTextureID );
void e_CubeTextureReleaseRef( int nCubeTextureID );
PRESULT e_TextureGetSize( int nTextureID, int& nWidthOut, int& nHeightOut );
PRESULT e_TextureGetSizeInMemory( int nTextureID, DWORD * pdwBytes );
PRESULT e_CubeTextureGetSizeInMemory( int nCubeTextureID, DWORD * pdwBytes );
void e_UseNVTT( BOOL bEnable ); // Use the new NVIDIA Texture Tools library for conversions.
BOOL e_GetNVTTEnabled();

#endif // __E_TEXTURE__
