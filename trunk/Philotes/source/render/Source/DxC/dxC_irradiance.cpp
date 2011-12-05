//----------------------------------------------------------------------------
// dx9_irradiance.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_model.h"
#include "dxC_drawlist.h"
#include "dxC_target.h"
#include "dxC_render.h"
#include "dxC_shadow.h"
#include "dxC_environment.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "performance.h"
#include "filepaths.h"
#include "dxC_pixo.h"


// debug
#include "dxC_caps.h"

#include "dxC_irradiance.h"
#include "dxC_texture.h"


using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_GenerateEnvironmentMap( int nSizeOverride, const char * pszFilenameOverride, DWORD dwFlags )
{
#ifdef ENGINE_TARGET_DX10  //KMNV TODO: Single pass in DX10
	return;
#endif

	DWORD dwCubeMapFlags = e_CubeMapFlags();
	dwCubeMapFlags = dwFlags;

	int & nCubeMapSize = e_CubeMapSize();
	if ( nSizeOverride > 0 && ( 0 == ( nSizeOverride & (nSizeOverride-1) ) ) )
		nCubeMapSize = nSizeOverride;
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );
	if ( nCubeMapSize > nWidth || nCubeMapSize > nHeight )
		nCubeMapSize = 128;

	char * pszFilename = e_CubeMapFilename();
	if ( pszFilenameOverride && pszFilenameOverride[ 0 ] )
	{
		PStrCopy( pszFilename, pszFilenameOverride, DEFAULT_FILE_WITH_PATH_SIZE );
	}
	else
	{
		ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
		if ( ! pEnvDef )
		{
			ErrorDialog( "Cannot generate env map without a current environment or specific filename specified!" );
			return;
		}
		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		//char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrGetPath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, pEnvDef->tHeader.pszName );
		PStrPrintf( pszFilename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", FILE_PATH_BACKGROUND, szPath, ENVMAP_GENERATE_FILENAME );
	}

	e_CubeMapCurrentFace() = 0;
	e_CubeMapSetGenerating( TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetupCubeMapMatrices(
	MATRIX *pmatView, 
	MATRIX *pmatProj, 
	MATRIX *pmatProj2, 
	VECTOR *pvEyeVector, 
	VECTOR *pvEyeLocation )
{
	// Use 90-degree field of view in the projection
	D3DXMatrixPerspectiveFovLH( (D3DXMATRIXA16*)pmatProj, (float)D3DX_PI / 2.0f, 1.0f, e_GetNearClipPlane(), e_GetFarClipPlane() );

	// Standard view that will be overridden below
	VECTOR vEnvEyePt = *pvEyeLocation;
	VECTOR vLookatPt, vUpVec;

	switch( e_CubeMapCurrentFace() )
	{
	case D3DCUBEMAP_FACE_POSITIVE_X:
		vLookatPt = VECTOR(1.0f, 0.0f, 0.0f);
		vUpVec    = VECTOR(0.0f, 1.0f, 0.0f);
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_X:
		vLookatPt = VECTOR(-1.0f, 0.0f, 0.0f);
		vUpVec    = VECTOR( 0.0f, 1.0f, 0.0f);
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Y:
		vLookatPt = VECTOR(0.0f, 1.0f, 0.0f);
		vUpVec    = VECTOR(0.0f, 0.0f,-1.0f);
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Y:
		vLookatPt = VECTOR(0.0f,-1.0f, 0.0f);
		vUpVec    = VECTOR(0.0f, 0.0f, 1.0f);
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Z:
		vLookatPt = VECTOR( 0.0f, 0.0f, 1.0f);
		vUpVec    = VECTOR( 0.0f, 1.0f, 0.0f);
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Z:
		vLookatPt = VECTOR(0.0f, 0.0f,-1.0f);
		vUpVec    = VECTOR(0.0f, 1.0f, 0.0f);
		break;
	}

	vLookatPt += vEnvEyePt;
	D3DXMatrixLookAtLH(
		(D3DXMATRIXA16*)pmatView,
		(D3DXVECTOR3*)&vEnvEyePt,
		(D3DXVECTOR3*)&vLookatPt,
		(D3DXVECTOR3*)&vUpVec );

	// make the proj2 matrix 0-scale, forcing stuff that uses it to not draw
	if ( pmatProj2 )
		ZeroMemory( pmatProj2, sizeof(D3DXMATRIXA16) );

	*pvEyeVector = vLookatPt - vEnvEyePt;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetupCubeMapGeneration()
{
	if ( e_GeneratingCubeMap() )
	{
		ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
		ASSERT( pEnvDef );

		e_CubeTextureReleaseRef( gnEnvMapTextureID );
		gnEnvMapTextureID = INVALID_ID;

		e_SetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS,		FALSE );
		e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				FALSE );
		e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			FALSE );
		e_CubeMapCurrentFace() = 0;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_CubeMapGenSetViewport()
{
	dxC_SetViewport( 0, 0, e_CubeMapSize(), e_CubeMapSize(), 0.0f, 1.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SaveCubeMapFace()
{
#ifdef ENGINE_TARGET_DX9
	if ( e_GeneratingCubeMap() )
	{
		// store off cube map face
		//dxC_SetRenderTarget( RENDER_TARGET_BACKBUFFER );
		V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, dxC_GetCurrentDepthTarget() ) );

		ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
		ASSERT( pEnvDef );

		D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( gnEnvMapTextureID );

		if ( ! pTexture || ! pTexture->pD3DTexture )
		{
			// create texture
			if ( ! pTexture )
			{
				V( dx9_CubeTextureAdd( &gnEnvMapTextureID, &pTexture, TEXTURE_GROUP_BACKGROUND, TEXTURE_ENVMAP ) );
				e_CubeTextureAddRef( gnEnvMapTextureID );
			}
			ASSERT_RETURN( pTexture && ! pTexture->pD3DTexture );

			// could generate mips here, but instead I'm generating on save

			V( D3DXCreateCubeTexture( dxC_GetDevice(),
				e_CubeMapSize(),
				1,
				0,
				ENVMAP_SAVE_FORMAT,
				D3DPOOL_MANAGED,
				&pTexture->pD3DTexture ) );
		}
		ASSERT( pTexture && pTexture->pD3DTexture );
		SPDIRECT3DSURFACE9 pDestSystemSurf, pBackBufferSurf;
		SPDIRECT3DSURFACE9 pFace;
		V( pTexture->pD3DTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES)e_CubeMapCurrentFace(), 0, &pFace ) );

		RECT tRect;
		SetRect( &tRect, 0, 0, e_CubeMapSize(), e_CubeMapSize() );

		// this is not the fastest way to do this, but it isn't a run-time thing, so no big deal
		RENDER_TARGET_INDEX nFullRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FULL );
		if ( ! dxC_RTSystemTextureGet( nFullRT ) )
		{

			int nWidth, nHeight;
			dxC_GetRenderTargetDimensions( nWidth, nHeight, nFullRT );
			DXC_FORMAT tFmt = dxC_GetRenderTargetFormat( nFullRT );
			V( D3DXCreateTexture( dxC_GetDevice(), nWidth, nHeight, 1,
				D3DX_DEFAULT, tFmt, D3DPOOL_SYSTEMMEM, &dxC_RTSystemTextureGet( nFullRT ) ) );
		}

		V( dxC_GetDevice()->GetBackBuffer( 
			0, 
			0, 
			D3DBACKBUFFER_TYPE_MONO, 
			&pBackBufferSurf ) );
		V( dxC_GetDevice()->StretchRect(
			pBackBufferSurf, 
			&tRect, 
			dxC_RTSurfaceGet( nFullRT ), 
			&tRect, 
			D3DTEXF_LINEAR ) );
		V( dxC_RTSystemTextureGet( nFullRT )->GetSurfaceLevel( 0, &pDestSystemSurf ) );
		V( dxC_GetDevice()->GetRenderTargetData( 
			dxC_RTSurfaceGet( nFullRT ), 
			pDestSystemSurf ) );
		V( D3DXLoadSurfaceFromSurface(
			pFace, 
			NULL, 
			&tRect, 
			pDestSystemSurf, 
			NULL, 
			&tRect, 
			D3DX_FILTER_NONE, 
			0 ) );

		e_CubeMapCurrentFace()++;
	}
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SaveFullCubeMap()
{
	if ( e_GeneratingCubeMap() )
	{
		// save cube map
		D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( gnEnvMapTextureID );
		ASSERT( pTexture && pTexture->pD3DTexture );

		char * pszFilePath = e_CubeMapFilename();
		ASSERT( pszFilePath && pszFilePath[ 0 ] );
		if ( FileIsReadOnly( pszFilePath ) )
		{
			DebugString( OP_ERROR, "ERROR: Could not overwrite read-only cube map file: \n%s", pszFilePath );
		}
		else
		{
			// make a copy of the texture to generate mip maps before saving

			SPD3DCBLOB pBuffer;
			SPD3DCTEXTURECUBE pNewCubeTex;
			V( dxC_SaveTextureToMemory( pTexture->pD3DTexture, D3DXIFF_DDS, &pBuffer) );
			V( dxC_CreateCubeTextureFromFileInMemoryEx(
				pBuffer->GetBufferPointer(),
				pBuffer->GetBufferSize(),
				0,
				0,
				D3DC_USAGE_2DTEX_STAGING,
				ENVMAP_SAVE_FORMAT,
				D3DX_FILTER_POINT,
				D3DX_FILTER_LINEAR,
				NULL,
				&pNewCubeTex ) );

			pBuffer = NULL;
			V( dxC_SaveTextureToMemory( pNewCubeTex, D3DXIFF_DDS, &pBuffer) );
			FileSave( pszFilePath, pBuffer->GetBufferPointer(), pBuffer->GetBufferSize() );
		}

		e_CubeMapSetGenerating( FALSE );

		if ( e_CubeMapFlags() & CUBEMAP_GEN_FLAG_APPLY_TO_ENV )
		{
			// release old env cube map
			ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
			if ( pEnvDef )
			{
				D3D_CUBETEXTURE * pEnvTexture = dx9_CubeTextureGet( pEnvDef->nEnvMapTextureID );
				// this releases the old d3d texture and addrefs the new one
				pEnvTexture->pD3DTexture = pTexture->pD3DTexture;
				DX10_BLOCK( pEnvTexture->pD3D10ShaderResourceView = NULL; )
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_GetSHCoefsFromCubeMap(
	__in int nCubeMapID,
	__out SH_COEFS_RGB<2> & tCoefs2,
	__out SH_COEFS_RGB<3> & tCoefs3,
	BOOL bForceLinear )
{
	dx9_SHCoefsInit( &tCoefs2 );
	dx9_SHCoefsInit( &tCoefs3 );

	// CML - This doesn't work in Pixo because we don't have a cubemap texture to use with D3DX SH Cubemap functions.
	if ( ! dxC_IsPixomaticActive() )
	{
		// just checking
		D3D_CUBETEXTURE * pTex = dx9_CubeTextureGet( nCubeMapID );
		if ( ! pTex )
			return S_FALSE;

		V( dx9_SHCoefsAddEnvMap( &tCoefs2, nCubeMapID, bForceLinear ) );
		V( dx9_SHCoefsAddEnvMap( &tCoefs3, nCubeMapID, bForceLinear ) );
	}

	return S_OK;
}
