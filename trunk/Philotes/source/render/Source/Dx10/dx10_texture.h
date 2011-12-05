//----------------------------------------------------------------------------
// dx10_texture.h
//
// - Header for dx10 texture functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_TEXTURE__
#define __DX10_TEXTURE__

typedef VOID (WINAPI *LPD3DXFILL2D)(
    D3DXVECTOR4* pOut, 
    CONST D3DXVECTOR2* pTexCoord, 
    CONST D3DXVECTOR2* pTexelSize, 
    LPVOID pData
);

struct ArrayCreationData
{
	int nParentID;
	int nIndex;
};

PRESULT dx10_FillTexture(  SPD3DCTEXTURE2D pTexture,
  LPD3DXFILL2D pFunction,
  LPVOID pData);

PRESULT dx10_Create1DTexture(UINT Width, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE1D* ppTexture, void* pInitialData = NULL, UINT D3D10MiscFlags = NULL);
PRESULT CreateSRVFromTex1D(LPD3DCTEXTURE1D pTex, ID3D10ShaderResourceView** ppSRV, int MipLevel = -1 );
PRESULT dx10_Create2DTexture(UINT Width, UINT Height, UINT MipLevels,  D3DC_USAGE usage, DXC_FORMAT fmt, LPD3DCTEXTURE2D* ppTexture, void* pInitialData = NULL, UINT D3D10MiscFlags = NULL);
PRESULT CreateSRVFromTex2D(LPD3DCTEXTURE2D pTex, ID3D10ShaderResourceView** ppSRV, int MipLevel = -1 );
PRESULT dx10_UpdateTextureArray( SPD3DCTEXTURE2D pDest, SPD3DCTEXTURE2D pSrc, int nArrayIndex );
void dx10_ArrayIndexLoaded( int nTextureID );

PRESULT dx10_CreateCPUTex( LPD3DCTEXTURE2D pGPUTexture, LPD3DCTEXTURE2D* ppCPUTexture );
void dx10_CleanupCPUTextures();

//Smart texture pointer that has an additional pointer to a CPU texture created on demand
PRESULT dxC_MapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, D3DLOCKED_RECT* pLockedRect, bool bReadOnly = false, bool bNoDirtyUpdate = false );
PRESULT dxC_UnmapTexture( LPD3DCTEXTURE2D pD3DTexture, UINT nLevel, bool bPersistent = false, bool bUpdate = true );

PRESULT dx10_CreateTexture2D( D3D10_TEXTURE2D_DESC* pDesc, D3D10_SUBRESOURCE_DATA* pSubData, ID3D10Texture2D** ppTexture );

#endif //__DX10_TEXTURE__