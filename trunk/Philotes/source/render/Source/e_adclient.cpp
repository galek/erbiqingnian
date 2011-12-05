//-------------------------------------------------------------------------------------------------
// Ad Client
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------

#include "e_pch.h"
#include "e_adclient.h"
#include "e_primitive.h"
#include "e_texture.h"
#include "e_settings.h"
#include "e_ui.h"
#include "e_model.h"

PFN_AD_CLIENT_ADD_AD gpfn_AdClientAddAd = NULL;


#ifdef AD_CLIENT_ENABLED

CHash<AD_INSTANCE_ENGINE_DATA> gtAdData;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

void e_AdClientInit()
{
	HashInit( gtAdData, NULL, 8 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_AdClientDestroy()
{
	for ( AD_INSTANCE_ENGINE_DATA * pData = HashGetFirst(gtAdData);
		pData;
		pData = HashGetNext(gtAdData, pData) )
	{
		e_TextureReleaseRef( pData->nTextureID );
	}

	HashFree( gtAdData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_AdClientAddInstance(
	const char * pszName,
	int nModelID,
	int nAdObjectDefIndex )
{
	AD_OBJECT_DEFINITION tAdObjDef;
	V( e_ModelDefinitionGetAdObjectDef( e_ModelGetDefinition( nModelID ), e_ModelGetDisplayLOD( nModelID ), nAdObjectDefIndex, tAdObjDef ) );

	// don't add non-reporting ad instances
	if ( TESTBIT_DWORD( tAdObjDef.dwFlags, AODF_NONREPORTING_BIT ) )
		return S_FALSE;


	if ( gpfn_AdClientAddAd )
	{
		int nID = gpfn_AdClientAddAd( pszName, e_AdInstanceDownloaded, e_AdInstanceCalculateImpression );
		if ( nID != INVALID_ID )
		{
			// create a data holder
			PRESULT pr = e_AdInstanceFreeAd( nID );
			ASSERTV( pr == S_FALSE, "Adding an ad instance returned an ID that already existed in the engine!" );

			AD_INSTANCE_ENGINE_DATA * pData = HashAdd( gtAdData, nID );
			pData->nModelID				= nModelID;
			pData->nAdObjectDefIndex	= nAdObjectDefIndex;
			pData->pvSamples			= NULL;
			pData->nSamples				= -1;
			pData->nTextureID			= INVALID_ID;

			V( e_AdInstanceSetModelData( pData ) );
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_AdInstanceFreeAd (
	int nInstanceID )
{
	AD_INSTANCE_ENGINE_DATA * pData = HashGet( gtAdData, nInstanceID );
	if ( pData )
	{
		e_TextureReleaseRef( pData->nTextureID );
		if ( pData->pvSamples )
		{
			FREE( NULL, pData->pvSamples );
		}
		HashRemove( gtAdData, nInstanceID );
		return S_OK;
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_AdInstanceUpdateTransform(
	int nInstance,
	AD_OBJECT_DEFINITION * pAdObjDef,
	MATRIX * pmWorld )
{
	ASSERT_RETINVALIDARG( pAdObjDef );
	ASSERT_RETINVALIDARG( pmWorld );
	AD_INSTANCE_ENGINE_DATA * pData = e_AdInstanceGet( nInstance );
	if ( ! pData )
		return S_FALSE;

	BOUNDING_BOX * pSrcBBox = &(pAdObjDef->tAABB_Obj);
	TransformAABBToOBB( pData->tOBBw, pmWorld, pSrcBBox );
	MatrixMultiply( &pData->vCenterWorld, &pAdObjDef->vCenter_Obj, pmWorld );
	MatrixMultiplyNormal( &pData->vNormalWorld, &pAdObjDef->vNormal_Obj, pmWorld );
	// unneccesary, but sane
	VectorNormalize( pData->vNormalWorld );

#if 0
	if ( ! pData->pvSamples && pData->nSamples == -1 )
	{
		const int nOrigin  = 0;
		const int nAxes[3] = { 1, 2, 4 };

		VECTOR vAxes[3];
		int nSamples[3];

		pData->nSamples = 1;
		for ( int i = 0; i < 3; ++i )
		{
			vAxes[i] = pData->tOBBw[ nAxes[i] ] - pData->tOBBw[ nOrigin ];
			float fLength = VectorNormalize( vAxes[i] );
			nSamples[i] = (int)(fLength * AD_CLIENT_MIN_SAMPLES_PER_METER) + 1;
			vAxes[i] = vAxes[i] * ( fLength / nSamples[i] );
			pData->nSamples *= nSamples[i] + 1; // +1, +1 for the endpoints
		}

		ASSERT( pData->nSamples >= 8 );
		if ( pData->nSamples <= 8 )
		{
			// this ad object is too small to need tesselation
			pData->nSamples = -1;
		}
		else
		{
			pData->pvSamples = (VECTOR*)MALLOC( NULL, sizeof(VECTOR) * pData->nSamples );

			VECTOR vO = pData->tOBBw[nOrigin];
			int nSample = 0;
			for ( int i = 0; i <= nSamples[0]; ++i )
			{
				VECTOR vI = vO + ( vAxes[0] * static_cast<float>(i) );
				for ( int j = 0; j <= nSamples[1]; ++j )
				{
					VECTOR vJ = vI + ( vAxes[1] * static_cast<float>(j) );
					for ( int k = 0; k <= nSamples[2]; ++k )
					{
						VECTOR vK = vJ + ( vAxes[2] * static_cast<float>(k) );
						pData->pvSamples[ nSample ] = vK;
						nSample++;
					}
				}
			}
		}
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)

PRESULT e_AdInstanceDebugRenderInfo(
	int nInstance )
{
	AD_INSTANCE_ENGINE_DATA * pData = HashGet( gtAdData, nInstance );
	if ( ! pData )
		return S_FALSE;

	AD_OBJECT_DEFINITION tAdObjDef;
	V( e_ModelDefinitionGetAdObjectDef( e_ModelGetDefinition( pData->nModelID ), e_ModelGetDisplayLOD( pData->nModelID ), pData->nAdObjectDefIndex, tAdObjDef ) );

	// don't display non-reporting ad objects
	if ( TESTBIT_DWORD( tAdObjDef.dwFlags, AODF_NONREPORTING_BIT ) )
		return S_FALSE;

	BOOL bImpression = FALSE;

	if ( pData->nDebugSize > 0 )
	{
		if ( pData->fDebugAngle > AD_CLIENT_DEBUG_IMPRESSION_ANGLE )
		{
			int nWidth, nHeight;
			e_GetWindowSize( nWidth, nHeight );
			unsigned int nDiag = (unsigned int)( sqrtf( static_cast<float>(nWidth * nWidth + nHeight * nHeight) ) / AD_CLIENT_DEBUG_IMPRESSION_SIZE_RATIO );
			if ( pData->nDebugSize > nDiag )
			{
				bImpression = TRUE;
			}
		}
	}

	// OBB
	if ( bImpression )
	{
		V( e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, pData->tOBBw, 0x1f002f00 ) );
	}
	V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, pData->tOBBw, 0xffffffff ) );

	// samples
	if ( pData->pvSamples )
	{
		for ( int i = 0; i < pData->nSamples; ++i )
		{
			BOUNDING_BOX tBBox;
			tBBox.vMin = pData->pvSamples[i];
			tBBox.vMax = pData->pvSamples[i];
			BoundingBoxExpandBySize( &tBBox, 0.02f );
			V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, &tBBox, 0xff00ffff ) );
		}
	}

	// normal
	VECTOR vEnd = pData->vCenterWorld + ( pData->vNormalWorld * 0.5f );
	V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &pData->vCenterWorld, &vEnd, 0xff00ffff ) );

	// max screenspace diagonal
	if ( IS_VALID_INDEX( pData->nDebugDiagPoints[0], OBB_POINTS ) && IS_VALID_INDEX( pData->nDebugDiagPoints[1], OBB_POINTS ) )
	{
		VECTOR vMaxDiags[2] = { pData->tOBBw[ pData->nDebugDiagPoints[0] ], pData->tOBBw[ pData->nDebugDiagPoints[1] ] };
		V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vMaxDiags[0], &vMaxDiags[1], 0xff0000ff ) );
	}

	WCHAR szText[256];
	PStrPrintf( szText, 256, L"\n\nInstance:  %d\nName:      %S\nTexture:   %d\nModel:     %d\nIndex:     %d\nSize:      %d\nAngle:     %4.3f",
		nInstance,
		tAdObjDef.szName,
		pData->nTextureID,
		pData->nModelID,
		pData->nAdObjectDefIndex,
		pData->nDebugSize,
		pData->fDebugAngle );
	V( e_UIAddLabel( szText, &pData->vCenterWorld, TRUE, 1.f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT, FALSE ) );

	return S_OK;
}

static PRESULT sAdInstanceDebugDownloaded( int nInstance )
{
	if ( nInstance == INVALID_ID )
		return S_FALSE;

	AD_INSTANCE_ENGINE_DATA * pData = HashGet( gtAdData, nInstance );
	if ( ! pData )
		return S_FALSE;

	AD_OBJECT_DEFINITION tAdObjDef;
	V( e_ModelDefinitionGetAdObjectDef( e_ModelGetDefinition( pData->nModelID ), e_ModelGetDisplayLOD( pData->nModelID ), pData->nAdObjectDefIndex, tAdObjDef ) );

	// don't display non-reporting ad objects
	if ( TESTBIT_DWORD( tAdObjDef.dwFlags, AODF_NONREPORTING_BIT ) )
		return S_FALSE;

	int nTexOver = e_ModelGetTextureOverride( pData->nModelID, tAdObjDef.nMeshIndex, TEXTURE_DIFFUSE, e_ModelGetDisplayLOD( pData->nModelID ) );
	if ( nTexOver != INVALID_ID )
		return S_FALSE;

	nTexOver = e_GetUtilityTexture( TEXTURE_RG_7F_BA_FF );
	BOOL bResult = e_AdInstanceDownloadedSetTexture( pData, nTexOver );
	ASSERT( bResult );

	return S_OK;
}


PRESULT e_AdInstanceDebugRenderAll()
{
	for ( AD_INSTANCE_ENGINE_DATA * pData = HashGetFirst(gtAdData);
		pData;
		pData = HashGetNext(gtAdData, pData) )
	{

		// test
		AD_CLIENT_IMPRESSION tImpression;
		e_AdInstanceCalculateImpression( pData->nId, tImpression );
		// end test

		V( e_AdInstanceDebugRenderInfo( pData->nId ) );
	}

	V( sAdInstanceDebugDownloaded( e_GetRenderFlag( RENDER_FLAG_ADOBJECT_DOWNLOADED ) ) );

	return S_OK;
}

#endif // DEVELOPMENT


#else // AD_CLIENT_ENABLED

void e_AdClientInit() {}
void e_AdClientDestroy() {}
int e_AdClientAddInstance( const char *, int, int ) { return INVALID_ID; }
PRESULT e_AdInstanceFreeAd ( int ) { return S_FALSE; }
PRESULT e_AdInstanceUpdateTransform( int, AD_OBJECT_DEFINITION*, MATRIX* ) { return S_FALSE; }

#if ISVERSION(DEVELOPMENT)
PRESULT e_AdInstanceDebugRenderInfo( int ) { return S_FALSE; }
PRESULT e_AdInstanceDebugRenderAll() { return S_FALSE; }
#endif

#endif // AD_CLIENT_ENABLED
