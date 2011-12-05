//----------------------------------------------------------------------------
// dx9_2d.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_primitive.h"
#include "dxC_effect.h"
#include "dxC_material.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_particle.h"
#include "dxC_main.h"
#include "dxC_caps.h"
#include "e_particles_priv.h"
#include "e_settings.h"
#include "e_optionstate.h"

#include "dxC_2d.h"
#include "dxC_buffer.h"
#include "dxC_texture.h"
#include "..\\dx9\\dx9_shaderversionconstants.h"
#include "filepaths.h"
#include "camera_priv.h"
#include "c_camera.h"
#include "dxC_pixo.h"

#ifdef ENGINE_TARGET_DX10
#include "dx10_effect.h"
struct 
{
	D3DXVECTOR3 paramStart;
	D3DXVECTOR3 paramEnd;
	float fFrame;
	float fFrameCount;
} DOF_PARAMS ;

#define DOF_START_PARAMS D3DXVECTOR3( 0.0f, 7.0f, 50.0f )
#endif


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

D3D_SCREEN_LAYER gtScreenLayers[ LAYER_MAX ];
D3D_VERTEX_BUFFER_DEFINITION gtScreenCoverVertexBufferDef; // for a polygon covering the entire screen

static const SWAP_CHAIN_RENDER_TARGET sgeTargets[ NUM_SCREEN_EFFECT_TARGETS ] =
{
	SWAP_RT_INVALID,
	SWAP_RT_FINAL_BACKBUFFER,
	SWAP_RT_FULL,
};

enum SCREENFX_PASS_ARGS
{
	SFX_PASS_ARG_OP = 0,
	SFX_PASS_ARG_FROM_RT = 1,
	SFX_PASS_ARG_TO_RT = 2,
};

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sSetScreenCoverVertex( SCREEN_COVER_VERTEX * pVertex, float fX, float fY, float fU, float fV )
{
	pVertex->vPosition.fX = fX;
	pVertex->vPosition.fY = fY;
	pVertex->vPosition.fZ = 0.0f;
//	pVertex->fRHW = 1.0f;
	pVertex->pfTexCoords[ 0 ] = fU;
	pVertex->pfTexCoords[ 1 ] = fV;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX10
void dx10_StepDOF()
{
	float fLerp = 0.0f;

	if( DOF_PARAMS.fFrame <= -1.0f )
		return;

	if( DOF_PARAMS.fFrame > 0.0f )
	{
		ASSERT_RETURN( DOF_PARAMS.fFrameCount > 0.0f );
		fLerp = DOF_PARAMS.fFrame  / DOF_PARAMS.fFrameCount;
	}
	
	float vec[4] = { 
		DOF_PARAMS.paramStart.x * fLerp + DOF_PARAMS.paramEnd.x * ( 1.0f - fLerp ),
		DOF_PARAMS.paramStart.y * fLerp + DOF_PARAMS.paramEnd.y * ( 1.0f - fLerp ),
		DOF_PARAMS.paramStart.z * fLerp + DOF_PARAMS.paramEnd.z * ( 1.0f - fLerp ),
		0.0f };

	dx10_PoolSetFloatVector( EFFECT_PARAM_DOF_PARAMS, vec );
	DOF_PARAMS.fFrame -= 1.0f;
}
#endif

PRESULT e_DOFSetParams(float fNear, float fFocus, float fFar, float fFrames )
{
#ifdef ENGINE_TARGET_DX10
	DOF_PARAMS.fFrame = DOF_PARAMS.fFrameCount = fFrames;
	DOF_PARAMS.paramStart = DOF_PARAMS.paramEnd;
	DOF_PARAMS.paramEnd = D3DXVECTOR3( fNear, fFocus, fFar );;
#endif

	return S_OK;
}

void dx9_AddScreenLayer( int nID, int nEffect, int nOption )
{
	gtScreenLayers[ nID ].nEffect = nEffect;
	gtScreenLayers[ nID ].nOption = nOption;
	gtScreenLayers[ nID ].tTechniqueCache.Reset();
}



PRESULT e_LoadNonStateScreenEffects( BOOL bSetActive /*= FALSE*/ )
{
	// For now Mythos doesn't do any screen effects
	if( !AppIsTugboat() && ! c_GetToolMode() )
	{
		for ( UINT i = 0; i < NUM_NONSTATE_SCREEN_EFFECTS; i++ )
		{
			NONSTATE_SCREEN_EFFECT_ENTRY * pEntry = NULL;
			int nDefID = INVALID_ID;
			V_CONTINUE( e_ScreenFXGetNonStateEffect( i, &nDefID, &pEntry ) );
			ASSERT_CONTINUE( nDefID != INVALID_ID );
			ASSERT_CONTINUE( pEntry );

#ifdef DX10_DOF
			if ( bSetActive )
			{
				V( e_ScreenEffectSetActive( nDefID, pEntry->bSetActive ) );
			}
#endif
		}
	}

	return S_OK;
}

PRESULT dx9_ScreenDrawRestore()
{
	ASSERT_RETFAIL( ! dxC_VertexBufferD3DExists( gtScreenCoverVertexBufferDef.nD3DBufferID[ 0 ] ) );

	gtScreenCoverVertexBufferDef.dwFVF = DX9_BLOCK( SCREEN_COVER_FVF ) DX10_BLOCK( D3DC_FVF_NULL );
	gtScreenCoverVertexBufferDef.nVertexCount = NUM_SCREEN_COVER_VERTICES;
	int nScreenCoverSize = NUM_SCREEN_COVER_VERTICES * sizeof( SCREEN_COVER_VERTEX );
	gtScreenCoverVertexBufferDef.nBufferSize[ 0 ] = nScreenCoverSize;
	gtScreenCoverVertexBufferDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	gtScreenCoverVertexBufferDef.eVertexType = VERTEX_DECL_XYZ_UV;
	V_DO_SUCCEEDED( dxC_CreateVertexBuffer( 0, gtScreenCoverVertexBufferDef ) )
	{
		SCREEN_COVER_VERTEX pScreenVertices[ NUM_SCREEN_COVER_VERTICES ];
		sSetScreenCoverVertex( &pScreenVertices[ 0 ], -1.0f, -1.0f, 0.0f, 1.0f );
		sSetScreenCoverVertex( &pScreenVertices[ 1 ], -1.0f,  1.0f, 0.0f, 0.0f );
		sSetScreenCoverVertex( &pScreenVertices[ 2 ],  1.0f, -1.0f, 1.0f, 1.0f );
		sSetScreenCoverVertex( &pScreenVertices[ 3 ], -1.0f,  1.0f, 0.0f, 0.0f );
		sSetScreenCoverVertex( &pScreenVertices[ 4 ],  1.0f,  1.0f, 1.0f, 0.0f );
		sSetScreenCoverVertex( &pScreenVertices[ 5 ],  1.0f, -1.0f, 1.0f, 1.0f );

		size_t nSize = sizeof(SCREEN_COVER_VERTEX) * NUM_SCREEN_COVER_VERTICES;
		gtScreenCoverVertexBufferDef.pLocalData[0] = MALLOC( NULL, nSize );
		MemoryCopy( gtScreenCoverVertexBufferDef.pLocalData[0], nSize, pScreenVertices, nSize );

		V_RETURN( dxC_UpdateBuffer( 0, gtScreenCoverVertexBufferDef, pScreenVertices, 0, nScreenCoverSize ) );
	}
	
	// create screen layer descriptions - no actual D3D allocations
	dx9_AddScreenLayer( LAYER_BLUR_FILTER,	NONMATERIAL_EFFECT_BLUR,		EFFECT_BLUR_PASS_FILTER );
	dx9_AddScreenLayer( LAYER_BLUR_X,		NONMATERIAL_EFFECT_BLUR,		EFFECT_BLUR_PASS_X );
	dx9_AddScreenLayer( LAYER_BLUR_Y,		NONMATERIAL_EFFECT_BLUR,		EFFECT_BLUR_PASS_Y );
	dx9_AddScreenLayer( LAYER_COMBINE,		NONMATERIAL_EFFECT_COMBINE,		0 );
	dx9_AddScreenLayer( LAYER_OVERLAY,		NONMATERIAL_EFFECT_OVERLAY,		0 );
	dx9_AddScreenLayer( LAYER_FADE,			NONMATERIAL_EFFECT_OVERLAY,		1 );
	dx9_AddScreenLayer( LAYER_DOWNSAMPLE,	NONMATERIAL_EFFECT_GAUSSIAN,	4 );
	dx9_AddScreenLayer( LAYER_GAUSS_S_X,	NONMATERIAL_EFFECT_GAUSSIAN,	0 );
	dx9_AddScreenLayer( LAYER_GAUSS_S_Y,	NONMATERIAL_EFFECT_GAUSSIAN,	1 );
	dx9_AddScreenLayer( LAYER_GAUSS_L_X,	NONMATERIAL_EFFECT_GAUSSIAN,	2 );
	dx9_AddScreenLayer( LAYER_GAUSS_L_Y,	NONMATERIAL_EFFECT_GAUSSIAN,	3 );

	V_RETURN( gtScreenEffects.Init() );

	V( e_LoadNonStateScreenEffects( TRUE ) );

	V_RETURN( dxC_VertexBufferPreload( gtScreenCoverVertexBufferDef.nD3DBufferID[ 0 ] ) );
	return S_OK;
}

PRESULT dx9_ScreenDrawRelease()
{
	dxC_VertexBufferReleaseResource( gtScreenCoverVertexBufferDef.nD3DBufferID[ 0 ] );
	gtScreenCoverVertexBufferDef.nD3DBufferID[ 0 ] = INVALID_ID;
	for ( int i=0; i < DXC_MAX_VERTEX_STREAMS; ++i )
	{
		if ( ! gtScreenCoverVertexBufferDef.pLocalData[i] )
			continue;

		FREE(NULL, gtScreenCoverVertexBufferDef.pLocalData[i] );
		gtScreenCoverVertexBufferDef.pLocalData[i] = NULL;
	}
	V_RETURN( gtScreenEffects.Destroy() );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ScreenBuffersInit()
{
#ifdef DX10_DOF
	ZeroMemory( &DOF_PARAMS, sizeof( DOF_PARAMS ) );
	DOF_PARAMS.paramEnd = DOF_START_PARAMS;
	DOF_PARAMS.fFrameCount = 1.0f;
//KMNV: I think we're ok to do this here
#endif
	dxC_VertexBufferDefinitionInit( gtScreenCoverVertexBufferDef );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ScreenBuffersDestroy()
{
	dxC_VertexBufferDestroy( gtScreenCoverVertexBufferDef.nD3DBufferID[ 0 ] );
	return S_OK;
}

//----------------------------------------------------------------------------
// SCREEN_EFFECTS
//----------------------------------------------------------------------------

PRESULT CScreenEffects::Init()
{
	ArrayInit(m_tInstances, NULL, 2);

	m_nSourceIdentifier = RENDER_TARGET_INVALID;
	m_bNeedSort = FALSE;

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::Destroy()
{
	m_tInstances.Destroy();

	return S_OK;
}

//----------------------------------------------------------------------------

int CScreenEffects::GetIndexByDefID( int nDefID )
{
	ASSERT_RETINVALID( nDefID != INVALID_ID );

	int nCount = m_tInstances.Count();

	for ( int i = 0; i < nCount; i++ )
	{
		if ( m_tInstances[ i ].nScreenEffectDef == nDefID )
			return i;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------

int CScreenEffects::sCompareInstances( const void * pE1, const void * pE2 )
{
	const INSTANCE * pI1 = static_cast<const INSTANCE*>(pE1);
	const INSTANCE * pI2 = static_cast<const INSTANCE*>(pE2);

	if ( pI1->nPriority < pI2->nPriority )
		return -1;
	if ( pI1->nPriority > pI2->nPriority )
		return 1;
	if ( pI1->bExclusive > pI2->bExclusive )
		return -1;
	if ( pI1->bExclusive < pI2->bExclusive )
		return 1;
	return 0;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::RestoreEffect( int nScreenEffectDef )
{
	if ( nScreenEffectDef == INVALID_ID )
		return S_FALSE;

	SCREEN_EFFECT_DEFINITION * pDef = (SCREEN_EFFECT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_SCREENEFFECT, nScreenEffectDef );
	if ( ! pDef )
		return S_FALSE;

	int nIndex = GetIndexByDefID( nScreenEffectDef );
	ASSERT_RETFAIL( nIndex >= 0 );

	INSTANCE & tInstance = m_tInstances[ nIndex ];

	tInstance.nPriority			= pDef->nPriority;
	tInstance.bExclusive		= !!( 0 != ( pDef->dwDefFlags & SCREEN_EFFECT_DEF_FLAG_EXCLUSIVE ) );
	tInstance.fTransitionPhase	= 0.f;

	SetNeedSort();

	V_RETURN( e_ScreenFXRestore( pDef ) );
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::RemoveInstance( int nIndex )
{
	ASSERTX( m_tInstances[ nIndex ].eState == INSTANCE::DEAD, "Trying to remove a non-dead screen effect instance!" );
	ASSERTX( m_tInstances[ nIndex ].tRefCount.GetCount() == 0, "Trying to remove a non-zero refcount screen effect instance!" );

	BOOL bFound = ArrayRemoveByIndex(m_tInstances, nIndex);
	ASSERT_RETFAIL( bFound );
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::BeginTransitionIn( CScreenEffects::INSTANCE * pInstance, SCREEN_EFFECT_DEFINITION * pDef )
{
	ASSERT_RETINVALIDARG( pInstance );
	ASSERT_RETINVALIDARG( pDef );

	ASSERTV( pInstance->tRefCount.GetCount() >= 1, "BeginTransitionIn Def %d, Refcount %d, State %d", pInstance->nScreenEffectDef, pInstance->tRefCount.GetCount(), pInstance->eState );

	if ( pInstance->fTransitionPhase < 0.f )
		pInstance->fTransitionPhase	= 0.f;
	if ( pDef->fTransitionIn <= 0.f )
	{
		pInstance->fTransitionPhase	= 1.f;
		pInstance->fTransitionRate  = 1.f;
		pInstance->eState = INSTANCE::ACTIVE;
	}
	else
	{
		pInstance->fTransitionRate = 1.f / pDef->fTransitionIn;
		pInstance->eState = INSTANCE::TRANSITION_IN;
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::SetActive( int nScreenEffectDef, BOOL bActive )
{
	if ( nScreenEffectDef == INVALID_ID )
		return S_FALSE;

	SCREEN_EFFECT_DEFINITION * pDef = (SCREEN_EFFECT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_SCREENEFFECT, nScreenEffectDef );

	int nIndex = GetIndexByDefID( nScreenEffectDef );

	if ( nIndex == INVALID_ID && bActive )
	{
		// add it
		INSTANCE tInstance;
		ZeroMemory( &tInstance, sizeof(INSTANCE) );

		tInstance.nScreenEffectDef	= nScreenEffectDef;
		tInstance.nPriority			= SCREENFX_PRIORITY_NOTLOADED;
		tInstance.eState			= INSTANCE::INACTIVE;
		RefCountInit( tInstance.tRefCount );
		nIndex = ArrayAddItem(m_tInstances, tInstance);

		if ( pDef )
		{
			V_RETURN( RestoreEffect( nScreenEffectDef ) );
		}
	}

	if ( nIndex == INVALID_ID )
		return S_FALSE;

	if ( bActive )
	{
		// turning on an effect
		INSTANCE * pInstance = &m_tInstances[ nIndex ];
		pInstance->tRefCount.AddRef();

		ASSERTV( pInstance->tRefCount.GetCount() >= 1, "Post-Addref Refcount: %d, def %d", pInstance->tRefCount.GetCount(), pInstance->nScreenEffectDef );

		// Don't transition if the effect is already active.
		if ( pInstance->tRefCount.GetCount() == 1 )
		{
			ASSERTV( pInstance->eState == INSTANCE::TRANSITION_OUT || pInstance->eState == INSTANCE::DEAD || pInstance->eState == INSTANCE::INACTIVE, "State: %d, def %d", pInstance->eState, pInstance->nScreenEffectDef );

			if ( pDef )
			{
				//trace( "SCREENFX: Activating index(%d), def [%d:%s]\n", nIndex, pDef->tHeader.nId, pDef->tHeader.pszName );

				V( BeginTransitionIn( pInstance, pDef ) );
			}
			else
			{
				pInstance->fTransitionPhase = 0.f;
				pInstance->fTransitionRate  = 1.f;
				pInstance->eState			= INSTANCE::INACTIVE;
			}
		}

		ASSERTV( pInstance->eState == INSTANCE::ACTIVE || pInstance->eState == INSTANCE::TRANSITION_IN || pInstance->nPriority == SCREENFX_PRIORITY_NOTLOADED, "State: %d, Pri: %d, def %d", pInstance->eState, pInstance->nPriority, pInstance->nScreenEffectDef );
	}
	else
	{
		INSTANCE * pInstance = &m_tInstances[ nIndex ];

		ASSERTV( pInstance->tRefCount.GetCount() >= 1, "Pre-release Refcount: %d, def %d", pInstance->tRefCount.GetCount(), pInstance->nScreenEffectDef );

		pInstance->tRefCount.Release();

		// Only turn off an effect if there are no more references to it.
		if ( pInstance->tRefCount.GetCount() == 0 )
		{
			if ( pDef )
			{
				//trace( "SCREENFX: Deactivating index(%d), def [%d:%s]\n", nIndex, pDef->tHeader.nId, pDef->tHeader.pszName );

				//pInstance->nPriority			= SCREENFX_PRIORITY_DYING;
				if ( pInstance->fTransitionPhase > 1.f )
					pInstance->fTransitionPhase	= 1.f;
				if ( pDef->fTransitionOut <= 0.f )
				{
					pInstance->fTransitionPhase	= 0.f;
					pInstance->fTransitionRate	= -1.f;
					pInstance->eState			= INSTANCE::DEAD;
				}
				else
				{
					pInstance->fTransitionRate	= -1.f / pDef->fTransitionOut;
					pInstance->eState			= INSTANCE::TRANSITION_OUT;
				}
			}
			else
			{
				pInstance->fTransitionPhase = 0.f;
				pInstance->fTransitionRate  = -1.f;
				pInstance->eState			= INSTANCE::DEAD;
			}
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::Update()
{
	// doing things like updating transition timers and invalidating the source texture

	m_nSourceIdentifier = RENDER_TARGET_INVALID;

	float fTimeDelta = e_GetElapsedTime();

	// handle async loading screen effects
	// and update transition states

	int nMax = m_tInstances.Count();
	for ( int i = 0; i < nMax; i++ )
	{
		INSTANCE * pInstance = &m_tInstances[ i ];

		ASSERTV( ! ( pInstance->eState == INSTANCE::ACTIVE && pInstance->tRefCount.GetCount() == 0 ), "Def: %d", pInstance->nScreenEffectDef );

		if ( pInstance->nPriority == SCREENFX_PRIORITY_NOTLOADED )
		{
			V( RestoreEffect( pInstance->nScreenEffectDef ) );

			SCREEN_EFFECT_DEFINITION * pDef = (SCREEN_EFFECT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_SCREENEFFECT, pInstance->nScreenEffectDef );
			if ( pDef )
			{
				V( BeginTransitionIn( pInstance, pDef ) );
			}
		}
		else
		{
			if ( pInstance->eState == INSTANCE::TRANSITION_IN && pInstance->fTransitionPhase >= 1.f )
			{
				pInstance->fTransitionRate = 0.f;
				pInstance->eState = INSTANCE::ACTIVE;
			} 

			if ( pInstance->eState == INSTANCE::TRANSITION_OUT && pInstance->fTransitionPhase <= 0.f )
			{
				pInstance->fTransitionRate = 0.f;
				pInstance->eState = INSTANCE::DEAD;
			} 

			if ( pInstance->eState == INSTANCE::DEAD )
			{
				// dead, so remove this instance
				V( RemoveInstance( i ) );
				nMax--;
				i--;
				continue;
			}

			if ( pInstance->eState == INSTANCE::TRANSITION_IN || pInstance->eState == INSTANCE::TRANSITION_OUT )
			{
				pInstance->fTransitionPhase += pInstance->fTransitionRate * fTimeDelta;
			}
		}
	}

	if ( m_bNeedSort )
	{
		m_tInstances.QSort( CScreenEffects::sCompareInstances );
		m_bNeedSort = TRUE;
	}

#if ISVERSION(DEVELOPMENT)
	// Audit and make sure we only have 1 instance per def
	nMax = m_tInstances.Count();
	for ( int i = 0; i < nMax; i++ )
	{
		for ( int n = i+1; n < nMax; n++ )
		{
			ASSERTV( m_tInstances[ i ].nScreenEffectDef != m_tInstances[ n ].nScreenEffectDef, "SCREENFX AUDIT FAILED: Found more than one instance of Def %d (at index %d, first %d)", m_tInstances[ i ].nScreenEffectDef, n, i );
		}
	}
#endif // DEVELOPMENT

	return S_OK;
}

//----------------------------------------------------------------------------
UINT CScreenEffects::nFullScreenPasses()
{
	return m_tInstances.Count();
}
//----------------------------------------------------------------------------

PRESULT CScreenEffects::Render( CScreenEffects::TYPE eType, CScreenEffects::LAYER eLayer )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SCREENEFFECTS_ENABLED ) )
		return S_FALSE;

	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eType, NUM_TYPES ) );

	// render the appropriate stack top effect(s) for each

	// this could be in part defined based on perf settings
	int nMax = 0;
	switch ( eType )
	{
	case TYPE_FULLSCREEN:	
		nMax = m_tInstances.Count();
		//nMax = MIN( 1, nMax );
		break;
	case TYPE_PARTICLE:
		nMax = e_ParticlesGetNeedScreenTexture() ? 1 : 0;
		break;
	}

	for ( int i = 0; i < nMax; i++ )
	{
		// when this becomes FALSE, stop iterating
		BOOL bContinue;
		V( ApplyEffect( eType, eLayer, i, bContinue ) );
		if ( ! bContinue )
			break;
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ScreenEffectSetActive( int nScreenEffect, BOOL bActive )
{
	if ( nScreenEffect == INVALID_ID )
		return S_FALSE;

	V_RETURN( gtScreenEffects.SetActive( nScreenEffect, bActive ) );
	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sScreenEffectSetEffectParams(
	float fTransition,
	SCREEN_EFFECT_DEFINITION * pScreenEffect,
	D3D_EFFECT * pEffect,
	EFFECT_TECHNIQUE & tTechnique )
{
	// transition
	float fPhase = SATURATE( fTransition );
	fPhase = EASE_INOUT_HQ( fPhase );
	D3DXVECTOR4 vTransition( fPhase, fPhase, fPhase, fPhase );
	V_RETURN( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TRANSITION, &vTransition ) );

	// floats
	D3DXVECTOR4 vFloats( pScreenEffect->fFloats );
	V_RETURN( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_FLOATS, &vFloats ) );

	// colors
	D3DXVECTOR3 tColor;
	tColor = *(D3DXVECTOR3*)& pScreenEffect->tColor0.GetValue( 0.0f );
	D3DXVECTOR4 vColor0( tColor, 0.f );
	V_RETURN( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_COLOR0, &vColor0 ) );
	tColor = *(D3DXVECTOR3*)& pScreenEffect->tColor1.GetValue( 0.0f );
	D3DXVECTOR4 vColor1( tColor, 0.f );
	V_RETURN( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_COLOR1, &vColor1 ) );

	// textures
	D3D_TEXTURE * pTexture;
	pTexture = dxC_TextureGet( pScreenEffect->nTextureIDs[ 0 ] );
	if ( pTexture )
	{
		V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TEXTURE0, pTexture->GetShaderResourceView() ) );
	}
	pTexture = dxC_TextureGet( pScreenEffect->nTextureIDs[ 1 ] );
	if ( pTexture )
	{
		V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TEXTURE1, pTexture->GetShaderResourceView() ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

// CHB 2007.10.02
PRESULT dx9_ScreenDrawCover(D3D_EFFECT * pEffect)
{
	V_RETURN( dxC_SetStreamSource( gtScreenCoverVertexBufferDef, 0, pEffect ) );
	V_RETURN( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, 0, DP_GETPRIMCOUNT_TRILIST( NUM_SCREEN_COVER_VERTICES ), pEffect, METRICS_GROUP_EFFECTS ) );
	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sScreenEffectRenderPass(
	SCREEN_EFFECT_DEFINITION * pScreenEffect,
	D3D_EFFECT * pEffect,
	EFFECT_TECHNIQUE & tTechnique,
	UINT nPass,
	LPD3DCBASESHADERRESOURCEVIEW pSourceTexture,
	SCREEN_EFFECT_TARGET eDest,
	float fTransition )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eDest, NUM_SCREEN_EFFECT_TARGETS ) );
	ASSERT_RETINVALIDARG( eDest != SFX_RT_NONE );

	V_RETURN( dxC_EffectBeginPass( pEffect, nPass ) );

	// AE 2007.08.18: Why do we need a depth target for screen effects?
	V_RETURN( dxC_SetRenderTargetWithDepthStencil( sgeTargets[ eDest ], DEPTH_TARGET_NONE /*dxC_GetCurrentDepthTarget()*/ ) );

	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER );
	V( dxC_EffectSetScreenSize( pEffect, tTechnique, nRT ) );
	V( dxC_EffectSetViewport( pEffect, tTechnique, nRT ) );
	V( dxC_EffectSetPixelAccurateOffset( pEffect, tTechnique ) );
	V( sScreenEffectSetEffectParams( fTransition, pScreenEffect, pEffect, tTechnique ) );
	V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TEXTURERT, pSourceTexture ) );

#ifdef DX10_DOF  //KMNV TODO: Don't need to do this for every effect
	RENDER_TARGET_INDEX nDepthRT  = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_DEPTH );
	RENDER_TARGET_INDEX nMotionRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_MOTION );

	dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, nDepthRT != RENDER_TARGET_INVALID ? dxC_RTShaderResourceGet( nDepthRT ) : NULL );
	dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_TEXTURE_MOTION, nMotionRT != RENDER_TARGET_INVALID ? dxC_RTShaderResourceGet( nMotionRT ) : NULL );
	dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_SMOKE_ZFAR,  e_GetFarClipPlane() );
	//dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_SMOKE_ZFAR,  e_GetFarClipPlane() );

	const CAMERA_INFO* pCameraInfo = CameraGetInfo();
	static VECTOR svLastCamPosition = CameraInfoGetPosition( pCameraInfo );
	VECTOR vCamPosition = CameraInfoGetPosition( pCameraInfo );
	VECTOR vCamPosView;

	MATRIX matView;
	e_InitMatrices( NULL, &matView, NULL, NULL, NULL, NULL, NULL, TRUE );
	MatrixMultiply( &vCamPosView, &vCamPosition, &matView );
	VECTOR vCamDiff = vCamPosition - svLastCamPosition;
	MatrixMultiply( &svLastCamPosition, &svLastCamPosition, &matView );
	VECTOR vCamVelocity = vCamPosView - svLastCamPosition;
	float fTimeDelta = e_GetElapsedTime();
	vCamVelocity /= fTimeDelta;

	VECTOR vCamLookAt = CameraInfoGetLookAt( pCameraInfo);
	VECTOR vRadius = vCamLookAt - vCamPosition;
	float fRadius = VectorLength( vRadius );

	// Convert to angular velocity.
	static const float scfMaxAngularVelocity = 10.f;
	vCamVelocity.x /= fRadius * scfMaxAngularVelocity; 
	vCamVelocity.y /= fRadius * scfMaxAngularVelocity;

	// Normalize z velocity.
	static const float scfMinVelocity = 5.f;
	static const float scfMaxVelocity = 14.f;
	vCamVelocity.z = MIN( MAX( vCamVelocity.z - scfMinVelocity, 0.f ) / ( scfMaxVelocity - scfMinVelocity ), 1.f );

	// Store off the camera position for the next update.
	svLastCamPosition = vCamPosition;

#if 0
	const int cnSize = 1024;
	WCHAR szInfo[ cnSize ] = L"";

	PStrPrintf( szInfo, cnSize, 
		L"Cam Velocity: ( %f %f %f )\n"
		L"Cam delta: ( %f %f %f )\n"
		L"Elapsed time: %f",
		vCamVelocity.x,	vCamVelocity.y,	vCamVelocity.z,
		vCamDiff.x,	vCamDiff.y,	vCamDiff.z,
		fTimeDelta);
	VECTOR vPos = VECTOR( 0.7f, 1.f, 0.f );
	V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_RIGHT, TRUE ) );
#endif
	
	VECTOR4 vEffectVector( vCamVelocity, fTimeDelta );
	dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_CAM_VELOCITY, &vEffectVector );
#endif 

	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );

	V( dx9_SetDiffuse2SamplerStates( D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP ) );
#if 1
	V_RETURN( dx9_ScreenDrawCover( pEffect ) );
#else
	V_RETURN( dxC_SetStreamSource( gtScreenCoverVertexBufferDef, 0, pEffect ) );
	V_RETURN( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, 0, DP_GETPRIMCOUNT_TRILIST( NUM_SCREEN_COVER_VERTICES ), pEffect, METRICS_GROUP_EFFECTS ) );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sScreenEffectDrawParticles(
	LPD3DCBASESHADERRESOURCEVIEW pSourceTexture,
	SCREEN_EFFECT_TARGET eDest )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eDest, NUM_SCREEN_EFFECT_TARGETS ) );
	ASSERT_RETINVALIDARG( eDest != SFX_RT_NONE );


	V_RETURN( dxC_SetRenderTargetWithDepthStencil( sgeTargets[ eDest ], dxC_GetCurrentDepthTarget() ) );
	dxC_ParticlesSetScreenTexture( pSourceTexture );

	MATRIX matWorld, matView, matProj, matProj2;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	VECTOR vEyeLookat;

	e_InitMatrices( &matWorld, &matView, &matProj, &matProj2, &vEyeVector, &vEyeLocation, &vEyeLookat, TRUE );

	V_RETURN( e_ParticlesDraw( DRAW_LAYER_SCREEN_EFFECT, &matView, &matProj, &matProj2, vEyeLocation, vEyeVector, TRUE, FALSE ) );
	return S_OK;
}

//----------------------------------------------------------------------------
PRESULT CScreenEffects::ApplyEffect( CScreenEffects::TYPE eType, CScreenEffects::LAYER eLayer, int nIndex, BOOL& bContinue )
{
	bContinue = TRUE;

	ASSERT_RETFAIL( IS_VALID_INDEX( eType, NUM_TYPES ) );

	// turn off fog
	V( e_SetFogEnabled( FALSE ) );

	if ( eType == TYPE_PARTICLE )
	{
		ASSERT( e_ParticlesGetNeedScreenTexture() );
		// assuming particles will want to use the backbuffer as a source
		V_RETURN( PrepareSourceTexture( SFX_RT_BACKBUFFER, SFX_RT_FULL ) );
		RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), sgeTargets[ SFX_RT_FULL ] );
		SPD3DCBASESHADERRESOURCEVIEW pSrc = dxC_RTShaderResourceGet( nRT );
		V_RETURN( sScreenEffectDrawParticles( pSrc, SFX_RT_BACKBUFFER ) );
		return S_OK;
	}

	if ( nIndex == INVALID_ID )
		return S_FALSE;

	INSTANCE * pInstance = &m_tInstances[ nIndex ];
	if ( pInstance->nPriority < 0 )
		return S_OK;

	SCREEN_EFFECT_DEFINITION * pScreenEffect = (SCREEN_EFFECT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_SCREENEFFECT, pInstance->nScreenEffectDef );
	if ( ! pScreenEffect )
		return S_FALSE;

	if ( pScreenEffect->nLayer != (int)eLayer )
		return S_FALSE;

	if ( TEST_MASK( pScreenEffect->dwDefFlags, SCREEN_EFFECT_DEF_FLAG_DX10_ONLY ) && ! e_OptionState_GetActive()->get_bDX10ScreenFX() )
		return S_FALSE;

	ASSERT_RETFAIL( eType == TYPE_FULLSCREEN );

	// two-pass effects
	// pass 1: process : backbuffer as source texture, rt_full as render target (optional)
	// pass 2: combine : rt_full as source texture, backbuffer as render target

	// get the effect
	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SCREEN_EFFECT ) );
	if ( ! pEffect )
		return S_FALSE;
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;
	if ( pScreenEffect->nTechniqueInEffect == INVALID_ID )
		return S_FALSE;

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = pScreenEffect->nTechniqueInEffect;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );

#if ISVERSION( DEVELOPMENT )
	// If the effects have been reloaded, then we need to update the effect feature index.
	if ( tFeat.nInts[ TF_INT_INDEX ] != ptTechnique->tFeatures.nInts[ TF_INT_INDEX ] )
	{
		V_RETURN( RestoreEffect( pInstance->nScreenEffectDef ) );
		V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
	}
#endif

	ASSERT_RETFAIL( ptTechnique && ptTechnique->hHandle );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );
	ASSERT_RETFAIL( nPassCount > 0 );

//#ifdef ENGINE_TARGET_DX9
	for ( UINT nPass = 0; nPass < nPassCount; nPass++ )
	{
		D3DC_EFFECT_PASS_HANDLE hPass = dxC_EffectGetPassByIndex( pEffect->pD3DEffect, ptTechnique->hHandle, nPass );
		ASSERT_CONTINUE( hPass );

#if ISVERSION(DEVELOPMENT)
		D3DC_PASS_DESC tPassDesc;
		V( dxC_EffectGetPassDesc( pEffect, hPass, &tPassDesc ) );
		//V( pEffect->pD3DEffect->GetPassDesc( hPass, &tPassDesc ) );
		ASSERTX_CONTINUE( tPassDesc.Annotations >= 1, "Screen effect pass is missing the minimum default annotations!" );
#endif

		SCREEN_EFFECT_OP	 eOp   = (SCREEN_EFFECT_OP)		dxC_EffectGetAnnotationAsIntByIndex( pEffect, hPass, SFX_PASS_ARG_OP );
		SCREEN_EFFECT_TARGET eSrc  = (SCREEN_EFFECT_TARGET)	dxC_EffectGetAnnotationAsIntByIndex( pEffect, hPass, SFX_PASS_ARG_FROM_RT );
		SCREEN_EFFECT_TARGET eDest = (SCREEN_EFFECT_TARGET)	dxC_EffectGetAnnotationAsIntByIndex( pEffect, hPass, SFX_PASS_ARG_TO_RT );

		ASSERT_CONTINUE( eDest != SFX_RT_NONE );

		switch ( eOp )
		{
		case SFX_OP_COPY:
			{
				V_RETURN( PrepareSourceTexture( eSrc, eDest ) );
			}
			break;
		case SFX_OP_SHADERS:
			ASSERT_CONTINUE( IS_VALID_INDEX( eSrc, NUM_SCREEN_EFFECT_TARGETS ) );
			ASSERT_CONTINUE( IS_VALID_INDEX( eDest, NUM_SCREEN_EFFECT_TARGETS ) );
			SPD3DCBASESHADERRESOURCEVIEW pSrc;
			if ( eSrc != SFX_RT_NONE )
			{
				RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), sgeTargets[ eSrc ] );
				pSrc = dxC_RTShaderResourceGet( nRT );
				ASSERT( pSrc );
			}
			{
				V_RETURN( sScreenEffectRenderPass( pScreenEffect, pEffect, *ptTechnique, nPass, pSrc, eDest, pInstance->fTransitionPhase ) );
			}
			break;
		}
	}
//#endif

	// if this effect was "exclusive", stop walking the priority list
	bContinue = ! pInstance->bExclusive;
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT CScreenEffects::PrepareSourceTexture( SCREEN_EFFECT_TARGET eSrc, SCREEN_EFFECT_TARGET eDest )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SCREENEFFECTS_ENABLED ) )
		return S_FALSE;

	// AE 2007.07.30: I've disabled this early return because I need src -> dst copy support for motion blur.
#if defined( DX10_DOF ) && 0
	//Just returning because we technically copy when applying the glow
	//see ln 1090 dxC_main.cpp
	//Chris we need to fix this system so we minimize copies
	return S_OK;
#endif

	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eSrc, NUM_SCREEN_EFFECT_TARGETS ) );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eDest, NUM_SCREEN_EFFECT_TARGETS ) );
	ASSERT_RETINVALIDARG( eSrc != SFX_RT_NONE );

	// only copy the backbuffer once per frame
	if ( m_nSourceIdentifier == sgeTargets[ eDest ] )
		return S_FALSE;

	RENDER_TARGET_INDEX nSrcRT  = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), sgeTargets[ eSrc  ] );
	RENDER_TARGET_INDEX nDestRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), sgeTargets[ eDest ] );

	if ( eSrc == SFX_RT_BACKBUFFER )
	{
		V_RETURN( dxC_CopyBackbufferToTexture( dxC_RTResourceGet( nDestRT ), NULL, NULL ) );
		V( dxC_RTSetDirty( nDestRT ) );
	}
	else
	{
		V_RETURN( dxC_CopyGPUTextureToGPUTexture( dxC_RTResourceGet( nSrcRT ), dxC_RTResourceGet( nDestRT ), 0 ) );
		V( dxC_RTSetDirty( nDestRT ) );
	}

	m_nSourceIdentifier = sgeTargets[ eDest ];

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ScreenFXRestore( void * pDef )
{
	SCREEN_EFFECT_DEFINITION * pDefinition = (SCREEN_EFFECT_DEFINITION*) pDef;
	ASSERT_RETFAIL( pDefinition );

	for ( int i = 0; i < SCREEN_EFFECT_TEXTURES; i++ )
	{
		if ( pDefinition->nTextureIDs[ i ] != INVALID_ID )
			continue;

		if ( pDefinition->szTextureFilenames[ i ][ 0 ] == NULL )
			continue;

		char szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrPrintf( szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_SCREENFX, pDefinition->szTextureFilenames[ i ] );

		e_TextureReleaseRef( pDefinition->nTextureIDs[ i ] ); 
		V_DO_SUCCEEDED( e_TextureNewFromFile(
			&pDefinition->nTextureIDs[ i ],
			szFilepath,
			TEXTURE_GROUP_NONE,
			TEXTURE_NONE ) )
		{
			e_TextureAddRef( pDefinition->nTextureIDs[ i ] );
		}

		ASSERTV( pDefinition->nTextureIDs[ i ] != INVALID_ID, "Failed to load screen effect texture:\n\n%s\n\nIn screen effect:\n\n%s", szFilepath, pDefinition->tHeader.pszName );
	}

	// don't bother checking for the effect when in fillpak mode
	if ( e_IsNoRender() )
		return S_OK;
	if ( dxC_IsPixomaticActive() )
		return S_OK;

	if ( TEST_MASK( pDefinition->dwDefFlags, SCREEN_EFFECT_DEF_FLAG_DX10_ONLY ) && ! e_IsDX10Enabled() )
		return S_OK;

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SCREEN_EFFECT ) );
	ASSERT_RETFAIL( pEffect && pEffect->pD3DEffect );

	D3DC_TECHNIQUE_HANDLE hTechnique = pEffect->pD3DEffect->GetTechniqueByName( pDefinition->szTechniqueName );
	if ( ! c_GetToolMode() )
	{
		ASSERTV( hTechnique, "Technique \"%s\" not found in screen effect \"%s\"!",
			pDefinition->szTechniqueName,
			pDefinition->tHeader.pszName );
	}

	if ( ! hTechnique )
	{
		pDefinition->nTechniqueInEffect = INVALID_ID;
		return S_FALSE;
	}

	for ( int i = 0; i < pEffect->nTechniques; i++ )
	{
		if ( pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ i ] ].hHandle == hTechnique )
		{
			pEffect->ptTechniques[ pEffect->pnTechniquesIndex[i] ].tFeatures.nInts[ TF_INT_INDEX ] = i;
			pDefinition->nTechniqueInEffect = i;
			pDefinition->nLayer = dx9_EffectGetAnnotation( pEffect, pEffect->ptTechniques[ pEffect->pnTechniquesIndex[i] ].hHandle, "nLayer" );
			break;
		}
	}

	return S_OK;
}