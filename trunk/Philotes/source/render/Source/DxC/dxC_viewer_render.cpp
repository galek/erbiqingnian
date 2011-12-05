//----------------------------------------------------------------------------
// dxC_viewer.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "camera_priv.h"
#include "plane.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_renderlist.h"
#include "dxC_meshlist.h"
#include "dxC_render.h"
#include "dxC_pass.h"
#include "dxC_target.h"
#include "e_main.h"
#include "dxC_rchandlers.h"
#include "dxC_environment.h"
#include <algorithm>
#include "e_shadow.h"
#include "e_optionstate.h"
#include "dx9_shadowtypeconstants.h"

#ifdef ENABLE_DPVS
#include "dxC_dpvs.h"
#endif

#include "dxC_viewer.h"

#ifdef ENGINE_TARGET_DX10
#include "dx10_effect.h"
#endif

extern DRAWLIST_STATE gtResetDrawListState;

namespace FSSE
{;

#define CREATE_RCHDATA( eType, nID, pData )				\
	int nID;											\
	RCD_ARRAY_NEW( eType, nID );						\
	RCD_TYPE( eType ) * pData;							\
	RCD_ARRAY_GET( eType, nID, pData );


//----------------------------------------------------------------------------
// HELPER FUNCTIONS
//----------------------------------------------------------------------------

typedef BOOL (*PFN_MESHLISTCMP)( MeshDrawSortable & tMD1, MeshDrawSortable & tMD2 );

static BOOL sMeshListCmpFrontToBack( MeshDrawSortable & tMD1, MeshDrawSortable & tMD2 )
{
	return tMD1.fDist < tMD2.fDist;
}

static BOOL sMeshListCmpBackToFront( MeshDrawSortable & tMD1, MeshDrawSortable & tMD2 )
{
	return tMD1.fDist > tMD2.fDist;
}

static BOOL sMeshListCmpState( MeshDrawSortable & tMD1, MeshDrawSortable & tMD2 )
{
	// Should check and guard against this up higher to keep the sort fast.
	//if ( ! tMD1.pTechnique || ! tMD2.pTechnique )
	//	return TRUE;

	return tMD1.pTechnique->hHandle > tMD2.pTechnique->hHandle;
}

PFN_MESHLISTCMP gpfn_MeshListCmp[ NUM_SORT_CRITERIA ] =
{
	NULL,	// none
	sMeshListCmpFrontToBack,
	sMeshListCmpBackToFront,
	sMeshListCmpState,
};

//----------------------------------------------------------------------------

static BOOL sCullMesh(
	const VISIBLE_MESH * pVM,
	const struct RENDER_CONTEXT & tContext,
	const RenderPassDefinition & tRPD,
	DWORD dwMeshFlags )
{
	if ( ! tContext.tState.bAllowInvisibleModels )
		if ( !dx9_ModelShouldRender( pVM->pModel ) )
			return TRUE;

	if ( dxC_ModelGetDrawLayer( pVM->pModel ) != tRPD.eDrawLayer )
		return TRUE;

	//DWORD dwModelFlags = pVM->pModel->dwFlags;

	C_ASSERT( sizeof(pVM->pModel->dwFlagBits) == sizeof(tRPD.dwModelFlags_MustNotHave ) );
	C_ASSERT( sizeof(pVM->pModel->dwFlagBits) == sizeof(tRPD.dwModelFlags_MustHaveSome) );
	C_ASSERT( sizeof(pVM->pModel->dwFlagBits) == sizeof(tRPD.dwModelFlags_MustHaveAll ) );

	// model flags -- old way
	//if ( tRPD.dwModelFlags_MustNotHave	&& 0								!=	( dwModelFlags 	& tRPD.dwModelFlags_MustNotHave  ) )	return TRUE;
	//if ( tRPD.dwModelFlags_MustHaveSome	&& 0								==	( dwModelFlags 	& tRPD.dwModelFlags_MustHaveSome ) )	return TRUE;
	//if ( tRPD.dwModelFlags_MustHaveAll	&& tRPD.dwModelFlags_MustHaveAll	!=	( dwModelFlags 	& tRPD.dwModelFlags_MustHaveAll  ) )	return TRUE;


	// model flags -- must not have
	for ( int nSet = 0; nSet < DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS); ++ nSet )
		if ( tRPD.dwModelFlags_MustNotHave[nSet]	&& 0								!=	( pVM->pModel->dwFlagBits[nSet] 	& tRPD.dwModelFlags_MustNotHave[nSet]  ) )	return TRUE;



	// model flags -- must have some
	BOOL bHasSome = FALSE;
	BOOL bWantSome = FALSE;
	for ( int nSet = 0; nSet < DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS); ++ nSet )
	{
		if ( tRPD.dwModelFlags_MustHaveSome[nSet] )
			bWantSome = TRUE;
		if ( tRPD.dwModelFlags_MustHaveSome[nSet]	&& 0								!=	( pVM->pModel->dwFlagBits[nSet]		& tRPD.dwModelFlags_MustHaveSome[nSet] ) )
		{
			bHasSome = TRUE;
			break;
		}
	}
	if ( bWantSome != bHasSome )
		return TRUE;


	// model flags -- must have all
	for ( int nSet = 0; nSet < DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS); ++ nSet )
		if ( tRPD.dwModelFlags_MustHaveAll[nSet]	&& tRPD.dwModelFlags_MustHaveAll[nSet]	!=	( pVM->pModel->dwFlagBits[nSet] 	& tRPD.dwModelFlags_MustHaveAll[nSet]  ) )	return TRUE;



	// mesh flags
	if ( tRPD.dwMeshFlags_MustNotHave	&& 0								!=	( dwMeshFlags 	& tRPD.dwMeshFlags_MustNotHave 	 ) )	return TRUE;
	if ( tRPD.dwMeshFlags_MustHaveSome	&& 0								==	( dwMeshFlags 	& tRPD.dwMeshFlags_MustHaveSome	 ) )	return TRUE;
	if ( tRPD.dwMeshFlags_MustHaveAll 	&& tRPD.dwMeshFlags_MustHaveAll		!=	( dwMeshFlags 	& tRPD.dwMeshFlags_MustHaveAll 	 ) )	return TRUE;

	BOOL bHasAlpha = ( dwMeshFlags & MESH_FLAG_ALPHABLEND ) || dxC_ModelGetFlagbit( pVM->pModel, MODEL_FLAGBIT_FORCE_ALPHA );
	BOOL bWantAlpha = 0 != ( tRPD.dwPassFlags & PASS_FLAG_ALPHABLEND );
	if ( bHasAlpha != bWantAlpha )
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------

static PRESULT sRenderContextSetViewParams( const Viewer * pViewer, RENDER_CONTEXT & tContext, int nViewParams )
{
	ViewParameters * pViewParams = NULL;

	// initialize with defaults
	MATRIX mWorld;
	VECTOR vEyePos_w;
	VECTOR vEyeDir_w;
	e_InitMatrices(
		&mWorld,
		&tContext.matView,
		&tContext.matProj,
		&tContext.matProj2,
		&vEyeDir_w,
		&vEyePos_w,
		NULL,
		TESTBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_ALLOW_STALE_CAMERA ),
		&pViewer->tCameraInfo );
	tContext.vEyeDir_w = vEyeDir_w;
	tContext.vEyePos_w = vEyePos_w;
	tContext.pEnvDef = e_GetCurrentEnvironmentDef();
	dxC_GetFullViewportDesc( tContext.tState.tVP );
	dxC_GetFullScissorRect( tContext.tScissor );


	if (    nViewParams == INVALID_ID 
		 || (    nViewParams == 0 
		      && ! TESTBIT( pViewer->dwFlagBits, Viewer::FLAGBIT_IGNORE_GLOBAL_CAMERA_PARAMS ) ) )
	{
		// This is the "global" level.

		goto finalize;
	}

	//ASSERT_RETINVALIDARG( nViewParams < 1 + MAX_PORTAL_DEPTH );

	V_RETURN( dxC_ViewParametersGet( nViewParams, pViewParams ) );
	ASSERT_RETFAIL( pViewParams );

	if ( TESTBIT_DWORD( pViewParams->dwFlagBits, ViewParameters::PROJ_MATRIX ) )
	{
		tContext.matProj  = pViewParams->mProj;
		//tContext.matProj2 = pViewParams->mProj;
	}

	if ( TESTBIT_DWORD( pViewParams->dwFlagBits, ViewParameters::VIEW_MATRIX ) )
		tContext.matView = pViewParams->mView;

	if ( TESTBIT_DWORD( pViewParams->dwFlagBits, ViewParameters::ENV_DEF ) )
		tContext.pEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pViewParams->nEnvDefID );

	if ( TESTBIT_DWORD( pViewParams->dwFlagBits, ViewParameters::EYE_POS ) )
		tContext.vEyePos_w = pViewParams->vEyePos;

	//dxC_GetFullViewportDesc( tContext.tState.tVP );
	if ( TESTBIT_DWORD( pViewParams->dwFlagBits, ViewParameters::SCISSOR_RECT ) )
	{
		//tContext.tState.tVP.TopLeftX = pViewParams->tScissor.left;
		//tContext.tState.tVP.TopLeftY = pViewParams->tScissor.top;
		//tContext.tState.tVP.Width    = pViewParams->tScissor.right - pViewParams->tScissor.left;
		//tContext.tState.tVP.Height   = pViewParams->tScissor.bottom - pViewParams->tScissor.top;
		tContext.tScissor = pViewParams->tScissor;
	}

finalize:
	ASSERT_RETFAIL( tContext.pEnvDef );

	tContext.tState.pmatView		= &tContext.matView;
	tContext.tState.pmatProj		= &tContext.matProj;
	tContext.tState.pmatProj2		= &tContext.matProj2;
	tContext.tState.pvEyeLocation	= &tContext.vEyePos_w;
	tContext.tState.bLights			= TRUE;

	// CHB 2007.07.04 - This might be necessary.
	//e_EnvironmentApplyCoefs( tContext.pEnvDef );

	return S_OK;
}


//----------------------------------------------------------------------------
// VIEWER_RENDER_COMMON
//----------------------------------------------------------------------------

static PRESULT sVRCommon_SetupFrame( Viewer * pViewer, RENDER_CONTEXT & tGlobalContext )
{
	ASSERT_RETINVALIDARG( pViewer );

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) );

	//		set targets
	{
		if ( pViewer->eSwapRT != SWAP_RT_INVALID || pViewer->eSwapDT != SWAP_DT_INVALID )
		{
			RENDER_TARGET_INDEX eRT	= pViewer->eSwapRT == SWAP_RT_INVALID	? dxC_GetCurrentRenderTarget() : dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, pViewer->eSwapRT );
			DEPTH_TARGET_INDEX  eDT	= pViewer->eSwapDT == SWAP_DT_INVALID	? dxC_GetCurrentDepthTarget()  : dxC_GetSwapChainDepthTargetIndex(  pViewer->nSwapChainIndex, pViewer->eSwapDT );

#ifndef ENGINE_TARGET_DX10  //For Dx10 we need to control this per pass
			CREATE_RCHDATA( SET_TARGET, nID, pData );
			pData->nLevel = 0;
			pData->eColor[ 0 ] = eRT;
			pData->nRTCount = 1;
			pData->eDepth = eDT;
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_TARGET, nID );
#endif 
		}
	}



	//		set viewport
	{
		CREATE_RCHDATA( SET_VIEWPORT, nID, pData );
		pData->tVP = pViewer->tVP;
		dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_VIEWPORT, nID );
	}


	//		clear targets
	{
		CREATE_RCHDATA( CLEAR_TARGET, nID, pData );
		pData->dwFlags		= D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
#ifdef ENGINE_TARGET_DX10
		pData->tColor[0]	= ARGB_DWORD_TO_RGBA_D3DXVECTOR4( e_GetBackgroundColor( tGlobalContext.pEnvDef ) );
#else
		pData->tColor[0]	= e_GetBackgroundColor( tGlobalContext.pEnvDef );
#endif
		pData->fDepth		= 1.f;
		pData->nStencil		= 0;
		pData->nRTCount		= 1;
		dxC_CommandAdd( pViewer->tCommands, RenderCommand::CLEAR_TARGET, nID );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ViewerRender_Common( Viewer * pViewer )
{
	ASSERT_RETINVALIDARG( pViewer );

	// Sometimes a full render is called after a loading sequence before the room update has been called.
	if ( ! TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::FLAGBIT_SELECTED_VISIBLE ) )
		return S_FALSE;

	pViewer->tCommands.Clear();

	// a holder for skybox callback data
	SKYBOX_CALLBACK_DATA tSkyboxData[ NUM_SKYBOX_PASSES ];


	//		determine render context (stack matching ViewParameters)
	//const int NUM_CONTEXTS = 1 + MAX_PORTAL_DEPTH;
	int nViewParams = dxC_ViewParametersGetCount();
	//ASSERT( NUM_CONTEXTS >= nViewParams );
	//RENDER_CONTEXT tContexts[ NUM_CONTEXTS ];
	V_RETURN( pViewer->tContexts.Resize( nViewParams ) );
	RENDER_CONTEXT * tContexts = pViewer->tContexts;

	for ( int i = 0; i < nViewParams; i++ )
	{
		tContexts[ i ].Reset();
		V( sRenderContextSetViewParams( pViewer, tContexts[i], i ) );
	}


	// Set and clear render target.
	V_RETURN( sVRCommon_SetupFrame( pViewer, tContexts[0] ) );


	// This is the number of pass "sections".  In addition to the "global section", a new section is
	// begun every time a portal is entered or exited.
	int nSections = ( nViewParams - 1 ) * 2 + 1;
	//const int cnMaxSections = ( NUM_CONTEXTS - 1 ) * 2 + 1;


	// This is the order in which passes are to be rendered.
	const int cnMaxPasses = 16;
	RENDER_PASS_TYPE ePasses[ cnMaxPasses ];
	int nPassCount = 0;


#define ADD_PASS(passtype)							\
	{ ASSERT_RETFAIL( nPassCount < cnMaxPasses );	\
	  ePasses[nPassCount++] = passtype; }

#define ADD_PASS_IF(passtype,flagbit)								\
	if ( TESTBIT_DWORD( pViewer->dwFlagBits, Viewer::flagbit ) )	\
		ADD_PASS(passtype)

	if ( AppIsTugboat() )
	{
		if ( e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ) {
			ADD_PASS( RPTYPE_DEPTH_SCENE	);
			ADD_PASS( RPTYPE_DEPTH_ALPHA_SCENE );
		}

		if ( e_GetActiveShadowTypeForEnv( FALSE, tContexts[0].pEnvDef ) != SHADOW_TYPE_NONE )
		{
			ADD_PASS_IF(   RPTYPE_OPAQUE_PRE_BEHIND,		FLAGBIT_ENABLE_BEHIND		)
			else ADD_PASS( RPTYPE_OPAQUE_SCENE											);
		}
		else
		{
			ADD_PASS_IF(   RPTYPE_OPAQUE_PRE_BLOB_BEHIND,	FLAGBIT_ENABLE_BEHIND		)
			else ADD_PASS( RPTYPE_OPAQUE_PRE_BLOB										);

			ADD_PASS( RPTYPE_OPAQUE_BLOB		);

			ADD_PASS_IF(   RPTYPE_OPAQUE_POST_BLOB_BEHIND,	FLAGBIT_ENABLE_BEHIND		)
			else ADD_PASS( RPTYPE_OPAQUE_POST_BLOB										);
		}
	
		ADD_PASS_IF( RPTYPE_OPAQUE_SKYBOX, 		FLAGBIT_ENABLE_SKYBOX		);
		ADD_PASS_IF( RPTYPE_OPAQUE_BEHIND, 		FLAGBIT_ENABLE_BEHIND		);
		ADD_PASS_IF( RPTYPE_OPAQUE_POST_BEHIND,	FLAGBIT_ENABLE_BEHIND		);
		ADD_PASS_IF( RPTYPE_ALPHA_SKYBOX,  		FLAGBIT_ENABLE_SKYBOX		);

		ADD_PASS_IF( RPTYPE_PARTICLES_ENV,		FLAGBIT_ENABLE_PARTICLES	);
		ADD_PASS( RPTYPE_ALPHA_SCENE		);
		ADD_PASS_IF( RPTYPE_PARTICLES_GENERAL,	FLAGBIT_ENABLE_PARTICLES	);
	}
	else
	{
		if ( e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ) {
			ADD_PASS( RPTYPE_DEPTH_1P		);
			ADD_PASS( RPTYPE_DEPTH_SCENE	);
			ADD_PASS( RPTYPE_DEPTH_ALPHA_SCENE );
		}
		ADD_PASS( RPTYPE_OPAQUE_1P			);

		if ( e_GetActiveShadowTypeForEnv( FALSE, tContexts[0].pEnvDef ) != SHADOW_TYPE_NONE ) {
			ADD_PASS( RPTYPE_OPAQUE_SCENE		);
		} else {
			ADD_PASS( RPTYPE_OPAQUE_PRE_BLOB	);
			ADD_PASS( RPTYPE_OPAQUE_BLOB		);
			ADD_PASS( RPTYPE_OPAQUE_POST_BLOB	);
		}

		ADD_PASS_IF( RPTYPE_OPAQUE_SKYBOX, 		FLAGBIT_ENABLE_SKYBOX		);
		ADD_PASS_IF( RPTYPE_ALPHA_SKYBOX,  		FLAGBIT_ENABLE_SKYBOX		);
		ADD_PASS_IF( RPTYPE_PARTICLES_ENV,		FLAGBIT_ENABLE_PARTICLES	);
		ADD_PASS( RPTYPE_ALPHA_SCENE		);
		ADD_PASS( RPTYPE_ALPHA_1P			);
		ADD_PASS_IF( RPTYPE_PARTICLES_GENERAL,	FLAGBIT_ENABLE_PARTICLES	);
	}
#undef ADD_PASS


	const int cnPassCount = nPassCount;
	int nPassTypeIndexList[ NUM_RENDER_PASS_TYPES ];
	memset( nPassTypeIndexList, -1, sizeof(nPassTypeIndexList) );

	ASSERT( NUM_RENDER_PASS_TYPES >= cnPassCount );
	// Determine where each pass type is in the list of passes.
	for ( int eType = 0; eType < NUM_RENDER_PASS_TYPES; ++eType )
	{
		for ( int i = 0; i < cnPassCount; i++ )
		{
			if ( ePasses[i] == (RENDER_PASS_TYPE)eType )
			{
				nPassTypeIndexList[ eType ] = i;
				break;
			}
		}
	}

	// There are (num_portals + 1) contexts, and a full pass type set for each.
	//RenderPassDefinition tRPDs[ NUM_CONTEXTS ][ NUM_RENDER_PASS_TYPES ];
	V_RETURN( pViewer->tRPDs.Resize( nViewParams * NUM_RENDER_PASS_TYPES ) );
	RenderPassDefinition * ptRPDs = pViewer->tRPDs;
#define TRPDS(nContext,nPass) ptRPDs[ nContext * NUM_RENDER_PASS_TYPES + nPass ]

	for ( int nContext = 0; nContext < nViewParams; ++nContext )
	{
		for ( int nPass = 0; nPass < cnPassCount; ++nPass )
		{
			RENDER_PASS_TYPE ePassType = ePasses[ nPass ];
			V_CONTINUE( dxC_PassDefinitionGet( ePassType, TRPDS( nContext, nPass ) ) );
			TRPDS( nContext, nPass ).nMeshListID	= INVALID_ID;
			TRPDS( nContext, nPass ).nSortIndex		= INVALID_ID;

			// This allows us to use the viewer system to render UI layer models as if they were GENERAL
			if ( DRAW_LAYER_DEFAULT == TRPDS( nContext, nPass ).eDrawLayer )
				TRPDS( nContext, nPass ).eDrawLayer		= pViewer->eDefaultDrawLayer;
		}
	}

	//RenderPassDefinitionSortable tRPDSorts[ NUM_CONTEXTS * NUM_RENDER_PASS_TYPES ];
	V_RETURN( pViewer->tRPDSorts.Resize( nViewParams * NUM_RENDER_PASS_TYPES ) );
	RenderPassDefinitionSortable * tRPDSorts = pViewer->tRPDSorts;
	int nNextSortable = 0;



	// CML HACK
	// Problem: no way to separate particles into render contexts, at present.
	// Workaround: add sortables for the particle render passes to go last.
	{
		if ( nPassTypeIndexList[ RPTYPE_PARTICLES_ENV ] >= 0 )
		{
			int n = nNextSortable++;
			int nPass = nPassTypeIndexList[ RPTYPE_PARTICLES_ENV ];
			tRPDSorts[ n ].pRPD			= &TRPDS( 0, nPass );
			tRPDSorts[ n ].nSection		= nSections - 1;
			tRPDSorts[ n ].nPass		= nPass;
			tRPDSorts[ n ].nViewParams	= 0;
		}

		if ( nPassTypeIndexList[ RPTYPE_PARTICLES_GENERAL ] >= 0 )
		{
			int n = nNextSortable++;
			int nPass = nPassTypeIndexList[ RPTYPE_PARTICLES_GENERAL ];
			tRPDSorts[ n ].pRPD			= &TRPDS( 0, nPass );
			tRPDSorts[ n ].nSection		= nSections - 1;
			tRPDSorts[ n ].nPass		= nPass;
			tRPDSorts[ n ].nViewParams	= 0;
		}
	}


	// Add sortables for the viewparams to go first in each render context.
	for ( int nContext = 0; nContext < nViewParams; ++nContext )
	{
		TRPDS( nContext, 0 ).dwPassFlags |= PASS_FLAG_SET_VIEW_PARAMS;
	}

	// For each visible mesh, add it to the render pass definition for that pass type in that context.
	for ( safe_vector<VISIBLE_MESH>::const_iterator i = pViewer->tVisibleMeshList.begin();
		i != pViewer->tVisibleMeshList.end();
		++i )
	{
		const VISIBLE_MESH * pVM = &(*i);

		ASSERT( pVM->nSection >= 0 );

		for ( int nPass = 0; nPass < cnPassCount; nPass++ )
		{
			ASSERT_CONTINUE( pVM->nViewParams != INVALID_ID );
			RenderPassDefinition & tRPD = TRPDS( pVM->nViewParams, nPass );
			if ( tRPD.dwPassFlags & PASS_FLAG_PARTICLES )
				continue;

			RENDER_CONTEXT & tContext = tContexts[ pVM->nViewParams ];

			// Lazy set up the sortable for that pass.
			if ( tRPD.nSortIndex == INVALID_ID )
			{
				tRPD.nSortIndex = nNextSortable;
				++nNextSortable;
				tRPDSorts[ tRPD.nSortIndex ].pRPD			= &tRPD;
				tRPDSorts[ tRPD.nSortIndex ].nSection		= pVM->nSection;
				tRPDSorts[ tRPD.nSortIndex ].nPass			= nPass;
				tRPDSorts[ tRPD.nSortIndex ].nViewParams	= pVM->nViewParams;

				ViewParameters * pVP = NULL;
				V( dxC_ViewParametersGet( pVM->nViewParams, pVP ) );
				ASSERT_CONTINUE( pVP );

				// All alpha should follow all opaques.  Therefore, the passes marked as "alpha" should go in the
				// last section for this "view parameters" set (and thereby after all opaques in this view param set).
				if ( dxC_PassTypeGetAlpha( ePasses[ nPass ] ) )
					tRPDSorts[ tRPD.nSortIndex ].nSection = pVP->nLastSection;
			}

			if ( tRPD.dwPassFlags & PASS_FLAG_BLOB_SHADOWS )
			{
				// Blob shadows are rendered together via a callback.
				// No meshlist is created.
				continue;
			}

			if ( tRPD.dwPassFlags & PASS_FLAG_SKYBOX )
			{
				// Skyboxes should be rendered per "view parameters" set.  The skybox sortable was set up above,
				// so now we should just continue.
				continue;
			}

#if ISVERSION(DEVELOPMENT)
			// Isolate model, if requested.
			int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
			if ( nIsolateModel != INVALID_ID && nIsolateModel != pVM->pModel->nId )
				continue;
			// Isolate mesh, if requested.
			if ( e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO ) == pVM->pModel->nId && e_GetRenderFlag( RENDER_FLAG_ISOLATE_MESH ) )
			{
				int nIsolateMesh = dxC_DebugTrackMesh( pVM->pModel, pVM->pModelDef );
				if ( nIsolateMesh != INVALID_ID && nIsolateMesh != pVM->nMesh )
					continue;
			}
#endif // DEVELOPMENT


			// CML TODO: Move this to the "add visible mesh" step and mark a vm flag for ALPHABLEND
			// Gather late culling information.
			DWORD dwMeshFlags = pVM->pMesh->dwFlags;
			int nMaterialID = dx9_MeshGetMaterial( pVM->pModel, pVM->pMesh, pVM->eMatOverrideType );
			if ( nMaterialID != pVM->pMesh->nMaterialId )
			{
				int nShaderType = dx9_GetCurrentShaderType( tContext.pEnvDef, &tContext.tState, pVM->pModel );
				EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialID, nShaderType );
				if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
					dwMeshFlags |= MESH_FLAG_ALPHABLEND;
			}
			if ( dxC_ModelGetFlagbit( pVM->pModel, MODEL_FLAGBIT_FORCE_ALPHA ) )
				dwMeshFlags |= MESH_FLAG_ALPHABLEND;


			// Cull mesh against this pass definition.
			if ( sCullMesh( pVM, tContext, tRPD, dwMeshFlags ) )
				continue;


			// Lazy create meshlist.
			MeshList * pMeshList = NULL;
			if ( tRPD.nMeshListID == INVALID_ID )
			{
				V_CONTINUE( dxC_MeshListBeginNew( tRPD.nMeshListID, pMeshList ) );
				ASSERT_CONTINUE( tRPD.nMeshListID != INVALID_ID );
			} else
			{
				V_CONTINUE( dxC_MeshListGet( tRPD.nMeshListID, pMeshList ) );
			}
			ASSERT_CONTINUE( pMeshList );


			// Add mesh to meshlist.
			ModelDraw * pModelDraw = pVM->pModelDraw;
			ASSERT_CONTINUE( pModelDraw );
			if ( ! TESTBIT_DWORD( pModelDraw->dwFlagBits, ModelDraw::FLAGBIT_DATA_INITIALIZED ) )
			{
				// Set the initial data from the VISIBLE_MESH.
				pModelDraw->nLOD = pVM->nLOD;
				pModelDraw->fDistanceToEyeSquared = pVM->fDistFromEyeSq;
				pModelDraw->fScreensize = pVM->fScreensize;
				if ( TESTBIT( pVM->dwFlagBits, VISIBLE_MESH::FLAGBIT_HAS_SCISSOR ) )
					pModelDraw->tScissor = pVM->tScissor;
				else
					pModelDraw->tScissor = tContext.tScissor;
				SETBIT( pModelDraw->dwFlagBits, ModelDraw::FLAGBIT_DATA_INITIALIZED, TRUE );
			}
			SETBIT( pModelDraw->dwFlagBits, ModelDraw::FLAGBIT_FORCE_NO_SCISSOR, TESTBIT_DWORD( pVM->dwFlagBits, VISIBLE_MESH::FLAGBIT_FORCE_NO_SCISSOR ) );
			V_CONTINUE( dxC_ModelDrawPrepare( tContext, *pModelDraw ) );
			ASSERT_CONTINUE( pModelDraw->nLOD == pVM->nLOD );
			ASSERT_CONTINUE( pModelDraw->pModelDefinition == pVM->pModelDef );
			V_CONTINUE( dxC_ModelDrawAddMesh( *pMeshList, tContext, *pModelDraw, pVM->nMesh, pVM->eMatOverrideType, tRPD.dwPassFlags ) );
		}
	}


	// Sort the render pass definitions by section followed by pass type.
	qsort( tRPDSorts, nNextSortable, sizeof(RenderPassDefinitionSortable), RenderPassDefinitionSortable::Compare );


#ifdef ENGINE_TARGET_DX10
	BOOL bDepthMRTsCleared = FALSE;
#endif

	int nCurViewParams = -1;
	for ( int n = 0; n < nNextSortable; ++n )
	{
		ASSERT_CONTINUE( tRPDSorts[ n ].pRPD );
		RenderPassDefinition & tRPD = *tRPDSorts[ n ].pRPD;

		// Add debug text indicating pass.
		{
			CREATE_RCHDATA( DEBUG_TEXT, nID, pData );
			WCHAR wszStr[ MAX_PASS_NAME_LEN ];
			dxC_PassEnumGetString( ePasses[ tRPDSorts[n].nPass ], wszStr, MAX_PASS_NAME_LEN );
			PStrCvt( pData->szText, wszStr, pData->cnTextLen );
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::DEBUG_TEXT, nID );
		}

		//if ( tRPD.dwPassFlags & PASS_FLAG_SET_VIEW_PARAMS )
		if ( nCurViewParams != tRPDSorts[n].nViewParams )
		{
			// Set the view parameters and continue.
			RENDER_CONTEXT & tContext = tContexts[ tRPDSorts[n].nViewParams ];
			nCurViewParams = tRPDSorts[n].nViewParams;

			//		set viewport
			{
				//CREATE_RCHDATA( SET_VIEWPORT, nID, pData );
				//pData->tVP = tContext.tState.tVP;
				//dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_VIEWPORT, nID );
			}

			//		set scissor rect
			{
				CREATE_RCHDATA( SET_SCISSOR, nID, pData );
				pData->tRect = tContext.tScissor;
				//dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_SCISSOR, nID );
			}
		}

		if ( tRPD.dwPassFlags & PASS_FLAG_PARTICLES )
		{
			// This is a particles pass.

			// Particles render in the global context, currently.
			RENDER_CONTEXT & tContext = tContexts[ tRPDSorts[n].nViewParams ];


			// set full scissor rect
			{
				CREATE_RCHDATA( SET_SCISSOR, nID, pData );
				pData->tRect = tContext.tScissor;
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_SCISSOR, nID );
			}


			// Add set fog command.
			{
				CREATE_RCHDATA( SET_FOG_ENABLED, nID, pData );
				pData->bEnabled = 0 != ( tRPD.dwPassFlags & PASS_FLAG_FOG_ENABLED );
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_FOG_ENABLED, nID );
			}

			// Add draw particles command.
			{
				CREATE_RCHDATA( DRAW_PARTICLES, nID, pData );
				pData->eDrawLayer = tRPD.eDrawLayer;
				pData->mView = tContext.matView;
				pData->mProj = tContext.matProj;
				pData->mProj2 = tContext.matProj2;
				pData->vEyePos = tContext.vEyePos_w;
				pData->vEyeDir = tContext.vEyeDir_w;
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::DRAW_PARTICLES, nID );
			}
			continue;
		}

		if ( tRPD.dwPassFlags & PASS_FLAG_SKYBOX )
		{
			// This is a skybox pass.

			RENDER_CONTEXT tContext = tContexts[ tRPDSorts[n].nViewParams ];
			int nSkyboxPass = INVALID_ID;
			if ( ePasses[ tRPDSorts[n].nPass ] == RPTYPE_OPAQUE_SKYBOX )
				nSkyboxPass = SKYBOX_PASS_BACKDROP;
			else if ( ePasses[ tRPDSorts[n].nPass ] == RPTYPE_ALPHA_SKYBOX )
				nSkyboxPass = SKYBOX_PASS_AFTER_MODELS;
			else
			{
				ASSERTV_MSG( "Encountered unexpected skybox pass type! %d", ePasses[ tRPDSorts[n].nPass ] );
				continue;
			}
			ASSERT_CONTINUE( IS_VALID_INDEX( nSkyboxPass, NUM_SKYBOX_PASSES ) );

			tSkyboxData[nSkyboxPass].nSkyboxPass = nSkyboxPass;
			tSkyboxData[nSkyboxPass].pContext    = &tContexts[ tRPDSorts[n].nViewParams ];

			// Add callback command.
			{
				CREATE_RCHDATA( CALLBACK_FUNC, nID, pData );
				pData->pUserData = &tSkyboxData[nSkyboxPass];
				pData->pfnCallback = dxC_SkyboxRenderCallback;
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::CALLBACK_FUNC, nID );
			}
			continue;
		}

		if ( tRPD.dwPassFlags & PASS_FLAG_BLOB_SHADOWS )
		{
			// This is a blob shadow pass.
			RENDER_CONTEXT & tContext = tContexts[ tRPDSorts[n].nViewParams ];

			// Add callback command.
			{
				CREATE_RCHDATA( CALLBACK_FUNC, nID, pData );
				pData->pfnCallback = dxC_RenderBlobShadows;
				pData->pUserData = (void*)&tContext;
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::CALLBACK_FUNC, nID );
			}

			continue;
		}

		// This is a meshlist pass.

		if ( tRPD.nMeshListID == INVALID_ID )
			continue;
		MeshList * pMeshList = NULL;
		V_CONTINUE( dxC_MeshListGet( tRPD.nMeshListID, pMeshList ) );
		ASSERT_CONTINUE( pMeshList );
		if ( pMeshList->size() == 0 )
			continue;


		// Sort meshlist via criteria.
		if ( tRPD.eMeshSort != SORT_NONE )
		{
			std::sort( pMeshList->begin(), pMeshList->end(), gpfn_MeshListCmp[tRPD.eMeshSort] );
		}


		// Add set fog command.
		{
			CREATE_RCHDATA( SET_FOG_ENABLED, nID, pData );
			pData->bEnabled = 0 != ( tRPD.dwPassFlags & PASS_FLAG_FOG_ENABLED );
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_FOG_ENABLED, nID );
		}

#ifdef ENGINE_TARGET_DX10
		{
			// CML 2007.10.23 - Make sure the depth-pass MRTs only get cleared during the first depth-only pass
			BOOL bNeedDepthClear = FALSE;

			{
				CREATE_RCHDATA( CLEAR_TARGET, nCTID, pCTData );
				CREATE_RCHDATA( SET_TARGET, nSTID, pSTData );

				if( TEST_MASK( tRPD.dwPassFlags, PASS_FLAG_DEPTH ) )
				{
					//Always write color because we need to read it in the shader
					pSTData->eColor[ pSTData->nRTCount ] = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_DEPTH );
					pCTData->tColor[ pSTData->nRTCount ][0] = e_GetFarClipPlane();
					bNeedDepthClear = ! bDepthMRTsCleared;
				}
				else
					pSTData->eColor[ 0 ] = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_BACKBUFFER );;

				pSTData->nRTCount++;
				
				if ( TEST_MASK( tRPD.dwPassFlags, PASS_FLAG_DEPTH ) && e_OptionState_GetActive()->get_bDX10ScreenFX() )
				{
					pSTData->eColor[ pSTData->nRTCount ] = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_MOTION );;
					pCTData->tColor[ pSTData->nRTCount ] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f );
					pSTData->nRTCount++;
				}

				pSTData->eDepth = dxC_GetSwapChainDepthTargetIndex( pViewer->nSwapChainIndex, SWAP_DT_AUTO );;
				dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_TARGET, nSTID );

				if ( bNeedDepthClear )
				{
					
					pCTData->fDepth = 1.f;
					pCTData->nStencil = 0;
					pCTData->nRTCount = pSTData->nRTCount;
					dxC_CommandAdd( pViewer->tCommands, RenderCommand::CLEAR_TARGET, nCTID );

					bDepthMRTsCleared = TRUE;
				}
			}

		}

#endif

		{
			CREATE_RCHDATA( SET_COLORWRITE, nID, pData );
			pData->nRT = 0;
#ifdef ENGINE_TARGET_DX10 //Always write color because we need to read it in the shader
			pData->dwValue =  D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
#else //Depth passes in dx9 shouldn't write color all others should
			pData->dwValue =  TEST_MASK( tRPD.dwPassFlags, PASS_FLAG_DEPTH ) ? 0 : D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
#endif
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_COLORWRITE, nID );
		}
		//if we're doing a Depth pass only it should write depth
		if( e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) )
		{
			CREATE_RCHDATA( SET_DEPTHWRITE, nID, pData );
			pData->bDepthWriteEnable = TEST_MASK( tRPD.dwPassFlags, PASS_FLAG_DEPTH ) ? TRUE : FALSE;
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_DEPTHWRITE, nID );
		}

#ifdef DX10_USE_MRTS
		{
			CREATE_RCHDATA( SET_COLORWRITE, nID, pData );
			pData->nRT = 1;

			// Only the depth passes writes motion to RENDER_TARGET_MOTION
			if ( TEST_MASK( tRPD.dwPassFlags, PASS_FLAG_DEPTH ) && e_OptionState_GetActive()->get_bDX10ScreenFX() )
			{
				// This is a fix for ATI's partial-write driver bug
#ifdef DX10_ATI_MSAA_FIX
				pData->dwValue = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN 
					| D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
#else
				pData->dwValue = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN; //If depth pass don't write velocity
#endif
			}
			else
				pData->dwValue = 0;

			dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_COLORWRITE, nID );
		}
#endif

		// Add draw meshlist command.
		{
			CREATE_RCHDATA( DRAW_MESH_LIST, nID, pData );
			pData->nMeshListID = tRPD.nMeshListID;
			dxC_CommandAdd( pViewer->tCommands, RenderCommand::DRAW_MESH_LIST, nID );
		}

#ifdef ENGINE_TARGET_DX10
		// Add particles to a separate color depth render target
		if( e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) )
		{
			if ( nPassTypeIndexList[ RPTYPE_DEPTH_SCENE ] == tRPDSorts[ n ].nPass )
			{
				// Save off the current color depth target
				ONCE
				{
					CREATE_RCHDATA( COPY_BUFFER, nID, pData );
					pData->eSrc = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_DEPTH );
					pData->eDest = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_DEPTH_WO_PARTICLES );
					ASSERT_BREAK( pData->eSrc != RENDER_TARGET_INVALID );
					ASSERT_BREAK( pData->eDest != RENDER_TARGET_INVALID );
					dxC_CommandAdd( pViewer->tCommands, RenderCommand::COPY_BUFFER, nID );
				}

				// Set the render target to the color depth target
				{
					CREATE_RCHDATA( SET_TARGET, nSTID, pSTData );
					pSTData->nLevel = 0;
					pSTData->eColor[ pSTData->nRTCount ] = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_DEPTH );
					pSTData->nRTCount++;
					pSTData->eDepth = dxC_GetSwapChainDepthTargetIndex( pViewer->nSwapChainIndex, SWAP_DT_AUTO );
					dxC_CommandAdd( pViewer->tCommands, RenderCommand::SET_TARGET, nSTID );

#if 0
					CREATE_RCHDATA( CLEAR_TARGET, nCTID, pCTData );
					pCTData->dwFlags = D3DCLEAR_TARGET;
					pCTData->tColor[ 0 ] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f );
					pCTData->nRTCount = 1;
					dxC_CommandAdd( pViewer->tCommands, RenderCommand::CLEAR_TARGET, nCTID );
#endif
				}

				// Add draw particles command.
				{
					// Particles render in the global context, currently.
					RENDER_CONTEXT & tContext = tContexts[ tRPDSorts[n].nViewParams ];

					CREATE_RCHDATA( DRAW_PARTICLES, nID, pData );
					pData->eDrawLayer = DRAW_LAYER_GENERAL; //tRPD.eDrawLayer;
					pData->mView = tContext.matView;
					pData->mProj = tContext.matProj;
					pData->mProj2 = tContext.matProj2;
					pData->vEyePos = tContext.vEyePos_w;
					pData->vEyeDir = tContext.vEyeDir_w;
					pData->bDepthOnly = TRUE;
					dxC_CommandAdd( pViewer->tCommands, RenderCommand::DRAW_PARTICLES, nID );
				}
			}
		}
#endif
	}


#if ISVERSION(DEVELOPMENT)
	dxC_ViewerSaveStats( pViewer );
#endif // DEVELOPMENT


	//		render meshlist(s)
	//			- commit command buffer
	V_RETURN( pViewer->tCommands.Commit() );

	return S_OK;
}



}; // namespace FSSE


