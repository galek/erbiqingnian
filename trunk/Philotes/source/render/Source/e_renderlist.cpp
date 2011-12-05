//----------------------------------------------------------------------------
// e_renderlist.cpp
//
// - Implementation for renderable object/effect and render list abstractions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


#include "perfhier.h"
#include "plane.h"
#include "boundingbox.h"
#include "e_frustum.h"
#include "e_settings.h"
#include "e_model.h"
#include "e_main.h"
#include "e_region.h"
#include "e_portal.h"

#include "e_renderlist.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum RENDERLIST_STATE
{
	RLSTATE_INVALID = -1,
	RLSTATE_INACTIVE = 0,
	RLSTATE_ACTIVE,
	// count
	NUM_RENDERLIST_STATES
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static RENDERLIST_STATE							sgeRenderListState = RLSTATE_INVALID;

static RENDERLIST_PASS							sgtRenderListPasses[ MAX_RENDERLIST_PASSES ];
static int										sgnRenderListPasses;
static int										sgpnRenderListSortedPasses[ MAX_RENDERLIST_PASSES ];

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

static RENDERLIST_PASS * sRenderListPassGet( PASSID nPass )
{
	ASSERT_RETNULL( IS_VALID_INDEX( nPass, sgnRenderListPasses ) );
	return & sgtRenderListPasses[ nPass ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RenderListInit()
{
	for ( int i = 0; i < MAX_RENDERLIST_PASSES; i++ )
	{
		ArrayInit( sgtRenderListPasses[ i ].tRenderList, NULL, 4 );
		ArrayInit( sgtRenderListPasses[ i ].tModelList, NULL, 4 );
	}
	sgeRenderListState = RLSTATE_INACTIVE;
	sgnRenderListPasses = 0;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListClear()
{
	sgnRenderListPasses = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RenderListDestroy()
{
	for ( int i = 0; i < MAX_RENDERLIST_PASSES; i++ )
	{
		sgtRenderListPasses[ i ].tRenderList.Destroy();
		sgtRenderListPasses[ i ].tModelList.Destroy();
	}
	sgeRenderListState = RLSTATE_INVALID;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListBegin()
{
	ASSERT_RETURN( sgeRenderListState == RLSTATE_INACTIVE );
	sgeRenderListState = RLSTATE_ACTIVE;

	e_RenderListClear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PASSID e_RenderListBeginPass( const RL_PASS_DESC & tDesc, int nCurRegion )
{
	ASSERT_RETINVALID( sgeRenderListState == RLSTATE_ACTIVE );
	if ( ( sgnRenderListPasses + 1 ) >= MAX_RENDERLIST_PASSES )
	{
		ASSERTV_RETINVALID( 0, "Too many render list passes:  cur: %d  max: %d", sgnRenderListPasses, MAX_RENDERLIST_PASSES );
	}

	PASSID nPass = sgnRenderListPasses;
	sgnRenderListPasses++;
	RENDERLIST_PASS & tPass = sgtRenderListPasses[ nPass ];

	tPass.eType			= tDesc.eType;
	tPass.dwFlags		= _RL_PASS_FLAG_OPEN | tDesc.dwFlags;
	ArrayClear( tPass.tRenderList );
	tPass.nPortalID		= INVALID_ID;
	tPass.nRegionID		= nCurRegion;
	tPass.nDrawLayer	= DRAW_LAYER_GENERAL;		// in theory, we could override this

	// this will control whether we occlusion query
	switch ( e_RegionGetLevelLocation( tPass.nRegionID ) )
	{
	case LEVEL_LOCATION_INDOOR:
	case LEVEL_LOCATION_INDOORGRID:
		tPass.eCull = RL_PASS_MODEL_CULL_INDOOR;
		break;
	case LEVEL_LOCATION_OUTDOOR:
	default:
		tPass.eCull = RL_PASS_MODEL_CULL_OUTDOOR;
		break;
	}

	switch ( tDesc.eType )
	{
	case RL_PASS_PORTAL:
		tPass.nPortalID = tDesc.nPortalID;
		break;
	}

	return nPass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListEndPass( PASSID nPass )
{
	ASSERT_RETURN( IS_VALID_INDEX( nPass, sgnRenderListPasses ) );
	ASSERT_RETURN( sgeRenderListState == RLSTATE_ACTIVE );

	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERT_RETURN( pPass->dwFlags & _RL_PASS_FLAG_OPEN );

	pPass->dwFlags &= ~_RL_PASS_FLAG_OPEN;

	int nCurRegion = e_GetCurrentRegionID();
	int nNewRegion = pPass->nRegionID;

	if ( pPass->eType == RL_PASS_PORTAL )
	{
		PORTAL * pPortal = e_PortalGet( pPass->nPortalID );
		if ( pPortal && pPortal->pConnection )
		{
			nNewRegion = pPortal->pConnection->nRegion;
			VECTOR vEye, vLookat;
			e_GetViewPosition( &vEye );
			e_GetViewFocusPosition( &vLookat );
			e_PortalUpdateMatrices( pPortal, vEye, vLookat );
			//e_SetCurrentViewOverrides( pPortal );
		}
	}

	V( e_SetRegion( nNewRegion ) );

	// finalize renderlist pass

	e_RenderListPassCull( nPass );
	e_RenderListPassBuildModelList( nPass );
	e_RenderListPassMarkVisible( nPass );

	V( e_SetRegion( nCurRegion ) );
	//if ( pPass->eType == RL_PASS_PORTAL )
	//	e_SetCurrentViewOverrides( NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListEnd()
{
	ASSERT_RETURN( sgeRenderListState == RLSTATE_ACTIVE );
	sgeRenderListState = RLSTATE_INACTIVE;

	for ( int i = 0; i < sgnRenderListPasses; i++ )
	{
		RENDERLIST_PASS * pPass = sRenderListPassGet( i );
		ASSERT_CONTINUE( pPass );
		ASSERTX( 0 == ( pPass->dwFlags & _RL_PASS_FLAG_OPEN ), "Pass still open when ending renderlist!" );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_RenderListAdd(
	PASSID			nPass,
	RENDERABLE_TYPE eType,
	DWORD			dwSubType,
	DWORD_PTR		dwData,
	DWORD			dwFlags )
{
	ASSERT_RETINVALID( sgeRenderListState == RLSTATE_ACTIVE );

	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETINVALID( pPass );
	ASSERTX_RETINVALID( pPass->dwFlags & _RL_PASS_FLAG_OPEN, "Tried to add renderable to closed pass!" );

	int nIndex;
	Renderable tRenderable;

	tRenderable.eType		= eType;
	tRenderable.dwData		= dwData;
	tRenderable.dwFlags		= dwFlags & ~_RENDERABLE_FLAG_PASSED_CULL;
	tRenderable.nParent		= INVALID_ID;

	tRenderable.tBBox.vMin	= VECTOR( -1.0f, -1.0f,  0.0f );
	tRenderable.tBBox.vMax	= VECTOR(  1.0f,  1.0f,  1.0f );

	nIndex = ArrayAddItem( pPass->tRenderList, tRenderable );

	return nIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_RenderListAddModel(
	PASSID	nPass,
	int		nModelID,
	DWORD	dwFlags /*=0*/ )
{
	ASSERT_RETINVALID( nModelID != INVALID_ID );
	return e_RenderListAdd( nPass, RENDERABLE_MODEL, 0, (DWORD)nModelID, dwFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListSetParent(
	PASSID	nPass,
	int		nIndex,
	int		nParent )
{
	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERTX_RETURN( pPass->dwFlags & _RL_PASS_FLAG_OPEN, "Tried to set parent on renderable in closed pass!" );

	int nCount = pPass->tRenderList.Count();
	ASSERT_RETURN( IS_VALID_INDEX( nIndex, nCount ) );
	pPass->tRenderList[ nIndex ].nParent = nParent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListSetBBox(
	PASSID	nPass,
	int		nIndex,
	const	BOUNDING_BOX * ptBBox )
{
	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERTX_RETURN( pPass->dwFlags & _RL_PASS_FLAG_OPEN, "Tried to set bbox on renderable in closed pass!" );

	int nCount = pPass->tRenderList.Count();
	ASSERT_RETURN( IS_VALID_INDEX( nIndex, nCount ) );
	ASSERT_RETURN( ptBBox );
	pPass->tRenderList[ nIndex ].tBBox = *ptBBox;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//int e_RenderListAddEffectBegin(
//	RENDER_EFFECT_TYPE eType,
//	DWORD_PTR dwData,
//	DWORD dwFlags )
//{
//	DWORD dwSubType = e_RenderEffectMakeTypeMask( eType, RFX_BEGIN );
//	return e_RenderListAdd( RENDERABLE_EFFECT, dwSubType, dwData, dwFlags );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//int e_RenderListAddEffectEnd(
//	RENDER_EFFECT_TYPE eType,
//	DWORD_PTR dwData,
//	DWORD dwFlags )
//{
//	DWORD dwSubType = e_RenderEffectMakeTypeMask( eType, RFX_END );
//	return e_RenderListAdd( RENDERABLE_EFFECT, dwSubType, dwData, dwFlags );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//int e_RenderListAddParticles(
//	int nGroup,
//	DWORD dwFlags )
//{
//	ASSERT_RETINVALID( nGroup >= 0 );
//	return e_RenderListAdd( RENDERABLE_PARTICLES, 0, (DWORD)nGroup, dwFlags );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListPassCull( PASSID nPass )
{
	PERF(ROOM_PASS_CULL);

	// iterate this render list pass
	// cull based on various tests

	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERTX_RETURN( 0 == ( pPass->dwFlags & _RL_PASS_FLAG_OPEN ), "Tried to cull renderables in open pass!" );

	//VECTOR vEye, vLookat;
	//e_GetViewPosition( &vEye );
	//e_GetViewFocusPosition( &vLookat );

	//int nCurRegion = e_GetCurrentRegionID();
	//if ( pPass->eType == RL_PASS_PORTAL )
	//{
	//	// set current region
	//	PORTAL * pPortal = e_PortalGet( pPass->nPortalID );
	//	ASSERT_RETURN( pPortal && pPortal->pConnection );
	//	e_SetRegion( pPortal->pConnection->nRegion );
	//	e_PortalUpdateMatrices( pPortal, vEye, vLookat );
	//	e_SetCurrentViewOverrides( pPortal );
	//}


	MATRIX matView, matProj;
	e_GetWorldViewProjMatricies( NULL, &matView, &matProj );

	PLANE tPlanes[ NUM_FRUSTUM_PLANES ];
	e_MakeFrustumPlanes( tPlanes, &matView, &matProj );


	int nCount = pPass->tRenderList.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		Renderable & tRenderable = pPass->tRenderList[ i ];

		if ( tRenderable.eType == RENDERABLE_MODEL )
		{
			int nModelID = (int)tRenderable.dwData;

			// model exists cull
			if ( ! e_IsValidModelID( nModelID ) )
			{
				continue;
			}

			// visibility flag cull
			if ( 0 == ( tRenderable.dwFlags & RENDERABLE_FLAG_ALLOWINVISIBLE ) )
			{
				if ( ! e_ModelIsVisible( nModelID ) )
				{
					continue;
				}
			}

			// frustum cull
			if ( 0 == ( tRenderable.dwFlags & RENDERABLE_FLAG_NOCULLING ) &&
				 e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) )
			{
				if ( ! e_ModelInFrustum( nModelID, tPlanes ) )
				{
					continue;
				}
			}

			// draw layer cull
			if ( e_ModelGetDrawLayer( nModelID ) != (DRAW_LAYER)pPass->nDrawLayer )
			{
				continue;
			}

			tRenderable.dwFlags |= _RENDERABLE_FLAG_PASSED_CULL;
		}
	}

	// restore current region
	//e_SetRegion( nCurRegion );
	//e_SetCurrentViewOverrides( NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sCompareRenderableModels( const void * p1, const void * p2 )
{
	const Renderable * pm1 = static_cast<const Renderable*>(p1);
	const Renderable * pm2 = static_cast<const Renderable*>(p2);

	if ( 0 != ( pm1->dwFlags & _RENDERABLE_FLAG_FIRST_PERSON_PROJ ) &&
		 0 == ( pm2->dwFlags & _RENDERABLE_FLAG_FIRST_PERSON_PROJ ) )
		return -1;
	if ( 0 == ( pm1->dwFlags & _RENDERABLE_FLAG_FIRST_PERSON_PROJ ) &&
		 0 != ( pm2->dwFlags & _RENDERABLE_FLAG_FIRST_PERSON_PROJ ) )
		return 1;

	// less goes earlier
	if ( pm1->fSortValue < pm2->fSortValue )
		return -1;
	if ( pm1->fSortValue > pm2->fSortValue )
		return 1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListPassBuildModelList( PASSID nPass )
{
	PERF(ROOM_PASS_BUILDML);

	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERTX_RETURN( 0 == ( pPass->dwFlags & _RL_PASS_FLAG_OPEN ), "Tried to build modellist in open pass!" );

	ArrayClear( pPass->tModelList );

	Renderable tModel;
	BOUNDING_BOX tBBox;
	VECTOR vEye;

	e_GetViewPosition( &vEye );

	int nCount = pPass->tRenderList.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		Renderable & tRenderable = pPass->tRenderList[ i ];

		BOOL bSkip = FALSE;
		BOOL bNoRender = FALSE;
		BOOL bIsolate = tRenderable.eType == RENDERABLE_MODEL && e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL ) == tRenderable.dwData;
	
		if ( tRenderable.eType != RENDERABLE_MODEL )
			bSkip = TRUE;
		if ( 0 == ( tRenderable.dwFlags & _RENDERABLE_FLAG_PASSED_CULL ) )
			bSkip = TRUE;
		if ( ! bIsolate && tRenderable.nParent != INVALID_ID && tRenderable.nParent < nCount )
			if ( 0 == ( pPass->tRenderList[ tRenderable.nParent ].dwFlags & _RENDERABLE_FLAG_PASSED_CULL ) )
				bSkip = TRUE;
		bNoRender = bSkip;
		// may be used to render an in-place mesh (like a bounding box)
		if ( 0 == ( tRenderable.dwFlags & RENDERABLE_FLAG_OCCLUDABLE ) )
			bSkip = FALSE;
		if ( bSkip )
			continue;


		tModel = tRenderable;
		if ( bNoRender )
		{
			// mark to not render as normal
			tModel.dwFlags |= _RENDERABLE_FLAG_NO_RENDER;
		}

		V( e_GetModelRenderAABBInWorld( (int)tModel.dwData, &tBBox ) );
		tModel.fSortValue = BoundingBoxDistanceSquared( &tBBox, &vEye );

		// Tyler 2006.1.13 we need to take into account the size of the model
		VECTOR vBoxSize = BoundingBoxGetSize( &tBBox );
		tModel.fSortValue /= MAX( 0.1f, MAX( vBoxSize.fX, MAX( vBoxSize.fY, vBoxSize.fZ ) ) );

		// CHB 2006.11.30
		//V( e_ModelSelectLOD((int)tModel.dwData, tModel.fSortValue) );

		if ( tRenderable.eType == RENDERABLE_MODEL )
		{
			int nModelDef = e_ModelGetDefinition( (int)tRenderable.dwData );
			DWORD dwModelDefFlags = e_ModelDefinitionGetFlags( nModelDef, e_ModelGetDisplayLOD( (int)tRenderable.dwData ) );
			if ( dwModelDefFlags & MODELDEF_FLAG_HAS_FIRST_PERSON_PROJ )
				tModel.dwFlags |= _RENDERABLE_FLAG_FIRST_PERSON_PROJ;


			//if ( AppIsHellgate() )
			//{
			//	if ( dwModelDefFlags & MODELDEF_FLAG_ANIMATED )
			//	{
			//		float fDistanceAlpha = 1.0f - SATURATE( ( tModel.fSortValue - 
			//			MIN_ALPHA_DISTANCE_SQ ) / ( MAX_ALPHA_DISTANCE_SQ - 
			//			MIN_ALPHA_DISTANCE_SQ ) );
			//		if ( fDistanceAlpha > 0.0f )
			//		{
			//			V( e_ModelSetDistanceAlpha( (int)tModel.dwData, fDistanceAlpha ) );
			//		}
			//		else
			//			// Don't render this model, since alpha is zero.
			//			continue;
			//	}
			//}
		}

		ArrayAddItem( pPass->tModelList, tModel );
	}

	pPass->tModelList.QSort( sCompareRenderableModels );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListPassMarkVisible( PASSID nPass )
{
	PERF(ROOM_PASS_MARK);

	RENDERLIST_PASS * pPass = sRenderListPassGet( nPass );
	ASSERT_RETURN( pPass );
	ASSERTX_RETURN( 0 == ( pPass->dwFlags & _RL_PASS_FLAG_OPEN ), "Tried to mark visible in open pass!" );

	// iterate render list and mark as visible
	int nCount = pPass->tModelList.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		Renderable & tInfo = pPass->tModelList[ i ];
		if ( tInfo.eType != RENDERABLE_MODEL )
			continue;

		BOOL bVisible = FALSE;
		if ( tInfo.dwFlags & _RENDERABLE_FLAG_PASSED_CULL )
			bVisible = TRUE;
		if ( tInfo.dwFlags & _RENDERABLE_FLAG_NO_RENDER )
			bVisible = FALSE;

		if ( bVisible )
			e_ModelUpdateVisibilityToken( (int)tInfo.dwData );
		//e_ModelSetVisible( (int)tInfo.dwData, bVisible );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sCompareRenderListPasses( const void * p1, const void * p2 )
{
	const RENDERLIST_PASS * pP1 = &sgtRenderListPasses[ *(const int*)p1 ];
	const RENDERLIST_PASS * pP2 = &sgtRenderListPasses[ *(const int*)p2 ];

	if ( pP1->eType < pP2->eType )
		return -1;
	if ( pP1->eType > pP2->eType )
		return 1;
	if ( pP1->eType == RL_PASS_PORTAL )
	{
		// sort portal passes depth first
		if ( pP1->fSortValue < pP2->fSortValue )
			return 1;
		if ( pP1->fSortValue > pP2->fSortValue )
			return -1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListSortPasses()
{
	VECTOR vEye;
	e_GetViewPosition( &vEye );

	// for all the portal type passes, find a distance
	for ( int i = 0; i < sgnRenderListPasses; i++ )
	{
		sgtRenderListPasses[ i ].fSortValue = 0.f;

		if ( sgtRenderListPasses[ i ].eType == RL_PASS_PORTAL )
		{
			PORTAL * pPortal = e_PortalGet( sgtRenderListPasses[ i ].nPortalID );
			if ( !pPortal )
				continue;
			sgtRenderListPasses[ i ].fSortValue = VectorDistanceSquared( pPortal->vCenter, vEye );
		}
	}

	for ( int i = 0; i < sgnRenderListPasses; i++ )
		sgpnRenderListSortedPasses[ i ] = i;

	qsort( sgpnRenderListSortedPasses, sgnRenderListPasses, sizeof(int), sCompareRenderListPasses );

	// should probably merge passes now that render together (eg, two views through the same portal)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListCommit()
{
	if ( e_GetUIOnlyRender() )
		return;

	for ( int nPass = 0; nPass < sgnRenderListPasses; nPass++ )
	{
		RENDERLIST_PASS * pPass = sRenderListPassGet( sgpnRenderListSortedPasses[ nPass ] );
		ASSERT_CONTINUE( pPass );

		V( e_SetRegion( pPass->nRegionID ) );

		switch ( pPass->eType )
		{
		case RL_PASS_PORTAL:
			// setup portal settings
			if ( !e_PortalGet( pPass->nPortalID ) )
				break;
			{
				V( e_SetupPortal( pPass->nPortalID ) );
			}
			e_RenderModelList( pPass );
			{
				V( e_PortalStoreRender( pPass->nPortalID ) );
			}
			//e_SetCurrentViewOverrides( NULL );
			// render models in render list

			break;
		case RL_PASS_ALPHAMODEL:
			break;
		case RL_PASS_GENERATECUBEMAP:
			BOOL bFrustumCull, bClip, bScissor;
			bFrustumCull   = e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS );
			bClip          = e_GetRenderFlag( RENDER_FLAG_CLIP_ENABLED );
			bScissor       = e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED );

			// setup cube map settings
			e_SetupCubeMapGeneration();
			int nRenderPasses;
			nRenderPasses = 6;
			// render models in render list once for each face
#ifdef ENGINE_TARGET_DX9  //KMNV TODO: Single pass in DX10
			for ( int nRenderPass = 0; nRenderPass < nRenderPasses; nRenderPass++ )
			{
				e_CubeMapGenSetViewport();
				V( e_SetupPortal( INVALID_ID ) );
				e_RenderModelList( pPass );
				e_SaveCubeMapFace();
			}
#endif
			// save cube map
			e_SaveFullCubeMap();

			e_SetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS,		bFrustumCull   );
			e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				bClip		   );
			e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			bScissor	   );
			break;
		case RL_PASS_REFLECTION:
			break;
		case RL_PASS_NORMAL:
			// render models in draw list
			{
				V( e_SetupPortal( INVALID_ID ) );
			}
			e_RenderModelList( pPass );
			break;
		default:
			ASSERTX_CONTINUE( 0, "Invalid pass type encountered" );
		}
	}

}