//----------------------------------------------------------------------------
// dx10_texture.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_texture.h"
#include "dxC_state.h"

#include <map>

std::map<LPD3DCTEXTURE2D,SPD3DCTEXTURE2D> gmCPUTextures;
typedef std::map<LPD3DCTEXTURE2D,SPD3DCTEXTURE2D>::iterator CPUTextureMapIterator;

void dx10_CleanupCPUTextures()
{
	CPUTextureMapIterator tIter = gmCPUTextures.begin();
	while ( tIter != gmCPUTextures.end() )
	{
		SPD3DCTEXTURE2D& tCPUTex = tIter->second;
		tCPUTex = NULL;
		tIter++;
	}
	gmCPUTextures.clear();
}

PRESULT dxC_MapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, D3DLOCKED_RECT* pLockedRect, bool bReadOnly, bool bNoDirtyUpdate )
{
	REF( bNoDirtyUpdate );

	ASSERT_RETFAIL( pD3DTexture );
	if( !gmCPUTextures[ pD3DTexture ] )
	{
		V_RETURN( dx10_CreateCPUTex( pD3DTexture, &gmCPUTextures[ pD3DTexture ] ) );
	}

	ASSERT_RETFAIL( gmCPUTextures[ pD3DTexture ] );

	V( gmCPUTextures[ pD3DTexture ]->Map( nLevel, bReadOnly ? D3D10_MAP_READ : D3D10_MAP_READ_WRITE, 0, pLockedRect ) );

	return S_OK;
}

PRESULT dxC_UnmapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, bool bPersistent, bool bUpdate /*=true*/ )
{
	ASSERT_RETFAIL( gmCPUTextures[ pD3DTexture ] );

	gmCPUTextures[ pD3DTexture ]->Unmap( nLevel );

	if( bUpdate && ( gmCPUTextures[ pD3DTexture ] != pD3DTexture ) ) //Don't want to copy to ourselves
	{
		dxC_GetDevice()->CopySubresourceRegion( pD3DTexture, nLevel, 0, 0, 0, 
			gmCPUTextures[ pD3DTexture ], nLevel, NULL );
	}

	if( !bPersistent )
	{
		gmCPUTextures[ pD3DTexture ] = NULL;
		gmCPUTextures.erase( pD3DTexture );
	}

	return S_OK;
}

PRESULT dx10_FillTexture( SPD3DCTEXTURE2D pTexture,
	LPD3DXFILL2D pFunction,
	LPVOID pData)
{
	D3D10_TEXTURE2D_DESC desc;
	pTexture->GetDesc(&desc);

	if (desc.MipLevels != 1) {
		ASSERTX_RETFAIL(0, "NUTTAPONG COMMENT: Support only 1 mip level for now");
	}

	DWORD bytePixel = dx9_GetTextureFormatBitDepth( desc.Format ) / 8;

	if ( bytePixel > 4 ) {
		ASSERTX_RETFAIL(0, "Only support 32 bit or less here");
	}
	D3DXVECTOR2 texelSize(1.0f / desc.Width, 1.0f / desc.Height);

	D3DXVECTOR4 color;
	D3DLOCKED_RECT mapped2DTex;
	BYTE* pPixel = NULL;

	for( DWORD n = 0; n < desc.ArraySize; ++n )
	{
		if( !SUCCEEDED( dxC_MapTexture( pTexture, D3D10CalcSubresource( 0, n, desc.MipLevels ), &mapped2DTex ) ) )
			return E_FAIL;

		pPixel = (BYTE*)mapped2DTex.pData;

		for (DWORD i = 0; i < desc.Height; i++)
		{
			for (DWORD j = 0; j < desc.Width; j++)
			{
				D3DXVECTOR2 texelPos((j+0.5f)*texelSize.x, (i+0.5f)*texelSize.y);
				pFunction(&color, &texelPos, &texelSize, pData);
				DWORD dwColor = ((((((DWORD(color.w*255.0f+0.5f)) << 8)+(DWORD(color.z*255.0f+0.5f))) << 8) + (DWORD(color.y*255.0f+0.5f))) << 8) + (DWORD(color.x*255.0f+0.5f));
				memcpy( &pPixel[ i * desc.Width * bytePixel + j * bytePixel ], &dwColor, bytePixel );
			}
		}

		dxC_UnmapTexture( pTexture, D3D10CalcSubresource( 0, n, desc.MipLevels ) );
	}
	

	return S_OK;
}

PRESULT CreateSRVFromTex1D( LPD3DCTEXTURE1D pTex, ID3D10ShaderResourceView** ppSRV, int MipLevel )
{
	ASSERTX_RETINVALIDARG( pTex, "Texture pointer NULL!!" );
	D3D10_TEXTURE1D_DESC TextureDesc;
	pTex->GetDesc( &TextureDesc );

	D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = dx10_ResourceFormatToSRVFormat( TextureDesc.Format );

	SRVDesc.ViewDimension = ( TextureDesc.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ) ? D3D10_SRV_DIMENSION_TEXTURECUBE : D3D10_SRV_DIMENSION_TEXTURE1DARRAY;
	SRVDesc.Texture1DArray.ArraySize = TextureDesc.ArraySize;
	SRVDesc.Texture1DArray.FirstArraySlice = 0;
	SRVDesc.Texture1DArray.MipLevels =  MipLevel == -1 ? -1 : 1;
	SRVDesc.Texture1DArray.MostDetailedMip = MipLevel == -1 ? 0 : MipLevel - 1;

	V_RETURN( dxC_GetDevice()->CreateShaderResourceView( pTex, &SRVDesc, ppSRV ) );
	return S_OK;
}

PRESULT dx10_Create1DTexture(UINT Width, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE1D* ppTexture, void* pInitialData, UINT D3D10MiscFlags )
{
	D3D10_TEXTURE1D_DESC Desc;
	Desc.ArraySize = D3D10MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ? 6 : 1;
	Desc.BindFlags = dx10_GetBindFlags( usage );
	Desc.CPUAccessFlags = dx10_GetCPUAccess( usage );
	Desc.Format = fmt;
	Desc.MipLevels = MipLevels;
	Desc.MiscFlags = D3D10MiscFlags;
	Desc.Usage = dx10_GetUsage( usage );
	Desc.Width = Width;

	if( pInitialData )
	{
		D3D10_SUBRESOURCE_DATA subData;
		subData.pSysMem = pInitialData;
		subData.SysMemPitch = dxC_GetTexturePitch( Width, fmt );
		subData.SysMemSlicePitch = 0;

		return dxC_GetDevice()->CreateTexture1D( &Desc, &subData, ppTexture );
	}
	else
		return dxC_GetDevice()->CreateTexture1D( &Desc, NULL, ppTexture );
}

PRESULT CreateSRVFromTex2D( LPD3DCTEXTURE2D pTex, ID3D10ShaderResourceView** ppSRV, int MipLevel )
{
	ASSERTX_RETINVALIDARG( pTex, "Texture pointer NULL!!" );
	D3D10_TEXTURE2D_DESC TextureDesc;
	pTex->GetDesc( &TextureDesc );

	D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = dx10_ResourceFormatToSRVFormat( TextureDesc.Format );

	int iMipLevels = MipLevel == -1 ? -1 : 1;
	int iMostDetailedMip = MipLevel == -1 ? 0 : MipLevel;

	if( TextureDesc.SampleDesc.Count > 1 )
	{
		SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY;
		SRVDesc.Texture2DMSArray.FirstArraySlice = 0;
		SRVDesc.Texture2DMSArray.ArraySize = TextureDesc.ArraySize;
	}
	else if( TextureDesc.ArraySize > 1 )
	{
		SRVDesc.ViewDimension = ( TextureDesc.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ) ? D3D10_SRV_DIMENSION_TEXTURECUBE : D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
		SRVDesc.Texture2DArray.MipLevels =  iMipLevels;
		SRVDesc.Texture2DArray.MostDetailedMip = iMostDetailedMip;
	}
	else
	{
		SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = iMipLevels;
		SRVDesc.Texture2D.MostDetailedMip = iMostDetailedMip;
	}

	V_RETURN( dxC_GetDevice()->CreateShaderResourceView( pTex, &SRVDesc, ppSRV ) );
	return S_OK;
}

HRESULT dx10_Create2DTexture(UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE2D* ppTexture, void* pInitialData, UINT D3D10MiscFlags )
{
	D3D10_TEXTURE2D_DESC Desc;
	Desc.ArraySize = D3D10MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE ? 6 : 1;
	Desc.BindFlags = dx10_GetBindFlags( usage );
	Desc.CPUAccessFlags = dx10_GetCPUAccess( usage );
	Desc.Format = fmt;
	Desc.MipLevels = MipLevels;
	Desc.MiscFlags = (MipLevels == 0 && usage == D3DC_USAGE_2DTEX_RT ) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0 | D3D10MiscFlags;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0; //Not handiling AA yet
	Desc.Usage = dx10_GetUsage( usage );
	Desc.Width = Width;
	Desc.Height = Height;

	if( pInitialData )
	{
		D3D10_SUBRESOURCE_DATA subData;
		subData.pSysMem = pInitialData;
		subData.SysMemPitch = dxC_GetTexturePitch( Width, fmt );
		subData.SysMemSlicePitch = 0;

		return dx10_CreateTexture2D( &Desc, &subData, ppTexture );
	}
	else
		return dx10_CreateTexture2D( &Desc, NULL, ppTexture );
}

PRESULT dx10_UpdateTextureArray( SPD3DCTEXTURE2D pDest, SPD3DCTEXTURE2D pSrc, int nArrayIndex )
{
	int nSrcMipLevels = dxC_GetNumMipLevels( pSrc );
	int nDestMipLevels = dxC_GetNumMipLevels( pDest );

	for(int i = 0; i < min( nSrcMipLevels, nDestMipLevels ); ++i)
	{
		dxC_GetDevice()->CopySubresourceRegion( pDest, D3D10CalcSubresource( i, nArrayIndex, nDestMipLevels ), 0, 0, 0, pSrc, D3D10CalcSubresource( i, 0, nSrcMipLevels ), NULL );
	}

	return S_OK;
}

void dx10_ArrayIndexLoaded( int nTextureID )
{
	D3D_TEXTURE* pTexture = dxC_TextureGet( nTextureID );
	ArrayCreationData* pCreationData = (ArrayCreationData*)pTexture->pArrayCreationData;
	ASSERTX( pCreationData, "Creation data is missing!  I don't know what texture this belongs to..." );
	ASSERT( pTexture->pD3DTexture );

	D3D_TEXTURE* pTexArray = dxC_TextureGet( pCreationData->nParentID );
	V( dx10_UpdateTextureArray( pTexArray->pD3DTexture, pTexture->pD3DTexture, pCreationData->nIndex ) );

	//Destroy self, temporary texture
	V( e_TextureRemove( pTexture->nId ) );
}

HRESULT dx10_CreateCPUTex( LPD3DCTEXTURE2D pGPUTexture, LPD3DCTEXTURE2D* ppCPUTexture )
{
	ASSERT_RETINVALIDARG( pGPUTexture && ppCPUTexture );

	D3DC_TEXTURE2D_DESC texDesc;
	V_RETURN( dxC_Get2DTextureDesc( pGPUTexture, 0, &texDesc ) );

	if(    texDesc.Usage == D3D10_USAGE_STAGING 
		/*|| texDesc.Usage == D3D10_USAGE_DYNAMIC*/ )  //We're already a CPU texture, no need to duplicate
	{
		*ppCPUTexture = pGPUTexture;
		(*ppCPUTexture)->AddRef();

		return S_OK;
	}

	V_RETURN( dxC_Create2DTexture( texDesc.Width, texDesc.Height, texDesc.MipLevels, D3DC_USAGE_2DTEX_STAGING, texDesc.Format, ppCPUTexture, NULL, texDesc.MiscFlags ) );

	ASSERT_RETFAIL( *ppCPUTexture );

	dxC_GetDevice()->CopyResource( *ppCPUTexture, pGPUTexture );
	
	return S_OK;
}

PRESULT dx10_CreateTexture2D( D3D10_TEXTURE2D_DESC* pDesc, D3D10_SUBRESOURCE_DATA* pSubData, ID3D10Texture2D** ppTexture )
{
	return dxC_GetDevice()->CreateTexture2D( pDesc, pSubData, ppTexture );
}
