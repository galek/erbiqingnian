//----------------------------------------------------------------------------
// dx9_texture.cpp
//
// - Implementation for DX9-specific texture functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_definition.h"
#include "boundingbox.h"
#include "dxC_model.h"
#include "performance.h"
#include "filepaths.h"
#include "camera_priv.h"
#include "dxC_texture.h"
#include "e_texture_priv.h"
#include "pakfile.h"
#include "perfhier.h"
#include "definition_priv.h"
#include "appcommon.h"
#include "dxC_caps.h"
#include "dxC_irradiance.h"
#include "e_settings.h"
#include "e_budget.h"
#include "dxC_state.h"
#include "jobs.h"
#include "dxC_wardrobe.h"
#include "dxC_resource.h"
#include "event_data.h"
#include "fileio.h"
#include "e_optionstate.h"	// CHB 2007.03.02
#include "datatables.h"
#include "e_main.h"
#include "prime.h"
#include "nvtt.h"
#include "dxC_pixo.h"

#ifdef ENGINE_TARGET_DX10
	#include "dx10_texture.h"
	#include "libtarga.h"
#endif

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define ENABLE_TEXTURE_REPORT		ISVERSION(DEVELOPMENT)

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define D3DC_SKIP_DDS_MIP_LEVELS_MASK   0x1F
#define D3DC_SKIP_DDS_MIP_LEVELS_SHIFT  26	// CHB 2007.09.05 - This is correct, even though the docs say bits 27-31
#define D3DC_SKIP_DDS_MIP_LEVELS(levels, filter) ((((levels) & D3DC_SKIP_DDS_MIP_LEVELS_MASK) << D3DC_SKIP_DDS_MIP_LEVELS_SHIFT) | ((filter) == 0 ? D3DX_FILTER_BOX : (filter)))

#define FASTEST_COMPRESSION_THRESHOLD 0
//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
// Texture Ids match texture definition Ids when possible.  
// When that isn't possible, we assign an Id starting with MAX_TEXTURE_DEFINITIONS
int					gnTextureIdNext = MAX_TEXTURE_DEFINITIONS;
int					gnCubeTextureIdNext = 0;
int					gnBreakOnTextureID = INVALID_ID;

CHash<D3D_TEXTURE>		gtTextures;
CHash<D3D_CUBETEXTURE>	gtCubeTextures;

extern int gnDefaultTextureIDs[ NUM_TEXTURE_TYPES ];
extern int gnDebugNeutralTextureIDs[ NUM_TEXTURE_TYPES ];
extern int gnDefaultCubeTextureID;

const char gszTextureGroups[ NUM_TEXTURE_GROUPS + 1 ][ 16 ] = { 
	"Background",
	"Units",
	"Particle",
	"UI",
	"Wardrobe",
	// special cases
	"None"
};

const char gszTextureTypes[ NUM_TEXTURE_TYPES + 1 ][ 16 ] = { 
	"Diffuse",
	"Normal",
	"Selfillum",
	"Diffuse2",
	"Specular",
	"Envmap",
	"Lightmap",
	// special cases
	"None"
};

// For deciding whether to use the new NVIDIA Texture Tools library for conversions.
static BOOL sgbUseNVTT = TRUE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

#ifdef DXTLIB	// CHB 2007.06.25 - Now matches definition's conditional.
static PRESULT sConvertTexture( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefinition, TEXTURE_DEFINITION * pDefOverride, const char * pszFileNameWithPath, DWORD dwReloadFlags, int nDefaultFormat = D3DFMT_UNKNOWN, DWORD dwExtraConvertFlags = 0, const char* pszSaveFileName = NULL );
#endif
static PRESULT sSaveTexture( SPD3DCTEXTURE2D pSaveTexture, const char * pszFileNameWithPath, D3DC_IMAGE_FILE_FORMAT tFileFormat = D3DXIFF_DDS, BOOL bUpdatePak = FALSE );
static PRESULT sLoadSourceTexture( D3D_TEXTURE * pTexture, const char * pszFileNameWithPath );
static PRESULT sLoadConvertedTexture( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefinition, const char * pszFileNameWithPathDWORD );
static PRESULT sCreateTextureFromMemory( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefOverride = NULL, DWORD dwReloadFlags = 0, D3DFORMAT nDefaultFormat = D3DFMT_FROM_FILE ); //KMNV TODO: Need equivelant of D3DX_FROM_FILE for DX10
static PRESULT sConvertTextureCheap( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefinition, TEXTURE_DEFINITION * pDefOverride, const char * pszFileNameWithPath, DWORD dwReloadFlags );

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void e_UseNVTT( BOOL bEnable )
{
	sgbUseNVTT = bEnable;
}

BOOL e_GetNVTTEnabled()
{
	return sgbUseNVTT;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char * e_GetTextureGroupName( TEXTURE_GROUP eGroup )
{
	if ( eGroup == TEXTURE_GROUP_NONE )
		return gszTextureGroups[ NUM_TEXTURE_GROUPS + 1 ];

	ASSERT_RETNULL( IS_VALID_INDEX( eGroup, NUM_TEXTURE_TYPES ) );

	return gszTextureGroups[ eGroup ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char * e_GetTextureTypeName( TEXTURE_TYPE eType )
{
	if ( eType == TEXTURE_NONE )
		return gszTextureTypes[ NUM_TEXTURE_TYPES + 1 ];

	ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_TEXTURE_TYPES ) );

	return gszTextureTypes[ eType ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline bool IsPower2(unsigned int x)
{              
	if ( x < 1 )
		return false;

	if (x == 1)
		return true;

	if ( x & (x-1) )        
		return false;

	return true;
}

inline bool IsMultiple4(unsigned int x)
{              
	if ( x == 0 )
		return false;

	if ((x % 4) == 0)
		return true;
	else
		return false;
}


//-------------------------------------------------------------------------------------------------
// Texture Reporting
//-------------------------------------------------------------------------------------------------

#if ENABLE_TEXTURE_REPORT

// allocate +1 for the "no group/type" slots
#define NUM_TEXTURE_REPORT_GROUPS	( NUM_TEXTURE_GROUPS + 1 )
#define NUM_TEXTURE_REPORT_TYPES	( NUM_TEXTURE_TYPES  + 1 )
struct TextureReport
{
	unsigned int nCounts [ NUM_TEXTURE_REPORT_GROUPS ][ NUM_TEXTURE_REPORT_TYPES ];
	unsigned int nSize   [ NUM_TEXTURE_REPORT_GROUPS ][ NUM_TEXTURE_REPORT_TYPES ];
};
static TextureReport sgtTextureReportTotal;
static TextureReport sgtTextureReportUse;
static BOOL sgbTextureReportUseNeedCompile = TRUE;
struct TextureUsage
{
	int			 * pnIDs;
	int			   nAlloc;
	int			   nUsed;
};
static TextureUsage sgtTextureUsage = { NULL, 0, 0 };

static inline void sAddToTextureReport( TextureReport & tReportStruct, const D3D_TEXTURE * pTexture )
{
	if ( ! pTexture )
		return;
	int nGroup = pTexture->nGroup == TEXTURE_GROUP_NONE ? NUM_TEXTURE_GROUPS : pTexture->nGroup;
	int nType  = pTexture->nType  == TEXTURE_NONE       ? NUM_TEXTURE_TYPES  : pTexture->nType;
	tReportStruct.nCounts [ nGroup ][ nType ]++;
	tReportStruct.nSize   [ nGroup ][ nType ] += dx9_GetTextureMIPLevelSizeInBytes( pTexture );
}

static int sTextureIDSortCompare( const void * elem1, const void * elem2 )
{
	int i1 = *(int*)elem1;
	int i2 = *(int*)elem2;
	if ( i1 < i2 ) return -1;
	if ( i1 > i2 ) return 1;
	return 0;
}

static inline void sResetTextureReportStats( TextureReport & tReportStruct )
{
	ZeroMemory( tReportStruct.nCounts,  sizeof(unsigned int) * NUM_TEXTURE_REPORT_GROUPS * NUM_TEXTURE_REPORT_TYPES );
	ZeroMemory( tReportStruct.nSize,    sizeof(unsigned int) * NUM_TEXTURE_REPORT_GROUPS * NUM_TEXTURE_REPORT_TYPES );
}

static void sCompileTextureUsedStats()
{
	sResetTextureReportStats( sgtTextureReportUse );
	qsort( sgtTextureUsage.pnIDs, sgtTextureUsage.nUsed, sizeof(int), sTextureIDSortCompare );
	int nLastID = INVALID_ID;
	for ( int i = 0; i < sgtTextureUsage.nUsed; i++ )
	{
		ASSERT( sgtTextureUsage.pnIDs[ i ] != INVALID_ID );
		if ( sgtTextureUsage.pnIDs[ i ] == nLastID )
			continue;
		D3D_TEXTURE * pTex = dxC_TextureGet( sgtTextureUsage.pnIDs[ i ] );
		sAddToTextureReport( sgtTextureReportUse, pTex );
		nLastID = sgtTextureUsage.pnIDs[ i ];
	}
	sgbTextureReportUseNeedCompile = FALSE;
}

#endif // ENABLE_TEXTURE_REPORT



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_Get2DTextureDesc( const LPD3DCTEXTURE2D pTex, UINT nLevel, D3DC_TEXTURE2D_DESC* pDesc)
{
	ASSERT_RETINVALIDARG( pTex );
	DX9_BLOCK( V_RETURN( pTex->GetLevelDesc( nLevel, pDesc ) );)

#ifdef ENGINE_TARGET_DX10
	pTex->GetDesc( pDesc );
	pDesc->Width >>= nLevel;
	pDesc->Height >>= nLevel;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_Get2DTextureDesc( const D3D_TEXTURE * pTexture, UINT nLevel, D3DC_TEXTURE2D_DESC* pDesc)
{
	ASSERT_RETINVALIDARG( pTexture );
	if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
	{
		if ( pTexture->ptSysmemTexture )
			return pTexture->ptSysmemTexture->GetDesc( nLevel, pDesc );
	}
	return dxC_Get2DTextureDesc( pTexture->pD3DTexture, nLevel, pDesc );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if defined(ENGINE_TARGET_DX10)
inline void sFillLoadInfo( UINT Width, UINT Height, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT fmt, D3DCX_FILTER texFilter, D3DCX_FILTER mipFilter, UINT D3D10MiscFlags, D3DX10_IMAGE_LOAD_INFO* pInfoLoad )
{
	if( !pInfoLoad )
		return;

	pInfoLoad->Width = Width != 0 ? Width : pInfoLoad->Width;
	pInfoLoad->Height = Height != 0 ? Height : pInfoLoad->Height;
	//pInfoLoad->Depth = 1;

	pInfoLoad->BindFlags = dx10_GetBindFlags( usage );
	pInfoLoad->CpuAccessFlags = dx10_GetCPUAccess( usage );
	pInfoLoad->Usage = dx10_GetUsage( usage );
	
	pInfoLoad->Filter = texFilter;
	pInfoLoad->MipFilter = mipFilter & ~( D3DC_SKIP_DDS_MIP_LEVELS_MASK << D3DC_SKIP_DDS_MIP_LEVELS_SHIFT );
	pInfoLoad->FirstMipLevel = ( mipFilter >> D3DC_SKIP_DDS_MIP_LEVELS_SHIFT ) & D3DC_SKIP_DDS_MIP_LEVELS_MASK;
	// CHB 2007.09.05
	//pInfoLoad->MipLevels = MipLevels != 0 ? MipLevels + pInfoLoad->FirstMipLevel : pInfoLoad->MipLevels;
	pInfoLoad->MipLevels = MipLevels != 0 ? MipLevels : pInfoLoad->MipLevels;
	pInfoLoad->Format = fmt == D3DFMT_UNKNOWN ? D3DFMT_FROM_FILE : fmt;  //DX10 Creation calls can't handle unknown format
	pInfoLoad->MiscFlags = ( pInfoLoad->MipLevels == 0 ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0 ) | D3D10MiscFlags;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int dx9_GetDXTBlockSizeInBytes( DXC_FORMAT eFormat )
{
	ASSERTX_RETZERO( dx9_IsCompressedTextureFormat( eFormat ), "Error, uncompressed texture format specified!" );

	switch( eFormat )
	{
	case D3DFMT_DXT1:	return 8;
		DX9_BLOCK( case D3DFMT_DXT2: )
	case D3DFMT_DXT3:
		DX9_BLOCK( case D3DFMT_DXT4: )
			DX10_BLOCK( case DXGI_FORMAT_BC5_UNORM: )
	case D3DFMT_DXT5:	return 16;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD dxC_GetTextureLevelSize( UINT nWidth, UINT nHeight, DXC_FORMAT tFormat )
{
	ASSERT_RETZERO( nWidth > 0 && nHeight > 0 && tFormat != D3DFMT_UNKNOWN );

	if ( dx9_IsCompressedTextureFormat( tFormat ) )
	{
		// DXTn
		// max(1, width ?4) x max(1, height ?4) x 8(DXT1) or 16(DXT2-5)
		return MAX(1U, (nWidth / 4)) * MAX(1U, (nHeight / 4)) * dx9_GetDXTBlockSizeInBytes( tFormat );
	}

	// uncompressed
	return nHeight * dxC_GetTexturePitch( nWidth, tFormat );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BYTE * SYSMEM_TEXTURE::GetLevel( UINT nLevel )
{
	if ( ! ppLevels )
		return NULL;
	if ( ! pbData )
		return NULL;
	if ( nLevel >= nLevels )
		return NULL;

	return ppLevels[ nLevel ];
}

DWORD SYSMEM_TEXTURE::GetLevelSize( UINT nLevel )
{
	if ( nLevel >= nLevels )
		return 0;

	int nX = MAX(1U, (nWidth  >> nLevel));
	int nY = MAX(1U, (nHeight >> nLevel));

	return dxC_GetTextureLevelSize( nX, nY, tFormat );
}


PRESULT SYSMEM_TEXTURE::GetDesc( UINT nLevel, D3DC_TEXTURE2D_DESC * pDesc )
{
	if ( nLevel >= nLevels )
		return E_INVALIDARG;
	if ( ! pDesc )
		return E_INVALIDARG;

	pDesc->Width = nWidth >> nLevel;
	pDesc->Height = nHeight >> nLevel;
	pDesc->Format = tFormat;

	return S_OK;
}


PRESULT SYSMEM_TEXTURE::Lock( UINT nLevel, D3DLOCKED_RECT * pRect )
{
	if ( ! pRect )
		return E_INVALIDARG;

	pRect->pBits = NULL;
	pRect->Pitch = 0;

	if ( nLevel >= nLevels )
		return E_INVALIDARG;
	if ( ! ppLevels )
		return E_FAIL;
	if ( ! pbData )
		return E_FAIL;
	if ( TESTBIT_DWORD( dwLockedLevels, nLevel ) )
		return E_FAIL;		// level already locked

	pRect->pBits = GetLevel( nLevel );
	pRect->Pitch = dxC_GetTexturePitch( nWidth >> nLevel, tFormat );

	SETBIT( dwLockedLevels, nLevel, TRUE );
	return S_OK;
}

PRESULT SYSMEM_TEXTURE::Unlock( UINT nLevel )
{
	if ( ! ppLevels )
		return E_FAIL;
	if ( ! pbData )
		return E_FAIL;
	if ( nLevel >= nLevels )
		return E_INVALIDARG;

	if ( ! TESTBIT_DWORD( dwLockedLevels, nLevel ) )
		return E_FAIL;		// level not locked

	SETBIT( dwLockedLevels, nLevel, FALSE );
	return S_OK;
}

DWORD SYSMEM_TEXTURE::GetSize()
{
	DWORD dwSize = 0;
	for ( UINT i = 0; i < nLevels; ++i )
		dwSize += GetLevelSize( i );

	return dwSize;
}

void SYSMEM_TEXTURE::Free()
{
	ASSERTV( dwLockedLevels == 0, "Freeing system memory texture while locked!  Lockflags: 0x%08x", dwLockedLevels );

	if ( ppLevels )
	{
		FREE( pPool, ppLevels );
		ppLevels = NULL;
	}
	if ( pbData )
	{
		FREE( pPool, pbData );
		pbData = NULL;
	}
}

//----------------------------------------------------------------------------

static PRESULT sSystemMemoryTextureCreate(
	UINT Width,
	UINT Height,
	UINT MipLevels,
	DXC_FORMAT Format,
	SYSMEM_TEXTURE *& pTexture,
	UINT & nSizeInMemory,
	MEMORYPOOL* pPool )
{
	ASSERT_RETINVALIDARG( Width > 0 );
	ASSERT_RETINVALIDARG( Height > 0 );
	ASSERT_RETINVALIDARG( MipLevels > 0 );
	ASSERT_RETINVALIDARG( Format != D3DFMT_UNKNOWN );

	pTexture = (SYSMEM_TEXTURE*)MALLOCZ( pPool, sizeof(SYSMEM_TEXTURE) );
	ASSERT_RETVAL( pTexture, E_OUTOFMEMORY );
	pTexture->nWidth  = Width;
	pTexture->nHeight = Height;
	pTexture->tFormat = Format;
	pTexture->nLevels = MipLevels;
	pTexture->pPool	  = pPool;

	nSizeInMemory = pTexture->GetSize();

	pTexture->pbData = (BYTE*)MALLOC( pPool, nSizeInMemory );
	ASSERT_RETVAL( pTexture->pbData, E_OUTOFMEMORY );
	pTexture->ppLevels = (BYTE**)MALLOC( pPool, sizeof(BYTE*) * pTexture->nLevels );
	ASSERT_RETVAL( pTexture->ppLevels, E_OUTOFMEMORY );

	// Fill in MIP level pointers

	UINT nOffset = 0;
	for ( UINT i = 0; i < pTexture->nLevels; ++i )
	{
		pTexture->ppLevels[i] = pTexture->pbData + nOffset;
		nOffset += pTexture->GetLevelSize( i );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
// Creates a system memory version of an existing texture in place in memory.
PRESULT dxC_TextureSystemMemNewEmptyInPlace( D3D_TEXTURE * pTexture )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETFAIL( ! pTexture->ptSysmemTexture );
	DWORD nMipMapLevels = 0;
	if ( pTexture->nDefinitionId != INVALID_ID )
	{
		TEXTURE_DEFINITION * pTextureDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		if ( pTextureDef )
		{
			nMipMapLevels = pTextureDef->nMipMapLevels;
		}
	}
	if ( nMipMapLevels == 0 )
		nMipMapLevels = e_GetNumMIPLevels( max( pTexture->nWidthInPixels, pTexture->nHeight ) );

	MEMORYPOOL* pPool = NULL;
	if ( TEST_MASK( pTexture->dwFlags, TEXTURE_FLAG_USE_SCRATCHMEM ) )
		pPool = g_ScratchAllocator;

	UINT nSizeInMemory = 0;
	V_RETURN( sSystemMemoryTextureCreate(
		pTexture->nWidthInPixels,
		pTexture->nHeight,
		nMipMapLevels,
		(DXC_FORMAT)pTexture->nFormat,
		pTexture->ptSysmemTexture,
		nSizeInMemory,
		pPool ) );
	ASSERT_RETFAIL( pTexture->ptSysmemTexture );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_Create2DSysMemTextureFromFileInMemory(
	const void * pSrcData,
	UINT SrcDataSize,
	UINT Width,
	UINT Height,
	UINT LoadMipLevels,
	UINT SkipMipLevels,
	DXC_FORMAT fmt,
	D3DXIMAGE_INFO *pImageInfo,
	SYSMEM_TEXTURE *& pTexture,
	MEMORYPOOL* pPool )
{
	ASSERT_RETINVALIDARG( pSrcData );
	ASSERT_RETINVALIDARG( SrcDataSize > 0 );
	ASSERT_RETINVALIDARG( pImageInfo );

	V_RETURN( dxC_GetImageInfoFromFileInMemory( pSrcData, SrcDataSize, pImageInfo ) );

	ASSERT_RETFAIL( fmt == pImageInfo->Format || fmt == D3DFMT_UNKNOWN );

	SkipMipLevels = MIN( SkipMipLevels, pImageInfo->MipLevels - 1 );

	// In Pixomatic mode, skip the top MIP level
	if ( dxC_IsPixomaticActive() )
	{
		UINT nMaxSkip = LoadMipLevels - 1;
		SkipMipLevels = MIN( SkipMipLevels + PIXOMATIC_SKIP_MIP_LEVELS, nMaxSkip );
	}

	if ( Width == 0 )
		Width = pImageInfo->Width;
	if ( Height == 0 )
		Height = pImageInfo->Height;
	if ( LoadMipLevels == 0 )
		LoadMipLevels = pImageInfo->MipLevels - SkipMipLevels;
	if ( fmt == D3DFMT_UNKNOWN )
		fmt = pImageInfo->Format;

	LoadMipLevels = MIN( LoadMipLevels, (pImageInfo->MipLevels - SkipMipLevels) );

	// Adjust the width/height downwards if we are skipping top MIP levels.
	for ( UINT i = 0; i < SkipMipLevels; ++i )
	{
		Width  = MAX(1U, Width  >> 1);
		Height = MAX(1U, Height >> 1);
	}

	ASSERT_RETFAIL( SrcDataSize > sizeof(DDS_FILE_HEADER) );

	const DDS_FILE_HEADER & tHeader = *(DDS_FILE_HEADER*)pSrcData;

	// Make sure we're loading a DDS file.
	ASSERTX_RETFAIL( tHeader.dwMagic == DDS_FILE_MAGIC, "Not a valid DDS file!" );
	ASSERTX_RETFAIL( tHeader.tDesc.dwSize == sizeof(DDCSURFACEDESC), "Not a valid DDS file!" );

	ASSERT_RETFAIL(    (tHeader.tDesc.dwFlags & DDSD_CAPS )
					&& (tHeader.tDesc.dwFlags & DDSD_PIXELFORMAT )
					&& (tHeader.tDesc.dwFlags & DDSD_WIDTH )
					&& (tHeader.tDesc.dwFlags & DDSD_HEIGHT ) );


	BOOL bHasMips =    !!( tHeader.tDesc.ddsCaps.dwCaps1 & DDSCAPS_COMPLEX )
					&& !!( tHeader.tDesc.ddsCaps.dwCaps1 & DDSCAPS_MIPMAP )
					&& !!( tHeader.tDesc.dwFlags		 & DDSD_MIPMAPCOUNT );


	const BYTE * pFile = (BYTE*)pSrcData + sizeof(DDS_FILE_HEADER);
	UINT nTextureSize = SrcDataSize - sizeof(DDS_FILE_HEADER);

	// If we are to skip mip levels, figure out the new offset and size
	for ( UINT i = 0; i < SkipMipLevels; ++i )
	{
		int nLevelWidth  = max(1, pImageInfo->Width  >> i);
		int nLevelHeight = max(1, pImageInfo->Height >> i);
		int nLevelSize = dxC_GetTextureLevelSize( nLevelWidth, nLevelHeight, fmt );
		pFile += nLevelSize;
		nTextureSize -= nLevelSize;
	}

	UINT nFinalTextureSize = nTextureSize;
	DXC_FORMAT tFinalFormat = fmt;

	if ( dxC_IsPixomaticActive() )
	{
		tFinalFormat = PIXO_TEXTURE_FORMAT;

		nFinalTextureSize = 0;
		int nWidth  = pImageInfo->Width >> SkipMipLevels;
		int nHeight = pImageInfo->Height >> SkipMipLevels;
		BOUNDED_WHILE( 1, 1000 )
		{
			nFinalTextureSize += dxC_GetTextureLevelSize( nWidth, nHeight, tFinalFormat );
			if ( nWidth == 1 && nHeight == 1 )
				break;
			nWidth  = max( 1, nWidth  >> 1 );
			nHeight = max( 1, nHeight >> 1 );
		}
	}

	V_RETURN( sSystemMemoryTextureCreate(
		Width,
		Height,
		LoadMipLevels,
		tFinalFormat,
		pTexture,
		nFinalTextureSize,
		pPool ) );

	ASSERT( pTexture->nLevels == ( pImageInfo->MipLevels - SkipMipLevels ) );

	if ( ! dxC_IsPixomaticActive() || tFinalFormat == fmt )
	{
		ASSERT( nTextureSize == nFinalTextureSize );
		MemoryCopy( pTexture->pbData, nFinalTextureSize, pFile, nTextureSize );
	}
	else if ( dxC_IsPixomaticActive() )
	{
		// Must convert to new format
		ASSERT_RETFAIL( dx9_IsCompressedTextureFormat( fmt ) );

		DWORD dwFileOffset = 0;

		for ( UINT i = 0; i < pTexture->nLevels; ++i )
		{
			BYTE * pLevel = pTexture->GetLevel( i );
			ASSERT_CONTINUE( pLevel );

			int nDestWidth  = max( 1, pTexture->nWidth  >> i );
			int nDestHeight = max( 1, pTexture->nHeight >> i );
			// Minimum DXT level size is 4x4, even if it would normally be 1x1 or even 4x2.
			int nSrcWidth   = max( 4, nDestWidth );
			int nSrcHeight  = max( 4, nDestHeight );

			V_RETURN( dx9_TextureLevelDecompress(
				nSrcWidth,
				nSrcHeight,
				fmt,
				pFile + dwFileOffset,
				nDestWidth,
				nDestHeight,
				tFinalFormat,
				pLevel ) );

			// dxC_GetTextureLevelSize handles the 4x4 minimum internally, if necessary.  In this case, it isn't.
			dwFileOffset += dxC_GetTextureLevelSize( nSrcWidth, nSrcHeight, fmt );
		}
	}


	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_Create2DTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, D3DCX_FILTER texFilter, D3DCX_FILTER mipFilter, D3DXIMAGE_INFO *pImageInfo, LPD3DCTEXTURE2D* ppTexture, UINT D3D10MiscFlags )
{
	ASSERT_RETFAIL( ! dxC_IsPixomaticActive() );

#ifdef ENGINE_TARGET_DX9
	V_RETURN( D3DXCreateTextureFromFileInMemoryEx( dxC_GetDevice(), pSrcData, SrcDataSize, Width, Height, MipLevels, dx9_GetUsage( usage ), fmt, dx9_GetPool( usage ), texFilter, mipFilter, 0, pImageInfo, NULL, ppTexture) );
	return S_OK;
#elif defined(ENGINE_TARGET_DX10)
		SPD3DCBASETEXTURE pRes = NULL;

		// CHB 2007.09.05
		/*
			The previous method of handling texture detail does not
			work. Setting FirstMipLevel of D3DX10_IMAGE_LOAD_INFO
			has no effect, except perhaps to set an internal offset
			or bias to point to a particular subresource. The
			texture is in fact created at full size with all mip
			levels. It doesn't actually result in the discarding of
			any mip levels. Although drawing mip level 0 may result
			in the drawing of the selected mip level, locking the
			bits for mip level 0 still gives you the actual high-res
			level 0 of the full texture.
			
			Note that the D3DX10_IMAGE_LOAD_INFO struct determines
			the attributes of the resulting output texture resource.

			One way to resolve this issue is to set the Width and
			Height members of D3DX10_IMAGE_LOAD_INFO to the desired
			final width and height taking into account the specified
			mip reduction factor. This works, however the
			documentation for D3DX10CreateTextureFromMemory states
			that it will scale textures that do not fit. It's not
			clear if D3DX10 is smart enough to start from a lower
			mip level of the source data in that case, or if it
			simply scales the highest resolution mip level to fit
			into the target.

			Also, it is not possible to remove individual
			subresources (mip levels) from a texture resource in
			DX10.

			Thus the method below was chosen, which:
				-	creates a final texture of the appropriate
					dimensions and mip levels;
				-	loads the full source image (and mips) into a
					separate texture;
				-	copies the desired mip levels from the
					temporary texture to the final texture;
				-	then finally discards the temporary texture.

			The primary concern regarding this approach is that
			CopySubresourceRegion() engages the pipeline, which has
			the theoretical potential to introduce synchronization
			or stalls (though this has not yet been observed in
			practice, as of this writing).

			It may be necessary to implement a custom thread pump to
			deal with this.

			Now, what about other texture types like 3D cube
			textures, etc.?
		*/
#define DX10_HANDLE_MIP_REDUCTION 1		// CHB 2007.09.05
#if DX10_HANDLE_MIP_REDUCTION
		unsigned nMipDown = (mipFilter >> D3DC_SKIP_DDS_MIP_LEVELS_SHIFT) & D3DC_SKIP_DDS_MIP_LEVELS_MASK;
		mipFilter = static_cast<D3DCX_FILTER>(mipFilter & ~(D3DC_SKIP_DDS_MIP_LEVELS_MASK << D3DC_SKIP_DDS_MIP_LEVELS_SHIFT));
		unsigned nFinalMipLevels = MipLevels;
		// Create final texture first to avoid fragmentation.
		SPD3DCTEXTURE2D pFinalTexture;
		if (nMipDown > 0)
		{
			D3DX10_IMAGE_INFO Info;
			V_RETURN(::D3DX10GetImageInfoFromMemory(pSrcData, SrcDataSize, 0, &Info, 0));
			ASSERT(Info.ArraySize == 1);	// If this ever trips, will need to add an array iterating loop below
			ASSERT_RETFAIL(Info.MipLevels > nMipDown);
			ASSERT_RETFAIL((MipLevels == 0) || (Info.MipLevels >= MipLevels));

			nFinalMipLevels = (MipLevels == 0) ? (Info.MipLevels - nMipDown) : MipLevels;

			D3D10_TEXTURE2D_DESC Desc;
			Desc.Width = ((Width == 0) ? Info.Width : Width) >> nMipDown;
			Desc.Height = ((Height == 0) ? Info.Height : Height) >> nMipDown;
			Desc.MipLevels = nFinalMipLevels;
			Desc.ArraySize = Info.ArraySize;
			Desc.Format = ((fmt == D3DFMT_UNKNOWN) || (fmt == D3DFMT_FROM_FILE)) ? Info.Format : fmt;
			Desc.SampleDesc.Count = 1;
			Desc.SampleDesc.Quality = 0;
			Desc.Usage = dx10_GetUsage(usage);
			Desc.BindFlags = dx10_GetBindFlags(usage);
			Desc.CPUAccessFlags = dx10_GetCPUAccess(usage);
			Desc.MiscFlags = D3D10MiscFlags;

			V_RETURN(dxC_GetDevice()->CreateTexture2D(&Desc, 0, &pFinalTexture));
			ASSERT_RETFAIL(pFinalTexture != 0);
			if (MipLevels > 0)
			{
				MipLevels += nMipDown;
			}
		}
#endif

		D3DX10_IMAGE_LOAD_INFO LoadInfo;
		sFillLoadInfo( Width, Height, MipLevels, usage, fmt, texFilter, mipFilter, D3D10MiscFlags, &LoadInfo );

		V_DO_FAILED( D3DX10CreateTextureFromMemory( dxC_GetDevice(), pSrcData, SrcDataSize, &LoadInfo, NULL, &pRes, NULL ) ) //KMNV TODO THREADPUMP
		{
			int width, height;
			unsigned char* tgaData = (unsigned char*) tga_load( (unsigned char*)pSrcData, SrcDataSize, &width, &height, TGA_TRUECOLOR_32 );

			if( !tgaData )
				return E_FAIL;

			//Our image is reversed...need to 
			UINT rowSize = width * 4;
			UINT byteCount = height * rowSize;
			unsigned char* tgaDataReversed = (unsigned char*)MALLOCZ( g_ScratchAllocator,  byteCount );


			for( int i = 0; i < height; ++i )
			{
				memcpy( &tgaDataReversed[ i * rowSize ], &tgaData[ ( height - i - 1 ) * rowSize ], rowSize );
			}

			V_DO_FAILED( dxC_Create2DTexture( width, height, 1, usage, DXGI_FORMAT_R8G8B8A8_UNORM, ppTexture, (void*)tgaDataReversed, 0) )
			{
				FREE( g_ScratchAllocator, tgaDataReversed );
				FREE( g_ScratchAllocator, tgaData );
				return E_FAIL;
			}
			FREE( g_ScratchAllocator, tgaDataReversed );
			FREE( g_ScratchAllocator, tgaData );
		}
		else
		{
			if( pRes )
				pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (LPVOID*)ppTexture );
			pRes = NULL;
		}

	ASSERT_RETFAIL( *ppTexture );
	ASSERT_RETFAIL( pImageInfo );

	D3DC_TEXTURE2D_DESC Desc;
	V_RETURN( dxC_Get2DTextureDesc( (*ppTexture), 0, &Desc ) );

	// CHB 2007.09.05
#if DX10_HANDLE_MIP_REDUCTION
	if (pFinalTexture != 0)
	{
		for (unsigned nLevel = 0; nLevel < nFinalMipLevels; ++nLevel)
		{
			dxC_GetDevice()->CopySubresourceRegion(
				pFinalTexture,
				D3D10CalcSubresource(nLevel, 0, nFinalMipLevels),
				0,
				0,
				0,
				*ppTexture,
				D3D10CalcSubresource(nLevel + nMipDown, 0, Desc.MipLevels),
				0
			);
		}

		(*ppTexture)->Release();
		*ppTexture = pFinalTexture;
		(*ppTexture)->AddRef();

		V_RETURN( dxC_Get2DTextureDesc( (*ppTexture), 0, &Desc ) );
	}
#endif

	pImageInfo->Depth = 1;
	pImageInfo->Width = Desc.Width;
	pImageInfo->Height = Desc.Height;
	pImageInfo->MipLevels = Desc.MipLevels;
	pImageInfo->ArraySize = D3D10MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ? 6 : 1;
	pImageInfo->Format = Desc.Format;
		
	pImageInfo->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
	pImageInfo->ImageFileFormat = D3DX10_IFF_DDS; //HACK: It was TGA which DX10 doesn't support	

	return S_OK;
#endif
}

//----------------------------------------------------------------------------

PRESULT dxC_Create2DTextureFromFileEx( LPCTSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT fmt, D3DCX_FILTER texFilter, D3DCX_FILTER mipFilter, D3DXIMAGE_INFO *pImageInfo, LPD3DCTEXTURE2D* ppTexture, UINT D3D10MiscFlags )
{
	ASSERTX_RETVAL(0, E_NOTIMPL, "Do not use this function!  Instead, use the normal texture file load functions!")
#if defined(ENGINE_TARGET_DX9)
	return D3DXCreateTextureFromFileEx( dxC_GetDevice(), pSrcFile, Width, Height, MipLevels, dx9_GetUsage( usage ), fmt, dx9_GetPool( usage ), texFilter, mipFilter, NULL, pImageInfo, NULL, ppTexture );
#elif defined(ENGINE_TARGET_DX10)
	SPD3DCBASETEXTURE pRes = NULL;
	D3DX10_IMAGE_LOAD_INFO LoadInfo;
	
	sFillLoadInfo( Width, Height, MipLevels, usage, fmt, texFilter, mipFilter, D3D10MiscFlags, &LoadInfo );
	LoadInfo.pSrcInfo = pImageInfo;

	HRESULT hr = D3DX10CreateTextureFromFile( dxC_GetDevice(), pSrcFile, &LoadInfo, NULL, &pRes, NULL );//KMNV TODO Thread pump
	if( FAILED( hr ) )  {
		ASSERTX( NULL, "No longer support TGA conversion here!" );
		//ASSERTX(LoadInfo.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "NUTTAPONG TODO: Support only 8 bits for TGA loading for now");
		//// Try to use TGA loader
		//int width, height;
		//unsigned char* tgaData = (unsigned char*) tga_load( pSrcFile, &width, &height, TGA_TRUECOLOR_32 );	
		//hr = dxC_Create2DTexture((UINT)width, (UINT)height, 1, usage, LoadInfo.Format, ppTexture, (void *)tgaData, LoadInfo.MiscFlags);
		//delete [] tgaData;
	} else {
		if( pRes )
				pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (LPVOID*)ppTexture );

		pRes = NULL;
	}
	return hr;
#endif
}

//----------------------------------------------------------------------------

PRESULT dxC_CreateCubeTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Size, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT Format, D3DCX_FILTER Filter, D3DCX_FILTER MipFilter, D3DXIMAGE_INFO * pSrcInfo, LPD3DCTEXTURECUBE * ppCubeTexture)
{
	DX9_BLOCK( V_RETURN( D3DXCreateCubeTextureFromFileInMemoryEx( dxC_GetDevice(), pSrcData, SrcDataSize, Size, MipLevels, dx9_GetUsage( usage ), Format, dx9_GetPool( usage ), Filter, MipFilter, 0, pSrcInfo, NULL, ppCubeTexture) ); )
	DX10_BLOCK( V_RETURN( dxC_Create2DTextureFromFileInMemoryEx( pSrcData, SrcDataSize, Size, Size, MipLevels, usage, Format, Filter, MipFilter, pSrcInfo, ppCubeTexture, D3D10_RESOURCE_MISC_TEXTURECUBE ) ); )
	//DX10_BLOCK( ASSERTX(0, "KMNV TODO: Need D3DX cubemap functionality!, NUTTAPONG how do we load a cubemap in DX10?"); return E_FAIL; )
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_UpdateTexture( LPD3DCTEXTURE2D pTex, CONST RECT* pDestRect, void* pData, UINT nLevel, DWORD D3D9Flags )
{
	//D3DLOCKED_RECT tLockedRect;
	//dxC_GetDevice()->LockRect( nLevel, &tLockedRect, pDestRect, D3D9Flags );
	//memcpy(
	return E_NOTIMPL;
}

PRESULT dxC_UpdateTexture( LPD3DCTEXTURE2D pTex, CONST RECT* pDestRect, void* pData, CONST RECT* pSrcRect, UINT nLevel, DXC_FORMAT fmt, UINT SrcPitch )
{
#ifdef ENGINE_TARGET_DX9
	SPDIRECT3DSURFACE9 pDestSurface;
	V_RETURN( pTex->GetSurfaceLevel( nLevel, &pDestSurface ) );
	V_RETURN( D3DXLoadSurfaceFromMemory( pDestSurface, NULL, pDestRect, pData, fmt, SrcPitch, NULL, pSrcRect, D3DX_FILTER_NONE, 0 ) );
#elif defined(ENGINE_TARGET_DX10)
	ASSERTX_RETVAL(0, E_NOTIMPL, "KMNV TODO get off your ASS!" );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_Create2DTexture(UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE2D* ppTexture, void* pInitialData, UINT D3D10MiscFlags, UINT nMSAASamples, UINT nMSAAQuality )
{
#ifdef ENGINE_TARGET_DX9
	ASSERT_RETINVALIDARG( nMSAASamples <= 1 );

	V_RETURN( D3DXCreateTexture( dxC_GetDevice(), Width, Height, MipLevels, dx9_GetUsage( usage ), fmt, dx9_GetPool( usage ), ppTexture) );

	if( pInitialData )
	{
		SPDIRECT3DSURFACE9 pDestSurface;
		V_RETURN( (*ppTexture)->GetSurfaceLevel( 0, &pDestSurface ) );
		
		RECT  tRect;
		SetRect( &tRect, 0, 0, Width, Height );
		UINT nPitch = dxC_GetTexturePitch( Width, fmt );
		DWORD dwFilter = D3DX_FILTER_POINT;
		V_RETURN( D3DXLoadSurfaceFromMemory( pDestSurface,
			NULL,
			NULL,
			pInitialData,
			fmt,
			nPitch,
			NULL,
			&tRect,
			dwFilter,
			0 ) );
		pDestSurface = NULL;
	}

	return S_OK;

#elif defined(ENGINE_TARGET_DX10)
	D3D10_TEXTURE2D_DESC Desc;
	Desc.ArraySize = D3D10MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ? 6 : 1;
	Desc.BindFlags = dx10_GetBindFlags( usage );
	Desc.CPUAccessFlags = dx10_GetCPUAccess( usage );
	Desc.Format = fmt;
	Desc.MipLevels = MipLevels;
	Desc.MiscFlags = (MipLevels == 0 && usage == D3DC_USAGE_2DTEX_RT ) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0 | D3D10MiscFlags;
	Desc.SampleDesc.Count = nMSAASamples == 0 ? 1 : nMSAASamples;
	Desc.SampleDesc.Quality = nMSAAQuality; 
	Desc.Usage = dx10_GetUsage( usage );
	Desc.Width = Width;
	Desc.Height = Height;

	if( pInitialData )
	{
		D3D10_SUBRESOURCE_DATA subData;
		subData.pSysMem = pInitialData;
		subData.SysMemPitch = dxC_GetTexturePitch( Width, fmt );
		subData.SysMemSlicePitch = 0;

		V_RETURN( dx10_CreateTexture2D( &Desc, &subData, ppTexture ) );
	}
	else
	{
		V_RETURN( dx10_CreateTexture2D( &Desc, NULL, ppTexture ) );
	}

	return S_OK;
#endif
}

//----------------------------------------------------------------------------

PRESULT dxC_CreateCubeTexture(UINT Size, UINT MipLevels, D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURECUBE* ppCubeTexture)
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	DX9_BLOCK( V_RETURN( D3DXCreateCubeTexture( dxC_GetDevice(), Size, MipLevels, dx9_GetUsage( usage ), fmt, dx9_GetPool( usage ), ppCubeTexture) ); )
	DX10_BLOCK( V_RETURN( dxC_Create2DTexture( Size, Size, MipLevels, usage, fmt, ppCubeTexture, NULL, D3D10_RESOURCE_MISC_TEXTURECUBE ) ); )
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_AddTextureUsedStats( int eGroup, int eType, int & nCount, int & nSize )
{
#if ENABLE_TEXTURE_REPORT
	int nStartGroup, nEndGroup;
	int nStartType, nEndType;

	if ( sgbTextureReportUseNeedCompile )
		sCompileTextureUsedStats();

	if ( eGroup == TEXTURE_GROUP_NONE )
	{
		nStartGroup = 0;
		nEndGroup = NUM_TEXTURE_REPORT_GROUPS;	// including the "unspecified" group
	} else
	{
		nStartGroup = eGroup;
		nEndGroup = eGroup + 1;
		ASSERT_RETINVALIDARG( eGroup < NUM_TEXTURE_REPORT_GROUPS );
	}

	if ( eType == TEXTURE_NONE )
	{
		nStartType = 0;
		nEndType = NUM_TEXTURE_REPORT_TYPES;	// including the "unspecified" type
	} else
	{
		nStartType = eType;
		nEndType = eType + 1;
		ASSERT_RETINVALIDARG( eType < NUM_TEXTURE_REPORT_TYPES );
	}

	for ( int nGroup = nStartGroup; nGroup < nEndGroup; nGroup++ )
	{
		for ( int nType = nStartType; nType < nEndType; nType++ )
		{
			nCount += sgtTextureReportUse.nCounts[ nGroup ][ nType ];
			nSize += sgtTextureReportUse.nSize[ nGroup ][ nType ];
		}
	}

#else
	REF( eGroup );
	REF( eType );
	REF( nCount );
	REF( nSize );
#endif // ENABLE_TEXTURE_REPORT

	return S_OK;
}

#if ENABLE_TEXTURE_REPORT
static inline void sReportTextureTotal( const D3D_TEXTURE * pTexture )
{
	sAddToTextureReport( sgtTextureReportTotal, pTexture );
}
#endif // ENABLE_TEXTURE_REPORT

//----------------------------------------------------------------------------

PRESULT dx9_ReportTextureUse( const D3D_TEXTURE * pTexture )
{
#if ENABLE_TEXTURE_REPORT

	const int nElemSize = sizeof(int);
	// add to texture usage
	if ( ( ( sgtTextureUsage.nUsed + 1 ) * nElemSize ) >= sgtTextureUsage.nAlloc )
	{
		if ( sgtTextureUsage.nAlloc == 0 )
		{
			ASSERT_RETFAIL( sgtTextureUsage.pnIDs == NULL );
			sgtTextureUsage.nAlloc   = 32 * sizeof(int);
			sgtTextureUsage.pnIDs    = (int*) MALLOC( NULL, sgtTextureUsage.nAlloc );
			sgtTextureUsage.nUsed    = 0;
		} else {
			sgtTextureUsage.nAlloc   *= 2;
			sgtTextureUsage.pnIDs    = (int*) REALLOC( NULL, sgtTextureUsage.pnIDs, sgtTextureUsage.nAlloc );
		}
	}
	ASSERT_RETFAIL( sgtTextureUsage.pnIDs );

	sgtTextureUsage.pnIDs[ sgtTextureUsage.nUsed ] = pTexture->nId;
	sgtTextureUsage.nUsed++;

	sgbTextureReportUseNeedCompile = TRUE;

#else
	REF( pTexture );
#endif // ENABLE_TEXTURE_REPORT

	return S_OK;
}

//----------------------------------------------------------------------------

void dx9_ResetTextureUseStats()
{
#if ENABLE_TEXTURE_REPORT
	if(sgtTextureUsage.pnIDs)
	{
		ZeroMemory( sgtTextureUsage.pnIDs, sgtTextureUsage.nUsed );
	}
	sgtTextureUsage.nUsed = 0;
#endif // ENABLE_TEXTURE_REPORT
}

//----------------------------------------------------------------------------

BOOL dx9_IsTextureFormatOk( DXC_FORMAT CheckFormat, BOOL bRenderTarget ) 
{
	PRESULT pr;

#ifdef ENGINE_TARGET_DX9
	DXC_FORMAT tBBFormat = dxC_GetBackbufferAdapterFormat();

	V_SAVE( pr, dx9_GetD3D()->CheckDeviceFormat( 
		dxC_GetAdapter(),
		D3DDEVTYPE_HAL,
		tBBFormat,
		bRenderTarget ? D3DUSAGE_RENDERTARGET : 0,
		D3DRTYPE_TEXTURE,
		CheckFormat ) );

	ASSERT_RETFALSE( pr != D3DERR_INVALIDCALL );
#elif defined(ENGINE_TARGET_DX10)
	UINT uFlags = (bRenderTarget ? D3D10_FORMAT_SUPPORT_RENDER_TARGET : 0) | D3D10_FORMAT_SUPPORT_TEXTURE2D;
	V_SAVE( pr, dxC_GetDevice()->CheckFormatSupport( CheckFormat, &uFlags) );
	ASSERT_RETFALSE( pr != E_INVALIDARG );
#endif
	return SUCCEEDED( pr );
}

//----------------------------------------------------------------------------

void dx9_ReportSetTexture( int nTextureID )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	if ( ! pTexture )
		return;
	V( dx9_ReportTextureUse( pTexture ) );
}

//----------------------------------------------------------------------------

static void sCleanupTextureReport()
{
#if ENABLE_TEXTURE_REPORT
	if ( sgtTextureUsage.pnIDs )
	{
		FREE( NULL, sgtTextureUsage.pnIDs );
		sgtTextureUsage.pnIDs = NULL;
	}
#endif // ENABLE_TEXTURE_REPORT
}


//-------------------------------------------------------------------------------------------------
// General texture functions
//-------------------------------------------------------------------------------------------------

inline PRESULT sReleaseTextureResource( D3D_TEXTURE * pTexture )
{
	pTexture->pD3DTexture = NULL;
	DX10_BLOCK( pTexture->pD3D10ShaderResourceView = NULL; )
	if ( pTexture->ptSysmemTexture )
	{
		pTexture->ptSysmemTexture->Free();
		FREE( g_ScratchAllocator, pTexture->ptSysmemTexture );
		pTexture->ptSysmemTexture = NULL;
	}
	return S_OK;
}

inline PRESULT sGetTextureInfoFromDesc( D3D_TEXTURE * pTexture, D3DXIMAGE_INFO * pImageInfo )
{
	if ( ! pTexture->pD3DTexture && ! ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture ) )
		return S_FALSE;

	ZeroMemory( pImageInfo, sizeof(D3DXIMAGE_INFO) );

	D3DC_TEXTURE2D_DESC tDesc;
	V_RETURN( dxC_Get2DTextureDesc( pTexture, 0, &tDesc ) );

	pImageInfo->Format			= tDesc.Format;
	pImageInfo->Height			= tDesc.Height;
	pImageInfo->Width			= tDesc.Width;


	if ( pTexture->pD3DTexture )
	{
		DX9_BLOCK
		(
			pImageInfo->MipLevels		= pTexture->pD3DTexture->GetLevelCount();
			pImageInfo->ResourceType	= pTexture->pD3DTexture->GetType();
		)
		DX10_BLOCK
		(
			pImageInfo->ArraySize = tDesc.ArraySize;
			pImageInfo->Depth = 0;
			pImageInfo->MipLevels =  tDesc.MipLevels;
			pImageInfo->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		)
	}
	else
	{
		pImageInfo->MipLevels = pTexture->ptSysmemTexture->nLevels;
		DX9_BLOCK
		(
			pImageInfo->ResourceType = D3DRTYPE_TEXTURE;
		)
		DX10_BLOCK
		(
			pImageInfo->ArraySize = tDesc.ArraySize;
			pImageInfo->Depth = 0;
			pImageInfo->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		)
	}


	return S_OK;
}

//----------------------------------------------------------------------------

inline PRESULT sGetTextureInfoFromFileInMemory( D3D_TEXTURE * pTexture, D3DXIMAGE_INFO * pImageInfo )
{
	if ( pTexture->pbLocalTextureFile )
	{
		PRESULT pr;
		V_SAVE( pr, dxC_GetImageInfoFromFileInMemory( pTexture->pbLocalTextureFile, pTexture->nTextureFileSize, pImageInfo ) );
		TEXTURE_DEFINITION * pDef = NULL;
		if ( FAILED(pr) && pTexture->nDefinitionId != INVALID_ID )
			pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		ASSERTV_RETVAL( SUCCEEDED(pr), pr, "Texture DDS file appears to be corrupt for texture \"%s\"", pDef ? pDef->tHeader.pszName : "unknown" );
	}
	else
	{
		ZeroMemory( pImageInfo, sizeof(D3DXIMAGE_INFO) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_GetTextureFormat( int nSourceFormat )
{
	return dxC_ConvertTextureFormat( nSourceFormat, CONVERT_TO_TEXFMT );
}

int e_GetD3DTextureFormat( int nSourceFormat )
{
	return dxC_ConvertTextureFormat( nSourceFormat, CONVERT_TO_D3D );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_TextureSlotIsUsed( TEXTURE_TYPE eType )
{
	// CML 2007.03.23 - For purposes of filling the pak, don't test against features/caps.
	if ( AppCommonIsFillpakInConvertMode() || AppCommonGetFillPakFile() || AppCommonGetMinPak() )
	{
		return TRUE;
	}

	EFFECT_TARGET eTarget = dxC_GetCurrentMaxEffectTarget();

	switch (eTarget)
	{
	case FXTGT_FIXED_FUNC:
		switch (eType)
		{
		case TEXTURE_DIFFUSE:
		case TEXTURE_LIGHTMAP:
			break;
		default:
			return FALSE;
		}
		break;
	case FXTGT_SM_11:
		switch (eType)
		{
		case TEXTURE_NORMAL:
		case TEXTURE_DIFFUSE2:
		case TEXTURE_SPECULAR:
		//case TEXTURE_ENVMAP:		// CML 2007.10.16 - Spheremaps may now be used on some 1.1 shaders.
		//case TEXTURE_SELFILLUM:
			return FALSE;
		// CHB 2007.03.20 - Light map is used by Mythos on 1.1.
		// CHB 2007.05.07 - Light map is now used by both products.
//		case TEXTURE_LIGHTMAP:
//			return AppIsTugboat();
		}
		break;
	case FXTGT_SM_20_LOW:
	case FXTGT_SM_20_HIGH:
		break;
	case FXTGT_SM_30:
		break;
	case FXTGT_SM_40:
		break;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ModifyAndSaveTextureDefinition( TEXTURE_DEFINITION * pSourceDefinition, TEXTURE_DEFINITION * pDestDefinition, DWORD dwFieldFlags, DWORD dwReloadFlags )
{
	ASSERT_RETINVALIDARG( pSourceDefinition );
	ASSERT_RETINVALIDARG( pDestDefinition );

	if ( dwFieldFlags & TEXTURE_DEF_WIDTH )
		pDestDefinition->nWidth			= pSourceDefinition->nWidth;
	if ( dwFieldFlags & TEXTURE_DEF_HEIGHT )
		pDestDefinition->nHeight		= pSourceDefinition->nHeight;
	if ( dwFieldFlags & TEXTURE_DEF_FORMAT )
		pDestDefinition->nFormat		= pSourceDefinition->nFormat;
	if ( dwFieldFlags & TEXTURE_DEF_MIPLEVELS )
		pDestDefinition->nMipMapLevels	= pSourceDefinition->nMipMapLevels;
	if ( dwFieldFlags & TEXTURE_DEF_MIPUSED )
		pDestDefinition->nMipMapUsed	= pSourceDefinition->nMipMapUsed;
	if ( dwFieldFlags & TEXTURE_DEF_MATERIAL )
		PStrCopy( pDestDefinition->pszMaterialName, pSourceDefinition->pszMaterialName, MAX_XML_STRING_LENGTH );
	if ( dwFieldFlags & TEXTURE_DEF_QUICKCOMPRESS )
	{
		if ( pSourceDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_QUICKCOMPRESS )
			pDestDefinition->dwConvertFlags	|= TEXTURE_CONVERT_FLAG_QUICKCOMPRESS;
		else
			pDestDefinition->dwConvertFlags	&= ~TEXTURE_CONVERT_FLAG_QUICKCOMPRESS;
	}
	if ( dwFieldFlags & TEXTURE_DEF_DXT5NORMALMAPS )
	{
		if ( pSourceDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT5NORMALMAP )
			pDestDefinition->dwConvertFlags	|= TEXTURE_CONVERT_FLAG_DXT5NORMALMAP;
		else
			pDestDefinition->dwConvertFlags	&= ~TEXTURE_CONVERT_FLAG_DXT5NORMALMAP;
	}
	if ( dwFieldFlags & TEXTURE_DEF_MIPFILTER )
		pDestDefinition->nMIPFilter		= pSourceDefinition->nMIPFilter;
	if ( dwFieldFlags & TEXTURE_DEF_BLURFACTOR )
		pDestDefinition->fBlurFactor	= pSourceDefinition->fBlurFactor;
	if ( dwFieldFlags & TEXTURE_DEF_SHARPENFILTER )
		pDestDefinition->nSharpenFilter	= pSourceDefinition->nSharpenFilter;
	if ( dwFieldFlags & TEXTURE_DEF_SHARPENPASSES )
	{
		pDestDefinition->nSharpenPasses	= pSourceDefinition->nSharpenPasses;
		if ( pSourceDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_SHARPENMIP0 )
			pDestDefinition->dwConvertFlags	|= TEXTURE_CONVERT_FLAG_SHARPENMIP0;
		else
			pDestDefinition->dwConvertFlags	&= ~TEXTURE_CONVERT_FLAG_SHARPENMIP0;
	}
	if ( dwFieldFlags & TEXTURE_DEF_DXT1ALPHA )
	{
		if ( pSourceDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT1ALPHA )
			pDestDefinition->dwConvertFlags	|= TEXTURE_CONVERT_FLAG_DXT1ALPHA;
		else
			pDestDefinition->dwConvertFlags	&= ~TEXTURE_CONVERT_FLAG_DXT1ALPHA;
	}

	char szXMLFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension( szXMLFilePath, DEFAULT_FILE_WITH_PATH_SIZE, pDestDefinition->tHeader.pszName, DEFINITION_FILE_EXTENSION );

	BOOL bSave = ( 0 == ( dwReloadFlags & TEXTURE_LOAD_NO_SAVE ) );

	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( ! c_GetToolMode() && ! TEST_MASK( pGlobals->dwFlags, GLOBAL_FLAG_GENERATE_ASSETS_IN_GAME ) )
		bSave = FALSE;

	if ( bSave )
	{
		if ( FileIsReadOnly( szXMLFilePath ) )
		{
			if ( 0 == ( dwReloadFlags & TEXTURE_LOAD_NO_ERROR_ON_SAVE ) )
			{
				bSave = DataFileCheck( szXMLFilePath );
			}
			else
				bSave = FALSE;
		}
	}

	if ( bSave )
	{
		CDefinitionContainer* pDefinitionContainer = CDefinitionContainer::GetContainerByType(DEFINITION_GROUP_TEXTURE);
		ASSERT(pDefinitionContainer);
		pDefinitionContainer->Save(pDestDefinition);
	}

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sModifyAndSaveTextureDefinition( D3DXIMAGE_INFO * pImageInfo, TEXTURE_DEFINITION * pDefinition, DWORD dwFieldFlags = TEXTURE_DEF_ALL, DWORD dwReloadFlags = 0 )
{
	ASSERT_RETINVALIDARG( pImageInfo );
	ASSERT_RETINVALIDARG( pDefinition );

	TEXTURE_DEFINITION tSourceDef;
	ZeroMemory( &tSourceDef, sizeof(tSourceDef) );

	if ( dwFieldFlags & TEXTURE_DEF_WIDTH )
		tSourceDef.nWidth        = pImageInfo->Width;
	if ( dwFieldFlags & TEXTURE_DEF_HEIGHT )
		tSourceDef.nHeight       = pImageInfo->Height;
	if ( dwFieldFlags & TEXTURE_DEF_FORMAT )
		tSourceDef.nFormat       = e_GetTextureFormat( pImageInfo->Format );
	if ( dwFieldFlags & TEXTURE_DEF_MIPLEVELS )
		tSourceDef.nMipMapLevels = pImageInfo->MipLevels;

	V_RETURN( dx9_ModifyAndSaveTextureDefinition( &tSourceDef, pDefinition, dwFieldFlags, dwReloadFlags ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetNumMIPLevels( int nDimension )
// Returns the number of mip levels for a given resolution, including the top level
// Example: 256x256 has 9 mip levels: 256, 128, 64, 32, 16, 8, 4, 2, 1.
{
	int nLevel, nRes;
	for ( nLevel = 0, nRes = 1; nLevel < 100; nLevel++, nRes *= 2 )
		if ( nRes > nDimension )
			break;
	ASSERT_RETZERO( nLevel < 100 ); // serious error
	return nLevel;
}

int e_GetNumMIPs( int nDimension )
// Returns the number of mips for a given resolution, excluding the current level
// Example: A 256x256 has 8 mips: 128, 64, 32, 16, 8, 4, 2, 1.
{
	return e_GetNumMIPLevels( nDimension ) - 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// stubs for nvDXTLib link purposes
//static void WriteDTXnFile( unsigned long count, void * buffer, void * user_data )
//{
//}
//static void ReadDTXnFile( unsigned long count, void * buffer, void * user_data )
//{
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sApplyDefinitionOverride( 
	TEXTURE_DEFINITION & tTarget, 
	const TEXTURE_DEFINITION & tOverride,
	const D3DXIMAGE_INFO & tImageInfo )
{
	if ( tOverride.nWidth )
		tTarget.nWidth = tOverride.nWidth;
	if ( tOverride.nHeight )
		tTarget.nHeight = tOverride.nHeight;
	if ( tOverride.nFormat )
		tTarget.nFormat = tOverride.nFormat;
	tTarget.tBinding = tOverride.tBinding;
	tTarget.nMipMapUsed = tOverride.nMipMapUsed;

	// if the texture is smaller than the resolution/format requested, report an error       
	if ( tTarget.nWidth > (int)tImageInfo.Width ||
		tTarget.nHeight > (int)tImageInfo.Height )
	{
		ASSERTV_RETFAIL(0, "Texture file %s is too small to meet requested resolution; texture is %dx%d, want %dx%d or larger.", tTarget.tHeader.pszName, tImageInfo.Width, tImageInfo.Height, tTarget.nWidth, tTarget.nHeight );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sTextureDefinitionInit(
	TEXTURE_DEFINITION * pDefinition,
	DWORD dwTextureFlags,
	int nDefaultFormat )
{
	if ( dwTextureFlags & TEXTURE_FLAG_FORCEFORMAT )
		pDefinition->nFormat = e_GetTextureFormat( nDefaultFormat );

	if ( pDefinition->nFormat == TEXFMT_UNKNOWN )
	{
		if ( nDefaultFormat == 0 )
			pDefinition->nFormat = TEXFMT_DXT1;
		else
			pDefinition->nFormat = e_GetTextureFormat( nDefaultFormat );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void sUpdateTextureAlphaFlag( const D3DXIMAGE_INFO& tImageInfo, const TEXTURE_DEFINITION* pDefinition, D3D_TEXTURE* pTexture )
{
	if ( dx9_TextureFormatHasAlpha( tImageInfo.Format ) )
	{
		if ( ( pDefinition->nFormat == TEXFMT_DXT1 
			&& !TEST_MASK( pDefinition->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT1ALPHA ) )
			// When previewing normal/specular textures in Hammer, ignore alpha
			|| (   c_GetToolMode() 
			&& pDefinition->nFormat == TEXFMT_DXT5 
			&& TEST_MASK( pDefinition->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT5NORMALMAP )
			&& PStrStrI( pDefinition->tHeader.pszName, "specular" ) ) )
		{
			CLEAR_MASK( pTexture->dwFlags, TEXTURE_FLAG_HAS_ALPHA );
		}
		else
		{
			SET_MASK( pTexture->dwFlags, TEXTURE_FLAG_HAS_ALPHA );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sTextureUpdateFromSource ( 
	D3D_TEXTURE* pTexture, 
	TEXTURE_DEFINITION * pDefinition,
	const char * pszFileNameWithPath, 
	const char * pszD3DTextureFileName,
	DWORD dwTextureFlags, 
	D3DFORMAT nDefaultFormat, 
	DWORD dwReloadFlags,
	TEXTURE_DEFINITION * pDefOverride,
	DWORD dwConvertFlags = 0,
	PAK_ENUM ePakEnum = PAK_DEFAULT )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( pDefinition );
	D3DXIMAGE_INFO tImageInfo;
	D3DXIMAGE_INFO tSourceInfo;
	DWORD		   dwTextureDefUpdate = 0;
	ASSERT_RETINVALIDARG( 0 == ( dwReloadFlags & TEXTURE_LOAD_NO_CONVERT ) );

	V( sTextureDefinitionInit( pDefinition, pTexture->dwFlags | dwTextureFlags, nDefaultFormat ) );

	BOOL bSaveConverted = ( 0 == ( dwReloadFlags & TEXTURE_LOAD_NO_SAVE ) );

	if ( bSaveConverted && FileIsReadOnly( pszD3DTextureFileName ) )
	{
		if ( 0 == ( dwReloadFlags & TEXTURE_LOAD_NO_ERROR_ON_SAVE ) )
		{
			bSaveConverted = DataFileCheck( pszD3DTextureFileName );
		}
		else
			bSaveConverted = FALSE;
	}

	if ( TEST_MASK( dwReloadFlags, TEXTURE_LOAD_NO_ERROR_ON_SAVE ) )
		SET_MASK( dwConvertFlags, TEXTURE_CONVERT_FLAG_NO_ERROR_ON_SAVE );

	char szTimer[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCopy( szTimer, "Updating dds texture for ", DEFAULT_FILE_WITH_PATH_SIZE );
	PStrCat( szTimer, pszFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
	TIMER_START( szTimer );

	// nvDXTLib texture file update
	//sTextureFileUpdate( pTexture, pDefinition, pszFileNameWithPath, pszD3DTextureFileName, dwTextureFlags, bSaveConverted );

	DebugString( OP_MID, STATUS_STRING(Generating dds texture), pszD3DTextureFileName );


	V_RETURN( sLoadSourceTexture( pTexture, pszFileNameWithPath ) );
	if ( ! pTexture->pbLocalTextureFile )
		return S_FALSE;
	V_RETURN( sGetTextureInfoFromFileInMemory( pTexture, &tSourceInfo ) );
	tImageInfo = tSourceInfo;


	// hack to force specular textures to DXT5 instead of 1
	if( AppIsHellgate() && pTexture->nGroup != TEXTURE_GROUP_BACKGROUND )
	{
		char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrGetPath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath );
		if ( ! PStrStrI( szPath, FILE_PATH_BACKGROUND ) )
		{
			PStrRemoveEntirePath( szFile, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath );
			if ( PStrStrI( szFile, "specular" ) )
			{
				if ( dx9_IsEquivalentTextureFormat( tSourceInfo.Format, D3DFMT_A8R8G8B8 ) )
				{
					// if DXT1, force DXT5 and flag to save the def
					if ( pDefinition->nFormat == TEXFMT_DXT1 )
					{
						if ( 0 == ( dwReloadFlags & TEXTURE_LOAD_SOURCE_ONLY ) )
							dwTextureDefUpdate |= TEXTURE_DEF_FORMAT;
						pDefinition->nFormat = TEXFMT_DXT5;
						LogMessage( PERF_LOG, "Forcing DXT1 texture with ARGB8 source to DXT5: %s", pszD3DTextureFileName );
					}
				}
			}
		}
	}

	{
#ifdef DXTLIB
		TIMER_START( "texture convert" )

		// for now, for textures using a presave function use cheap convert
		if ( TRUE && pTexture->pfnPreSave )
		{
			V_RETURN( sConvertTextureCheap( pTexture, pDefinition, pDefOverride, pszFileNameWithPath, dwReloadFlags ) );
		}
		else
		{
#ifdef ENGINE_TARGET_DX9 //In DX10 we can't convert TGAs without NVDXT, so for now we just punt
			if ( TRUE && ! c_GetToolMode() && ! AppCommonGetFillPakFile() 
			  && !TEST_MASK( dwReloadFlags, TEXTURE_CONVERT_FLAG_FORCE_BC5 ) )
			{
				V_RETURN( sConvertTextureCheap( pTexture, pDefinition, pDefOverride, pszFileNameWithPath, dwReloadFlags ) );
			}
			else
#endif
			{
				const char* pszSaveFileName = NULL;
				if ( bSaveConverted )
				{
					pszSaveFileName = pszD3DTextureFileName;
					bSaveConverted = FALSE;
				}

				V_RETURN( sConvertTexture( pTexture, pDefinition, pDefOverride, pszFileNameWithPath, dwReloadFlags, nDefaultFormat, dwConvertFlags, pszSaveFileName ) );
			}	
		}

		unsigned int tMS = (unsigned int)TIMER_END();
		trace( "DXT conversion duration: %ums\n", tMS );
#else
		V_RETURN( sConvertTextureCheap( pTexture, pDefinition, pDefOverride, pszFileNameWithPath, dwReloadFlags ) );
#endif
	}

	if ( dwTextureDefUpdate )
	{
		// if I don't get info here, sometimes it saves uninitialized values
		V_RETURN( sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
		V_RETURN( sModifyAndSaveTextureDefinition( &tImageInfo, pDefinition, dwTextureDefUpdate, dwReloadFlags | TEXTURE_LOAD_NO_SAVE ) );
	}

	if ( bSaveConverted )
	{
		V_RETURN( sSaveTexture( pTexture->pD3DTexture, pszD3DTextureFileName, D3DXIFF_DDS, TRUE ) );
	}

	// compute MIP levels
	tImageInfo.MipLevels = e_GetNumMIPLevels( max( tImageInfo.Width, tImageInfo.Height ) );
	V_RETURN( sModifyAndSaveTextureDefinition( &tImageInfo, pDefinition, TEXTURE_DEF_WIDTH | TEXTURE_DEF_HEIGHT | TEXTURE_DEF_MIPLEVELS, dwReloadFlags | TEXTURE_LOAD_NO_ERROR_ON_SAVE | TEXTURE_LOAD_NO_SAVE ) );

	V_RETURN( sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
	if ( pTexture->nWidthInPixels <= 0 || pTexture->nHeight <= 0 || pTexture->nWidthInBytes <= 0 )
	{
		pTexture->nWidthInPixels = tImageInfo.Width;
		pTexture->nHeight		 = tImageInfo.Height;
		pTexture->nWidthInBytes	 = dxC_GetTexturePitch( tImageInfo.Width, tImageInfo.Format );
	}
	pTexture->nFormat = tImageInfo.Format;

	sUpdateTextureAlphaFlag( tImageInfo, pDefinition, pTexture );

	if ( pTexture->nGroup == TEXTURE_GROUP_UI || pTexture->nGroup == TEXTURE_GROUP_NONE )
		pTexture->dwFlags |= TEXTURE_FLAG_NOLOD;

	// AE: What is the point of all this?
	if ( ( ( ! pDefOverride ) && ( pTexture->dwFlags & TEXTURE_FLAG_FORCEFORMAT ) != 0 && 
		!dx9_IsEquivalentTextureFormat( (DXC_FORMAT)pTexture->nFormat, (DXC_FORMAT)nDefaultFormat ) )  &&
		!( dwReloadFlags & TEXTURE_LOAD_ALREADY_UPDATING ) || /* Prevent cyclical calls to dx9_TextureReload() */ 
		 ( pDefOverride && c_GetToolMode() &&
		   ( ( pDefOverride->nWidth  && ( pTexture->nWidthInPixels != pDefOverride->nWidth  ) ) ||
		     ( pDefOverride->nHeight && ( pTexture->nHeight        != pDefOverride->nHeight ) ) ||
		     ( pDefOverride->nFormat && ( pTexture->nFormat     != e_GetD3DTextureFormat( pDefOverride->nFormat ) ) ) ) ) )
	{
		V_RETURN( dx9_TextureReload (pTexture, pszFileNameWithPath, 
						   dwTextureFlags, 
						   nDefaultFormat, dwReloadFlags | TEXTURE_LOAD_FORCECREATETFILE, pDefOverride, ePakEnum ) );
	}

	ASSERT_RETFAIL( pTexture->tUsage >= 0 );
	ASSERT_RETFAIL( pTexture->nFormat != D3DFMT_UNKNOWN );
	ASSERT_RETFAIL( pTexture->nWidthInPixels > 0 && pTexture->nHeight > 0 && pTexture->nWidthInBytes > 0 );

	pTexture->nD3DTopMIPSize = dx9_GetTextureMIPLevelSizeInBytes( pTexture );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_TexturePreload( D3D_TEXTURE * pTexture )  //KMNV TODO: Anything in dx10?
{
#if defined(ENGINE_TARGET_DX9)
	if ( ! pTexture || ! pTexture->pD3DTexture )
		return S_FALSE;

	if ( dx9_GetPool( pTexture->tUsage ) != D3DPOOL_MANAGED )
		return S_FALSE;
	//pTexture->pD3DTexture->PreLoad();
#endif
	//TEXTURE_DEFINITION * pDef = pTexture->nDefinitionId == INVALID_ID ? NULL : (TEXTURE_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
	//trace( "Texture preload! [%4d] \"%s\"\n", pTexture->nId, pDef ? pDef->tHeader.pszName : "" );

	return S_OK;
}

PRESULT e_TexturePreload( int nTexture )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTexture );
	if ( pTexture )
		return dxC_TexturePreload( pTexture );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MAX_TEXTURE_EXTENSION 5
struct TEXTURE_LOADED_CALLBACKDATA
{
	DWORD dwTextureFlags;
	DWORD dwReloadFlags;
	int nTextureDefId;
	D3DFORMAT nDefaultFormat;
	BOOL bUseOverride;
	TEXTURE_DEFINITION tDefOverride;
	char szExtension[ MAX_TEXTURE_EXTENSION ];
};

PRESULT dx9_TextureFileProcess( 
	D3D_TEXTURE * pTexture,
	TEXTURE_DEFINITION * pDefinition,
	PAK_ENUM ePakEnum /*= PAK_DEFAULT*/ )
{
	char szTimer[ MAX_PATH ];
	PStrPrintf( szTimer, MAX_PATH, "sTextureFileProcess %s", pDefinition->tHeader.pszName );
	TIMER_START( szTimer );

	ASSERT_RETINVALIDARG( pTexture ); 
	ASSERT_RETINVALIDARG( pDefinition ); // it should be loaded by now
	TEXTURE_LOADED_CALLBACKDATA * pCallbackData = pTexture->pLoadedCallbackData;
	pTexture->pLoadedCallbackData = NULL;

	PRESULT pr;
	D3DXIMAGE_INFO tImageInfo;

	V_SAVE_GOTO( pr, cleanup, sTextureDefinitionInit( pDefinition, pTexture->dwFlags | pCallbackData->dwTextureFlags, pCallbackData->nDefaultFormat ) );

	BOOL bReload = FALSE;
	if ( ! pTexture->pbLocalTextureFile )
	{ // file didn't exist. Force it to convert.
		bReload = TRUE;
	} else {
		pDefinition->nFileSize = pTexture->nTextureFileSize;

		V_SAVE_GOTO( pr, cleanup, sGetTextureInfoFromFileInMemory( pTexture, &tImageInfo ) );

		if ( pCallbackData->bUseOverride )
		{
			V_SAVE_GOTO( pr, cleanup, sApplyDefinitionOverride( *pDefinition, pCallbackData->tDefOverride, tImageInfo ) );
			if( pCallbackData->nDefaultFormat != tImageInfo.Format )
			{
				const int cnFormatLen = 32;
				char szFormat[ cnFormatLen ];
				dxC_TextureFormatGetDisplayStr( pCallbackData->nDefaultFormat, szFormat, cnFormatLen );

				ASSERTV( pCallbackData->nDefaultFormat == tImageInfo.Format,
					"Source texture [%s] format does not match the required format [%s], "
					"which will result in a costly conversion at run-time!", 
					pDefinition->tHeader.pszName, szFormat );
			}
		}

		V_SAVE_GOTO( pr, cleanup, sCreateTextureFromMemory( pTexture, 
									pCallbackData->bUseOverride ? &pCallbackData->tDefOverride : NULL, 
									pCallbackData->dwReloadFlags, 
									(D3DFORMAT)pCallbackData->nDefaultFormat ) );
	}

	if ( ( pTexture->dwFlags & TEXTURE_FLAG_FORCEFORMAT ) != 0 && 
		! dx9_IsEquivalentTextureFormat( (DXC_FORMAT)pTexture->nFormat, (DXC_FORMAT)pCallbackData->nDefaultFormat ) )
	{
		bReload = TRUE;
	}

	if ( bReload )
	{ // async loading didn't work.  Just do it the old fashioned way
		char szTimer[ MAX_PATH ];
		PStrPrintf( szTimer, MAX_PATH, "sTextureFileProcess Load from Source %s", pDefinition->tHeader.pszName );
		TIMER_START( szTimer );

		char szSourceFileName[ MAX_PATH ];
		PStrReplaceExtension( szSourceFileName,     MAX_PATH, pDefinition->tHeader.pszName, pCallbackData->szExtension );
		char szD3DTextureFileName[ MAX_PATH ];
		PStrReplaceExtension( szD3DTextureFileName, MAX_PATH, pDefinition->tHeader.pszName, D3D_TEXTURE_FILE_EXTENSION );

		if ( AppCommonIsAnyFillpak() )
		{
			ASSERTV_MSG( "Texture file needs update, but cannot update in fillpak mode:  %s", szSourceFileName );
		}
		else
		{
			V_SAVE_GOTO( pr, cleanup, sTextureUpdateFromSource ( pTexture, pDefinition, 
				szSourceFileName, szD3DTextureFileName, 
				pCallbackData->dwTextureFlags, 
				(D3DFORMAT)pCallbackData->nDefaultFormat,
				pCallbackData->dwReloadFlags | TEXTURE_LOAD_FORCECREATETFILE,
				pCallbackData->bUseOverride ? &pCallbackData->tDefOverride : NULL ) );
		}

		FREE( g_ScratchAllocator, pCallbackData );
		return S_OK;
	} 

	pr = E_FAIL;
	if ( ! e_IsNoRender() )
	{
		ASSERT_GOTO( pTexture->pD3DTexture || ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture ), cleanup );
	}

	tImageInfo.MipLevels = e_GetNumMIPLevels( max( tImageInfo.Width, tImageInfo.Height ) );

	pDefinition->nWidth			= tImageInfo.Width;
	pDefinition->nHeight		= tImageInfo.Height;
	pDefinition->nFormat		= e_GetTextureFormat( tImageInfo.Format );
	pDefinition->nMipMapLevels	= tImageInfo.MipLevels;

	V_SAVE_GOTO( pr, cleanup, sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
	pTexture->nWidthInPixels	= tImageInfo.Width;
	pTexture->nHeight			= tImageInfo.Height;
	pTexture->nWidthInBytes		= dxC_GetTexturePitch( tImageInfo.Width, tImageInfo.Format );
	pTexture->nFormat			= tImageInfo.Format;

	sUpdateTextureAlphaFlag(tImageInfo, pDefinition, pTexture);

	if ( pTexture->nGroup == TEXTURE_GROUP_UI || pTexture->nGroup == TEXTURE_GROUP_NONE )
		pTexture->dwFlags |= TEXTURE_FLAG_NOLOD;

	pr = E_FAIL;
	ASSERT_GOTO( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) || pTexture->tUsage >= 0, cleanup );
	ASSERT_GOTO( pTexture->nFormat != D3DFMT_UNKNOWN, cleanup );
	ASSERT_GOTO( pTexture->nWidthInPixels > 0 && pTexture->nHeight > 0 && pTexture->nWidthInBytes > 0, cleanup );

	pTexture->nD3DTopMIPSize = dx9_GetTextureMIPLevelSizeInBytes( pTexture );

	pr = S_OK;
cleanup:
	FREE( g_ScratchAllocator, pCallbackData );

	unsigned int nMS = (unsigned int)TIMER_END();
	if ( nMS > 20 )
		trace( "TextureFileProcess slow: %5d ms, [%s]\n", nMS, pDefinition->tHeader.pszName );

	//This is where we check for texture arrays and load the rest of the textures in DX10
	if( pDefinition->nArraySize > 1 )
	{
#ifdef ENGINE_TARGET_DX9
		// only do the texture array creation code if in fillPak when in DX9
		if ( e_IsNoRender() )
#endif
		{
			V_RETURN( dxC_CreateTextureArray( pTexture, pDefinition ) );
		}
	}

	if ( SUCCEEDED(pr) )
	{
		V_RETURN( dxC_TexturePreload( pTexture ) );
	}
	
	if( pTexture->pfnPostLoad )
	{
		V_RETURN( pTexture->pfnPostLoad( pTexture->nId ) );
	}

	V( e_ReaperUpdate() );

	return pr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sCubeTextureLoadedCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	TEXTURE_LOADED_CALLBACKDATA * pCallbackData = (TEXTURE_LOADED_CALLBACKDATA *)spec->callbackData;
	void *pData = spec->buffer;

	// nTextureDefID is actually just the cube texture id (cube textures don't have definitions)
	D3D_CUBETEXTURE* pTexture = dx9_CubeTextureGet( pCallbackData->nTextureDefId );
	if ( ! pTexture )
	{
		return S_FALSE;
	}

	if ( !spec->buffer || data.m_IOSize <= 0 )
	{
		if ( !AppCommonGetDirectLoad() )
			pTexture->dwFlags |= _TEXTURE_FLAG_LOAD_FAILED;
		return E_FAIL;
	}

	if ( e_IsNoRender() )
		return S_FALSE;
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	D3DXIMAGE_INFO tImageInfo;

	V_RETURN( dxC_CreateCubeTextureFromFileInMemoryEx(
		spec->buffer,
		data.m_IOSize,
		0, // 0 == D3DX_DEFAULT
		0,
		D3DC_USAGE_CUBETEX_DYNAMIC,
		ENVMAP_LOAD_FORMAT,
		D3DX_FILTER_LINEAR,
		D3DX_FILTER_LINEAR,
		&tImageInfo,
		&pTexture->pD3DTexture ) );

	pTexture->nSize = tImageInfo.Width;
	ASSERT_RETFAIL( pTexture->nSize > 0 );

	ASSERT_RETFAIL(!pTexture->pbLocalTextureFile);
/*
	if (pTexture->pbLocalTextureFile)
	{
		FREE(g_ScratchAllocator, pTexture->pbLocalTextureFile);
	}
*/
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sTextureLoadedCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	TEXTURE_LOADED_CALLBACKDATA * pCallbackData = (TEXTURE_LOADED_CALLBACKDATA *)spec->callbackData;
	void *pData = spec->buffer;

	ASSERT_RETFAIL(pCallbackData);

	D3D_TEXTURE* pTexture = dxC_TextureGet( pCallbackData->nTextureDefId );
	if ( ! pTexture )
	{
		return S_FALSE;
	}

	if (pTexture->pLoadedCallbackData)
	{
		FREE(g_ScratchAllocator, pTexture->pLoadedCallbackData);
		pTexture->pLoadedCallbackData = NULL;
	}
	CLEAR_MASK(spec->flags, PAKFILE_LOAD_FREE_CALLBACKDATA);
	pTexture->pLoadedCallbackData = pCallbackData;

	if (pTexture->pbLocalTextureFile)
	{
		FREE(g_ScratchAllocator, pTexture->pbLocalTextureFile);
		pTexture->pbLocalTextureFile = NULL;
	}

	CLEAR_MASK(spec->flags, PAKFILE_LOAD_FREE_BUFFER);
	pTexture->pbLocalTextureFile = (BYTE *) pData;
	pTexture->nTextureFileSize	 = data.m_IOSize;

	// AE 2007.02.09 - I commented this out because I need it to mark a failed load for LOD wardrobe textures.
	if ( /*!AppCommonGetDirectLoad() && */( !pTexture->pbLocalTextureFile || pTexture->nTextureFileSize <= 0 ) )
	{
		pTexture->dwFlags |= _TEXTURE_FLAG_LOAD_FAILED;
		return E_FAIL;
	}
/*
	// phu - currently only reason this doesn't work is because of texture arrays (raintexture)
	if (AppCommonIsAnyFillpak() && (spec->flags & PAKFILE_LOAD_DONT_LOAD_FROM_PAK))
	{
		return S_FALSE;
	}
*/
	TEXTURE_DEFINITION * pDefinition = (TEXTURE_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_TEXTURE, pCallbackData->nTextureDefId);

#ifndef ON_DEMAND_TEXTURE_CREATION
	if (pDefinition)
	{
		V_RETURN( dx9_TextureFileProcess(pTexture, pDefinition, spec->pakEnum ) );
	}
#endif

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sTextureDefinitionLoadedCallback(
	void * pDef, 
	EVENT_DATA * pEventData)
{
	// we don't need to free pEventData
	PAK_ENUM ePakEnum = (PAK_ENUM)pEventData->m_Data1;
	TEXTURE_DEFINITION * pDefinition = (TEXTURE_DEFINITION *) pDef;
	ASSERT_RETURN( pDefinition );
	D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->tHeader.nId );

	if ( ! pTexture || ! pTexture->pLoadedCallbackData )
		return; // the texture file hasn't loaded yet

#ifndef ON_DEMAND_TEXTURE_CREATION
	V( dx9_TextureFileProcess( pTexture, pDefinition, ePakEnum ) ); 
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// returns: S_OK when texture is created and ready, S_FALSE when it's still loading, and E_FAIL on error
PRESULT dx9_TextureCreateIfReady( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDef )
{
	ASSERT_RETFAIL( pTexture );
	if ( pTexture->pD3DTexture || ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture ) )
		return S_OK;

	// AE 2007.02.09 - I commented this out because I need it to skip texture creation if the load failed for LOD wardrobe textures.
	if(/*!AppCommonGetDirectLoad() &&*/ (!pTexture->pbLocalTextureFile || pTexture->nTextureFileSize <= 0))
	{
		//e_TextureRemove(pCallbackData->nTextureDefId);
		return S_FALSE;
	}

	if ( pDef && pTexture->pLoadedCallbackData)
	{
//#ifdef ON_DEMAND_TEXTURE_CREATION
		// finalize the texture creation
		V_RETURN( dx9_TextureFileProcess( pTexture, pDef ) );
		ASSERTV_RETFAIL( pTexture->pD3DTexture || ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture ), "On-demand texture file process failed! Texture filename:\n\n%s", pDef->tHeader.pszName );
		return S_OK;
//#endif
	}
	return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PAK_ENUM sModifyPakEnumForTexture(
	DWORD dwTextureLoadFlags,
	PAK_ENUM ePakfile,
	int nTextureGroup)
{
	if (TEST_MASK( dwTextureLoadFlags, TEXTURE_LOAD_ADD_TO_HIGH_PAK ))
	{
		// if not putting this in the localized pak file, we are allowed to put it in the
		// high graphics one ... the localized pak will contain both low and high versions of 
		// textures if we have them
		if (ePakfile != PAK_LOCALIZED)
		{
			switch(nTextureGroup)
			{
			case TEXTURE_GROUP_BACKGROUND:
				ePakfile = PAK_GRAPHICS_HIGH_BACKGROUNDS;
				break;
			case TEXTURE_GROUP_WARDROBE:
				ePakfile = PAK_GRAPHICS_HIGH_PLAYERS;
				break;
			default:
				ePakfile = PAK_GRAPHICS_HIGH;
				break;
			}
		}
	}
	return ePakfile;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sFillpakAddOptionalTextureFile( 
	const char * pszOptionalD3DTextureFileName, 
	DWORD dwReloadFlags,
	int nTextureGroup,
	PAK_ENUM ePakfile = PAK_DEFAULT )
{
	// during fillpak, load the "optional" file to get it in the pak
	if ( AppIsHellgate() && AppCommonGetFillPakFile() && pszOptionalD3DTextureFileName )
	{
		DECLARE_LOAD_SPEC( spec, pszOptionalD3DTextureFileName );
		spec.pool = g_ScratchAllocator;
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_DONT_LOAD_FROM_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_WARN_IF_MISSING;
		spec.pakEnum = sModifyPakEnumForTexture( dwReloadFlags, ePakfile, nTextureGroup );		
		PakFileLoadNow(spec);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_TextureReload ( 
	D3D_TEXTURE* pTexture, 
	const char * pszFileNameWithPath, 
	DWORD dwTextureFlags, 
	D3DFORMAT nDefaultFormat, 
	DWORD dwReloadFlags,
	TEXTURE_DEFINITION * pDefOverride,
	PAK_ENUM ePakfile /*= PAK_DEFAULT*/ )
{
	// find our texture
	if (!pTexture)
	{
		int nTextureID;
		V_RETURN( e_TextureNewFromFile( &nTextureID, pszFileNameWithPath, INVALID_ID, INVALID_ID, dwTextureFlags, nDefaultFormat, NULL, NULL, NULL, pDefOverride, dwReloadFlags, 0.0f, 0, ePakfile ) );
		return S_OK;
	}

	V( sReleaseTextureResource( pTexture ) );

	// CHB 2007.05.03 - This could happen if the shader model
	// is altered mid-game.
	if (!e_TextureSlotIsUsed(static_cast<TEXTURE_TYPE>(pTexture->nType)))
	{
		return S_FALSE;
	}

	BOOL bAllowUpdate = AppCommonAllowAssetUpdate();

	// ... unless a flag is set saying otherwise
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( c_GetToolMode() )
	{
		if ( TEST_MASK( pGlobals->dwFlags, GLOBAL_FLAG_DONT_UPDATE_TEXTURES_IN_HAMMER ) 
			&& ! TEST_MASK( dwReloadFlags, TEXTURE_LOAD_FORCECREATETFILE ) )
			bAllowUpdate = FALSE;
	}
	else
	{
		if ( ! TEST_MASK( pGlobals->dwFlags, GLOBAL_FLAG_UPDATE_TEXTURES_IN_GAME ) )
			bAllowUpdate = FALSE;
		if ( AppCommonIsFillpakInConvertMode() )
			bAllowUpdate = TRUE;
	}

	// test for context/resource
	//pTexture->hResource = dxC_ResourceCreate( DXCRTYPE_TEXTURE2D );
	//DXC_RESOURCE_TEXTURE2D * pRes = dxC_ResourceGetTexture2D( pTexture->hResource );
	//ASSERT(pRes);
	//if ( pRes )
	//{
	//	pRes->SetContext( e_ContextGetActive() );
	//}


	TEXTURE_DEFINITION * pDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );

	if ( ! pDefinition )
	{
		EVENT_DATA eventData(ePakfile);
		DefinitionAddProcessCallback( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId, sTextureDefinitionLoadedCallback, &eventData);
	}

	char pszTextureFileName[ 2 ][ DEFAULT_FILE_WITH_PATH_SIZE ] = { "", "" };
	char * pszD3DTextureFileName = pszTextureFileName[ 0 ];
	const char * pszLoadFilename = NULL;
	BOOL bUpdate = FALSE;

	if ( 0 == ( dwReloadFlags & TEXTURE_LOAD_SOURCE_ONLY ) )
	{
		// look for a .dds file that matches this name

		char* pszOptionalD3DTextureFileName = NULL;

		PStrReplaceExtension(pszD3DTextureFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath, D3D_TEXTURE_FILE_EXTENSION);

		BOOL bCreateBC5Texture = FALSE;

		if ( AppIsHellgate() )
		{
#if defined( DX10_BC5_TEXTURES )
			if ( ( pTexture->nType == TEXTURE_NORMAL )
#ifdef ENGINE_TARGET_DX10
			  // AE 2008.01.07: I had to add this, so that distortion particles
			  // would use BC5 textures instead of DXT5 textures. TEXTURE_DIFFUSE 
			  // is passed in as the texture type by c_sParticleSystemLoadTextures() 
			  // instead of TEXTURE_NORMAL.
			  || ( pDefinition && TEST_MASK( pDefinition->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT5NORMALMAP ) )
#endif
			  || ( dwReloadFlags & TEXTURE_LOAD_FORCE_BC5_UPDATE ) )
			{
				pszOptionalD3DTextureFileName = pszTextureFileName[ 1 ];

				char pszBaseName[ DEFAULT_FILE_WITH_PATH_SIZE ];
				PStrRemoveExtension( pszBaseName, DEFAULT_FILE_WITH_PATH_SIZE, 
									 pszD3DTextureFileName );
				PStrPrintf( pszOptionalD3DTextureFileName, DEFAULT_FILE_WITH_PATH_SIZE, 
							BC5_TEXTURE_FILENAME_FORMAT, pszBaseName, D3D_TEXTURE_FILE_EXTENSION );

				if ( bAllowUpdate && FileExists( pszD3DTextureFileName ) 
				  && !FileExists( pszOptionalD3DTextureFileName ) )
				{
					bCreateBC5Texture = TRUE;					
				}

#ifdef ENGINE_TARGET_DX10
				char* pszSwap = pszD3DTextureFileName;
				pszD3DTextureFileName = pszOptionalD3DTextureFileName;
				pszOptionalD3DTextureFileName = pszSwap;
				nDefaultFormat = DXGI_FORMAT_BC5_UNORM;
#endif  // DX10
			}
#endif  // DX10_BC5_TEXTURES
		}

		pszLoadFilename = pszD3DTextureFileName;

		char pszXMLFileName[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrReplaceExtension(pszXMLFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath, DEFINITION_FILE_EXTENSION);

		DebugString( OP_LOW, LOADING_STRING(texture), pszD3DTextureFileName );

		bUpdate = ( dwReloadFlags & TEXTURE_LOAD_FORCECREATETFILE ) && 
				 !( dwReloadFlags & TEXTURE_LOAD_ALREADY_UPDATING );

		if ( bAllowUpdate && !bUpdate && PStrICmp( pszD3DTextureFileName, pszFileNameWithPath ) != 0 &&
			 ( FileNeedsUpdate( pszD3DTextureFileName, pszFileNameWithPath ) ||
		       FileNeedsUpdate( pszD3DTextureFileName, pszXMLFileName, 0, 0, FILE_UPDATE_OPTIONAL_FILE ) ) )
		{
			bUpdate = TRUE;
		} 

		if ( bAllowUpdate && ( bUpdate || bCreateBC5Texture ) )
		{
			// We can't update the texture if we are missing the definition.
			if ( ! pDefinition )
			{
				// Force definition to be loaded
				pDefinition = (TEXTURE_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_TEXTURE,
					DefinitionGetNameById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId ),
					-1, TRUE );
				ASSERT( pDefinition );
			}

			DWORD dwConvertFlags = 0;

			if ( AppIsHellgate() )
			{
#ifdef DX10_BC5_TEXTURES
				if ( pszOptionalD3DTextureFileName 
					 DX10_BLOCK( && bUpdate ) /* Only update DXT5 if necessary */ )
				{
#ifdef ENGINE_TARGET_DX9
					// Create the BC5 texture first, then the DXT5 texture since we will 
					// continue with the DXT5 version.
					dwConvertFlags |= TEXTURE_CONVERT_FLAG_FORCE_BC5;
#endif

					V( sTextureUpdateFromSource ( pTexture, pDefinition, pszFileNameWithPath, pszOptionalD3DTextureFileName, 
						dwTextureFlags, nDefaultFormat, dwReloadFlags | TEXTURE_LOAD_ALREADY_UPDATING, pDefOverride,
						dwConvertFlags, ePakfile ) );

					V( sReleaseTextureResource( pTexture ) );

#ifdef ENGINE_TARGET_DX9
					dwConvertFlags = 0;
					if ( !bUpdate )
						dwReloadFlags |= TEXTURE_LOAD_NO_SAVE;
#else // ENGINE_TARGET_DX10
					// Create the DXT5 texture first, then the BC5 texture since we will
					// continue with the BC5 version.
					dwConvertFlags |= TEXTURE_CONVERT_FLAG_FORCE_BC5;
					dwReloadFlags &= ~TEXTURE_LOAD_NO_SAVE;
#endif
				}
#endif // DX10_BC5_TEXTURES
			}

			{
				TIMER_START3( "%s\n", pszFileNameWithPath );
				V_RETURN( sTextureUpdateFromSource ( pTexture, pDefinition, pszFileNameWithPath, 
					pszD3DTextureFileName, dwTextureFlags, nDefaultFormat, 
					dwReloadFlags | TEXTURE_LOAD_ALREADY_UPDATING, pDefOverride,
					dwConvertFlags, ePakfile ) );
			}

			if ( bCreateBC5Texture && !( dwReloadFlags & TEXTURE_LOAD_FORCE_BC5_UPDATE )  )
			{
				if ( !TEST_MASK( dwReloadFlags, TEXTURE_LOAD_NO_ERROR_ON_SAVE ) )
					ShowDataWarning( "Creating BC5 DDS file because it did not exist! "
									 "Please add this file to Perforce.\n\n%s", 
									 DXC_9_10( pszOptionalD3DTextureFileName, pszD3DTextureFileName ) );
			}

			sFillpakAddOptionalTextureFile( pszOptionalD3DTextureFileName, dwReloadFlags, pTexture->nGroup, ePakfile );

			if ( TEST_MASK( pTexture->dwFlags, TEXTURE_FLAG_SYSMEM ) )
			{
				V( sReleaseTextureResource( pTexture ) );
				bUpdate = FALSE;
				// Fall through to reload the texture.
			}
			else
				return S_OK;
		}
		else
			sFillpakAddOptionalTextureFile( pszOptionalD3DTextureFileName, dwReloadFlags, pTexture->nGroup, ePakfile );
	}
	else
	{
		// load source texture only
		pszLoadFilename = pszFileNameWithPath;
		DebugString( OP_LOW, LOADING_STRING(texture), pszLoadFilename );
	}

	TEXTURE_LOADED_CALLBACKDATA * pCallbackData = (TEXTURE_LOADED_CALLBACKDATA *) MALLOCZ( NULL, sizeof( TEXTURE_LOADED_CALLBACKDATA ) );
	pCallbackData->dwTextureFlags = dwTextureFlags;
	pCallbackData->dwReloadFlags = dwReloadFlags;
	pCallbackData->nTextureDefId = pTexture->nDefinitionId;
	pCallbackData->nDefaultFormat = nDefaultFormat;
	// preserve the source extension in case an update-from-source is required
	const char * pszExtension = PStrGetExtension( pszFileNameWithPath );
	if ( pszExtension )// add one to remove the dot
		StrCpyN( pCallbackData->szExtension, pszExtension + 1, MAX_TEXTURE_EXTENSION );
	else
		pCallbackData->szExtension[ 0 ] = 0;

	if ( pDefOverride )
	{
		pCallbackData->bUseOverride = TRUE;
		memcpy( &pCallbackData->tDefOverride, pDefOverride, sizeof( TEXTURE_DEFINITION ) );
	}

	DECLARE_LOAD_SPEC( spec, pszLoadFilename );
	spec.pool = g_ScratchAllocator;
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;
	spec.priority = pTexture->nPriority;
	spec.fpLoadingThreadCallback = sTextureLoadedCallback;
	spec.callbackData = pCallbackData;
	spec.pakEnum = sModifyPakEnumForTexture( dwReloadFlags, ePakfile, pTexture->nGroup );

	if (AppCommonIsAnyFillpak() && !(dwReloadFlags & TEXTURE_LOAD_FILLPAK_LOAD_FROM_PAK))
	{
		spec.flags |= PAKFILE_LOAD_DONT_LOAD_FROM_PAK;
	}
	
	if ( dwTextureFlags & TEXTURE_FLAG_NOERRORIFMISSING )
		spec.flags |= PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;

	FREE( g_ScratchAllocator, pTexture->pLoadedCallbackData );
	pTexture->pLoadedCallbackData = NULL;

	FREE( g_ScratchAllocator, pTexture->pbLocalTextureFile);
	pTexture->pbLocalTextureFile = NULL;

	// let async complete the texture file loads/frees, which helps keep 
	// the memory footprint low.
	AsyncFileProcessComplete( AppCommonGetMainThreadAsyncId(), TRUE );

	if (bUpdate)
	{
		ASSERT_RETFAIL(!pDefinition);
		pTexture->pLoadedCallbackData = pCallbackData;
	}
	else if ((dwReloadFlags & TEXTURE_LOAD_NO_ASYNC_TEXTURE) || AppCommonIsAnyFillpak())
	{
		PakFileLoadNow(spec);
	} 
	else 
	{
		PakFileLoad(spec);
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void dx9_GetTextureSurfaceDC( int nTextureID, void ** ppDC, int nLevel )
//{
//	ASSERT( ppDC );
//
//	// retrieves the device context for the requested surface
//	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
//	if ( !pTexture )
//	{
//		*ppDC = NULL;
//		return;
//	}
//
//	DX9_BLOCK
//	(
//		SPDIRECT3DSURFACE9 pSurface;
//		ASSERT_RETURN( pTexture->pD3DTexture );
//		HRVERIFY( pTexture->pD3DTexture->GetSurfaceLevel( nLevel, &pSurface ) );
//		ASSERT_RETURN( pSurface );
//		HRVERIFY( pSurface->GetDC( (HDC*)ppDC ) );
//	)
//	DX10_BLOCK
//	(
//		ASSERTX(0, "KMNV TODO: No way to do this in DX10 yet");
//	)
//}
//
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void dx9_ReleaseTextureSurfaceDC( int nTextureID, void * hDC, int nLevel )
//{
//	// retrieves the device context for the requested surface
//	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
//	if ( !pTexture )
//		return;
//
//	DX9_BLOCK
//	(
//		SPDIRECT3DSURFACE9 pSurface;
//		HRVERIFY( pTexture->pD3DTexture->GetSurfaceLevel( nLevel, &pSurface ) );
//		HRVERIFY( pSurface->ReleaseDC( (HDC)hDC ) );
//	)
//	DX10_BLOCK
//	(
//		ASSERTX(0, "KMNV TODO: No way to do this in DX10 yet");
//	)
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_TextureFormatGetDisplayStr( DXC_FORMAT eFormat, char * pszStr, int nBufLen )
{
	switch( eFormat )
	{
DX9_BLOCK
(
	case D3DFMT_R8G8B8               : PStrCopy( pszStr, "D3DFMT_R8G8B8", 			nBufLen ); return;
	case D3DFMT_X1R5G5B5             : PStrCopy( pszStr, "D3DFMT_X1R5G5B5", 		nBufLen ); return;
	case D3DFMT_A4R4G4B4             : PStrCopy( pszStr, "D3DFMT_A4R4G4B4", 		nBufLen ); return;
	case D3DFMT_R3G3B2               : PStrCopy( pszStr, "D3DFMT_R3G3B2", 			nBufLen ); return;
	case D3DFMT_A8R3G3B2             : PStrCopy( pszStr, "D3DFMT_A8R3G3B2", 		nBufLen ); return;
	case D3DFMT_X4R4G4B4             : PStrCopy( pszStr, "D3DFMT_X4R4G4B4", 		nBufLen ); return;
	case D3DFMT_X8B8G8R8             : PStrCopy( pszStr, "D3DFMT_X8B8G8R8", 		nBufLen ); return;
	case D3DFMT_A2R10G10B10          : PStrCopy( pszStr, "D3DFMT_A2R10G10B10", 		nBufLen ); return;
	case D3DFMT_A8P8                 : PStrCopy( pszStr, "D3DFMT_A8P8", 			nBufLen ); return;
	case D3DFMT_P8                   : PStrCopy( pszStr, "D3DFMT_P8", 				nBufLen ); return;
	case D3DFMT_A8L8                 : PStrCopy( pszStr, "D3DFMT_A8L8", 			nBufLen ); return;
	case D3DFMT_A4L4                 : PStrCopy( pszStr, "D3DFMT_A4L4", 			nBufLen ); return;
	case D3DFMT_L6V5U5               : PStrCopy( pszStr, "D3DFMT_L6V5U5", 			nBufLen ); return;
	case D3DFMT_X8L8V8U8             : PStrCopy( pszStr, "D3DFMT_X8L8V8U8", 		nBufLen ); return;
	case D3DFMT_A2W10V10U10          : PStrCopy( pszStr, "D3DFMT_A2W10V10U10", 		nBufLen ); return;
	case D3DFMT_UYVY                 : PStrCopy( pszStr, "D3DFMT_UYVY", 			nBufLen ); return;
	case D3DFMT_YUY2                 : PStrCopy( pszStr, "D3DFMT_YUY2", 			nBufLen ); return;
	case D3DFMT_D32                  : PStrCopy( pszStr, "D3DFMT_D32", 				nBufLen ); return;
	case D3DFMT_D15S1                : PStrCopy( pszStr, "D3DFMT_D15S1", 			nBufLen ); return;
	case D3DFMT_D24X8                : PStrCopy( pszStr, "D3DFMT_D24X8", 			nBufLen ); return;
	case D3DFMT_D24X4S4              : PStrCopy( pszStr, "D3DFMT_D24X4S4", 			nBufLen ); return;
	case D3DFMT_D24FS8               : PStrCopy( pszStr, "D3DFMT_D24FS8", 			nBufLen ); return;
	case D3DFMT_VERTEXDATA           : PStrCopy( pszStr, "D3DFMT_VERTEXDATA", 		nBufLen ); return;
	case D3DFMT_MULTI2_ARGB8         : PStrCopy( pszStr, "D3DFMT_MULTI2_ARGB8", 	nBufLen ); return;
	case D3DFMT_CxV8U8               : PStrCopy( pszStr, "D3DFMT_CxV8U8", 			nBufLen ); return;
)
	case D3DFMT_UNKNOWN              : PStrCopy( pszStr, "D3DFMT_UNKNOWN", 			nBufLen ); return;

	case D3DFMT_A8R8G8B8             : PStrCopy( pszStr, "D3DFMT_A8R8G8B8", 		nBufLen ); return;
	case D3DFMT_X8R8G8B8             : PStrCopy( pszStr, "D3DFMT_X8R8G8B8", 		nBufLen ); return;
	case D3DFMT_R5G6B5               : PStrCopy( pszStr, "D3DFMT_R5G6B5", 			nBufLen ); return;
	
	case D3DFMT_A1R5G5B5             : PStrCopy( pszStr, "D3DFMT_A1R5G5B5", 		nBufLen ); return;


	case D3DFMT_A8                   : PStrCopy( pszStr, "D3DFMT_A8", 				nBufLen ); return;


	case D3DFMT_A2B10G10R10          : PStrCopy( pszStr, "D3DFMT_A2B10G10R10", 		nBufLen ); return;
	DX9_BLOCK( case D3DFMT_A8B8G8R8  : PStrCopy( pszStr, "D3DFMT_A8B8G8R8", 		nBufLen ); return; ) //DX10 makes no distinction

	case D3DFMT_G16R16               : PStrCopy( pszStr, "D3DFMT_G16R16", 			nBufLen ); return;

	case D3DFMT_A16B16G16R16         : PStrCopy( pszStr, "D3DFMT_A16B16G16R16", 	nBufLen ); return;




	case D3DFMT_L8                   : PStrCopy( pszStr, "D3DFMT_L8", 				nBufLen ); return;



	case D3DFMT_V8U8                 : PStrCopy( pszStr, "D3DFMT_V8U8", 			nBufLen ); return;


	case D3DFMT_Q8W8V8U8             : PStrCopy( pszStr, "D3DFMT_Q8W8V8U8", 		nBufLen ); return;
	case D3DFMT_V16U16               : PStrCopy( pszStr, "D3DFMT_V16U16", 			nBufLen ); return;



	case D3DFMT_R8G8_B8G8            : PStrCopy( pszStr, "D3DFMT_R8G8_B8G8", 		nBufLen ); return;

	case D3DFMT_G8R8_G8B8            : PStrCopy( pszStr, "D3DFMT_G8R8_G8B8", 		nBufLen ); return;
	case D3DFMT_DXT1                 : PStrCopy( pszStr, "D3DFMT_DXT1", 			nBufLen ); return;
	DX9_BLOCK( case D3DFMT_DXT2                 : PStrCopy( pszStr, "D3DFMT_DXT2", 			nBufLen ); return; )
	case D3DFMT_DXT3                 : PStrCopy( pszStr, "D3DFMT_DXT3", 			nBufLen ); return;
	DX9_BLOCK( case D3DFMT_DXT4                 : PStrCopy( pszStr, "D3DFMT_DXT4", 			nBufLen ); return; )
	case D3DFMT_DXT5                 : PStrCopy( pszStr, "D3DFMT_DXT5", 			nBufLen ); return;
	DX10_BLOCK( case DXGI_FORMAT_BC5_UNORM : PStrCopy( pszStr, "DXGI_FORMAT_BC5_UNORM", nBufLen ); return;)

	case D3DFMT_D24S8                : PStrCopy( pszStr, "D3DFMT_D24S8", 			nBufLen ); return;

#ifdef ENGINE_TARGET_DX9
	case D3DFMT_D16_LOCKABLE         : PStrCopy( pszStr, "D3DFMT_D16_LOCKABLE", 	nBufLen ); return; //DX10 no distinction for lockable
	case D3DFMT_D32F_LOCKABLE        : PStrCopy( pszStr, "D3DFMT_D32F_LOCKABLE", 	nBufLen ); return;
#endif

	case D3DFMT_D16                  : PStrCopy( pszStr, "D3DFMT_D16", 				nBufLen ); return;

	case D3DFMT_L16                  : PStrCopy( pszStr, "D3DFMT_L16", 				nBufLen ); return;


	case D3DFMT_INDEX16              : PStrCopy( pszStr, "D3DFMT_INDEX16", 			nBufLen ); return;
	case D3DFMT_INDEX32              : PStrCopy( pszStr, "D3DFMT_INDEX32", 			nBufLen ); return;

	case D3DFMT_Q16W16V16U16         : PStrCopy( pszStr, "D3DFMT_Q16W16V16U16", 	nBufLen ); return;



	// Floating point surface formats

	// s10e5 formats (16-bits per channel)
	case D3DFMT_R16F                 : PStrCopy( pszStr, "D3DFMT_R16F", 			nBufLen ); return;
	case D3DFMT_G16R16F              : PStrCopy( pszStr, "D3DFMT_G16R16F", 			nBufLen ); return;
	case D3DFMT_A16B16G16R16F        : PStrCopy( pszStr, "D3DFMT_A16B16G16R16F", 	nBufLen ); return;

	// IEEE s23e8 formats (32-bits per channel)
	DX10_BLOCK( case DXGI_FORMAT_R32_FLOAT: )
	case D3DFMT_R32F                 : PStrCopy( pszStr, "D3DFMT_R32F", 			nBufLen ); return;
	case D3DFMT_G32R32F              : PStrCopy( pszStr, "D3DFMT_G32R32F", 			nBufLen ); return;
	case D3DFMT_A32B32G32R32F        : PStrCopy( pszStr, "D3DFMT_A32B32G32R32F", 	nBufLen ); return;

	// Special formats
	case MAKEFOURCC('N','U','L','L') : PStrCopy( pszStr, "D3DFMT_NULL", 			nBufLen ); return;
	}
	ASSERTX( 0, "Error, unsupported texture format specified!" );
	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int dx9_GetTextureFormatBitDepth( DXC_FORMAT eFormat )
{
	switch( eFormat )
	{
	// render targets:
#ifdef ENGINE_TARGET_DX9
	case D3DFMT_X8B8G8R8	  : return 32;
	case D3DFMT_X1R5G5B5	  : return 16;
	case D3DFMT_D24X8		  : return 32;
#endif

	case D3DFMT_A8R8G8B8	  : return 32;
	DX9_BLOCK( case D3DFMT_A8B8G8R8	  : return 32; ) //DX10 Makes no distinction
	
	case D3DFMT_X8R8G8B8	  : return 32;
	case D3DFMT_R5G6B5		  : return 16;
	
	case D3DFMT_R16F		  : return 16;
	case D3DFMT_G16R16F		  : return 32;
	case D3DFMT_A16B16G16R16F : return 64;
	DX10_BLOCK( case DXGI_FORMAT_R32_FLOAT: )
	case D3DFMT_R32F		  : return 32;
	case D3DFMT_G32R32F		  :	return 64;
	case D3DFMT_A32B32G32R32F : return 128;
	
	case D3DFMT_D24S8		  : return 32;
	case D3DFMT_D16			  : return 16;
	// non-render targets:
#ifdef ENGINE_TARGET_DX9
	case D3DFMT_R8G8B8		  : return 24;
	case D3DFMT_A4R4G4B4	  : return 16;
	case D3DFMT_A16B16G16R16  : return 64;
#endif
	case D3DFMT_A1R5G5B5	  : return 16;
	case D3DFMT_A8			  : return 8;
	case D3DFMT_L8			  : return 8;
	case D3DFMT_DXT1		  : return 4;
	DX9_BLOCK( case D3DFMT_DXT2		  : )
	case D3DFMT_DXT3		  :
	DX9_BLOCK( case D3DFMT_DXT4		  : )
	DX10_BLOCK( case DXGI_FORMAT_BC5_UNORM: )
	case D3DFMT_DXT5		  : return 8;
	case MAKEFOURCC('N','U','L','L'): return 0;
	}
	ASSERT( 0 && "Error, unsupported texture format specified!" );
	return 0;
}

BOOL dx9_TextureFormatHasAlpha( DXC_FORMAT eFormat )
{
	switch( eFormat )
	{
DX9_BLOCK
(
	case D3DFMT_R8G8B8               : return FALSE;
	case D3DFMT_X1R5G5B5             : return FALSE;
	case D3DFMT_A4R4G4B4             : return TRUE;
	case D3DFMT_R3G3B2               : return FALSE;
	case D3DFMT_A8R3G3B2             : return TRUE;
	case D3DFMT_X4R4G4B4             : return FALSE;
	case D3DFMT_X8B8G8R8             : return FALSE;
	case D3DFMT_A2R10G10B10          : return TRUE;
	case D3DFMT_A8P8                 : return TRUE;
	case D3DFMT_P8                   : return FALSE;
	case D3DFMT_A8L8                 : return TRUE;
	case D3DFMT_A4L4                 : return TRUE;
	case D3DFMT_L6V5U5               : return FALSE;
	case D3DFMT_X8L8V8U8             : return FALSE;
	case D3DFMT_A2W10V10U10          : return TRUE;
	case D3DFMT_UYVY                 : return FALSE;
	case D3DFMT_YUY2                 : return FALSE;
	case D3DFMT_D32                  : return FALSE;
	case D3DFMT_D15S1                : return FALSE;
	case D3DFMT_D24X8                : return FALSE;
	case D3DFMT_D24X4S4              : return FALSE;
	case D3DFMT_D24FS8               : return FALSE;
	case D3DFMT_VERTEXDATA           : return FALSE;
	case D3DFMT_MULTI2_ARGB8         : return TRUE;
	case D3DFMT_CxV8U8               : return FALSE;
)
	case D3DFMT_UNKNOWN              : return FALSE;
	case D3DFMT_A8R8G8B8             : return TRUE;
	case D3DFMT_X8R8G8B8             : return FALSE;
	case D3DFMT_R5G6B5               : return FALSE;
	case D3DFMT_A1R5G5B5             : return TRUE;
	case D3DFMT_A8                   : return TRUE;
	case D3DFMT_A2B10G10R10          : return TRUE;
	DX9_BLOCK( case D3DFMT_A8B8G8R8             : return TRUE; ) //DX10 makes no distinction
	case D3DFMT_G16R16               : return FALSE;
	case D3DFMT_A16B16G16R16         : return TRUE;
	case D3DFMT_L8                   : return FALSE;
	case D3DFMT_V8U8                 : return FALSE;
	case D3DFMT_Q8W8V8U8             : return FALSE;
	case D3DFMT_V16U16               : return FALSE;
	case D3DFMT_R8G8_B8G8            : return FALSE;
	case D3DFMT_G8R8_G8B8            : return FALSE;
	case D3DFMT_DXT1                 : return TRUE;
	DX9_BLOCK( case D3DFMT_DXT2                 : return TRUE; )
	case D3DFMT_DXT3                 : return TRUE;
	DX9_BLOCK( case D3DFMT_DXT4                 : return TRUE; )
	case D3DFMT_DXT5                 : return TRUE;

	case D3DFMT_D24S8                : return FALSE;
#ifdef ENGINE_TARGET_DX9
	case D3DFMT_D16_LOCKABLE		 : return FALSE;
	case D3DFMT_D32F_LOCKABLE        : return FALSE;
#endif //DX10 no distinction for lockable
	case D3DFMT_D16                  : return FALSE;
	case D3DFMT_L16                  : return FALSE;
	case D3DFMT_INDEX16              : return FALSE;
	case D3DFMT_INDEX32              : return FALSE;
	case D3DFMT_Q16W16V16U16         : return FALSE;

	// Floating point surface formats

	// s10e5 formats (16-bits per channel)
	case D3DFMT_R16F                 : return FALSE;
	case D3DFMT_G16R16F              : return FALSE;
	case D3DFMT_A16B16G16R16F        : return TRUE;

	// IEEE s23e8 formats (32-bits per 
	case D3DFMT_R32F                 : return FALSE;
	case D3DFMT_G32R32F              : return FALSE;
	case D3DFMT_A32B32G32R32F        : return TRUE;

	//Compressed normal maps
	DX10_BLOCK( case DXGI_FORMAT_BC5_UNORM : return TRUE;)
	}

	ASSERT( 0 && "Error, unsupported texture format specified!" );
	return 0;
}

BOOL dx9_IsCompressedTextureFormat( DXC_FORMAT eFormat )
{
	switch( eFormat )
	{
	case D3DFMT_DXT1:
	DX9_BLOCK( case D3DFMT_DXT2: )
	case D3DFMT_DXT3:
	DX9_BLOCK( case D3DFMT_DXT4: )
	case D3DFMT_DXT5:
	DX10_BLOCK( case DXGI_FORMAT_BC5_UNORM: )
		return TRUE;
	default:
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL dx9_IsEquivalentTextureFormat( DXC_FORMAT eFormat1, DXC_FORMAT eFormat2 )
{
	if ( eFormat1 == eFormat2 )
		return TRUE;
	if ( eFormat1 == D3DFMT_A8R8G8B8 && eFormat2 == D3DFMT_X8R8G8B8 )
		return TRUE;
	if ( eFormat1 == D3DFMT_X8R8G8B8 && eFormat2 == D3DFMT_A8R8G8B8 )
		return TRUE;
	DX9_BLOCK //D3DFMT_X1R5G5B5  not supported by DX10
	(
		if ( eFormat1 == D3DFMT_A1R5G5B5 && eFormat2 == D3DFMT_X1R5G5B5 )
			return TRUE;
		if ( eFormat1 == D3DFMT_X1R5G5B5 && eFormat2 == D3DFMT_A1R5G5B5 )
			return TRUE;
	)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// returns size in bytes of the texture if setLOD to nLOD (or leave as -1 for current)
int dx9_GetTextureLODSizeInBytes( const D3D_TEXTURE * pTexture, int nLOD )
{
	ASSERT_RETZERO( pTexture );
	ASSERT_RETZERO( pTexture->nHeight && pTexture->nWidthInPixels );

	int nMaxLOD = dxC_GetNumMipLevels( pTexture );//->GetLevelCount();
	ASSERT_RETZERO( nMaxLOD > 0 );
	nMaxLOD -= 1;

	// for non pow2 textures, usually UI, use the top miplevel
	if ( ! ( ( pTexture->nHeight        & ( pTexture->nHeight        - 1 ) ) == 0 ) || 
		 ! ( ( pTexture->nWidthInPixels & ( pTexture->nWidthInPixels - 1 ) ) == 0 ) )
		nLOD = nMaxLOD;

	if ( nLOD == -1 )
		nLOD = 0;
	ASSERT_RETZERO( nLOD >= 0 );
	ASSERT_RETZERO( nLOD <= nMaxLOD );

	int nHeight   = pTexture->nHeight		 >> nLOD;
	int nWidth    = pTexture->nWidthInPixels >> nLOD;
	nHeight       = max( 1, nHeight );
	nWidth        = max( 1, nWidth );
	return dxC_GetTextureLevelSize( nWidth, nHeight, (DXC_FORMAT)pTexture->nFormat );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int dx9_GetTextureMIPLevelSizeInBytes( const D3D_TEXTURE * pTexture, int nMIPLevel /*= 0*/ )
{
	if ( e_IsNoRender() )
		return 0;

	ASSERT_RETZERO( pTexture );
	//ASSERTX_RETZERO( nMIPLevel == 0, "Not yet supporting getting size of MIP other than 0!" );

	if ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture )
	{
		return (int)pTexture->ptSysmemTexture->GetLevelSize( nMIPLevel );
	}

	if ( dxC_IsPixomaticActive() )
		return 0;

	D3DC_TEXTURE2D_DESC tDesc;
	V_DO_FAILED( dxC_Get2DTextureDesc( pTexture, (UINT)nMIPLevel, &tDesc ) )
	{
		ASSERTX_RETZERO( 0, "Failed to get texture desc for MIP level for size calculation!" );
	}

	int nFmtBits = dx9_GetTextureFormatBitDepth( (DXC_FORMAT)tDesc.Format );
	return tDesc.Height * tDesc.Width * nFmtBits / 8;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int dx9_GetCubeTextureMIPLevelSizeInBytes( const D3D_CUBETEXTURE * pCubeTexture, int nMIPLevel /*= 0*/ )
{
	ASSERT_RETZERO( pCubeTexture );
	ASSERTX_RETZERO( nMIPLevel == 0, "Not yet supporting getting size of MIP other than 0!" );

	return dxC_GetTextureLevelSize( pCubeTexture->nSize, pCubeTexture->nSize, DXC_FORMAT(pCubeTexture->nFormat ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int dxC_GetTexturePitch( int nWidth, DXC_FORMAT eFormat )
{
	if ( dx9_IsCompressedTextureFormat( eFormat ) )
	{
		return MAX(nWidth / 4, 1) * dx9_GetDXTBlockSizeInBytes( eFormat );
	} else
	{
		return nWidth * dx9_GetTextureFormatBitDepth( eFormat ) / 8;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_TextureDefinitionUpdateFromTexture ( const char * pszTextureFilename, TEXTURE_DEFINITION * pTextureDef )
{
	char * pszFile;
	char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension( szFile, DEFAULT_FILE_WITH_PATH_SIZE, pszTextureFilename, D3D_TEXTURE_FILE_EXTENSION );

	TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
	FileGetFullFileName(szFullname, szFile, DEFAULT_FILE_WITH_PATH_SIZE);

	if ( ! FileExists( szFullname ) )
	{
		FileGetFullFileName(szFullname, pszTextureFilename, DEFAULT_FILE_WITH_PATH_SIZE);
		BOOL bExists = FileExists( szFullname );
		ASSERT_RETFAIL( bExists );
	}
	pszFile = szFullname;

	D3DXIMAGE_INFO tImageInfo;
	V_RETURN( dxC_GetImageInfoFromFile( pszFile, &tImageInfo ) );

	//	if ( pTextureDef->nFormat == D3DFMT_UNKNOWN )
	//		pTextureDef->nFormat = tImageInfo.Format;
	pTextureDef->nHeight = tImageInfo.Height;
	pTextureDef->nWidth = tImageInfo.Width;
	// why?
	//if ( pTextureDef->nFormat == D3DFMT_DXT1 ||
	//	pTextureDef->nFormat  == D3DFMT_DXT2 ||
	//	pTextureDef->nFormat  == D3DFMT_DXT3 ||
	//	pTextureDef->nFormat  == D3DFMT_DXT4 ||
	//	pTextureDef->nFormat  == D3DFMT_DXT5 )
	//{
	//	pTextureDef->nMipMapUsed = tImageInfo.MipLevels;
	//}
	//pTextureDef->nMipMapLevels = e_GetNumMIPLevels( max( tImageInfo.Width, tImageInfo.Height ) );
	pTextureDef->nMipMapLevels = tImageInfo.MipLevels;  //Trust definition

	return S_OK;
}




//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sAddStringToReport( char *& pBuf, int & nBufAlloc, int & nBufUsed, char * pszFmt, ... )
{
	va_list args;
	va_start(args, pszFmt);

	const int cnMaxLineLen = 256;
	char szLine[ cnMaxLineLen ];
	PStrVprintf( szLine, cnMaxLineLen, pszFmt, args );

	int nLen = PStrLen( szLine );
	if ( nBufUsed + nLen >= nBufAlloc )
	{
		// double size on each realloc
		nBufAlloc *= 2;
		pBuf = (char*) REALLOC( NULL, pBuf, nBufAlloc );
		ASSERT_RETURN( pBuf );
	}
	PStrCat( pBuf, szLine, nBufAlloc );
	nBufUsed += nLen;
}

inline void sAddTableLineToReport( char *& pBuf, int & nBufAlloc, int & nBufUsed, const char * pszRowHdr, unsigned int * pnColumns, unsigned int nFinal )
{
	sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "%10s %9d %9d %9d %9d %9d %9d %9d %9d %9d\r\n",
		pszRowHdr,
		pnColumns[ 0 ],
		pnColumns[ 1 ],
		pnColumns[ 2 ],
		pnColumns[ 3 ],
		pnColumns[ 4 ],
		pnColumns[ 5 ],
		pnColumns[ 6 ],
		pnColumns[ 7 ],
		nFinal );
}

#define ADDLINE sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "------------------------------------------------------------------------------------------\r\n" )

PRESULT e_DumpTextureReport()
{
#if ENABLE_TEXTURE_REPORT

	char szFilename[ MAX_PATH ];
	PStrPrintf( szFilename, MAX_PATH, "%s%s", FILE_PATH_DATA, "texture_report.txt" );

	// collect total used load data here  -- safest
	sResetTextureReportStats( sgtTextureReportUse );
	qsort( sgtTextureUsage.pnIDs, sgtTextureUsage.nUsed, sizeof(int), sTextureIDSortCompare );
	int nLastID = INVALID_ID;
	for ( int i = 0; i < sgtTextureUsage.nUsed; i++ )
	{
		ASSERT( sgtTextureUsage.pnIDs[ i ] != INVALID_ID );
		if ( sgtTextureUsage.pnIDs[ i ] == nLastID )
			continue;
		D3D_TEXTURE * pTex = dxC_TextureGet( sgtTextureUsage.pnIDs[ i ] );
		sAddToTextureReport( sgtTextureReportUse, pTex );
		nLastID = sgtTextureUsage.pnIDs[ i ];
	}

	// collect total texture load data here  -- safest
	sResetTextureReportStats( sgtTextureReportTotal );

	for (D3D_TEXTURE* pTex = HashGetFirst(gtTextures); pTex; pTex = HashGetNext(gtTextures, pTex))
	{
		sReportTextureTotal( pTex );
	}


	int nBufAlloc = 1024;
	char * pBuf   = (char*) MALLOC( NULL, nBufAlloc );
	ASSERT( pBuf );
	pBuf[ 0 ]     = NULL;
	int nBufUsed  = 1;	// for the '\0'

	ADDLINE;
	sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "  PRIME Texture Usage Report\r\n" );
	ADDLINE;
	sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "\r\n\r\n\r\n\r\n" );


	TextureReport * pReportCurrent;
	unsigned int  * pData;
	unsigned int  * pAvgByCount = NULL;
	BOOL			bAvgData;
	int nPitch = NUM_TEXTURE_REPORT_TYPES;

	// run once for each report and type (count or size)
	for ( int r = 0; r < 4; r++ )
	{
		pAvgByCount = FALSE;
		bAvgData = FALSE;
		switch ( r )
		{
		case 0:
			pReportCurrent = &sgtTextureReportUse;
			pData          = (unsigned int *)pReportCurrent->nCounts;
			ADDLINE;
			sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "  Textures used this frame: (count) \r\n" );
			ADDLINE;
			break;
		case 1:
			pReportCurrent = &sgtTextureReportUse;
			pData          = (unsigned int *)pReportCurrent->nSize;
			ADDLINE;
			sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "  Textures used this frame: (full-size in memory) \r\n" );
			ADDLINE;
			break;
		case 2:
			pReportCurrent = &sgtTextureReportTotal;
			pData          = (unsigned int *)pReportCurrent->nCounts;
			ADDLINE;
			sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "  Total textures allocated: (count) \r\n" );
			ADDLINE;
			break;
		case 3:
			pReportCurrent = &sgtTextureReportTotal;
			pData          = (unsigned int *)pReportCurrent->nSize;
			ADDLINE;
			sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "  Total textures allocated: (full-size in memory) \r\n" );
			ADDLINE;
			break;
		}

		sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "%10s %9s %9s %9s %9s %9s %9s %9s %9s %9s\r\n",
			"Group",
			gszTextureTypes[ 0 ],
			gszTextureTypes[ 1 ],
			gszTextureTypes[ 2 ],
			gszTextureTypes[ 3 ],
			gszTextureTypes[ 4 ],
			gszTextureTypes[ 5 ],
			gszTextureTypes[ 6 ],
			gszTextureTypes[ 7 ],
			"Total" );
		ADDLINE;

		unsigned int pnTotals[ NUM_TEXTURE_REPORT_TYPES ];
		ZeroMemory( pnTotals, sizeof(unsigned int) * NUM_TEXTURE_REPORT_TYPES );
		unsigned int pnTotalAvgs[ NUM_TEXTURE_REPORT_TYPES ];
		ZeroMemory( pnTotalAvgs, sizeof(unsigned int) * NUM_TEXTURE_REPORT_TYPES );
		unsigned int nTotalTotal = 0;
		unsigned int nTotalAvgTotal = 0;

		for ( int g = 0; g < NUM_TEXTURE_REPORT_GROUPS; g++ )
		{
			unsigned int pnColumns[ NUM_TEXTURE_REPORT_TYPES ];
			unsigned int pnAvgColumns[ NUM_TEXTURE_REPORT_TYPES ];
			unsigned int nRowTotal = 0;
			unsigned int nRowAvgTotal = 0;
			for ( int t = 0; t < NUM_TEXTURE_REPORT_TYPES; t++ )
			{
				if ( bAvgData && pAvgByCount )
				{
					int nCount		  =  pAvgByCount[ g * nPitch + t ];
					pnColumns[ t ]    =  int( (float)pData[ g * nPitch + t ] / (float)nCount + 0.5f );
					pnAvgColumns[ t ] =  pData[ g * nPitch + t ];
					nRowAvgTotal	  += nCount;
					nRowTotal	      += pnAvgColumns[ t ];
					pnTotals[ t ]     += pnAvgColumns[ t ];
					pnTotalAvgs[ t ]  += nCount;
				}
				else
				{
					pnColumns[ t ] =  pData[ g * nPitch + t ];
					nRowTotal	   += pnColumns[ t ];
					pnTotals[ t ]  += pnColumns[ t ];
				}
			}
			nTotalTotal += nRowTotal;
			if ( bAvgData )
			{
				nRowTotal      =  int( (float)nRowTotal / (float)nRowAvgTotal + 0.5f );
				nTotalAvgTotal += nRowAvgTotal;
			}
			sAddTableLineToReport( pBuf, nBufAlloc, nBufUsed, gszTextureGroups[ g ], pnColumns, nRowTotal );
		}
		if ( bAvgData )
		{
			nTotalTotal = int( (float)nTotalTotal / (float)nTotalAvgTotal + 0.5f );
			for ( int t = 0; t < NUM_TEXTURE_REPORT_TYPES; t++ )
				pnTotals[ t ] = int( (float)pnTotals[ t ] / (float)pnTotalAvgs[ t ] * 0.5f );
		}
		sAddTableLineToReport( pBuf, nBufAlloc, nBufUsed, "Totals:", pnTotals, nTotalTotal );
		sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "\r\n" );
	}

	pBuf[ nBufUsed-1 ] = NULL;
	FileSave( szFilename, pBuf, nBufUsed );
	FREE( NULL, pBuf );

#endif // ENABLE_TEXTURE_REPORT

	return S_OK;
}
#undef ADDLINE

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_GetTextureChannelFromType( int nModelID, TEXTURE_TYPE eType )
{
	if ( e_ModelGetFlagbit( nModelID, MODEL_FLAGBIT_ANIMATED ) )
	{
		// MODEL_FLAG_ANIMATED... assume anim vertex
		switch ( eType )
		{
		case TEXTURE_SELFILLUM:
		case TEXTURE_DIFFUSE:
		case TEXTURE_NORMAL:
		case TEXTURE_SPECULAR:
			return TEXCOORDS_CHANNEL_DIFFUSE;
		case TEXTURE_DIFFUSE2:
			return TEXCOORDS_CHANNEL_DIFFUSE2;
		}		
	}
	else
	{
		// no MODEL_FLAG_ANIMATED... assume rigid vertex
		switch ( eType )
		{
		case TEXTURE_SELFILLUM:
			return TEXCOORDS_CHANNEL_LIGHTMAP;
		case TEXTURE_DIFFUSE:
		case TEXTURE_NORMAL:
		case TEXTURE_SPECULAR:
			return TEXCOORDS_CHANNEL_DIFFUSE;
		case TEXTURE_DIFFUSE2:
			return TEXCOORDS_CHANNEL_DIFFUSE2;
		}
	}

	return -1;
}








//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

struct TEXCONVERT_USER_DATA
{
	void*			 pTextureData;
	HANDLE			 pFileData;
};

NV_ERROR_CODE dx9_LoadAllMipSurfaces( const void* pData, size_t count, const MIPMapData* pMipMapData, void* pUserData)
// callback to save all the MIP levels of a texture being compressed/mipped into a D3D texture
{
	ASSERT( pUserData );

	TEXCONVERT_USER_DATA* pTexUserData = (TEXCONVERT_USER_DATA*)pUserData;

	if ( pTexUserData->pFileData != INVALID_FILE )
	{
		FileWrite( pTexUserData->pFileData, pData, (DWORD)count );
	}

	if ( !pMipMapData )
	{
		if ( count == 4 || count == 124 )
			// DDS header being written. Skip this callback.
			return NV_OK;
		else
			// Expecting to receive MipMap data, but didn't.
			ASSERT( pMipMapData );
	}
	SPD3DCTEXTURE2D pNvDXTTexture = *(SPD3DCTEXTURE2D*)&pTexUserData->pTextureData;

	D3DLOCKED_RECT lr;
	dxC_MapTexture( pNvDXTTexture, (UINT)pMipMapData->mipLevel, &lr );
	memcpy( dxC_pMappedRectData( lr ), pData, count );
	dxC_UnmapTexture( pNvDXTTexture, (UINT)pMipMapData->mipLevel );

	return NV_OK;
}

// texture utility functions
static PRESULT sTextureReportError( PRESULT prErrorCode, const char * pszFileNameWithPath, DWORD dwReloadFlags )
{
	BOOL bDisplay = TRUE;

	if ( dwReloadFlags & TEXTURE_LOAD_NO_ERROR_ON_SAVE )
	{
		bDisplay = FALSE;
	}

	const int cnErrStrLen = 512;
	char szError[ cnErrStrLen ];

	ASSERT_RETFAIL( pszFileNameWithPath );
	switch ( prErrorCode )
	{
	case NV_DEPTH_IS_NOT_3_OR_4:
		PStrPrintf( szError, cnErrStrLen, "Texture file %s must be 24 or 32 bit!\n\nWill attempt conversion to valid texture...", pszFileNameWithPath );
		break;
	case NV_IMAGE_NOT_POWER_2:
		PStrPrintf( szError, cnErrStrLen, "Texture file %s resolution must be power of two!\n\nWill attempt conversion to valid texture...", pszFileNameWithPath );
		break;
	default:
#ifdef DXTLIB
		PStrPrintf( szError, cnErrStrLen, "Texture file %s error: %s", 
			pszFileNameWithPath, getErrorString((NV_ERROR_CODE)prErrorCode) );
#else
		PStrPrintf( szError, cnErrStrLen, "Texture file %s error: %d", 
			pszFileNameWithPath, prErrorCode );
#endif
		break;
	}

	if ( bDisplay )
		ShowDataWarning( szError );
	else
	{	
		DebugString( OP_HIGH, szError );
		LogMessage( ERROR_LOG, szError );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

struct NVTTOutputHandler : public nvtt::OutputHandler
{
public:
	NVTTOutputHandler( LPD3DCTEXTURE2D pData ) 
		: pD3DTexture( pData ), nBytesRemaining( 0 ), nMipLevel( 0 ), dwOffset( 0 ), 
			pFileData( INVALID_FILE ), bWroteHeader( FALSE ) {}
	virtual ~NVTTOutputHandler() {}

private:
	LPD3DCTEXTURE2D pD3DTexture;
	D3DLOCKED_RECT tRect;
	int nBytesRemaining;
	int nMipLevel;
	DWORD dwOffset;
	HANDLE pFileData;
	BOOL bWroteHeader;

public:
	virtual void setFileHandle( HANDLE pHandle ) { pFileData = pHandle; }
	
	/// Indicate the start of a new compressed image that's part of the final texture.
	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
		ASSERT_RETURN( pD3DTexture );
		nBytesRemaining = size;
		nMipLevel = miplevel;
		dwOffset = 0;
		bWroteHeader = TRUE;
		dxC_MapTexture( pD3DTexture, nMipLevel, &tRect );
	}

	/// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
	virtual bool writeData(const void * data, int size)
	{
		if ( pFileData != INVALID_FILE )
		{
			FileWrite( pFileData, data, size );

			// If we haven't set the mip level yet, then we are still writing
			// out the header, so only write the data to disk.
			if ( ! bWroteHeader )
				return true;
		}

		memcpy( (BYTE*)dxC_pMappedRectData( tRect ) + dwOffset, data, size );
		nBytesRemaining -= size;
		dwOffset += size;

		if ( nBytesRemaining <= 0 )
			dxC_UnmapTexture( pD3DTexture, nMipLevel );

		return true;
	}
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef DXTLIB

static PRESULT sConvertTexture( 
	D3D_TEXTURE * pTexture, 
	TEXTURE_DEFINITION * pDefinition, 
	TEXTURE_DEFINITION * pDefOverride, 
	const char * pszFileNameWithPath, 
	DWORD dwReloadFlags, 
	int nDefaultFormat, 
	DWORD dwExtraConvertFlags, 
	const char* pszSaveFileName )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( pDefinition );

#ifdef ENGINE_TARGET_DX10
	ASSERTX_DO( ! sgbUseNVTT, "Need texture swizzle support before NVIDIA Texture Tools is supported by us for DX10" )
	{
		sgbUseNVTT = FALSE;
	}
#endif

	void * pSourceData = pTexture->pbLocalTextureFile;
	ASSERT_RETFAIL( pSourceData );
	int nSourceSize = pTexture->nTextureFileSize;
	V( sReleaseTextureResource( pTexture ) );

	D3DXIMAGE_INFO tImageInfo;
	V_RETURN( sGetTextureInfoFromFileInMemory( pTexture, &tImageInfo ) );

	int nSourceDepth = dx9_GetTextureFormatBitDepth( tImageInfo.Format );
	if ( nSourceDepth != 24 && nSourceDepth != 32 )
	{
		V( sTextureReportError( NV_DEPTH_IS_NOT_3_OR_4, pszFileNameWithPath, 0 ) );

		pDefinition->nFormat = TEXFMT_A8R8G8B8;
		tImageInfo.Format = (DXC_FORMAT)e_GetD3DTextureFormat( pDefinition->nFormat );
	}

	BOOL bWidthPow2  = ! ( tImageInfo.Width  & (tImageInfo.Width -1) );
	BOOL bHeightPow2 = ! ( tImageInfo.Height & (tImageInfo.Height-1) );
	if ( ! bWidthPow2 || ! bHeightPow2 )
	{
		V( sTextureReportError( NV_IMAGE_NOT_POWER_2, pszFileNameWithPath, 0 ) );

		if ( ! bWidthPow2 )
			tImageInfo.Width  = 1 << ( e_GetNumMIPs( tImageInfo.Width  ) );
		if ( ! bHeightPow2 )
			tImageInfo.Height = 1 << ( e_GetNumMIPs( tImageInfo.Height ) );
	}

	if ( pDefOverride && pDefOverride->nFormat != TEXFMT_UNKNOWN )
	{
		tImageInfo.Format = (DXC_FORMAT)e_GetD3DTextureFormat( pDefOverride->nFormat );
	}

	DXC_FORMAT tLoadFormat = tImageInfo.Format;
	int nLoadWidth  = tImageInfo.Width;
	int nLoadHeight = tImageInfo.Height;

	if ( pTexture->pfnPreSave && nDefaultFormat )
	{
		if ( ! dx9_IsCompressedTextureFormat( (DXC_FORMAT)nDefaultFormat ) )
			tLoadFormat = (DXC_FORMAT)nDefaultFormat;
	}

	if ( sgbUseNVTT  )
		tLoadFormat = D3DFMT_A8R8G8B8;

	SPD3DCTEXTURE2D pSourceD3DTexture;
	V_RETURN( dxC_Create2DTextureFromFileInMemoryEx(
		pSourceData, 
		nSourceSize, 
		nLoadWidth,
		nLoadHeight, 
		1,
		D3DC_USAGE_2DTEX_SCRATCH,
		tLoadFormat,
		(D3DCX_FILTER)( D3DX_FILTER_POINT ),
		(D3DCX_FILTER)( D3DX_FILTER_NONE ),
		&tImageInfo,
		&pSourceD3DTexture ) );

	// if the format read in had alpha
	BOOL bSourceAlpha = FALSE;
	if ( tImageInfo.Format == D3DFMT_A8R8G8B8 ||
		 tImageInfo.Format == D3DFMT_X8R8G8B8 ||
		 tImageInfo.Format == D3DFMT_A2B10G10R10 ||
		 DX9_BLOCK( tImageInfo.Format == D3DFMT_A2R10G10B10 || ) //Not supported by dx10
		 DX9_BLOCK( tImageInfo.Format == D3DFMT_A2W10V10U10 || ) //Not supported by dx10
		 DX9_BLOCK( tImageInfo.Format == D3DFMT_X8B8G8R8 || ) //Not supported by dx10 
		 tImageInfo.Format == D3DFMT_A8B8G8R8
		 )
		bSourceAlpha = TRUE;

	tImageInfo.Format = tLoadFormat;
	tImageInfo.Width  = nLoadWidth;
	tImageInfo.Height = nLoadHeight;

	// AE 2007.02.14 - I moved this to the end of the file, so that all MIP levels
	// would exist when calling pfnPreSave (which are needed for wardrobe blends).
	//if ( pTexture->pfnPreSave )
	//{
	//	V( pTexture->pfnPreSave( pDefinition, pSourceD3DTexture, &tImageInfo ) );
	//}


	BOOL bForce4Planes = FALSE;
#if 0 // We don't support 888 texture definitions any more.
	DX9_BLOCK
	(
		// temp workaround for requested 888 textures
		if ( pDefinition->nTexFormat == TEXFMT_R8G8B8 )
		{
			// 888s don't work with the nvDXT compression
			pDefinition->nTexFormat = TEXFMT_A8R8G8B8;
			tImageInfo.Format = (DXC_FORMAT)e_GetD3DTextureFormat( 
												pDefinition->nTexFormat );
			bForce4Planes = TRUE;
		}
	)
#endif


	// Create the .dds file texture
	D3DC_USAGE tPoolUsage = D3DC_USAGE_2DTEX;

	// must obey def override here
	int nWidth	   = tImageInfo.Width;
	int nHeight	   = tImageInfo.Height;
	int nMipLevels = pDefinition->nMipMapUsed;
	DXC_FORMAT nD3DFormat = (DXC_FORMAT)e_GetD3DTextureFormat( pDefinition->nFormat );
	if ( pDefOverride )
	{
		//ASSERT( pDefOverride->nWidth );
		nWidth	   = pDefOverride->nWidth  ? pDefOverride->nWidth  : tImageInfo.Width;
		nHeight    = pDefOverride->nHeight ? pDefOverride->nHeight : tImageInfo.Height;
		nD3DFormat = (DXC_FORMAT)e_GetD3DTextureFormat( pDefOverride->nFormat ? 
						pDefOverride->nFormat : pDefinition->nFormat );
		nMipLevels = pDefOverride->nMipMapUsed;
	}

	BOOL bForceBC5 = dwExtraConvertFlags & TEXTURE_CONVERT_FLAG_FORCE_BC5;

	if ( AppIsHellgate() )
	{
#if defined( DX10_BC5_TEXTURES ) && defined( ENGINE_TARGET_DX10 )
		if (  bForceBC5 && 
			( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT5NORMALMAP ) &&
			nD3DFormat == D3DFMT_DXT5 )
			nD3DFormat = DXGI_FORMAT_BC5_UNORM;
#endif
	}

	SPD3DCTEXTURE2D pFinalD3DTexture;
	V_RETURN( dxC_Create2DTexture(
		nWidth,
		nHeight,
		nMipLevels,
		tPoolUsage,
		nD3DFormat,
		&pFinalD3DTexture.p ) );

	D3DLOCKED_RECT tLRect;

	V_RETURN( dxC_MapTexture( pSourceD3DTexture, 0, &tLRect) );

	// set these based on options in TEXTURE_DEFINITION
	int nDepth = dx9_GetTextureFormatBitDepth( tImageInfo.Format );
	int nPlanes = ( pDefinition->nFormat == TEXFMT_L8 ) ? 1 : ( bSourceAlpha ) ? 4 : 3;
	if ( nPlanes != 1 )
		ASSERT_RETFAIL( nDepth == 24 || nDepth == 32 );

	// for 888 override:
	if ( !bForce4Planes && ( nPlanes != 1 ) && ! sgbUseNVTT )
	{
		// making sure everything matches up
		ASSERT_RETFAIL( ( nDepth == 24 && nPlanes == 3 ) || ( nDepth == 32 && nPlanes == 4 ) || ( nDepth == 64 && nPlanes == 4 ) );
	}

	trace( "!!!   CONVERTING TEXTURE: %s ... ", pszFileNameWithPath );

	PRESULT pr = S_OK;
	if ( sgbUseNVTT )
	{
		using namespace nvtt;

		// Regular image.
		//nv::Image image;
		//if ( ! image.load( pszFileNameWithPath ) )
		//{
		//	fprintf(stderr, "The file '%s' is not a supported image type.\n", pszFileNameWithPath );
		//	return E_FAIL;
		//}

		InputOptions tInputOptions;
		//tInputOptions.setTextureLayout(nvtt::TextureType_2D, image.width(), image.height());
		//tInputOptions.setMipmapData(image.pixels(), image.width(), image.height());
		if ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT5NORMALMAP )
			tInputOptions.setNormalMap( true );
		tInputOptions.setTextureLayout( TextureType_2D, tImageInfo.Width, tImageInfo.Height );
		tInputOptions.setMipmapData( dxC_pMappedRectData( tLRect ), tImageInfo.Width, tImageInfo.Height );
		tInputOptions.setWrapMode( WrapMode_Clamp );

		MipmapFilter eFilter;
		switch( (nvMipFilterTypes)pDefinition->nMIPFilter )
		{
		case kMipFilterPoint:
		case kMipFilterBox:
			eFilter = MipmapFilter_Box;
			break;

		case kMipFilterTriangle:
		case kMipFilterQuadratic:
		case kMipFilterCubic:
			eFilter = MipmapFilter_Triangle;
			break;

		case kMipFilterCatrom:
		case kMipFilterMitchell:
		case kMipFilterGaussian:
		case kMipFilterSinc:
		case kMipFilterBessel:
		case kMipFilterHanning:
		case kMipFilterHamming:
		case kMipFilterBlackman:
		case kMipFilterKaiser:
		default:
			eFilter = MipmapFilter_Kaiser;
			break;
		}
		tInputOptions.setMipmapFilter( eFilter );

		if( pDefinition->nMipMapUsed == 1 ) // just the base texture.
			tInputOptions.setMipmapGeneration( false );
		else
			tInputOptions.setMipmapGeneration( true, 
				( pDefinition->nMipMapUsed != 0 ) ?     (pDefinition->nMipMapUsed - 1)     : -1  );


		CompressionOptions tCompressionOptions;
		Format eFormat;
		switch ( pDefinition->nFormat )
		{
		case TEXFMT_DXT3:		
			eFormat = Format_DXT3; 
			break;

		case TEXFMT_DXT5:		
			if ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT5NORMALMAP )
				if( bForceBC5 )
					eFormat = Format_BC5;
				else
					eFormat = Format_DXT5n; 
			else
				eFormat = Format_DXT5;
			break;

		case TEXFMT_A8R8G8B8:	
			eFormat = Format_RGBA; 
			break;

		case TEXFMT_A1R5G5B5:
		case TEXFMT_A4R4G4B4:	
		case TEXFMT_L8:
			ASSERT_MSG( "Unsupported texture format conversion requested!" );
			return E_FAIL;

		case TEXFMT_DXT1:
		default:				
			if ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT1ALPHA )
				eFormat = Format_DXT1a;
			else
				eFormat = Format_DXT1;
		}
		tCompressionOptions.setFormat( eFormat );
		//tCompressionOptions.setPixelFormat()

		bool bDither = !!( TEST_MASK( pDefinition->dwConvertFlags, TEXTURE_CONVERT_FLAG_DITHERMIP0 ) );
		bool bBinAlpha = !!( eFormat == Format_DXT1a );
		tCompressionOptions.setQuantization( bDither, bDither, bBinAlpha, 127 );

		tCompressionOptions.setQuality( Quality_Highest );
		if ( TEST_MASK( pDefinition->dwConvertFlags, TEXTURE_CONVERT_FLAG_QUICKCOMPRESS ) )
			tCompressionOptions.setQuality( Quality_Fastest );

		OutputOptions tOutputOptions;
		NVTTOutputHandler tOutputHandler( pFinalD3DTexture );
		HANDLE pFileData = INVALID_FILE;
		if ( pszSaveFileName )
		{
			pFileData = FileOpen( pszSaveFileName, GENERIC_WRITE, 0, 
					CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN );
			ASSERT( pFileData != INVALID_FILE );
			
			if ( pFileData )
			{
				tOutputHandler.setFileHandle( pFileData );
				tOutputOptions.setOutputHeader( true );
			}
		}
		else
		{
			tOutputOptions.setOutputHeader( false );
		}
		tOutputOptions.setOutputHandler( &tOutputHandler );

		Compressor tCompressor;
		//tCompressor.enableCudaAcceleration( false );
		tCompressor.process( tInputOptions, tCompressionOptions, tOutputOptions );

		FileClose( pFileData );
	}
	else
	{
		nvCompressionOptions tCompressOptions;
		tCompressOptions.mipFilterType = (nvMipFilterTypes)pDefinition->nMIPFilter;
		//OLD: tCompressOptions.filterBlur			  = pDefinition->fBlurFactor;
		tCompressOptions.SetFilterSharpness( pDefinition->fBlurFactor );
		tCompressOptions.sharpenFilterType	  = (nvSharpenFilterTypes)pDefinition->nSharpenFilter;
		for ( int i = 0; i < dxC_GetNumMipLevels( pFinalD3DTexture ); i++ )
			tCompressOptions.sharpening_passes_per_mip_level[ i ] = pDefinition->nSharpenPasses;
		if ( !( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_SHARPENMIP0 ) )
			tCompressOptions.sharpening_passes_per_mip_level[ 0 ] = 0;
		tCompressOptions.bDitherMip0 = (0 != ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DITHERMIP0 ));
		tCompressOptions.bDitherColor = (0 != ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DITHERMIPS ));
			
		if ( 0 != ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_QUICKCOMPRESS ) )
			tCompressOptions.SetQuality(kQualityFastest, FASTEST_COMPRESSION_THRESHOLD);
		
		// checking above to force pow2, so can safely use 0 ("all")
		if( pDefinition->nMipMapUsed == 1 ) // just the base texture.
			tCompressOptions.DoNotGenerateMIPMaps();
		else
			tCompressOptions.GenerateMIPMaps(pDefinition->nMipMapUsed);
		
		if ( nWidth != tImageInfo.Width || nHeight != tImageInfo.Height )
			tCompressOptions.PreScaleImage((float)nWidth, (float)nHeight, tCompressOptions.mipFilterType);

		// non-input options:
		tCompressOptions.textureType = kTextureTypeTexture2D;
		TEXCONVERT_USER_DATA tUserData;
		tUserData.pTextureData = pFinalD3DTexture;
		tUserData.pFileData = INVALID_FILE;
		tCompressOptions.user_data = &tUserData;

		switch ( pDefinition->nFormat )
		{
		case TEXFMT_DXT3:		tCompressOptions.textureFormat= kDXT3; break;
		case TEXFMT_DXT5:		
			if ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT5NORMALMAP )
				if( bForceBC5 )
					tCompressOptions.textureFormat= k3Dc;
				else
					tCompressOptions.textureFormat= kDXT5_NM; 
			else
				tCompressOptions.textureFormat= kDXT5;
			break;
		case TEXFMT_A8R8G8B8:	tCompressOptions.textureFormat = k8888; break;
		//DX9_BLOCK ( case D3DFMT_R8G8B8:		tCompressOptions.textureFormat = k888; break; ) //No 3 component 8-bit DX10 textures
		case TEXFMT_A1R5G5B5:	tCompressOptions.textureFormat = k1555; break;
		case TEXFMT_A4R4G4B4:	tCompressOptions.textureFormat = k4444; break;
		//case TEXFMT_R5G6B5:		tCompressOptions.textureFormat = k565; break;
		//case TEXFMT_A8:			tCompressOptions.textureFormat = kA8; break;
		case TEXFMT_L8:			tCompressOptions.textureFormat = kL8; break;
		case TEXFMT_DXT1:
		default:				
			if ( pDefinition->dwConvertFlags & TEXTURE_CONVERT_FLAG_DXT1ALPHA )
				tCompressOptions.textureFormat = kDXT1a;
			else
				tCompressOptions.textureFormat = kDXT1;
		}

		// sanity check -- there have been errors with this
		ASSERT_RETFAIL( tCompressOptions.numMipMapsToWrite < 20 );

		nvPixelOrder nPixelOrder = (nPlanes == 4) ? DXC_9_10( nvBGRA : nvBGR, nvRGBA : nvRGB );

		if ( nPlanes == 1 )
			nPixelOrder = nvGREY;

		if ( pszSaveFileName )
		{
			tUserData.pFileData = FileOpen( pszSaveFileName, GENERIC_WRITE, 0, 
				CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN );
		}


		V_SAVE( pr, nvDDS::nvDXTcompress( (const unsigned char*)dxC_pMappedRectData( tLRect ),
			tImageInfo.Width, tImageInfo.Height, dxC_nMappedRectPitch( tLRect ), nPixelOrder,	
			&tCompressOptions, dx9_LoadAllMipSurfaces ) );

		FileClose( tUserData.pFileData );
	}

	V( dxC_UnmapTexture( pSourceD3DTexture, 0 ) );
	pSourceD3DTexture = NULL;

	trace( "done!\n" );

	if ( FAILED( pr ) )
	{
		V( sTextureReportError( pr, pszFileNameWithPath, 0 ) );
		return pr;
	}

	pDefinition->nMipMapLevels = e_GetNumMIPLevels( max( tImageInfo.Width, tImageInfo.Height ) );

	pTexture->pD3DTexture		= pFinalD3DTexture;
	pFinalD3DTexture			= NULL;

	D3DC_TEXTURE2D_DESC tDesc;
	V_RETURN( dxC_Get2DTextureDesc( pTexture, 0, &tDesc ) );

	pTexture->nWidthInPixels	= tDesc.Width;
	pTexture->nWidthInBytes		= dxC_GetTexturePitch( tDesc.Width, tDesc.Format );
	pTexture->nHeight			= tDesc.Height;
	pTexture->nFormat			= tDesc.Format;

	//Get usage/pool from specification
	pTexture->tUsage		= tPoolUsage;
	//	FREE ( NULL, pSourceData );

	V( dxC_TextureInitEngineResource( pTexture ) );

	if ( pTexture->pfnPreSave )
	{
		void * pTexParam = (void*)(LPD3DCTEXTURE2D)pTexture->pD3DTexture;
		//if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
		//	pTexParam = pTexture->ptSysmemTexture;
		V( pTexture->pfnPreSave( pDefinition, pTexParam, &tImageInfo, dwReloadFlags ) );
	}

	return S_OK;
}

#endif // DXTLIB

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sConvertTextureCheap( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefinition, TEXTURE_DEFINITION * pDefOverride, const char * pszFileNameWithPath, DWORD dwReloadFlags )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( pDefinition );

	void * pSourceData = pTexture->pbLocalTextureFile;
	ASSERT_RETFAIL( pSourceData );
	int nSourceSize = pTexture->nTextureFileSize;
	V( sReleaseTextureResource( pTexture ) );

	D3DXIMAGE_INFO tImageInfo;
	V_RETURN( sGetTextureInfoFromFileInMemory( pTexture, &tImageInfo ) );

	BOOL bWidthPow2  = ! ( tImageInfo.Width  & (tImageInfo.Width -1) );
	BOOL bHeightPow2 = ! ( tImageInfo.Height & (tImageInfo.Height-1) );
	if ( ! bWidthPow2 || ! bHeightPow2 )
	{
		V( sTextureReportError( NV_IMAGE_NOT_POWER_2, pszFileNameWithPath, 0 ) );
		//return;
	}

	D3DC_USAGE tUsagePool = D3DC_USAGE_2DTEX;

	// must obey def override here
	int nWidth	   = tImageInfo.Width;//D3DX_DEFAULT;  D3DX_DEFAULT==0
	int nHeight	   = tImageInfo.Height;//D3DX_DEFAULT;  D3DX_DEFAULT==0
	int nMipLevels = pDefinition->nMipMapUsed;
	DXC_FORMAT nD3DFormat = (DXC_FORMAT)e_GetD3DTextureFormat( pDefinition->nFormat );

	if ( pDefOverride )
	{
		//ASSERT( pDefOverride->nWidth );
		nWidth	   = pDefOverride->nWidth  ? pDefOverride->nWidth  : 0;//Same as D3DX_DEFAULT;
		nHeight    = pDefOverride->nHeight ? pDefOverride->nHeight : 0;//Same as DDX_DEFAULT;
		nD3DFormat = (DXC_FORMAT)e_GetD3DTextureFormat( pDefOverride->nFormat ? 
						pDefOverride->nFormat : pDefinition->nFormat );
		nMipLevels = pDefOverride->nMipMapUsed;
	}

#if defined( ENGINE_TARGET_DX10 ) && defined( DXTLIB )
	if( tImageInfo.ImageFileFormat == D3DXIFF_TGA )
		sConvertTexture( pTexture, pDefinition, pDefOverride, NULL, dwReloadFlags, nD3DFormat );
	else
#endif
	{
		PRESULT pr;
		//if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
		//{
		//	V_SAVE( pr, dxC_Create2DSysMemTextureFromFileInMemory(
		//		pSourceData,
		//		nSourceSize,
		//		nWidth,
		//		nHeight,
		//		nMipLevels,
		//		0,
		//		(DXC_FORMAT)nD3DFormat,
		//		&tImageInfo,
		//		pTexture->ptSysmemTexture ) );
		//}
		//else
		{
			V_SAVE( pr, dxC_Create2DTextureFromFileInMemoryEx(
				pSourceData,
				nSourceSize,
				nWidth,
				nHeight,
				nMipLevels,
				tUsagePool,
				nD3DFormat,
				(D3DCX_FILTER)(D3DX_FILTER_POINT ),
				(D3DCX_FILTER)(D3DX_FILTER_POINT ),
				&tImageInfo,
				&pTexture->pD3DTexture ) );
		}

		V( dxC_TextureInitEngineResource( pTexture ) );

		if ( FAILED( pr ) )
		{
			V( sTextureReportError( pr, pszFileNameWithPath, 0 ) );
			return pr;
		}
	}

	ASSERT( pTexture->pD3DTexture );

	D3DC_TEXTURE2D_DESC tDesc;
	V_RETURN( dxC_Get2DTextureDesc( pTexture, 0, &tDesc ) );

	pTexture->nWidthInPixels	= tDesc.Width;
	pTexture->nWidthInBytes		= dxC_GetTexturePitch( tDesc.Width, tDesc.Format );
	pTexture->nHeight			= tDesc.Height;
	pTexture->nFormat			= tDesc.Format;
	
	//Grab from specified use
	pTexture->tUsage		= tUsagePool;

	pDefinition->nMipMapLevels = dxC_GetNumMipLevels( pTexture->pD3DTexture );//->GetLevelCount();

	if ( pTexture->pfnPreSave )
	{
		V_RETURN( sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
		void * pTexParam = (void*)(LPD3DCTEXTURE2D)pTexture->pD3DTexture;
		//if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
		//	pTexParam = pTexture->ptSysmemTexture;
		V_RETURN( pTexture->pfnPreSave( pDefinition, pTexParam, &tImageInfo, dwReloadFlags ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSaveTargaFile( SPD3DCTEXTURE2D pSaveTexture, const char * pszFileNameWithPath )
{
	DX10_BLOCK( ASSERTX_RETVAL(0, E_NOTIMPL, "KMNV Todo: Need to implement Targa Saving in DX10...do we really need this???"); )

#ifdef ENGINE_TARGET_DX9

	struct HEADER {
		char  idlength;
		char  colourmaptype;
		char  datatypecode;
		short colourmaporigin;
		short colourmaplength;
		char  colourmapdepth;
		short x_origin;
		short y_origin;
		short width;
		short height;
		char  bitsperpixel;
		char  imagedescriptor;
	};
	// this struct gets byte-aligned, it seems -- causing an incorrect size, need to save piece-wise as a result
	//ASSERT( sizeof(HEADER) == 18 );

	SPDIRECT3DSURFACE9 pSrcSurface;
	V_RETURN( pSaveTexture->GetSurfaceLevel( 0, &pSrcSurface ) );
	D3DC_TEXTURE2D_DESC tDesc;
	V_RETURN( pSrcSurface->GetDesc( &tDesc ) );

	HEADER tHeader = {0};
	tHeader.datatypecode = 2;		// uncompressed RGB
	tHeader.width = (short)tDesc.Width;	// need to compensate for endian-ness?
	tHeader.height = (short)tDesc.Height;
	tHeader.bitsperpixel = 32;

	SPDIRECT3DSURFACE9 pSaveSurface;
	V_RETURN( dxC_GetDevice()->CreateOffscreenPlainSurface( tDesc.Width, tDesc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pSaveSurface, NULL ) );
	V_RETURN( D3DXLoadSurfaceFromSurface(
		pSaveSurface,
		NULL,
		NULL,
		pSrcSurface,
		NULL,
		NULL,
		D3DX_FILTER_NONE,
		0 ) );

	HANDLE hFile = FileOpen( pszFileNameWithPath, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL );
	ASSERTV_RETFAIL( hFile != INVALID_FILE, "Unable to open file: '%s' for writing", pszFileNameWithPath);

	// first write the header
	// have to write them piece-wise because of byte alignment on the struct :/
	FileWrite( hFile, (void *)&tHeader.idlength,		sizeof(tHeader.idlength) );
	FileWrite( hFile, (void *)&tHeader.colourmaptype,	sizeof(tHeader.colourmaptype) );
	FileWrite( hFile, (void *)&tHeader.datatypecode,	sizeof(tHeader.datatypecode) );
	FileWrite( hFile, (void *)&tHeader.colourmaporigin,	sizeof(tHeader.colourmaporigin) );
	FileWrite( hFile, (void *)&tHeader.colourmaplength,	sizeof(tHeader.colourmaplength) );
	FileWrite( hFile, (void *)&tHeader.colourmapdepth,	sizeof(tHeader.colourmapdepth) );
	FileWrite( hFile, (void *)&tHeader.x_origin,		sizeof(tHeader.x_origin) );
	FileWrite( hFile, (void *)&tHeader.y_origin,		sizeof(tHeader.y_origin) );
	FileWrite( hFile, (void *)&tHeader.width,			sizeof(tHeader.width) );
	FileWrite( hFile, (void *)&tHeader.height,			sizeof(tHeader.height) );
	FileWrite( hFile, (void *)&tHeader.bitsperpixel,	sizeof(tHeader.bitsperpixel) );
	FileWrite( hFile, (void *)&tHeader.imagedescriptor,	sizeof(tHeader.imagedescriptor) );

	D3DLOCKED_RECT tRect;
	PRESULT pr = S_OK;
	V_SAVE_GOTO( pr, close, pSaveSurface->LockRect( &tRect, NULL, D3DLOCK_READONLY ) );

	// then write the argb data
	for ( int i = tDesc.Height - 1; i >= 0; i-- )
	{
		BYTE * pBits = (BYTE*)dxC_pMappedRectData( tRect )+ i * dxC_nMappedRectPitch( tRect );
		FileWrite( hFile, (void *)pBits, dxC_nMappedRectPitch( tRect ) );
	}

	V( pSaveSurface->UnlockRect() );

	pr = S_OK;
close:
	FileClose(hFile);

	pSaveSurface = NULL;

	return pr;
#endif // DX9
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sSaveTexture( SPD3DCTEXTURE2D pSaveTexture, const char * pszFileNameWithPath, D3DC_IMAGE_FILE_FORMAT tFileFormat, BOOL bUpdatePak )
{
	ASSERT_RETINVALIDARG( pSaveTexture );
	ASSERT_RETINVALIDARG( pszFileNameWithPath );

	TCHAR szFullname[ DEFAULT_FILE_WITH_PATH_SIZE ];
	FileGetFullFileName( szFullname, pszFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );

	V_RETURN( dxC_SaveTextureToFile( szFullname, tFileFormat, pSaveTexture, bUpdatePak ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sLoadSourceTexture( 
	D3D_TEXTURE * pTexture, 
	const char * pszFileNameWithPath )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( pszFileNameWithPath );

	// load the original file
	DWORD dwSourceSize = 0;
	void* pSourceData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, pszFileNameWithPath, &dwSourceSize));

	if ( pSourceData == NULL )
	{
		if (    pTexture->nType != TEXTURE_LIGHTMAP 
			 && pTexture->nGroup != TEXTURE_GROUP_BACKGROUND 
			 && ( pTexture->dwFlags & TEXTURE_FLAG_NOERRORIFMISSING ) == 0 )
		{
#ifdef _DEBUG
			ASSERTV_MSG( "Texture file is missing:\n%s", pszFileNameWithPath );
#endif
		}
		if ( pTexture->pbLocalTextureFile )
			FREE( g_ScratchAllocator, pTexture->pbLocalTextureFile );
		pTexture->pbLocalTextureFile = NULL;
		pTexture->nTextureFileSize = 0;
		pTexture->dwFlags |= _TEXTURE_FLAG_LOAD_FAILED;
		return S_FALSE;
	}

	if ( pTexture->pbLocalTextureFile )
		FREE( g_ScratchAllocator, pTexture->pbLocalTextureFile );

	pTexture->pbLocalTextureFile = (BYTE*)pSourceData;
	pTexture->nTextureFileSize = dwSourceSize;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sLoadConvertedTexture( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefinition, const char * pszFileNameWithPath )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( pszFileNameWithPath );

	// Load the .dds file
	if (pTexture->pbLocalTextureFile)
	{
		FREE(g_ScratchAllocator, pTexture->pbLocalTextureFile);
	}

	DECLARE_LOAD_SPEC( spec, pszFileNameWithPath );
	spec.pool = g_ScratchAllocator;
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	pTexture->pbLocalTextureFile = (BYTE *)PakFileLoadNow(spec);
	ASSERT_RETFAIL(pTexture->pbLocalTextureFile);
	pTexture->nTextureFileSize = spec.bytesread;
	pDefinition->nFileSize = pTexture->nTextureFileSize;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sValidateCreateTextureResolution( int nWidth, int nHeight, D3D_TEXTURE * pTexture )
{
	if ( e_IsNoRender() )
		return S_OK;
	if ( dxC_IsPixomaticActive() )
		return S_OK;

	if ( nWidth <= 0 || nHeight <= 0 )
		return S_OK;

	if ( (DWORD)nWidth  > dx9_CapGetValue( DX9CAP_MAX_TEXTURE_WIDTH  ) || 
		 (DWORD)nHeight > dx9_CapGetValue( DX9CAP_MAX_TEXTURE_HEIGHT ) )
	{
		TEXTURE_DEFINITION * pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );

		ASSERTV_MSG( "Texture to create is too large for this GPU -- requested res %dx%d, max %dx%d\n\n%s",
			nWidth,
			nHeight,
			dx9_CapGetValue( DX9CAP_MAX_TEXTURE_WIDTH ),
			dx9_CapGetValue( DX9CAP_MAX_TEXTURE_HEIGHT ),
			pDef ? pDef->tHeader.pszName : "unknown filename" );

		return S_FALSE;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.05.09
DWORD sAdjustMipDownForDeviceRestrictions(unsigned nOriginalWidth, unsigned nOriginalHeight, DWORD dwMIPDown)
{
	const unsigned nMaxWidth = dx9_CapGetValue(DX9CAP_MAX_TEXTURE_WIDTH);
	const unsigned nMaxHeight = dx9_CapGetValue(DX9CAP_MAX_TEXTURE_HEIGHT);
	
	// Paranoid safety code.
	if ((nMaxWidth < 1) || (nMaxHeight < 1))
	{
		return dwMIPDown;
	}

	while (((nOriginalWidth >> dwMIPDown) > nMaxWidth) || ((nOriginalHeight >> dwMIPDown) > nMaxHeight))
	{
		++dwMIPDown;
	}

	return dwMIPDown;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sValidateTextureLoad( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDef )
{
	ASSERT_RETINVALIDARG( pTexture );
	if ( pTexture->nDefinitionId == INVALID_ID )
		return S_FALSE;

	if ( ! pDef )
		pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
	ASSERT_RETFAIL( pDef );

	const char szLogHeader[] = "TEXTURE:";

	if ( pTexture->nType == TEXTURE_NORMAL )
	{
		if ( pTexture->nFormat != DXC_9_10( D3DFMT_DXT5, DXGI_FORMAT_BC5_UNORM ) )
			ShowDataWarning( "\t%s \tNormal maps must be DXT5 with \"DXT5 Normal map\" selected! \t%s", szLogHeader, pDef->tHeader.pszName );
	} else if ( pTexture->nType == TEXTURE_SPECULAR )
	{
		if ( AppIsHellgate() && pTexture->nGroup != TEXTURE_GROUP_BACKGROUND && pTexture->nFormat != D3DFMT_DXT5 && pTexture->nFormat != D3DFMT_DXT3 )
			ShowDataWarning( "\t%s \tSpecular textures must be DXT3 or DXT5! \t%s", szLogHeader, pDef->tHeader.pszName );
		else if( AppIsTugboat() && pTexture->nFormat != D3DFMT_DXT5 && pTexture->nFormat != D3DFMT_DXT3 && pTexture->nFormat != D3DFMT_DXT1 )
			ShowDataWarning( "\t%s \tSpecular textures must be DXT1, DXT3 or DXT5! \t%s", szLogHeader, pDef->tHeader.pszName );
	} else if ( pTexture->nType == TEXTURE_LIGHTMAP )
	{
		if ( pTexture->nFormat != D3DFMT_DXT1 )
			ShowDataWarning( "\t%s \tLightmap textures should be DXT1! \t%s", szLogHeader, pDef->tHeader.pszName );
	} else if ( pTexture->nGroup != TEXTURE_GROUP_UI )
	{
		if ( ! dx9_IsCompressedTextureFormat( (DXC_FORMAT)pTexture->nFormat ) )
			ShowDataWarning( "\t%s \tTexture should be compressed! (DXT1-5) \t%s", szLogHeader, pDef->tHeader.pszName );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sCreateTextureFromMemory( D3D_TEXTURE * pTexture, TEXTURE_DEFINITION * pDefOverride, DWORD dwReloadFlags, D3DFORMAT nDefaultFormat )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pTexture );

	// CHB 2006.10.09 - Don't allow loading of unused textures.
	ASSERT_RETFAIL(e_TextureSlotIsUsed(static_cast<TEXTURE_TYPE>(pTexture->nType)));

	PRESULT pr = S_OK;

	{
		//TEXTURE_DEFINITION * pDef = pTexture->nDefinitionId == INVALID_ID ? NULL : (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		//trace( "Texture create from memory! [%4d] \"%s\"\n", pTexture->nId, pDef ? pDef->tHeader.pszName : "" );
	}

	V( sReleaseTextureResource( pTexture ) );

	int nWidth	   = 0;// 0 == D3DX_DEFAULT
	int nHeight	   = 0;// 0 == D3DX_DEFAULT
	int nMipLevels = D3DX_FROM_FILE;
	DXC_FORMAT nD3DFormat = D3DFMT_FROM_FILE;
	D3DC_USAGE tUsagePool = pTexture->tUsage;

	if ( dwReloadFlags & TEXTURE_LOAD_SOURCE_ONLY )
	{
		// load the source as a full texture, with whatever MIPs it has (or doesn't have)
		nMipLevels = D3DX_FROM_FILE;
		// load the source as a full texture, as per the definition (MIPs and all)
		//TEXTURE_DEFINITION * pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		//if ( pDef )
		//{
		//	nMipLevels = pDef->nMipMapUsed;
		//}
	}

	if ( pDefOverride )
	{
		nWidth	   = pDefOverride->nWidth  ? pDefOverride->nWidth  : 0;// 0 == D3DX_DEFAULT
		nHeight    = pDefOverride->nHeight ? pDefOverride->nHeight : 0;// 0 == D3DX_DEFAULT
		nD3DFormat = (DXC_FORMAT)( pDefOverride->nFormat ? 
			e_GetD3DTextureFormat( pDefOverride->nFormat ) : D3DFMT_FROM_FILE );
		nMipLevels = pDefOverride->nMipMapUsed;
	}

	if ( !pDefOverride || pDefOverride->nFormat <= 0 )
		nD3DFormat = nDefaultFormat;

	DWORD dwMIPDown = 0;

#ifdef ENGINE_TARGET_DX9
	if ( ! e_IsNoRender() )
	{
		// in case the device was lost, try to reset it before proceeding
		V_SAVE_GOTO( pr, cleanup, e_CheckValid() );
	}
#endif

	if ( pTexture->pfnRestore )
	{
		V_SAVE_GOTO( pr, cleanup, pTexture->pfnRestore( pTexture ) );
	}
	else if ( pTexture->pbLocalTextureFile )
	{
		// restoring texture from file in memory

		D3DXIMAGE_INFO tOrigImageInfo;
		V_SAVE_GOTO( pr, cleanup, dxC_GetImageInfoFromFileInMemory( pTexture->pbLocalTextureFile, pTexture->nTextureFileSize, &tOrigImageInfo ) );

		if ( pDefOverride && nWidth > 0 && nHeight > 0 )
		{
			// if the new width/height are the same pow2 smaller than the orig, use the MIP skip method instead of refiltering
			int nWidthDown  = tOrigImageInfo.Width  / nWidth;
			int nHeightDown = tOrigImageInfo.Height / nHeight;
			if ( nWidthDown == nHeightDown && nWidthDown > 1 )
			{
				// if nWidthDown is a pow2
				if ( 0 == ( nWidthDown & ( nWidthDown - 1 ) ) )
				{
					// figure out what pow2
					DWORD dwMask = nWidthDown;
					while ( dwMask > 1 )
					{
						dwMask >>= 1;
						dwMIPDown++;
					}
					//dwMIPDown = e_GetNumMIPLevels( nWidthDown );
					nWidth  = 0;// 0 == D3DX_DEFAULT
					nHeight = 0;// 0 == D3DX_DEFAULT
				}
			}
		}

		// check group and type, then mip appropriately
		int nGroup = pTexture->nGroup;
		int nType  = pTexture->nType;
		if ( !pDefOverride && nGroup != INVALID_ID && nType != INVALID_ID /*&& nGroup != TEXTURE_GROUP_UI*/ )
		{
			nWidth  = tOrigImageInfo.Width;
			nHeight = tOrigImageInfo.Height;
			int nMipDown = e_TextureBudgetAdjustWidthAndHeight( (TEXTURE_GROUP)nGroup, (TEXTURE_TYPE)nType, nWidth, nHeight );
			//int nMipW = e_GetNumMIPLevels( nWidth );
			//int nMipH = e_GetNumMIPLevels( nHeight );
			//nNewMIPLevels = max( nMipW, nMipH );
			//ASSERT_RETURN( nNewMipLevels > 0 );
			//nMipLevels = 0;  // specify auto-fill instead of load-from-file
			nWidth = 0;// 0 == D3DX_DEFAULT
			nHeight = 0;// 0 == D3DX_DEFAULT
			dwMIPDown += nMipDown;
		}

	    DWORD dwDefFilter = D3DX_FILTER_SRGB;
		// for some reason, PNGs don't seem to SRGB convert correctly for filtering  -- only matters for linear (and other math-based) filtering
		if ( tOrigImageInfo.ImageFileFormat == D3DXIFF_PNG )
			dwDefFilter &= ~(D3DX_FILTER_SRGB);

		// CHB 2007.05.09 - Adjust mip down as necessary to
		// accomodate device texture size restrictions.
		dwMIPDown = sAdjustMipDownForDeviceRestrictions(tOrigImageInfo.Width, tOrigImageInfo.Height, dwMIPDown);

		// make sure we don't try to MIP down further than we have MIP levels
		dwMIPDown = MIN( (DWORD)( tOrigImageInfo.MipLevels - 1 ), dwMIPDown );

		if ( nMipLevels == D3DX_FROM_FILE )
		{
			nMipLevels = tOrigImageInfo.MipLevels - (int)dwMIPDown;
		} else
		{
			nMipLevels = nMipLevels - (int)dwMIPDown;
		}

		// CHB 2007.05.09 - Validate correct width and height.
		// Previously this checking was not useful because it relied
		// on 'nWidth' and 'nHeight' which were often set to 0
		// (D3DX_DEFAULT).
		int nRealWidth = tOrigImageInfo.Width >> dwMIPDown;
		int nRealHeight = tOrigImageInfo.Height >> dwMIPDown;
		// validate texture size/format
		if ( S_OK != sValidateCreateTextureResolution( nRealWidth, nRealHeight, pTexture ) )
		{
			pr = S_FALSE;
			goto cleanup;
		}

		if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
		{
			// rarely used, except with Pixomatic

			MEMORYPOOL* pPool = NULL;
			if ( TEST_MASK( pTexture->dwFlags, TEXTURE_FLAG_USE_SCRATCHMEM ) )
				pPool = g_ScratchAllocator;

			D3DXIMAGE_INFO tImageInfo;
			V_SAVE_GOTO( pr, cleanup, dxC_Create2DSysMemTextureFromFileInMemory(
				pTexture->pbLocalTextureFile,
				pTexture->nTextureFileSize,
				nWidth,
				nHeight,
				nMipLevels,
				dwMIPDown,
				nD3DFormat,
				&tImageInfo,
				pTexture->ptSysmemTexture,
				pPool ) );

			ASSERT( pTexture->ptSysmemTexture );
			D3DC_TEXTURE2D_DESC tDesc;
			V_SAVE_GOTO( pr, cleanup, dxC_Get2DTextureDesc( pTexture, 0, &tDesc ) );

			if ( !pDefOverride )
			{
				ASSERT( tOrigImageInfo.Width  == tImageInfo.Width );
				ASSERT( tOrigImageInfo.Height == tImageInfo.Height );
				pTexture->nHeight		 = tImageInfo.Height;
				pTexture->nWidthInPixels = tImageInfo.Width;
				if ( nDefaultFormat > 0 )
					pTexture->nFormat	 = nD3DFormat;
				else
					pTexture->nFormat	 = tImageInfo.Format;
			} else {
				ASSERT( ( tOrigImageInfo.Width  >> dwMIPDown ) == tDesc.Width );
				ASSERT( ( tOrigImageInfo.Height >> dwMIPDown ) == tDesc.Height );
				pTexture->nHeight		 = nHeight > 0 ? nHeight			: tDesc.Height;
				pTexture->nWidthInPixels = nWidth  > 0 ? nWidth				: tDesc.Width;
				pTexture->nFormat		 = nD3DFormat ? nD3DFormat : tDesc.Format;
			}
		} 
		else 
		{
			// common case - loading texture from a file

			// load texture and existing mip levels
			DWORD dwMIPFilter = D3DC_SKIP_DDS_MIP_LEVELS( dwMIPDown, D3DX_FILTER_NONE );
			D3DXIMAGE_INFO tImageInfo;

			V_SAVE_GOTO( pr, cleanup, dxC_Create2DTextureFromFileInMemoryEx(
				pTexture->pbLocalTextureFile,
				pTexture->nTextureFileSize,
				nWidth,
				nHeight,
				nMipLevels,
				tUsagePool,
				nD3DFormat,
				(D3DCX_FILTER)D3DX_FILTER_NONE,
				(D3DCX_FILTER)dwMIPFilter,
				&tImageInfo,
				&pTexture->pD3DTexture ) );

			pTexture->nHeight		 = tImageInfo.Height;
			pTexture->nWidthInPixels = tImageInfo.Width;
			pTexture->nFormat		 = tImageInfo.Format;

		}
		pTexture->tUsage = tUsagePool;
	}
	else if ( pTexture->pbLocalTextureData ) 
	{
		// rarely used, except for UI empty textures like the font textures

		// This codepath only supports updating the first MIP level of a texture!
		// It is intended for one-level textures like UI font textures.

		if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
		{
			if ( ! pTexture->ptSysmemTexture )
			{
				V_SAVE_GOTO( pr, cleanup, dxC_TextureSystemMemNewEmptyInPlace( pTexture ) );
			}
			ASSERT_DO( NULL != pTexture->ptSysmemTexture )
			{
				pr = E_FAIL;
				goto cleanup;
			}

			// Lock and update sysmem texture top MIP level
			D3DLOCKED_RECT tRect;
			DXC_FORMAT tFormat = pTexture->ptSysmemTexture->GetFormat();

			// ONLY DOING THE TOP MIPLEVEL
			for ( int i = 0; i < 1; ++i )
			{
				V_CONTINUE( dxC_MapSystemMemTexture( pTexture->ptSysmemTexture, i, &tRect ) );

				ASSERT_DO( NULL != tRect.pBits )
				{
					dxC_UnmapSystemMemTexture( pTexture->ptSysmemTexture, i );
					continue;
				}

				size_t nSize = pTexture->ptSysmemTexture->GetLevelSize( i );
				MemoryCopy( tRect.pBits, nSize, pTexture->pbLocalTextureData, nSize );

				dxC_UnmapSystemMemTexture( pTexture->ptSysmemTexture, i );
			}
		}
		else
		{
			// validate texture size/format
			if ( S_OK != sValidateCreateTextureResolution( pTexture->nWidthInPixels, pTexture->nHeight, pTexture ) )
			{
				pr = S_FALSE;
				goto cleanup;
			}

			V_SAVE_GOTO( pr, cleanup, dxC_Create2DTexture( 
				pTexture->nWidthInPixels,
				pTexture->nHeight, 
				1,
				tUsagePool,
				*(DXC_FORMAT*)&pTexture->nFormat,
				&pTexture->pD3DTexture,
				pTexture->pbLocalTextureData) );

			pTexture->tUsage = tUsagePool;
		}
	}
	ASSERT( pTexture->nFormat != 0 );
	ASSERT( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM || pTexture->tUsage >= 0 );

	V( dxC_TextureInitEngineResource( pTexture ) );

	pr = S_OK;
cleanup:

	if ( dxC_IsPixomaticActive() || 0 == ( dwReloadFlags & TEXTURE_LOAD_DONT_FREE_LOCAL ) )
	{
		// only keep around system copies for video-memory-only textures if running DX9
		DX9_BLOCK( if ( pTexture->tUsage != D3DC_USAGE_2DTEX_STAGING || pTexture->tUsage != D3DC_USAGE_2DTEX ) )
		{
			if (pTexture->pbLocalTextureFile)
			{
				FREE(g_ScratchAllocator, pTexture->pbLocalTextureFile);
				pTexture->pbLocalTextureFile = NULL;
			}
			if (pTexture->pbLocalTextureData)
			{
				FREE(g_ScratchAllocator, pTexture->pbLocalTextureData);
				pTexture->pbLocalTextureData = NULL;
			}
		}
	}

	V( sValidateTextureLoad( pTexture, NULL ) );

	return pr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sShowTextureWarnings( D3D_TEXTURE * pTexture, const char * pszFilename, int nGroup, int nType )
{
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( 0 == ( pGlobals->dwFlags & GLOBAL_FLAG_DATA_WARNINGS ) )
		return;

	// If the texture hasn't been loaded yet, then we can't warn about formats.
	if (     ! pTexture->pD3DTexture 
		|| ( TEST_MASK( pTexture->dwFlags, TEXTURE_FLAG_SYSMEM ) && ! pTexture->ptSysmemTexture ) )
		return;


	if ( nGroup				== TEXTURE_GROUP_BACKGROUND &&
		 nType				== TEXTURE_SELFILLUM &&
		 pTexture->nFormat	!= D3DFMT_DXT1 )
	{
		ASSERTV_MSG( "Error with texture file:  Background Lightmap textures should be in DXT1 format!\n\n%s", pszFilename );
	}

	//if ( nGroup				== TEXTURE_GROUP_PARTICLE &&
	//	 pTexture->nFormat	== D3DFMT_DXT1 )
	//{
	//	ASSERTV_MSG( "Error with texture file:  Particle textures should NOT be in DXT1 format!\n\n%s", pszFilename );
	//}

	if ( nGroup				== TEXTURE_GROUP_BACKGROUND &&
		 nType				== TEXTURE_SPECULAR &&
		 pTexture->nFormat	== D3DFMT_DXT1 )
	{
		ASSERTV_MSG( "Error with texture file:  Background Specular textures should NOT be in DXT1 format!\n\n%s", pszFilename );
	}

	if ( nGroup				!= TEXTURE_GROUP_UI &&
		AppIsHellgate() &&
		pTexture->nFormat != 0 &&
		! dx9_IsCompressedTextureFormat( (DXC_FORMAT) pTexture->nFormat ) )
	{
		ASSERTV_MSG( "%s\nError with texture file:  All Textures in Hellgate should be compressed!", pszFilename );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_DiffuseTextureDefinitionLoadedCallback(
	void * pDef,
	EVENT_DATA * pEventData )
{
	TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) pDef;
	BOOL bBackground = pEventData->m_Data1 != 0;
	BOOL bFillingPak = AppCommonGetFillPakFile();
	D3D_MESH_DEFINITION * pMeshDef = dx9_MeshDefinitionGet( int(pEventData->m_Data2) );

	// if we're filling the pak file, OK to have no meshdef -- otherwise, bail
	if ( ! bFillingPak && ! pMeshDef )
		return;

	if (! bBackground &&  PStrCmp( pTextureDefinition->pszMaterialName, "Default" ) == 0 )
	{
		int nMatID;
		V( e_MaterialNew( &nMatID, "Animated" ) );
		if ( pMeshDef )
			pMeshDef->nMaterialId = nMatID;
	} else {
		int nMatID;
		V( e_MaterialNew( &nMatID, pTextureDefinition->pszMaterialName ) );
		if ( pMeshDef )
			pMeshDef->nMaterialId = nMatID;
	}

	if ( pMeshDef )
	{
		D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( int(pEventData->m_Data3), int(pEventData->m_Data4) );
		ASSERT( pModelDef );
		V( dx9_MeshApplyMaterial( pMeshDef, pModelDef, NULL, -1, -1 ) );	// CHB 2006.12.08
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_TextureGetFullFileName (
	char* pszFullFileName,
	int nMaxLen,
	const char * pszBaseFolder,
	const char * pszTextureSetFolder,
	const char * pszFilename )
{
	char szTempFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	int nBufLen = min( nMaxLen, DEFAULT_FILE_WITH_PATH_SIZE );
	PStrCopy( szTempFolder, pszBaseFolder, nBufLen );
	PStrForceBackslash( szTempFolder, nBufLen );
	if ( pszTextureSetFolder && pszTextureSetFolder[ 0 ] != 0 )
	{
		PStrCat( szTempFolder, pszTextureSetFolder, nBufLen );
		PStrForceBackslash( szTempFolder, nBufLen );
	}
	const char * pszCurExt = PStrGetExtension( pszFilename );
	if ( ! pszCurExt )
	{
		// fill in the default extension
		char szExt[8];
		if ( AppIsTugboat() )
		{
			PStrCopy( szExt, 8, "png", 4 );
		} else
		{
			PStrCopy( szExt, 8, "tga", 4 );
		}
		PStrPrintf( pszFullFileName, nBufLen, "%s%s.%s", szTempFolder, pszFilename, szExt );
	}
	else
	{
		PStrPrintf( pszFullFileName, nBufLen, "%s%s", szTempFolder, pszFilename );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_TextureNewFromFile (
	int* pnTextureID,
	const char* pszFileNameWithPath,
	int nGroup,
	int nType, 
	DWORD dwTextureFlags, 
	int nDefaultFormat /*= 0*/, 
	void* pfnRestore /*= 0*/, 
	void* pfnPreSave /*= 0*/,
	void* pfnPostLoad /*= 0*/,
	TEXTURE_DEFINITION* pDefOverride /*= 0*/,
	DWORD dwReloadFlags /*= 0*/,
	float fDistanceToCamera,
	int usageOverride /*= D3DC_USAGE_INVALID*/,
	PAK_ENUM ePakfile /*= PAK_DEFAULT*/ )
{
	ASSERT_RETINVALIDARG( pnTextureID );
	*pnTextureID = INVALID_ID;
	ASSERT_RETINVALIDARG( pszFileNameWithPath );
	//TIMER_START( pszFilename ); 

	// CHB 2006.10.09 - Don't allow loading of unused textures.
	// CHB 2007.05.03 - No longer assert for this case; fail instead.
	//ASSERT_RETFAIL(e_TextureSlotIsUsed(static_cast<TEXTURE_TYPE>(nType)));
	if (!e_TextureSlotIsUsed(static_cast<TEXTURE_TYPE>(nType)))
	{
		return S_FALSE;
	}

	if ( dxC_IsPixomaticActive() )
	{
		// All textures are sysmem textures!
		dwTextureFlags |= TEXTURE_FLAG_SYSMEM;
	}


	int nPriority = 0;
	{
		if ( nType != INVALID_ID && nGroup != INVALID_ID )
		{
			const TEXTURE_TYPE_DATA * pTextureTypeData = (const TEXTURE_TYPE_DATA *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_TEXTURE_TYPES, nType );
			ASSERT_RETFAIL( pTextureTypeData );
			nPriority = pTextureTypeData->pnPriority[ nGroup ];
		}
		nPriority = AsyncFileAdjustPriorityByDistance(nPriority, fDistanceToCamera);
	}

	// get the definition
	int nDefinitionId;
	{
		nDefinitionId = DefinitionGetIdByFilename( 
			DEFINITION_GROUP_TEXTURE, 
			pszFileNameWithPath, 
			nPriority, 
			(dwReloadFlags & TEXTURE_LOAD_NO_ASYNC_DEFINITION) != 0,
			TRUE,
			ePakfile);
	}
	ASSERT_RETFAIL( nDefinitionId != INVALID_ID );

	if ( nType == TEXTURE_DIFFUSE && nGroup != TEXTURE_GROUP_UI )
	{
		// if diffuse texture, load the texture definition's material
		TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, nDefinitionId );
		EVENT_DATA tEventData( ( nGroup == TEXTURE_GROUP_BACKGROUND ) ? 1 : 0, INVALID_ID, INVALID_ID );
		if ( pTextureDefinition )
		{
			dxC_DiffuseTextureDefinitionLoadedCallback( pTextureDefinition, &tEventData );
		} else {
			DefinitionAddProcessCallback( DEFINITION_GROUP_TEXTURE, nDefinitionId, 
				dxC_DiffuseTextureDefinitionLoadedCallback, &tEventData );
		}
	} 

	D3D_TEXTURE * pTexture;
	{
		pTexture = dxC_TextureGet( nDefinitionId );
	}

	// see if the texture is already loaded
	if ( pTexture )
	{
		BOOL bMatch = TRUE;
		if ( pDefOverride )
		{
			if ( !( ( pDefOverride->nFormat <= 0 || e_GetD3DTextureFormat( pDefOverride->nFormat ) == pTexture->nFormat ) &&
					( pDefOverride->nWidth  <= 0 || pDefOverride->nWidth  == pTexture->nWidthInPixels  ) &&
					( pDefOverride->nHeight <= 0 || pDefOverride->nHeight == pTexture->nHeight ) &&
					( D3DC_USAGE_2DTEX == pTexture->tUsage ) ) )
				bMatch = FALSE;
		}

		if ( ( dwTextureFlags & TEXTURE_FLAG_SYSMEM ) != ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM ) )
		{
			ASSERTV_MSG( "ERROR: New texture requested that matches existing texture, except for the SYSMEM flag!\n\n%s", pszFileNameWithPath );
			//bMatch = FALSE;
		}

		if ( bMatch && ( 0 == ( dwReloadFlags & TEXTURE_LOAD_FORCECREATETFILE ) ) )
		{
			//e_TextureAddRef( nDefinitionId );
			*pnTextureID = nDefinitionId;
			return S_OK;
		}
	} 
	else 
	{
		// add the texture to the list of textures
		V_RETURN( dxC_TextureAdd( nDefinitionId, &pTexture, nGroup, nType ) );
		ASSERT_RETFAIL( pTexture );
	}

	pTexture->nDefinitionId = nDefinitionId;
	pTexture->pfnRestore = (PFN_TEXTURE_RESTORE)pfnRestore;
	pTexture->pfnPreSave = (PFN_TEXTURE_PRESAVE)pfnPreSave;
	pTexture->pfnPostLoad = (PFN_TEXTURE_POSTLOAD)pfnPostLoad;
	pTexture->dwFlags = dwTextureFlags;
	pTexture->tUsage = (D3DC_USAGE)usageOverride != D3DC_USAGE_INVALID ? (D3DC_USAGE)usageOverride : D3DC_USAGE_2DTEX;
	pTexture->nUserID = INVALID_ID;
	pTexture->nGroup = nGroup;
	pTexture->nType = nType;
	pTexture->nPriority = nPriority;

	if ( nType == TEXTURE_NORMAL || nType == TEXTURE_SPECULAR )
		SET_MASK( dwReloadFlags, TEXTURE_LOAD_ADD_TO_HIGH_PAK );
	else
	{
		BOOL bAllowDirectFileCheck = AppCommonGetUpdatePakFile() || c_GetToolMode();
		if ( bAllowDirectFileCheck )
		{
			char szLowFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			V_DO_RESULT( S_OK, e_ModelDefinitionGetLODFileName( szLowFilepath, ARRAYSIZE( szLowFilepath ), pszFileNameWithPath ) )
			{
				// We got a low filename from the standard filename
				if ( FileExists( szLowFilepath ) )
					SET_MASK( dwReloadFlags, TEXTURE_LOAD_ADD_TO_HIGH_PAK );
			}
		}
	}

	// Now do all of the stuff you would do if you were reloading the texture
	{
		V_RETURN( dx9_TextureReload ( pTexture, pszFileNameWithPath, dwTextureFlags, (D3DFORMAT)nDefaultFormat, dwReloadFlags, pDefOverride, ePakfile ) );
	}

	// texture warnings
	sShowTextureWarnings( pTexture, pszFileNameWithPath, nGroup, nType );

	ASSERT_RETFAIL( pTexture->nDefinitionId != INVALID_ID );

	//e_TextureAddRef( nDefinitionId );
	*pnTextureID = nDefinitionId;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Sets the texture field of a geometry node with a point to a BMP file in memory

PRESULT e_TextureNewFromFileInMemory ( 
	int* pnTextureId,
	BYTE* pbData, 
	unsigned int nDataSize,
	int nGroup,
	int nType, 
	DWORD dwLoadFlags /*= 0*/ )
{
	ASSERT_RETINVALIDARG( pnTextureId );
	*pnTextureId = INVALID_ID;
	//int nTextureId = gnTextureIdNext;
	//gnTextureIdNext++;
	//D3D_TEXTURE* pTexture = HashAdd(gtTextures, nTextureId);
	D3D_TEXTURE * pTexture;
	V_RETURN( dxC_TextureAdd( INVALID_ID, &pTexture, nGroup, nType ) );
	ASSERT_RETFAIL(pTexture);

	pTexture->nGroup = nGroup;
	pTexture->nType = nType;

	// make sure it doesn't have a texture
	ASSERT_RETFAIL( !pTexture->pbLocalTextureFile );
	pTexture->pbLocalTextureFile = pbData;
	pTexture->nTextureFileSize = nDataSize;
	pTexture->tUsage = D3DC_USAGE_2DTEX;

	V_RETURN( sCreateTextureFromMemory( pTexture, NULL, dwLoadFlags ) );
	D3DXIMAGE_INFO  tImageInfo;
	V_RETURN( sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
	pTexture->nWidthInPixels  = tImageInfo.Width;
	pTexture->nHeight		  = tImageInfo.Height;
	pTexture->nWidthInBytes   = dxC_GetTexturePitch( tImageInfo.Width, tImageInfo.Format );
	pTexture->nFormat         = tImageInfo.Format;
	pTexture->nD3DTopMIPSize = dx9_GetTextureMIPLevelSizeInBytes( pTexture );

	//dxC_TextureAddRef( pTexture );

	*pnTextureId = pTexture->nId;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Creates a new empty texture in memory
PRESULT dx9_TextureNewEmpty ( 
	int* pnTextureId,
	int nWidth, 
	int nHeight,
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup, 
	int nType,
	D3DC_USAGE tUsagePool, 
	PFN_TEXTURE_RESTORE pfnRestore,
	int nUserID )
{
	ASSERT_RETINVALIDARG( pnTextureId );
	*pnTextureId = INVALID_ID;

	D3D_TEXTURE * pTexture;
	V_RETURN( dxC_TextureAdd( INVALID_ID, &pTexture, nGroup, nType ) );
	ASSERT_RETFAIL(pTexture);

	if ( dxC_IsPixomaticActive() )
	{
		pTexture->dwFlags |= TEXTURE_FLAG_SYSMEM;
	}


	V_RETURN( dx9_TextureNewEmptyInPlace( pTexture->nId, nWidth, nHeight, nMIPLevels, tFormat, nGroup, nType, tUsagePool, pfnRestore, nUserID ) );

	//dxC_TextureAddRef( pTexture );

	*pnTextureId = pTexture->nId;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Creates a new empty texture in memory
PRESULT dx9_TextureNewEmptyInPlace ( 
	int nTextureId,
	int nWidth, 
	int nHeight, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup,
	int nType,
	D3DC_USAGE tUsagePool, 
	PFN_TEXTURE_RESTORE pfnRestore, 
	int nUserID )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( nTextureId != INVALID_ID );
	D3D_TEXTURE* pTexture = HashGet(gtTextures, nTextureId);
	ASSERT_RETFAIL(pTexture);

	pTexture->nGroup = nGroup;
	pTexture->nType = nType;
	pTexture->tUsage = tUsagePool;
	pTexture->pfnRestore = pfnRestore;
	pTexture->nUserID = nUserID;

	if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
	{
		MEMORYPOOL* pPool = NULL;
		if ( TEST_MASK( pTexture->dwFlags, TEXTURE_FLAG_USE_SCRATCHMEM ) )
			pPool = g_ScratchAllocator;
		UINT nSizeInMemory = 0;
		V_RETURN( sSystemMemoryTextureCreate( nWidth, nHeight, nMIPLevels, tFormat, pTexture->ptSysmemTexture, nSizeInMemory, pPool ) );

		pTexture->nWidthInPixels	= nWidth;
		pTexture->nHeight			= nHeight;
		pTexture->nFormat			= (int)tFormat;
		pTexture->nWidthInBytes		= dxC_GetTexturePitch( nWidth, tFormat );
		pTexture->nD3DTopMIPSize	= dx9_GetTextureMIPLevelSizeInBytes( pTexture );
	}
	else
	{
		ASSERT_RETFAIL( tUsagePool >= 0 );

		// make a texture
		V_RETURN( dxC_Create2DTexture(
			nWidth,
			nHeight,
			nMIPLevels,
			tUsagePool,
			tFormat,
			&pTexture->pD3DTexture ) );

		V( dxC_TextureInitEngineResource( pTexture ) );

		D3DXIMAGE_INFO  tImageInfo;
		V_RETURN( sGetTextureInfoFromDesc( pTexture, &tImageInfo ) );
		pTexture->nWidthInPixels  = tImageInfo.Width;
		pTexture->nHeight		  = tImageInfo.Height;
		pTexture->nWidthInBytes	  = dxC_GetTexturePitch( tImageInfo.Width, tImageInfo.Format );
		pTexture->nFormat         = tImageInfo.Format;
		pTexture->nD3DTopMIPSize = dx9_GetTextureMIPLevelSizeInBytes( pTexture );
	}

	if ( pfnRestore )
		pfnRestore( pTexture );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Creates a new empty cube texture in memory
PRESULT dx9_CubeTextureNewEmpty ( 
	int* pnCubeTextureId,
	int nSize, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup, 
	int nType,
	D3DC_USAGE tUsagePool )
{
	ASSERT_RETINVALIDARG( pnCubeTextureId );
	*pnCubeTextureId = INVALID_ID;
	int nTempID;
	D3D_CUBETEXTURE * pCubeTexture;
	V_RETURN( dx9_CubeTextureAdd( &nTempID, &pCubeTexture, nGroup, nType ) );
	ASSERT_RETFAIL(pCubeTexture);

	V_RETURN( dx9_CubeTextureNewEmptyInPlace( nTempID, nSize, nMIPLevels, tFormat, nGroup, nType, tUsagePool ) );

	//dxC_CubeTextureAddRef( pTexture );

	*pnCubeTextureId = nTempID;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Creates a new empty texture in memory
PRESULT dx9_CubeTextureNewEmptyInPlace ( 
	int nCubeTextureId,
	int nSize, 
	int nMIPLevels,
	DXC_FORMAT tFormat, 
	int nGroup,
	int nType,
	D3DC_USAGE tUsagePool )
{
	ASSERT_RETINVALIDARG( nCubeTextureId != INVALID_ID );
	D3D_CUBETEXTURE* pCubeTexture = HashGet(gtCubeTextures, nCubeTextureId);
	ASSERT_RETFAIL(pCubeTexture);
	ASSERT_RETFAIL( tUsagePool >= 0 );

	pCubeTexture->nGroup = nGroup;
	pCubeTexture->nType = nType;

	// make a cube texture
	V_RETURN( dxC_CreateCubeTexture(
		nSize,
		nMIPLevels,
		tUsagePool,
		tFormat,
		&pCubeTexture->pD3DTexture ) );

	pCubeTexture->nSize = nSize;
	DX9_BLOCK( pCubeTexture->nD3DPool = dx9_GetPool( tUsagePool ) );
	pCubeTexture->nFormat = tFormat;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_CubeTextureNewFromFile(
	int* pnCubeTextureId,
	const char * pszFileNameWithPath,
	TEXTURE_GROUP eGroup /*= TEXTURE_GROUP_NONE*/,
	BOOL bForceSync /*= FALSE*/ )
{
	ASSERT_RETINVALIDARG( pnCubeTextureId );
	*pnCubeTextureId = INVALID_ID;
	ASSERT_RETINVALIDARG( pszFileNameWithPath );
	if ( ! pszFileNameWithPath[ 0 ] )
		return S_FALSE;

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCopy( szFilePath, pszFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
	PStrFixPathBackslash<char>( szFilePath );

	// check to see if this cubemap is already loaded
	for ( D3D_CUBETEXTURE * pCubeTex = dx9_CubeTextureGetFirst();
		pCubeTex;
		pCubeTex = dx9_CubeTextureGetNext( pCubeTex ) )
	{
		if ( 0 == PStrICmp( pCubeTex->szFilepath, szFilePath ) )
		{
			*pnCubeTextureId = pCubeTex->nId;
			return S_OK;
		}
	}

	int nTextureID = INVALID_ID;
	D3D_CUBETEXTURE * pTexture;
	V_RETURN( dx9_CubeTextureAdd( &nTextureID, &pTexture, TEXTURE_GROUP_BACKGROUND, TEXTURE_ENVMAP ) );  // heh, envmap... duh
	ASSERT_RETFAIL( pTexture && nTextureID != INVALID_ID );

	PStrCopy( pTexture->szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, szFilePath, DEFAULT_FILE_WITH_PATH_SIZE );

	TEXTURE_LOADED_CALLBACKDATA * pCallbackData = (TEXTURE_LOADED_CALLBACKDATA *) MALLOCZ( g_ScratchAllocator, sizeof( TEXTURE_LOADED_CALLBACKDATA ) );
	//pCallbackData->dwTextureFlags = dwTextureFlags;
	//pCallbackData->dwReloadFlags = dwReloadFlags;
	pCallbackData->nTextureDefId = nTextureID;			// not actually the definition ID -- cubetextures don't have defs
	//pCallbackData->nDefaultFormat = nDefaultFormat;

	const TEXTURE_TYPE_DATA * pTextureTypeData = (const TEXTURE_TYPE_DATA *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_TEXTURE_TYPES, TEXTURE_ENVMAP);
	ASSERT_RETFAIL( pTextureTypeData );
	int nPriority = pTextureTypeData->pnPriority[ eGroup ];

	DECLARE_LOAD_SPEC( spec, pszFileNameWithPath );
	spec.pool = g_ScratchAllocator;
	if (!AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) || AppGetState() == APP_STATE_FILLPAK) {
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	}
	spec.flags |= PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;
	spec.priority = nPriority;
	spec.fpLoadingThreadCallback = sCubeTextureLoadedCallback;
	spec.callbackData = pCallbackData;

	if ( bForceSync )
	{
		PakFileLoadNow(spec);
	}
	else
	{
		PakFileLoad(spec);
	}

	//dxC_CubeTextureAddRef( pTexture );

	*pnCubeTextureId = nTextureID;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3DC_IMAGE_FILE_FORMAT dxC_TextureSaveFmtToImageFileFmt( TEXTURE_SAVE_FORMAT tTSF )
{
	switch ( tTSF )
	{
	case TEXTURE_SAVE_TGA:
		return (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_TGA;
	case TEXTURE_SAVE_PNG:
		return (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_PNG;
	case TEXTURE_SAVE_DDS:
	default:
		return (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_DDS;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const OS_PATH_CHAR * dxC_TextureSaveFmtGetExtension( D3DC_IMAGE_FILE_FORMAT tIFF )
{
	switch ( tIFF )
	{
	case (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_TGA:
		return OS_PATH_TEXT("tga");
	case (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_PNG:
		return OS_PATH_TEXT("png");
	case (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_DDS:
	default:
		return OS_PATH_TEXT("dds");
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sTextureSaveCommon(
	D3D_TEXTURE * pTexture,
	DWORD dwLoadFlags,
	TEXTURE_SAVE_FORMAT eSaveFormat,
	const char * pszFileNameWithPath,
	char * pszFilePath,
	char * pszExtension,
	BOOL & bSave,
	D3DC_IMAGE_FILE_FORMAT & tFormat
	)
{
	switch ( eSaveFormat )
	{
	case TEXTURE_SAVE_TGA:
		ASSERTV_MSG( "Engine does not support saving TGA files!  Use the PNG format instead.\n\n%s", pszFileNameWithPath );
		return E_NOTIMPL;
		//PStrCopy( pszExtension, "tga", 4 );
		//break; 
	case TEXTURE_SAVE_PNG:
		PStrCopy( pszExtension, "png", 4 );
		break; 
	case TEXTURE_SAVE_DDS:
	default:
		PStrCopy( pszExtension, D3D_TEXTURE_FILE_EXTENSION, 4 );
	};

	PStrReplaceExtension( pszFilePath, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath, pszExtension );

	if ( FileIsReadOnly( pszFilePath ) )
	{
		if ( 0 == ( dwLoadFlags & TEXTURE_LOAD_NO_ERROR_ON_SAVE ) )
			bSave = DataFileCheck( pszFilePath );
		else
			bSave = FALSE;
	} 

	switch ( eSaveFormat )
	{
	case TEXTURE_SAVE_TGA:
		ASSERT_MSG( "Cannot save TGA format files!" );
		return E_NOTIMPL;
		//tFormat = (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_TGA;
		//break;
	case TEXTURE_SAVE_PNG:
		tFormat = (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_PNG;
		break;
	case TEXTURE_SAVE_DDS:
	default:
		tFormat = D3DXIFF_DDS;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_TextureSave (
	int nTextureID,
	const char * pszFileNameWithPath,
	DWORD dwLoadFlags,
	TEXTURE_SAVE_FORMAT eSaveFormat,
	BOOL bUpdatePak /*= FALSE*/ )
{
	ASSERT_RETINVALIDARG( pszFileNameWithPath );
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	ASSERT_RETINVALIDARG( pTexture );

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char szExtension[ 4 ];
	BOOL bSave = TRUE;
	D3DC_IMAGE_FILE_FORMAT tFormat = D3DXIFF_DDS;

	V_RETURN( sTextureSaveCommon( pTexture, dwLoadFlags, eSaveFormat, pszFileNameWithPath, szFilePath, szExtension, bSave, tFormat ) );

	if ( bSave )
	{
		V_RETURN( sSaveTexture( pTexture->pD3DTexture, szFilePath, tFormat, bUpdatePak ) );
		return S_OK;
	}

	return S_FALSE;
}

PRESULT e_TextureSaveRect (
	int nTextureID,
	OS_PATH_CHAR * poszFileNameWithPath,
	E_RECT * pRect,
	TEXTURE_SAVE_FORMAT eSaveFormat /*= TEXTURE_SAVE_DDS*/,
	BOOL bUpdatePak /*= FALSE*/ )
{
	ASSERT_RETINVALIDARG( poszFileNameWithPath );
	char szFileNameWithPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCvt( szFileNameWithPath, poszFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	ASSERT_RETINVALIDARG( pTexture );

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char szExtension[ 4 ];
	BOOL bSave = TRUE;
	D3DC_IMAGE_FILE_FORMAT tFormat = D3DXIFF_DDS;

	V_RETURN( sTextureSaveCommon( pTexture, 0, eSaveFormat, szFileNameWithPath, szFilePath, szExtension, bSave, tFormat ) );

	FileGetFullFileName( szFileNameWithPath, szFilePath, DEFAULT_FILE_WITH_PATH_SIZE );

	if ( bSave )
	{
#ifdef ENGINE_TARGET_DX9
		ASSERT_RETFAIL( pTexture->pD3DTexture );
		SPDIRECT3DSURFACE9 pSurf;
		V_RETURN( pTexture->pD3DTexture->GetSurfaceLevel( 0, &pSurf ) );
		return D3DXSaveSurfaceToFile( szFileNameWithPath, tFormat, pSurf, NULL, (const RECT*)pRect );
#else
		ASSERT_MSG( "Saving a texture subrect not available on DX10!" );
		return E_NOTIMPL;
#endif
	}

	return S_FALSE;
}


#ifdef ENGINE_TARGET_DX10
static inline PRESULT sConvertTextureToSaveableFormat( LPD3DCTEXTURE2D pSrcTexture, LPD3DCTEXTURE2D* ppSaveTexture )
{
	ASSERT_RETINVALIDARG( pSrcTexture && ppSaveTexture );

	D3DC_TEXTURE2D_DESC texDesc;
	V_RETURN( dxC_Get2DTextureDesc( pSrcTexture, 0, &texDesc ) );

	DXC_FORMAT saveFmt = dx10_ResourceFormatToSRVFormat( texDesc.Format );
	bool bMSAA = texDesc.SampleDesc.Count > 1;

	if( saveFmt == texDesc.Format && !bMSAA ) 
		(*ppSaveTexture) = pSrcTexture;
	else //We can't save in this format because it's typeless or using MSAA
	{
		SPD3DCTEXTURE2D pTempResolve = NULL;
		//Create a temporary staging buffer to convert formats
		texDesc.Format = saveFmt;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
		texDesc.Usage = D3D10_USAGE_STAGING;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		V_RETURN( dx10_CreateTexture2D( &texDesc, NULL, ppSaveTexture ) );
		ASSERT_RETFAIL( (*ppSaveTexture) );

		if( bMSAA )
		{
			texDesc.Usage = D3D10_USAGE_DEFAULT;
			texDesc.CPUAccessFlags = 0;
			texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;

			V_RETURN( dx10_CreateTexture2D( &texDesc, NULL, &pTempResolve ) );
			ASSERT_RETFAIL( (pTempResolve) );

			dxC_GetDevice()->ResolveSubresource( pTempResolve, 0, pSrcTexture, 0, texDesc.Format );
			dxC_GetDevice()->CopyResource( (*ppSaveTexture), pTempResolve );
		}
		else
			dxC_GetDevice()->CopyResource( (*ppSaveTexture), pSrcTexture );
	}
	return S_OK;
}
#endif

static inline D3DC_IMAGE_FILE_FORMAT sWarnOnTGA( D3DC_IMAGE_FILE_FORMAT Format )
{
	if( Format == D3DXIFF_TGA )
	{
		ShowDataWarning( "Saving textures to TGA is unsupported in DX10. Please don't do it." );
		DX10_BLOCK( return D3DX10_IFF_DDS; )
	}
	return Format;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SaveTextureToMemory(LPD3DCBASETEXTURE pSrcTexture, D3DC_IMAGE_FILE_FORMAT DestFormat, LPD3DCBLOB * ppDestBuf)
{
	DestFormat = sWarnOnTGA( DestFormat );
	DX9_BLOCK( return D3DXSaveTextureToFileInMemory( ppDestBuf, DestFormat, pSrcTexture, NULL); )
	
#ifdef ENGINE_TARGET_DX10
	LPD3DCTEXTURE2D pSaveTexture = NULL;
	V_RETURN( sConvertTextureToSaveableFormat( (LPD3DCTEXTURE2D)pSrcTexture, &pSaveTexture ) );
	return D3DX10SaveTextureToMemory( pSaveTexture, DestFormat, ppDestBuf, NULL );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SaveTextureToFile(
	LPCTSTR szFileName,
	D3DC_IMAGE_FILE_FORMAT fFmt,
	LPD3DCTEXTURE2D pTex,
	BOOL bUpdatePak /*= FALSE*/ )
{
	ASSERTX( ! (fFmt == D3DXIFF_TGA && bUpdatePak), "We can not save TGAs to the pakfile.  File will be saved to disk" );

	fFmt = sWarnOnTGA( fFmt );

	if( bUpdatePak )
	{
		SPD3DCBLOB pBuf;
		V_RETURN( dxC_SaveTextureToMemory( pTex, fFmt, &pBuf ) );

		FileSave( szFileName, pBuf->GetBufferPointer(), pBuf->GetBufferSize() );

		UINT64 gentime = FileGetLastModifiedTime( szFileName );

		DECLARE_LOAD_SPEC( spec, szFileName );
		spec.pool = g_ScratchAllocator;
		spec.buffer = (void *)pBuf->GetBufferPointer();
		spec.size = pBuf->GetBufferSize();
		spec.gentime = gentime;
		PakFileAddFile( spec );
	}
	else
	{
#if defined(ENGINE_TARGET_DX10)
	    SPD3DCTEXTURE2D pSaveTexture = NULL;
	    V_RETURN( sConvertTextureToSaveableFormat( pTex, &pSaveTexture ) );
	    V_RETURN( D3DX10SaveTextureToFile( pSaveTexture, fFmt, szFileName ) );
#else
	    if ( fFmt == D3DXIFF_TGA )
		{
			V_RETURN( sSaveTargaFile( pTex, szFileName ) );
		}
	    else
		{
			V_RETURN( D3DXSaveTextureToFile( szFileName, fFmt, pTex, NULL ) );
		}
#endif
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ReloadAllTextures( BOOL bCompressAndSave )
{
	DWORD dwFlags = bCompressAndSave ? 0 : TEXTURE_LOAD_SOURCE_ONLY;
	dwFlags |= TEXTURE_LOAD_NO_ERROR_ON_SAVE;

	// iterate all active textures
	for (D3D_TEXTURE* pTex = HashGetFirst(gtTextures); pTex; pTex = HashGetNext(gtTextures, pTex))
	{
		if ( pTex->nGroup == TEXTURE_GROUP_UI )
			continue;
		if ( pTex->nDefinitionId == INVALID_ID )
			continue;
		if ( pTex->nGroup == TEXTURE_GROUP_WARDROBE )
			continue;
		if ( pTex->nGroup == INVALID_ID )
			continue;
		if ( pTex->nType == INVALID_ID )
			continue;

		TEXTURE_DEFINITION * pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTex->nDefinitionId );
		if ( pDef )
		{
			V_CONTINUE( dx9_TextureReload( pTex, pDef->tHeader.pszName, 0, D3DFMT_FROM_FILE, dwFlags ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_TextureReload( int nTexture )
{
	D3D_TEXTURE* pTex = dxC_TextureGet( nTexture );

	if ( !pTex )
		return S_FALSE;
	if ( pTex->nGroup == TEXTURE_GROUP_UI )
		return S_FALSE;
	if ( pTex->nDefinitionId == INVALID_ID )
		return S_FALSE;
	if ( pTex->nGroup == TEXTURE_GROUP_WARDROBE )
		return S_FALSE;
	if ( pTex->nGroup == INVALID_ID )
		return S_FALSE;
	if ( pTex->nType == INVALID_ID )
		return S_FALSE;

	TEXTURE_DEFINITION* pDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTex->nDefinitionId );
	if ( pDef )
	{		
		int nNewWidth = pTex->nWidthInPixels;
		int nNewHeight = pTex->nHeight;

		e_TextureBudgetAdjustWidthAndHeight( (TEXTURE_GROUP)pTex->nGroup, 
			(TEXTURE_TYPE)pTex->nType, nNewWidth, nNewHeight );

		if ( nNewWidth != pTex->nWidthInPixels && nNewHeight != pTex->nHeight )
		{
			V_RETURN( dx9_TextureReload( pTex, pDef->tHeader.pszName, 0, D3DFMT_FROM_FILE, 0 ) );
		}
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_TextureRemove(
	int nTextureId)
{
	D3D_TEXTURE* pTexture = HashGet(gtTextures, nTextureId);
	if (!pTexture)
	{
		return S_FALSE;
	}


	// test for context/resource
	//dxC_ResourceDestroy( pTexture->hResource );


	if ( pTexture->pbLocalTextureFile )
		FREE( g_ScratchAllocator, pTexture->pbLocalTextureFile );
	if ( pTexture->pbLocalTextureData )
		FREE( g_ScratchAllocator, pTexture->pbLocalTextureData );
	if ( pTexture->ptSysmemTexture )
	{
		pTexture->ptSysmemTexture->Free();
		FREE( g_ScratchAllocator, pTexture->ptSysmemTexture );
	}
	if ( pTexture->pLoadedCallbackData )
		FREE( g_ScratchAllocator, pTexture->pLoadedCallbackData );

	if ( pTexture->pChunkData )
	{
		if ( pTexture->pChunkData->pnChunkMap )
			FREE( NULL, pTexture->pChunkData->pnChunkMap );

		FREE(NULL, pTexture->pChunkData);
	}

	if( pTexture->pArrayCreationData )
		FREE( g_ScratchAllocator, pTexture->pArrayCreationData );

	pTexture->pD3DTexture = NULL;
	DX10_BLOCK( pTexture->pD3D10ShaderResourceView = NULL; )

	if ( pTexture->tEngineRes.IsTracked() )
		gtReaper.tTextures.Remove( &pTexture->tEngineRes );
	HashRemove(gtTextures, nTextureId);

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_CubeTextureRemove(
	int nTextureId)
{
	D3D_CUBETEXTURE* pTexture = HashGet(gtCubeTextures, nTextureId);
	if (!pTexture)
	{
		return S_FALSE;
	}

	pTexture->pD3DTexture = NULL;
	DX10_BLOCK( pTexture->pD3D10ShaderResourceView = NULL; )

	if ( pTexture->pbLocalTextureFile )
		FREE( g_ScratchAllocator, pTexture->pbLocalTextureFile );
	if ( pTexture->pbLocalTextureData )
		FREE( g_ScratchAllocator, pTexture->pbLocalTextureData );
	if ( pTexture->ptSysmemTexture )
	{
		pTexture->ptSysmemTexture->Free();
		FREE( g_ScratchAllocator, pTexture->ptSysmemTexture );
	}

	HashRemove(gtCubeTextures, nTextureId);

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_TextureRestore(
	int id)
{
	//if ( e_TextureIsLoaded( id, TRUE ) )
	//	return S_FALSE;

	D3D_TEXTURE * pTexture = dxC_TextureGet( id );
	ASSERT_RETINVALIDARG( pTexture );
	V_RETURN( sCreateTextureFromMemory( pTexture ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_VerifyValidTextureSet( const LPD3DCBASETEXTURE pTexture )
{
#ifdef EXTRA_SET_TEXTURE_VALIDATION
	if ( ! pTexture )
		return S_FALSE;
	if ( pTexture->GetType() == D3DRTYPE_TEXTURE )
	{
		LPD3DCTEXTURE2D pTex = (LPD3DCTEXTURE2D)pTexture;
		D3DC_TEXTURE2D_DESC tDesc;
		V( pTex->GetLevelDesc( 0, &tDesc ) );
		ASSERT( tDesc.Pool == D3DPOOL_DEFAULT || tDesc.Pool == D3DPOOL_MANAGED );
	}
#endif // EXTRA_SET_TEXTURE_VALIDATION
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dxC_TextureAdd(
	int nTextureId,
	D3D_TEXTURE** ppTexture,
	int nGroup,
	int nType )
{
	ASSERT_RETINVALIDARG( ppTexture );
	if ( nTextureId == INVALID_ID )
	{
		nTextureId = gnTextureIdNext;
		gnTextureIdNext++;
	} else {
		ASSERT_RETINVALIDARG( nTextureId < MAX_TEXTURE_DEFINITIONS );
	}
	ASSERT_RETINVALIDARG( ! HashGet( gtTextures, nTextureId ) );
	*ppTexture = HashAdd(gtTextures, nTextureId);
	ASSERT_RETFAIL(*ppTexture);
	(*ppTexture)->nDefinitionId = INVALID_ID;
	(*ppTexture)->tUsage = D3DC_USAGE_2DTEX;
	(*ppTexture)->nGroup = nGroup;
	(*ppTexture)->nType = nType;

	(*ppTexture)->tEngineRes.nId = nTextureId;
	(*ppTexture)->tEngineRes.fQuality = gtTextureBudget.fQualityBias;
	(*ppTexture)->tEngineRes.dwFlagBits = 0;
	gtReaper.tTextures.Add( &(*ppTexture)->tEngineRes, 1 );

	if ( gnBreakOnTextureID == nTextureId )
		DebugBreak();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dx9_CubeTextureAdd(
	int * pnId,
	D3D_CUBETEXTURE ** ppCubeTexture,
	int nGroup,
	int nType )
{
	ASSERT_RETINVALIDARG( ppCubeTexture );
	int nTextureId = gnCubeTextureIdNext;
	gnCubeTextureIdNext++;
	*ppCubeTexture = HashAdd(gtCubeTextures, nTextureId);
	ASSERT_RETFAIL(*ppCubeTexture);
	(*ppCubeTexture)->nD3DPool = INVALID_ID;
	(*ppCubeTexture)->nGroup = nGroup;
	(*ppCubeTexture)->nType = nType;
	(*ppCubeTexture)->pD3DTexture = NULL;
	DX10_BLOCK( (*ppCubeTexture)->pD3D10ShaderResourceView = NULL; )
	(*ppCubeTexture)->nFormat = ENVMAP_LOAD_FORMAT;
	(*ppCubeTexture)->tEngineRes.dwFlagBits = 0;

	if ( pnId )
		*pnId = (*ppCubeTexture)->nId;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TexturesInit()
{
	HashInit(gtTextures, NULL, 512);
	HashInit(gtCubeTextures, NULL, 8);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TexturesDestroy()
{
	V( e_TexturesRemoveAll() );
	HashFree( gtTextures );
	HashFree( gtCubeTextures );
	sCleanupTextureReport();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TexturesRemoveAll( DWORD dwOnlyGroup )
{
	// free all textures
	D3D_TEXTURE* pTexture = dxC_TextureGetFirst();
	BOUNDED_WHILE( pTexture, 1000000 )
	{
		D3D_TEXTURE* pNextTexture = dxC_TextureGetNext( pTexture );
		ASSERT_BREAK( pNextTexture != pTexture );
		if ( dwOnlyGroup == TEXTURE_GROUP_NONE || pTexture->nGroup == dwOnlyGroup )
		{
			V( e_TextureRemove( pTexture->nId ) );
		}
		pTexture = pNextTexture;
	}

	// free all cube textures
	D3D_CUBETEXTURE* pCubeTexture = dx9_CubeTextureGetFirst();
	BOUNDED_WHILE( pCubeTexture, 1000000 )
	{
		D3D_CUBETEXTURE* pNextCubeTexture = dx9_CubeTextureGetNext( pCubeTexture );
		ASSERT_BREAK( pNextCubeTexture != pCubeTexture );
		if ( dwOnlyGroup == TEXTURE_GROUP_NONE || pCubeTexture->nGroup == dwOnlyGroup )
		{
			V( e_CubeTextureRemove( pCubeTexture->nId ) );
		}
		pCubeTexture = pNextCubeTexture;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_TextureIsValidID( int nTextureID )
{
	if ( nTextureID < 0 )
		return FALSE;
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	return BOOL(pTexture != NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureGetOriginalResolution( int nTextureID, int & nWidth, int & nHeight )
{
	nWidth = 0;
	nHeight = 0;
	if ( nTextureID != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );

		if ( pTexture && pTexture->nDefinitionId != INVALID_ID )
		{
			TEXTURE_DEFINITION* pDef = (TEXTURE_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
			if ( pDef )
			{
				nWidth  = pDef->nWidth;
				nHeight = pDef->nHeight;
			}
		}
	}
	else
	{
		return S_FALSE;
	}

	if ( nWidth == 0 || nHeight == 0 )
		return e_TextureGetLoadedResolution( nTextureID, nWidth, nHeight );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureGetLoadedResolution( int nTextureID, int & nWidth, int & nHeight )
{
	nWidth = 0;
	nHeight = 0;
	if ( nTextureID != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
		if ( pTexture )
		{
			nWidth  = pTexture->nWidthInPixels;
			nHeight = pTexture->nHeight;
			return S_OK;
		}
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_TextureGetDefinition( int nTextureID )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	if ( pTexture )
		return pTexture->nDefinitionId;
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_FontInit( int * pnTextureID, int nLetterCountWidth, int nLetterCountHeight, int nGDISize, BOOL bBold, BOOL bItalic, const char * pszSystemName, const char * pszLocalPath, float * pfFontWidth, float fTabWidth )
{
	ASSERT_RETINVALIDARG( pnTextureID );
	*pnTextureID = INVALID_ID;

	// create device independent bitmap (DIB)
	BITMAPINFOHEADER tBitMapInfo;
	structclear(tBitMapInfo);

	//int width = font->nSizeInTexture;
	//int height = font->nSizeInTexture;
	int width = FONT_BITMAP_SIZE;
	int height = FONT_BITMAP_SIZE;

	int nBitmapWidth = width * nLetterCountWidth;
	int nBitmapHeight = height * nLetterCountHeight;

	ASSERTX_RETFAIL((nBitmapWidth/width) * (nBitmapHeight/height) >= 128 - 32, "Error initializing font");

	BITMAPINFOHEADER *ptHeader = &tBitMapInfo;
	ptHeader->biSize = sizeof(BITMAPINFOHEADER);
	ptHeader->biWidth = nBitmapWidth;
	ptHeader->biHeight = -nBitmapHeight;
	ptHeader->biPlanes = 1;
	ptHeader->biBitCount = 16;
	ptHeader->biCompression = BI_RGB;

	HDC hDC = CreateCompatibleDC(NULL);
	ASSERTX_RETFAIL(hDC, "Error initializing font");

	WORD *pwFontBuffer;
	HBITMAP hBitMap = CreateDIBSection(hDC, (tagBITMAPINFO *)&tBitMapInfo, DIB_RGB_COLORS, (void **)&pwFontBuffer, NULL, 0);
	ASSERTX_RETFAIL(hBitMap, "Error initializing font");

	// load a font to work with
	int nWeight = bBold ? FW_BOLD : FW_NORMAL;
	//HFONT hGDIFont = CreateFont(font->nGDISize, 0, 0, 0, nWeight, font->bItalic, FALSE, FALSE,
	//TCHAR fullname[MAX_PATH];
	BOOL  bLoadedFont = FALSE;
	// look for local font
	//if ( AppIsTugboat() )
	//{
	//	if( PStrLen( pszLocalPath ) != 0 )
	//	{
	//		FileGetFullFileName(fullname, pszLocalPath, MAX_PATH);

	//		DECLARE_LOAD_SPEC(spec, fullname);
	//		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	//		spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	//		void * pBits = PakFileLoadNow( spec );

	//		HANDLE hFont = AddFontMemResourceEx( fullname, FR_PRIVATE, 0 );
	//		if( hFont != 0 )
	//		{
	//			bLoadedFont = TRUE;
	//			// I don't think we actually need to send this
	//			//SendMessage( HWND_BROADCAST, WM_FONTCHANGE, 0, 0 );
	//		}
	//	}
	//}

	HFONT hGDIFont = CreateFont(FONT_SET_SIZE, 0, 0, 0, nWeight, bItalic, FALSE, FALSE,
		ANSI_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH, pszSystemName);
	ASSERTX_RETFAIL(hGDIFont, "Error initializing font");

	SelectObject(hDC, hBitMap);
	SelectObject(hDC, hGDIFont);
	SetTextColor(hDC, 0x00ffffff);
	SetBkColor(hDC, 0);
	SetBkMode(hDC, TRANSPARENT);

	char szTemp[2] = { 0, 0 };
	int y = 0;
	for (int chr = ' ', x = 0, y = 0; chr < 128; chr++)
	{
		RECT rcOut;
		SetRect(&rcOut, x, y, x + width, y + width);
		szTemp[0] = (char)chr;

		int ret = DrawText(hDC, szTemp, 1, &rcOut, DT_LEFT | DT_NOCLIP);
		ASSERTX_RETFAIL(ret, "Error initializing font");

		//int nSize;
		//GetCharWidth32(hDC, chr, chr, &nSize);
		//font->pfFontWidth[chr - ' '] = (float)nSize;

		x += width;
		if (x >= nBitmapHeight)		//should this be width?
		{
			x = 0;
			y += height;
		}
	}

	// Create the texture
	D3D_TEXTURE * texture;
	V_RETURN( dxC_TextureAdd( INVALID_ID, &texture, TEXTURE_GROUP_UI, TEXTURE_DIFFUSE ) );
//	strcpy(texture->pszFilename, font->pszName);
	texture->nWidthInPixels = nBitmapWidth;
	texture->nFormat = DX9_BLOCK( D3DFMT_A4R4G4B4 ) DX10_BLOCK( D3DFMT_A8R8G8B8 );  //KMNV TODO VERIFY think we're ok with this substitution
	texture->nWidthInBytes = dxC_GetTexturePitch( nBitmapWidth, (DXC_FORMAT)texture->nFormat );
	//texture->nWidthInBytes = 2 * nBitmapWidth;
	texture->nHeight = nBitmapHeight;

	// make the alpha channel in a buffer
	//texture->nTextureFileSize = texture->nWidthInBytes * texture->nHeight;
	texture->nTextureFileSize = dx9_GetTextureMIPLevelSizeInBytes( texture );
	texture->pbLocalTextureData = (BYTE *)MALLOC(g_ScratchAllocator, texture->nTextureFileSize);
	WORD * pwSrc = pwFontBuffer;
	WORD * pwDest = (WORD *)texture->pbLocalTextureData;
	for (int dy = 0; dy < nBitmapHeight; dy++)
	{
		for (int dx = 0; dx < nBitmapWidth; dx++)
		{
			WORD wData = *pwSrc++;
			if ( wData )
			{
				*pwDest++ = wData;
				//int nAlias = ( ( wData & 0xf00 ) >> 8 ) + ( ( wData & 0xf0 ) >> 4 ) + ( wData & 0xf );
				//nAlias = ( ( nAlias * 15 ) / 45 ) << 12;
				//*pwDest++ = nAlias | ( wData & 0xfff );
			}
			else
			{
				*pwDest++ = 0;
			}
		}
	}

	// restore to create the D3D texture
	V_RETURN( e_TextureRestore( texture->nId ) );

	// set size table
	DeleteObject( hGDIFont );
	hGDIFont = CreateFont(nGDISize, 0, 0, 0, nWeight, bItalic, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY, FIXED_PITCH, pszSystemName);
	ASSERTX_RETFAIL(hGDIFont, "Error initializing font");

	SelectObject(hDC, hGDIFont);

	float fTotalWidth = 0.0f;
	for (int chr = ' '; chr < 128; chr++)
	{
		int nSize;
		GetCharWidth32(hDC, chr, chr, &nSize);
		pfFontWidth[chr - ' '] = (float)nSize;
		fTotalWidth += pfFontWidth[chr - ' '];
	}
	float fAvgWidth = fTotalWidth / 128.0f;
	fTabWidth = TAB_CHAR_COUNT * fAvgWidth;

	// release the DC and its parts (BMP freed by geometry list)
	DeleteObject(hGDIFont);
	DeleteObject(hBitMap);
	ReleaseDC(NULL, hDC);

	// remove the local font
	//if ( bLoadedFont )
	//{
	//	RemoveFontMemResourceEx( fullname );
	//	SendMessage( HWND_BROADCAST, WM_FONTCHANGE, 0, 0 );
	//}
	*pnTextureID = texture->nId;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_CubeTextureIsLoaded (
	int nCubeTextureId,
	BOOL bAllowFinalize )
{
	D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( nCubeTextureId );
	return pTexture && pTexture->pD3DTexture != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_TextureIsLoaded (
	int nTextureId,
	BOOL bAllowFinalize )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );

	if ( pTexture && ( pTexture->pD3DTexture == NULL && ! ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture ) ) && bAllowFinalize )
	{
		// finish the load
		TEXTURE_DEFINITION * pTexDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		BOOL bLoaded = ( S_OK == dx9_TextureCreateIfReady( pTexture, pTexDef ) );
		return bLoaded;
	}

	return pTexture && ( pTexture->pD3DTexture != NULL || ( (pTexture->dwFlags & TEXTURE_FLAG_SYSMEM) && pTexture->ptSysmemTexture != NULL) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_CubeTextureLoadFailed (
	int nCubeTextureId )
{
	D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( nCubeTextureId );
	if ( ! pTexture )
		return FALSE;
	return pTexture->dwFlags & _TEXTURE_FLAG_LOAD_FAILED;
}

BOOL e_TextureLoadFailed (
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return FALSE;
	return pTexture->dwFlags & _TEXTURE_FLAG_LOAD_FAILED;
}

void e_TextureSetLoadFailureWarned (
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return;
	SET_MASK( pTexture->dwFlags, _TEXTURE_FLAG_LOAD_FAILURE_WARNED );
}

BOOL e_TextureLoadFailureWarned (
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return TRUE;
	return TEST_MASK( pTexture->dwFlags, _TEXTURE_FLAG_LOAD_FAILURE_WARNED );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void WINAPI sFillTextureCallback(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
	D3DXVECTOR4 vColor = *(D3DXVECTOR4*)pData;
	*pOut = vColor;
}

static PRESULT sFillTexture( SYSMEM_TEXTURE * pSysmemTexture, D3DXVECTOR4 vColor )
{
#ifdef ENGINE_TARGET_DX9
	D3DLOCKED_RECT tRect;
	int nLevels = pSysmemTexture->GetLevelCount();
	DXC_FORMAT tFormat = pSysmemTexture->GetFormat();

	for ( int i = 0; i < nLevels; ++i )
	{
		V_CONTINUE( dxC_MapSystemMemTexture( pSysmemTexture, i, &tRect ) );

		ASSERT_DO( NULL != tRect.pBits )
		{
			dxC_UnmapSystemMemTexture( pSysmemTexture, i );
			continue;
		}

		if ( tFormat == D3DFMT_A8R8G8B8 )
		{
			DWORD dwPixelBitDepth = dx9_GetTextureFormatBitDepth( tFormat );
			ASSERT_RETFAIL( dwPixelBitDepth == 32 );
			int nPixels = pSysmemTexture->GetLevelSize( i ) * 8 / dwPixelBitDepth;
			unsigned int dwColor = ARGB_MAKE_FROM_FLOAT(vColor.x, vColor.y, vColor.z, vColor.w);
			unsigned int * pBits = (unsigned int *)tRect.pBits;
			for ( int xy = 0; xy < nPixels; ++xy )
			{
				pBits[ xy ] = dwColor;
			}
		}
		else
		{
			return E_NOTIMPL;
		}

		dxC_UnmapSystemMemTexture( pSysmemTexture, i );
	}
	//return dxC_FillTexture( pTexture, sFillTextureCallback, &vColor );
	return S_OK;
#else
	return E_NOTIMPL;
#endif
}

static PRESULT sFillTexture( SPD3DCTEXTURE2D pTexture, D3DXVECTOR4 vColor )
{
	return dxC_FillTexture( pTexture, sFillTextureCallback, &vColor );
}

static PRESULT sFillTexture( int nTextureID, DWORD dwColor )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	ASSERT_RETINVALIDARG( pTexture );

	D3DXVECTOR4 vColor;
	UCHAR * pColor = (UCHAR*)&dwColor;
	float fInv255 = 1.f / 255.f;
	vColor.z = pColor[ 0 ] * fInv255; // blue
	vColor.y = pColor[ 1 ] * fInv255; // green
	vColor.x = pColor[ 2 ] * fInv255; // red
	vColor.w = pColor[ 3 ] * fInv255; // alpha

	if ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM )
	{
		ASSERT_RETFAIL( pTexture->ptSysmemTexture );
		V_RETURN( sFillTexture( pTexture->ptSysmemTexture, vColor ) );
	}
	else
	{
		ASSERT_RETFAIL( pTexture->pD3DTexture );
		V_RETURN( sFillTexture( pTexture->pD3DTexture, vColor ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void WINAPI sColorCubeFill (
	D3DXVECTOR4* pOut,
	const D3DXVECTOR3* pTexCoord, 					   
	const D3DXVECTOR3* pTexelSize,
	LPVOID pData )
{
	D3DXVECTOR4 vColor = *(D3DXVECTOR4*)pData;
	*pOut = vColor;
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------

PRESULT e_RestoreDefaultTextures()
{
	const int			nDim		= 2;
	const DXC_FORMAT		tFormat		= D3DFMT_A8R8G8B8;
	extern UTILITY_TEXTURE gnUtilityTextures[];

	for ( int i = 0; i < NUM_UTILITY_TEXTURES; i++ )
	{
		if ( gnUtilityTextures[ i ].nTextureID != INVALID_ID )
			e_TextureReleaseRef( gnUtilityTextures[ i ].nTextureID );
        switch(i)
        {
		case 0: // passthru to get around a warning in ENGINE_TARGET_DX9
        default:
			V( dx9_TextureNewEmpty(
				&gnUtilityTextures[ i ].nTextureID,
				nDim,
				nDim,
				1,
				tFormat,
				INVALID_ID,
				INVALID_ID  ) );
			V( sFillTexture( gnUtilityTextures[ i ].nTextureID, gnUtilityTextures[ i ].dwColor ) );
        }

		e_TextureAddRef( gnUtilityTextures[ i ].nTextureID );
	}

	for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
	{
		switch (i)
		{
		case TEXTURE_DIFFUSE:
		case TEXTURE_DIFFUSE2:
			gnDefaultTextureIDs[ i ]		= e_GetUtilityTexture( TEXTURE_RGB_7F_A_FF );
			gnDebugNeutralTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGB_7F_A_FF );
			break;
		case TEXTURE_NORMAL:
			// DXT5 normal maps should be 7F RGBA
			gnDefaultTextureIDs[ i ]		= e_GetUtilityTexture( TEXTURE_RGBA_7F );
			gnDebugNeutralTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGBA_7F );
			break;
		case TEXTURE_SPECULAR:
			gnDefaultTextureIDs[ i ]		= e_GetUtilityTexture( TEXTURE_RGBA_00 );
			gnDebugNeutralTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGBA_7F );
			break;
		case TEXTURE_LIGHTMAP:
			if ( AppIsTugboat() )
				gnDefaultTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGB_7F_A_FF );
			else
				gnDefaultTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGBA_00 );
			gnDebugNeutralTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGBA_FF );
			break;
		case TEXTURE_ENVMAP:
		case TEXTURE_SELFILLUM:
			gnDefaultTextureIDs[ i ]		= e_GetUtilityTexture( TEXTURE_RGBA_00 );
			gnDebugNeutralTextureIDs[ i ]	= e_GetUtilityTexture( TEXTURE_RGB_00_A_FF );
			break;
		default:
			ErrorDialog( "Someone forgot to fill in the default texture for texture type %d!", i );
		}
	}

	if ( ! dxC_IsPixomaticActive() )
	{
		// create utility default cubemap
		int nTextureID;
		D3D_CUBETEXTURE * pCubeTex;
		V_RETURN( dx9_CubeTextureAdd( &nTextureID, &pCubeTex, INVALID_ID, TEXTURE_ENVMAP ) );  // heh, envmap... duh
		ASSERT_RETFAIL( pCubeTex && nTextureID != INVALID_ID );
		V_RETURN( dxC_CreateCubeTexture(
			nDim,
			1,
			D3DC_USAGE_CUBETEX_DYNAMIC,
			tFormat,
			&pCubeTex->pD3DTexture ) );
		ASSERT_RETFAIL( pCubeTex->pD3DTexture );

		DWORD dwColor = 0x00000000;
		D3DXVECTOR4 vColor;
		UCHAR * pColor = (UCHAR*)&dwColor;
		float fInv255 = 1.f / 255.f;
		vColor.z = pColor[ 0 ] * fInv255; // blue
		vColor.y = pColor[ 1 ] * fInv255; // green
		vColor.x = pColor[ 2 ] * fInv255; // red
		vColor.w = pColor[ 3 ] * fInv255; // alpha

		DX9_BLOCK( V_RETURN( D3DXFillCubeTexture ( pCubeTex->pD3DTexture, sColorCubeFill, &vColor ) ); )
		DX10_BLOCK( sFillTexture(pCubeTex->pD3DTexture, vColor); )
		gnDefaultCubeTextureID = nTextureID;
		e_CubeTextureAddRef( nTextureID );
	}

#ifdef _DEBUG
	if ( ! dxC_IsPixomaticActive() )
	{
		for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
		{
			int nID = e_GetDefaultTexture( (TEXTURE_TYPE)i );
			ASSERT( INVALID_ID != nID );
			if ( i == TEXTURE_ENVMAP )
			{
				D3D_CUBETEXTURE * pTex = dx9_CubeTextureGet( nID );
				ASSERT( pTex && pTex->pD3DTexture );
				V( dx9_VerifyValidTextureSet( pTex->pD3DTexture ) );
			} else
			{
				D3D_TEXTURE * pTex = dxC_TextureGet( nID );
				ASSERT( pTex && pTex->pD3DTexture );
				V( dx9_VerifyValidTextureSet( pTex->pD3DTexture ) );
			}

		}
	}
#endif // _DEBUG

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_TextureAddRef( 
	int nTextureID )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	if ( pTexture )
		dxC_TextureAddRef( pTexture );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_TextureReleaseRef( 
	int nTextureID,
	BOOL bCleanupNow /*= FALSE*/ )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	if ( ! pTexture  )
		return;

	dxC_TextureReleaseRef( pTexture );

	if ( bCleanupNow && pTexture->tRefCount.GetCount() <= 0 )
	{
		V( e_TextureRemove( nTextureID ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_TextureGetRefCount( int nTexture )
{
	D3D_TEXTURE* pTexture = dxC_TextureGet( nTexture );
	if ( !pTexture )
		return 0;
	
	return pTexture->tRefCount.GetCount();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_CubeTextureAddRef( int nCubeTextureID )
{
	D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGet( nCubeTextureID );
	if ( pCubeTexture )
		dxC_CubeTextureAddRef( pCubeTexture );
}

void e_CubeTextureReleaseRef( int nCubeTextureID )
{
	D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGet( nCubeTextureID );
	if ( ! pCubeTexture )
		return;
	dxC_CubeTextureReleaseRef( pCubeTexture );
	//if ( pTexture->tRefCount.GetCount() <= 0 )
	//	e_CubeTextureRemove( nTextureID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureGetSize( int nTextureID, int& nWidthOut, int& nHeightOut )
{
	D3D_TEXTURE* pTexture = dxC_TextureGet( nTextureID );
	if ( !pTexture )
	{
		nWidthOut = -1;
		nHeightOut = -1;
		return S_FALSE;
	}

	nWidthOut = pTexture->nWidthInPixels;
	nHeightOut = pTexture->nHeight;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_TextureGetSizeInMemory( 
	const D3D_TEXTURE * pTexture,
	DWORD * pdwBytes )
{
	ASSERT_RETINVALIDARG( pdwBytes );
	*pdwBytes = 0;

	if ( pTexture )
	{
		if ( pTexture->pD3DTexture )
		{
			//*pdwBytes = static_cast<DWORD>(pTexture->nD3DTopMIPSize);
			UINT nLevels = 0;
			V_DO_SUCCEEDED( dxC_Get2DTextureLevels( pTexture->pD3DTexture, nLevels ) )
			{
				for ( UINT i = 0; i < nLevels; ++i )
				{
					*pdwBytes += (DWORD)dx9_GetTextureMIPLevelSizeInBytes( pTexture, i );
				}
			}
		}

		ASSERTV( ! ( pTexture->pbLocalTextureData && pTexture->pbLocalTextureFile ), "Both local data and local file memory buffers exist, but we only have one size!\n\nTexture ID: %d", pTexture->nId );
		if ( pTexture->pbLocalTextureData || pTexture->pbLocalTextureFile )
		{
			*pdwBytes += pTexture->nTextureFileSize;
		}
		if ( pTexture->ptSysmemTexture )
		{
			*pdwBytes += pTexture->ptSysmemTexture->GetSize();
		}

		return S_OK;
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureGetSizeInMemory( 
	int nTextureID,
	DWORD * pdwBytes )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	return dxC_TextureGetSizeInMemory( pTexture, pdwBytes );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CubeTextureGetSizeInMemory( 
	int nCubeTextureID,
	DWORD * pdwBytes )
{
	ASSERT_RETINVALIDARG( pdwBytes );
	D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGet( nCubeTextureID );

	*pdwBytes = 0;
	if ( pCubeTexture )
	{
		*pdwBytes = dx9_GetCubeTextureMIPLevelSizeInBytes( pCubeTexture );
		return S_OK;
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureDumpList( TEXTURE_GROUP eGroup /*= TEXTURE_GROUP_NONE*/, TEXTURE_TYPE eType /*= TEXTURE_NONE*/ )
{
#if ISVERSION(DEVELOPMENT)
#ifdef ENGINE_TARGET_DX9
	trace( "\n--- Texture List:\n" );
	trace( "-- 2D Textures:\n" );
	trace( " ID    | D3D | WID  HGHT LV | REFS | TYPE      | GROUP      | FORMAT      | SIZE      | UPLD | FILE\n" );

	const int cnFormatLen = 32;
	char szFormat[ cnFormatLen ];

	for ( D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
		pTexture;
		pTexture = dxC_TextureGetNext( pTexture ) )
	{
		int nRef = pTexture->tRefCount.GetCount();
		D3DC_TEXTURE2D_DESC tDesc;
		ZeroMemory( &tDesc, sizeof(tDesc) );
		int nLevels = 0;
		if ( pTexture->pD3DTexture )
		{
			V( pTexture->pD3DTexture->GetLevelDesc( 0, &tDesc ) );
			nLevels = pTexture->pD3DTexture->GetLevelCount();
		}

		DWORD dwBytes = 0;
		V( e_TextureGetSizeInMemory( 
			pTexture->nId,
			&dwBytes ) );

		dxC_TextureFormatGetDisplayStr( (DXC_FORMAT)pTexture->nFormat, szFormat, cnFormatLen );
		// strip the "D3DFMT_" off of the front
		char * pszFormat = &szFormat[7];

		char szUpld[8] = "n/a";
		if ( TESTBIT_DWORD( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED ) )
		{
			if ( TESTBIT_DWORD( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
				PStrCopy( szUpld, "yes", 8 );
			else
				PStrCopy( szUpld, "no", 8 );
		}

		trace( " %-5d | %-3s | %4d %4d %2d | %-4d | %-9s | %-10s | %-11s | %-9d | %-4s |",
			pTexture->nId,
			pTexture->pD3DTexture ? "yes" : "no",
			tDesc.Width,
			tDesc.Height,
			nLevels,
			nRef,
			pTexture->nType  >= 0 ? gszTextureTypes [pTexture->nType]  : "",
			pTexture->nGroup >= 0 ? gszTextureGroups[pTexture->nGroup] : "",
			pszFormat,
			dwBytes,
			szUpld );
		if ( pTexture->nDefinitionId != INVALID_ID )
		{
			TEXTURE_DEFINITION * pDefinition = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
			if ( pDefinition )
				trace( " [ %s ]", pDefinition->tHeader.pszName );
		}
		trace( "\n" );
	}

	trace( "-- Cube Textures:\n" );

	for ( D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGetFirst();
		pCubeTexture;
		pCubeTexture = dx9_CubeTextureGetNext( pCubeTexture ) )
	{
		int nRef = pCubeTexture->tRefCount.GetCount();
	}

	trace( "---\n" );
#endif
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dxC_TextureCopy(
	int nDestTexture,
	SPD3DCTEXTURE2D pD3DSource,
	int nStartingMip )
{
	D3D_TEXTURE * pDestTexture = dxC_TextureGet( nDestTexture );
	if ( ! pDestTexture )
		return S_FALSE;
	if ( ! pDestTexture->pD3DTexture )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pD3DSource );

	int nLevelCount = dxC_GetNumMipLevels( pDestTexture->pD3DTexture );

	ASSERT_RETFAIL( nLevelCount == dxC_GetNumMipLevels( pD3DSource ) );
	ASSERT_RETFAIL( pDestTexture->nHeight );

	ASSERT( dx9_IsCompressedTextureFormat( (DXC_FORMAT) pDestTexture->nFormat ) );
	int nHeight = pDestTexture->nHeight;
	nHeight >>= nStartingMip;

	D3DLOCKED_RECT tDestLocked;	
	D3DLOCKED_RECT tSourceLocked;	

	for ( int nLevel = nStartingMip; nLevel < nLevelCount; nLevel++ )
	{
		V_GOTO( cont, dxC_MapTexture( pDestTexture->pD3DTexture, nLevel, &tDestLocked ) );
		V_GOTO( cont, dxC_MapTexture( pD3DSource, nLevel, &tSourceLocked, true ) );

		BYTE * pbDestStart   = (BYTE *) dxC_pMappedRectData( tDestLocked );
		BYTE * pbSourceStart = (BYTE *) dxC_pMappedRectData( tSourceLocked );

		ASSERT( dxC_nMappedRectPitch( tDestLocked ) );
		ASSERT( dxC_nMappedRectPitch( tDestLocked ) == dxC_nMappedRectPitch( tSourceLocked ) );
		int nSize = dxC_nMappedRectPitch( tDestLocked ) * nHeight / 4;
		ASSERT( nSize );
		if ( nSize )
			memcpy( pbDestStart, pbSourceStart, nSize );

cont:
		V( dxC_UnmapTexture( pDestTexture->pD3DTexture, nLevel ) );
		V( dxC_UnmapTexture( pD3DSource, nLevel ) );

		nHeight /= 2;
	}

	DX9_BLOCK( V( pDestTexture->pD3DTexture->AddDirtyRect(NULL) ); )

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFilterTexture ( 
	JOB_DATA & tJobData )
{
	//DebugString( OP_DEBUG, "Job Filter   [%08x] Execute", tJobData.data1 );

	TEXTURE_FILTER_DATA * pFilterData = (TEXTURE_FILTER_DATA *) tJobData.data1;

	if ( dx9_IsCompressedTextureFormat( (DXC_FORMAT) pFilterData->tDestFormat ) )
	{
		V( dx9_TextureLevelDecompress ( pFilterData->dwWidth, pFilterData->dwHeight, 
			pFilterData->tDestFormat, pFilterData->pbDestLevels[ 0 ], // we are pushing from dest to source so that the call to dx9_TextureFilter has what it needs
			pFilterData->dwWidth, pFilterData->dwHeight, pFilterData->tSrcFormat, pFilterData->pbSrc ) );
	} 

	V( dx9_TextureFilter( pFilterData, pFilterData->nLevelCount, D3DX_FILTER_BOX ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFilterCleanup ( 
	JOB_DATA & tJobData )
{
	//DebugString( OP_DEBUG, "Job Filter   [%08x] Cleanup", tJobData.data1 );
	TEXTURE_FILTER_DATA * pFilterData = (TEXTURE_FILTER_DATA *) tJobData.data1;


	D3D_TEXTURE * pTexture = dxC_TextureGet( pFilterData->nTextureId );
	if ( pTexture )
		pTexture->hFilterJob = NULL;

	if ( pFilterData->pbSrc )
		FREE( g_ScratchAllocator, pFilterData->pbSrc );
	if ( pFilterData->pbDestLevels )
	{
		for ( int i = 0; i < pFilterData->nLevelCount; i++ )
		{
			FREE( g_ScratchAllocator, pFilterData->pbDestLevels[ i ] );
		}
		FREE( g_ScratchAllocator, pFilterData->pbDestLevels );
	}

	FREE( g_ScratchAllocator, pFilterData );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFilterComplete ( 
	JOB_DATA & tJobData )
{
	//DebugString( OP_DEBUG, "Job Filter   [%08x] Complete", tJobData.data1 );

	TEXTURE_FILTER_DATA * pFilterData = (TEXTURE_FILTER_DATA *) tJobData.data1;

	D3D_TEXTURE * pDestTexture = dxC_TextureGet( pFilterData->nTextureId );
	if ( ! pDestTexture )
		return;
	if ( ! pDestTexture->pD3DTexture )
		return;

	int nLevelCount = dxC_GetNumMipLevels( pDestTexture->pD3DTexture );

	ASSERT_RETURN( nLevelCount == pFilterData->nLevelCount );
	ASSERT_RETURN( pDestTexture->nHeight );

	BOOL bIsCompressed = dx9_IsCompressedTextureFormat( (DXC_FORMAT) pDestTexture->nFormat );
	int nStartingMip = 0;
	int nHeight = pDestTexture->nHeight;
	nHeight >>= nStartingMip;

	for ( int nLevel = nStartingMip; nLevel < nLevelCount; nLevel++ )
	{
		D3DLOCKED_RECT tDestLocked;	
		V_GOTO( nxt, dxC_MapTexture( pDestTexture->pD3DTexture, nLevel, &tDestLocked ) );

		BYTE * pbDestStart   = (BYTE *) dxC_pMappedRectData( tDestLocked );

		ASSERT( dxC_nMappedRectPitch( tDestLocked ) );
		int nSize = dxC_nMappedRectPitch( tDestLocked ) * nHeight;
		if ( bIsCompressed )
			nSize /= 4;
		ASSERT( nSize );
		if ( nSize )
			memcpy( pbDestStart, pFilterData->pbDestLevels[ nLevel ], nSize );

nxt:
		V( dxC_UnmapTexture( pDestTexture->pD3DTexture, nLevel ) );

		nHeight /= 2;
	}

	DX9_BLOCK( V( pDestTexture->pD3DTexture->AddDirtyRect(NULL) ); )

	sFilterCleanup( tJobData );
}

//----------------------------------------------------------------------------
// D3DXFilterTexture is slow.  Call this to have it done on another thread
//----------------------------------------------------------------------------
#ifdef DXTLIB

#define THREAD_TEXTURE_FILTERING
PRESULT e_TextureFilterRequest ( 
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return S_FALSE;
	if ( ! pTexture->pD3DTexture )
		return S_FALSE;

	if ( pTexture->hFilterJob )
	{
		c_JobCancel( pTexture->hFilterJob );
		pTexture->hFilterJob = NULL;
 	}

	TEXTURE_FILTER_DATA * pData = (TEXTURE_FILTER_DATA *) MALLOCZ( g_ScratchAllocator, sizeof( TEXTURE_FILTER_DATA ) );

	pData->nTextureId = nTextureId;
	pData->nLevelCount =  dxC_GetNumMipLevels( pTexture->pD3DTexture );
	pData->tDestFormat = (DXC_FORMAT)pTexture->nFormat;
	int nSrcBytesPerBlock = 0;
	switch ( pData->tDestFormat )
	{
	case D3DFMT_A8R8G8B8:
	case D3DFMT_DXT5:
		pData->tSrcFormat = D3DFMT_A8R8G8B8;
		nSrcBytesPerBlock = 4;
		break;
	case D3DFMT_DXT1:
	DX9_BLOCK( case D3DFMT_R8G8B8: )
		pData->tSrcFormat = D3DFMT_R8G8B8;
		nSrcBytesPerBlock = 3;
		break;
	default:
		ASSERTXONCE_RETVAL( 0, E_FAIL, "unsupported texture format for filtering"); 
		break;
	}
	pData->dwHeight = pTexture->nHeight;
	pData->dwWidth = pTexture->nWidthInPixels;

	int nSrcSize = pData->dwWidth * pData->dwHeight * nSrcBytesPerBlock;
	pData->pbSrc = (BYTE *) MALLOC( g_ScratchAllocator, nSrcSize );
	ASSERT_RETVAL ( pData->pbSrc, E_OUTOFMEMORY );

	// copy the top mip level to the scratch texture
	BOOL bCompressed = dx9_IsCompressedTextureFormat( (DXC_FORMAT) pTexture->nFormat );
	int nSize;
	D3DLOCKED_RECT tTextureLocked;	
	V_RETURN( dxC_MapTexture( pTexture->pD3DTexture, 0, &tTextureLocked, true ) );
	{
		BYTE * pbTextureStart  = (BYTE *) dxC_pMappedRectData( tTextureLocked );

		nSize = dxC_nMappedRectPitch( tTextureLocked ) * pTexture->nHeight;

		if ( bCompressed )
			nSize /= 4;

		pData->pbDestLevels = (BYTE **) MALLOC( g_ScratchAllocator, sizeof( BYTE * ) * pData->nLevelCount );

		pData->pbDestLevels[ 0 ] = (BYTE *) MALLOC( g_ScratchAllocator, sizeof( BYTE ) * nSize );

		ASSERT( nSize );
		if ( nSize )
			memcpy( pData->pbDestLevels[ 0 ], pbTextureStart, nSize );
	}
	V( dxC_UnmapTexture( pTexture->pD3DTexture, 0 ) );

	if ( ! bCompressed )
		memcpy( pData->pbSrc, pData->pbDestLevels[ 0 ], nSrcSize );

	int nMinLevelSize = 0;
	if ( bCompressed )
		nMinLevelSize = (pData->tDestFormat == D3DFMT_DXT1) ? cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;
	int nSizeCurr = nSize;
	for ( int i = 1; i < pData->nLevelCount; i++ )
	{
		nSizeCurr /= 4;
		nSizeCurr = max( nSizeCurr, nMinLevelSize );
		ASSERT( nSizeCurr > 0 );
		pData->pbDestLevels[ i ] = (BYTE *) MALLOC( g_ScratchAllocator, sizeof( BYTE ) * nSizeCurr );
	}


	// submit the job to be done on another thread
#ifdef THREAD_TEXTURE_FILTERING
	pTexture->hFilterJob = c_JobSubmit( JOB_TYPE_BIG, sFilterTexture, JOB_DATA( (DWORD_PTR) pData ), sFilterComplete, sFilterCleanup );
#else
	JOB_DATA tJobData( (DWORD_PTR) pData );
	sFilterTexture( tJobData );
	sFilterComplete( tJobData );
#endif

	return S_OK;
}

#endif // DXTLIB

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_TextureIsFiltering( 
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return FALSE;

	return pTexture->hFilterJob != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_TextureIsColorCombining( 
	int nTextureId )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
	if ( ! pTexture )
		return FALSE;

	return pTexture->dwFlags & TEXTURE_FLAG_WARDROBE_COLOR_COMBINING;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static NV_ERROR_CODE sNVDXTCopySurface( const void* pData, size_t count, const MIPMapData* pMipMapData, void* pUserData )
// callback to save the MIP level of a texture being compressed/mipped into a byte *
{	
	ASSERT( pUserData );
	BYTE** pbLevels = (BYTE**)pUserData;
	memcpy( pbLevels[ pMipMapData ? pMipMapData->mipLevel : 0 ], pData, count );
	return NV_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static nvMipFilterTypes sNVDXTGetFilterType( DWORD dwD3DXFilter )
{
	switch ( dwD3DXFilter )
	{
	case D3DX_FILTER_POINT		: return kMipFilterPoint;
	case D3DX_FILTER_LINEAR		: return kMipFilterQuadratic;
	case D3DX_FILTER_TRIANGLE	: return kMipFilterTriangle;
	case D3DX_FILTER_BOX		: return kMipFilterBox;
	case D3DX_FILTER_NONE		:
	default						: return kMipFilterPoint;
	}
}

static nvTextureFormats sGetDestNVFormat ( DXC_FORMAT dxcFormat, bool bNormalMap = false )
{
	switch ( dxcFormat )
	{
	case D3DFMT_DXT3:		return kDXT3; break;
	case D3DFMT_DXT5:		
		if ( bNormalMap )
			return kDXT5_NM; 
		else
			return kDXT5;
		break;
	case D3DFMT_A8R8G8B8:	return k8888; break;
	DX9_BLOCK ( case D3DFMT_R8G8B8:		return k888; break; ) //No 3 component 8-bit DX10 textures
	case D3DFMT_A1R5G5B5:	return k1555; break;
	case D3DFMT_R5G6B5:		return k565; break;
	case D3DFMT_A8:			return kA8; break;
	case D3DFMT_L8:			return kL8; break;
	case D3DFMT_DXT1:
	default:				
		// if the format read in had alpha
		if ( dx9_TextureFormatHasAlpha( dxcFormat ) )
			return kDXT1a;
		else
			return kDXT1;
	}
}

#ifdef DXTLIB 

PRESULT dxC_CompressToGPUTexture( SPD3DCTEXTURE2D pSrcTexture, SPD3DCTEXTURE2D pDestTexture )
 {
	D3DC_TEXTURE2D_DESC destDesc;
	V_RETURN( dxC_Get2DTextureDesc( pDestTexture, 0, &destDesc ) );

	D3DLOCKED_RECT tLRect;
	V_RETURN( dxC_MapTexture( pSrcTexture, 0, &tLRect) );

	// set these based on options in TEXTURE_DEFINITION
	nvCompressionOptions tCompressOptions;
	tCompressOptions.mipFilterType = kMipFilterPoint;
	tCompressOptions.SetFilterSharpness( 1.0f );
	tCompressOptions.sharpenFilterType	  = kSharpenFilterNone;

	for ( int i = 0; i < dxC_GetNumMipLevels( pDestTexture ); i++ )
		tCompressOptions.sharpening_passes_per_mip_level[ i ] = 1;

	tCompressOptions.bDitherMip0 = false;
	tCompressOptions.bDitherColor = false;
		

	tCompressOptions.SetQuality(kQualityFastest, FASTEST_COMPRESSION_THRESHOLD);
	
	// checking above to force pow2, so can safely use 0 ("all")
	if( dxC_GetNumMipLevels( pDestTexture ) == 1 ) // just the base texture.
		tCompressOptions.DoNotGenerateMIPMaps();
	else
		tCompressOptions.GenerateMIPMaps( dxC_GetNumMipLevels( pDestTexture ) );

	tCompressOptions.textureFormat = sGetDestNVFormat( destDesc.Format );
	// non-input options:
	tCompressOptions.textureType = kTextureTypeTexture2D;
	TEXCONVERT_USER_DATA tUserData;
	tUserData.pTextureData = pDestTexture;
	tUserData.pFileData = INVALID_FILE;
	tCompressOptions.user_data = &tUserData;

	int nPlanes = dx9_TextureFormatHasAlpha( destDesc.Format ) ? 4 : 3;

	nvPixelOrder nPixelOrder = (nPlanes == 4) ? nvBGRA : nvBGR;
	return nvDDS::nvDXTcompress( (const unsigned char*)dxC_pMappedRectData( tLRect ),
		destDesc.Width, destDesc.Height, dxC_nMappedRectPitch( tLRect ), nPixelOrder,	
		&tCompressOptions, dx9_LoadAllMipSurfaces );
}

//----------------------------------------------------------------------------
// Generates MIP levels for a given top level (surface)
// Optionally compresses
// Takes an array of pointers to each level, and expects the top level (index 0) to already be filled in
// DOES NOT CALL OR USE ANY DIRECTX FUNCTIONS/INTERFACES
//----------------------------------------------------------------------------
PRESULT dx9_TextureFilter(
	struct TEXTURE_FILTER_DATA * ptFilterData,
	DWORD dwMIPLevels,
	DWORD dwD3DXFilter )
{
	ASSERT_RETINVALIDARG( ptFilterData );

	DWORD		dwWidth			= ptFilterData->dwWidth;
	DWORD		dwHeight		= ptFilterData->dwHeight;
	DXC_FORMAT	tSrcFormat		= ptFilterData->tSrcFormat;
	BYTE *&		pbSrc			= ptFilterData->pbSrc;
	DXC_FORMAT	tDestFormat		= ptFilterData->tDestFormat;
	
	ASSERT_RETINVALIDARG( pbSrc );
	ASSERT_RETINVALIDARG( ptFilterData->pbDestLevels );
	ASSERTX_RETINVALIDARG( ! dx9_IsCompressedTextureFormat( tSrcFormat ), "Input texture data must be uncompressed!" );
	ASSERT_RETINVALID( IsPower2( dwWidth ) );
	ASSERT_RETINVALID( IsPower2( dwHeight ) );
	ASSERT_RETINVALID( IsMultiple4( dwWidth ) );
	ASSERT_RETINVALID( IsMultiple4( dwHeight ) );

	//int nSourceSize = pTexture->nTextureFileSize;

	int nSourceDepth = dx9_GetTextureFormatBitDepth( tSrcFormat );
	ASSERTX_RETFAIL( nSourceDepth == 32 || nSourceDepth == 24, "Input texture data must be in 888 or 8888 format!" );

	// if the format read in had alpha
	BOOL bSourceAlpha = dx9_TextureFormatHasAlpha( tSrcFormat );

	nvCompressionOptions tCompressOptions;  // has constructor that defaults values
	tCompressOptions.GenerateMIPMaps(dwMIPLevels);
	tCompressOptions.mipFilterType		= sNVDXTGetFilterType( dwD3DXFilter );
	tCompressOptions.SetQuality(kQualityFastest, FASTEST_COMPRESSION_THRESHOLD); // QuickCompress
	tCompressOptions.rescaleImageType	= kRescaleNone;
	tCompressOptions.textureType		= kTextureTypeTexture2D;

	TEXCONVERT_USER_DATA tUserData;
	tUserData.pTextureData = ptFilterData->pbDestLevels;
	tUserData.pFileData = INVALID_FILE;
	tCompressOptions.user_data			= &tUserData;

	switch ( tDestFormat )
	{
	case D3DFMT_A8R8G8B8:	tCompressOptions.textureFormat = k8888; break;
	case D3DFMT_X8R8G8B8:	tCompressOptions.textureFormat = k8888; break;
	DX9_BLOCK ( case D3DFMT_R8G8B8:		tCompressOptions.textureFormat = k888; break; ) //No 3 component 8-bit DX10 textures
	case D3DFMT_A1R5G5B5:	tCompressOptions.textureFormat = k1555; break;
	case D3DFMT_R5G6B5:		tCompressOptions.textureFormat = k565; break;
	case D3DFMT_A8:			tCompressOptions.textureFormat = kA8; break;
	case D3DFMT_L8:			tCompressOptions.textureFormat = kL8; break;
	DX9_BLOCK( case D3DFMT_DXT2: )
	case D3DFMT_DXT3:		tCompressOptions.textureFormat = kDXT3; break;
	DX9_BLOCK( case D3DFMT_DXT4: )
	case D3DFMT_DXT5:		tCompressOptions.textureFormat = kDXT5; break;
	case D3DFMT_DXT1:
	default:				
		// if the format read in had alpha
		if ( bSourceAlpha )
			tCompressOptions.textureFormat = kDXT1a;
		else
			tCompressOptions.textureFormat = kDXT1;
	}

	int nPlanes = bSourceAlpha ? 4 : 3;
	int nSrcPitch = dxC_GetTexturePitch( dwWidth, tSrcFormat );

	nvPixelOrder nPixelOrder = (nPlanes == 4) ? nvBGRA : nvBGR;
	V_RETURN( nvDDS::nvDXTcompress(
		pbSrc,
		dwWidth,
		dwHeight,
		nSrcPitch,
		nPixelOrder,
		&tCompressOptions,
		sNVDXTCopySurface ) );
	return S_OK;
}

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
	BOOL bDestHasMips )
{
	ASSERT_RETINVALIDARG( pbSrc );
	ASSERT_RETINVALIDARG( pbDest );
	ASSERT_RETINVALIDARG( ! dx9_IsCompressedTextureFormat( tSrcFormat ) );
	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( tDestFormat ) );
//	ASSERT_RETURN( IsPower2( dwWidth ) );
//	ASSERT_RETURN( IsPower2( dwHeight ) );
//	ASSERT_RETURN( IsMultiple4( dwWidth ) );
//	ASSERT_RETURN( IsMultiple4( dwHeight ) );

	//int nSourceSize = pTexture->nTextureFileSize;

	int nSourceDepth = dx9_GetTextureFormatBitDepth( tSrcFormat );
	ASSERT_RETFAIL( nSourceDepth == 24 || nSourceDepth == 32 );

	// if the format read in had alpha
	BOOL bSourceAlpha = dx9_TextureFormatHasAlpha( tSrcFormat );

	nvCompressionOptions tCompressOptions;  // has constructor that defaults values
	tCompressOptions.SetQuality(kQualityFastest, FASTEST_COMPRESSION_THRESHOLD ); // QuickCompress
	if ( bDestHasMips )
		tCompressOptions.UseExisitingMIPMaps();
	else
		tCompressOptions.DoNotGenerateMIPMaps();
	tCompressOptions.rescaleImageType	= kRescaleNone;
	tCompressOptions.textureType		= kTextureTypeTexture2D;

	TEXCONVERT_USER_DATA tUserData;
	tUserData.pTextureData = pbDest;
	tUserData.pFileData = INVALID_FILE;
	tCompressOptions.user_data			= &tUserData;

	switch ( tDestFormat )
	{
	DX9_BLOCK( case D3DFMT_DXT2: )
	case D3DFMT_DXT3:		tCompressOptions.textureFormat = kDXT3; break;
	DX9_BLOCK( case D3DFMT_DXT4: )
	case D3DFMT_DXT5:		tCompressOptions.textureFormat = kDXT5; break;
	case D3DFMT_DXT1:
	default:				
		// if the format read in had alpha
		if ( bSourceAlpha )
			tCompressOptions.textureFormat = kDXT1a;
		else
			tCompressOptions.textureFormat = kDXT1;
	}

	int nPlanes = bSourceAlpha ? 4 : 3;
	int nSrcPitch = dxC_GetTexturePitch( dwWidth, tSrcFormat );

	nvPixelOrder nPixelOrder = (nPlanes == 4) ? nvBGRA : nvBGR;
	V_RETURN( nvDDS::nvDXTcompress(
		pbSrc,
		dwWidth,
		dwHeight,
		nSrcPitch,
		nPixelOrder,
		&tCompressOptions,
		sNVDXTCopySurface) );
	return S_OK;
}

#endif // DXTLIB

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sDecompressBlocks(
	DWORD dwSrcWidth,
	DWORD dwSrcHeight,
	DXC_FORMAT tSrcFormat,
	const BYTE * pbSrcStart,
	DWORD dwDestWidth,
	DWORD dwDestHeight,
	DXC_FORMAT tDestFormat,
	BYTE * pbDestStart,
	int nSrcRowSize,				// represents 4 pixel rows (one block row)
	int nSrcBytesTotalPerBlock,
	int nDestRowSize,				// represents 1 pixel row
	int nDestBytesWidePerBlock )
{
	// This loop processes 4 rows per iteration.
	for ( DWORD nY = 0; nY < dwSrcHeight; nY += 4 )
	{
		const BYTE * pbSrcCurr			= pbSrcStart;
		BYTE *		 pbDestCurr[ 4 ]	= { pbDestStart, pbDestStart + nDestRowSize, pbDestStart + (2 * nDestRowSize), pbDestStart + (3 * nDestRowSize) };

		// This loop processes 4 columns per iteration.
		for( DWORD nX = 0; nX < dwSrcWidth; nX += 4 )
		{
			switch ( tSrcFormat )
			{
			case D3DFMT_DXT1:
				if ( tDestFormat == D3DFMT_R8G8B8 )
					dxC_ConvertDXT1ToRGB( pbDestCurr, pbSrcCurr, TRUE );
				else
					dxC_ConvertDXT1ToRGBA( pbDestCurr, pbSrcCurr, FALSE );
				break;
			case D3DFMT_DXT3:
				dxC_ConvertDXT3ToRGBA( pbDestCurr, pbSrcCurr );
				break;
			default:
				dxC_ConvertDXT5ToRGBA( pbDestCurr, pbSrcCurr );
			}

			pbDestCurr[ 0 ] += nDestBytesWidePerBlock;
			pbDestCurr[ 1 ] += nDestBytesWidePerBlock;
			pbDestCurr[ 2 ] += nDestBytesWidePerBlock;
			pbDestCurr[ 3 ] += nDestBytesWidePerBlock;
			pbSrcCurr += nSrcBytesTotalPerBlock;
		}
		pbSrcStart  += nSrcRowSize;
		pbDestStart += 4 * nDestRowSize;
	}
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Handles decompressing 4x4 compressed blocks to 2x2, 2x4, 1x1, etc. destination blocks
static PRESULT sDecompressPartialSingleBlock(
	DWORD dwSrcWidth,
	DWORD dwSrcHeight,
	DXC_FORMAT tSrcFormat,
	const BYTE * pbSrcBlock,
	DWORD dwDestWidth,
	DWORD dwDestHeight,
	DXC_FORMAT tDestFormat,
	BYTE * pbDestBlock,
	int nSrcRowSize,				// represents 4 pixel rows (one block row)
	int nSrcBytesTotalPerBlock )
{
	ASSERT_RETINVALIDARG( dwSrcWidth  == 4 );
	ASSERT_RETINVALIDARG( dwSrcHeight == 4 );
	ASSERT_RETINVALIDARG( dwDestWidth < 4 || dwDestHeight < 4 );

	ASSERTX_RETVAL( tDestFormat == D3DFMT_A8R8G8B8, E_NOTIMPL, "Unsupported texture format for partial block decompression" );

	// Decompress to a dummy block and copy the appropriate values into the real dest block.

	const int cnBlockWidth  = 4;
	const int cnBlockHeight = 4;
	// Assumes ARGB8
	typedef DWORD  DEST_PIXEL;
	DEST_PIXEL pDummyDestBlock[ cnBlockWidth * cnBlockHeight ];
	int nDestBytesWidePerBlock = 16; // for ARGB8
	int nDummyDestRowSize = nDestBytesWidePerBlock * cnBlockWidth / 4;	// represents 1 pixel row

	V_RETURN( sDecompressBlocks(
		dwSrcWidth,
		dwSrcHeight,
		tSrcFormat,
		pbSrcBlock,
		cnBlockWidth,
		cnBlockHeight,
		tDestFormat,
		(BYTE*)pDummyDestBlock,
		nSrcRowSize,
		nSrcBytesTotalPerBlock,
		nDummyDestRowSize,
		nDestBytesWidePerBlock ) );

	int nDestRowSize = nDestBytesWidePerBlock * dwDestWidth / 4;
	BYTE * pDummyStart = (BYTE*)pDummyDestBlock;
	BYTE * pDestStart = pbDestBlock;

	int nDestPixelSize = dx9_GetTextureFormatBitDepth( tDestFormat ) / 8;

	// Copy the appropriate values from the dummy block into our real dest block, starting at the top-left.
	// In theory, it'd be better to sub-sample or even interpolate to get our values.
	for ( DWORD y = 0; y < dwDestHeight; ++y )
	{
		size_t nSizeToCopy = nDestPixelSize * dwDestWidth;
		MemoryCopy( pDestStart, nSizeToCopy, pDummyStart, nSizeToCopy );

		pDummyStart += nDummyDestRowSize;
		pDestStart  += nDestRowSize;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
// DXT compresses a single texture level (surface)
// DOES NOT CALL OR USE ANY DIRECTX FUNCTIONS/INTERFACES
//----------------------------------------------------------------------------
PRESULT dx9_TextureLevelDecompress (
	DWORD dwSrcWidth,
	DWORD dwSrcHeight,
	DXC_FORMAT tSrcFormat,
	const BYTE * pbSrc,
	DWORD dwDestWidth,
	DWORD dwDestHeight,
	DXC_FORMAT tDestFormat,
	BYTE * pbDest )
{
	ASSERT_RETINVALIDARG( pbSrc );
	ASSERT_RETINVALIDARG( pbDest );
	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( tSrcFormat ) );
	ASSERT_RETINVALIDARG( !dx9_IsCompressedTextureFormat( tDestFormat ) );
	ASSERT_RETINVALIDARG( IsPower2( dwSrcWidth ) );
	ASSERT_RETINVALIDARG( IsPower2( dwSrcHeight ) );
	// Lower levels of the source (DXT) texture are still minimum 4x4.
	ASSERT_RETINVALIDARG( IsMultiple4( dwSrcWidth ) );
	ASSERT_RETINVALIDARG( IsMultiple4( dwSrcHeight ) );

	// If the source width/height is 4, the dest must be 4, 2, or 1.
	ASSERT_RETINVALIDARG( dwSrcWidth  > 4 || ( dwDestWidth  == 4 || dwDestWidth  == 2 || dwDestWidth  == 1 ) );
	ASSERT_RETINVALIDARG( dwSrcHeight > 4 || ( dwDestHeight == 4 || dwDestHeight == 2 || dwDestHeight == 1 ) );
	// Otherwise, the source and dest width/height have to match.
	ASSERT_RETINVALIDARG( dwSrcWidth  == 4 || dwSrcWidth  == dwDestWidth  );
	ASSERT_RETINVALIDARG( dwSrcHeight == 4 || dwSrcHeight == dwDestHeight );

	int nDestDepth = dx9_GetTextureFormatBitDepth( tDestFormat );
	ASSERT_RETFAIL( nDestDepth == 24 || nDestDepth == 32 );

	// Width of the dest block in bytes
	int nDestBytesWidePerBlock = 0;
	// Total width*height of the src block in bytes
	int nSrcBytesTotalPerBlock  = 0;


	switch ( tSrcFormat )
	{
	case D3DFMT_DXT5:		
	case D3DFMT_DXT3:
		ASSERTXONCE_RETVAL( tDestFormat == D3DFMT_A8R8G8B8, E_NOTIMPL, "unsupported texture format for decompression"); 
		nDestBytesWidePerBlock = 16;
		nSrcBytesTotalPerBlock  = cnDXT2_5BytesPerPacket;
		break;

	case D3DFMT_DXT1:
		ASSERTXONCE_RETVAL( tDestFormat == D3DFMT_R8G8B8 || tDestFormat == D3DFMT_A8R8G8B8, E_NOTIMPL, "unsupported texture format for decompression"); 
		if ( tDestFormat == D3DFMT_R8G8B8 )
			nDestBytesWidePerBlock = 12;
		else
			nDestBytesWidePerBlock = 16;
		nSrcBytesTotalPerBlock  = cnDXT1BytesPerPacket;
		break;

	default:
		ASSERTXONCE_RETVAL( 0, E_NOTIMPL, "unsupported texture format for decompression"); 
		break;
	}


	int nSrcRowSize  = nSrcBytesTotalPerBlock * dwSrcWidth / 4;
	int nDestRowSize = nDestBytesWidePerBlock * dwDestWidth / 4;


	// If the destination is less than a 4x4 block in width or height, call a special function.
	if ( dwDestWidth < 4 || dwDestHeight < 4 )
	{
		const BYTE * pbSrcStart = pbSrc;
		BYTE * pbDestStart		= pbDest;

		// This loop processes 4 rows per iteration.
		for ( DWORD y = 0; y < dwDestHeight; y += 4 )
		{
			const BYTE * pbSrcCurr			= pbSrcStart;
			BYTE *		 pbDestCurr			= pbDestStart;

			// This loop processes 4 columns per iteration.
			for ( DWORD x = 0; x < dwDestWidth; x += 4 )
			{
				V_RETURN( sDecompressPartialSingleBlock(
					4,
					4,
					tSrcFormat,
					pbSrcCurr,
					dwDestWidth,
					dwDestHeight,
					tDestFormat,
					pbDestCurr,
					nSrcRowSize,
					nSrcBytesTotalPerBlock ) );

				pbDestCurr += nDestBytesWidePerBlock * 4;
				pbSrcCurr += nSrcBytesTotalPerBlock;
			}

			pbSrcStart  += nSrcRowSize;
			pbDestStart += 4 * nDestRowSize;
		}
	}
	else
	{
		V_RETURN( sDecompressBlocks(
			dwSrcWidth,
			dwSrcHeight,
			tSrcFormat,
			pbSrc,
			dwDestWidth,
			dwDestHeight,
			tDestFormat,
			pbDest,
			nSrcRowSize,
			nSrcBytesTotalPerBlock,
			nDestRowSize,
			nDestBytesWidePerBlock ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TexturesCleanup(
	TEXTURE_GROUP eOnlyGroup /*= TEXTURE_GROUP_NONE*/,
	TEXTURE_TYPE eOnlyType /*= TEXTURE_NONE*/ )
{
	// prune zero-ref 2D textures
	D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
	while( pTexture )
	{
		D3D_TEXTURE * pNext = dxC_TextureGetNext( pTexture );
		int nRef = pTexture->tRefCount.GetCount();
		ASSERT( nRef >= 0 );
		if ( nRef <= 0 && 
			 ( eOnlyGroup == TEXTURE_GROUP_NONE || eOnlyGroup == pTexture->nGroup ) && 
			 ( eOnlyType == TEXTURE_NONE || eOnlyType == pTexture->nType ) )
		{
			V( e_TextureRemove( pTexture->nId ) );
		}
		pTexture = pNext;
	}

	// prune zero-ref cube textures
	D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGetFirst();
	while ( pCubeTexture )
	{
		D3D_CUBETEXTURE * pNext = dx9_CubeTextureGetNext( pCubeTexture );
		int nRef = pCubeTexture->tRefCount.GetCount();
		ASSERT( nRef >= 0 );
		if ( nRef <= 0 &&
			( eOnlyGroup == TEXTURE_GROUP_NONE || eOnlyGroup == pCubeTexture->nGroup ) && 
			( eOnlyType == TEXTURE_NONE || eOnlyType == pCubeTexture->nType ) )
		{
			V( e_CubeTextureRemove( pCubeTexture->nId ) );
		}
		pCubeTexture = pNext;
	}

#ifdef ENGINE_TARGET_DX10
	if ( eOnlyGroup == TEXTURE_GROUP_WARDROBE )
		dx10_CleanupCPUTextures();
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_TextureGetBits( int nTextureID, void ** ppBits, unsigned int& nSize, BOOL bAlloc )
{
	nSize = 0;

	ASSERT_RETINVALIDARG( ppBits );
	if ( bAlloc )
	{
		*ppBits = NULL;
	} else
	{
		ASSERT_RETINVALIDARG( *ppBits );
		ASSERT_RETINVALIDARG( nSize > 0 );
	}

	D3D_TEXTURE * texture = dxC_TextureGet( nTextureID );
	ASSERT_RETINVALIDARG( texture );
	BOOL bSysmem = !!(texture->dwFlags & TEXTURE_FLAG_SYSMEM);
	ASSERT_RETINVALIDARG( texture->pD3DTexture || ( bSysmem && texture->ptSysmemTexture ) );
	ASSERT_RETFAIL( texture->nWidthInBytes > 0 );
	ASSERT_RETFAIL( texture->nWidthInPixels > 0 );
	ASSERT_RETFAIL( texture->nHeight > 0 );

	ASSERTX_RETFAIL( ! dx9_IsCompressedTextureFormat( (DXC_FORMAT)texture->nFormat ), "Compressed texture formats are not supported in e_TextureGetBits()!" );
	int nBitDepth = dx9_GetTextureFormatBitDepth( (DXC_FORMAT)texture->nFormat );
	ASSERT_RETFAIL( nBitDepth >= 8 );
	int nByteDepth = nBitDepth / 8;

	if ( bAlloc )
	{
		nSize = texture->nWidthInBytes * texture->nHeight;
		*ppBits = (void*)MALLOC(g_ScratchAllocator, nSize);
		ASSERT_RETVAL( *ppBits, E_OUTOFMEMORY );
	}

	D3DLOCKED_RECT tLockedRect;
	if ( bSysmem )
	{
		V_GOTO( err, dxC_MapSystemMemTexture( texture->ptSysmemTexture, 0, &tLockedRect ) );
	}
	else
	{
		V_GOTO( err, dxC_MapTexture( texture->pD3DTexture, 0, &tLockedRect, true ) );
		ASSERT_GOTO(tLockedRect.Pitch == texture->nWidthInPixels * nByteDepth, err);
	}
	MemoryCopy( *ppBits, nSize, tLockedRect.pBits, texture->nHeight * tLockedRect.Pitch );
	if ( bSysmem )
	{
		V_GOTO( err, dxC_UnmapSystemMemTexture( texture->ptSysmemTexture, 0 ) );
	}
	else
	{
		V_GOTO( err, dxC_UnmapTexture( texture->pD3DTexture, 0 ) );
	}

	return S_OK;

err:
	if ( bSysmem )
	{
		V( dxC_UnmapSystemMemTexture( texture->ptSysmemTexture, 0 ) );
	}
	else
	{
		V( dxC_UnmapTexture( texture->pD3DTexture, 0 ) );
	}
	if ( bAlloc )
	{
		FREE( g_ScratchAllocator, *ppBits );
		*ppBits = NULL;
		nSize = 0;
	}
	return E_FAIL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_TextureShouldSet( const D3D_TEXTURE * pTexture, int & nReplacement )
{
	nReplacement = INVALID_ID;
	ASSERT_RETINVALIDARG( pTexture );
	if ( TESTBIT_DWORD( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
		return S_OK;
	if ( ! TESTBIT_DWORD( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED ) )
		return S_OK;

	if ( e_UploadManagerGet().CanUpload() )
		return S_OK;

	nReplacement = e_GetDefaultTexture( (TEXTURE_TYPE)pTexture->nType );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_TextureDidSet( D3D_TEXTURE * pTexture )
{
	ASSERT_RETINVALIDARG( pTexture );

	if ( pTexture->tEngineRes.IsTracked() )
		gtReaper.tTextures.Touch( &pTexture->tEngineRes );

	if ( TESTBIT_DWORD( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
		return S_OK;

	DWORD dwBytes;
	V( dxC_TextureGetSizeInMemory( pTexture, &dwBytes ) );
	e_UploadManagerGet().MarkUpload( dwBytes, RESOURCE_UPLOAD_MANAGER::TEXTURE );
	SETBIT( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, TRUE );
	
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_TextureInitEngineResource( D3D_TEXTURE * pTexture )
{
	BOOL bManaged;

#ifdef ENGINE_TARGET_DX9
	bManaged = !!( dx9_GetPool( pTexture->tUsage ) == D3DPOOL_MANAGED );
#else	// DX10
	// CML TODO:  I don't know how to handle upload throttling in DX10 yet.
	bManaged = FALSE;
#endif

	SETBIT( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED, bManaged );
	SETBIT( pTexture->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, FALSE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CreateTextureArray( D3D_TEXTURE* pTexture, TEXTURE_DEFINITION* pDefinition )
{
	//This is where we expand one texture request into an entire array request
	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char szPathChild[ DEFAULT_FILE_WITH_PATH_SIZE ];
	const char* szExt = PStrGetExtension( pDefinition->tHeader.pszName );
	PStrRemoveExtension( szPath, DEFAULT_FILE_WITH_PATH_SIZE, pDefinition->tHeader.pszName );

#ifdef ENGINE_TARGET_DX10
	SPD3DCTEXTURE2D pNewArrayTexture;

	//Get description
	D3DC_TEXTURE2D_DESC texDesc;
	V_RETURN( dxC_Get2DTextureDesc( pTexture, 0, &texDesc ) );

	//Set array size
	texDesc.ArraySize = pDefinition->nArraySize;

	V_RETURN( dx10_CreateTexture2D( &texDesc, NULL, &pNewArrayTexture.p ) );

	//Plaster the first layer in
	V_RETURN( dx10_UpdateTextureArray( pNewArrayTexture, pTexture->pD3DTexture, 0 ) );
	pTexture->pD3DTexture = NULL;
	DX10_BLOCK( pTexture->pD3D10ShaderResourceView = NULL; )
	pTexture->pD3DTexture = pNewArrayTexture;

	PFN_TEXTURE_POSTLOAD pfnPostLoad = (PFN_TEXTURE_POSTLOAD) dx10_ArrayIndexLoaded;
#else
	PFN_TEXTURE_POSTLOAD pfnPostLoad = NULL;
#endif

	for(int i = 1; i < pDefinition->nArraySize; ++i )
	{
		PStrPrintf( szPathChild, DEFAULT_FILE_WITH_PATH_SIZE, "%s%d%s", szPath, i, szExt );
		int nTempTexID;
		V_CONTINUE( e_TextureNewFromFile( &nTempTexID, szPathChild, pTexture->nGroup, pTexture->nType, 0, 0, NULL, NULL, pfnPostLoad ) );

#ifdef ENGINE_TARGET_DX10
		e_TextureAddRef( nTempTexID );

		D3D_TEXTURE* pTempTex = dxC_TextureGet( nTempTexID );

		pTempTex->pArrayCreationData = MALLOC( g_ScratchAllocator, sizeof( ArrayCreationData ) );
		if ( ! pTempTex->pArrayCreationData )
			return E_OUTOFMEMORY;
		((ArrayCreationData*)pTempTex->pArrayCreationData)->nIndex = i;
		((ArrayCreationData*)pTempTex->pArrayCreationData)->nParentID = pTexture->nId;
#endif
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CopyGPUTextureToGPUTexture( LPD3DCTEXTURE2D pSrc, LPD3DCTEXTURE2D pDest, UINT nLevel )
{
#if defined(ENGINE_TARGET_DX9)
	SPD3DCRENDERTARGETVIEW pSrcLevel;
	SPD3DCRENDERTARGETVIEW pDestLevel;

	V_RETURN( pSrc->GetSurfaceLevel( nLevel, &pSrcLevel ) );
	V_RETURN( pDest->GetSurfaceLevel( nLevel, &pDestLevel ) );
	V_RETURN( D3DXLoadSurfaceFromSurface( pSrcLevel, NULL, NULL, pDestLevel, NULL, NULL, D3DTEXF_NONE, 0 ) );
#elif defined(ENGINE_TARGET_DX10)
	dxC_GetDevice()->CopySubresourceRegion( pDest, nLevel, 0, 0, 0, pSrc, nLevel, NULL );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_SetMaxLOD( D3D_TEXTURE* pTexture, UINT lod )
{
	DX9_BLOCK( pTexture->pD3DTexture->SetLOD( lod ); )

#if defined(ENGINE_TARGET_DX10)
		D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;

	pTexture->pD3D10ShaderResourceView->GetDesc( &SRVDesc );
	pTexture->pD3D10ShaderResourceView.Release();

	SRVDesc.Texture2D.MostDetailedMip = lod;
	SRVDesc.Texture2D.MipLevels = -1;

	dxC_GetDevice()->CreateShaderResourceView(	pTexture->pD3DTexture, &SRVDesc, &pTexture->pD3D10ShaderResourceView );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GetImageInfoFromFileInMemory(LPCVOID pSrcData, UINT SrcDataSize, D3DXIMAGE_INFO* pSrcInfo)
{
	DX9_BLOCK( return D3DXGetImageInfoFromFileInMemory( pSrcData, SrcDataSize, pSrcInfo); )
#ifdef ENGINE_TARGET_DX10
		HRESULT hr = D3DX10GetImageInfoFromMemory( pSrcData, SrcDataSize, NULL, pSrcInfo, NULL ); //KMNV TODO THREADPUMP
	if( SUCCEEDED( hr ) )
		return hr;

	ASSERT( pSrcInfo );
	UINT width = 0;
	UINT height = 0;
	tga_get_dimensions((void*) pSrcData, SrcDataSize, &width, &height );

	if( width <= 0 || height <= 0 )
		return E_FAIL;

	pSrcInfo->ArraySize = 1;
	pSrcInfo->Height = height;
	pSrcInfo->Width =	width;
	pSrcInfo->Depth = 1;
	pSrcInfo->MipLevels = 1;
	pSrcInfo->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
	pSrcInfo->ImageFileFormat = (D3DX10_IMAGE_FILE_FORMAT)D3DXIFF_TGA;
	pSrcInfo->Format = D3DFMT_A8B8G8R8;

	return S_OK;

#endif
}
