//-------------------------------------------------------------------------------------------------
// Ad Client
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------

#include "e_pch.h"
#include "e_adclient.h"
#include "dxC_model.h"
#include "dxC_target.h"
#include "e_main.h"
#include "complexmaths.h"

#ifdef AD_CLIENT_ENABLED

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_AdInstanceSetModelData( AD_INSTANCE_ENGINE_DATA * pData )
{
	ASSERT_RETINVALIDARG( pData );

	D3D_MODEL * pModel = dx9_ModelGet( pData->nModelID );
	ASSERT_RETINVALIDARG( pModel );

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
											e_ModelGetDisplayLOD( pData->nModelID ) );
	ASSERT_RETINVALIDARG( pModelDef );
	ASSERT_RETFAIL( pModelDef->nAdObjectDefCount > 0 );
	ASSERT_RETFAIL( IS_VALID_INDEX( pData->nAdObjectDefIndex, pModelDef->nAdObjectDefCount ) );

	if ( ! pModel->pnAdObjectInstances )
	{
		pModel->pnAdObjectInstances = (int*) MALLOC( NULL, sizeof(int) * pModelDef->nAdObjectDefCount );
		ASSERT_RETFAIL( pModel->pnAdObjectInstances );
		ASSERT( INVALID_ID == -1 );
		memset( pModel->pnAdObjectInstances, INVALID_ID, sizeof(int) * pModelDef->nAdObjectDefCount );
		pModel->nAdObjectInstances = pModelDef->nAdObjectDefCount;
	}

	ASSERT_RETFAIL( pModel->nAdObjectInstances == pModelDef->nAdObjectDefCount );

	pModel->pnAdObjectInstances[ pData->nAdObjectDefIndex ] = pData->nId;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_AdInstanceDownloadedSetTexture( AD_INSTANCE_ENGINE_DATA * pData, int nTextureID )
{
	ASSERT_RETFALSE( pData );

	D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureID );
	ASSERT_RETFALSE( pTexture );

	// we don't want the engine trying to clean this up later
	pTexture->pbLocalTextureFile = NULL;
	pTexture->pbLocalTextureData = NULL;
	pTexture->ptSysmemTexture    = NULL;


	int nModelDef = e_ModelGetDefinition( pData->nModelID );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, e_ModelGetDisplayLOD( pData->nModelID ) );
	ASSERT_RETFALSE( pModelDef );
	int nAdObjectDefCount = pModelDef->nAdObjectDefCount;
	ASSERT_RETFALSE( pModelDef->nAdObjectDefCount > 0 );
	ASSERT_RETFALSE( IS_VALID_INDEX( pData->nAdObjectDefIndex, pModelDef->nAdObjectDefCount ) );

	char szName[ MAX_ADOBJECT_NAME_LEN ];
	PStrCopy( szName, pModelDef->pAdObjectDefs[pData->nAdObjectDefIndex].szName, MAX_ADOBJECT_NAME_LEN );

	pData->nTextureID = nTextureID;
	e_TextureAddRef( nTextureID );

	// override the mesh textures for this adobject
	for ( int nLOD = 0; nLOD < LOD_COUNT; ++nLOD )
	{
		pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );

		if ( pModelDef )
		{
			for ( int nAdIndex = 0; nAdIndex < pModelDef->nAdObjectDefCount; ++nAdIndex )
			{
				BOOL bSetTexture = !!( pData->nAdObjectDefIndex == nAdIndex );

				// Find all non-reporting ad object defs with the same name and override their textures as well
				if ( ! bSetTexture && TESTBIT_DWORD(pModelDef->pAdObjectDefs[ nAdIndex ].dwFlags, AODF_NONREPORTING_BIT) )
				{
					if ( 0 == PStrICmp( szName, pModelDef->pAdObjectDefs[nAdIndex].szName, MAX_ADOBJECT_NAME_LEN ) )
					{
						bSetTexture = TRUE;
					}
				}
				if ( ! bSetTexture )
					continue;

				ASSERTX_BREAK( pModelDef->nAdObjectDefCount == nAdObjectDefCount, "Every LOD modeldef should have the same number of ad objects!" );
				ASSERT_BREAK( IS_VALID_INDEX( pModelDef->pAdObjectDefs[ nAdIndex ].nMeshIndex, pModelDef->nMeshCount ) );

				V( e_ModelSetTextureOverrideLODMesh(
					pData->nModelID,
					nLOD, 
					pModelDef->pAdObjectDefs[ nAdIndex ].nMeshIndex,
					TEXTURE_DIFFUSE,
					nTextureID,
					TRUE ) );
			}
		}
	}

	return TRUE;
}


BOOL e_AdInstanceDownloaded(int nAdInstance, const char * pAdData, unsigned int nDataLengthBytes, BOOL & bNonReporting )
{
	AD_INSTANCE_ENGINE_DATA * pData = e_AdInstanceGet( nAdInstance );
	ASSERT_RETFALSE( pData );

	bNonReporting = FALSE;

	// just got an adinstance texture, so create it
	int nTextureID;
	V_DO_FAILED( e_TextureNewFromFileInMemory(
		&nTextureID,
		(BYTE*)pAdData,
		nDataLengthBytes,
		TEXTURE_GROUP_BACKGROUND,
		TEXTURE_DIFFUSE,
		TEXTURE_LOAD_DONT_FREE_LOCAL ) )
	{
		return FALSE;
	}

	return e_AdInstanceDownloadedSetTexture( pData, nTextureID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sPointInView(
	VECTOR & vPoint_s,
	const VECTOR & vPoint_w,
	const MATRIX & mView,
	const MATRIX & mProj,
	const RECT & tVP )
{
	V_DO_FAILED( VectorProject( &vPoint_s, &vPoint_w, &mProj, &mView, NULL, &tVP ) )
	{
		return FALSE;
	}

	if (	vPoint_s.x > tVP.left && vPoint_s.x < tVP.right 
		 && vPoint_s.y > tVP.top  && vPoint_s.y < tVP.bottom
		 && vPoint_s.z > 0.f      && vPoint_s.z < 1.f )
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sClipLineSegmentToBox( VECTOR2 & vL1, VECTOR2 & vL2, const VECTOR2 & vMin, const VECTOR2 & vMax )
{
	float fIx, fIy;
	VECTOR2 vI[2];
	int nI = 0;

	if ( HLineIntersect2D( vMin.x, vMax.x, vMin.y, vL1.x, vL1.y, vL2.x, vL2.y, &fIx, &fIy ) )
	{
		ASSERT_RETFALSE( nI < 2 );
		vI[nI] = VECTOR2(fIx, fIy);
		nI++;
	}
	if ( HLineIntersect2D( vMin.x, vMax.x, vMax.y, vL1.x, vL1.y, vL2.x, vL2.y, &fIx, &fIy ) )
	{
		ASSERT_RETFALSE( nI < 2 );
		vI[nI] = VECTOR2(fIx, fIy);
		nI++;
	}
	if ( VLineIntersect2D( vMin.y, vMax.y, vMin.x, vL1.x, vL1.y, vL2.x, vL2.y, &fIx, &fIy ) )
	{
		ASSERT_RETFALSE( nI < 2 );
		vI[nI] = VECTOR2(fIx, fIy);
		nI++;
	}
	if ( VLineIntersect2D( vMin.y, vMax.y, vMax.x, vL1.x, vL1.y, vL2.x, vL2.y, &fIx, &fIy ) )
	{
		ASSERT_RETFALSE( nI < 2 );
		vI[nI] = VECTOR2(fIx, fIy);
		nI++;
	}

	if ( nI == 0 )
		return FALSE;

	if ( nI == 2 )
	{
		vL1 = vI[0];
		vL2 = vI[1];
		return TRUE;
	}

	// only one intersection -- use the endpoint in the box for one and the intersection point for the other
	if ( vL1.x > vMin.x && vL1.x < vMax.x && vL1.y > vMin.y && vL1.y < vMax.y )
	{
		vL2 = vI[0];
		return TRUE;
	}

	vL1 = vI[0];
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_AdInstanceCalculateImpression(
	int nAdInstance,
	AD_CLIENT_IMPRESSION & tImpression )
{
	AD_INSTANCE_ENGINE_DATA * pData = e_AdInstanceGet( nAdInstance );
	ASSERT_RETURN( pData );

	// initialize in case of early-out
	tImpression.bInView = FALSE;
	tImpression.fAngle  = 0.f;
	tImpression.nSize   = 0;

	const D3D_MODEL * pModel = dx9_ModelGet( pData->nModelID );
	ASSERT_RETURN( pModel );
	const D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, e_ModelGetDisplayLOD( pData->nModelID ) );
	ASSERT_RETURN( pModelDef );
	ASSERT_RETURN( pModelDef->nAdObjectDefCount > 0 );
	ASSERT_RETURN( pData->nAdObjectDefIndex >= 0 );
	ASSERT_RETURN( pData->nAdObjectDefIndex < pModelDef->nAdObjectDefCount );

	const AD_OBJECT_DEFINITION * pAdObjectDef = &pModelDef->pAdObjectDefs[pData->nAdObjectDefIndex];
	ASSERT_RETURN( pAdObjectDef );

	ASSERT_RETURN( ! TESTBIT_DWORD( pAdObjectDef->dwFlags, AODF_NONREPORTING_BIT ) );

	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FINAL_BACKBUFFER );

	RECT tVP;
	V( dxC_GetRenderTargetDimensions( (int&)tVP.right, (int&)tVP.bottom, nRT ) );
	tVP.left = 0;
	tVP.top  = 0;

	MATRIX mView, mProj;
	V( e_GetWorldViewProjMatricies( NULL, &mView, &mProj ) );

	// CML 2007.07.10: New policy: only mark not visible if the entire bounding box is not visible.

	BOOL bInView = FALSE;
	VECTOR vPoint_s[ OBB_POINTS ];

	for ( int i = 0; i < OBB_POINTS; ++i )
	{
		if ( sPointInView( vPoint_s[i], pData->tOBBw[i], mView, mProj, tVP ) )
		{
			bInView = TRUE;
			// Need to transform all the points if we're in view at all, anyway.
			//break;
		}
	}

	if ( bInView )
	{
		// get the facing angle to eye for the ad object mesh
		VECTOR vEyePos;
		VECTOR vToEye;
		e_GetViewPosition( &vEyePos );
		vToEye = vEyePos - pData->vCenterWorld;
		VectorNormalize( vToEye );
		float fAngle = VectorDot( pData->vNormalWorld, vToEye );

		// use the transformed screen vectors to get a "size on screen" value
		const int cnDiags = OBB_POINTS / 2;
		int nDiags[ cnDiags ][ 2 ] =
		{
			{ 0, 7 },
			{ 1, 6 },
			{ 2, 5 },
			{ 3, 4 }
		};
		float fMaxDist = 0.f;
		int nMaxDiag = -1;
		VECTOR2 vMin( (float)tVP.left,  (float)tVP.top    );
		VECTOR2 vMax( (float)tVP.right, (float)tVP.bottom );
		for ( int i = 0; i < cnDiags; ++i )
		{
			VECTOR vPoints[ 2 ] = { vPoint_s[ nDiags[i][0] ], vPoint_s[ nDiags[i][1] ] };

			// clip the diagonal to the screen extents
			BOOL bIntersect = sClipLineSegmentToBox(
				*(VECTOR2*)&vPoints[0],
				*(VECTOR2*)&vPoints[1],
				vMin,
				vMax );

			float fDist = VectorDistanceSquaredXY( vPoints[0], vPoints[1] );
			if ( fDist >= fMaxDist )
			{
				fMaxDist = fDist;
				nMaxDiag = i;
			}
		}
		ASSERT_RETURN( nMaxDiag >= 0 && nMaxDiag < cnDiags );
		unsigned int nSize = static_cast<unsigned int>( sqrtf( fMaxDist ) );

		tImpression.fAngle  = fAngle;
		tImpression.nSize   = nSize;
		tImpression.bInView = TRUE;

#if ISVERSION(DEVELOPMENT)
		pData->nDebugDiagPoints[ 0 ] = nDiags[ nMaxDiag ][ 0 ];
		pData->nDebugDiagPoints[ 1 ] = nDiags[ nMaxDiag ][ 1 ];
		pData->nDebugSize  = tImpression.nSize;
		pData->fDebugAngle = tImpression.fAngle;
#endif
		return;
	}

#if ISVERSION(DEVELOPMENT)
	pData->nDebugDiagPoints[ 0 ] = -1;
	pData->nDebugDiagPoints[ 1 ] = -1;
	pData->nDebugSize			 = 0;
	pData->fDebugAngle			 = -1.f;
#endif

	return;
}


#else // AD_CLIENT_ENABLED

PRESULT e_AdInstanceSetModelData( AD_INSTANCE_ENGINE_DATA * ) { return S_FALSE; }
BOOL e_AdInstanceDownloaded( int, const char*, unsigned int, BOOL & ) { return FALSE; }
void e_AdInstanceCalculateImpression( int ) {};

#endif // AD_CLIENT_ENABLED
