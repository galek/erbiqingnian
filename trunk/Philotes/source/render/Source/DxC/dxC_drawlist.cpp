//----------------------------------------------------------------------------
// dx9_drawlist.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_main.h"
#include "e_frustum.h"
#include "dxC_render.h"
#include "dxC_target.h"
#include "dxC_profile.h"
#include "perfhier.h"
#include "e_primitive.h"
#include "dxC_material.h"
#include "dxC_occlusion.h"
#include "dxC_drawlist.h"
#include "dxC_renderlist.h"
#include "dxC_meshlist.h"

#include <algorithm>
#include <vector>


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

SIMPLE_DYNAMIC_ARRAY<D3D_DRAWLIST_COMMAND> gtDrawList[ NUM_DRAWLISTS ];
const int gnDrawListSize[ NUM_DRAWLISTS ] = 
{
	150, 10, 10, 10, 200, 100
};

CArrayIndexed<DRAWLIST_STATE> gtDrawListState;

DRAWLIST_STATE gtResetDrawListState =
{
	INVALID_ID,		// nOverrideShaderType
	INVALID_ID,		// nOverrideEnvDefID
	NULL,			// pmatView
	NULL,			// pmatProj
	NULL,			// pmatProj2
	NULL,			// pvEyeLocation
	0,				// dwMeshFlagsDoDraw
	0,				// dwMeshFlagsDontDraw
	FALSE,			// bLights
	FALSE,			// bAllowInvisibleModels
	FALSE,			// bNoBackupShaders
	FALSE,			// bNoDepthTest
	FALSE,			// bForceLoadMaterial	
};


//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

const DRAWLIST_STATE * dx9_GetDrawListState( int nDrawList )
{ 
	return gtDrawListState.Get( nDrawList );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static WCHAR * sGetDrawListNameWStr( int nDrawlist )
{
	switch ( nDrawlist )
	{
	case DRAWLIST_PRIMARY:   return L"Primary";
	case DRAWLIST_INTERFACE: return L"Interface";
	case DRAWLIST_SCREEN:    return L"Screen";
	case DRAWLIST_SCRATCH:   return L"Scratch";
	case DRAWLIST_MESH:		 return L"Mesh";
	default:                 return L"Unknown";
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static char * sGetDrawListNameStr( int nDrawlist )
{
	static char szDrawListName[ 256 ];
	PStrCvt( szDrawListName, sGetDrawListNameWStr( nDrawlist ), 256 );
	return szDrawListName;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sSetDrawListState( int nDrawList, int nState, QWORD qwValue )
{
	DRAWLIST_STATE * pDLState = gtDrawListState.Get( nDrawList );
	ASSERT_RETURN( pDLState );

	switch( nState )
	{
	case DLSTATE_OVERRIDE_SHADERTYPE:
		pDLState->nOverrideShaderType = *(int*)&qwValue; break;
	case DLSTATE_OVERRIDE_ENVDEF:
		pDLState->nOverrideEnvDefID = *(int*)&qwValue; break;
	case DLSTATE_VIEW_MATRIX_PTR:
		pDLState->pmatView = *(MATRIX**)&qwValue; break;
	case DLSTATE_PROJ_MATRIX_PTR:
		pDLState->pmatProj = *(MATRIX**)&qwValue; break;
	case DLSTATE_PROJ2_MATRIX_PTR:
		pDLState->pmatProj2 = *(MATRIX**)&qwValue; break;
	case DLSTATE_EYE_LOCATION_PTR:
		pDLState->pvEyeLocation = *(VECTOR**)&qwValue; break;
	case DLSTATE_MESH_FLAGS_DO_DRAW:
		pDLState->dwMeshFlagsDoDraw = (DWORD)qwValue; break;
	case DLSTATE_MESH_FLAGS_DONT_DRAW:
		pDLState->dwMeshFlagsDontDraw = (DWORD)qwValue; break;
	case DLSTATE_USE_LIGHTS:
		pDLState->bLights = *(BOOL*)&qwValue; break;
	case DLSTATE_ALLOW_INVISIBLE_MODELS:
		pDLState->bAllowInvisibleModels = *(BOOL*)&qwValue; break;
	case DLSTATE_NO_BACKUP_SHADERS:
		pDLState->bNoBackupShaders = *(BOOL*)&qwValue; break;
	case DLSTATE_NO_DEPTH_TEST:
		pDLState->bNoDepthTest = *(BOOL*)&qwValue; break;
	case DLSTATE_FORCE_LOAD_MATERIAL:
		pDLState->bForceLoadMaterial = *(BOOL*)&qwValue; break;
	case DLSTATE_VIEWPORT:
		pDLState->tVP = *(D3DC_VIEWPORT*)qwValue; break;
	default:
		return;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sResetDrawListState( int nDrawList )
{
	// reset all drawlist states
	DRAWLIST_STATE * pDLState = gtDrawListState.Get( nDrawList );
	if ( pDLState )
		*pDLState = gtResetDrawListState;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_AppendModelToDrawList( int nDrawList, int nModelID, int nCommandType, DWORD dwFlags )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
	{
		return;
	}

	if ( nCommandType == DLCOM_INVALID )
		nCommandType = DLCOM_DRAW;

	D3D_DRAWLIST_COMMAND * pCommand;
	int nCommandID;
	pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERT( pCommand && "Ran out of Draw List slots!" );
	pCommand->Clear();
	pCommand->nCommand = nCommandType;
	pCommand->nID = nModelID;
	pCommand->qwFlags = dwFlags;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_AddAllModelsToDrawList(
	int nDrawList,
	int nCommandType,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bForceRender )
{
	// filter based on model flags?

	BOOL bFrustumCullEnabled = e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS );

	// get the view parameters
	MATRIX matWorld, matView, matProj;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	if ( bFrustumCullEnabled )
	{
		// make sure frustum is filled in
		e_InitMatrices( &matWorld, &matView, &matProj, NULL, &vEyeVector, &vEyeLocation );
	}

	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );


	for (D3D_MODEL* pModel = dx9_ModelGetFirst(); pModel; pModel = dx9_ModelGetNext(pModel))
	{
		if ( nIsolateModel != INVALID_ID && nIsolateModel != pModel->nId )
			continue;

		BOOL bAdd = TRUE;
		BOOL bInFrustum = TRUE;

		if ( bFrustumCullEnabled )
		{
			// frustum cull
			bInFrustum = e_ModelInFrustum( pModel->nId, e_GetCurrentViewFrustum()->GetPlanes() );
		}

		bAdd = bInFrustum;

		if ( bAdd )
		{
			// ensure model is visible
			dxC_ModelUpdateVisibilityToken( pModel );

			// if this model has been debug marked, render visible bounding box around it
			if ( gnDebugIdentifyModel != INVALID_ID && gnDebugIdentifyModel == pModel->nId && nCommandType == DLCOM_DRAW )
				dx9_DrawListAddModel( 
					NULL,
					nDrawList,
					pModel->nId,
					vEyeLocation,
					dwModelDefFlagsDoDraw,
					dwModelDefFlagsDontDraw,
					FALSE,
					DLCOM_DRAW_BOUNDING_BOX,
					FALSE,
					NULL,
					TRUE,
					TRUE );

			if ( ! bForceRender )
			{
				if ( ! dx9_ModelShouldRender( pModel ) )
					continue;

				D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
																e_ModelGetDisplayLOD( pModel->nId ) );
				if ( ! pModelDefinition )
					continue;

				if ( ! dx9_ModelDefinitionShouldRender( pModelDefinition, pModel, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw ) )
					continue;
			}


			dx9_AppendModelToDrawList( nDrawList, pModel->nId, nCommandType );
		}
		else
		{
			// not visible -- make it so
			//e_ModelSetVisible( pModel->nId, FALSE );
		}
	}


}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, int nData, void * pData, void * pData2, QWORD qwFlags, BOUNDING_BOX * pBBox )
{
	D3D_DRAWLIST_COMMAND * pCommand;
	int nCommandID;
	pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERT( pCommand && "Ran out of Draw List slots!" );
	pCommand->Clear();
	pCommand->nSortValue	  = -1;
	pCommand->pData			  = pData;
	pCommand->pData2		  = pData2;
	pCommand->nCommand		  = nCommandType;
	pCommand->nID			  = nID;
	pCommand->nData			  = nData;
	pCommand->qwFlags		  = qwFlags;
	if ( pBBox )
		pCommand->bBox			  = *pBBox;

	return nCommandID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, void * pData, void * pData2, DWORD dwFlags )
{
	dx9_DrawListAddCommand( nDrawList, nCommandType, INVALID_ID, 0, pData, pData2, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, void * pData, void * pData2 , DWORD dwFlags )
{
	dx9_DrawListAddCommand( nDrawList, nCommandType, nID, 0, pData, pData2, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, int nData, DWORD dwFlags )
{
	dx9_DrawListAddCommand( nDrawList, nCommandType, nID, nData, NULL, NULL, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, DWORD dwFlags )
{
	dx9_DrawListAddCommand( nDrawList, nCommandType, INVALID_ID, 0, NULL, NULL, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
D3D_DRAWLIST_COMMAND * dx9_DrawListAddCommand( int nDrawList )
{
	D3D_DRAWLIST_COMMAND * pCommand;
	int nCommandID;
	pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERT( pCommand && "Ran out of Draw List slots!" );
	pCommand->Clear();
	return pCommand;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCommand( int nDrawList, const D3D_DRAWLIST_COMMAND * pSrcCommand )
{
	D3D_DRAWLIST_COMMAND * pCommand = dx9_DrawListAddCommand( nDrawList );
	*pCommand = *pSrcCommand;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddCallback( int nDrawList, PFN_DRAWLIST_CALLBACK pCallback, int nID, int nData, void * pData2, DWORD dwFlags, BOUNDING_BOX * pBBox )
{
	dx9_DrawListAddCommand( nDrawList, DLCOM_CALLBACK, nID, nData, pCallback, pData2, dwFlags, pBBox );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddModel(
	SIMPLE_DYNAMIC_ARRAY< D3D_MODEL* >* Models,
	int nDrawList,
	int nModelID,
	VECTOR & vEyeLocation,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bOccluded,
	int nCommandType,
	BOOL bQueryOcclusion,
	CArrayIndexed<D3D_DRAWLIST_COMMAND>::PFN_COMPARE_FUNCTION pfnSortFunction,
	BOOL bAllowInvisible,
	BOOL bNoDepthTest )
{
	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	if ( nIsolateModel != INVALID_ID && nIsolateModel != nModelID )
		return;

	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETURN(pModel);

	// if this model has been debug marked, render visible bounding box around it
	if ( gnDebugIdentifyModel != INVALID_ID && gnDebugIdentifyModel == nModelID && nCommandType == DLCOM_DRAW )
	{
		//dx9_DrawListAddModel( nDrawList, nModelID, vEyeLocation, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw, FALSE, DLCOM_DRAW_BOUNDING_BOX, FALSE, NULL, TRUE, TRUE );
		VECTOR * pOBB = e_GetModelRenderOBBInWorld( nModelID );
		V( e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, pOBB, 0x00101320 ) );
	}

	if ( ! dx9_ModelShouldRender( pModel ) && ! bAllowInvisible )
		return;
	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return;
	if ( ! dx9_ModelDefinitionShouldRender( pModelDefinition, pModel, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw ) && ! bAllowInvisible )
		return;


	int nCommand = nCommandType;
	if ( bOccluded == TRUE )
		nCommand = DLCOM_DRAW_BOUNDING_BOX;

	if ( AppIsHellgate() )
	{
	    if ( bNoDepthTest )
		    DL_SET_STATE_DWORD( nDrawList, DLSTATE_NO_DEPTH_TEST, TRUE );
	    if ( bAllowInvisible )
		    DL_SET_STATE_DWORD( nDrawList, DLSTATE_ALLOW_INVISIBLE_MODELS, TRUE );
	}

	if ( pfnSortFunction )
	{
		// sorted

		// can't sort this way -- need separate model list to sort, followed by append
		// unless the "sort order" values on all appended commands was incremented like 1000, 2000, 3000, etc.
		// then could sort a bunch of models like 1024, 1026, 1029, 1030, 1044, etc.

		//D3DXVECTOR4 vEyeLocationInObject;
		//MatrixMultiply( (VECTOR *)&vEyeLocationInObject, &vEyeLocation, (MATRIX *)&pModel->matScaledWorldInverse );

		//BOUNDING_BOX tBBox = pModelDefinition->tRenderBoundingBox;
		//c_sGetModelRenderBoundingBox( nModelID, &tBBox );
		//float fDistanceToEye = BoundingBoxManhattanDistance( &tBBox, (VECTOR *)&vEyeLocationInObject );
		//fDistanceToEye *= pModel->fScale;
		//int nSortValue = (int)fDistanceToEye;

		//D3D_DRAWLIST_COMMAND * pCommand;
		//int nCommandID;
		//pCommand = sgtDrawList[ nDrawList ].InsertSorted( &nCommandID, nSortValue, pfnSortFunction );
		//ASSERT( pCommand && "Ran out of Draw List slots!" );
		//pCommand->Clear();
		//pCommand->nSortValue = nSortValue;
		//pCommand->nCommand = nCommand;
		//pCommand->nID = nModelID;
		//pCommand->dwFlags = bQueryOcclusion ? DLFLAG_QUERY_OCCLUSION : 0;
	} else {

		// unsorted
		if ( AppIsTugboat() )
		{
			//Models->push_back( pModel );
			dx9_AppendModelToDrawList( nDrawList, nModelID, nCommandType, 0 );
		}
		else
		{
		    DWORD dwFlags = 0;
		    dwFlags |= bQueryOcclusion    ? DLFLAG_QUERY_OCCLUSION    : 0;
		    dx9_AppendModelToDrawList( nDrawList, nModelID, nCommandType, dwFlags );
		}
	}

	if ( AppIsHellgate() )
	{
	    if ( bNoDepthTest )
		    DL_SET_STATE_DWORD( nDrawList, DLSTATE_NO_DEPTH_TEST, FALSE );
	    if ( bAllowInvisible )
		    DL_SET_STATE_DWORD( nDrawList, DLSTATE_ALLOW_INVISIBLE_MODELS, FALSE );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddClipPlanes( int nDrawList, const BOUNDING_BOX *pClipBox )
{
	ASSERT( pClipBox );
	int nCommandID = -1;
	D3D_DRAWLIST_COMMAND * pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERTX( pCommand, "Ran out of Draw List slots!" );
	pCommand->Clear();
	pCommand->nCommand = DLCOM_SET_CLIP;
	pCommand->nID = -1;
	pCommand->bBox = *pClipBox;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddScissorRects( int nDrawList, const BOUNDING_BOX *pScissorRect )
{
	ASSERT( pScissorRect );
	int nCommandID = -1;
	D3D_DRAWLIST_COMMAND * pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERTX( pCommand, "Ran out of Draw List slots!" );
	pCommand->Clear();
	pCommand->nCommand = DLCOM_SET_SCISSOR;
	pCommand->nID = -1;
	pCommand->bBox = *pScissorRect;

	// swap and negate
	float fTemp = pCommand->bBox.vMin.fY;
	pCommand->bBox.vMin.fY = - pCommand->bBox.vMax.fY;
	pCommand->bBox.vMax.fY = - fTemp;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_DrawListAddViewport( int nDrawList, const BOUNDING_BOX *pViewportParams )
{
	ASSERT( pViewportParams );
	int nCommandID = -1;
	D3D_DRAWLIST_COMMAND * pCommand = ArrayAddGetIndex(gtDrawList[nDrawList], &nCommandID);
	ASSERTX( pCommand, "Ran out of Draw List slots!" );
	pCommand->Clear();
	pCommand->nCommand = DLCOM_SET_VIEWPORT;
	pCommand->nID = -1;
	pCommand->bBox = *pViewportParams;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_ModelGetDistanceToEye( int nModelID, VECTOR & vEyeLocation )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETVAL(pModel, 0.0f);

	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
													e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return 0.0f;

	//D3DXVECTOR4 vEyeLocationInObject;
	//MatrixMultiply( (VECTOR *)&vEyeLocationInObject, &vEyeLocation, (MATRIX *)&pModel->matScaledWorldInverse );

	BOUNDING_BOX tBBox;
	V( e_GetModelRenderAABBInWorld( nModelID, &tBBox ) );
	float fDistanceToEye = BoundingBoxManhattanDistance( &tBBox, (VECTOR *)&vEyeLocation );
	fDistanceToEye *= e_ModelGetScale( nModelID ).fX;
	return fDistanceToEye;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_ClearDrawList ( int nDrawList )
{
	ArrayClear(gtDrawList[nDrawList]);
	dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_STATE );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sDrawListDepthLastCompareGreaterThan( D3D_DRAWLIST_COMMAND * pFirst, int nKey )
{
	if ( nKey > pFirst->nSortValue )
	{
		return -1;
	}
	else if ( nKey < pFirst->nSortValue )
	{
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sDrawListDepthFirstCompareLessThan( D3D_DRAWLIST_COMMAND * pFirst, int nKey )
{
	if ( nKey < pFirst->nSortValue )
	{
		return -1;
	}
	else if ( nKey > pFirst->nSortValue )
	{
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define MESH_RENDER_COMMIT() \
	sSortMeshDrawList(); \
	dx9_RenderDrawList( DRAWLIST_MESH ); \
	dxC_RenderableMeshListClear(); \
	DL_CLEAR_LIST( DRAWLIST_MESH ); \

PRESULT dx9_RenderDrawList (
	int nDrawList )
{
	D3D_PROFILE_REGION_STR( L"Draw List ", sGetDrawListNameWStr( nDrawList ) );

	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	ASSERT( pState );
	if ( !pState )
		pState = &gtResetDrawListState;

	DWORD dwRenderStateFlags = 0;

	int nNumCommands = gtDrawList[ nDrawList ].Count();
	for ( int nCommandIndex = 0; nCommandIndex < nNumCommands; nCommandIndex++ )
	{
		D3D_DRAWLIST_COMMAND* pCommand = gtDrawList[ nDrawList ].GetPointer( nCommandIndex );
		ASSERT( pCommand );

		// CHB 2007.01.17 - Skip model drawing only if model drawing is off.
		if ( !e_GetRenderFlag( RENDER_FLAG_RENDER_MODELS ) )
		{
			switch ( pCommand->nCommand )
			{
				// Who's Who of model drawing commands.
				case DLCOM_DRAW:
				case DLCOM_DRAW_BEHIND:
				case DLCOM_DRAW_BOUNDING_BOX:
				case DLCOM_DRAW_SHADOW:
				case DLCOM_DRAW_DEPTH:
				case DLCOM_DRAW_BLOB_SHADOW:
					// Note, the continue applies to the for loop,
					// though the break applies to the switch.
					continue;
				default:
					break;
			}
		}

		if ( gbDumpDrawList )
		{
			V( dx9_DumpDrawListCommand( nDrawList, pCommand ) );
		}

		switch ( (DRAWLIST_COMMAND_TYPE)pCommand->nCommand )
		{
		case DLCOM_DRAW:
			{
				V( dx9_RenderModel( nDrawList, pCommand->nID ) );

				if ( e_GetRenderFlag( RENDER_FLAG_RENDER_BOUNDING_BOX ) )
				{
					V( dx9_RenderModelBoundingBox( nDrawList, pCommand->nID, TRUE ) );
				}
			}
			break;
		case DLCOM_DRAW_BEHIND:
			if ( AppIsTugboat() )
			{
				V( dx9_RenderModelBehind( nDrawList, pCommand->nID ) );
			}
			break;
		case DLCOM_DRAW_BOUNDING_BOX:
			{
				V( dx9_RenderModelBoundingBox( nDrawList, pCommand->nID, TRUE ) );
			}
			break;
		case DLCOM_DRAW_SHADOW:
			{
				V( dx9_RenderModelShadow( nDrawList, pCommand->nData, pCommand->nID ) );
			}
			break;
		case DLCOM_DRAW_BLOB_SHADOW:
			{
				V( dxC_RenderBlobShadows( pCommand->pData ) );
			}
			break;
		case DLCOM_DRAW_DEPTH:
			{
				V( dx9_RenderModelDepth( nDrawList, pCommand->nID ) );
			}			
			break;
		case DLCOM_DRAW_LAYER:
			{
				V( dx9_RenderLayer( pCommand->nID, *(LPD3DCSHADERRESOURCEVIEW*)&pCommand->pData, *(LPD3DCSHADERRESOURCEVIEW*)&pCommand->pData2, pCommand->nData ) );
			}
			break;
		case DLCOM_SET_CLIP:
			{
				V( dxC_SetClipPlaneRect( &pCommand->bBox, (D3DXMATRIXA16*)pState->pmatProj ) );
			}
			break;
		case DLCOM_RESET_CLIP:
			{
				V( dxC_ResetClipPlanes() );
			}
			break;
		case DLCOM_SET_SCISSOR:
			//V( dxC_SetScissorRect( pCommand->bBox ) );
			break;
		case DLCOM_RESET_SCISSOR:
			//V( dxC_SetScissorRect( NULL ) );
			break;
		case DLCOM_SET_VIEWPORT:
			{
				V( dxC_SetViewport( 
					DWORD( pCommand->bBox.vMin.fX ),
					DWORD( pCommand->bBox.vMin.fY ), 
					DWORD( pCommand->bBox.vMax.fX - pCommand->bBox.vMin.fX ),
					DWORD( pCommand->bBox.vMax.fY - pCommand->bBox.vMin.fY ),
					pCommand->bBox.vMin.fZ,
					pCommand->bBox.vMax.fZ ) );
			}
			break;
		case DLCOM_RESET_VIEWPORT:
			{
				V( dxC_ResetViewport() );
			}
			break;
		case DLCOM_RESET_FULLVIEWPORT:			
			{
				V( dxC_ResetFullViewport() );
			}
			break;
		case DLCOM_SET_RENDER_DEPTH_TARGETS:
			{
				V( dxC_SetRenderTargetWithDepthStencil( (RENDER_TARGET_INDEX)pCommand->nID, (DEPTH_TARGET_INDEX)pCommand->nData ) );
			}
			break;
		case DLCOM_CLEAR_COLOR:
			{
				V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET, (DWORD)pCommand->qwFlags ) );
			}
			break;
		case DLCOM_CLEAR_COLORDEPTH:
			{
				V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, (DWORD)pCommand->qwFlags ) );
			}
			break;
		case DLCOM_CLEAR_COLORFLOATDEPTH:
			{
#ifdef ENGINE_TARGET_DX9
				ASSERTX( 0, "Floating point color clears not supported in Dx9" );
#elif defined( ENGINE_TARGET_DX10)
				dxC_ClearFPBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, (float*)pCommand->pData );
#endif
			}
		case DLCOM_CLEAR_DEPTH:
			{
				V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_ZBUFFER ) );
			}
			break;
		case DLCOM_CLEAR_DEPTH_VALUE:
			{
				V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_ZBUFFER, 0, *(float*)pCommand->pData ) );
			}
			break;
		case DLCOM_COPY_BACKBUFFER:
			{
				V( dxC_CopyBackbufferToTexture( *(SPD3DCTEXTURE2D*)&pCommand->pData, NULL, NULL ) );
			}
			break;
		case DLCOM_COPY_RENDERTARGET:
			{
				V( dxC_CopyRenderTargetToTexture( (RENDER_TARGET_INDEX)pCommand->nID, *(SPD3DCTEXTURE2D*)&pCommand->pData, pCommand->bBox ) );
			}
			break;
		case DLCOM_RESET_STATE:
			sResetDrawListState( nDrawList );
			break;
		case DLCOM_SET_STATE:
			sSetDrawListState( nDrawList, pCommand->nID, pCommand->qwFlags );
			break;
		case DLCOM_CALLBACK:
			{
				if ( pCommand->pData )
				{
					V( ( (PFN_DRAWLIST_CALLBACK)pCommand->pData )( nDrawList, pCommand ) );
				}
			}
			break;
		case DLCOM_FENCE:
			// Encountered fence, which means that we wanted to separate mesh draw commands.
			break;
		case DLCOM_INVALID:
		default:
			ASSERTX( 0, "Invalid Draw List Command!" );
			continue;
		}
	}

	// restore full-screen scissor rect & clip planes
	//dxC_SetScissorRect( NULL );
	//dx9_SetClipPlanes( NULL );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_DrawListsInit()
{
	for (int ii = 0; ii < NUM_DRAWLISTS; ii++)
	{
		ArrayInit(gtDrawList[ii], NULL, gnDrawListSize[ii]);
	}
	ArrayInit(gtDrawListState, NULL, NUM_DRAWLISTS);

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_DrawListsDestroy()
{
	for ( int i = 0; i < NUM_DRAWLISTS; i++ )
		gtDrawList[ i ].Destroy();
	gtDrawListState.Destroy();

	return S_OK;
}
