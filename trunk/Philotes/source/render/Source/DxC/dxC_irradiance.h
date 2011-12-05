//----------------------------------------------------------------------------
// dx9_irradiance.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_IRRADIANCE__
#define __DX9_IRRADIANCE__

#include "e_irradiance.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define ENVMAP_GENERATE_FILENAME		"GeneratedCubeMap.dds"
#define ENVMAP_LOAD_FORMAT				D3DFMT_DXT1
#define ENVMAP_SAVE_FORMAT				DX9_BLOCK( D3DFMT_R8G8B8 ) DX10_BLOCK( D3DFMT_X8R8G8B8 )

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template <int O>
void dx9_SHCoefsInit( SH_COEFS_RGB<O> * pSHCoefs )
{
	// zero coefficients
	memset( pSHCoefs->pfRed,   0, sizeof(float) * pSHCoefs->nCoefs );
	memset( pSHCoefs->pfGreen, 0, sizeof(float) * pSHCoefs->nCoefs );
	memset( pSHCoefs->pfBlue,  0, sizeof(float) * pSHCoefs->nCoefs );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
PRESULT dx9_SHCoefsAddDirectionalLight( SH_COEFS_RGB<O> * pSHCoefs, const VECTOR4 & vLightColor, const VECTOR & vLightDir )
{
	float fCoefs[ 3 ][ pSHCoefs->nCoefs ];

	VECTOR vDir = -vLightDir;

	V_RETURN( D3DXSHEvalDirectionalLight(
		pSHCoefs->nOrder,
		(D3DXVECTOR3*)&vDir,
		vLightColor.x,
		vLightColor.y,
		vLightColor.z,
		fCoefs[0],
		fCoefs[1],
		fCoefs[2] ) );

	D3DXSHAdd( 
		pSHCoefs->pfRed,
		pSHCoefs->nOrder,
		pSHCoefs->pfRed,
		fCoefs[ 0 ] );
	D3DXSHAdd( 
		pSHCoefs->pfGreen,
		pSHCoefs->nOrder,
		pSHCoefs->pfGreen,
		fCoefs[ 1 ] );
	D3DXSHAdd( 
		pSHCoefs->pfBlue,
		pSHCoefs->nOrder,
		pSHCoefs->pfBlue,
		fCoefs[ 2 ] );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
PRESULT dx9_SHCoefsAddPointLight( SH_COEFS_RGB<O> * pCoefs, const VECTOR4 & vColor, const VECTOR & vSHPos, const VECTOR & vLightPos )
{
	VECTOR vLightDir = vSHPos - vLightPos;
	VectorNormalize( vLightDir );
	return dx9_SHCoefsAddDirectionalLight( pCoefs, vColor, vLightDir );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
PRESULT dx9_SHCoefsAddEnvMap( SH_COEFS_RGB<O> * pSHCoefs, int nEnvMapTextureID, BOOL bForceLinear )
{
	if ( nEnvMapTextureID == INVALID_ID )
		return S_FALSE;
	D3D_CUBETEXTURE * pCubeTex = dx9_CubeTextureGet( nEnvMapTextureID );
	ASSERT_RETFAIL( pCubeTex && pCubeTex->pD3DTexture );

	if ( bForceLinear )
	{
		int nMipLevels = dxC_GetNumMipLevels( pCubeTex->pD3DTexture );

		// allocate a new texture and read it into linear space
		int nTempTex;
		V( dx9_CubeTextureNewEmpty(
			&nTempTex,
			pCubeTex->nSize,
			nMipLevels,
			(DXC_FORMAT)pCubeTex->nFormat,
			pCubeTex->nGroup,
			pCubeTex->nType,
			D3DC_USAGE_CUBETEX_DYNAMIC ) );
		ASSERT_RETFAIL( nTempTex != INVALID_ID );

		D3D_CUBETEXTURE * pTempTex = dx9_CubeTextureGet( nTempTex );
		ASSERT_RETFAIL( pTempTex && pTempTex->pD3DTexture );

		dxC_CubeTextureAddRef( pTempTex );

#ifdef ENGINE_TARGET_DX9
		//unsigned char bySRGB2Lin[ 256 ];
		//for ( int i = 0; i < 256; i++ )
		//{
		//	bySRGB2Lin[ i ] = (unsigned char)( pow( (float)i / 255.f, 2.2f ) * 255.f );
		//}

		// don't know if DX10 supports this yet

		for ( int nMip = 0; nMip < nMipLevels; nMip++ )
		{
			for ( int nFace = 0; nFace < 6; nFace++ )
			{
				// copy to the dest surface, color-converting to linear space along the way

				SPDIRECT3DSURFACE9 pSrcSurf;
				V_CONTINUE( pCubeTex->pD3DTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES)nFace, nMip, &pSrcSurf ) );
				ASSERT_CONTINUE( pSrcSurf );

				SPDIRECT3DSURFACE9 pDestSurf;
				V_CONTINUE( pTempTex->pD3DTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES)nFace, nMip, &pDestSurf ) );
				ASSERT_CONTINUE( pDestSurf );

				V_CONTINUE( D3DXLoadSurfaceFromSurface( pDestSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE | D3DX_FILTER_SRGB_IN, 0 ) );

				//D3DLOCKED_RECT tLockedRect;
				//V_CONTINUE( pSurf->LockRect( &tLockedRect, NULL, FALSE ) );
				//ASSERT_CONTINUE( tLockedRect.pBits );

				//BYTE pBits = tLockedRect.pBits;
				//for ( int y = 0; y < tDesc.Height; y++ )
				//{
				//	for ( int x = 0; x < tDesc.Width; x++ )
				//	{

				//	}
				//	pBits += tLockedRect.Pitch;
				//}
			}
		}
#endif

		nEnvMapTextureID = nTempTex;
	}

	// re-grab, in case we switched textures
	pCubeTex = dx9_CubeTextureGet( nEnvMapTextureID );
	ASSERT_RETFAIL( pCubeTex && pCubeTex->pD3DTexture );

#ifdef ENGINE_TARGET_DX9
	V_RETURN( D3DXSHProjectCubeMap(
		pSHCoefs->nOrder,
		pCubeTex->pD3DTexture,
		pSHCoefs->pfRed,
		pSHCoefs->pfGreen,
		pSHCoefs->pfBlue ) );
#elif defined(ENGINE_TARGET_DX10)
	D3DX10SHProjectCubeMap( pSHCoefs->nOrder, pCubeTex->pD3DTexture, pSHCoefs->pfRed, pSHCoefs->pfGreen, pSHCoefs->pfBlue );
#endif

	if ( bForceLinear )
		dxC_CubeTextureReleaseRef( pCubeTex );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
PRESULT dx9_SHCoefsAdd( SH_COEFS_RGB<O> * ptOutCoefs, const SH_COEFS_RGB<O> * ptCoefs1, const SH_COEFS_RGB<O> * ptCoefs2 )
{
	ASSERT_RETINVALIDARG( ptOutCoefs && ptCoefs1 && ptCoefs2 );
	ASSERT_RETINVALIDARG( ptOutCoefs->nOrder == ptCoefs1->nOrder );
	ASSERT_RETINVALIDARG( ptCoefs1->nOrder	  == ptCoefs2->nOrder );

	D3DXSHAdd( 
		ptOutCoefs->pfRed,
		ptCoefs1->nOrder,
		ptCoefs1->pfRed,
		ptCoefs2->pfRed );
	D3DXSHAdd( 
		ptOutCoefs->pfGreen,
		ptCoefs1->nOrder,
		ptCoefs1->pfGreen,
		ptCoefs2->pfGreen );
	D3DXSHAdd( 
		ptOutCoefs->pfBlue,
		ptCoefs1->nOrder,
		ptCoefs1->pfBlue,
		ptCoefs2->pfBlue );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
PRESULT dx9_SHCoefsScale( SH_COEFS_RGB<O> * ptOutCoefs, const SH_COEFS_RGB<O> * ptInCoefs, float fScale )
{
	ASSERT_RETINVALIDARG( ptOutCoefs && ptInCoefs );
	ASSERT_RETINVALIDARG( ptOutCoefs->nOrder == ptInCoefs->nOrder );

	D3DXSHScale( 
		ptOutCoefs->pfRed,
		ptInCoefs->nOrder,
		ptInCoefs->pfRed,
		fScale );
	D3DXSHScale( 
		ptOutCoefs->pfGreen,
		ptInCoefs->nOrder,
		ptInCoefs->pfGreen,
		fScale );
	D3DXSHScale( 
		ptOutCoefs->pfBlue,
		ptInCoefs->nOrder,
		ptInCoefs->pfBlue,
		fScale );

	return S_OK;
}

#endif // __DX9_IRRADIANCE__