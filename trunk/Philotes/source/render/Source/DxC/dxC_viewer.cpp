//----------------------------------------------------------------------------
// dxC_viewer.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "camera_priv.h"
#include "plane.h"
#include "dxC_model.h"
#include "dx9_model.h"
#include "dxC_state.h"
#include "dxC_renderlist.h"
#include "dxC_meshlist.h"
#include "dxC_render.h"
#include "dxC_pass.h"
#include "dxC_target.h"
#include "e_main.h"
#include "dxC_rchandlers.h"
#include <algorithm>
#include "dxC_viewer_render.h"
#include "e_screenshot.h"

#ifdef ENABLE_DPVS
#include "dxC_dpvs.h"
#endif

#include "dxC_viewer.h"

namespace FSSE
{;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef PRESULT (*PFN_VIEWER_RENDER)( struct Viewer * pViewer );
typedef safe_vector<int>	ViewParametersStack;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

ViewerMap *				g_ViewerMap			= NULL;
int						gnNextViewerID			= 0;
ViewParametersVector *	g_ViewParametersVector	= NULL;
int						gnViewParamsCurrent		= INVALID_ID;
Viewer *				gpViewerCurrent			= NULL;
ViewParametersStack *	gpnViewParametersStack	= NULL;

#if ISVERSION(DEVELOPMENT)
static const int sgcnViewerStatsBufLen = 2048;
static WCHAR sgwszViewersStats[ sgcnViewerStatsBufLen ] = L"";
#endif

//----------------------------------------------------------------------------
// Include viewer render functions array
#define INCLUDE_VR_ARRAY
#include "e_viewer_def.h"
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static PRESULT sViewerRemove( ViewerMap::iterator i, BOOL bErase )
{
	i->second.tContexts.Destroy();
	i->second.tRPDs.Destroy();
	i->second.tRPDSorts.Destroy();

	//DPVS_SAFE_RELEASE( i->second.pCameraCell );
	//if ( i->second.pCameraCell )
	//	i->second.pCameraCell->release();
#ifdef ENABLE_DPVS
	if ( i->second.pDPVSCamera )
		i->second.pDPVSCamera->release();
	i->second.pDPVSCamera = NULL;
#endif

	MEMORYPOOL_DELETE(g_StaticAllocator, i->second.pModelDrawPool);

	if ( bErase )
		g_ViewerMap->erase( i );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT e_ViewersInit()
{
	ASSERT_RETFAIL( g_ViewerMap == NULL );
	g_ViewerMap = MEMORYPOOL_NEW(g_StaticAllocator) ViewerMap(ViewerMap::key_compare(), ViewerMap::allocator_type(g_StaticAllocator));
	ASSERT_RETVAL( g_ViewerMap, E_OUTOFMEMORY );

	ASSERT_RETFAIL( g_ViewParametersVector == NULL );
	g_ViewParametersVector = MALLOC_NEW( NULL, ViewParametersVector );
	ASSERT_RETVAL( g_ViewParametersVector, E_OUTOFMEMORY );

	ASSERT_RETFAIL( gpnViewParametersStack == NULL );
	gpnViewParametersStack = MALLOC_NEW( NULL, ViewParametersStack );
	ASSERT_RETVAL( gpnViewParametersStack, E_OUTOFMEMORY );

	V( dxC_RenderCommandInit() );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewersDestroy()
{
	if ( g_ViewerMap )
	{
		for ( ViewerMap::iterator i = g_ViewerMap->begin();
			i != g_ViewerMap->end();
			++i)
		{
			V( sViewerRemove( i, FALSE ) );
		}
		MEMORYPOOL_PRIVATE_DELETE( g_StaticAllocator, g_ViewerMap, ViewerMap );
		g_ViewerMap = NULL;
	}

	if ( g_ViewParametersVector )
	{
		FREE_DELETE( NULL, g_ViewParametersVector, ViewParametersVector );
		g_ViewParametersVector = NULL;
	}

	if ( gpnViewParametersStack )
	{
		FREE_DELETE( NULL, gpnViewParametersStack, ViewParametersStack );
		gpnViewParametersStack = NULL;
	}

	V( dxC_RenderCommandDestroy() );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewersResizeScreen()
{
	if ( g_ViewerMap )
	{
		for ( ViewerMap::iterator i = g_ViewerMap->begin();
			i != g_ViewerMap->end();
			++i)
		{
#ifdef ENABLE_DPVS
			if ( ! i->second.pDPVSCamera )
				continue;
			i->second.pDPVSCamera->release();
			i->second.pDPVSCamera = NULL;
#endif
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerGet( int nViewerID, Viewer *& pViewer )
{
	pViewer = NULL;
	ASSERT_RETINVALIDARG( nViewerID >= 0 );

	DWORD nKey = static_cast<DWORD>(nViewerID);

	ViewerMap::iterator iViewer = g_ViewerMap->find( nKey );
	//ASSERTV_RETFAIL( iViewer != g_ViewerMap->end(), "Couldn't find Viewer with ID %d", nViewerID );
	if ( iViewer == g_ViewerMap->end() )
	{
		return S_FALSE;
	}

	pViewer = &(*g_ViewerMap)[ nKey ];

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewParametersGet( int nViewParametersID, ViewParameters *& pViewParams )
{
	pViewParams = NULL;
	ASSERT_RETINVALIDARG( nViewParametersID >= 0 );
	ASSERT_RETFAIL( g_ViewParametersVector );
	ASSERT_RETINVALIDARG( nViewParametersID < (int)g_ViewParametersVector->size() );

	//ViewerMap::iterator iViewer = g_ViewerMap->find( nKey );
	//ASSERTV_RETFAIL( iViewer != g_ViewerMap->end(), "Couldn't find Viewer with ID %d", nViewerID );
	pViewParams = &(*g_ViewParametersVector)[ nViewParametersID ];

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewParametersStackPush( int & nViewParametersID )
{
	ASSERT_RETFAIL( g_ViewParametersVector );
	ASSERT_RETFAIL( gpnViewParametersStack );

	ViewParameters tViewParams;
	tViewParams.dwFlagBits = 0;
	tViewParams.nLastSection = 0;
	g_ViewParametersVector->push_back( tViewParams );

	nViewParametersID = g_ViewParametersVector->size() - 1;

	gpnViewParametersStack->push_back( nViewParametersID );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewParametersStackPop()
{
	ASSERT_RETFAIL( gpnViewParametersStack );
	ASSERT_RETFAIL( gpnViewParametersStack->size() > 0 );

	gpnViewParametersStack->erase( gpnViewParametersStack->end() - 1 );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewParametersGetCurrent_Private( int & nViewParametersID )
{
	ASSERT_RETFAIL( gpnViewParametersStack );
	ASSERT_RETFAIL( gpnViewParametersStack->size() > 0 );

	nViewParametersID = *( gpnViewParametersStack->end() - 1 );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerCreate( int & nID )
{
	DWORD dwID = (DWORD)gnNextViewerID;
	Viewer * pViewer = &(*g_ViewerMap)[ dwID ];
	++gnNextViewerID;
	ASSERT_RETFAIL( pViewer );
	nID = (int)dwID;

	pViewer->nID = nID;
	pViewer->dwFlagBits = 0;
	structclear( pViewer->tCameraInfo );
	structclear( pViewer->tVP );
	pViewer->eSwapRT = SWAP_RT_BACKBUFFER;
	pViewer->eSwapDT = SWAP_DT_AUTO;
	pViewer->nCameraRoom = INVALID_ID;
	pViewer->nCameraCell = INVALID_ID;
	//pViewer->ptGameWindow = NULL;
	pViewer->nSwapChainIndex = DEFAULT_SWAP_CHAIN;
	pViewer->eDefaultDrawLayer = DRAW_LAYER_GENERAL;

	pViewer->tContexts.Init( NULL );
	pViewer->tRPDs.Init( NULL );
	pViewer->tRPDSorts.Init( NULL );

	pViewer->pModelDrawPool = MEMORYPOOL_NEW(g_StaticAllocator) FSCommon::CPool<ModelDraw, false>(g_StaticAllocator);
	pViewer->pModelDrawPool->SetBucketSize(64);

#ifdef ENABLE_DPVS
	SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_USE_DPVS, TRUE );
	pViewer->pDPVSCamera = NULL;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerRemove( int nID )
{
	if ( nID == INVALID_ID )
		return S_FALSE;

	DWORD dwID = (DWORD)nID;

	ViewerMap::iterator i = g_ViewerMap->find( dwID );
	if ( i != g_ViewerMap->end() )
	{
		V( sViewerRemove( i, TRUE ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sViewerResetLists( int nViewerID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	pViewer->tModelsToTest.clear();
	SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_SELECTED_VISIBLE, FALSE );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewersResetAllLists()
{
	for ( ViewerMap::iterator i = g_ViewerMap->begin();
		i != g_ViewerMap->end();
		++i )
	{
		V( sViewerResetLists( (int)i->first ) );
	}

	// perform once-per-frame actions here
	if ( gpnViewParametersStack )
		gpnViewParametersStack->clear();
	if ( g_ViewParametersVector )
		g_ViewParametersVector->clear();
	gpViewerCurrent = NULL;

	V( dxC_MeshListsClear() );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerUpdateCamera( int nViewerID, const struct CAMERA_INFO * pCameraInfo, int nRoomID, int nCellID )
{
	if ( ! g_ViewerMap )
		return S_FALSE;
	if ( ! pCameraInfo )
		return S_FALSE;

	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETFAIL( pViewer );
	ASSERT_RETINVALIDARG( pCameraInfo );

	pViewer->tCameraInfo = *pCameraInfo;
	pViewer->nCameraRoom = nRoomID;


	if ( pViewer->nCameraCell != nCellID )
	{
		pViewer->nCameraCell = nCellID;
	}

//#ifdef ENABLE_DPVS
//	if ( pViewer->pDPVSCamera )
//	{
//		V( e_DPVS_CameraUpdate( pViewer->pDPVSCamera, pViewer->nCameraCell, &pViewer->tCameraInfo, e_GetNearClipPlane(), e_GetFarClipPlane() ) );
//	}
//#endif

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerSetRenderType( int nViewerID, VIEWER_RENDER_TYPE eRenderType )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eRenderType, NUM_VIEWER_RENDER_TYPES ) );
	pViewer->eRenderType = eRenderType;
	return S_OK;
}


//----------------------------------------------------------------------------

PRESULT e_ViewerSetRenderFlag( int nViewerID, VIEWER_RENDER_FLAGBIT eRenderFlagbit, BOOL bValue )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eRenderFlagbit, _VIEWER_NUM_FLAGBITS ) );

	switch ( eRenderFlagbit )
	{
	case VIEWER_FLAGBIT_SKYBOX:
		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ENABLE_SKYBOX, bValue );
		break;
	case VIEWER_FLAGBIT_BEHIND:
		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ENABLE_BEHIND, bValue );
		break;
	case VIEWER_FLAGBIT_PARTICLES:
		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ENABLE_PARTICLES, bValue );
	case VIEWER_FLAGBIT_ALLOW_STALE_CAMERA:
		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ALLOW_STALE_CAMERA, bValue );
		break;
	case VIEWER_FLAGBIT_IGNORE_GLOBAL_CAMERA_PARAMS:
		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_IGNORE_GLOBAL_CAMERA_PARAMS, bValue );
		break;
	}

	return S_OK;
}


//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetViewport( Viewer * pViewer, const D3DC_VIEWPORT & tVP )
{
	ASSERT_RETINVALIDARG( pViewer );
	pViewer->tVP = tVP;
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetViewport( Viewer * pViewer, RENDER_TARGET_INDEX eTarget )
{
	ASSERT_RETINVALIDARG( pViewer );

	D3DC_VIEWPORT tVP;

	V_RETURN( dxC_ViewportGetDimensions( tVP, eTarget, FALSE, pViewer->nSwapChainIndex ) );

	return dxC_ViewerSetViewport( pViewer, tVP );
}

//----------------------------------------------------------------------------

PRESULT e_ViewerSetCustomViewport( int nViewerID, const E_RECT & tRect )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	D3DC_VIEWPORT tVP;

	tVP.TopLeftX = tRect.left;
	tVP.TopLeftY = tRect.top;
	tVP.Width	 = tRect.right - tRect.left;
	tVP.Height	 = tRect.bottom - tRect.top;
	tVP.MinDepth = 0.f;
	tVP.MaxDepth = 1.f;

	return dxC_ViewerSetViewport( pViewer, tVP );
}

//----------------------------------------------------------------------------

PRESULT e_ViewerSetFullViewport( int nViewerID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );
	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
	return dxC_ViewerSetViewport( pViewer, nRT );
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetGameWindow( Viewer * pViewer, const struct GAME_WINDOW * ptGameWindow )
{
	//pViewer->ptGameWindow = ptGameWindow;

	if ( ! ptGameWindow )
	{
		pViewer->nSwapChainIndex = DEFAULT_SWAP_CHAIN;
	}

	BOOL bSwapChainIsValid = !!( dxC_GetD3DSwapChain( pViewer->nSwapChainIndex ) );

	if ( ptGameWindow && ( pViewer->nSwapChainIndex == DEFAULT_SWAP_CHAIN || ! bSwapChainIsValid ) )
	{
		// Create separate swap chain for this window
		V_RETURN( dxC_CreateAdditionalSwapChain( pViewer->nSwapChainIndex, ptGameWindow->m_nWindowWidth, ptGameWindow->m_nWindowHeight, ptGameWindow->m_hWnd ) );
	}

	return S_OK;
}

PRESULT e_ViewerSetGameWindow( int nViewerID, const struct GAME_WINDOW * ptGameWindow )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );
	return dxC_ViewerSetGameWindow( pViewer, ptGameWindow );
}

//----------------------------------------------------------------------------

PRESULT e_ViewerSetDefaultDrawLayer( int nViewerID, DRAW_LAYER eDefaultDrawLayer )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eDefaultDrawLayer, NUM_DRAW_LAYERS ) );
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );
	pViewer->eDefaultDrawLayer = eDefaultDrawLayer;
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetColorTarget( Viewer * pViewer, SWAP_CHAIN_RENDER_TARGET eSwapRT )
{
	ASSERT_RETINVALIDARG( pViewer );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eSwapRT, NUM_SWAP_CHAIN_RENDER_TARGETS ) );
	pViewer->eSwapRT = eSwapRT;
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetDepthTarget( Viewer * pViewer, SWAP_CHAIN_DEPTH_TARGET eSwapDT )
{
	ASSERT_RETINVALIDARG( pViewer );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eSwapDT, NUM_SWAP_CHAIN_DEPTH_TARGETS ) );
	pViewer->eSwapDT = eSwapDT;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sViewerLazyUpdateMesh( VISIBLE_MESH & tVisMesh )
{
	V_RETURN( dx9_MeshApplyMaterial(
		tVisMesh.pMesh,
		tVisMesh.pModelDef,
		tVisMesh.pModel,
		tVisMesh.nLOD,
		tVisMesh.nMesh ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sViewerProcessVisibleMesh( Viewer * pViewer, VISIBLE_MESH & tVisMesh )
{
	// This step performs MeshApplyMaterial.
	V_RETURN( sViewerLazyUpdateMesh( tVisMesh ) );
	if ( ! dx9_MeshIsVisible( tVisMesh.pModel, tVisMesh.pMesh ) )
		return S_FALSE;

	BOOL bNoScissor =    dxC_ModelGetFlagbit( tVisMesh.pModel, MODEL_FLAGBIT_ANIMATED )
					  && ( pViewer->tCurNoScissor.GetCount() > 0 );
	SETBIT( tVisMesh.dwFlagBits, VISIBLE_MESH::FLAGBIT_FORCE_NO_SCISSOR, bNoScissor );

	// survived basic culling, so add to the list of visible meshes
	pViewer->tVisibleMeshList.push_back( tVisMesh );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sCullModel(
	const D3D_MODEL * pModel )
{
	// temp workaround for cube map gen
	if ( e_GeneratingCubeMap() && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
		return TRUE;

	// HACK -- this means it isn't in a room
	//if ( pModel->dwUserData == INVALID_ID )
	//	return TRUE;

	if ( ! dx9_ModelIsVisible( pModel ) )
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------

static PRESULT sViewerUpdateModelAndCull( VISIBLE_MESH & tVisMesh )
{
	ASSERT_RETINVALIDARG( tVisMesh.pModel );

#if ISVERSION(DEVELOPMENT)
	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	if ( nIsolateModel != INVALID_ID && nIsolateModel != tVisMesh.pModel->nId )
		return S_FALSE;
#endif

	BOUNDING_BOX tBBox;
	V( dxC_GetModelRenderAABBInWorld( tVisMesh.pModel, &tBBox ) );
	VECTOR vEye;
	e_GetViewPosition( &vEye );
	float fDistSq = BoundingBoxDistanceSquared( &tBBox, &vEye );
	tVisMesh.fDistFromEyeSq = fDistSq;
	if( AppIsTugboat() )
	{
		VECTOR vPos;
		tVisMesh.fPointDistFromEyeSq = VectorDistanceSquaredXY( tBBox.Center( vPos ), vEye );
	}


	// Temporarily, grab any working model definition in case we need to query its bounding box.
	tVisMesh.pModelDef = NULL;
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		if ( NULL != ( tVisMesh.pModelDef = dxC_ModelDefinitionGet( tVisMesh.pModel->nModelDefinitionId, nLOD ) ) )
			break;
	// Get the on-screen size of this model.
	float fWorldSize = 0.f;
	V( dxC_ModelGetWorldSizeAvg( tVisMesh.pModel, fWorldSize, tVisMesh.pModelDef ) );
	fWorldSize = MAX( fWorldSize, 1.f );
	if( AppIsTugboat() )
	{
		tVisMesh.fScreensize = e_GetWorldDistanceBiasedScreenSizeByVertFOV( SQRT_SAFE(tVisMesh.fDistFromEyeSq), fWorldSize );
	}
	else
	{
		tVisMesh.fScreensize = e_GetWorldDistanceBiasedScreenSizeByVertFOV( SQRT_SAFE(fDistSq), fWorldSize );
	}


	// Now, select the actual proper LOD and model definition.
	dxC_ModelSelectLOD( tVisMesh.pModel, tVisMesh.fScreensize, fDistSq );
	tVisMesh.nLOD = dxC_ModelGetDisplayLOD( tVisMesh.pModel );
	tVisMesh.pModelDef = dxC_ModelDefinitionGet( tVisMesh.pModel->nModelDefinitionId, tVisMesh.nLOD );
	if ( ! tVisMesh.pModelDef )
		return S_FALSE;


	//if ( AppIsHellgate() )
	{
		float fDistanceAlpha;
		V( dxC_ModelCalculateDistanceAlpha( fDistanceAlpha, tVisMesh.pModel, tVisMesh.fScreensize, tVisMesh.fDistFromEyeSq, tVisMesh.fPointDistFromEyeSq ) );

		if ( fDistanceAlpha > 0.0f )
		{
			V( dxC_ModelSetDistanceAlpha( tVisMesh.pModel, fDistanceAlpha ) );
		}
		else
			// Don't render this model, since alpha is zero.
			return S_FALSE;
	}

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sViewerAddModelToMeshList( Viewer * pViewer, D3D_MODEL * pModel, E_RECT * pRect, int nViewParams = INVALID_ID )
{
	ASSERT_RETINVALIDARG( pViewer && pModel );

	VISIBLE_MESH tVisMesh;
	tVisMesh.dwFlagBits = 0;
	tVisMesh.nViewParams = nViewParams;
	tVisMesh.nSection = pViewer->nCurSection;

	if ( pRect )
	{
		tVisMesh.tScissor = *pRect;
		SETBIT( tVisMesh.dwFlagBits, VISIBLE_MESH::FLAGBIT_HAS_SCISSOR, TRUE );
	}

	ViewParameters * pVP = NULL;
	V( dxC_ViewParametersGet( nViewParams, pVP ) );
	if ( pVP )
	{
		// Track the last section for each nViewParams for opaque->alpha pass sorting.
		//pViewer->anLastSection[ nViewParams ] = MAX( pViewer->anLastSection[ nViewParams ], pViewer->nCurSection );
		pVP->nLastSection = MAX( pVP->nLastSection, pViewer->nCurSection );
	}

	tVisMesh.pModel = pModel;

	PRESULT pr;
	V_SAVE( pr, sViewerUpdateModelAndCull( tVisMesh ) );
	if ( FAILED(pr) || pr == S_FALSE )
		return pr;

	tVisMesh.pModelDraw = pViewer->pModelDrawPool->Get();
	tVisMesh.pModelDraw->Clear();
	tVisMesh.pModelDraw->nModelID = pModel->nId;

	for ( int i = 0; i < tVisMesh.pModelDef->nMeshCount; i++ )
	{
		tVisMesh.pMesh = dx9_ModelDefinitionGetMesh( tVisMesh.pModelDef, i );
		if ( ! tVisMesh.pMesh )
			continue;

		tVisMesh.nMesh = i;
		tVisMesh.eMatOverrideType = MATERIAL_OVERRIDE_NORMAL;

		V( sViewerProcessVisibleMesh( pViewer, tVisMesh ) );

		// If this model requests a clone, add another visible mesh.
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CLONE ) )
		{
			tVisMesh.eMatOverrideType = MATERIAL_OVERRIDE_CLONE;
			V( sViewerProcessVisibleMesh( pViewer, tVisMesh ) );
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dxC_ViewerAddModelToVisibleList_Private( Viewer * pViewer, D3D_MODEL * pModel, E_RECT * pRect, int nViewParams /*= INVALID_ID*/ )
{
	// any extra per-model culling (not dependent on render context) goes here
	if ( sCullModel( pModel ) )
		return S_FALSE;

	V( dxC_ModelUpdateVisibilityToken( pModel ) );

	V( sViewerAddModelToMeshList( pViewer, pModel, pRect, nViewParams ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sViewerUpdateDPVSCamera( Viewer * pViewer )
{
#ifdef ENABLE_DPVS
	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		return S_FALSE;

	if ( ! TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::FLAGBIT_USE_DPVS ) )
		return S_FALSE;

	if ( ! pViewer->pDPVSCamera )
	{
		int nWidth, nHeight;
		RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_BACKBUFFER );
		V_RETURN( dxC_GetRenderTargetDimensions( nWidth, nHeight, nRT ) );
		V_RETURN( e_DPVS_CameraCreate( pViewer->pDPVSCamera, TRUE, nWidth, nHeight ) );
	}
	ASSERT_RETFAIL( pViewer->pDPVSCamera );

	V_RETURN( e_DPVS_CameraUpdate( pViewer->pDPVSCamera, pViewer->nCameraCell, &pViewer->tCameraInfo, e_GetNearClipPlane(), e_GetFarClipPlane() ) );
	return S_OK;
#endif
	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ViewerGetCameraRoom( int nViewerID, int & nRoomID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	nRoomID = pViewer->nCameraRoom;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ViewerGetSwapChainIndex( int nViewerID, int & nSwapChainIndex )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	nSwapChainIndex = pViewer->nSwapChainIndex;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ViewerSelectVisible( int nViewerID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	pViewer->tVisibleMeshList.clear();
	pViewer->pModelDrawPool->PutAll();
	pViewer->nCurSection = 0;
	RefCountInit( pViewer->tCurNoScissor, 0 );
	//ZeroMemory( pViewer->anLastSection, sizeof(pViewer->anLastSection) );


#ifdef ENABLE_DPVS
	if (   		
		 ! c_GetToolMode() &&
		 e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED )
		&& TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::FLAGBIT_USE_DPVS ) )
	{
		V_RETURN( sViewerUpdateDPVSCamera( pViewer ) );

		dxC_ViewerSetCurrent_Private( pViewer );
		e_DPVS_QueryVisibility( pViewer->pDPVSCamera );
		dxC_ViewerSetCurrent_Private( NULL );

		//SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_SELECTED_VISIBLE, TRUE );
		//return S_OK;
	}
#endif // DPVS


	if ( pViewer->tModelsToTest.size() > 0 )
	{
		

		SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_FRUSTUM_CULL, !!( e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) ) );

		MATRIX mProj;
		BOUNDING_BOX tVPBox;
		tVPBox.vMin = VECTOR( (float)pViewer->tVP.TopLeftX, (float)pViewer->tVP.TopLeftY, 0.f );
		tVPBox.vMax = VECTOR( (float)pViewer->tVP.Width,    (float)pViewer->tVP.Height,   0.f ) + tVPBox.vMin;
		CameraGetProjMatrix( &pViewer->tCameraInfo, &mProj, TESTBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ALLOW_STALE_CAMERA ), &tVPBox );
		e_ScreenShotGetProjectionOverride( &mProj, pViewer->tCameraInfo.fVerticalFOV, e_GetNearClipPlane(), e_GetFarClipPlane() );
		MATRIX mView;
		CameraGetViewMatrix( &pViewer->tCameraInfo, &mView, TESTBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ALLOW_STALE_CAMERA ) );

		e_GetCurrentViewOverrides( &mView, &pViewer->tCameraInfo.vPosition, NULL, NULL );

		// update frustum planes
		if ( TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::FLAGBIT_FRUSTUM_CULL ) )
		{
			e_MakeFrustumPlanes( pViewer->tFrustumPlanes, &mView, &mProj );
		}

		int nViewParams = INVALID_ID;
		V( dxC_ViewParametersStackPush( nViewParams ) );
		ViewParameters * pViewParams = NULL;
		V( dxC_ViewParametersGet( nViewParams, pViewParams ) );

		// Projection matrix
		SETBIT( pViewParams->dwFlagBits, ViewParameters::PROJ_MATRIX, TRUE );
		pViewParams->mProj = mProj;

		// View matrix
		SETBIT( pViewParams->dwFlagBits, ViewParameters::VIEW_MATRIX, TRUE );
		pViewParams->mView = mView;

		// Eye position
		SETBIT( pViewParams->dwFlagBits, ViewParameters::EYE_POS, TRUE );
		pViewParams->vEyePos = pViewer->tCameraInfo.vPosition;

		// Env def
		SETBIT( pViewParams->dwFlagBits, ViewParameters::ENV_DEF, TRUE );
		pViewParams->nEnvDefID = e_GetCurrentEnvironmentDefID();

		// break all models into meshes
		for ( safe_vector<D3D_MODEL*>::iterator i = pViewer->tModelsToTest.begin();
			i != pViewer->tModelsToTest.end();
			++i )
		{
			V_CONTINUE( dxC_ModelLazyUpdate( *i ) );

			if ( TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::FLAGBIT_FRUSTUM_CULL ) )
			{
				// basic frustum culling
				if ( ! dxC_ModelInFrustum( *i, pViewer->tFrustumPlanes ) )
					continue;
			}

			V( dxC_ViewerAddModelToVisibleList_Private( pViewer, *i, NULL, nViewParams ) );
		}
	}


	SETBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_SELECTED_VISIBLE, TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ViewerAddModel( Viewer * pViewer, D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pViewer );
	ASSERT_RETINVALIDARG( pModel );

	//V( dx9_ModelSetVisible( pModel, TRUE ) );

	pViewer->tModelsToTest.push_back( pModel );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerAddModel( int nViewerID, int nModelID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );

	return dxC_ViewerAddModel( pViewer, pModel );
}


//----------------------------------------------------------------------------

PRESULT e_ViewerAddModels( int nViewerID, SIMPLE_DYNAMIC_ARRAY<int>* pModels )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	for ( unsigned int nModel = 0; nModel < pModels->Count(); nModel++ )
	{
		D3D_MODEL * pModel = dx9_ModelGet( (*pModels)[nModel] );
		dxC_ViewerAddModel( pViewer, pModel );

	}
	return S_OK;
}


//----------------------------------------------------------------------------

PRESULT e_ViewerAddAllModels( int nViewerID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	PRESULT pr = S_OK;
	for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
		  pModel;
		  pModel = dx9_ModelGetNext( pModel ) )
	{
		V_SAVE_ERROR( pr, dxC_ViewerAddModel( pViewer, pModel ) );
		if ( AppIsTugboat() )
			dxC_ModelUpdateVisibilityToken( pModel );
	}

	return pr;
}

//----------------------------------------------------------------------------

PRESULT dxC_ViewerRender( int nViewerID )
{
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );

	ASSERTV_RETFAIL( IS_VALID_INDEX( pViewer->eRenderType, NUM_VIEWER_RENDER_TYPES ), "Invalid viewer render type!  %d", pViewer->eRenderType );
	ASSERTV_RETFAIL( gpfn_ViewerRender[ pViewer->eRenderType ], "Missing viewer render function!  %d", pViewer->eRenderType );

	return gpfn_ViewerRender[ pViewer->eRenderType ]( pViewer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ViewerExists( int nViewerID )
{
	Viewer * pViewer = NULL;
	V( dxC_ViewerGet( nViewerID, pViewer ) );
	return NULL != pViewer;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)
void dxC_ViewerSaveStats( Viewer * pViewer )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_VIEWER_INFO ) )
		return;

	VIEWER_STATS & tStats = pViewer->tStats;
	const int cnLine = 2048;
	WCHAR wszLine[ cnLine ];
	WCHAR wszCmds[ cnLine ] = L"";
	V( pViewer->tCommands.GetStats( wszCmds, cnLine ) );
	PStrPrintf( wszLine, cnLine, L"Viewer %d%s\nModelsToTest: %-4d  VisibleMeshes: %-4d  Commands: %d\n%s\n",
		pViewer->nID,
		( pViewer->nID == gnMainViewerID ) ? L" (main viewer)" : L"" ,
		tStats.nModelsToTest,
		tStats.nVisibleMeshes,
		tStats.nCommands,
		wszCmds );
	PStrCat( sgwszViewersStats, wszLine, sgcnViewerStatsBufLen );
}
#endif // DEVELOPMENT

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ViewersGetStats( WCHAR * pwszStr, int nBufLen )
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETINVALIDARG( pwszStr );

	PStrCopy( pwszStr, sgwszViewersStats, nBufLen );
	sgwszViewersStats[0] = NULL;
#endif
	return S_OK;
}

}; // namespace FSSE


