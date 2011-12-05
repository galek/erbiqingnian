//----------------------------------------------------------------------------
// dxC_texture.h
//
// - Header for texture functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_TEXTURE__
#define __DXC_TEXTURE__

#include "refcount.h"
#include "e_budget.h"
#include "e_texture.h"
#include "dxC_resource.h"
#include <ddraw.h>

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define ON_DEMAND_TEXTURE_CREATION

#define D3D_TEXTURE_FILE_EXTENSION		"dds"
#define D3D_TEXTURE_FILE_HEADER_BYTES	128
#define TEXTURE_DEFAULT_D3D_LOD			0
#define BC5_TEXTURE_FILENAME_FORMAT		"%s_bc5.%s"

#define DDS_FILE_MAGIC					MAKEFOURCCSTR("DDS ")

#define TEXTURE_CONVERT_FLAG_SHARPENMIP0				MAKE_MASK(0)
#define TEXTURE_CONVERT_FLAG_BINARYALPHA__DEPRECATED	MAKE_MASK(1)
#define TEXTURE_CONVERT_FLAG_DITHERMIPS					MAKE_MASK(2)
#define TEXTURE_CONVERT_FLAG_DITHERMIP0					MAKE_MASK(3)
#define TEXTURE_CONVERT_FLAG_QUICKCOMPRESS				MAKE_MASK(4)
#define TEXTURE_CONVERT_FLAG_GREYSCALE					MAKE_MASK(5)
#define TEXTURE_CONVERT_FLAG_DXT5NORMALMAP				MAKE_MASK(6)
#define TEXTURE_CONVERT_FLAG_FORCE_BC5					MAKE_MASK(7)
#define TEXTURE_CONVERT_FLAG_NO_ERROR_ON_SAVE			MAKE_MASK(8)
#define TEXTURE_CONVERT_FLAG_DXT1ALPHA					MAKE_MASK(9)

#define TEXTURE_DEF_ALL				0xffffffff
#define TEXTURE_DEF_WIDTH			MAKE_MASK(0)
#define TEXTURE_DEF_HEIGHT			MAKE_MASK(1)
#define TEXTURE_DEF_FORMAT			MAKE_MASK(2)
#define TEXTURE_DEF_MIPLEVELS		MAKE_MASK(3)
#define TEXTURE_DEF_MIPUSED			MAKE_MASK(4)
#define TEXTURE_DEF_MATERIAL		MAKE_MASK(5)
#define TEXTURE_DEF_QUICKCOMPRESS	MAKE_MASK(6)
#define TEXTURE_DEF_MIPFILTER		MAKE_MASK(7)
#define TEXTURE_DEF_BLURFACTOR		MAKE_MASK(8)
#define TEXTURE_DEF_SHARPENFILTER	MAKE_MASK(9)
#define TEXTURE_DEF_SHARPENPASSES	MAKE_MASK(10)
#define TEXTURE_DEF_DXT5NORMALMAPS	MAKE_MASK(11)
#define TEXTURE_DEF_DXT1ALPHA		MAKE_MASK(12)

// for fonts
#define TAB_CHAR_COUNT					4

// Temporary workaround
//#ifdef ENGINE_TARGET_DX9
//	#define OLD_TEXTURE_FILTER_CODE
//#endif

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

const int cnDXT1BytesPerPacket   = 8;
const int cnDXT2_5BytesPerPacket = 16;

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
struct TEXTURE_CHUNK_DATA
{
	WORD nChunkDataVersion;
	UINT nChunkWidth;
	UINT nChunkHeight; 
	UINT nOrigWidthInPixels;
	UINT nOrigHeight;
	UINT nHorizChunks; 
	UINT nVertChunks;
	UINT nMapSize;
	UINT *pnChunkMap;
};


typedef PRESULT (* PFN_TEXTURE_RESTORE ) ( struct D3D_TEXTURE * pTexture );
typedef PRESULT (* PFN_TEXTURE_PRESAVE ) ( TEXTURE_DEFINITION * pTextureDefinition, void* pTexture, D3DXIMAGE_INFO * pImageInfo, DWORD dwFlags );
typedef PRESULT (* PFN_TEXTURE_POSTLOAD ) ( int nTextureID );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct SYSMEM_TEXTURE;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

#define CHUNK_DATA_VERSION		(1 + GLOBAL_FILE_VERSION)


template<typename texType>
struct D3D_BASIC_TEXTURE
{
#ifdef ENGINE_TARGET_DX9
	CComPtr< texType >			pD3DTexture;
#elif defined( ENGINE_TARGET_DX10 )
	SPD3DCTEXTURE2D		pD3DTexture;
	SPD3DCSHADERRESOURCEVIEW	pD3D10ShaderResourceView;
#endif

	DX9_BLOCK( texType* )
	DX10_BLOCK( SPD3DCSHADERRESOURCEVIEW )
	GetShaderResourceView(UINT iLevel = 0)//KMNV TODO: Need to access various levels for TexCubes and the likes
	{
#ifdef ENGINE_TARGET_DX9
		return pD3DTexture;
#elif defined(ENGINE_TARGET_DX10)
		if( ! pD3D10ShaderResourceView && pD3DTexture) //If SRV isn't there, we need to create it
			CreateSRVFromTex2D( pD3DTexture, &pD3D10ShaderResourceView );
		else if ( pD3D10ShaderResourceView && ! pD3DTexture ) // Texture is NULL, so SRV should match
			pD3D10ShaderResourceView = NULL;

		return pD3D10ShaderResourceView;
#endif
	}

	REFCOUNT					tRefCount;				// reference counter to be used whenever something wants to prevent gbg col
	ENGINE_RESOURCE				tEngineRes;

	int							nFormat;
	DWORD						dwFlags;

	BYTE*						pbLocalTextureFile;		// Memory copy of texture file
	int							nTextureFileSize;			// used in creation
	BYTE*						pbLocalTextureData;
	SYSMEM_TEXTURE*				ptSysmemTexture;		// For system-memory textures (non-D3D)

	void*						pArrayCreationData;		//See struct ArrayCreationData in dx10_texture.cpp
};

struct D3D_TEXTURE : public D3D_BASIC_TEXTURE<DXC_9_10( IDirect3DTexture9, ID3D10Texture2D )>
{
	int							nId;
	D3D_TEXTURE					*pNext;

	int							nDefinitionId;			// Definition used - if it had one
	
	int							nWidthInPixels;
	int							nWidthInBytes;
	int							nHeight;
	D3DC_USAGE					tUsage;
	int							nD3DTopMIPSize;			// D3D texture top level surface size in bytes
	int							nUserID;
	PFN_TEXTURE_RESTORE			pfnRestore;
	PFN_TEXTURE_PRESAVE			pfnPreSave;
	PFN_TEXTURE_POSTLOAD		pfnPostLoad;

	int							nGroup;
	int							nType;
	int							nPriority;
	struct TEXTURE_LOADED_CALLBACKDATA * pLoadedCallbackData;

	HJOB						hFilterJob;
	TEXTURE_CHUNK_DATA*			pChunkData;
};

struct D3D_CUBETEXTURE : public D3D_BASIC_TEXTURE<DXC_9_10( IDirect3DCubeTexture9, ID3D10Texture2D )>
{
	int							nId;
	D3D_CUBETEXTURE				*pNext;

	int							nSize;
	int							nD3DPool;

	int							nGroup;
	int							nType;

	// Need to have this somewhere, or else we load multiple copies since we have no cube map definitions
	char						szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
};

struct DDCPIXELFORMAT
{
	DWORD				dwSize;					// Size of structure.
	DWORD				dwFlags;
	DWORD				dwFourCC;
	DWORD				dwRGBBitCount;
	DWORD				dwRBitMask;
	DWORD				dwGBitMask;
	DWORD				dwBBitMask;
	DWORD				dwRGBAlphaBitMask;
};

struct DDCCAPS
{
	DWORD				dwCaps1;
	DWORD				dwCaps2;
	DWORD				dwReserved[2];			// unused
};

struct DDCSURFACEDESC
{
	DWORD				dwSize;					// Size of structure.  Must be set to 124.
	DWORD				dwFlags;
	DWORD				dwHeight;
	DWORD				dwWidth;
	DWORD				dwPitchOrLinearSize;
	DWORD				dwDepth;
	DWORD				dwMipMapCount;
	DWORD				dwReserved1[11];		// unused
	DDCPIXELFORMAT		ddpfPixelFormat;
	DDCCAPS				ddsCaps;
	DWORD				dwReserved2;			// unused
};

struct DDS_FILE_HEADER
{
	DWORD			dwMagic;
	DDCSURFACEDESC	tDesc;
};


struct SYSMEM_TEXTURE
{
	UINT		nWidth;
	UINT		nHeight;
	DXC_FORMAT	tFormat;
	UINT		nLevels;
	BYTE *		pbData;			// actual data pointer
	BYTE **		ppLevels;		// array of MIP levels pointing into pbData
	DWORD		dwLockedLevels;

	MEMORYPOOL*	pPool;

	BYTE * GetLevel( UINT nLevel );
	DWORD GetLevelSize( UINT nLevel );
	PRESULT Lock( UINT nLevel, D3DLOCKED_RECT * pRect );
	PRESULT Unlock( UINT nLevel );
	PRESULT GetDesc( UINT nLevel, D3DC_TEXTURE2D_DESC * pDesc );

	UINT GetLevelCount()			{ return nLevels; }
	DXC_FORMAT GetFormat()			{ return tFormat; }

	DWORD GetSize();
	void Free();
};

inline int dxC_GetNumMipLevels( const LPD3DCTEXTURE2D pTexture )
{
	ASSERT_RETVAL( pTexture, 0 );
	DX9_BLOCK( return pTexture->GetLevelCount(); )

#ifdef ENGINE_TARGET_DX10
	D3D10_TEXTURE2D_DESC Desc;
	pTexture->GetDesc( &Desc );
	return Desc.MipLevels;
#endif
}
#ifdef ENGINE_TARGET_DX9
inline int dxC_GetNumMipLevels( LPD3DCTEXTURECUBE pTexture )
{
	ASSERT_RETVAL( pTexture, 0 );
	DX9_BLOCK( return pTexture->GetLevelCount(); )
	DX10_BLOCK
	(
		D3D10_TEXTURE2D_DESC Desc;
		pTexture->GetDesc( &Desc );  //KMNV TODO PERF: Probably not the fastest thing
		return Desc.MipLevels;
	)
}
#endif


inline int dxC_GetNumMipLevels( const D3D_TEXTURE * pTexture )
{
	ASSERT_RETVAL( pTexture, 0 );
	if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
	{
		ASSERT_RETVAL( pTexture->ptSysmemTexture, 0 );
		return (int)pTexture->ptSysmemTexture->nLevels;
	}
	return dxC_GetNumMipLevels( pTexture->pD3DTexture );
}

#if defined(ENGINE_TARGET_DX10)
#include "../DX10/dx10_texture.h"
#endif

#if defined( ENGINE_TARGET_DX9 )
inline PRESULT dxC_MapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, D3DLOCKED_RECT* pLockedRect, bool bReadOnly = false, bool bNoDirtyUpdate = false, bool bDiscard = false )
{
	DWORD dwFlags = 0;
	dwFlags |= bReadOnly ? D3DLOCK_READONLY : 0;
	dwFlags |= bNoDirtyUpdate ? D3DLOCK_NO_DIRTY_UPDATE : 0;
	if ( nLevel == 0 )
		dwFlags |= bDiscard ? D3DLOCK_DISCARD : 0;
	V_RETURN( pD3DTexture->LockRect( nLevel, pLockedRect, NULL, dwFlags ) );
	return S_OK;
}

inline PRESULT dxC_UnmapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, bool bPersistent = false, bool bUpdate = true )
{
	REF( bPersistent );
	REF( bUpdate );
	V_RETURN( pD3DTexture->UnlockRect( nLevel ) );
	return S_OK;
}
#endif

inline PRESULT dxC_MapSystemMemTexture( SYSMEM_TEXTURE * pSysmem, UINT nLevel, D3DLOCKED_RECT* pLockedRect )
{
	ASSERT_RETINVALIDARG( pSysmem );
	return pSysmem->Lock( nLevel, pLockedRect );
}

inline PRESULT dxC_UnmapSystemMemTexture( SYSMEM_TEXTURE * pSysmem, UINT nLevel )
{
	ASSERT_RETINVALIDARG( pSysmem );
	return pSysmem->Unlock( nLevel );
}

//----------------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------------

PRESULT dxC_Create2DTexture(UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE2D* ppTexture, void* pInitialData = NULL, UINT D3D10MiscFlags = NULL, UINT nMSAASamples = 1, UINT nMSAAQuality = 0 );
PRESULT dxC_Create2DTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, D3DCX_FILTER texFilter, D3DCX_FILTER mipFilter, D3DXIMAGE_INFO *pImageInfo, LPD3DCTEXTURE2D* ppTexture, UINT D3D10MiscFlags = NULL);
PRESULT dxC_Create2DTextureFromFileEx( LPCTSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT fmt, D3DCX_FILTER texFilter, D3DCX_FILTER mipFilter, D3DXIMAGE_INFO *pImageInfo, LPD3DCTEXTURE2D* ppTexture, UINT D3D10MiscFlags = NULL );

PRESULT dxC_CreateCubeTexture(UINT Size, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURECUBE* ppCubeTexture);
PRESULT dxC_CreateCubeTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Size, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT Format, D3DCX_FILTER Filter, D3DCX_FILTER MipFilter, D3DXIMAGE_INFO * pSrcInfo, LPD3DCTEXTURECUBE * ppCubeTexture);
void dxC_TextureFormatGetDisplayStr( DXC_FORMAT eFormat, char * pszStr, int nBufLen );
PRESULT dxC_UpdateTexture( LPD3DCTEXTURE2D pTex, CONST RECT* pDestRect, void* pData, CONST RECT* pSrcRect, UINT nLevel, DXC_FORMAT fmt, UINT SrcPitch );
#ifdef DXTLIB	// CHB 2007.06.25 - Now matches definition's conditional.
PRESULT dxC_CompressToGPUTexture( SPD3DCTEXTURE2D pSrcTexture, SPD3DCTEXTURE2D pDestTexture );
#endif
void dxC_DiffuseTextureDefinitionLoadedCallback( void * pDef, EVENT_DATA * pEventData );

PRESULT dxC_TextureSystemMemNewEmptyInPlace( D3D_TEXTURE * pTexture );


inline PRESULT dxC_Get2DTextureLevels( const LPD3DCTEXTURE2D pTex, UINT & nLevels )
{
	ASSERT_RETINVALIDARG( pTex );

	DX9_BLOCK( nLevels = pTex->GetLevelCount(); )

#ifdef ENGINE_TARGET_DX10
	D3DC_TEXTURE2D_DESC tDesc;
	pTex->GetDesc( &tDesc );
	nLevels = tDesc.MipLevels;
#endif

	return S_OK;
}

PRESULT dxC_Get2DTextureDesc( const LPD3DCTEXTURE2D pTex, UINT nLevel, D3DC_TEXTURE2D_DESC* pDesc);
PRESULT dxC_Get2DTextureDesc( const D3D_TEXTURE * pTexture, UINT nLevel, D3DC_TEXTURE2D_DESC* pDesc);
DWORD dxC_GetTextureLevelSize( UINT nWidth, UINT nHeight, DXC_FORMAT tFormat );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
inline D3D_TEXTURE * dxC_TextureGet(
	int id )
{
	extern CHash<D3D_TEXTURE> gtTextures;
	return HashGet(gtTextures, id);
}

inline PRESULT dxC_Create2DTextureFromFileInMemory( LPCVOID pSrcData, UINT SrcDataSize, LPD3DCTEXTURE2D* ppTexture)
{
	return dxC_Create2DTextureFromFileInMemoryEx( pSrcData, SrcDataSize, 0, 0, 0, D3DC_USAGE_2DTEX, D3DFMT_UNKNOWN, (D3DCX_FILTER)(D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER), (D3DCX_FILTER)D3DX_FILTER_BOX, NULL, ppTexture);
}

inline PRESULT dxC_GetImageInfoFromFile( LPCSTR pSrcFile, D3DXIMAGE_INFO * pSrcInfo)
{
	DX9_BLOCK( return D3DXGetImageInfoFromFile( pSrcFile, pSrcInfo); )
	DX10_BLOCK( return D3DX10GetImageInfoFromFile( pSrcFile, NULL, pSrcInfo, NULL ); ) //KMNV TODO THREADPUMP
}

PRESULT dxC_GetImageInfoFromFileInMemory(LPCVOID pSrcData, UINT SrcDataSize, D3DXIMAGE_INFO* pSrcInfo);

PRESULT dxC_SaveTextureToMemory(LPD3DCBASETEXTURE pSrcTexture, D3DC_IMAGE_FILE_FORMAT DestFormat, LPD3DCBLOB * ppDestBuf);

inline PRESULT dxC_CopyRT( LPD3DCTEXTURE2D pSrc, LPD3DCTEXTURE2D pDest )
{
	REF(pSrc);
	REF(pDest);
	return E_NOTIMPL;
}

PRESULT dxC_CopyGPUTextureToGPUTexture( LPD3DCTEXTURE2D pSrc, LPD3DCTEXTURE2D pDest, UINT nLevel );

void dxC_SetMaxLOD( D3D_TEXTURE* pTexture, UINT lod );

inline PRESULT dxC_FillTexture(
	LPD3DCTEXTURE2D pTexture,
	LPD3DXFILL2D pFunction,
	LPVOID pData)
{
	DX9_BLOCK( return D3DXFillTexture(pTexture, pFunction, pData) );
	DX10_BLOCK
	(
		return dx10_FillTexture(pTexture, pFunction, pData);
	);
}

inline void dxC_TextureAddRef( D3D_TEXTURE * pTexture )
{
	pTexture->tRefCount.AddRef();
}

inline int dxC_TextureReleaseRef( D3D_TEXTURE * pTexture )
{
	return pTexture->tRefCount.Release();
}

inline void dxC_CubeTextureAddRef( D3D_CUBETEXTURE * pCubeTexture )
{
	pCubeTexture->tRefCount.AddRef();
}

inline int dxC_CubeTextureReleaseRef( D3D_CUBETEXTURE * pCubeTexture )
{
	return pCubeTexture->tRefCount.Release();
}

enum TEXTURE_FORMAT_CONVERSION
{
	CONVERT_TO_TEXFMT,
	CONVERT_TO_D3D
};

inline int dxC_ConvertTextureFormat( int nSourceFormat, 
									 TEXTURE_FORMAT_CONVERSION eDirection )
{
#ifdef ENGINE_TARGET_DX9
	REF( eDirection );
	return nSourceFormat;
#else

	switch ( eDirection )
	{
	case CONVERT_TO_D3D:
		switch ( nSourceFormat )
		{
		// compressed formats
		case TEXFMT_DXT1:
			return D3DFMT_DXT1;
		case TEXFMT_DXT3:
			return D3DFMT_DXT3;
		case TEXFMT_DXT5:
			return D3DFMT_DXT5;

		// standard formats
		case TEXFMT_A8R8G8B8:
			return D3DFMT_A8R8G8B8;
		case TEXFMT_A1R5G5B5:
			return D3DFMT_A1R5G5B5;
#ifndef ENGINE_TARGET_DX10
		case TEXFMT_A4R4G4B4:
			return D3DFMT_A4R4G4B4;
#endif
		case TEXFMT_L8:
			return D3DFMT_L8;

		default:
			ASSERT_RETINVALID( 0 && "Unsupported format for conversion!" );			
		}

	case CONVERT_TO_TEXFMT:
		switch ( nSourceFormat )
		{
		// compressed formats
		case D3DFMT_DXT1:
			return TEXFMT_DXT1;
		case D3DFMT_DXT3:
			return TEXFMT_DXT3;
		case D3DFMT_DXT5:
		case DXGI_FORMAT_BC5_UNORM:
			return TEXFMT_DXT5;

		// standard formats
		case D3DFMT_A8R8G8B8:
			return TEXFMT_A8R8G8B8;
		case D3DFMT_A1R5G5B5:
			return TEXFMT_A1R5G5B5;
#ifndef ENGINE_TARGET_DX10
		case D3DFMT_A4R4G4B4:
			return TEXFMT_A4R4G4B4;
#endif
		case D3DFMT_L8:
			return TEXFMT_L8;

		default:
			ASSERT_RETINVALID( 0 && "Unsupported format for conversion!" );			
		}

	default:
		ASSERT_RETINVALID( 0 && "Incorrect conversion direction!" );
	}
#endif
}


//KMNV following ripped from DX9Texture needs to be renamed
//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct TEXTURE_FILTER_DATA
{
	int nTextureId;

	DWORD		dwWidth;
	DWORD		dwHeight;
	DXC_FORMAT	tSrcFormat;
	BYTE *		pbSrc;
	DXC_FORMAT	tDestFormat;
	int			nLevelCount;
	BYTE **		pbDestLevels;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline D3D_TEXTURE * dxC_TextureGetFirst()
{
	extern CHash<D3D_TEXTURE> gtTextures;
	return HashGetFirst(gtTextures);
}

inline D3D_TEXTURE * dxC_TextureGetNext(
	D3D_TEXTURE * pTexCur )
{
	extern CHash<D3D_TEXTURE> gtTextures;
	return HashGetNext(gtTextures, pTexCur);
}


inline D3D_CUBETEXTURE * dx9_CubeTextureGet(
	int id )
{
	extern CHash<D3D_CUBETEXTURE> gtCubeTextures;
	return HashGet(gtCubeTextures, id);
}

inline D3D_CUBETEXTURE * dx9_CubeTextureGetFirst()
{
	extern CHash<D3D_CUBETEXTURE> gtCubeTextures;
	return HashGetFirst(gtCubeTextures);
}

inline D3D_CUBETEXTURE * dx9_CubeTextureGetNext(
	D3D_CUBETEXTURE * pTexCur )
{
	extern CHash<D3D_CUBETEXTURE> gtCubeTextures;
	return HashGetNext(gtCubeTextures, pTexCur);
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_TextureReload ( 
	D3D_TEXTURE * pTexture, 
	const char * pszFileNameWithPath, 
	DWORD dwTextureFlags, 
	D3DFORMAT nDefaultFormat, 
	DWORD dwReloadFlags = 0,
	TEXTURE_DEFINITION * pDefOverride = NULL,
	PAK_ENUM ePakfile = PAK_DEFAULT );

PRESULT dx9_TextureNewEmpty ( 
	int* pnTextureId,
	int nWidth,
	int nHeight, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup,
	int nType,
	D3DC_USAGE tUsagePool = D3DC_USAGE_2DTEX,
	PFN_TEXTURE_RESTORE pfnRestore = NULL, 
	int nUserID = INVALID_ID );

PRESULT dx9_TextureNewEmptyInPlace ( 
	int nTextureId,
	int nWidth, 
	int nHeight, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup,
	int nType,
	D3DC_USAGE tUsagePool = D3DC_USAGE_2DTEX, 
	//D3DC_BINDING tBinding = D3DC_BINDING_DEFAULT, 
	PFN_TEXTURE_RESTORE pfnRestore = NULL, 
	int nUserID = INVALID_ID );

PRESULT dx9_CubeTextureNewEmpty ( 
	int* pnCubeTextureId,
	int nSize, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup, 
	int nType,
	D3DC_USAGE tUsagePool = D3DC_USAGE_2DTEX );

PRESULT dx9_CubeTextureNewEmptyInPlace ( 
	int nCubeTextureId,
	int nSize, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup,
	int nType,
	D3DC_USAGE tUsagePool = D3DC_USAGE_2DTEX );

//void dx9_GetTextureSurfaceDC( int nTextureID, void ** ppDC, int nLevel = 0 );
//void dx9_ReleaseTextureSurfaceDC( int nTextureID, void * hDC, int nLevel = 0 );
int dx9_GetTextureFormatBitDepth( DXC_FORMAT eFormat );
int dx9_GetTextureMIPLevelSizeInBytes( const D3D_TEXTURE * pTexture, int nMIPLevel = 0 );
int dx9_GetCubeTextureMIPLevelSizeInBytes( const D3D_CUBETEXTURE * pCubeTexture, int nMIPLevel = 0 );
int dx9_GetTextureLODSizeInBytes( const D3D_TEXTURE * pTexture, int nLOD = -1 );
int dxC_GetTexturePitch( int nWidth, DXC_FORMAT eFormat );
BOOL dx9_TextureFormatHasAlpha( DXC_FORMAT eFormat );
BOOL dx9_IsEquivalentTextureFormat( DXC_FORMAT eFormat1, DXC_FORMAT eFormat2 );
BOOL dx9_IsCompressedTextureFormat( DXC_FORMAT eFormat );
void dx9_Temp_ConvertAllTextures();
void dx9_ReportSetTexture( int nTextureID );
PRESULT dx9_ReportTextureUse( const D3D_TEXTURE * pTexture );
PRESULT dx9_TextureDefinitionUpdateFromTexture ( const char * pszTextureFilename, TEXTURE_DEFINITION * pTextureDef );
PRESULT dx9_ModifyAndSaveTextureDefinition( TEXTURE_DEFINITION * pSourceDefinition, TEXTURE_DEFINITION * pDestDefinition, DWORD dwFieldFlags, DWORD dwReloadFlags = 0 );
BOOL dx9_IsTextureFormatOk( DXC_FORMAT CheckFormat, BOOL bRenderTarget = FALSE ) ;

//----------------------------------------------------------------------------
// Generates MIP levels for a given top level (surface)
// Optionally compresses
// Takes an array of pointers to each level, and expects the top level (index 0) to already be filled in
// DOES NOT CALL OR USE ANY DIRECTX FUNCTIONS/INTERFACES
//----------------------------------------------------------------------------
PRESULT dx9_TextureFilter(
	struct TEXTURE_FILTER_DATA * ptFilterData,
	DWORD dwMIPLevels  = D3DX_FILTER_LINEAR,
	DWORD dwD3DXFilter = D3DX_FILTER_BOX );

//----------------------------------------------------------------------------
// DXT compresses a single texture level (surface)
// DOES NOT CALL OR USE ANY DIRECTX FUNCTIONS/INTERFACES
//----------------------------------------------------------------------------
PRESULT dx9_TextureLevelCompress (
	DWORD dwWidth,
	DWORD dwHeight,
	DXC_FORMAT tSrcFormat,
	BYTE * pbSrc,
	DXC_FORMAT tDestFormat,
	BYTE ** pbDest,
	BOOL bDestHasMips );

PRESULT dx9_TextureLevelDecompress (
	DWORD dwSrcWidth,
	DWORD dwSrcHeight,
	DXC_FORMAT tSrcFormat,
	const BYTE * pbSrc,
	DWORD dwDestWidth,
	DWORD dwDestHeight,
	DXC_FORMAT tDestFormat,
	BYTE * pbDest );

void dx9_ResetTextureUseStats();
PRESULT dx9_VerifyValidTextureSet( const LPD3DCBASETEXTURE pTexture );

PRESULT dxC_TextureAdd(
	int nDefaultId,
	D3D_TEXTURE ** ppTexture,
	int nGroup,
	int nType );

PRESULT dx9_CubeTextureAdd(
	int * pnId,
	D3D_CUBETEXTURE ** ppCubeTexture,
	int nGroup,
	int nType );

PRESULT dx9_TextureFileProcess( 
	D3D_TEXTURE * pTexture,
	TEXTURE_DEFINITION * pDefinition,
	PAK_ENUM ePakEnum = PAK_DEFAULT );
	
PRESULT dx9_TextureCreateIfReady(
	D3D_TEXTURE * pTexture,
	TEXTURE_DEFINITION * pDef );

PRESULT dxC_TexturePreload(
	D3D_TEXTURE * pTexture );

PRESULT dxC_SaveTextureToFile(
	LPCTSTR szFileName,
	D3DC_IMAGE_FILE_FORMAT fFmt,
	LPD3DCTEXTURE2D pTex,
	BOOL bUpdatePak = false );

PRESULT dxC_TextureShouldSet(
	const D3D_TEXTURE * pTexture,
	int & nReplacement );

PRESULT dxC_TextureDidSet(
	D3D_TEXTURE * pTexture );

PRESULT dxC_TextureGetSizeInMemory( 
	const D3D_TEXTURE * pTexture,
	DWORD * pdwBytes );

PRESULT dxC_TextureInitEngineResource(
	D3D_TEXTURE * pTexture );

PRESULT dxC_CreateTextureArray(
	D3D_TEXTURE* pTexture,
	TEXTURE_DEFINITION* pDefinition );

D3DC_IMAGE_FILE_FORMAT dxC_TextureSaveFmtToImageFileFmt(
	TEXTURE_SAVE_FORMAT tTSF );

const OS_PATH_CHAR * dxC_TextureSaveFmtGetExtension(
	D3DC_IMAGE_FILE_FORMAT tIFF );

#endif //__DXC_TEXTURE__