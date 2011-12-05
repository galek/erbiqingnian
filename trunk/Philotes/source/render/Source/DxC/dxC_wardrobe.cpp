//----------------------------------------------------------------------------
// dx9_wardrobe.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "e_pch.h"
#include "e_texture.h"
#include "excel.h"
#include "dxC_state.h"
#include "dxC_texture.h"
#include "dxC_model.h"
#include "dxC_material.h"
#include "dxC_anim_model.h"
#include "dxC_wardrobe.h"
#include "dxC_target.h"
#include "definition_priv.h"
#include "pakfile.h"
#include "filepaths.h"
#include "performance.h"
#include "granny.h"
#include "e_texture_priv.h"
#include "colorset.h"
#include "jobs.h"
#include "wardrobe.h"
#include "squish.h"

#include <map>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define WARDROBE_MODEL_FILE_VERSION					(12 + ANIMATED_MODEL_FILE_VERSION)
#define MIN_REQUIRED_WARDROBE_MODEL_FILE_VERSION	(12 + MIN_REQUIRED_ANIMATED_MODEL_FILE_VERSION)
#define WARDROBE_MODEL_FILE_EXTENSION "wm"
#define WARDROBE_MODEL_FILE_MAGIC_NUMBER 0x43342abf

#ifdef FORCE_REMAKE_MODEL_BIN
#define FORCE_REMAKE_WARDROBE_MODEL_FILES
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define FORMAT_NAME_LEN 8
struct LAYER_TEXTURE_TYPE_DATA {
	DXC_FORMAT eFormat;
	char szFormatName[ FORMAT_NAME_LEN ];
	unsigned char nDefaultCanvas;
	unsigned char nDefaultLayer;
	int nTextureType;
};

static const LAYER_TEXTURE_TYPE_DATA * gpTextureData = NULL;

static const LAYER_TEXTURE_TYPE_DATA gpTextureDataTugboat[ NUM_LAYER_TEXTURES ] =
{
	{	D3DFMT_DXT1,		"DXT1",		0x00,	0x00,	TEXTURE_DIFFUSE,	}, // LAYER_TEXTURE_DIFFUSE
	{	D3DFMT_DXT5,		"DXT5",		0x00,	0x00,	TEXTURE_NORMAL,		}, // LAYER_TEXTURE_NORMAL
	{	D3DFMT_DXT1,		"DXT1",		0x00,	0x00,	TEXTURE_SPECULAR,	}, // LAYER_TEXTURE_SPECULAR
	{	D3DFMT_DXT1,		"DXT1",		0x00,	0x00,	TEXTURE_SELFILLUM,	}, // LAYER_TEXTURE_SELFILLUM
	{	D3DFMT_DXT1,		"DXT1",		0xff,	0x00,	TEXTURE_DIFFUSE,	}, // LAYER_TEXTURE_COLORMASK
	{	D3DFMT_L8,			"L8",		0x00,	0x00,	INVALID_ID,			}, // LAYER_TEXTURE_BLEND
};

static const LAYER_TEXTURE_TYPE_DATA gpTextureDataHellgate[ NUM_LAYER_TEXTURES ] =
{
	{	D3DFMT_DXT5,		"DXT5",		0x00,	0x00,	TEXTURE_DIFFUSE,	}, // LAYER_TEXTURE_DIFFUSE
#if defined(DX10_BC5_TEXTURES) && defined(ENGINE_TARGET_DX10)
	{	DXGI_FORMAT_BC5_UNORM,	"ATI2",	0x00,	0x00,	TEXTURE_NORMAL,	}, // LAYER_TEXTURE_NORMAL
#else
	{	D3DFMT_DXT5,		"DXT5",		0x00,	0x00,	TEXTURE_NORMAL,		}, // LAYER_TEXTURE_NORMAL
#endif
	{	D3DFMT_DXT5,		"DXT5",		0x00,	0x00,	TEXTURE_SPECULAR,	}, // LAYER_TEXTURE_SPECULAR
	{	D3DFMT_DXT5,		"DXT5",		0x00,	0x00,	TEXTURE_SELFILLUM,	}, // LAYER_TEXTURE_SELFILLUM
	{	D3DFMT_DXT5,		"DXT5",		0xff,	0x00,	TEXTURE_DIFFUSE,	}, // LAYER_TEXTURE_COLORMASK
	{	D3DFMT_L8,			"L8",		0x00,	0x00,	INVALID_ID,			}, // LAYER_TEXTURE_BLEND
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

void dxC_ConvertDXT1ToRGBByBlock(
	BYTE * pbDestRGB,
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha );
void dxC_ConvertDXT5ToRGBAByBlock(
	BYTE * pbDestRGBA,
	const BYTE * pbSourceDXT5 );

PRESULT e_WardrobeInit()
{
	if ( AppIsTugboat() )
	{
		gpTextureData = gpTextureDataTugboat;
	}
	else
	{
		gpTextureData = gpTextureDataHellgate;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum {
	ROWS_PER_PIXEL_BLOCK = 4,
	PIXELS_PER_BLOCK_ROW = 4,

	RGB_BYTES_PER_PIXEL		= 3,
	RGBA_BYTES_PER_PIXEL	= 4,

	RGB_BYTES_PER_ROW	= RGB_BYTES_PER_PIXEL	* PIXELS_PER_BLOCK_ROW,
	RGBA_BYTES_PER_ROW	= RGBA_BYTES_PER_PIXEL	* PIXELS_PER_BLOCK_ROW,

	RGB_BYTES_PER_BLOCK		= RGB_BYTES_PER_ROW		* ROWS_PER_PIXEL_BLOCK,
	RGBA_BYTES_PER_BLOCK	= RGBA_BYTES_PER_ROW	* ROWS_PER_PIXEL_BLOCK,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This function takes a SYSMEM_TEXTURE!
PRESULT dxC_WardrobeLayerTexturePreSaveBlendTexture(
	TEXTURE_DEFINITION * pTextureDefinition,
	void* pTextureOrig,
	D3DXIMAGE_INFO * pImageInfoFull,
	DWORD dwFlags )
{
	ASSERT_RETINVALIDARG( pTextureOrig );

	SPD3DCTEXTURE2D pTextureFull = *(SPD3DCTEXTURE2D*)&pTextureOrig;

	PRESULT pr = E_FAIL;

	int nBitDepth = dx9_GetTextureFormatBitDepth( pImageInfoFull->Format );
	ASSERTX_RETFAIL( nBitDepth == 8, "Wardrobe blend texture must be 8 bit!" );

	int nMipMapLevels = e_GetNumMIPLevels( max( pImageInfoFull->Width, pImageInfoFull->Height ) );
	ASSERTX_RETFAIL( (UINT)nMipMapLevels == pImageInfoFull->MipLevels, "Blend texture conversion needs a full MIP chain!" );

	SIMPLE_DYNAMIC_ARRAY<BLEND_RLE> tBlendRLEs;
	ArrayInit(tBlendRLEs, NULL, 10);

	SIMPLE_DYNAMIC_ARRAY<BLEND_RUN> tBlendRuns;
	ArrayInit(tBlendRuns, NULL, 100);
	
	BYTE pbSrcAlpha[ 16 ];
	for ( int nLevel = 0; nLevel < nMipMapLevels; nLevel++ )
	{
		D3DLOCKED_RECT tFullLocked;
		V_SAVE_GOTO( pr, cleanup, dxC_MapTexture( pTextureFull, nLevel, &tFullLocked ) );
		BYTE *pbFull  = (BYTE *) dxC_pMappedRectData( tFullLocked );
		int nBlendPitch = dxC_nMappedRectPitch( tFullLocked );
		BYTE* pbBlend = pbFull;

		UINT nCopyHeight = MAX( pImageInfoFull->Height >> nLevel, (UINT)ROWS_PER_PIXEL_BLOCK );
		UINT nCopyWidth = MAX( pImageInfoFull->Width >> nLevel, (UINT)PIXELS_PER_BLOCK_ROW );

		// get the blend color, adjusting for resolution if necessary
		// need to get a 4x4 blend alpha value
		for ( UINT nBY = 0; nBY < nCopyHeight; nBY += 4 )
		{
			BLEND_RUN* pBlendRun = NULL;

			for ( UINT nBX = 0; nBX < nCopyWidth; nBX += 4 )
			{
				int nTotalSrcAlpha = 0;
				int nAlphaIndex = 0;

				for ( int nBlockY = 0; nBlockY < ROWS_PER_PIXEL_BLOCK; nBlockY++ )
				{
					int nBlendY = ( ( nBY + nBlockY ) * nBlendPitch );
					for ( int nBlockX = 0; nBlockX < PIXELS_PER_BLOCK_ROW; nBlockX++ )
					{
						int nByteOffset = nBlendY + ( nBX + nBlockX );
						nTotalSrcAlpha += pbSrcAlpha[ nAlphaIndex ] = pbBlend[ nByteOffset ];
						nAlphaIndex++;
					}
				}

				if ( nTotalSrcAlpha == 0 )
					continue;

				if ( !pBlendRun || ( nTotalSrcAlpha != pBlendRun->nTotalAlpha )
					|| ( pBlendRun->pbAlpha && 
					memcmp( pBlendRun->pbAlpha, pbSrcAlpha, sizeof( pbSrcAlpha ) ) != 0 ) )
				{
					pBlendRun = ArrayAdd(tBlendRuns);
					ASSERT( pBlendRun );
					structclear( *pBlendRun );

					pBlendRun->nBlockStart = ( ( nBY / ROWS_PER_PIXEL_BLOCK ) * 
						( nBlendPitch / PIXELS_PER_BLOCK_ROW ) ) + ( nBX / PIXELS_PER_BLOCK_ROW );
					pBlendRun->nBlockRun = 1;
					pBlendRun->nTotalAlpha = nTotalSrcAlpha;

					if ( nTotalSrcAlpha > 0 && nTotalSrcAlpha < ( 255 * 16 ) )
					{
						pBlendRun->nAlphaCount = PIXELS_PER_BLOCK_ROW * ROWS_PER_PIXEL_BLOCK;
						pBlendRun->pbAlpha = (BYTE*)MALLOC( NULL, sizeof( pbSrcAlpha ) );
						MemoryCopy( pBlendRun->pbAlpha, sizeof( pbSrcAlpha ), pbSrcAlpha, sizeof( pbSrcAlpha ) );
					}
					else
					{
						pBlendRun->nAlphaCount = 0;
						pBlendRun->pbAlpha = NULL;
					}
				}
				else
				{
					pBlendRun->nBlockRun++;
				}
			}
		}

		V_SAVE_GOTO( pr, cleanup, dxC_UnmapTexture( pTextureFull, nLevel ) );

		BLEND_RLE* pBlendRLE = ArrayAdd(tBlendRLEs);
		pBlendRLE->nCount = tBlendRuns.Count();
		if ( pBlendRLE->nCount > 0 )
		{
			int nByteCount = sizeof( BLEND_RUN ) * pBlendRLE->nCount;
			pBlendRLE->pRuns = (BLEND_RUN*)MALLOC( NULL, nByteCount );
			MemoryCopy( pBlendRLE->pRuns, nByteCount, tBlendRuns.GetPointer( 0 ), nByteCount );
			ArrayClear(tBlendRuns);
		}
		else
			pBlendRLE->pRuns = NULL;

		USER_CODE
		( AE,
			trace( "Blend Texture Id: %d (%s)\tLevel: %d\n", pTextureDefinition->tHeader.nId, 
				pTextureDefinition->tHeader.pszName, nLevel );
			for ( int nBlendRun = 0; nBlendRun < pBlendRLE->nCount; nBlendRun++ )
			{
				BLEND_RUN* pBlendRun = &pBlendRLE->pRuns[ nBlendRun ];
				ASSERT( pBlendRun );

				trace( "%d: Start: %d\tRun: %d\tTotalAlpha: %d\n", 
					nBlendRun, pBlendRun->nBlockStart, pBlendRun->nBlockRun, 
					pBlendRun->nTotalAlpha );
			} 
		)
	}

	pr = S_OK;
cleanup:
	pTextureDefinition->nBlendWidth  = pImageInfoFull->Width;
	pTextureDefinition->nBlendHeight = pImageInfoFull->Height;

	int nBlendRLECount = tBlendRLEs.Count() ;
	if ( nBlendRLECount > 0 )
	{
		pTextureDefinition->nBlendRLELevels = nBlendRLECount;
		pTextureDefinition->nMipMapLevels = nBlendRLECount;
		int nByteCount = sizeof( BLEND_RLE ) * nBlendRLECount;
		pTextureDefinition->pBlendRLE = (BLEND_RLE*)MALLOC( NULL, nByteCount );
		MemoryCopy( pTextureDefinition->pBlendRLE, nByteCount, tBlendRLEs.GetPointer( 0 ), nByteCount );
	}

	CDefinitionContainer* pDefinitionContainer = CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_TEXTURE );
	ASSERT_RETFAIL( pDefinitionContainer );

	BOOL bFreeResources = TRUE;
	BOOL bReloadDefinition = FALSE;
	if ( SUCCEEDED(pr) )
	{
		TCHAR szXMLFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrReplaceExtension( szXMLFilename, DEFAULT_FILE_WITH_PATH_SIZE, pTextureDefinition->tHeader.pszName, DEFINITION_FILE_EXTENSION );

#if ISVERSION(DEVELOPMENT)
		if ( c_GetToolMode() && ! TEST_MASK( dwFlags, TEXTURE_LOAD_DONT_CHECKOUT_XML ) && TEST_MASK( dwFlags, TEXTURE_LOAD_DONT_ASK_TO_CHECKOUT_XML ) )
		{
			// force check-out
			FileCheckOut( szXMLFilename );
			bReloadDefinition = TRUE;
		}
		else
		{
			if ( ! TEST_MASK( dwFlags, TEXTURE_LOAD_NO_ERROR_ON_SAVE ) && ! TEST_MASK( dwFlags, TEXTURE_LOAD_DONT_ASK_TO_CHECKOUT_XML ) && ! TEST_MASK( dwFlags, TEXTURE_LOAD_DONT_CHECKOUT_XML ) )
			{
				bReloadDefinition = DataFileCheck( szXMLFilename );
			}

			if ( ! bReloadDefinition )
			{
				// We are not freeing resources because there will be no XML/cooked file
				// to reload from. Unfortunately, this means that the memory for the 
				// blend RLE data will not be freed. This only occurs during development 
				// though, since all blend RLE data should be current in release.
				bFreeResources = FALSE;
			}
		}
#endif

		pDefinitionContainer->Save( pTextureDefinition );
	}

	if ( bFreeResources )
	{
		for ( int nBlendRLE = 0; nBlendRLE < nBlendRLECount; nBlendRLE++ )
		{
			BLEND_RLE* pBlendRLE = &pTextureDefinition->pBlendRLE[ nBlendRLE ];

			for ( int nBlendRun = 0; nBlendRun < pBlendRLE->nCount; nBlendRun++ )
			{
				BLEND_RUN* pBlendRun = &pBlendRLE->pRuns[ nBlendRun ];
				FREE( NULL, pBlendRun->pbAlpha );
			}

			FREE( NULL, pBlendRLE->pRuns );
		}
		FREE( NULL, pTextureDefinition->pBlendRLE );
		pTextureDefinition->pBlendRLE = NULL;
	}

	if ( bReloadDefinition )
		pDefinitionContainer->Reload( pTextureDefinition );
	
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeTextureNewLayer( 
	int * pnTextureID,
	const char * pszBaseFolder,
	const char * pszTextureSetFolder,
	const char * pszFilename,
	int nMaxLen,
	LAYER_TEXTURE_TYPE eTextureType,
	int nLOD,
	int nModelDef,
	TEXTURE_DEFINITION* pDefOverride,
	DWORD dwReloadFlags /*= 0*/,
	DWORD dwTextureFlags /*= 0*/ )
{
	ASSERT_RETINVALIDARG( pnTextureID );
	*pnTextureID = INVALID_ID;
	ASSERT_RETINVALIDARG( pszFilename );

	char pszFileWithPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	e_TextureGetFullFileName( pszFileWithPath, sizeof( pszFileWithPath ), 
		pszBaseFolder, pszTextureSetFolder, pszFilename );
	const LAYER_TEXTURE_TYPE_DATA * pTextureTypeData = &gpTextureData[ eTextureType ];

	dwTextureFlags |= TEXTURE_FLAG_FORCEFORMAT | TEXTURE_FLAG_SYSMEM;
	TEXTURE_TYPE eType = (TEXTURE_TYPE) pTextureTypeData->nTextureType;
	if ( eTextureType == LAYER_TEXTURE_BLEND )
		eType = TEXTURE_DIFFUSE; // this is to help with priorities
	ASSERT_RETFAIL( eType != TEXTURE_NONE );

	// CHB 2006.10.09 - Don't load unused texture types.
	if (!e_TextureSlotIsUsed(eType))
	{
		return S_FALSE;
	}

	if ( eTextureType != LAYER_TEXTURE_BLEND && nLOD != LOD_HIGH )
	{
		e_ModelDefinitionGetLODFileName(pszFileWithPath, ARRAYSIZE(pszFileWithPath), pszFileWithPath);
		dwTextureFlags |= TEXTURE_FLAG_NOERRORIFMISSING;
	}

	PFN_TEXTURE_PRESAVE fpnPreSave = NULL;
	if ( eTextureType == LAYER_TEXTURE_BLEND )
	{
		if ( AppCommonAllowAssetUpdate() )
		{
			char pszXMLFileName[DEFAULT_FILE_WITH_PATH_SIZE];
			PStrReplaceExtension(pszXMLFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileWithPath, DEFINITION_FILE_EXTENSION);

			TEXTURE_DEFINITION* pBlendDefinition = (TEXTURE_DEFINITION*)DefinitionGetByFilename( DEFINITION_GROUP_TEXTURE, pszFileWithPath, -1, FALSE, TRUE );
			if ( !pBlendDefinition )
			{
				*pnTextureID = BLEND_TEXTURE_DEFINITION_LOADING_ID;
				return S_OK;
			}
			else if ( !pBlendDefinition->pBlendRLE || ( c_GetToolMode() && FileNeedsUpdate( pszXMLFileName, pszFileWithPath ) ) )
			{
				// Create/update the blend RLE data.
				fpnPreSave = dxC_WardrobeLayerTexturePreSaveBlendTexture;
				dwReloadFlags |= TEXTURE_LOAD_FORCECREATETFILE | TEXTURE_LOAD_NO_SAVE 
									| TEXTURE_LOAD_NO_ASYNC_TEXTURE;
			}
			else
			{
				// Use the existing blend RLE data.
				*pnTextureID = pBlendDefinition->tHeader.nId;
				return S_OK;
			}
		}
		else
		{
			int nBlendDefId = INVALID_ID; 
			TEXTURE_DEFINITION* pBlendDefinition = (TEXTURE_DEFINITION*)DefinitionGetByFilename( DEFINITION_GROUP_TEXTURE, pszFileWithPath, -1, FALSE, TRUE );
			
			if ( !pBlendDefinition )
				nBlendDefId = BLEND_TEXTURE_DEFINITION_LOADING_ID;
			else
			{
				ASSERTXONCE_RETVAL( pBlendDefinition->pBlendRLE, S_FALSE, "No Blend RLE Data!" );
				nBlendDefId = pBlendDefinition->tHeader.nId;
			}

			*pnTextureID = nBlendDefId;
			return S_OK;
		}
	}

	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	if (	 e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD ) && ( nLOD == LOD_HIGH ) 
		&& ( !pHash 
		  || !TEST_MASK( pHash->dwLODFlags[ LOD_LOW ], MODELDEF_HASH_FLAG_LOAD_FAILED ) ) )
		dwReloadFlags |= TEXTURE_LOAD_ADD_TO_HIGH_PAK;

	{
		V_RETURN( e_TextureNewFromFile( pnTextureID, pszFileWithPath, TEXTURE_GROUP_WARDROBE, 
			eType, dwTextureFlags, pTextureTypeData->eFormat, NULL, fpnPreSave, NULL, 
			pDefOverride, dwReloadFlags ) );
	}

	if ( eTextureType == LAYER_TEXTURE_BLEND )
		*pnTextureID = BLEND_TEXTURE_DEFINITION_LOADING_ID;
	else
	{
		D3D_TEXTURE* pTexture = dxC_TextureGet( *pnTextureID );
		ASSERTV_RETFAIL( pTexture, pszFileWithPath );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD e_WardrobeGetLayerFormat( LAYER_TEXTURE_TYPE eLayer )
{
	return gpTextureData[ eLayer ].eFormat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_WardrobeGetLayerTextureType( LAYER_TEXTURE_TYPE eLayer )
{	
	return gpTextureData[ eLayer ].nTextureType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeValidateTexture( int nTextureID, LAYER_TEXTURE_TYPE eLayer )
{
	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	if ( pTexture )
	{
		ASSERT_RETFAIL( pTexture->nWidthInBytes );
		ASSERT_RETFAIL( pTexture->nWidthInPixels );
		ASSERT_RETFAIL( pTexture->nHeight );
		TEXTURE_DEFINITION * pDef = (TEXTURE_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		if ( pDef )
		{
			const LAYER_TEXTURE_TYPE_DATA * pTextureTypeData = &gpTextureData[ eLayer ];
			ASSERTV_RETFAIL( pTexture->nFormat == pTextureTypeData->eFormat, "Wardrobe texture is in the wrong format!  Should be %s.\n\n%s", pTextureTypeData->szFormatName, pDef->tHeader.pszName );
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline WORD sLerp565Colors( 
	WORD wColor1, 
	WORD wColor2, 
	int nColor2Blend )
{
	const WORD wRedMask = 0xf800;
	const WORD wGreenMask = 0x07e0;
	const WORD wBlueMask = 0x001f;

	DWORD wRed1   = wColor1 & wRedMask;
	DWORD wRed2   = wColor2 & wRedMask;
	DWORD wGreen1 = wColor1 & wGreenMask;
	DWORD wGreen2 = wColor2 & wGreenMask;
	DWORD wBlue1  = wColor1 & wBlueMask;
	DWORD wBlue2  = wColor2 & wBlueMask;

	WORD wRed   = (WORD) (((wRed1   * (255 - nColor2Blend) + wRed2   * nColor2Blend) + 127) >> 8);
	WORD wGreen = (WORD) (((wGreen1 * (255 - nColor2Blend) + wGreen2 * nColor2Blend) + 127) >> 8);
	WORD wBlue  = (WORD) (((wBlue1  * (255 - nColor2Blend) + wBlue2  * nColor2Blend) + 127) >> 8);

	WORD wResult = (WORD)( (wRed & wRedMask) | (wGreen & wGreenMask) | (wBlue & wBlueMask) );

	return wResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sGetDefaultCompressedPacket (
	LAYER_TEXTURE_TYPE eLayerTextureType,
	BYTE * pbPacketToFill,
	int nPacketMaxSize,
	BOOL bCanvas )
{
	const LAYER_TEXTURE_TYPE_DATA * pLayerType = &gpTextureData[ eLayerTextureType ];
	int nDefaultValue = bCanvas ? pLayerType->nDefaultCanvas : pLayerType->nDefaultLayer;

	if ( pLayerType->eFormat == D3DFMT_DXT1 )
	{
		// DXT1
		// 2 bytes color 0 (565)
		// 2 bytes color 1 (565)
		// 4 bytes of 0 (indexes)
		ASSERT_RETFAIL( nPacketMaxSize >= cnDXT1BytesPerPacket );

		if ( eLayerTextureType == LAYER_TEXTURE_NORMAL )
			*(UINT16*)&pbPacketToFill[ 0 ] = MAKE_565( 127, 127, 255 );
		else
		{
			*(UINT16*)&pbPacketToFill[ 0 ] = MAKE_565( nDefaultValue, nDefaultValue, nDefaultValue );
		}
		*(UINT16*)&pbPacketToFill[ 2 ] = *(UINT16*)&pbPacketToFill[ 0 ];
		*(UINT32*)&pbPacketToFill[ 4 ] = 0;

	} 
	else if ( (DXC_FORMAT)pLayerType->eFormat == D3DFMT_DXT2 || (DXC_FORMAT)pLayerType->eFormat == D3DFMT_DXT3 )
	{
		// DXT2-3
		// 8 bytes alpha
		// 8 bytes dxt1-like color
		ASSERT_RETFAIL( nPacketMaxSize >= cnDXT2_5BytesPerPacket );

		// alpha (explicit)
		BYTE byAlpha = ( nDefaultValue & 15 ) + ( ( nDefaultValue & 15 ) << 4 );
		for ( int i = 0; i < 8; i++ )
			pbPacketToFill[ i ] = byAlpha;

		// color
		*(UINT16*)&pbPacketToFill[  8 ] = MAKE_565( nDefaultValue, nDefaultValue, nDefaultValue );
		*(UINT16*)&pbPacketToFill[ 10 ] = *(UINT16*)&pbPacketToFill[ 8 ];
		*(UINT32*)&pbPacketToFill[ 12 ] = 0;
	} 
	else
	{
		// DXT4-5
		// 8 bytes alpha
		// 8 bytes dxt1-like color
		ASSERT_RETFAIL( nPacketMaxSize >= cnDXT2_5BytesPerPacket );

		// alpha (interpolated)
		pbPacketToFill[ 0 ]			  = nDefaultValue;
		pbPacketToFill[ 1 ]			  = pbPacketToFill[ 0 ];
		*(UINT16*)&pbPacketToFill[  2 ] = 0;
		*(UINT16*)&pbPacketToFill[  4 ] = 0;
		*(UINT16*)&pbPacketToFill[  6 ] = 0;

		// color
		*(UINT16*)&pbPacketToFill[  8 ] = MAKE_565( nDefaultValue, nDefaultValue, nDefaultValue );
		*(UINT16*)&pbPacketToFill[ 10 ] = *(UINT16*)&pbPacketToFill[ 8 ];
		*(UINT32*)&pbPacketToFill[ 12 ] = 0;
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sCopyLayerToCanvasCompressed (
	BYTE* pbCanvas,
	const BYTE* pbLayer,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	const int nBlendDefinitionId,
	int nCanvasWidth,
	int nBlendWidth,
	DXC_FORMAT eCanvasFormat )
{
	ASSERT_RETINVALIDARG( pbCanvas );
	ASSERT_RETINVALIDARG( nBlendWidth >= nCanvasWidth );

	if ( nCanvasWidth < 4 )
		return S_FALSE;

	int nBlendScale = nBlendWidth / nCanvasWidth;
	int nBlendLevel = int( logf( float( nBlendScale ) ) / logf( 2.0f ) );
	ASSERT_RETFAIL( nBlendScale > 0 );

	BOOL bDXT1 = ( eCanvasFormat == D3DFMT_DXT1 );
	int nBytesPerBlock = bDXT1 ? cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;

	DWORD pbDefaultPacket[ 4 ] = { 0, 0, 0, 0 };
	BYTE pbDefaultRGBA [ RGBA_BYTES_PER_BLOCK ];
	if ( !pbLayer )
	{
		V_RETURN( sGetDefaultCompressedPacket ( eLayerTextureType, (BYTE *)pbDefaultPacket, sizeof( pbDefaultPacket ), FALSE ) );
		const LAYER_TEXTURE_TYPE_DATA * pLayerType = &gpTextureData[ eLayerTextureType ];
		squish::Decompress( pbDefaultRGBA, pbDefaultPacket, ( pLayerType->eFormat == D3DFMT_DXT1 ) ? squish::kDxt1 : squish::kDxt5 );
	}

	TEXTURE_DEFINITION* pBlendDefinition = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, nBlendDefinitionId );
	ASSERT_RETFAIL( pBlendDefinition && pBlendDefinition->pBlendRLE );

	ASSERT_RETFAIL( nBlendLevel <= pBlendDefinition->nBlendRLELevels );
	ASSERT_RETFAIL( nBlendLevel <= pBlendDefinition->nMipMapLevels );
	BLEND_RLE* pBlendRLE = &pBlendDefinition->pBlendRLE[ nBlendLevel ];
	ASSERT_RETFAIL( pBlendRLE );
	for ( int nBlendRun = 0; nBlendRun < pBlendRLE->nCount; nBlendRun++ )
	{
		BLEND_RUN* pBlendRun = &pBlendRLE->pRuns[ nBlendRun ];
		ASSERT_RETFAIL( pBlendRun );

		int nByteOffset = pBlendRun->nBlockStart * nBytesPerBlock;

		BYTE* pbCanvasCompressedCurr = &pbCanvas[ nByteOffset ];
		const BYTE* pbLayerCompressedCurr = pbLayer ? &pbLayer[ nByteOffset ] : NULL;

		if ( pBlendRun->nTotalAlpha >= 255 * 16 )
		{
			if ( !pbLayer )
			{
				int nBlocksToCopy = pBlendRun->nBlockRun;
				while ( nBlocksToCopy > 0 )
				{
					MemoryCopy( pbCanvasCompressedCurr, nBytesPerBlock, pbDefaultPacket, nBytesPerBlock );
					pbCanvasCompressedCurr += nBytesPerBlock;
					nBlocksToCopy--;
				}
			}
			else
			{
				int nBytesToCopy = pBlendRun->nBlockRun * nBytesPerBlock;
				MemoryCopy( pbCanvasCompressedCurr, nBytesToCopy, pbLayerCompressedCurr, nBytesToCopy );
			}
		}
		else if ( pBlendRun->nTotalAlpha > 0 ) 
		{
			int nBlocksToCopy = pBlendRun->nBlockRun;

			while ( nBlocksToCopy > 0 )
			{
				if ( pbLayer && memcmp( pbLayerCompressedCurr, pbCanvasCompressedCurr, nBytesPerBlock ) == 0 )
				{ 
					// don't bother to do anything complicated - the layer and canvas are the same
				}
				else 
				{
					// do it the slow, ugly way: decompress, blend, compress
					ALIGN(16) BYTE pbLayerDecompressed [ RGBA_BYTES_PER_BLOCK ];
					ALIGN(16) BYTE pbCanvasDecompressed[ RGBA_BYTES_PER_BLOCK ];

					int squishFlags = bDXT1 ? squish::kDxt1 : squish::kDxt5;
					squishFlags |= squish::kColourRangeFit | squish::kColourMetricUniform;

					if ( pbLayer )
					{
						//dxC_ConvertDXT5ToRGBAByBlock( pbLayerDecompressed, pbLayerCompressedCurr );
						squish::Decompress( pbLayerDecompressed, pbLayerCompressedCurr, squishFlags );
					} 
					//dxC_ConvertDXT5ToRGBAByBlock	 ( pbCanvasDecompressed,	pbCanvasCompressedCurr );
					squish::Decompress( pbCanvasDecompressed, pbCanvasCompressedCurr, squishFlags );

					// combine to form the target 24 bit block
					BYTE * pbLayerByteCurr	= pbLayer ? pbLayerDecompressed : pbDefaultRGBA;
					BYTE * pbCanvasByteCurr	= pbCanvasDecompressed;
					for ( int i = 0; i < ROWS_PER_PIXEL_BLOCK * PIXELS_PER_BLOCK_ROW; i++ )
					{
						BYTE bAlpha = pBlendRun->pbAlpha[ i ];
						if ( !bAlpha )
						{
							// Don't copy anything
						}
						else if ( bAlpha == 255 )
						{
							*(DWORD*)pbCanvasByteCurr = *(DWORD*)pbLayerByteCurr;
						}
						else
						{
							// 8-bit packed number lerp
							// blended color = a*(1-f) + b*f = a + (b-a)*f
							// a = pbCanvasByteCurr
							// b = pbLayerByteCurr
							// f = bAlpha / 255
							DWORD dwCanvas = *(DWORD*)pbCanvasByteCurr;
							DWORD dwLayer = *(DWORD*)pbLayerByteCurr;

							UINT nDstRB = dwCanvas & 0xff00ff;
							UINT nDstAG = dwCanvas >> 8 & 0xff00ff;

							UINT nSrcRB = dwLayer & 0xff00ff;
							UINT nSrcAG = dwLayer >> 8 & 0xff00ff;

							// Treat bAlpha as 1..256
							UINT nBlendRB = ( ( nSrcRB - nDstRB ) * ( bAlpha + 1 ) ) >> 8;
							UINT nBlendAG = ( ( nSrcAG - nDstAG ) * ( bAlpha + 1 ) ) >> 8;

							const UINT nFinalRB = ( nBlendRB + nDstRB ) & 0x00ff00ff;
							const UINT nFinalAG = ( nBlendAG + nDstAG ) << 8 & 0xff00ff00;

							*(DWORD*)pbCanvasByteCurr = nFinalRB | nFinalAG;
						}

						pbLayerByteCurr += RGBA_BYTES_PER_PIXEL;
						pbCanvasByteCurr+= RGBA_BYTES_PER_PIXEL;
					}

					squish::Compress( pbCanvasDecompressed, pbCanvasCompressedCurr, squishFlags );
				}

				pbLayerCompressedCurr += nBytesPerBlock;
				pbCanvasCompressedCurr += nBytesPerBlock;
				nBlocksToCopy--;
			}
		}
	}
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeAddTextureToCanvas(
	int nCanvas,
	int nLayer,
	int nBlend,
	LAYER_TEXTURE_TYPE eLayerTextureType )
{
	TIMER_STARTEX2("e_WardrobeAddTextureToCanvas",10);

	BOOL bLayerIsDefaultColor = (nLayer == TEXTURE_WITH_DEFAULT_COLOR);
	D3D_TEXTURE * pCanvas = dxC_TextureGet( nCanvas );
	D3D_TEXTURE * pLayer  = (!bLayerIsDefaultColor) ? dxC_TextureGet( nLayer ) : NULL;

	ASSERT_RETINVALIDARG( pCanvas );
	ASSERT_RETINVALIDARG( pCanvas->ptSysmemTexture );

	int nMipBonus = 0;

	if ( ! bLayerIsDefaultColor )
	{
		ASSERT_RETINVALIDARG( pLayer );
		ASSERT_RETINVALIDARG( pLayer->dwFlags  & TEXTURE_FLAG_SYSMEM );
		ASSERT_RETINVALIDARG( pLayer->ptSysmemTexture  );

		int nCanvasMipLevelCount = dxC_GetNumMipLevels( pCanvas );
		int nLayerMipLevelCount = dxC_GetNumMipLevels( pLayer );
		
		TEXTURE_DEFINITION* pLayerDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pLayer->nDefinitionId );
		ASSERTV_RETFAIL( nLayerMipLevelCount >= nCanvasMipLevelCount, 
			"'%s' has less mip levels than the canvas: %d < %d\n",
			pLayer ? pLayerDef->tHeader.pszName : "NoTextureDef",
			nLayerMipLevelCount, nCanvasMipLevelCount );
		nMipBonus = nLayerMipLevelCount - nCanvasMipLevelCount;

		ASSERT_RETINVALIDARG( pCanvas->nHeight == ( pLayer->nHeight >> nMipBonus ) );
		ASSERT_RETINVALIDARG( pCanvas->nD3DTopMIPSize >= dx9_GetTextureLODSizeInBytes( pLayer, nMipBonus ) );
		ASSERT_RETINVALIDARG( pCanvas->nWidthInBytes == ( pLayer->nWidthInBytes >> nMipBonus ) );
	}

	TEXTURE_DEFINITION * pBlendDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, nBlend );
	ASSERT_RETINVALIDARG( pBlendDefinition );
	int nBlendWidth  = pBlendDefinition->nBlendWidth;
	
	ASSERT_RETINVALIDARG( bLayerIsDefaultColor || pCanvas->nWidthInPixels == ( pLayer->nWidthInPixels >> nMipBonus ) );
	ASSERT_RETINVALIDARG( nBlendWidth >= pCanvas->nWidthInPixels );

	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( (DXC_FORMAT)pCanvas->nFormat ) );
	
	PRESULT pr = E_FAIL;
	
	DX9_BLOCK( DXC_FORMAT eDecompressedFormat = pCanvas->nFormat == D3DFMT_DXT1 ? D3DFMT_R8G8B8 : D3DFMT_A8R8G8B8; )
	DX10_BLOCK( DXC_FORMAT eDecompressedFormat = D3DFMT_A8R8G8B8; )

	if ( pLayer )
	{
		TEXTURE_DEFINITION* pLayerDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pLayer->nDefinitionId );
		ASSERT( pLayerDef );
		if ( ( pLayerDef->nFormat != TEXFMT_DXT1 ) 
			|| TEST_MASK( pLayerDef->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT1ALPHA ) )
			SET_MASK( pCanvas->dwFlags, TEXTURE_FLAG_HAS_ALPHA );
	}

	int nLevelCount = dxC_GetNumMipLevels( pCanvas );
	ASSERT_GOTO( bLayerIsDefaultColor || nLevelCount == ( dxC_GetNumMipLevels( pLayer ) - nMipBonus ), done );
	for ( int nLevel = 0; nLevel < nLevelCount; nLevel++ )
	{
		int nLevelRGB = nLevel + nMipBonus;

		// lock textures
		D3DLOCKED_RECT tLayerLocked; 
		D3DLOCKED_RECT tCanvasLocked; 
		if ( ! bLayerIsDefaultColor )
		{
			V_SAVE_GOTO( pr, lvlunlock, dxC_MapSystemMemTexture( pLayer->ptSysmemTexture, nLevelRGB, &tLayerLocked ) );
		}
		D3DC_TEXTURE2D_DESC tCanvasDesc;
		V_SAVE_GOTO( pr, lvlunlock, dxC_MapSystemMemTexture( pCanvas->ptSysmemTexture, nLevel, &tCanvasLocked ) );
		V_SAVE_GOTO( pr, lvlunlock, dxC_Get2DTextureDesc( pCanvas, nLevel, &tCanvasDesc ) );

		if ( tCanvasDesc.Width < 4 )
		{
			// For compressed texture formats, the minimum texture size is a 4x4,
			// so we might as well copy from the 4x4 mip level.
			int nCopyFromLevel = nLevel - ( 2 / tCanvasDesc.Width ); // this will jump either 1 or 2 mip levels.
			D3DLOCKED_RECT tCanvasCopyLocked; 
			V_SAVE_GOTO( pr, copyunlock, dxC_MapSystemMemTexture( pCanvas->ptSysmemTexture, nCopyFromLevel, &tCanvasCopyLocked ) );

			BYTE* pbCanvas	 = ((BYTE *) dxC_pMappedRectData( tCanvasLocked ) );
			BYTE* pbCanvasCopy = ((BYTE *) dxC_pMappedRectData( tCanvasCopyLocked ) );
			
			BOOL bDXT1 = ( pCanvas->nFormat == D3DFMT_DXT1 );
			int nBytesPerBlock = bDXT1 ? cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;

			DWORD pbDefaultPacket[ 4 ] = { 0, 0, 0, 0 };
			V_RETURN( sGetDefaultCompressedPacket ( eLayerTextureType, (BYTE*)pbDefaultPacket, sizeof( pbDefaultPacket ), FALSE ) );
			MemoryCopy( pbCanvas, nBytesPerBlock, pbDefaultPacket, nBytesPerBlock );

copyunlock:
			V( dxC_UnmapSystemMemTexture( pCanvas->ptSysmemTexture, nCopyFromLevel ) );
		}
		else
		{
			int nBlendScale = nBlendWidth / tCanvasDesc.Width;

			// copy the texture
			BYTE *pbLayer = NULL;
			BYTE *pbCanvas;

			if ( !bLayerIsDefaultColor )
				pbLayer  = ((BYTE *) dxC_pMappedRectData( tLayerLocked ) );
			pbCanvas	 = ((BYTE *) dxC_pMappedRectData( tCanvasLocked ) );

			V_SAVE_GOTO( pr, lvlunlock, sCopyLayerToCanvasCompressed( pbCanvas, pbLayer, 
				eLayerTextureType, nBlend, tCanvasDesc.Width, nBlendWidth, 
				(DXC_FORMAT)pCanvas->nFormat ) );
		}
	
lvlunlock:
		if ( !bLayerIsDefaultColor )
		{
			V( dxC_UnmapSystemMemTexture( pLayer->ptSysmemTexture, nLevelRGB ) );
		}
		// DX10: Keep the canvas CPU texture persistent until we complete.
		V( dxC_UnmapSystemMemTexture( pCanvas->ptSysmemTexture, nLevel ) );

		if ( FAILED( pr ) )
			break;
	}

done:
	pr = S_OK;

	TIMER_END2();

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sReleaseCanvasSysmemTexture( D3D_TEXTURE * pCanvas )
{
	ASSERT_RETFAIL( pCanvas );
	if ( ! pCanvas->ptSysmemTexture )
		return S_FALSE;
	pCanvas->ptSysmemTexture->Free();
	FREE( g_ScratchAllocator, pCanvas->ptSysmemTexture );
	pCanvas->ptSysmemTexture = NULL;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUpdateCanvasD3DTexture( D3D_TEXTURE * pCanvas )
{
	TIMER_STARTEX2("sUpdateCanvasD3DTexture",10);

	if ( ! pCanvas->ptSysmemTexture )
		return S_FALSE;

	//ASSERT_RETINVALIDARG( pCanvas );
	ASSERT_RETFAIL( 0 == ( pCanvas->dwFlags & TEXTURE_FLAG_WARDROBE_COLOR_COMBINING ) );
	ASSERT_RETINVALIDARG( pCanvas->pD3DTexture );

	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( (DXC_FORMAT)pCanvas->nFormat ) );

	D3DLOCKED_RECT tLR;
	int nLevelCount = dxC_GetNumMipLevels( pCanvas->pD3DTexture );
	for ( int nLevel = 0; nLevel < nLevelCount; nLevel++ )
	{	
		V_CONTINUE( pCanvas->ptSysmemTexture->Lock( nLevel, &tLR ) );

#ifdef ENGINE_TARGET_DX9

		D3DLOCKED_RECT tLRCanvas;
		//V_GOTO( looperror, dxC_MapTexture( pCanvas->pD3DTexture, nLevel, &tLRCanvas, false, false, true ) );
		V_GOTO( looperror, dxC_MapTexture( pCanvas->pD3DTexture, nLevel, &tLRCanvas, false ) );

		if ( dxC_pMappedRectData( &tLRCanvas ) )
		{
			DWORD nLevelSize = pCanvas->ptSysmemTexture->GetLevelSize( nLevel );
			//ASSERT( dxC_nMappedRectPitch( tLR ) == dxC_nMappedRectPitch( tLRCanvas ) );
			//ASSERT( nLevelSize == (pCanvas->nHeight * dxC_nMappedRectPitch( tLRCanvas )) );
			MemoryCopy( dxC_pMappedRectData( tLRCanvas ), nLevelSize, dxC_pMappedRectData( tLR ), nLevelSize );

			//for ( DWORD dwY = 0; dwY < pCanvas->nHeight; ++dwY )
			//{
			//	MemoryCopy( dxC_pMappedRectData( tLRCanvas ), dxC_nMappedRectPitch( tLRCanvas ), dxC_pMappedRectData( tLR ), dxC_nMappedRectPitch( tLR ) );
			//	memcpy( pbCanvas, pbDefault, dxC_nMappedRectPitch( tCanvasLocked ) );
			//	pbCanvas += dxC_nMappedRectPitch( tCanvasLocked );
			//}

		}

		V_GOTO( looperror, dxC_UnmapTexture( pCanvas->pD3DTexture, nLevel ) );

#else // ENGINE_TARGET_DX10

		//extern std::map<LPD3DCTEXTURE2D,SPD3DCTEXTURE2D> gmCPUTextures;


		dxC_GetDevice()->UpdateSubresource( pCanvas->pD3DTexture, 
			nLevel, NULL, dxC_pMappedRectData( tLR ), dxC_nMappedRectPitch( tLR ), 0 );


		//if( gmCPUTextures[ pCanvas->pD3DTexture ] != pCanvas->pD3DTexture ) //Don't want to copy to ourselves
		//{
		//	dxC_GetDevice()->CopyResource( pCanvas->pD3DTexture, gmCPUTextures[ pCanvas->pD3DTexture ] );
		//	//int nLevelCount = dxC_GetNumMipLevels( pCanvas->pD3DTexture );
		//	//for ( int nLevel = 0; nLevel < nLevelCount; nLevel++ )
		//	//{	
		//	//	dxC_GetDevice()->CopySubresourceRegion( pCanvas->pD3DTexture, nLevel, 
		//	//		0, 0, 0, gmCPUTextures[ pCanvas->pD3DTexture ], nLevel, NULL );

		//	//	//V_CONTINUE( dxC_UnmapTexture( pCanvas->pD3DTexture, nLevel, true ) );
		//	//}
		//}
#endif		// DX9 ?: DX10

#ifdef ENGINE_TARGET_DX9
looperror:
#endif
		V( pCanvas->ptSysmemTexture->Unlock( nLevel ) );
	}

	// This marks it as not uploaded.
	V( dxC_TextureInitEngineResource( pCanvas ) );

	TIMER_END2();

	return S_OK;
}

PRESULT e_WardrobeCanvasComplete( int nCanvas, BOOL bReleaseSysMem /*= TRUE*/ )
{
	D3D_TEXTURE * pCanvas = dxC_TextureGet( nCanvas );
	if ( ! pCanvas )
		return S_FALSE;

	// CML 2007.11.04 - During a colormask-only change, some canvases (like normal or specular map) won't get a recreated sysmem texture.
	if ( ! pCanvas->ptSysmemTexture )
		return S_FALSE;

	PRESULT pr = S_OK;
	V_SAVE( pr, sUpdateCanvasD3DTexture( pCanvas ) );
	if ( bReleaseSysMem )
	{
		V( sReleaseCanvasSysmemTexture( pCanvas ) );
	}
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sMakeBaseCanvasCompressed(
	D3DLOCKED_RECT & tCanvasLocked,
	D3DLOCKED_RECT & tLayerRGBLocked,
	D3D_TEXTURE * pCanvas,
	D3D_TEXTURE * pLayerRGB,
	int nLevel,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int nMipBonus )
{
	ASSERT_RETINVALIDARG( pCanvas );
	//ASSERT_RETURN( pLayerRGB );
	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( (DXC_FORMAT)pCanvas->nFormat ) );
	ASSERT_RETINVALIDARG( ! pLayerRGB || dx9_IsCompressedTextureFormat( (DXC_FORMAT)pLayerRGB->nFormat ) );
	ASSERT_RETINVALIDARG( TEST_MASK( pCanvas->dwFlags, TEXTURE_FLAG_SYSMEM ) || pCanvas->pD3DTexture );
	ASSERT_RETINVALIDARG( pCanvas->ptSysmemTexture );
	ASSERT_RETINVALIDARG( nLevel >= 0 && nLevel < (int)dxC_GetNumMipLevels( pCanvas ) );

	D3DC_TEXTURE2D_DESC tCanvasDesc, tLayerRGBDesc;
	V_RETURN( dxC_Get2DTextureDesc( pCanvas, nLevel, &tCanvasDesc ) );
	if ( pLayerRGB && pLayerRGB->ptSysmemTexture )
	{
		V_RETURN( dxC_Get2DTextureDesc( pLayerRGB, nLevel + nMipBonus, &tLayerRGBDesc ) );
	}

	BYTE * pbCanvas		  = (BYTE*) dxC_pMappedRectData( tCanvasLocked );
	BYTE * pbLayerRGBBase = pLayerRGB ? (BYTE*) dxC_pMappedRectData( tLayerRGBLocked ) : NULL;

	BYTE * pbLayerRGB = pbLayerRGBBase;

	ASSERT_RETFAIL( pbCanvas );
	ASSERT_RETFAIL( ! pLayerRGB || pbLayerRGBBase );

	BOOL bDXT1 = ( (DXC_FORMAT)pCanvas->nFormat == D3DFMT_DXT1 );

	float fLayerScale = 0.0f;
	if ( pbLayerRGB )
		fLayerScale = (float)tLayerRGBDesc.Width / (float)tCanvasDesc.Width;
	//int nBitsPerPixel = ( pCanvas->nWidthInBytes * 8 ) / ( pCanvas->nWidthInPixels * 8 );
	//int nBitsPerPixel = e_GetTextureFormatBitDepth( (DXC_FORMAT)pCanvas->nFormat );

	// DXTx is organized differently -- 4 lines at once, in 8 or 16 byte chunks of 16 pixels

	int nCanvasBlockHeight = tCanvasDesc.Height / 4;
	int nBytesPerBlock = bDXT1 ? cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;
	BYTE * pbDefault = (BYTE*)MALLOC( NULL, dxC_nMappedRectPitch( tCanvasLocked ) );
	ASSERT_RETVAL( pbDefault, E_OUTOFMEMORY );

	BYTE pbDefaultBlock[ 32 ];
	PRESULT pr = E_FAIL;
	V_SAVE_GOTO( pr, cleanup, sGetDefaultCompressedPacket( eLayerTextureType, pbDefaultBlock, sizeof( pbDefaultBlock ), TRUE ) );

	for ( UINT i = 0; i < dxC_nMappedRectPitch( tCanvasLocked ); i += nBytesPerBlock )
		memcpy( &pbDefault[ i ], pbDefaultBlock, nBytesPerBlock );

	for( int nY = 0; nY < nCanvasBlockHeight; nY++ )
	{
		if ( ! pLayerRGB )
		{
			memcpy( pbCanvas, pbDefault, dxC_nMappedRectPitch( tCanvasLocked ) );
			pbCanvas += dxC_nMappedRectPitch( tCanvasLocked );
			continue;
		}
		pbLayerRGB = pbLayerRGBBase + ( (int)( (float)nY * fLayerScale ) * dxC_nMappedRectPitch( tLayerRGBLocked ) );

		// assuming no alpha, for now

		if ( dxC_nMappedRectPitch( tCanvasLocked ) == dxC_nMappedRectPitch( tLayerRGBLocked ) )
			memcpy ( pbCanvas, pbLayerRGB, dxC_nMappedRectPitch( tCanvasLocked ) );
		else {

			// we could do a bilinear scale here
			switch ( pCanvas->nFormat )
			{
			case D3DFMT_DXT1:
				for ( UINT nX = 0; nX < dxC_nMappedRectPitch( tCanvasLocked ); nX += cnDXT1BytesPerPacket )
				{
					// DXT1 is in 8 byte, 4*4=16 pixel chunks
					memcpy( &pbCanvas[ nX ], &pbLayerRGB[ (int)( (float)nX + fLayerScale ) ], cnDXT1BytesPerPacket );
				}
				break;
			DX9_BLOCK( case D3DFMT_DXT2: )
			case D3DFMT_DXT3:
			DX9_BLOCK( case D3DFMT_DXT4: )
			case D3DFMT_DXT5:
				for ( UINT nX = 0; nX < dxC_nMappedRectPitch( tCanvasLocked ); nX += cnDXT2_5BytesPerPacket )
				{
					// DXT5 is in 16 byte, 4*4=16 pixel chunks
					memcpy( &pbCanvas[ nX ], &pbLayerRGB[ (int)( (float)nX + fLayerScale ) ], cnDXT2_5BytesPerPacket );
				}
				break;
			default:
				ASSERTX_CONTINUE( 0, "Unreachable code reached!" );
			}
		}
		pbCanvas += dxC_nMappedRectPitch( tCanvasLocked );
	}

	pr = S_OK;
cleanup:
	FREE( NULL, pbDefault );

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeTextureAllocateBaseCanvas(
	int & rnCanvasTextureID,
	LAYER_TEXTURE_TYPE eLayerTextureTypeRGB,
	DWORD dwTextureFlags )
{
	// make Canvas - the base textures that we layer on top of
	if ( ! e_TextureIsValidID( rnCanvasTextureID ) )
	{
		e_TextureReleaseRef( rnCanvasTextureID );  // just in case

		int nType = gpTextureData[ eLayerTextureTypeRGB ].nTextureType;
		D3D_TEXTURE * pCanvas;
		V_RETURN( dxC_TextureAdd( INVALID_ID, &pCanvas, TEXTURE_GROUP_WARDROBE, nType ) );
		pCanvas->dwFlags = dwTextureFlags;
		rnCanvasTextureID = pCanvas->nId;

		e_TextureAddRef( rnCanvasTextureID );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sCreateCanvasD3DTexture( int nCanvasID, int nWidth, int nHeight, DXC_FORMAT tFormat, TEXTURE_TYPE eType )
{
	V_RETURN( dx9_TextureNewEmptyInPlace(
		nCanvasID,
		nWidth,
		nHeight, 
		0, // create full MIP chain
		tFormat, 
		TEXTURE_GROUP_WARDROBE, 
		eType, 
		D3DC_USAGE_2DTEX ) );
		//D3DC_USAGE_2DTEX_DEFAULT ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeTextureMakeBaseCanvas(
	int nCanvas,
	int nLayer,
	LAYER_TEXTURE_TYPE eTextureType,
	BOOL bOnlyFillInNewTexture,
	int nCanvasWidth /*= -1*/,
	int nCanvasHeight /*= -1*/)
{
	TIMER_STARTEX2("e_WardrobeTextureMakeBaseCanvas",10);

	D3D_TEXTURE * pCanvas   = dxC_TextureGet( nCanvas );
	D3D_TEXTURE * pLayerRGB = dxC_TextureGet( nLayer );
	
	ASSERT_RETINVALIDARG( pCanvas );

	ASSERT_RETINVALIDARG( ! pLayerRGB || pLayerRGB->dwFlags & TEXTURE_FLAG_SYSMEM );
	
	const LAYER_TEXTURE_TYPE_DATA * pTextureTypeData = &gpTextureData[ eTextureType ];
	
	int nMipBonus = 0;

	BOOL bIsNewTexture = FALSE;
	if ( pCanvas->pD3DTexture == NULL )
	{
		ASSERT_RETFAIL( nCanvasWidth > 0 && nCanvasHeight > 0 );
		bIsNewTexture = TRUE;
		pCanvas->nWidthInPixels = nCanvasWidth;
		pCanvas->nHeight		= nCanvasHeight;
		pCanvas->nWidthInBytes = dxC_GetTexturePitch( pCanvas->nWidthInPixels, pTextureTypeData->eFormat );

		TEXTURE_TYPE eType = (TEXTURE_TYPE) pTextureTypeData->nTextureType;

		if ( ! TEST_MASK( pCanvas->dwFlags, TEXTURE_FLAG_SYSMEM ) )
		{
			V( sCreateCanvasD3DTexture(
				pCanvas->nId,
				pCanvas->nWidthInPixels,
				pCanvas->nHeight, 
				pTextureTypeData->eFormat, 
				eType ) );
			ASSERT_RETFAIL( pCanvas->pD3DTexture );
		}
		else
		{
			pCanvas->nFormat = pTextureTypeData->eFormat;
		}

		if ( pCanvas->ptSysmemTexture == NULL )
		{
			V( dxC_TextureSystemMemNewEmptyInPlace( pCanvas ) );
		}
		ASSERT_RETFAIL( pCanvas->ptSysmemTexture );

		if ( TEST_MASK( pCanvas->dwFlags, TEXTURE_FLAG_SYSMEM ) )
			pCanvas->nD3DTopMIPSize = dx9_GetTextureMIPLevelSizeInBytes( pCanvas );		

		if ( pLayerRGB )
		{
			int nCanvasMipLevelCount = dxC_GetNumMipLevels( pCanvas );
			int nLayerMipLevelCount = dxC_GetNumMipLevels( pLayerRGB );
			ASSERT_RETFAIL( nLayerMipLevelCount >= nCanvasMipLevelCount );
			nMipBonus = nLayerMipLevelCount - nCanvasMipLevelCount;

			ASSERT_RETFAIL( pLayerRGB->nD3DTopMIPSize );
			ASSERT_RETFAIL( pCanvas->nD3DTopMIPSize == dx9_GetTextureLODSizeInBytes( pLayerRGB, nMipBonus ) );
		}
	}

	if ( pCanvas->ptSysmemTexture == NULL )
	{
		ASSERT_RETFAIL( pCanvas->nWidthInPixels > 0 && pCanvas->nHeight > 0 );
		V_RETURN( dxC_TextureSystemMemNewEmptyInPlace( pCanvas ) );
		ASSERT_RETFAIL( pCanvas->ptSysmemTexture );
		bIsNewTexture = TRUE;
	}

	if ( bOnlyFillInNewTexture && ! bIsNewTexture )
		return S_OK;

	ASSERT_RETFAIL( pCanvas->nWidthInPixels > 0 );
	ASSERT_RETFAIL( pCanvas->nWidthInBytes  > 0 );
	ASSERT_RETFAIL( pCanvas->nHeight        > 0 );
	ASSERT_RETFAIL( TEST_MASK( pCanvas->dwFlags, TEXTURE_FLAG_SYSMEM ) || pCanvas->pD3DTexture );
	ASSERT_RETFAIL( pCanvas->ptSysmemTexture );

	PRESULT pr = E_FAIL;

	int nLevelCount = dxC_GetNumMipLevels( pCanvas );
	if ( pLayerRGB && (pLayerRGB->pD3DTexture || pLayerRGB->ptSysmemTexture) )
	{
		int nCanvasMipLevelCount = dxC_GetNumMipLevels( pCanvas );
		int nLayerMipLevelCount = dxC_GetNumMipLevels( pLayerRGB );
		ASSERT_RETFAIL( nLayerMipLevelCount >= nCanvasMipLevelCount );
		nMipBonus = nLayerMipLevelCount - nCanvasMipLevelCount;

		ASSERT_RETFAIL( pLayerRGB->nD3DTopMIPSize );
		ASSERT_RETFAIL( pCanvas->nD3DTopMIPSize == dx9_GetTextureLODSizeInBytes( pLayerRGB, nMipBonus ) );
	}
	for ( int nLevel = 0; nLevel < nLevelCount; nLevel++ )
	{
		int nLevelRGB = nLevel + nMipBonus;
		D3DLOCKED_RECT tLayerRGBLocked;
		D3DLOCKED_RECT tCanvasLocked;
		BYTE *pbLayerRGB = NULL;
		D3DC_TEXTURE2D_DESC tLayerRGBDesc;
		if ( pLayerRGB )
		{
			ASSERT_BREAK( pLayerRGB->ptSysmemTexture );
			V_SAVE_GOTO( pr, lvlclean, dxC_MapSystemMemTexture( pLayerRGB->ptSysmemTexture, nLevelRGB, &tLayerRGBLocked ) );
			pbLayerRGB = (BYTE *) dxC_pMappedRectData( tLayerRGBLocked );
			V_SAVE_GOTO( pr, lvlclean, dxC_Get2DTextureDesc( pLayerRGB, nLevelRGB, &tLayerRGBDesc ) );

			TEXTURE_DEFINITION* pLayerDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pLayerRGB->nDefinitionId );
			ASSERT( pLayerDef );
			if ( ( pLayerDef->nFormat != TEXFMT_DXT1 ) 
				|| TEST_MASK( pLayerDef->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT1ALPHA ) )
				SET_MASK( pCanvas->dwFlags, TEXTURE_FLAG_HAS_ALPHA );
		}

		ASSERT_GOTO( pCanvas->ptSysmemTexture, lvlclean );
		V_SAVE_GOTO( pr, lvlclean, dxC_MapSystemMemTexture( pCanvas->ptSysmemTexture, nLevel, &tCanvasLocked ) );
		D3DC_TEXTURE2D_DESC tCanvasDesc;
		V_SAVE_GOTO( pr, lvlclean, dxC_Get2DTextureDesc( pCanvas, nLevel, &tCanvasDesc) );

		BYTE *pbCanvas = (BYTE *) dxC_pMappedRectData( tCanvasLocked );

		BYTE * pbLayerRGBBase = pbLayerRGB;

		unsigned char nDefaultValue = gpTextureData[ eTextureType ].nDefaultCanvas;

		ASSERT ( dx9_IsCompressedTextureFormat( (DXC_FORMAT)pCanvas->nFormat ) )
		{
			V_SAVE_GOTO( pr, lvlclean, sMakeBaseCanvasCompressed( tCanvasLocked, tLayerRGBLocked, pCanvas, 
				pLayerRGB, nLevel, eTextureType, nMipBonus ) );
		} 
		// AE 2007.09.19: We only support compressed texture formats for wardrobe. Disabling un-compressed format copy. 
		//else
		//{
		//	ASSERT_GOTO( !pLayerRGB || dx9_GetTextureFormatBitDepth( (DXC_FORMAT)pLayerRGB->nFormat ) == 32, lvlclean );// the copy function assumes this
		//	ASSERT_GOTO( dx9_GetTextureFormatBitDepth( (DXC_FORMAT)pCanvas->nFormat ) == 32, lvlclean );// the copy function assumes this

		//	float fLayerScale = 0.0f;
		//	if ( pbLayerRGB )
		//		fLayerScale = (float)tLayerRGBDesc.Width / (float)tCanvasDesc.Width;
		//	int nBytesPerPixel = dx9_GetTextureFormatBitDepth( (DXC_FORMAT)pCanvas->nFormat ) / 8;
		//	int nDefaultValueFull = nDefaultValue + (nDefaultValue << 8) + (nDefaultValue << 16) + (nDefaultValue << 24);

		//	for( int nY = 0; nY < (int)tCanvasDesc.Height; nY++, pbCanvas += dxC_nMappedRectPitch( tCanvasLocked ) )
		//	{
		//		if ( ! pLayerRGB )
		//		{
		//			memset ( pbCanvas, nDefaultValueFull, dxC_nMappedRectPitch( tCanvasLocked ));
		//			continue;
		//		}
		//		pbLayerRGB = pbLayerRGBBase + (int)( (float)nY * fLayerScale * (float)dxC_nMappedRectPitch( tLayerRGBLocked ));

		//		if ( dxC_nMappedRectPitch( tCanvasLocked ) == dxC_nMappedRectPitch( tLayerRGBLocked ) )
		//			memcpy ( pbCanvas, pbLayerRGB, dxC_nMappedRectPitch( tCanvasLocked ) );
		//		else {

		//			// we could do a bilinear scale here
		//			for ( int nX = 0; nX < (int)tCanvasDesc.Width; nX++ ) {

		//				if ( nBytesPerPixel == 4 )
		//					((DWORD*)pbCanvas)[ nX ] = ((DWORD*)pbLayerRGB)[ (int)((float)nX * fLayerScale) ];
		//				else
		//					pbCanvas[ nX ] = pbLayerRGB[ (int)((float)nX * fLayerScale) ];
		//			}
		//		}
		//	}
		//}

		pr = S_OK;
lvlclean:
		if ( pLayerRGB )
		{
			V( dxC_UnmapSystemMemTexture( pLayerRGB->ptSysmemTexture, nLevelRGB ) );
		}
		V( dxC_UnmapSystemMemTexture( pCanvas->ptSysmemTexture, nLevel ) );
	}

	TIMER_END2();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_WardrobeTextureCanvasHasAlpha( LAYER_TEXTURE_TYPE eTextureType )
{
	return dx9_TextureFormatHasAlpha( gpTextureData[ eTextureType ].eFormat );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_WardrobeModelDefinitionNewFromMemory( 
	const char * pszFileName, 
	int nLOD,
	int nUseExistingModelDef /*= INVALID_ID*/ )
{
	ASSERT_RETINVALID( pszFileName );
	D3D_MODEL_DEFINITION* pModelDefinition = MALLOCZ_TYPE( D3D_MODEL_DEFINITION, NULL, 1 );
	pModelDefinition->dwFlags |= MODELDEF_FLAG_ANIMATED;
	PStrCopy( pModelDefinition->tHeader.pszName, pszFileName, MAX_XML_STRING_LENGTH );

	int nModelDef = dx9_ModelDefinitionNewFromMemory( pModelDefinition, nLOD, FALSE, nUseExistingModelDef );
	
	// We don't want to track wardrobe models.
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	if ( pHash && pHash->tEngineRes.IsTracked() )
		gtReaper.tModels.Remove( &pHash->tEngineRes );

	return nModelDef;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD e_WardrobeGetBlendTextureFormat()
{
	return (DWORD)gpTextureData[ LAYER_TEXTURE_BLEND ].eFormat;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeLoadBlendTexture(
	int * pnTextureID,
	const char * pszFilePath,
	DWORD dwReloadFlags,
	DWORD dwTextureFlags )
{
	// This is just used to set up some pointers.
	e_WardrobeInit();

	dwReloadFlags  |= TEXTURE_LOAD_FORCECREATETFILE | TEXTURE_LOAD_NO_SAVE;
	dwTextureFlags |= TEXTURE_FLAG_FORCEFORMAT | TEXTURE_FLAG_SYSMEM;

	TEXTURE_DEFINITION tDefOverride;
	ZeroMemory( &tDefOverride, sizeof(tDefOverride) );
	tDefOverride.nMipMapUsed = 0;
	tDefOverride.nFormat = e_GetTextureFormat( e_WardrobeGetBlendTextureFormat() );

	return e_TextureNewFromFile( pnTextureID, pszFilePath, TEXTURE_GROUP_WARDROBE, TEXTURE_NONE, dwTextureFlags, e_WardrobeGetBlendTextureFormat(), NULL, dxC_WardrobeLayerTexturePreSaveBlendTexture, NULL, &tDefOverride, dwReloadFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFillVertexBufferAndMeshByPart( 
	D3D_MESH_DEFINITION * pMesh, 
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, 
	PART_CHART * pPartChart, 
	int nPartCount, 
	int * pnModelList, 
	int nModelCount,
	int nTextureIndex )
{
	// clear the vertex used flags
	for ( int i = 0; i < nModelCount; i++ )
	{
		WARDROBE_MODEL* pModel = (WARDROBE_MODEL*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_WARDROBE_MODEL, pnModelList[i]);
		ASSERT_CONTINUE( pModel );
		WARDROBE_MODEL_ENGINE_DATA* pEngineData = pModel->pEngineData;
		ASSERT_RETFAIL( pEngineData );
		ZeroMemory( pEngineData->pbVertexUsedArray,   sizeof(BOOL) * pEngineData->tVertexBufferDef.nVertexCount );
		ZeroMemory( pEngineData->pbTriangleUsedArray, sizeof(BOOL) * pEngineData->nTriangleCount );
	}

	// flag the vertices that are used and count up triangles
	pMesh->nVertexStart   = 0;
	pMesh->wTriangleCount = 0;
	for ( int nPart = 0; nPart < nPartCount; nPart++ )
	{
		if ( pPartChart[ nPart ].nMaterial != pMesh->nMaterialId )
			continue;
		if ( pPartChart[ nPart ].nTextureIndex != nTextureIndex )
			continue;

		WARDROBE_MODEL* pModel = (WARDROBE_MODEL*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_WARDROBE_MODEL, pPartChart[nPart].nModel);
		ASSERT_CONTINUE( pModel );
		WARDROBE_MODEL_ENGINE_DATA* pEngineData = pModel->pEngineData;
		ASSERT_RETFAIL( pEngineData );
		ASSERT_RETFAIL( pEngineData->pParts );
		const WARDROBE_MODEL_PART * pModelParts = pEngineData->pParts;
		int nPartIndexInModel = pPartChart[ nPart ].nIndexInModel;
		ASSERT_RETFAIL( nPartIndexInModel < pEngineData->nPartCount );
		for( int i = 0; i < pModelParts[ nPartIndexInModel ].nTriangleCount; i++ )
		{
			int nTriangle = pModelParts[ nPartIndexInModel ].nTriangleStart + i;
			if ( pEngineData->pbTriangleUsedArray[ nTriangle ] )
				continue;
			pMesh->wTriangleCount++;
			pEngineData->pbTriangleUsedArray[ nTriangle ] = TRUE;
			for ( int j = 0; j < 3; j++ )
			{
				int nVertex = pEngineData->pIndexBuffer[ 3 * nTriangle + j ];
				pEngineData->pbVertexUsedArray[ nVertex ] = TRUE;
			}
		}
	}
	pMesh->tIndexBufferDef.nBufferSize = pMesh->wTriangleCount * 3 * sizeof( INDEX_BUFFER_TYPE );
	pMesh->tIndexBufferDef.pLocalData = (void *) REALLOC( NULL, pMesh->tIndexBufferDef.pLocalData, pMesh->tIndexBufferDef.nBufferSize );

	int nTriangleCountCurrent = 0;
	// add models to vertex buffer and index buffer
	for ( int i = 0; i < nModelCount; i++ )
	{
		// map old indices to new indices
		WARDROBE_MODEL * pModel = (WARDROBE_MODEL*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_WARDROBE_MODEL, pnModelList[i]);
		ASSERT_CONTINUE( pModel );
		WARDROBE_MODEL_ENGINE_DATA* pEngineData = pModel->pEngineData;
		
		int nNewVertexIndex = pVertexBufferDef->nVertexCount;
		for ( int nVertex = 0; nVertex < pEngineData->tVertexBufferDef.nVertexCount; nVertex++ )
		{
			if ( pEngineData->pbVertexUsedArray[ nVertex ] )
			{
				pEngineData->pIndexMapping[ nVertex ] = nNewVertexIndex;
				nNewVertexIndex++;
			}
		}

		// grow vertex buffer
		int nLastVertex = pVertexBufferDef->nVertexCount;
		pVertexBufferDef->nVertexCount = nNewVertexIndex;

		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
		{						
			pVertexBufferDef->nBufferSize[ nStream ] = dxC_GetVertexSize( nStream, pVertexBufferDef ) * pVertexBufferDef->nVertexCount;
			pVertexBufferDef->pLocalData[ nStream ] = (void*)REALLOC( NULL, pVertexBufferDef->pLocalData[ nStream ], pVertexBufferDef->nBufferSize[ nStream ] );

			// copy vertices
			int nCurrentVertex = nLastVertex;

			ASSERT( dxC_GetVertexSize( nStream, pVertexBufferDef ) == dxC_GetVertexSize( nStream, &pEngineData->tVertexBufferDef ) );
			int nVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
			for ( int nVertex = 0; nVertex < pEngineData->tVertexBufferDef.nVertexCount; nVertex++ )
			{
				if ( pEngineData->pbVertexUsedArray[ nVertex ] )
				{
					memcpy( (BYTE*)pVertexBufferDef->pLocalData[ nStream ]				+ nCurrentVertex	* nVertexSize,
							(BYTE*)pEngineData->tVertexBufferDef.pLocalData[ nStream ]	+ nVertex			* nVertexSize,
							nVertexSize );
					nCurrentVertex++;
				}
			}
		}

		// copy indices
		for ( int nTriangle = 0; nTriangle < pEngineData->nTriangleCount; nTriangle++ )
		{
			if ( pEngineData->pbTriangleUsedArray[ nTriangle ] )
			{
				for ( int i = 0; i < 3; i++ )
				{
					INDEX_BUFFER_TYPE * pwIndex = (INDEX_BUFFER_TYPE*)pMesh->tIndexBufferDef.pLocalData + 3 * nTriangleCountCurrent + i;
					*pwIndex = *(pEngineData->pIndexBuffer + 3 * nTriangle + i);
					*pwIndex = pEngineData->pIndexMapping[ *pwIndex ];
				}
				nTriangleCountCurrent++;
			}
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeSetTexture( 
	int nModelDefinitionId,
	int nLOD,
	int nTexture,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int nWardrobeTextureIndex )
{
	if ( nModelDefinitionId == INVALID_ID )
		return S_FALSE;

	D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( !pModelDef )
		return S_FALSE;

	TEXTURE_TYPE eTextureType = (TEXTURE_TYPE)gpTextureData[ eLayerTextureType ].nTextureType;

	if ( nTexture != INVALID_ID )
	{
		ASSERT( e_TextureIsLoaded( nTexture ) );
	}

	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMeshDef = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
		ASSERT_CONTINUE( pMeshDef );

		if ( pMeshDef->nWardrobeTextureIndex == nWardrobeTextureIndex )
		{
			e_TextureReleaseRef( pMeshDef->pnTextures[ eTextureType ] );
			pMeshDef->pnTextures[ eTextureType ] = nTexture;
			e_TextureAddRef( pMeshDef->pnTextures[ eTextureType ] );
			if ( eTextureType == TEXTURE_DIFFUSE )
			{
				// force it to re-apply to models
				pMeshDef->dwFlags &= ~MESH_FLAG_MATERIAL_APPLIED;
				V( dx9_MeshUpdateMaterialVersion( pMeshDef ) );
			}
		}
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeAddLayerModelMeshes( 
	int nModelDefinitionId,
	int nLOD,
	int ppnTexture[MAX_WARDROBE_TEXTURE_GROUPS][ NUM_TEXTURES_PER_SET ],
	int * pnWardrobeTextureIndex,
	int * pnWardrobeMaterials,
	int nWardrobeMaterialCount,
	int * pnWardrobeModelList,
	int nWardrobeModelCount,
	WARDROBE_MODEL * pWardrobeModelBase,
	PART_CHART * pPartChart,
	int nPartCount,
	BOOL bUsesColorMask )
{
	TIMER_STARTEX2("e_WardrobeAddLayerModelMeshes",10);

	ASSERT_RETINVALIDARG( ppnTexture );
	ASSERT_RETINVALIDARG( pnWardrobeTextureIndex );
	ASSERT_RETINVALIDARG( pnWardrobeMaterials );
	ASSERT_RETINVALIDARG( pnWardrobeModelList );
	ASSERT_RETINVALIDARG( pWardrobeModelBase );
	ASSERT_RETINVALIDARG( pPartChart );

	// create a template mesh to be used by all of the meshes
	D3D_MESH_DEFINITION pMeshTemplates[ MAX_WARDROBE_TEXTURE_GROUPS ];
	for ( int ii = 0; ii < MAX_WARDROBE_TEXTURE_GROUPS; ii++ )
	{
		D3D_MESH_DEFINITION * pTemplate = &pMeshTemplates[ ii ];
		ZeroMemory( pTemplate, sizeof( D3D_MESH_DEFINITION ) );
		pTemplate->ePrimitiveType = D3DPT_TRIANGLELIST;
		pTemplate->nBoneToAttachTo = INVALID_ID;
		dxC_IndexBufferDefinitionInit( pTemplate->tIndexBufferDef );
		pTemplate->pnTextures[ TEXTURE_DIFFUSE   ]	= ppnTexture[ ii ][ LAYER_TEXTURE_DIFFUSE ];
		pTemplate->pnTextures[ TEXTURE_DIFFUSE2  ]	= INVALID_ID; // ppnTexture[ 1  ][ LAYER_TEXTURE_DIFFUSE  ];// yes, there is a 1 here instead of an ii
		pTemplate->pnTextures[ TEXTURE_SELFILLUM ]	= ppnTexture[ ii ][ LAYER_TEXTURE_SELFILLUM ];
		pTemplate->pnTextures[ TEXTURE_NORMAL    ]	= ppnTexture[ ii ][ LAYER_TEXTURE_NORMAL    ];
		pTemplate->pnTextures[ TEXTURE_SPECULAR  ]	= ppnTexture[ ii ][ LAYER_TEXTURE_SPECULAR  ];
		pTemplate->pnTextures[ TEXTURE_ENVMAP    ]	= INVALID_ID;
		pTemplate->pnTextures[ TEXTURE_LIGHTMAP  ]	= INVALID_ID;
		//pTemplate->pnTextures[ TEXTURE_HEIGHT	 ]	= INVALID_ID; // AE TODO: support heightmaps
		pTemplate->dwFlags |= MESH_FLAG_SKINNED;
		//pTemplate->nDiffuse2AddressMode[ 0 ] = MESH_TEXTURE_WRAP;
		//pTemplate->nDiffuse2AddressMode[ 1 ] = MESH_TEXTURE_WRAP;
	}

	D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	ASSERT_RETFAIL( pModelDefinition );

	V( dx9_ModelClearPerMeshDataForDefinition( pModelDefinition->tHeader.nId ) );

	// for each draw group add a new mesh
	D3D_MESH_DEFINITION pMeshes[ MAX_WARDROBE_MATERIAL_TEXTURE_PAIRS ];
	for ( int nMaterialIndex = 0; nMaterialIndex < nWardrobeMaterialCount; nMaterialIndex++ )
	{
		D3D_MESH_DEFINITION * pMesh = &pMeshes[ nMaterialIndex ];
		int nTextureIndex = pnWardrobeTextureIndex[ nMaterialIndex ];

		ASSERT_RETFAIL( nTextureIndex >= 0 && nTextureIndex < MAX_WARDROBE_TEXTURE_GROUPS );
		// create mesh definition
		memcpy( pMesh, &pMeshTemplates[ nTextureIndex ], sizeof( D3D_MESH_DEFINITION ) );
		// this is a copy -- don't addref textures here
		pMesh->nMaterialId = pnWardrobeMaterials[ nMaterialIndex ];
		pMesh->nWardrobeTextureIndex = pnWardrobeTextureIndex[ nMaterialIndex ];
		MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, pMesh->nMaterialId );
		V( dx9_MeshApplyMaterial( pMesh, pModelDefinition, NULL, nLOD, -1 ) );

		// create vertex buffer
		ASSERT_CONTINUE( pWardrobeModelBase->pEngineData );
		int nVertexBufferIndex = 0;
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( pModelDefinition, &pMesh->nVertexBufferIndex );
		WARDROBE_MODEL_ENGINE_DATA* pEngineData = pWardrobeModelBase->pEngineData;
		ASSERT( pEngineData->tVertexBufferDef.nD3DBufferID[ 0 ] == INVALID_ID 
			 && pEngineData->tVertexBufferDef.nD3DBufferID[ 1 ] == INVALID_ID );
		memcpy ( pVertexBufferDef, &pEngineData->tVertexBufferDef, sizeof( D3D_VERTEX_BUFFER_DEFINITION ) );

		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
			pVertexBufferDef->pLocalData[ nStream ] = NULL;
		pVertexBufferDef->nVertexCount = 0;

		V( sFillVertexBufferAndMeshByPart( pMesh, pVertexBufferDef, pPartChart, nPartCount, pnWardrobeModelList, nWardrobeModelCount, pMesh->nWardrobeTextureIndex ) );
	}


	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition = dx9_AnimatedModelDefinitionGet( nModelDefinitionId, nLOD );

	// we have to do this to each mesh after all of the meshes have been added
	for ( int nMaterialIndex = 0; nMaterialIndex < nWardrobeMaterialCount; nMaterialIndex++ )
	{
		D3D_MESH_DEFINITION * pMesh = &pMeshes[ nMaterialIndex ];
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

		dx9_AnimatedModelDefinitionUpdateSkinnedBoneMask ( pAnimatingModelDefinition, pVertexBufferDef );

		dx9_AnimatedModelDefinitionSplitMeshByBones( pModelDefinition, 
			pAnimatingModelDefinition, pMesh );
	}


	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
		ASSERT_CONTINUE( pMesh );
		D3D_TEXTURE * pTexture = dxC_TextureGet( pMesh->pnTextures[ TEXTURE_DIFFUSE ] );
		if ( pTexture && pTexture->tRefCount.GetCount() == 0 )
		{
			int a = 0;
		}
	}


	V_RETURN( e_ModelDefinitionRestore ( nModelDefinitionId, nLOD ) );

	TIMER_END2();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeModelResourceFree(
	WARDROBE_MODEL* pModel )
{
	ASSERT_RETINVALIDARG( pModel );
	WARDROBE_MODEL_ENGINE_DATA* pEngineData = pModel->pEngineData;
	if ( !pEngineData )
		return S_FALSE;

	if ( pEngineData->pbVertexUsedArray )
		FREE( NULL, pEngineData->pbVertexUsedArray );
	if ( pEngineData->pbTriangleUsedArray )
		FREE( NULL, pEngineData->pbTriangleUsedArray );
	if ( pEngineData->pIndexMapping )
		FREE( NULL, pEngineData->pIndexMapping );
	if ( pEngineData->pIndexBuffer )
		FREE( NULL, pEngineData->pIndexBuffer );
	if ( pEngineData->pParts )
		FREE( NULL, pEngineData->pParts );

	int nStreamCount = dxC_GetStreamCount( &pEngineData->tVertexBufferDef );
	for ( int nStream = 0; nStream < nStreamCount; nStream++ )
	{
		FREE( NULL, pEngineData->tVertexBufferDef.pLocalData[ nStream ] );
	}

	FREE( NULL, pEngineData );
	pModel->pEngineData = NULL;
	pModel->bAsyncLoadRequested = FALSE;
	
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT sWardrobeModelPartXfer( 	
	BYTE_XFER<mode> & tXfer,
	WARDROBE_MODEL_PART& tPart )
{
	BYTE_XFER_ELEMENT( tXfer, tPart.nPartId );
	BYTE_XFER_ELEMENT( tXfer, tPart.nTriangleStart );
	BYTE_XFER_ELEMENT( tXfer, tPart.nTriangleCount );
	BYTE_XFER_ELEMENT( tXfer, tPart.nMaterial );
	BYTE_XFER_ELEMENT( tXfer, tPart.nTextureIndex );

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT sWardrobeModelEngineDataXfer( 	
	BYTE_XFER<mode>& tXfer,
	WARDROBE_MODEL_ENGINE_DATA& tEngineData )
{
	BYTE_XFER_ELEMENT( tXfer, tEngineData.nTriangleCount );
	BYTE_XFER_ELEMENT( tXfer, tEngineData.nIndexBufferSize );

	if ( tXfer.IsLoad() && tEngineData.nIndexBufferSize > 0 )
		tEngineData.pIndexBuffer = (INDEX_BUFFER_TYPE*)MALLOC( NULL, 
			tEngineData.nIndexBufferSize );
	if ( tEngineData.nIndexBufferSize > 0 )
		BYTE_XFER_POINTER( tXfer, tEngineData.pIndexBuffer,
			tEngineData.nIndexBufferSize );

	BYTE_XFER_ELEMENT( tXfer, tEngineData.nBoneCount );
	BYTE_XFER_ELEMENT( tXfer, tEngineData.nPartCount );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_WardrobeModelFileUpdate(
	WARDROBE_MODEL * pWardrobeModel,
	int nLOD, 
	const char * pszAppearancePath,
	const char * pszSkeletonFilename,
	MATRIX * pmInverseSkinTransform,
#ifdef HAVOK_ENABLED
	hkSkeleton * pHavokSkeleton,
#endif
	granny_skeleton * pSkeletonGranny,
	BOOL bForceUpdate )
{
	char* pszFileName = pWardrobeModel->pszFileName;

	char LowFileName[DEFAULT_FILE_WITH_PATH_SIZE];

	if ( nLOD == LOD_LOW )
	{
		e_ModelDefinitionGetLODFileName(LowFileName, ARRAYSIZE(LowFileName), pszFileName );
		pszFileName = LowFileName;
	}

	char pszWardrobeModelFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszWardrobeModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName, WARDROBE_MODEL_FILE_EXTENSION);

	// if the part list changes, we have to update the file
	char pszWardrobeModelPartTxt[DEFAULT_FILE_WITH_PATH_SIZE];
	const char *pszFilePath = ExcelTableGetFilePath( DATATABLE_WARDROBE_PART );
	PStrPrintf(pszWardrobeModelPartTxt, DEFAULT_FILE_WITH_PATH_SIZE, "%s", pszFilePath);

	char pszSkeletonFullFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(pszSkeletonFullFilename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszAppearancePath, pszSkeletonFilename );

	// check and see if we need to update at all
#ifndef FORCE_REMAKE_WARDROBE_MODEL_FILES
	if (bForceUpdate)
	{
		trace("bForceUpdate == TRUE  FILE: %s\n", pszWardrobeModelFileName);
	}
	if ( ! AppCommonAllowAssetUpdate() )
	{
		return S_FALSE;
	}
	else if (FileNeedsUpdate(pszWardrobeModelFileName, pszFileName, 
		WARDROBE_MODEL_FILE_MAGIC_NUMBER, WARDROBE_MODEL_FILE_VERSION, 
		FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG, 0, FALSE,
		MIN_REQUIRED_WARDROBE_MODEL_FILE_VERSION))
	{
		trace("Model Needs Update  FILE: %s  FILE: %s\n", pszWardrobeModelFileName, pszFileName);
	}
	else if (FileNeedsUpdate(pszWardrobeModelFileName, pszSkeletonFullFilename))
	{
		trace("Skeleton Needs Update  FILE: %s  FILE: %s\n", pszWardrobeModelFileName, pszSkeletonFullFilename);
	}
	else if (FileNeedsUpdate(pszWardrobeModelFileName, pszWardrobeModelPartTxt))
	{
		trace("Skeleton Needs Update  FILE: %s  FILE: %s\n", pszWardrobeModelFileName, pszWardrobeModelPartTxt);
	}
	else
	{
		return S_FALSE;
	}
#endif

	TIMER_START(pszFileName);

	granny_file * pGrannyFile = e_GrannyReadFile( pszFileName );
	if ( ! pGrannyFile ) 
	{
		if ( nLOD != LOD_HIGH )
			// We don't want to throw error dialogs for missing low detail files.
			return S_FALSE;
	}
	ASSERTV_RETFAIL( pGrannyFile, "Could not find file %s", pszFileName);

	granny_file_info * pFileInfo =	GrannyGetFileInfo( pGrannyFile );
	ASSERT_RETFAIL( pFileInfo );
	BOOL bMax9MaterialNames = e_IsMax9Exporter( pFileInfo->ExporterInfo );

	// -- Coordinate System --
	// Tell Granny to transform the file into our coordinate system
	e_GrannyConvertCoordinateSystem ( pFileInfo );

	// -- The Model ---
	// Right now we assumes that there is only one model in the file.
	ASSERTV_RETFAIL( pFileInfo->ModelCount == 1, "Too many models in %s.  Model count is %d", pszFileName, pFileInfo->MaterialCount);
	granny_model * pGrannyModel = pFileInfo->Models[0]; 
	ASSERTX_RETFAIL( pGrannyModel, pszWardrobeModelFileName );

	WARDROBE_MODEL_ENGINE_DATA tEngineData;
	structclear( tEngineData );

	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION> tSerializedFileSections;
	ArrayInit(tSerializedFileSections, NULL, DEFAULT_SERIALIZED_FILE_SECTION_COUNT);
	BYTE_BUF tBuf( (MEMORYPOOL*)NULL, 1024 * 1024 ); // 1MB
	BYTE_XFER<XFER_SAVE> tXfer(&tBuf);

#ifdef HAVOK_ENABLED
	int pnBoneMappingGrannyToHavok[ MAX_BONES_PER_MODEL ];
	if( pHavokSkeleton )
	{
		pSkeletonGranny = pGrannyModel->Skeleton;

		tEngineData.nBoneCount = pHavokSkeleton ? pHavokSkeleton->m_numBones : 0;
		
		e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping( pSkeletonGranny, pHavokSkeleton, 
																pnBoneMappingGrannyToHavok, pszFileName );
	}
	else 
#endif
	if( pSkeletonGranny )
	{
		tEngineData.nBoneCount = pSkeletonGranny->BoneCount;
	}

	// find the mesh that has the material bindings
	granny_mesh * pMesh = NULL;
	int nAnimatingMeshIndex = 0;
	for ( int nGrannyMeshIndex = 0; nGrannyMeshIndex < pGrannyModel->MeshBindingCount; ++nGrannyMeshIndex )
	{
		if ( pGrannyModel->MeshBindings[ nGrannyMeshIndex ].Mesh->MaterialBindings == NULL )
			continue;

		pMesh = pGrannyModel->MeshBindings[ nGrannyMeshIndex ].Mesh;
		break;
	}

	ASSERTV_RETFAIL( pMesh, "Error: Granny file has no material bindings!\n\n%s", pszFileName );

	// -- Index buffer --
	tEngineData.nTriangleCount = GrannyGetMeshIndexCount ( pMesh ) / 3;
	tEngineData.nIndexBufferSize = 3 * tEngineData.nTriangleCount * sizeof ( INDEX_BUFFER_TYPE );
	INDEX_BUFFER_TYPE * pwMeshIndexBuffer = NULL;
	if ( tEngineData.nTriangleCount )
	{
		tEngineData.pIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOC( NULL, tEngineData.nIndexBufferSize );
		GrannyCopyMeshIndices ( pMesh, sizeof ( INDEX_BUFFER_TYPE ), (BYTE *) tEngineData.pIndexBuffer );
	}

	// -- Vertex Buffer --
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &tEngineData.tVertexBufferDef;
	dxC_VertexBufferDefinitionInit( *pVertexBufferDef );

#ifdef HAVOK_ENABLED
	if( pHavokSkeleton )
	{
		V_RETURN( dx9_GetAnimatedVertexBuffer ( pMesh, pVertexBufferDef, pSkeletonGranny, 
									pnBoneMappingGrannyToHavok,
									pmInverseSkinTransform ) );
	}
	else 
#endif
	if( pSkeletonGranny )
	{
		V_RETURN( dx9_GetAnimatedVertexBuffer ( pMesh, pVertexBufferDef, pSkeletonGranny, 
									NULL,
									pmInverseSkinTransform ) );
	}

	// -- Parts --
	int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount( pMesh );
	granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pMesh );
	int nPartDefCount = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_WARDROBE_PART );
	for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
	{
		// Get the group
		granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

		granny_material * pMaterial = pMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;
		if ( ! pMaterial )
			continue;

		BOOL bMaterialMatched = FALSE;

		int nPartId = INVALID_ID;
		int nGrannyMaterialIndex = pGrannyTriangleGroup->MaterialIndex;
		for ( int i = 0; i < nPartDefCount; i++ )
		{
			const WARDROBE_MODEL_PART_DATA * pPartDef = (const WARDROBE_MODEL_PART_DATA *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_WARDROBE_PART, i);
			ASSERT_CONTINUE( pPartDef );
			if ( pWardrobeModel->nPartGroup != INVALID_ID )
			{
				if ( pPartDef->nPartGroup == pWardrobeModel->nPartGroup )
				{
					// Handle this with material indexes if using a GR2 file from the old exporter, and names if using a newer exporter
					if ( bMax9MaterialNames && ( 0 == PStrICmp( pMaterial->Name, pPartDef->pszMaterialName, DEFAULT_INDEX_SIZE ) ) )
						bMaterialMatched = TRUE;
					else if ( ! bMax9MaterialNames && ( pPartDef->nMaterialIndex == nGrannyMaterialIndex ) )
						bMaterialMatched = TRUE;

					if ( bMaterialMatched )
					{
						nPartId = i;
						break;
					}
				}
			} else {
				if ( PStrICmp( pMaterial->Name, pPartDef->pszLabel, WARDROBE_LABEL_LENGTH ) == 0 )
				{
					bMaterialMatched = TRUE;
					nPartId = i;
					break;
				}
			}
		}

#ifdef _DEBUG
		GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
		if ( pGlobals->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
		{
			WARNX( nPartId != INVALID_ID, pMaterial->Name );
		}

		if ( bMax9MaterialNames )
		{
			ASSERTV( bMaterialMatched, "Unknown wardrobe material name!\nMaterial: %s\nFilename: %s", pMaterial->Name, pszFileName );
		}
#endif

		tEngineData.nPartCount++;
		tEngineData.pParts = (WARDROBE_MODEL_PART *)REALLOCZ( NULL, tEngineData.pParts, sizeof( WARDROBE_MODEL_PART ) * tEngineData.nPartCount );
		WARDROBE_MODEL_PART * pPart = &tEngineData.pParts[ tEngineData.nPartCount - 1 ];

		pPart->nPartId = nPartId;
		pPart->nTriangleStart = pGrannyTriangleGroup->TriFirst;
		pPart->nTriangleCount = pGrannyTriangleGroup->TriCount;
		pPart->nMaterial = INVALID_ID;
	}

	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_VERTEX_BUFFER_DEFINITION;			
		pSection->dwOffset = tXfer.GetCursor();

		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		dxC_VertexBufferDefinitionXfer( tXfer, tEngineData.tVertexBufferDef, nStreamCount );

		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		for ( int i = 0; i < nStreamCount; i++ )
		{
			FREE ( NULL, pVertexBufferDef->pLocalData[ i ] );
			pVertexBufferDef->pLocalData[ i ] = NULL;
		}
	}

	if ( tEngineData.pParts )
	{
		ASSERT_RETFAIL( tEngineData.nPartCount );	// doesn't this leak pParts and hFile?

		for ( int nPart = 0; nPart < tEngineData.nPartCount; nPart++ )
		{
			WARDROBE_MODEL_PART& tPart = tEngineData.pParts[ nPart ];

			SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
			pSection->nSectionType = MODEL_FILE_SECTION_WARDROBE_MODEL_PART;
			pSection->dwOffset = tXfer.GetCursor();
			sWardrobeModelPartXfer( tXfer, tPart );
			pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
		}

		FREE( NULL, tEngineData.pParts );
		tEngineData.pParts = NULL;
	}

	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_WARDROBE_MODEL_ENGINE_DATA;
		pSection->dwOffset = tXfer.GetCursor();
		sWardrobeModelEngineDataXfer( tXfer, tEngineData );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		// save to file
		TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
		FileGetFullFileName(szFullname, pszWardrobeModelFileName, DEFAULT_FILE_WITH_PATH_SIZE);

		HANDLE hFile = CreateFile( szFullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == INVALID_HANDLE_VALUE )
		{
			if ( DataFileCheck( szFullname ) )
				hFile = CreateFile( szFullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		}		
		ASSERTX_RETFAIL( hFile != INVALID_HANDLE_VALUE, szFullname );

		BYTE_BUF tHeaderBuf( (MEMORYPOOL*)NULL, sizeof( SERIALIZED_FILE_HEADER ) + 
			sizeof( SERIALIZED_FILE_SECTION ) * DEFAULT_SERIALIZED_FILE_SECTION_COUNT );
		BYTE_XFER<XFER_SAVE> tHeaderXfer(&tHeaderBuf);

		// SERIALIZED_FILE_HEADER inherits from FILE_HEADER
		SERIALIZED_FILE_HEADER tFileHeader;
		tFileHeader.dwMagicNumber	= WARDROBE_MODEL_FILE_MAGIC_NUMBER;
		tFileHeader.nVersion		= WARDROBE_MODEL_FILE_VERSION;
		tFileHeader.nSFHVersion		= SERIALIZED_FILE_HEADER_VERSION;
		tFileHeader.nNumSections	= tSerializedFileSections.Count();
		tFileHeader.pSections		= tSerializedFileSections.GetPointer( 0 );

		dxC_SerializedFileHeaderXfer( tHeaderXfer, tFileHeader );

		for ( int nSection = 0; nSection < tFileHeader.nNumSections; nSection++ )
		{
			dxC_SerializedFileSectionXfer( tHeaderXfer, 
				tFileHeader.pSections[ nSection ], tFileHeader.nSFHVersion );
		}

		DWORD dwBytesWritten;
		// Write out the header
		UINT nHeaderLen = tHeaderXfer.GetLen();
		tHeaderXfer.SetCursor( 0 );
		WriteFile( hFile, tHeaderXfer.GetBufferPointer(), nHeaderLen, &dwBytesWritten, NULL );

		// Now, write out the file data
		UINT nDataLen = tXfer.GetLen();
		tXfer.SetCursor( 0 );
		WriteFile( hFile, tXfer.GetBufferPointer(), nDataLen, &dwBytesWritten, NULL );

		CloseHandle( hFile );
	}

	FREE( NULL, tEngineData.pIndexBuffer );

	GrannyFreeFile( pGrannyFile );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct WARDROBE_MODEL_CALLBACK_DATA
{
	int nWardrobeModel;
	BOOL bForceSync;
};
//----------------------------------------------------------------------------
static PRESULT sWardrobeFileLoadCallback( 
	ASYNC_DATA & data)
{
	TIMER_STARTEX2("sWardrobeFileLoadCallback",10);

	PAKFILE_LOAD_SPEC* spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	// get callback data
	WARDROBE_MODEL_CALLBACK_DATA* pCallbackData = (WARDROBE_MODEL_CALLBACK_DATA*)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);

	int nWardrobeModel = pCallbackData->nWardrobeModel;	
	
	void* pData = spec->buffer;
	WARDROBE_MODEL* pWardrobeModel = (WARDROBE_MODEL*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_WARDROBE_MODEL, nWardrobeModel);
	ASSERT_RETFAIL( pWardrobeModel );
	int nLOD = pWardrobeModel->nLOD;

	pWardrobeModel->bAsyncLoadRequested = FALSE;
	if (data.m_IOSize == 0 || !pData) 
	{
		if ( c_GetToolMode() 
		  && e_GetRenderFlag( RENDER_FLAG_WARDROBE_WARN ) 
		  && nLOD == LOD_LOW )
		{
			e_WardrobeModelLoad( pWardrobeModel, LOD_HIGH_AS_LOW );
			return S_OK;
		}
		else
		{
			pWardrobeModel->bLoadFailed[ nLOD ] = TRUE;

			//if ( c_GetToolMode() )
			//{
			//	LogText( ERROR_LOG, "Missing wardrobe file for LOD %d: '%s'\n", nLOD, spec->fixedfilename );
			//}
		}

		return S_FALSE;
	}

	if ( pWardrobeModel->pEngineData )
		e_WardrobeModelResourceFree( pWardrobeModel );

	BYTE* pbFileStart = (BYTE*) pData;

	ASSERTX_RETFAIL( pbFileStart, spec->filename );
	ASSERTX_RETFAIL( spec->bytesread >= sizeof(FILE_HEADER), spec->filename );

	SERIALIZED_FILE_HEADER tFileHeader;
	BYTE_BUF tHeaderBuf( pbFileStart, spec->bytesread );
	BYTE_XFER<XFER_LOAD> tHeaderXfer(&tHeaderBuf);
	dxC_SerializedFileHeaderXfer( tHeaderXfer, tFileHeader );

	ASSERTX_RETFAIL( tFileHeader.dwMagicNumber == WARDROBE_MODEL_FILE_MAGIC_NUMBER, spec->filename );
	ASSERTX_RETFAIL( tFileHeader.nVersion >= MIN_REQUIRED_WARDROBE_MODEL_FILE_VERSION, spec->filename );
	ASSERTX_RETFAIL( tFileHeader.nVersion <= WARDROBE_MODEL_FILE_VERSION, spec->filename );
	ASSERTX_RETFAIL( tFileHeader.nNumSections > 0, spec->filename );

	tFileHeader.pSections = MALLOC_TYPE( SERIALIZED_FILE_SECTION, NULL, 
		tFileHeader.nNumSections );

	for ( int nSection = 0; nSection < tFileHeader.nNumSections; nSection++ )
		dxC_SerializedFileSectionXfer( tHeaderXfer, tFileHeader.pSections[ nSection ], 
			tFileHeader.nSFHVersion );

	BYTE* pbFileDataStart = pbFileStart + tHeaderXfer.GetCursor();
	BYTE_BUF tFileBuf( pbFileDataStart, spec->bytesread - (DWORD)(pbFileDataStart - pbFileStart) );
	BYTE_XFER<XFER_LOAD> tXfer(&tFileBuf);

	// The animating model definition should be stored one prior to the end.
	SERIALIZED_FILE_SECTION* pSection = &tFileHeader.pSections[ tFileHeader.nNumSections - 1 ];
	ASSERTX_RETFAIL( pSection->nSectionType == MODEL_FILE_SECTION_WARDROBE_MODEL_ENGINE_DATA, spec->filename );
	tXfer.SetCursor( pSection->dwOffset );

	pWardrobeModel->pEngineData = MALLOCZ_TYPE( WARDROBE_MODEL_ENGINE_DATA, NULL, 1 );
	ASSERTX_RETFAIL( pWardrobeModel->pEngineData, spec->filename );
	sWardrobeModelEngineDataXfer( tXfer, *pWardrobeModel->pEngineData );

	WARDROBE_MODEL_ENGINE_DATA& tEngineData = *pWardrobeModel->pEngineData;

	//ASSERT_RETFAIL( (size_t)data.m_IOSize > (size_t)pFile->tEngineData.pParts );
	//ASSERT_RETFAIL( (size_t)data.m_IOSize > (size_t)pFile->tEngineData.pIndexBuffer );
	//ASSERT_RETFAIL( (size_t)data.m_IOSize > (size_t)pFile->tEngineData.tVertexBufferDef.pLocalData[ 0 ] + (size_t)pFile->tEngineData.tVertexBufferDef.pLocalData[ 1 ] );

	int nVertexBufferDefsXferred = 0;

	int nWardrobeModelPartsXferred = 0;
	if ( tEngineData.nPartCount > 0 )
		tEngineData.pParts = MALLOCZ_TYPE( WARDROBE_MODEL_PART, NULL, 
			tEngineData.nPartCount );

	for ( int nSection = 0; nSection < tFileHeader.nNumSections; nSection++ )
	{
		SERIALIZED_FILE_SECTION* pSection = &tFileHeader.pSections[ nSection ];
		tXfer.SetCursor( pSection->dwOffset );
		switch ( pSection->nSectionType )
		{
		case MODEL_FILE_SECTION_VERTEX_BUFFER_DEFINITION:
			{
				ASSERTX_RETFAIL( nVertexBufferDefsXferred == 0, spec->filename );
				int nStreamCount = 0;
				dxC_VertexBufferDefinitionXfer( tXfer, 
					tEngineData.tVertexBufferDef,
					nStreamCount );
				nVertexBufferDefsXferred++;
			}
			break;

		case MODEL_FILE_SECTION_WARDROBE_MODEL_PART:
			{
				sWardrobeModelPartXfer( tXfer, 
					tEngineData.pParts[ nWardrobeModelPartsXferred ] );
				nWardrobeModelPartsXferred++;
			}
			break;
		}
	}

	FREE( NULL, tFileHeader.pSections );

	pWardrobeModel->pEngineData->pbVertexUsedArray   = MALLOCZ_TYPE( BOOL, NULL, pWardrobeModel->pEngineData->tVertexBufferDef.nVertexCount );
	pWardrobeModel->pEngineData->pbTriangleUsedArray = MALLOCZ_TYPE( BOOL, NULL, pWardrobeModel->pEngineData->nTriangleCount );
	pWardrobeModel->pEngineData->pIndexMapping	   = MALLOCZ_TYPE( INDEX_BUFFER_TYPE, NULL, pWardrobeModel->pEngineData->tVertexBufferDef.nVertexCount );
	pWardrobeModel->pEngineData->tVertexBufferDef.eVertexType = VERTEX_DECL_ANIMATED;

	TIMER_END2();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_WardrobeModelLoad(
	WARDROBE_MODEL* pWardrobeModel,
	int nLOD,
	BOOL bForceSync /*= FALSE*/ )
{
	TIMER_STARTEX2("e_WardrobeModelLoad",10);

	char* pszFileName = pWardrobeModel->pszFileName;
	char pszWardrobeModelFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	DWORD dwPakFlags = 0;

	char szLowFileName[DEFAULT_FILE_WITH_PATH_SIZE];

	if ( nLOD == LOD_LOW )
	{
		e_ModelDefinitionGetLODFileName(szLowFileName, ARRAYSIZE(szLowFileName), pszFileName );
		pszFileName = szLowFileName;
		if ( c_GetToolMode() && e_GetRenderFlag( RENDER_FLAG_WARDROBE_WARN ) )
			dwPakFlags |= PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING;
		else
			dwPakFlags |= PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;
	}
	else if ( nLOD == LOD_HIGH_AS_LOW )
		nLOD = LOD_LOW;

	PStrReplaceExtension(pszWardrobeModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName, WARDROBE_MODEL_FILE_EXTENSION);

	WARDROBE_MODEL_CALLBACK_DATA* pCallbackData = (WARDROBE_MODEL_CALLBACK_DATA*) MALLOC( NULL, sizeof(WARDROBE_MODEL_CALLBACK_DATA) );
	pCallbackData->nWardrobeModel = pWardrobeModel->nRow;
	pCallbackData->bForceSync = bForceSync;

	DECLARE_LOAD_SPEC( spec, pszWardrobeModelFileName );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | 
		PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND | dwPakFlags;
	spec.priority = ASYNC_PRIORITY_WARDROBE_MODELS;
	spec.fpLoadingThreadCallback = sWardrobeFileLoadCallback;
	spec.callbackData = pCallbackData;

	if ( e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD ) && ( nLOD == LOD_HIGH ) 
	  && !pWardrobeModel->bLoadFailed[ LOD_LOW ] )
		spec.pakEnum = PAK_GRAPHICS_HIGH_PLAYERS;

	pWardrobeModel->bLoadFailed[ nLOD ] = FALSE;
	pWardrobeModel->bAsyncLoadRequested = TRUE;
	pWardrobeModel->nLOD = nLOD;

	if ( bForceSync || c_GetToolMode() )
	{
		PakFileLoadNow(spec); 
	} else 
	{
		PakFileLoad(spec);
	}

	TIMER_END2();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

WARDROBE_MODEL_PART* e_WardrobeModelGetParts(
	WARDROBE_MODEL* pWardrobeModel,
	int& nPartCount )
{
	ASSERT_RETNULL( pWardrobeModel );
	WARDROBE_MODEL_ENGINE_DATA* pEngineData = pWardrobeModel->pEngineData;
	if ( ! pEngineData )
	{
		nPartCount = 0;
		return NULL;
	}

	nPartCount = pEngineData->nPartCount; 
	return pEngineData->pParts;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORD sConvertARGBTo565(
	DWORD dwColor )
{
	BYTE bRed   = (BYTE) ((dwColor >> 16) & 0xff);
	BYTE bGreen = (BYTE) ((dwColor >>  8) & 0xff);
	BYTE bBlue  = (BYTE) ((dwColor >>  0) & 0xff);
	return MAKE_565( bRed, bGreen, bBlue );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConvert565ToRGB(
	WORD w565Color,
	BYTE & bRed,
	BYTE & bGreen,
	BYTE & bBlue )
{
	bRed   = (BYTE)((w565Color << 3) & 0x00f8);
	bGreen = (BYTE)((w565Color >> 3) & 0x00fc);
	bBlue  = (BYTE)((w565Color >> 8) & 0x00f8);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConvertARGBtoRGB(
	DWORD dwColor,
	BYTE & bFirst,
	BYTE & bSecond,
	BYTE & bThird )
{
	bFirst  = (BYTE) ((dwColor >>  0) & 0xff);
	bSecond = (BYTE) ((dwColor >>  8) & 0xff);
	bThird  = (BYTE) ((dwColor >> 16) & 0xff);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_ConvertDXT1ToRGBByBlock(
	BYTE * pbDestRGB,
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha )
{
	BYTE * pbPointers[ 4 ] = { &pbDestRGB[ 0 ], &pbDestRGB[ 12 ], &pbDestRGB[ 24 ], &pbDestRGB[ 36 ] };
	dxC_ConvertDXT1ToRGB( pbPointers, pbSourceDXT1, bForceNoAlpha );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void dxC_ConvertDXT5ToRGBAByBlock(
	BYTE * pbDestRGBA,
	const BYTE * pbSourceDXT5 )
{
#if 1
	int squishFlags = squish::kDxt5;
	squish::Decompress( pbDestRGBA, pbSourceDXT5, squishFlags );
#else
	BYTE * pbPointers[ 4 ] = { &pbDestRGBA[ 0 ], &pbDestRGBA[ 16 ], &pbDestRGBA[ 32 ], &pbDestRGBA[ 48 ] };
	dxC_ConvertDXT5ToRGBA( pbPointers, pbSourceDXT5 );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void dxC_ConvertDXT1ToRGB(
	BYTE * pbDestRGB[ 4 ],
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha )
{
	// DXT1
	// 2 bytes color 0 (565)
	// 2 bytes color 1 (565)
	// 4 bytes of 0 (indexes)

	// some help from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Opaque_and_1_Bit_Alpha_Textures.asp
	WORD wColor0 = ((WORD *)pbSourceDXT1)[ 0 ];
	WORD wColor1 = ((WORD *)pbSourceDXT1)[ 1 ];

	BYTE pColorRGB[ 4 ][ 3 ];
	sConvert565ToRGB( wColor0, pColorRGB[ 0 ][ 0 ], pColorRGB[ 0 ][ 1 ], pColorRGB[ 0 ][ 2 ] );
	sConvert565ToRGB( wColor1, pColorRGB[ 1 ][ 0 ], pColorRGB[ 1 ][ 1 ], pColorRGB[ 1 ][ 2 ] );

	if (bForceNoAlpha || wColor0 > wColor1) 
	{
		// Four-color block: derive the other two colors.    
		// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
		// These 2-bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block.
		BYTE * pbColorRGBCurr0 = pColorRGB[ 0 ];
		BYTE * pbColorRGBCurr1 = pColorRGB[ 1 ];
		BYTE * pbColorRGBCurr2 = pColorRGB[ 2 ];
		BYTE * pbColorRGBCurr3 = pColorRGB[ 3 ];
		for ( int i = 0; i < 3; i++ )
		{
			*pbColorRGBCurr2 = (BYTE) ((2 * (WORD)(*pbColorRGBCurr0) + (WORD)(*pbColorRGBCurr1)) / 3); 
			*pbColorRGBCurr3 = (BYTE) (((WORD)(*pbColorRGBCurr0) + 2 * (WORD)(*pbColorRGBCurr1)) / 3);
			pbColorRGBCurr0++;
			pbColorRGBCurr1++;
			pbColorRGBCurr2++;
			pbColorRGBCurr3++;
		}
	}    
	else
	{ 
		// Three-color block: derive the other color.
		// 00 = color_0,  01 = color_1,  10 = color_2,  
		// 11 = transparent.
		// These 2-bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block. 
		BYTE * pbColorRGBCurr0 = pColorRGB[ 0 ];
		BYTE * pbColorRGBCurr1 = pColorRGB[ 1 ];
		BYTE * pbColorRGBCurr2 = pColorRGB[ 2 ];
		BYTE * pbColorRGBCurr3 = pColorRGB[ 3 ];
		for ( int i = 0; i < 3; i++ )
		{
			*pbColorRGBCurr2 = (BYTE) (((WORD)(*pbColorRGBCurr0) + (WORD)(*pbColorRGBCurr1)) / 2);
			*pbColorRGBCurr3 = 0;
			pbColorRGBCurr0++;
			pbColorRGBCurr1++;
			pbColorRGBCurr2++;
			pbColorRGBCurr3++;
		}
	}

	WORD wIndicesCurr = ((WORD *)pbSourceDXT1)[ 2 ];
	int nShiftCurr = 0;
	WORD wMaskCurr = 3;
	for ( int nY = 0; nY < 4; nY++ )
	{
		BYTE * pbDestRGBCurr = pbDestRGB[ nY ];
		for ( int nX = 0; nX < 4; nX++ )
		{
			int nIndex = (wIndicesCurr & wMaskCurr) >> nShiftCurr;
			BYTE * pbColorRGBCurr = pColorRGB[ nIndex ];
			*(pbDestRGBCurr + 0) = *(pbColorRGBCurr + 0); 
			*(pbDestRGBCurr + 1) = *(pbColorRGBCurr + 1); 
			*(pbDestRGBCurr + 2) = *(pbColorRGBCurr + 2); 
			pbDestRGBCurr += 3;
			nShiftCurr += 2;
			wMaskCurr <<= 2;
		}
		// set up for the second block
		if ( nY == 1 )
		{
			wMaskCurr = 3;
			nShiftCurr = 0;
			wIndicesCurr = ((WORD *)pbSourceDXT1)[ 3 ];
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void dxC_ConvertDXT1ToRGBA(
	BYTE * pbDestRGBA[ 4 ],
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha )
{
	// DXT1
	// 2 bytes color 0 (565)
	// 2 bytes color 1 (565)
	// 4 bytes of 0 (indexes)

	// some help from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Opaque_and_1_Bit_Alpha_Textures.asp
	WORD wColor0 = ((WORD *)pbSourceDXT1)[ 0 ];
	WORD wColor1 = ((WORD *)pbSourceDXT1)[ 1 ];

	BYTE pColorRGBA[ 4 ][ 4 ];
	sConvert565ToRGB( wColor0, pColorRGBA[ 0 ][ 0 ], pColorRGBA[ 0 ][ 1 ], pColorRGBA[ 0 ][ 2 ] );
	sConvert565ToRGB( wColor1, pColorRGBA[ 1 ][ 0 ], pColorRGBA[ 1 ][ 1 ], pColorRGBA[ 1 ][ 2 ] );

	if (bForceNoAlpha || wColor0 > wColor1) 
	{
		// Four-color block: derive the other two colors.    
		// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
		// These 2-bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block.
		BYTE * pbColorRGBACurr0 = pColorRGBA[ 0 ];
		BYTE * pbColorRGBACurr1 = pColorRGBA[ 1 ];
		BYTE * pbColorRGBACurr2 = pColorRGBA[ 2 ];
		BYTE * pbColorRGBACurr3 = pColorRGBA[ 3 ];
		for ( int i = 0; i < 3; i++ )
		{
			*pbColorRGBACurr2 = (BYTE) ((2 * (WORD)(*pbColorRGBACurr0) + (WORD)(*pbColorRGBACurr1)) / 3); 
			*pbColorRGBACurr3 = (BYTE) (((WORD)(*pbColorRGBACurr0) + 2 * (WORD)(*pbColorRGBACurr1)) / 3);
			pbColorRGBACurr0++;
			pbColorRGBACurr1++;
			pbColorRGBACurr2++;
			pbColorRGBACurr3++;
		}

		// Opaque alpha for all
		*pbColorRGBACurr0 = 0xff;
		*pbColorRGBACurr1 = 0xff;
		*pbColorRGBACurr2 = 0xff;
		*pbColorRGBACurr3 = 0xff;
	}    
	else
	{ 
		// Three-color block: derive the other color.
		// 00 = color_0,  01 = color_1,  10 = color_2,  
		// 11 = transparent.
		// These 2-bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block. 
		BYTE * pbColorRGBACurr0 = pColorRGBA[ 0 ];
		BYTE * pbColorRGBACurr1 = pColorRGBA[ 1 ];
		BYTE * pbColorRGBACurr2 = pColorRGBA[ 2 ];
		BYTE * pbColorRGBACurr3 = pColorRGBA[ 3 ];
		for ( int i = 0; i < 3; i++ )
		{
			*pbColorRGBACurr2 = (BYTE) (((WORD)(*pbColorRGBACurr0) + (WORD)(*pbColorRGBACurr1)) / 2);
			*pbColorRGBACurr3 = 0;
			pbColorRGBACurr0++;
			pbColorRGBACurr1++;
			pbColorRGBACurr2++;
			pbColorRGBACurr3++;
		}

		// Opaque alpha for the first 3, transparent for the last
		*pbColorRGBACurr0 = 0xff;
		*pbColorRGBACurr1 = 0xff;
		*pbColorRGBACurr2 = 0xff;
		*pbColorRGBACurr3 = 0x00;
	}

	WORD wIndicesCurr = ((WORD *)pbSourceDXT1)[ 2 ];
	int nShiftCurr = 0;
	WORD wMaskCurr = 3;
	for ( int nY = 0; nY < 4; nY++ )
	{
		BYTE * pbDestRGBACurr = pbDestRGBA[ nY ];
		for ( int nX = 0; nX < 4; nX++ )
		{
			int nIndex = (wIndicesCurr & wMaskCurr) >> nShiftCurr;
			BYTE * pbColorRGBACurr = pColorRGBA[ nIndex ];
			*(pbDestRGBACurr + 0) = *(pbColorRGBACurr + 0); 
			*(pbDestRGBACurr + 1) = *(pbColorRGBACurr + 1); 
			*(pbDestRGBACurr + 2) = *(pbColorRGBACurr + 2); 
			*(pbDestRGBACurr + 3) = *(pbColorRGBACurr + 3); 
			pbDestRGBACurr += 4;
			nShiftCurr += 2;
			wMaskCurr <<= 2;
		}
		// set up for the second block
		if ( nY == 1 )
		{
			wMaskCurr = 3;
			nShiftCurr = 0;
			wIndicesCurr = ((WORD *)pbSourceDXT1)[ 3 ];
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// this function doesn't work.  The texture looks like garbage in about 40% of the pixels.
//----------------------------------------------------------------------------
void dxC_ConvertDXT3ToRGBA(
	BYTE * pbDestRGBA[ 4 ],
	const BYTE * pbSourceDXT3 )
{
	//Word address 64-bit block 
	//	3:0 Transparency block 
	//	7:4 Previously described 64-bit block - similar to DXT1 block

	// some help from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Textures_with_Alpha_Channels.asp

	// decompress the Alpha
	// DXT3 alpha is a 4x4 array of 4-bit values (total 4 words, or 64 bits).

	for ( int nY = 0; nY < 4; nY++ )
	{
		const BYTE * pbSrcAlpha = ( pbSourceDXT3 + ( nY * 2 ) );
		BYTE * pbDestRGBCurr = pbDestRGBA[ nY ] + 3;
		BYTE byAlpha;

		// X loop unrolled for perf reasons, in order to avoid the bit math and directly address the nibbles.
		// In order to make the 0x0 nibble transparent and the 0xf nibble opaque, replicate the nibbles throughout the byte

		byAlpha = pbSrcAlpha[ 0 ] & 0x0f;
		*pbDestRGBCurr = (byAlpha << 4) | byAlpha;
		pbDestRGBCurr += RGBA_BYTES_PER_PIXEL;

		byAlpha = pbSrcAlpha[ 0 ] & 0xf0;
		*pbDestRGBCurr = (byAlpha >> 4) | byAlpha;
		pbDestRGBCurr += RGBA_BYTES_PER_PIXEL;

		byAlpha = pbSrcAlpha[ 1 ] & 0x0f;
		*pbDestRGBCurr = (byAlpha << 4) | byAlpha;
		pbDestRGBCurr += RGBA_BYTES_PER_PIXEL;

		byAlpha = pbSrcAlpha[ 1 ] & 0xf0;
		*pbDestRGBCurr = (byAlpha >> 4) | byAlpha;
		pbDestRGBCurr += RGBA_BYTES_PER_PIXEL;

	}

	// decompress the color
	BYTE pbColorRGB [ RGB_BYTES_PER_BLOCK ];
	dxC_ConvertDXT1ToRGBByBlock( pbColorRGB, &pbSourceDXT3[ 8 ], TRUE );

	// copy the color into the output buffer
	BYTE * pbColorRGBCurr	= pbColorRGB;
	for ( int nY = 0; nY < ROWS_PER_PIXEL_BLOCK; nY++ )
	{
		BYTE * pbDestRGBACurr = pbDestRGBA[ nY ];
		for ( int nX = 0; nX < PIXELS_PER_BLOCK_ROW; nX++ )
		{
			pbDestRGBACurr[ 0 ] = pbColorRGBCurr[ 0 ];
			pbDestRGBACurr[ 1 ] = pbColorRGBCurr[ 1 ];
			pbDestRGBACurr[ 2 ] = pbColorRGBCurr[ 2 ];
			pbDestRGBACurr += RGBA_BYTES_PER_PIXEL;
			pbColorRGBCurr += RGB_BYTES_PER_PIXEL;
		}
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// this function doesn't work.  The texture looks like garbage in about 40% of the pixels.
//----------------------------------------------------------------------------
void dxC_ConvertDXT5ToRGBA(
	BYTE * pbDestRGBA[ 4 ],
	const BYTE * pbSourceDXT5 )
{
	//Word address 64-bit block 
	//	3:0 Transparency block 
	//	7:4 Previously described 64-bit block - similar to DXT1 block

	// some help from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Textures_with_Alpha_Channels.asp
	// decompress the Alpha
	BYTE pbAlpha[ 8 ];
	pbAlpha[ 0 ] = ((BYTE *)pbSourceDXT5)[ 0 ];
	pbAlpha[ 1 ] = ((BYTE *)pbSourceDXT5)[ 1 ];

	if (pbAlpha[ 0 ] > pbAlpha[ 1 ]) {    
		// 8-alpha block:  derive the other six alphas.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		pbAlpha[ 2 ] = (6 * pbAlpha[ 0 ] + 1 * pbAlpha[ 1 ] + 3) / 7;    // bit code 010
		pbAlpha[ 3 ] = (5 * pbAlpha[ 0 ] + 2 * pbAlpha[ 1 ] + 3) / 7;    // bit code 011
		pbAlpha[ 4 ] = (4 * pbAlpha[ 0 ] + 3 * pbAlpha[ 1 ] + 3) / 7;    // bit code 100
		pbAlpha[ 5 ] = (3 * pbAlpha[ 0 ] + 4 * pbAlpha[ 1 ] + 3) / 7;    // bit code 101
		pbAlpha[ 6 ] = (2 * pbAlpha[ 0 ] + 5 * pbAlpha[ 1 ] + 3) / 7;    // bit code 110
		pbAlpha[ 7 ] = (1 * pbAlpha[ 0 ] + 6 * pbAlpha[ 1 ] + 3) / 7;    // bit code 111  
	} else {  
		// 6-alpha block.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		pbAlpha[ 2 ] = (4 * pbAlpha[ 0 ] + 1 * pbAlpha[ 1 ] + 2) / 5;    // Bit code 010
		pbAlpha[ 3 ] = (3 * pbAlpha[ 0 ] + 2 * pbAlpha[ 1 ] + 2) / 5;    // Bit code 011
		pbAlpha[ 4 ] = (2 * pbAlpha[ 0 ] + 3 * pbAlpha[ 1 ] + 2) / 5;    // Bit code 100
		pbAlpha[ 5 ] = (1 * pbAlpha[ 0 ] + 4 * pbAlpha[ 1 ] + 2) / 5;    // Bit code 101
		pbAlpha[ 6 ] = 0;												 // Bit code 110
		pbAlpha[ 7 ] = 255;												 // Bit code 111
	}

	DWORD dwIndicesCurr = *(WORD *)(pbSourceDXT5 + 2);
	for ( int nY = 0; nY < 4; nY++ )
	{
		BYTE * pbDestRGBCurr = pbDestRGBA[ nY ] + 3;
		for ( int nX = 0; nX < 4; nX++ )
		{
			int nIndex = dwIndicesCurr & 0x7;
			dwIndicesCurr >>= 3;

			*pbDestRGBCurr = pbAlpha[ nIndex ];
			pbDestRGBCurr += RGBA_BYTES_PER_PIXEL;

			if ( nY == 1 && nX == 0 )
			{
				DWORD wCarry = dwIndicesCurr & 1;
				dwIndicesCurr = *(WORD *)(pbSourceDXT5 + 4);
				dwIndicesCurr &= 0x00007fff;
				dwIndicesCurr |= wCarry << 15;
			} 
			else if ( nY == 2 && nX == 1 )
			{
				DWORD wCarry = dwIndicesCurr & 3;
				dwIndicesCurr = *(WORD *)(pbSourceDXT5 + 6);
				dwIndicesCurr &= 0x00003fff;
				dwIndicesCurr |= wCarry << 14;
			}
		}
	}

	// decompress the color
	BYTE pbColorRGB [ RGB_BYTES_PER_BLOCK ];
	dxC_ConvertDXT1ToRGBByBlock( pbColorRGB, &pbSourceDXT5[ 8 ], TRUE );

	// copy the color into the output buffer
	BYTE * pbColorRGBCurr	= pbColorRGB;
	for ( int nY = 0; nY < ROWS_PER_PIXEL_BLOCK; nY++ )
	{
		BYTE * pbDestRGBACurr = pbDestRGBA[ nY ];
		for ( int nX = 0; nX < PIXELS_PER_BLOCK_ROW; nX++ )
		{
			pbDestRGBACurr[ 0 ] = pbColorRGBCurr[ 0 ];
			pbDestRGBACurr[ 1 ] = pbColorRGBCurr[ 1 ];
			pbDestRGBACurr[ 2 ] = pbColorRGBCurr[ 2 ];
			pbDestRGBACurr += RGBA_BYTES_PER_PIXEL;
			pbColorRGBCurr += RGB_BYTES_PER_PIXEL;
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplyPaletteToRGB( 
	BYTE pbPaletteColorsRGB[ WARDROBE_COLORMASK_TOTAL_COLORS ][ 3 ],
	int nPaletteIndex,
	BYTE bPaletteAlphaIn,
	BYTE * pbDiffuseBytes )
{
	WORD wPaletteAlpha			= bPaletteAlphaIn;
	WORD wPaletteAlphaInverse	= (255 - bPaletteAlphaIn);
	BYTE * pbPaletteColorCurr = pbPaletteColorsRGB[ nPaletteIndex ];
	BYTE * pbDiffuseColorCurr = &pbDiffuseBytes[ 0 ];
	if ( nPaletteIndex != COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_SKIN )
	{
		// blend using "overlay" method - see http://www.pegtop.net/delphi/articles/blendmodes/overlay.htm
		for ( int j = 0; j < 3; j++ )
		{
			// compute the blend between diffuse and palette
			WORD BlendedColor;
			WORD DiffuseColor = *pbDiffuseColorCurr;
			//if ( *pbPaletteColorCurr < 128 )
			if ( DiffuseColor < 128 )
			{
				BlendedColor = ((DiffuseColor * (WORD)*pbPaletteColorCurr) >> 7);
			} else {
				BlendedColor = 255 - (((255 - DiffuseColor) * (WORD)(255 - *pbPaletteColorCurr)) >> 7);
			}

			// lerp blended color with either diffuse or palette color
			*pbDiffuseColorCurr = (wPaletteAlpha * BlendedColor + wPaletteAlphaInverse * DiffuseColor) >> 8;
			pbPaletteColorCurr++;
			pbDiffuseColorCurr++;
		}
	} else {
		// for the skin...
		// blend using "multiply" method - see http://www.pegtop.net/delphi/articles/blendmodes/multiply.htm
		for ( int j = 0; j < 3; j++ )
		{
			// compute the blend between diffuse and palette
			WORD DiffuseColor = *pbDiffuseColorCurr;
			WORD BlendedColor = (WORD)((DWORD)(DiffuseColor * (DWORD)*pbPaletteColorCurr) >> 8);

			// lerp blended color with either diffuse or palette color
			*pbDiffuseColorCurr = (wPaletteAlpha * BlendedColor + wPaletteAlphaInverse * DiffuseColor) >> 8;
			pbPaletteColorCurr++;
			pbDiffuseColorCurr++;
		}

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PAL_DECOMP_DATA
{
	unsigned int		count;
	unsigned int		rgb[3];
	unsigned int		outidx;
	unsigned int		error;
	PAL_DECOMP_DATA *	next;
};
#define DXT1_PIXELS_PER_BLOCK			16
#define DXT1_BITS_PER_PIXEL				2
#define DXT1_BITS_PER_BLOCK				(DXT1_PIXELS_PER_BLOCK * DXT1_BITS_PER_PIXEL)
#define DXT1_COLORS_PER_BLOCK			4
#define DXT1_COLOR_HEADER_BYTES			4


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL g_bUseSquish = FALSE;
unsigned int g_nColorMaskStrategy = 0;
unsigned int g_nColorMaskErrorThreshold = 0xffffffff;
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline unsigned int distsq(
	int x,
	int y,
	int z)
{
	return (unsigned int)(x * x + y * y + z * z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetDxt1ColorsFromBlock(
	const BYTE * dxt1Block,
	unsigned int color[DXT1_COLORS_PER_BLOCK][3])
{
	WORD c0 = *(WORD *)(dxt1Block + 0);
	WORD c1 = *(WORD *)(dxt1Block + 2);
	color[0][0] = ((c0 << 3) & 0xf8);
	color[0][1] = ((c0 >> 3) & 0xfc);
	color[0][2] = ((c0 >> 8) & 0xf8);
	color[1][0] = ((c1 << 3) & 0xf8);
	color[1][1] = ((c1 >> 3) & 0xfc);
	color[1][2] = ((c1 >> 8) & 0xf8);
	color[2][0] = (color[0][0] * 2 + color[1][0] * 1) / 3;
	color[2][1] = (color[0][1] * 2 + color[1][1] * 1) / 3;
	color[2][2] = (color[0][2] * 2 + color[1][2] * 1) / 3;
	color[3][0] = (color[0][0] * 1 + color[1][0] * 2) / 3;
	color[3][1] = (color[0][1] * 1 + color[1][1] * 2) / 3;
	color[3][2] = (color[0][2] * 1 + color[1][2] * 2) / 3;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplyPaletteBlend( 
	unsigned int dest[3],
	const BYTE pbPaletteRGB[WARDROBE_COLORMASK_TOTAL_COLORS][3],
	unsigned int index,
	unsigned int alpha,
	const unsigned int diffuse[3])
{
	unsigned int alphaInverse = 255 - alpha;

	if (index != COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_SKIN)
	{
		// blend using "overlay" method - see http://www.pegtop.net/delphi/articles/blendmodes/overlay.htm
		const unsigned int * curDiffuse = diffuse;
		const BYTE * curColorMask = pbPaletteRGB[index];
		unsigned int * end = dest + 3;
		for (; dest < end; ++curDiffuse, ++curColorMask, ++dest)
		{
			unsigned int blend;
			if (*curDiffuse < 128)
			{
				blend = ((*curDiffuse * *curColorMask) >> 7);
			}
			else
			{
				blend = 255 - (((255 - *curDiffuse) * (255 - *curColorMask)) >> 7);
			}
			// lerp blended color with either diffuse or palette color
			*dest = (alpha * blend + alphaInverse * *curDiffuse) >> 8;
		}
	} 
	else 
	{
		// for the skin, blend using "multiply" method - see http://www.pegtop.net/delphi/articles/blendmodes/multiply.htm
		const unsigned int * curDiffuse = diffuse;
		const BYTE * curColorMask = pbPaletteRGB[index];
		unsigned int * end = dest + 3;
		for (; dest < end; ++curDiffuse, ++curColorMask, ++dest)
		{
			unsigned int blend = ((*curDiffuse * *curColorMask) >> 8);

			// lerp blended color with either diffuse or palette color
			*dest = (alpha * blend + alphaInverse * *curDiffuse) >> 8;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define MASK_HIGH_MIN		128
#define MASK_HIGH_RANGE		128 //(255 - MASK_HIGH_MIN)
#define MASK_LOW_MAX		112
#define MASK_LOW_OFFSET		(255 - 2 * MASK_LOW_MAX)
#define BRIGHT_MASK_MIN		148
#define BRIGHT_MASK_ALPHA	(255 * 3 / 4)

static void sApplyPaletteGetIndexAndAlpha(
	const unsigned int col[3],
	unsigned int & index,
	unsigned int & alpha)
{
	// If our color mask is above a minimum threshold, then interpret
	// the index and alpha based on the channels used (red and green, 
	// green and blue, or red and blue).
	if (col[0] > BRIGHT_MASK_MIN && col[1] > BRIGHT_MASK_MIN)
	{
		index = COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_SKIN;
		alpha = (col[1] - BRIGHT_MASK_MIN) * 2;
		return;
	}
	if (col[1] > BRIGHT_MASK_MIN && col[2] > BRIGHT_MASK_MIN)
	{
		index = COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_HAIR;
		alpha = (col[1] - BRIGHT_MASK_MIN) * 2;
		return;
	}
	if (col[0] > BRIGHT_MASK_MIN && col[2] > BRIGHT_MASK_MIN)
	{
		index = COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_UNUSED;
		alpha = (col[0] - BRIGHT_MASK_MIN) * 2;
		return;
	}

	// If we didn't fall within ranges specific to body areas, then
	// pick our closest match for our index/alpha pair.

	// We don't want to bleed over from one range to another 
	// when mipping, so we keep a gap between MASK_LOW_MAX 
	// and MASK_HIGH_MIN.
	unsigned int indexHigh = MAXIDX(col[0], col[1], col[2]);	// index high is the largest value > MASK_HIGH_MIN
	static const unsigned int INDEX_LOW_OFFSET = 0xffffffffUL - (MASK_LOW_MAX - 1UL);	// used to roll-over any values >= MASK_LOW_MAX
	unsigned int indexLow = MAXIDX(col[0] + INDEX_LOW_OFFSET, col[1] + INDEX_LOW_OFFSET, col[2] + INDEX_LOW_OFFSET);
	if (col[indexHigh] <= MASK_HIGH_MIN)						// no appropriate indexHigh found
	{
		if (col[indexLow] - 1UL >= MASK_LOW_MAX - 1UL)			// no appropriate indexLow found (uses roll-over to ignore col[ii] == 0)
		{
			index = 0;											// return 0 alpha to copy diffuse
			alpha = 0;
		}
		else													// use indexLow
		{
			alpha = 2 * col[indexLow] + MASK_LOW_OFFSET;
			index = 5 - indexLow;
		}
	}
	else if (col[indexLow] - 1UL >= MASK_LOW_MAX - 1UL)			// no appropriate indexLow, use indexHigh (uses roll-over to ignore col[ii] == 0)
	{
		alpha = col[indexHigh] - MASK_HIGH_MIN;
		if (alpha < MASK_LOW_OFFSET / 2)
		{
			index = 0;
			alpha = 0;
		}
		else
		{
			alpha = alpha * 2;
			index = 2 - indexHigh;
		}
	}
	else														// both indexHigh and indexLow are available, so compare them
	{
		unsigned int alphaHigh = col[indexHigh] - MASK_HIGH_MIN;
		unsigned int alphaLow = col[indexLow];

		if (alphaLow + (MASK_LOW_OFFSET/2) > alphaHigh)
		{
			alpha = 2 * alphaLow + MASK_LOW_OFFSET;
			index = 5 - indexLow;
		}
		else
		{
			alpha = 2 * alphaHigh;
			index = 2 - indexHigh;
		}
	}
}


//----------------------------------------------------------------------------
// return TRUE if we need to compress, FALSE if we don't
//----------------------------------------------------------------------------
static BOOL sApplyPaletteColorData(
	const BYTE * srcDiffuseBlock,
	const BYTE * srcColorMaskBlock,
	const BYTE pbPaletteRGB[WARDROBE_COLORMASK_TOTAL_COLORS][3],
	PAL_DECOMP_DATA colors[DXT1_COLORS_PER_BLOCK * DXT1_COLORS_PER_BLOCK],
	PAL_DECOMP_DATA *& last,
	unsigned int srcLookup[DXT1_PIXELS_PER_BLOCK])
{
	// clear color data
	for (unsigned int ii = 0; ii < DXT1_COLORS_PER_BLOCK * DXT1_COLORS_PER_BLOCK; ++ii)
	{
		colors[ii].count = 0;
	}
	last = NULL;

	// get RGB565 colors for src blocks
	unsigned int colDiffuseBlock[DXT1_COLORS_PER_BLOCK][3];
	sGetDxt1ColorsFromBlock(srcDiffuseBlock, colDiffuseBlock);

	unsigned int colColorMaskBlock[DXT1_COLORS_PER_BLOCK][3];
	sGetDxt1ColorsFromBlock(srcColorMaskBlock, colColorMaskBlock);

	// build color data
	BOOL needCompress = FALSE;		// don't need to compress if all color mask alpha == 0
	DWORD srcDiffuseBits = *(DWORD *)(srcDiffuseBlock + DXT1_COLOR_HEADER_BYTES);
	DWORD srcColorMaskBits = *(DWORD *)(srcColorMaskBlock + DXT1_COLOR_HEADER_BYTES);
	unsigned int * lookupCur = srcLookup;
	for (unsigned int bit = 0; bit < DXT1_BITS_PER_BLOCK; bit += DXT1_BITS_PER_PIXEL, ++lookupCur)
	{
		unsigned int diffuseIndex = (srcDiffuseBits & 3);
		srcDiffuseBits >>= 2;
		unsigned int colormaskIndex = (srcColorMaskBits & 3);
		srcColorMaskBits >>= 2;
		unsigned int colorIndex = diffuseIndex * DXT1_COLORS_PER_BLOCK + colormaskIndex;
		*lookupCur = colorIndex;
		PAL_DECOMP_DATA * col = colors + colorIndex;
		++col->count;
		if (col->count == 1)		// first time this color has occurred
		{
			col->next = last;
			last = col;
	
			unsigned int colorMaskIndex;
			unsigned int colorMaskAlpha;
			sApplyPaletteGetIndexAndAlpha(colColorMaskBlock[colormaskIndex], colorMaskIndex, colorMaskAlpha);

			if ( colorMaskAlpha )			// blend diffuse & color mask
			{
				needCompress = TRUE;
				sApplyPaletteBlend(col->rgb, pbPaletteRGB, colorMaskIndex, colorMaskAlpha, colDiffuseBlock[diffuseIndex]);
			}
			else						// use diffuse colors
			{
				col->rgb[0] = colDiffuseBlock[diffuseIndex][0];
				col->rgb[1] = colDiffuseBlock[diffuseIndex][1];
				col->rgb[2] = colDiffuseBlock[diffuseIndex][2];
			}
		}
	}

	return needCompress;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplyPaletteCompress(
	BYTE * destBlock,
	PAL_DECOMP_DATA colors[DXT1_COLORS_PER_BLOCK * DXT1_COLORS_PER_BLOCK],
	PAL_DECOMP_DATA *& first,
	unsigned int col[DXT1_COLORS_PER_BLOCK][3],
	unsigned int srcLookup[DXT1_PIXELS_PER_BLOCK],
	WORD col565[DXT1_COLORS_PER_BLOCK])
{
#if ISVERSION(DEVELOPMENT)
	unsigned int totalerror = 0;
#endif

	// select best match color
	for (PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int best = 0;
		unsigned int bestdist = 0xffffffff;
		for (unsigned int cc = 0; cc < 4; ++cc)
		{
			unsigned int dist = distsq(cur->rgb[0] - col[cc][0], cur->rgb[1] - col[cc][1], cur->rgb[2] - col[cc][2]);
			if (dist < bestdist)
			{
				best = cc;
				bestdist = dist;
			}
		}
		cur->outidx = best;

#if ISVERSION(DEVELOPMENT)
		if (g_nColorMaskErrorThreshold != 0xffffffff)
		{
			WORD perfect = MAKE_565(cur->rgb[0], cur->rgb[1], cur->rgb[2]);
			int perf[3];
			perf[0] = ((perfect >>  0) & 0x1f);
			perf[1] = ((perfect >>  5) & 0x3f);
			perf[2] = ((perfect >> 11) & 0x1f);
			int selected[3];
			selected[0] = ((col565[best] >>  0) & 0x1f);
			selected[1] = ((col565[best] >>  5) & 0x3f);
			selected[2] = ((col565[best] >> 11) & 0x1f);
			cur->error = ABS(selected[0] - perf[0]) + ABS(selected[1] - perf[1]) + ABS(selected[2] - perf[2]);
		}
#endif
	}

	// write output
	DWORD dest = 0;
	unsigned int * lookup = srcLookup;
	for (unsigned int bit = 0; bit < DXT1_BITS_PER_BLOCK; bit += DXT1_BITS_PER_PIXEL, ++lookup)
	{
		PAL_DECOMP_DATA * col = colors + *lookup;
		dest |= (col->outidx << bit);
#if ISVERSION(DEVELOPMENT)
		totalerror += col->error;
#endif
	}
	*(DWORD *)(destBlock + DXT1_COLOR_HEADER_BYTES) = dest;

#if ISVERSION(DEVELOPMENT)
	if (totalerror >= g_nColorMaskErrorThreshold)
	{
		col565[0] = MAKE_565(255, 74, 255);
		col565[1] = MAKE_565(255, 74, 240);
	}
#endif

	// write colors
	*(DWORD *)(destBlock) = col565[0] + ((unsigned int)col565[1] << 16);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplyPaletteComputeColors(
	unsigned int col[DXT1_COLORS_PER_BLOCK][3],
	WORD col565[DXT1_COLORS_PER_BLOCK])
{
	// swap col0 & col1 if necessary
	col565[0] = MAKE_565(col[0][0], col[0][1], col[0][2]);
	col565[1] = MAKE_565(col[1][0], col[1][1], col[1][2]);
	if (col565[0] < col565[1])
	{
		::SWAP(col565[0], col565[1]);
		::SWAP(col[0][0], col[1][0]);
		::SWAP(col[0][1], col[1][1]);
		::SWAP(col[0][2], col[1][2]);
	}
	else if (col565[0] == col565[1])
	{
		if ((col565[1] & 31) != 0)
		{
			col565[1] -= 1;
			col[1][2] -= 8;
		}
		else
		{
			col565[0] += 1;
			col[0][2] += 8;
		}
	}

	// compute intermediate colors
	col[2][0] = (col[0][0] * 2 + col[1][0] * 1) / 3;
	col[2][1] = (col[0][1] * 2 + col[1][1] * 1) / 3;
	col[2][2] = (col[0][2] * 2 + col[1][2] * 1) / 3;
	col[3][0] = (col[0][0] * 1 + col[1][0] * 2) / 3;
	col[3][1] = (col[0][1] * 1 + col[1][1] * 2) / 3;
	col[3][2] = (col[0][2] * 1 + col[1][2] * 2) / 3;
}


//----------------------------------------------------------------------------
// pick c0 & c1 by getting longest axis & averaging on each side
//----------------------------------------------------------------------------
static void sApplyPaletteByDecompression0(
	PAL_DECOMP_DATA *& first,
	unsigned int col[DXT1_COLORS_PER_BLOCK][3])
{
	// get average (mid) of each color dimension
	int total[3] = {0, 0, 0};
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		total[0] += cur->rgb[0] * cur->count;
		total[1] += cur->rgb[1] * cur->count;
		total[2] += cur->rgb[2] * cur->count;
	}
	unsigned int mid[3];
	mid[0] = (total[0] + 8) / 16;
	mid[1] = (total[1] + 8) / 16;
	mid[2] = (total[2] + 8) / 16;
	// get average of each color dimension for each side of mid
	int min[3] = {0, 0, 0};
	int max[3] = {0, 0, 0};
	int mincount[3]  = {0, 0, 0};
	int maxcount[3]  = {0, 0, 0};
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int count = cur->count;
		if (cur->rgb[0] >= mid[0]) { max[0] += cur->rgb[0] * count; maxcount[0] += count; } else { min[0] += cur->rgb[0] * count; mincount[0] += count; }
		if (cur->rgb[1] >= mid[1]) { max[1] += cur->rgb[1] * count; maxcount[1] += count; } else { min[1] += cur->rgb[1] * count; mincount[1] += count; }
		if (cur->rgb[2] >= mid[2]) { max[2] += cur->rgb[2] * count; maxcount[2] += count; } else { min[2] += cur->rgb[2] * count; mincount[2] += count; }
	}

	// determine which axis is longest
	unsigned int longaxis = 0;
	unsigned int len[3];
	len[0] = ((maxcount[0] > 0 ? max[0]/maxcount[0] : mid[0]) - (mincount[0] > 0 ? min[0]/mincount[0] : mid[0]));
	len[1] = ((maxcount[1] > 0 ? max[1]/maxcount[1] : mid[1]) - (mincount[1] > 0 ? min[1]/mincount[1] : mid[1]));
	len[2] = ((maxcount[2] > 0 ? max[2]/maxcount[2] : mid[2]) - (mincount[2] > 0 ? min[2]/mincount[2] : mid[2]));
	if (len[1] > len[0])
	{
		longaxis = 1;
	}
	if (len[2] > len[longaxis])
	{
		longaxis = 2;
	}

	// compute averages of points to each side of midpoint of longest axis
	int fmin[3] = {0, 0, 0};
	int fmax[3] = {0, 0, 0};
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int count = cur->count;
		if (cur->rgb[longaxis] >= mid[longaxis])
		{
			fmax[0] += cur->rgb[0] * count;
			fmax[1] += cur->rgb[1] * count;
			fmax[2] += cur->rgb[2] * count;
		}
		else
		{
			fmin[0] += cur->rgb[0] * count;
			fmin[1] += cur->rgb[1] * count;
			fmin[2] += cur->rgb[2] * count;
		}
	}

	// get final c0 & c1 by splitting points along longaxis and averaging
	if (maxcount[longaxis] > 0)
	{
		col[0][0] = ((fmax[0] + (maxcount[longaxis] / 2)) / maxcount[longaxis]);
		col[0][1] = ((fmax[1] + (maxcount[longaxis] / 2)) / maxcount[longaxis]);
		col[0][2] = ((fmax[2] + (maxcount[longaxis] / 2)) / maxcount[longaxis]);
	}
	else
	{
		col[0][0] = mid[0];
		col[0][1] = mid[1];
		col[0][2] = mid[2];
	}
	if (mincount[longaxis] > 0)
	{
		col[1][0] = ((fmin[0] + (mincount[longaxis] / 2)) / mincount[longaxis]);
		col[1][1] = ((fmin[1] + (mincount[longaxis] / 2)) / mincount[longaxis]);
		col[1][2] = ((fmin[2] + (mincount[longaxis] / 2)) / mincount[longaxis]);
	}
	else
	{
		col[1][0] = mid[0];
		col[1][1] = mid[1];
		col[1][2] = mid[2];
	}
}


//----------------------------------------------------------------------------
// pick c0 & c1 independently averaging each side of each midpoint of each dimension
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sApplyPaletteByDecompression1(
	PAL_DECOMP_DATA *& first,
	unsigned int col[DXT1_COLORS_PER_BLOCK][3])
{
	// get average (mid) of each color dimension
	int total[3] = {0, 0, 0};
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		total[0] += cur->rgb[0] * cur->count;
		total[1] += cur->rgb[1] * cur->count;
		total[2] += cur->rgb[2] * cur->count;
	}
	unsigned int mid[3];
	mid[0] = (total[0] + 8) / 16;
	mid[1] = (total[1] + 8) / 16;
	mid[2] = (total[2] + 8) / 16;

	// get averages on each dimension for each half
	int min[3] = {0, 0, 0};
	int max[3] = {0, 0, 0};
	int mincount[3]  = {0, 0, 0};
	int maxcount[3]  = {0, 0, 0};

	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int count = cur->count;
		if (cur->rgb[0] >= mid[0]) { max[0] += cur->rgb[0] * count; maxcount[0] += count; } else { min[0] += cur->rgb[0] * count; mincount[0] += count; }
		if (cur->rgb[1] >= mid[1]) { max[1] += cur->rgb[1] * count; maxcount[1] += count; } else { min[1] += cur->rgb[1] * count; mincount[1] += count; }
		if (cur->rgb[2] >= mid[2]) { max[2] += cur->rgb[2] * count; maxcount[2] += count; } else { min[2] += cur->rgb[2] * count; mincount[2] += count; }
	}

	col[0][0] = mincount[0] > 0 ? ((min[0] + (mincount[0] / 2)) / mincount[0]) : mid[0];
	col[0][1] = mincount[1] > 0 ? ((min[1] + (mincount[1] / 2)) / mincount[1]) : mid[1];
	col[0][2] = mincount[2] > 0 ? ((min[2] + (mincount[2] / 2)) / mincount[2]) : mid[2];
	col[1][0] = maxcount[0] > 0 ? ((max[0] + (maxcount[0] / 2)) / maxcount[0]) : mid[0];
	col[1][1] = maxcount[1] > 0 ? ((max[1] + (maxcount[1] / 2)) / maxcount[1]) : mid[1];
	col[1][2] = maxcount[2] > 0 ? ((max[2] + (maxcount[2] / 2)) / maxcount[2]) : mid[2];
}
#endif


//----------------------------------------------------------------------------
// pick c0 & c1 by getting furthest from middle, and furthest from c0
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sApplyPaletteByDecompression2(
	PAL_DECOMP_DATA *& first,
	unsigned int col[DXT1_COLORS_PER_BLOCK][3])
{
	int total[3] = {0, 0, 0};
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		total[0] += cur->rgb[0] * cur->count;
		total[1] += cur->rgb[1] * cur->count;
		total[2] += cur->rgb[2] * cur->count;
	}
	int mid[3];
	mid[0] = (total[0] + 8) / 16;
	mid[1] = (total[1] + 8) / 16;
	mid[2] = (total[2] + 8) / 16;

	const PAL_DECOMP_DATA * best = first;
	unsigned int bestdist = 0;
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int dist = distsq(cur->rgb[0] - mid[0], cur->rgb[1] - mid[1], cur->rgb[2] - mid[2]);
		if (dist > bestdist)
		{
			bestdist = dist;
			best = cur;
		}
	}

	col[0][0] = best->rgb[0];
	col[0][1] = best->rgb[1];
	col[0][2] = best->rgb[2];

	best = first;
	bestdist = 0;
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int dist = distsq(cur->rgb[0] - col[0][0], cur->rgb[1] - col[0][1], cur->rgb[2] - col[0][2]);
		if (dist > bestdist)
		{
			bestdist = dist;
			best = cur;
		}
	}

	col[1][0] = best->rgb[0];
	col[1][1] = best->rgb[1];
	col[1][2] = best->rgb[2];
}
#endif


//----------------------------------------------------------------------------
// pick c0 & c1 by getting brightest and darkest
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sApplyPaletteByDecompression3(
	PAL_DECOMP_DATA *& first,
	unsigned int col[DXT1_COLORS_PER_BLOCK][3])
{
	const PAL_DECOMP_DATA * bright = first;
	unsigned int brightdist = 0;
	const PAL_DECOMP_DATA * dark = first;
	unsigned int darkdist = 0xffffffff;
	for (const PAL_DECOMP_DATA * cur = first; cur; cur = cur->next)
	{
		unsigned int curdist = distsq(cur->rgb[0], cur->rgb[1], cur->rgb[2]);
		if (curdist > brightdist)
		{
			brightdist = curdist;
			bright = cur;
		}
		if (curdist < darkdist)
		{
			darkdist = curdist;
			dark = cur;
		}
	}

	col[0][0] = bright->rgb[0];
	col[0][1] = bright->rgb[1];
	col[0][2] = bright->rgb[2];
	col[1][0] = dark->rgb[0];
	col[1][1] = dark->rgb[1];
	col[1][2] = dark->rgb[2];
}
#endif


//----------------------------------------------------------------------------
// return FALSE if we didn't need to re-compress or TRUE if we did
//----------------------------------------------------------------------------
static BOOL sApplyPaletteByDecompressionEx(
	BYTE * destBlock,
	const BYTE * srcDiffuseBlock,
	const BYTE * srcColorMaskBlock,
	const BYTE pbPaletteRGB[WARDROBE_COLORMASK_TOTAL_COLORS][3])
{
	PAL_DECOMP_DATA colors[DXT1_COLORS_PER_BLOCK * DXT1_COLORS_PER_BLOCK];
	PAL_DECOMP_DATA * first = NULL;
	unsigned int srcLookup[DXT1_PIXELS_PER_BLOCK];
	if (!sApplyPaletteColorData(srcDiffuseBlock, srcColorMaskBlock, pbPaletteRGB, colors, first, srcLookup))
	{
		return FALSE;
	}

	unsigned int col[DXT1_COLORS_PER_BLOCK][3];

#if ISVERSION(DEVELOPMENT)
	switch (g_nColorMaskStrategy)
	{
	case 0:
		sApplyPaletteByDecompression0(first, col);
		break;

	case 1:
		sApplyPaletteByDecompression1(first, col);
		break;

	case 2:
		sApplyPaletteByDecompression2(first, col);
		break;

	case 3:
		sApplyPaletteByDecompression3(first, col);
		break;

	default:
		__assume(0);
	}
#else
	sApplyPaletteByDecompression0(first, col);
#endif

	WORD col565[DXT1_COLORS_PER_BLOCK];
	sApplyPaletteComputeColors(col, col565);

	sApplyPaletteCompress(destBlock, colors, first, col, srcLookup, col565);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sApplyPaletteByDecompression(
	BYTE * pbDest,
	BYTE * pbDiffuseBlockStart,
	BYTE * pbColorMaskBlockStart,
	BYTE pbPaletteColorsRGB[ WARDROBE_COLORMASK_TOTAL_COLORS ][ 3 ] )
{
	BYTE pbDiffuseRGB[RGB_BYTES_PER_BLOCK];
	BYTE pbColorMaskRGB[RGB_BYTES_PER_BLOCK];
	dxC_ConvertDXT1ToRGBByBlock( pbDiffuseRGB,	pbDiffuseBlockStart,  FALSE );
	dxC_ConvertDXT1ToRGBByBlock( pbColorMaskRGB, pbColorMaskBlockStart,FALSE );

	BOOL bNeedsToCompress = FALSE;
	// combine to form the target 24 bit block
	// blend using "hard light" method - see http://www.pegtop.net/delphi/articles/blendmodes/hardlight.htm
	BYTE * pbDiffuseByteCurr	= pbDiffuseRGB;
	BYTE * pbColorMaskByteCurr	= pbColorMaskRGB;
	for ( int i = 0; i < RGB_BYTES_PER_BLOCK; i += 3 )
	{
		unsigned int nPaletteIndex = 0;
		unsigned int nPaletteAlpha = 0;
		unsigned int colColorMask[ 3 ] = { pbColorMaskRGB[ 0 ], pbColorMaskRGB[ 1 ], pbColorMaskRGB[ 2 ] };
		sApplyPaletteGetIndexAndAlpha( colColorMask, nPaletteIndex, nPaletteAlpha );

		if ( nPaletteAlpha )
		{
			bNeedsToCompress = TRUE;
			sApplyPaletteToRGB( pbPaletteColorsRGB, nPaletteIndex, nPaletteAlpha, pbDiffuseByteCurr );
		}

		pbDiffuseByteCurr += 3;
		pbColorMaskByteCurr += 3;
	}

	// compress the block
	if ( ! bNeedsToCompress )
		return FALSE;
	
	BYTE pbDestRGBA[ RGBA_BYTES_PER_BLOCK ];
	BYTE* pbColorRGBCurr = pbDiffuseRGB;
	for ( int nY = 0; nY < ROWS_PER_PIXEL_BLOCK; nY++ )
	{
		BYTE* pbDestRGBACurr = 
			&pbDestRGBA[ nY * RGBA_BYTES_PER_PIXEL * PIXELS_PER_BLOCK_ROW ];
		for ( int nX = 0; nX < PIXELS_PER_BLOCK_ROW; nX++ )
		{
			// Swizzle the colors from ARGB8 to ABGR8.
			pbDestRGBACurr[ 0 ] = pbColorRGBCurr[ 2 ];
			pbDestRGBACurr[ 1 ] = pbColorRGBCurr[ 1 ];
			pbDestRGBACurr[ 2 ] = pbColorRGBCurr[ 0 ];
			pbDestRGBACurr[ 3 ] = 0xff;
			pbDestRGBACurr += RGBA_BYTES_PER_PIXEL;
			pbColorRGBCurr += RGB_BYTES_PER_PIXEL;
		}
	}

	int squishFlags = squish::kDxt1 | squish::kColourRangeFit | squish::kColourMetricUniform;
	squish::Compress( pbDestRGBA, pbDest, squishFlags);

#if ISVERSION(DEVELOPMENT)
	if (g_nColorMaskErrorThreshold != 0xffffffff)
	{
		unsigned int totalerror = 0;

		// get RGB565 colors for src blocks
		unsigned int colSquished[DXT1_COLORS_PER_BLOCK][3];
		sGetDxt1ColorsFromBlock(pbDest, colSquished);

		BYTE * curRGBA = pbDestRGBA;
		DWORD destPixels = *(DWORD *)(pbDest + DXT1_COLOR_HEADER_BYTES);
		for (int ii = 0; ii < DXT1_PIXELS_PER_BLOCK; ++ii)
		{
			WORD perfect = MAKE_565(curRGBA[0], curRGBA[1], curRGBA[2]);
			curRGBA += RGBA_BYTES_PER_PIXEL;
			int perf[3];
			perf[0] = ((perfect >>  0) & 0x1f);
			perf[1] = ((perfect >>  5) & 0x3f);
			perf[2] = ((perfect >> 11) & 0x1f);

			int index = (destPixels >> (ii * DXT1_BITS_PER_PIXEL)) & 3;		

			WORD col565 = MAKE_565(colSquished[index][0], colSquished[index][1], colSquished[index][2]);
			int selected[3];
			selected[0] = ((col565 >>  0) & 0x1f);
			selected[1] = ((col565 >>  5) & 0x3f);
			selected[2] = ((col565 >> 11) & 0x1f);
			totalerror += ABS(selected[0] - perf[0]) + ABS(selected[1] - perf[1]) + ABS(selected[2] - perf[2]);
		}
		if (totalerror >= g_nColorMaskErrorThreshold)
		{
			*(WORD *)(pbDest + 0) = MAKE_565(255, 74, 255);
			*(WORD *)(pbDest + 2) = MAKE_565(255, 74, 240);
		}
	}
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORD sApplyPaletteTo565Palette(
	BYTE pbPaletteColorsRGB[ WARDROBE_COLORMASK_TOTAL_COLORS ][ 3 ],
	int nPaletteIndex,
	BYTE bPaletteAlpha,
	WORD wDiffuse565)
{
	BYTE pbDiffuseRGB[ 3 ]; 
	sConvert565ToRGB( wDiffuse565, pbDiffuseRGB[ 0 ], pbDiffuseRGB[ 1 ], pbDiffuseRGB[ 2 ] );
	sApplyPaletteToRGB( pbPaletteColorsRGB, nPaletteIndex, bPaletteAlpha, pbDiffuseRGB );
	return MAKE_565( pbDiffuseRGB[ 0 ], pbDiffuseRGB[ 1 ], pbDiffuseRGB[ 2 ] );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sApplyPaletteWhileCompressed(
	BYTE * pbDest,
	BYTE * pbDiffuseCurr,
	WORD wColorMask565,
	BYTE pbPaletteColorsRGB[ WARDROBE_COLORMASK_TOTAL_COLORS ][ 3 ],
	BOOL bDXT1 )
{
	BYTE pbColorMaskRGB[ 3 ]; 
	sConvert565ToRGB( wColorMask565, pbColorMaskRGB[ 0 ], pbColorMaskRGB[ 1 ], pbColorMaskRGB[ 2 ] );

	unsigned int nPaletteIndex = 0;
	unsigned int nPaletteAlpha = 0;
	unsigned int colColorMask[ 3 ] = { pbColorMaskRGB[ 0 ], pbColorMaskRGB[ 1 ], pbColorMaskRGB[ 2 ] };
	sApplyPaletteGetIndexAndAlpha( colColorMask, nPaletteIndex, nPaletteAlpha );

	ASSERT( ! bDXT1 || ((WORD*)pbDiffuseCurr)[ 0 ] > ((WORD*)pbDiffuseCurr)[ 1 ] );
	BOOL bZeroIsGreater = ((WORD *)pbDiffuseCurr)[ 0 ] > ((WORD *)pbDiffuseCurr)[ 1 ];

	WORD& wDestColor0 = ((WORD*)pbDest)[ 0 ];
	WORD& wDestColor1 = ((WORD*)pbDest)[ 1 ];

	wDestColor0 = sApplyPaletteTo565Palette( pbPaletteColorsRGB, nPaletteIndex, nPaletteAlpha, ((WORD*)pbDiffuseCurr)[ 0 ] );
	wDestColor1 = sApplyPaletteTo565Palette( pbPaletteColorsRGB, nPaletteIndex, nPaletteAlpha, ((WORD*)pbDiffuseCurr)[ 1 ] );
	if ( bZeroIsGreater != ( wDestColor0 > wDestColor1 ) )
	{
		// swap the palettes so that they are in the correct order
		WORD wTemp = wDestColor0;
		wDestColor0 = wDestColor1;
		wDestColor1 = wTemp;

		// swap the indices see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Opaque_and_1_Bit_Alpha_Textures.asp
		//   for info about how the indices work.
		// Isn't this fun?
		if ( bZeroIsGreater )
		{
			if ( wDestColor0 == wDestColor1 )
			{ 
				// If both colors are black, then switch all of the colors in the
				// 4x4 bitmap to use index 1 and then make color0 > color1.
				if ( wDestColor0 == 0 )
				{
					wDestColor0++;
					((DWORD*)pbDest)[ 1 ] = 0x55555555;
				}

				// Subtract one from the first available channel that has color.
				if ( ( wDestColor1 & 0x07e0 ) != 0 )
				{
					wDestColor1 -= 0x0020;
				}
				else if ( ( wDestColor1 & 0xf800 ) != 0 )
				{
					wDestColor1 -= 0x0800;
				}
				else if ( ( wDestColor1 & 0x001f ) != 0 )
				{
					wDestColor1 -= 0x0001;
				}
			} 
			else 
			{
				// Swap indices used for the colors in the 4x4 bitmap.
				((DWORD *)pbDest)[ 1 ] ^= 0x55555555;
			}
		} 
		else 
		{
			DWORD dwMaskHigh = ((DWORD *)pbDest)[ 1 ] & 0xaaaaaaaa;
			DWORD dwMaskLow  = ((DWORD *)pbDest)[ 1 ] & 0x55555555;
			DWORD dwMaskHighShifted = dwMaskHigh >> 1;
			((DWORD *)pbDest)[ 1 ] = dwMaskHigh | ( ((~dwMaskHighShifted) & 0x5555555) ^ dwMaskLow );
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COLORMASK_ASYNC_DATA
{
	int nNumMipLevels;
	int* nHeight;
	int* nWidth;
	DXC_FORMAT eFormat;
	DXC_FORMAT eDecompressedFormat;
	int* nPitch;
	BYTE** pbDiffuse;
	BYTE** pbColorMask;
	DWORD pdwColors[ WARDROBE_COLORMASK_TOTAL_COLORS ];
	int nQuality;
	int nDestTexture;
	//int nDiffuseTexture;
	//int nColorMaskTexture;
	BOOL bDontApplyBody;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombinerExecute( 
	JOB_DATA & tJobData)
{
#if ISVERSION(DEVELOPMENT)
	DWORD start_tick = GetTickCount();	// not using high frequency timer because this is on another thread
#endif

	COLORMASK_ASYNC_DATA * pData = (COLORMASK_ASYNC_DATA *)tJobData.data1;

	ASSERT(dx9_IsCompressedTextureFormat(pData->eFormat));

	BOOL bDXT1 = (pData->eFormat == D3DFMT_DXT1);
	//ASSERT(pData->eFormat == D3DFMT_DXT1);
	//DXC_FORMAT eDecompressedFormat = DX9_BLOCK(D3DFMT_R8G8B8) DX10_BLOCK(D3DFMT_A8R8G8B8);

	int nBytesPerBlock = bDXT1 ? cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;

	WORD pwPaletteColors[WARDROBE_COLORMASK_TOTAL_COLORS];
	BYTE pbPaletteColorsRGB	[WARDROBE_COLORMASK_TOTAL_COLORS][3];

	for ( int i = 0; i < WARDROBE_COLORMASK_TOTAL_COLORS; i++ )
	{
		pwPaletteColors[ i ] = sConvertARGBTo565( pData->pdwColors[ i ] );
		sConvertARGBtoRGB( pData->pdwColors[ i ],	pbPaletteColorsRGB[ i ][ 0 ], pbPaletteColorsRGB[ i ][ 1 ], pbPaletteColorsRGB[ i ][ 2 ] );
	}

#if ISVERSION(DEVELOPMENT)
	int nCompressedBlocks = 0;
	int nSolidUncompressedBlocks = 0;
	int nTotalBlocks = 0;

	g_nColorMaskStrategy = PIN(g_nColorMaskStrategy, (unsigned int)0, (unsigned int)3);
#endif

	for ( int nMipLevel = 0; nMipLevel < pData->nNumMipLevels; nMipLevel++ )
	{
		BYTE * pbDiffuseRowStart   = pData->pbDiffuse[ nMipLevel ];
		BYTE * pbColorMaskRowStart = pData->pbColorMask[ nMipLevel ];

		ASSERT_CONTINUE( pbDiffuseRowStart );
		ASSERT_CONTINUE( pbColorMaskRowStart );

		if ( !bDXT1 )
		{
			// DXT5 has 64 bits of alpha channel data, then 64 bits of color data.
			// Let's skip the alpha and only combine the color data.
			pbDiffuseRowStart += cnDXT1BytesPerPacket;
			pbColorMaskRowStart += cnDXT1BytesPerPacket;
		}

		for( int nY = 0; nY < pData->nHeight[ nMipLevel ]; nY += 4 )
		{
			BYTE * pbDiffuseCurr	= pbDiffuseRowStart;
			BYTE * pbColorMaskCurr	= pbColorMaskRowStart;
			for ( int nX = 0; nX < pData->nWidth[ nMipLevel ]; nX += 4 )
			{
#if ISVERSION(DEVELOPMENT)
				nTotalBlocks++;
#endif
				BYTE * pbDest = pbDiffuseCurr;

				// DXT1
				// 2 bytes color 0 (565)
				// 2 bytes color 1 (565)
				// 4 bytes of 0 (indexes)

				// see if we can just change the palettes in compressed space
				BOOL bColorMaskIsMostlyBlack = (((DWORD *)pbColorMaskCurr)[ 0 ] & 0xe79ce79c) == 0;
				if ( !bColorMaskIsMostlyBlack )
				{
					BOOL bDoInCompressedSpace = FALSE;
					WORD wColorMask565 = 0;

					DWORD dwColorMaskIndices = ((DWORD *)pbColorMaskCurr)[ 1 ];

					if ( dwColorMaskIndices == 0 ) // index 0
					{
						wColorMask565 = ((WORD *)pbColorMaskCurr)[ 0 ];
						bDoInCompressedSpace = TRUE;
					}
					else if ( (dwColorMaskIndices == 0x55555555) ) // index 1
					{
						wColorMask565 = ((WORD *)pbColorMaskCurr)[ 1 ];
						bDoInCompressedSpace = TRUE;
					} 
					else if ( dwColorMaskIndices == 0xaaaaaaaa 
						   || dwColorMaskIndices == 0xffffffff ) // index 2 or 3
					{						
						// We only need index 2 or 3
						int nIndex = ( dwColorMaskIndices == 0xaaaaaaaa ) ? 
										 2 : 3;
						WORD wColor0 = ((WORD *)pbColorMaskCurr)[ 0 ];
						WORD wColor1 = ((WORD *)pbColorMaskCurr)[ 1 ];

						BYTE pColorRGB[ 2 ][ 3 ];
						sConvert565ToRGB( wColor0, pColorRGB[ 0 ][ 0 ], pColorRGB[ 0 ][ 1 ], pColorRGB[ 0 ][ 2 ] );
						sConvert565ToRGB( wColor1, pColorRGB[ 1 ][ 0 ], pColorRGB[ 1 ][ 1 ], pColorRGB[ 1 ][ 2 ] );

						ASSERT( ! bDXT1 || ( wColor0 > wColor1 ) );
						BYTE pbFinalColorRGB[ 3 ];
						if ( ( wColor0 > wColor1 ) ) 
						{
							// Four-color block: derive the other two colors.    
							// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
							// These 2-bit codes correspond to the 2-bit fields 
							// stored in the 64-bit block.
							BYTE* pbColorRGBCurr0 = pColorRGB[ 0 ];
							BYTE* pbColorRGBCurr1 = pColorRGB[ 1 ];
							BYTE* pbFinalColorRGBCurr = pbFinalColorRGB;
							for ( int nChannel = 0; nChannel < 3; nChannel++ )
							{
								*pbFinalColorRGBCurr = (nIndex == 2) ?
									(BYTE) ((2 * (WORD)(*pbColorRGBCurr0) + (WORD)(*pbColorRGBCurr1)) / 3)
									: (BYTE) (((WORD)(*pbColorRGBCurr0) + 2 * (WORD)(*pbColorRGBCurr1)) / 3);

								pbColorRGBCurr0++;
								pbColorRGBCurr1++;
								pbFinalColorRGBCurr++;
							}
						}    
						else
						{ 
							// Three-color block: derive the other color.
							// 00 = color_0,  01 = color_1,  10 = color_2,  
							// 11 = transparent.
							// These 2-bit codes correspond to the 2-bit fields 
							// stored in the 64-bit block. 
							BYTE* pbColorRGBCurr0 = pColorRGB[ 0 ];
							BYTE* pbColorRGBCurr1 = pColorRGB[ 1 ];
							BYTE* pbFinalColorRGBCurr = pbFinalColorRGB;
							for ( int nChannel = 0; nChannel < 3; nChannel++ )
							{
								*pbFinalColorRGBCurr = ( nIndex == 2 ) ?
									(BYTE) (((WORD)(*pbColorRGBCurr0) + (WORD)(*pbColorRGBCurr1)) / 2)
									: 0;
								pbColorRGBCurr0++;
								pbColorRGBCurr1++;
								pbFinalColorRGBCurr++;
							}
						}

						wColorMask565 = MAKE_565( pbFinalColorRGB[0], pbFinalColorRGB[1], pbFinalColorRGB[2]);
						bDoInCompressedSpace = TRUE;
					}

#if 1
					if ( bDoInCompressedSpace )
					{
						if ( sApplyPaletteWhileCompressed(	pbDest,
															pbDiffuseCurr,
															wColorMask565,
															pbPaletteColorsRGB,
															bDXT1 ) )
						{
#if ISVERSION(DEVELOPMENT)
							nSolidUncompressedBlocks++;
#endif
						}
					} 
					else 
					{
#if ISVERSION(DEVELOPMENT)
						BOOL result;
						if (g_bUseSquish)
						{
							result = sApplyPaletteByDecompression(pbDest, pbDiffuseCurr, pbColorMaskCurr, pbPaletteColorsRGB);
						}
						else
						{
							result = sApplyPaletteByDecompressionEx(pbDest, pbDiffuseCurr, pbColorMaskCurr, pbPaletteColorsRGB);
						}
						if (result)
						{
							++nCompressedBlocks;							
						}
#else
						sApplyPaletteByDecompressionEx(pbDest, pbDiffuseCurr, pbColorMaskCurr, pbPaletteColorsRGB);
#endif
					}
#else
					memcpy( pbDiffuseCurr + cnDXT1BytesPerPacket, pbColorMaskCurr + cnDXT1BytesPerPacket, cnDXT1BytesPerPacket );
#endif
				}

//#define WARDROBED_INVENTORY_NO_BODY
#ifdef WARDROBED_INVENTORY_NO_BODY
				if ( pData->bDontApplyBody )
				{
					BOOL bSetTransparent[ 2 ] = { FALSE, FALSE };

					for ( int i = 0; i < 2; i++ )
					{
						WORD wColorMask565 = ((WORD*)pbColorMaskCurr)[ i ];
						BYTE pbColorMaskRGB[ 3 ];
						sConvert565ToRGB( wColorMask565, pbColorMaskRGB[ 0 ], pbColorMaskRGB[ 1 ], pbColorMaskRGB[ 2 ] );

						unsigned int nPaletteIndex = 0;
						unsigned int nPaletteAlpha = 0;
						unsigned int colColorMask[ 3 ] = { pbColorMaskRGB[ 0 ], pbColorMaskRGB[ 1 ], pbColorMaskRGB[ 2 ] };
						sApplyPaletteGetIndexAndAlpha( colColorMask, nPaletteIndex, nPaletteAlpha );
				
						switch ( nPaletteIndex )
						{
						case ( COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_SKIN ):
						case ( COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_HAIR):
						case ( COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_EYES ):
							bSetTransparent[ i ] = TRUE;
							break;
						}
					}

					if ( bSetTransparent[ 0 ] || bSetTransparent[ 1 ] )
					{
						// Set color 1 > color 0, so that 1-bit alpha kicks in.
						// Set appropriate indices to transparent color and
						// remap others.
						WORD* pwColor0 = &((WORD*)pbDest)[ 0 ];
						WORD* pwColor1 = &((WORD*)pbDest)[ 1 ];
						
						BOOL bSwappedColors = FALSE;
						if ( *pwColor0 > *pwColor1 )
						{
							WORD wSwapColor = *pwColor0;
							*pwColor0 = *pwColor1;
							*pwColor1 = wSwapColor;
							bSwappedColors = TRUE;
						}
						

						BYTE* pbColorIndices = &pbDest[4];

						if( bSetTransparent[ 0 ] && bSetTransparent[ 1 ] )
							*(DWORD*)pbColorIndices = 0xffffffff; // set all to transparent.
						else
						{
							while ( pbColorIndices < ( &pbDest[ 4 ] + 
								ROWS_PER_PIXEL_BLOCK ) )
							{
								BYTE bIndices = *pbColorIndices;
								BYTE bNewIndices = 0;
								
								for (int i = 0; i < PIXELS_PER_BLOCK_ROW; i++ )
								{
									unsigned int nShift = ( i * 2);
									BYTE bIndex = ( ( bIndices >> nShift ) & 0x3 );

									switch ( bIndex )
									{
									case 3:
										if( bSwappedColors )
											bIndex = 2;
										break;
									case 1:
									case 0:
										if ( bSetTransparent[ bIndex ] )
											bIndex = 3;
										else if ( bSwappedColors )
											bIndex = ~bIndex & 0x1;
										break;
									}

									bNewIndices |= ( bIndex << nShift );
								}
								*pbColorIndices = bNewIndices;
													
								pbColorIndices++;
							}
						}
					}					
				}
#endif // WARDROBED_INVENTORY_NO_BODY

				pbDiffuseCurr   += nBytesPerBlock;
				pbColorMaskCurr += nBytesPerBlock;
			}
			pbDiffuseRowStart   += pData->nPitch[ nMipLevel ];
			pbColorMaskRowStart += pData->nPitch[ nMipLevel ];
		}
#if ISVERSION(DEVELOPMENT)
		int nSkippedBlocks = nTotalBlocks - (nCompressedBlocks + nSolidUncompressedBlocks );
/*
		trace("Level %2d  %5d compressed blocks (%5.1f%%)  %5d solid blocks (%5.1f%%)  %5d skipped blocks (%5.1f%%)\n", 
			nMipLevel, 
			nCompressedBlocks, 100.0f * (float)nCompressedBlocks / (float)nTotalBlocks,
			nSolidUncompressedBlocks, 100.0f * (float)nSolidUncompressedBlocks / (float)nTotalBlocks,
			nSkippedBlocks, 100.0f * (float)nSkippedBlocks / (float)nTotalBlocks);
*/
		nCompressedBlocks = 0;
		nSolidUncompressedBlocks = 0;
		nTotalBlocks = 0;
#endif
	}

#if ISVERSION(DEVELOPMENT)
	trace("Job: Color Masking [%08x] Execute  SQUISH:%d  TIME:%d\n", tJobData.data1, g_bUseSquish, GetTickCount() - start_tick);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombinerCleanup( 
	JOB_DATA & tJobData )
{
	//DebugString( OP_DEBUG, "Job Combiner [%08x] Cleanup", tJobData.data1 );

	COLORMASK_ASYNC_DATA * pData = (COLORMASK_ASYNC_DATA *) tJobData.data1;
	ASSERT_RETURN( pData );

	const int nNumMipLevels = pData->nNumMipLevels;
	for ( int nMipLevel = 0; nMipLevel < nNumMipLevels; nMipLevel++ )
	{
		if ( pData->pbDiffuse[ nMipLevel ] )
		{
			FREE( NULL, pData->pbDiffuse[ nMipLevel ] );
		}
		if ( pData->pbColorMask[ nMipLevel ] )
		{
			FREE( NULL, pData->pbColorMask[ nMipLevel ] );
		}
	}
	FREE( NULL, pData->pbDiffuse );
	FREE( NULL, pData->pbColorMask );
	FREE( NULL, pData->nPitch );
	FREE( NULL, pData->nHeight );
	FREE( NULL, pData->nWidth );
	FREE( NULL, pData );
	tJobData.data1 = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombinerComplete( 
	JOB_DATA & tJobData )
{
	TIMER_STARTEX2("sCombinerComplete",10);

	//DebugString( OP_DEBUG, "Job Combiner [%08x] Complete", tJobData.data1 );

	COLORMASK_ASYNC_DATA * pData = (COLORMASK_ASYNC_DATA *) tJobData.data1;
	ASSERT_RETURN( pData );

	D3D_TEXTURE * pFinal		= dxC_TextureGet( pData->nDestTexture );
	if ( pFinal && pFinal->ptSysmemTexture )
	{
		for ( int nMipLevel = 0; nMipLevel < pData->nNumMipLevels; nMipLevel++ )
		{
			ASSERT_CONTINUE( pData->nWidth [ nMipLevel ] > 0 );
			ASSERT_CONTINUE( pData->nHeight[ nMipLevel ] > 0 );

			DWORD dwDiffuseSize = dxC_GetTextureLevelSize( pData->nWidth[ nMipLevel ], pData->nHeight[ nMipLevel ], pData->eFormat );

			ASSERTV_CONTINUE( (pFinal->nWidthInPixels >> nMipLevel ) == pData->nWidth [ nMipLevel ], "nMipLevel=%d, pFinalWidth=%d, pDataWidth=%d",   nMipLevel, pFinal->nWidthInPixels, pData->nWidth[nMipLevel] );
			ASSERTV_CONTINUE( (pFinal->nHeight        >> nMipLevel ) == pData->nHeight[ nMipLevel ], "nMipLevel=%d, pFinalHeight=%d, pDataHeight=%d", nMipLevel, pFinal->nHeight,        pData->nHeight[nMipLevel] );
			ASSERTV_CONTINUE( dwDiffuseSize == pFinal->ptSysmemTexture->GetLevelSize( nMipLevel ), "nMipLevel=%d, dwSize=%d, GetLevelSize()=%d", nMipLevel, dwDiffuseSize, pFinal->ptSysmemTexture->GetLevelSize( nMipLevel ) );

			D3DLOCKED_RECT tMappedRect;
			tMappedRect.pBits = NULL;
			V( dxC_MapSystemMemTexture( pFinal->ptSysmemTexture, nMipLevel, &tMappedRect ) );

			BYTE * pbTextureStart = (BYTE *) dxC_pMappedRectData( tMappedRect );

			int nCopyFromLevel = nMipLevel;			
			if ( pData->nWidth[ nMipLevel ] < 4 )
			{
				// For compressed texture formats, the minimum texture size is a 4x4,
				// so we might as well copy from the 4x4 mip level.
				nCopyFromLevel = nMipLevel - ( 2 / pData->nWidth[ nMipLevel ] ); // this will jump either 1 or 2 mip levels.
			}
			
			if ( pbTextureStart && pData->pbDiffuse[ nCopyFromLevel ] && dwDiffuseSize )
			{
				memcpy( pbTextureStart, pData->pbDiffuse[ nCopyFromLevel ], dwDiffuseSize );
			}

			V( dxC_UnmapSystemMemTexture( pFinal->ptSysmemTexture, nMipLevel ) );

		}
	}

	if ( pFinal )
	{
		pFinal->dwFlags &= ~TEXTURE_FLAG_WARDROBE_COLOR_COMBINING;
	}

	sCombinerCleanup( tJobData ); // keep this last - it frees stuff

	TIMER_END2();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_WardrobePreloadTexture(
	int nTexture )
{
	//trace("Pre-loading wardrobe texture [%d]\n", nTexture);
	V( e_TexturePreload( nTexture ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_WardrobeCombineDiffuseAndColorMask( 
	int nDiffuseTexture,
	int nColorMaskTexture,
	int nDiffusePostColorMaskTexture,
	DWORD pdwColors[ WARDROBE_COLORMASK_TOTAL_COLORS ],
	int nQuality,
	BOOL bDontApplyBody,
	BOOL bNoAsync )
{
	TIMER_STARTEX2("e_WardrobeCombineDiffuseAndColorMask",10);

	D3D_TEXTURE * pDiffuse		= dxC_TextureGet( nDiffuseTexture   );
	D3D_TEXTURE * pColorMask	= dxC_TextureGet( nColorMaskTexture );
	D3D_TEXTURE * pFinalTexture	= dxC_TextureGet( nDiffusePostColorMaskTexture );

	if (pDiffuse == NULL || pColorMask == NULL || pFinalTexture == NULL )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pDiffuse->ptSysmemTexture );
	ASSERT_RETINVALIDARG( pColorMask->ptSysmemTexture  );
	ASSERT_RETINVALIDARG( pFinalTexture->ptSysmemTexture  );
	ASSERT_RETINVALIDARG( pDiffuse->nHeight == pColorMask->nHeight );
	ASSERT_RETINVALIDARG( pDiffuse->nD3DTopMIPSize == pColorMask->nD3DTopMIPSize );
	ASSERT_RETINVALIDARG( pDiffuse->nWidthInBytes == pColorMask->nWidthInBytes );

	ASSERT_RETINVALIDARG( pDiffuse->nFormat == pColorMask->nFormat );
	DXC_FORMAT eFormat = (DXC_FORMAT) pDiffuse->nFormat;
	ASSERT_RETINVALIDARG( dx9_IsCompressedTextureFormat( eFormat ) );

	pFinalTexture->dwFlags |= TEXTURE_FLAG_WARDROBE_COLOR_COMBINING;
	if ( TEST_MASK( pDiffuse->dwFlags, TEXTURE_FLAG_HAS_ALPHA ) )
		SET_MASK( pFinalTexture->dwFlags, TEXTURE_FLAG_HAS_ALPHA );

	COLORMASK_ASYNC_DATA * pAsyncData = (COLORMASK_ASYNC_DATA *) MALLOCZ( NULL, sizeof( COLORMASK_ASYNC_DATA ) );
	pAsyncData->bDontApplyBody = bDontApplyBody;
	memcpy( pAsyncData->pdwColors, pdwColors, sizeof( DWORD ) * WARDROBE_COLORMASK_TOTAL_COLORS );

	pAsyncData->nQuality = nQuality;
	pAsyncData->eFormat = (DXC_FORMAT) pDiffuse->nFormat;
	pAsyncData->eDecompressedFormat = DX9_BLOCK( D3DFMT_R8G8B8 ) DX10_BLOCK( D3DFMT_A8R8G8B8 );
	pAsyncData->nDestTexture		= nDiffusePostColorMaskTexture;
	//pAsyncData->nColorMaskTexture	= nColorMaskTexture;
	//pAsyncData->nDiffuseTexture		= nDiffuseTexture;

	const int nNumMipLevels = dxC_GetNumMipLevels( pDiffuse );
	pAsyncData->nNumMipLevels = nNumMipLevels;
	pAsyncData->pbDiffuse   = (BYTE**) MALLOCZ( NULL, sizeof( BYTE* ) * nNumMipLevels );
	pAsyncData->pbColorMask = (BYTE**) MALLOCZ( NULL, sizeof( BYTE* ) * nNumMipLevels );
	pAsyncData->nPitch = (int*) MALLOC( NULL, sizeof(int) * nNumMipLevels );
	pAsyncData->nHeight = (int*) MALLOC( NULL, sizeof(int) * nNumMipLevels );
	pAsyncData->nWidth = (int*) MALLOC( NULL, sizeof(int) * nNumMipLevels );

	int nMinLevelSize = 0;
	nMinLevelSize = (pAsyncData->eFormat == D3DFMT_DXT1) ? 
		cnDXT1BytesPerPacket : cnDXT2_5BytesPerPacket;

	for ( int i = 0; i < nNumMipLevels; i++ )
	{
		D3DLOCKED_RECT tDiffuseLocked;
		D3DLOCKED_RECT tColorMaskLocked;
		//D3DLOCKED_RECT tFinalLocked;

		// CML 2007.09.21 - Now that these are just non-d3d system memory bits, we can keep them "locked"
		//   until the combiner job is complete.

		V_GOTO( looperror, dxC_MapSystemMemTexture( pDiffuse->ptSysmemTexture, i, &tDiffuseLocked ) );
		ASSERTX_GOTO( dxC_pMappedRectData( tDiffuseLocked ), "Lock failed!", looperror );
		V_GOTO( looperror, dxC_MapSystemMemTexture( pColorMask->ptSysmemTexture, i, &tColorMaskLocked ) );
		ASSERTX_GOTO( dxC_pMappedRectData( tColorMaskLocked ), "Lock failed!", looperror );

		D3DC_TEXTURE2D_DESC tDiffuseDesc;
		D3DC_TEXTURE2D_DESC tColorMaskDesc;
		D3DC_TEXTURE2D_DESC tFinalDesc;
		V_GOTO( looperror, dxC_Get2DTextureDesc( pDiffuse,		i, &tDiffuseDesc ) );
		V_GOTO( looperror, dxC_Get2DTextureDesc( pColorMask,	i, &tColorMaskDesc ) );
		V_GOTO( looperror, dxC_Get2DTextureDesc( pFinalTexture,	i, &tFinalDesc ) );

		ASSERTV( tDiffuseDesc.Width     == tColorMaskDesc.Width,	"Diff.W=%d, Color.W=%d", tDiffuseDesc.Width,  tColorMaskDesc.Width );
		ASSERTV( tDiffuseDesc.Height    == tColorMaskDesc.Height,	"Diff.H=%d, Color.H=%d", tDiffuseDesc.Height, tColorMaskDesc.Height );
		ASSERTV( tDiffuseDesc.Width     == tFinalDesc.Width,		"Diff.W=%d, Final.W=%d", tDiffuseDesc.Width,  tFinalDesc.Width );
		ASSERTV( tDiffuseDesc.Height    == tFinalDesc.Height,		"Diff.H=%d, Final.H=%d", tDiffuseDesc.Height, tFinalDesc.Height );

		pAsyncData->nHeight[ i ] = tDiffuseDesc.Height;
		pAsyncData->nWidth[ i ] = tDiffuseDesc.Width;

		// I am assuming Diffuse and ColorMask pitch is the same.
		ASSERT_GOTO( dxC_nMappedRectPitch( tDiffuseLocked ) == dxC_nMappedRectPitch( tColorMaskLocked ), looperror );
		pAsyncData->nPitch[ i ]   = dxC_nMappedRectPitch( tDiffuseLocked );
	
		ASSERT_GOTO( dx9_IsCompressedTextureFormat( eFormat ), looperror );
		int nDiffuseSize   = tDiffuseLocked.  Pitch * tDiffuseDesc.  Height / 4;
		int nColorMaskSize = tColorMaskLocked.Pitch * tColorMaskDesc.Height / 4;
		nDiffuseSize = max( nDiffuseSize, nMinLevelSize );
		nColorMaskSize  = max( nColorMaskSize, nMinLevelSize );

		ASSERT( nDiffuseSize   == dxC_GetTextureLevelSize( tDiffuseDesc  .Width, tDiffuseDesc  .Height, tDiffuseDesc  .Format ) );
		ASSERT( nColorMaskSize == dxC_GetTextureLevelSize( tColorMaskDesc.Width, tColorMaskDesc.Height, tColorMaskDesc.Format ) );

		pAsyncData->pbDiffuse[ i ]   = (BYTE*) MALLOC( NULL, nDiffuseSize );
		pAsyncData->pbColorMask[ i ] = (BYTE*) MALLOC( NULL, nColorMaskSize );

		BYTE * pbDiffuseRowStart   = (BYTE *) dxC_pMappedRectData( tDiffuseLocked );
		BYTE * pbColorMaskRowStart = (BYTE *) dxC_pMappedRectData( tColorMaskLocked );

		memcpy( pAsyncData->pbDiffuse[ i ],   pbDiffuseRowStart,   nDiffuseSize );
		memcpy( pAsyncData->pbColorMask[ i ], pbColorMaskRowStart, nColorMaskSize );

		//pAsyncData->pbDiffuse[i]   = pbDiffuseRowStart;
		//pAsyncData->pbColorMask[i] = pbColorMaskRowStart;
		//continue;
looperror:
		V( dxC_UnmapSystemMemTexture( pDiffuse->ptSysmemTexture,      i ) );
		V( dxC_UnmapSystemMemTexture( pColorMask->ptSysmemTexture,    i ) );
	}

#define THREAD_COLOR_COMBINING		1
#if THREAD_COLOR_COMBINING
	if ( ! bNoAsync )
	{
		HJOB hJob = c_JobSubmit(JOB_TYPE_BIG, sCombinerExecute, JOB_DATA( (DWORD_PTR)pAsyncData ), sCombinerComplete, sCombinerCleanup);
		REF(hJob);
	}
	else
#endif
	{
		JOB_DATA tJobData( (DWORD_PTR)pAsyncData );
		sCombinerExecute( tJobData );
		sCombinerComplete( tJobData );
		REF(bNoAsync);
	}

	TIMER_END2();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_WardrobeDeviceLost()
{
	for ( D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
		pTexture;
		pTexture = dxC_TextureGetNext( pTexture ) )
	{
		if ( pTexture->nGroup == TEXTURE_GROUP_WARDROBE && pTexture->tUsage == D3DC_USAGE_2DTEX_DEFAULT )
		{
			pTexture->pD3DTexture = NULL;
			DX10_BLOCK( pTexture->pD3D10ShaderResourceView = NULL; )
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_WardrobeDeviceReset()
{
	for ( D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
		pTexture;
		pTexture = dxC_TextureGetNext( pTexture ) )
	{
		if ( pTexture->nGroup != TEXTURE_GROUP_WARDROBE || pTexture->tUsage != D3DC_USAGE_2DTEX_DEFAULT )
			continue;
		if ( ! pTexture->ptSysmemTexture )
			continue;

		// recreate the canvas D3D texture
		V_CONTINUE( sCreateCanvasD3DTexture(
			pTexture->nId,
			pTexture->nWidthInPixels,
			pTexture->nHeight, 
			(DXC_FORMAT)pTexture->nFormat, 
			(TEXTURE_TYPE)pTexture->nType ) );
		ASSERT_CONTINUE( pTexture->pD3DTexture );

		// copy up from the sysmem buffer
		V( sUpdateCanvasD3DTexture( pTexture ) );
	}

	return S_OK;
}