//----------------------------------------------------------------------------
// dx9_state.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_target.h"
#include "dxC_model.h"

#include "perfhier.h"

#include "dxC_particle.h"
#include "dxC_caps.h"
#include "dxC_anim_model.h"
#include "dxC_material.h"
#include "dxC_texture.h"
#include "e_texture_priv.h"
#include "e_settings.h"
#include "e_main.h"
#include "dxC_2d.h"
#include "dxC_environment.h"
#include "dxC_statemanager.h"

#pragma warning(disable: 4995)
#include "dxC_state.h"
#pragma warning(default: 4995)

#include "dxC_state_defaults.h"
#include "dxC_effect.h"
#include "dxC_VertexDeclarations.h"
#include "dxC_pixo.h"


#define DXC_VERTEX_PIXO_DECL
#undef __DXC_VERTEX_DECLARATIONS__
#include "dxC_VertexDeclarations.h"


#include "game.h"
#include "units.h"
#include "prime.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define NUM_DISPLAY_FORMATS			DXC_9_10( 6, 4 )

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct CLIP_PLANES
{
	PLANE						tPlanes[ MAX_CLIP_PLANES ];
	int							nCount;
	DWORD						dwFlags;
};

struct VERTEX_TYPE_DATA
{
	SPD3DCIALAYOUT					pVertexDeclaration;
	const D3DC_INPUT_ELEMENT_DESC*	pVertexElements;
	PIXO_VERTEX_DECL				pPixoVertexDecl;
	int								nStreamCount;
	UINT							nElementCount;
	int								nVertexSize[ DXC_MAX_VERTEX_STREAMS ];
	VERTEX_SHADER_VERSION			nMinimumShaderVersion;
#if defined(ENGINE_TARGET_DX10)
	D3D10_PASS_DESC					PassDesc;
#endif
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
CLIP_PLANES					gtClipPlanes;

static VERTEX_TYPE_DATA gpVertexTypeData[ NUM_VERTEX_DECL_TYPES ] =
{
	// If you add/remove/reorder, be sure to update enum VERTEX_DECL_TYPE in e_effect_priv.h in parallel.
	{ NULL, VS_SIMPLE_INPUT,									PIXO_VS_SIMPLE_INPUT,						VS_SIMPLE_INPUT_STREAM_COUNT,				VS_SIMPLE_INPUT_COUNT,				{ sizeof( VS_SIMPLE_INPUT_STREAM_0), 0, 0, 0 },																									VS_1_1	},
	{ NULL, VS_BACKGROUND_INPUT_64,								PIXO_VS_BACKGROUND_INPUT_64,				VS_BACKGROUND_INPUT_64_STREAM_COUNT,		VS_BACKGROUND_INPUT_64_COUNT,		{ sizeof( VS_BACKGROUND_INPUT_64_STREAM_0 ), sizeof( VS_BACKGROUND_INPUT_64_STREAM_1 ), sizeof( VS_BACKGROUND_INPUT_64_STREAM_2 ), 0 },			VS_1_1	},
	{ NULL, VS_BACKGROUND_INPUT_32,								PIXO_VS_BACKGROUND_INPUT_32,				VS_BACKGROUND_INPUT_32_STREAM_COUNT,		VS_BACKGROUND_INPUT_32_COUNT,		{ sizeof( VS_BACKGROUND_INPUT_32_STREAM_0 ), sizeof( VS_BACKGROUND_INPUT_32_STREAM_1 ), sizeof( VS_BACKGROUND_INPUT_32_STREAM_2 ), 0 },			VS_1_1	},
	{ NULL, VS_BACKGROUND_INPUT_16,								PIXO_VS_BACKGROUND_INPUT_16,				VS_BACKGROUND_INPUT_16_STREAM_COUNT,		VS_BACKGROUND_INPUT_16_COUNT,		{ sizeof( VS_BACKGROUND_INPUT_16_STREAM_0 ), sizeof( VS_BACKGROUND_INPUT_16_STREAM_1 ), 0, 0 },													VS_1_1	},
	{ NULL, VS_ANIMATED_INPUT,									PIXO_VS_ANIMATED_INPUT,						VS_ANIMATED_INPUT_STREAM_COUNT,				VS_ANIMATED_INPUT_COUNT,			{ sizeof( VS_ANIMATED_INPUT_STREAM_0 ), sizeof( VS_ANIMATED_INPUT_STREAM_1 ), sizeof( VS_ANIMATED_INPUT_STREAM_2 ), 0 },						VS_2_0	},
	{ NULL, VS_ANIMATED_INPUT_11,								PIXO_VS_ANIMATED_INPUT_11,					VS_ANIMATED_INPUT_11_STREAM_COUNT,			VS_ANIMATED_INPUT_11_COUNT,			{ sizeof( VS_ANIMATED_INPUT_11_STREAM_0 ), sizeof( VS_ANIMATED_INPUT_11_STREAM_1 ), sizeof( VS_ANIMATED_INPUT_11_STREAM_2 ), 0 },				VS_1_1	},
	{ NULL, g_ptParticleMeshVertexDeclarationArray_Multi,		NULL,										1,											4,									{ sizeof( PARTICLE_MESH_VERTEX_MULTI ), 0, 0, 0 },																								VS_2_0	},
	{ NULL, g_ptParticleMeshVertexDeclarationArray_Multi_11,	NULL,										1,											1,									{ sizeof( PARTICLE_MESH_VERTEX_MULTI ),	0, 0, 0 },																								VS_1_1	},
#ifdef HAVOKFX_ENABLED
	{ NULL, g_ptParticleMeshVertexDeclarationArray_HavokFX,		NULL,										1,											7,									{ sizeof( PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS ), 0, 0, 0 },																					VS_2_0	},
	{ NULL, g_ptParticleVertexDeclarationArray_HavokFXParticle,	NULL,										1,											3,									{ sizeof( HAVOKFX_PARTICLE_VERTEX ), 0, 0, 0 },																									VS_2_0	},
#else
	{ NULL, NULL,												NULL,										1,											7,									{ sizeof( PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS ), 0, 0, 0 },																					VS_2_0	},
	{ NULL, NULL,												NULL,										1,											3,									{ sizeof( HAVOKFX_PARTICLE_VERTEX ), 0, 0, 0 },																									VS_2_0	},
#endif
	//{ NULL, g_ptXYZColVertexDeclarationArray,					NULL,										1,											2,									{ sizeof( XYZ_COL_VERTEX ), 0, 0, 0 }, 																											VS_1_1	},
	{ NULL, VS_XYZ_COL,											PIXO_VS_XYZ_COL,							VS_XYZ_COL_STREAM_COUNT,					VS_XYZ_COL_COUNT,					{ sizeof( VS_XYZ_COL_STREAM_0 ), 0, 0, 0 },																										VS_1_1	},
	{ NULL, VS_XYZ_COL_UV,										PIXO_VS_XYZ_COL_UV,							VS_XYZ_COL_UV_STREAM_COUNT,					VS_XYZ_COL_UV_COUNT,				{ sizeof( VS_XYZ_COL_UV_STREAM_0 ), 0, 0, 0 },																									VS_1_1	},
	//{ NULL, VS_UI_XYZW_COL_UV,									PIXO_VS_UI_XYZW_COL_UV,						VS_UI_XYZW_COL_UV_STREAM_COUNT,				VS_UI_XYZW_COL_UV_COUNT,			{ sizeof( VS_UI_XYZW_COL_UV_STREAM_0 ), 0, 0, 0 },																							VS_1_1	},
	{ NULL, VS_POSITION_TEX,									PIXO_VS_POSITION_TEX,						VS_POSITION_TEX_STREAM_COUNT,				VS_POSITION_TEX_COUNT,				{ sizeof( VS_POSITION_TEX_STREAM_0 ), 0, 0, 0 },																								VS_1_1	},
	{ NULL, VS_PARTICLE_INPUT,									PIXO_VS_PARTICLE_INPUT,						VS_PARTICLE_INPUT_STREAM_COUNT,				VS_PARTICLE_INPUT_COUNT,			{ sizeof( VS_PARTICLE_INPUT_STREAM_0 ), 0, 0, 0 },																								VS_2_0	},
	{ NULL, VS_GPU_PARTICLE_INPUT,								PIXO_VS_GPU_PARTICLE_INPUT,					VS_GPU_PARTICLE_INPUT_STREAM_COUNT,			VS_GPU_PARTICLE_INPUT_COUNT,		{ sizeof( VS_GPU_PARTICLE_INPUT_STREAM_0 ), 0, 0, 0 },																							VS_3_0	},
	{ NULL, VS_PARTICLE_SIMULATION,								PIXO_VS_PARTICLE_SIMULATION,				VS_PARTICLE_SIMULATION_STREAM_COUNT,		VS_PARTICLE_SIMULATION_COUNT,		{ sizeof( VS_PARTICLE_SIMULATION_STREAM_0 ),	0, 0, 0 },																						VS_2_0	},
	{ NULL, VS_FLUID_SIMULATION_INPUT,							PIXO_VS_FLUID_SIMULATION_INPUT,				VS_FLUID_SIMULATION_INPUT_STREAM_COUNT, 	VS_FLUID_SIMULATION_INPUT_COUNT,	{ sizeof( VS_FLUID_SIMULATION_INPUT_STREAM_0 ), 0, 0, 0 },	  																					VS_3_0	},
	{ NULL, VS_FLUID_RENDERING_INPUT,							PIXO_VS_FLUID_RENDERING_INPUT,				VS_FLUID_RENDERING_INPUT_STREAM_COUNT,		VS_FLUID_RENDERING_INPUT_COUNT,		{ sizeof( VS_FLUID_RENDERING_INPUT_STREAM_0 ), 0, 0, 0 },  																						VS_3_0	},
	{ NULL, VS_FULLSCREEN_QUAD,									PIXO_VS_FULLSCREEN_QUAD,					VS_FULLSCREEN_QUAD_STREAM_COUNT,			VS_FULLSCREEN_QUAD_COUNT,			{ sizeof( VS_FULLSCREEN_QUAD_STREAM_0 ),	0, 0, 0 },																							VS_1_1	},
	{ NULL, VS_BACKGROUND_ZBUFFER_INPUT,						PIXO_VS_BACKGROUND_ZBUFFER_INPUT,			VS_BACKGROUND_ZBUFFER_INPUT_STREAM_COUNT,	VS_BACKGROUND_ZBUFFER_INPUT_COUNT,	{ sizeof( VS_BACKGROUND_ZBUFFER_INPUT_STREAM_0 ), 0, 0, 0 },																					VS_1_1	},
	{ NULL, VS_ANIMATED_ZBUFFER_INPUT,							PIXO_VS_ANIMATED_ZBUFFER_INPUT,				VS_ANIMATED_ZBUFFER_INPUT_STREAM_COUNT,		VS_ANIMATED_ZBUFFER_INPUT_COUNT,	{ sizeof( VS_ANIMATED_ZBUFFER_INPUT_STREAM_0 ), 0, 0, 0 },																						VS_2_0	},
	{ NULL, VS_ANIMATED_ZBUFFER_INPUT_11,						PIXO_VS_ANIMATED_ZBUFFER_INPUT_11,			VS_ANIMATED_ZBUFFER_INPUT_11_STREAM_COUNT,	VS_ANIMATED_ZBUFFER_INPUT_11_COUNT,	{ sizeof( VS_ANIMATED_ZBUFFER_INPUT_11_STREAM_0 ), 0, 0, 0 },																					VS_1_1	},
	{ NULL, VS_R3264_POS_TEX,									PIXO_VS_R3264_POS_TEX,						VS_R3264_POS_TEX_STREAM_COUNT,				VS_R3264_POS_TEX_COUNT,				{ sizeof( VS_R3264_POS_TEX_STREAM_0 ), sizeof( VS_R3264_POS_TEX_STREAM_1 ), 0, 0 },																VS_2_0	},
	{ NULL, VS_R16_POS_TEX,										PIXO_VS_R16_POS_TEX,						VS_R16_POS_TEX_STREAM_COUNT,				VS_R16_POS_TEX_COUNT,				{ sizeof( VS_R16_POS_TEX_STREAM_0 ), sizeof( VS_R16_POS_TEX_STREAM_1 ), 0, 0 },																	VS_2_0	},
	{ NULL, VS_ANIMATED_POS_TEX_INPUT,							PIXO_VS_ANIMATED_POS_TEX_INPUT,				VS_ANIMATED_POS_TEX_INPUT_STREAM_COUNT,		VS_ANIMATED_POS_TEX_INPUT_COUNT,	{ sizeof( VS_ANIMATED_POS_TEX_INPUT_STREAM_0 ), sizeof( VS_ANIMATED_POS_TEX_INPUT_STREAM_1 ), 0, 0 },											VS_2_0	},	
};

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

// ensure virtual vert decl types are mapped properly to hardware-optimized concrete types
static VERTEX_DECL_TYPE sResolveVertDeclType( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect )
{
	ASSERT_RETVAL( eType >= 0, VERTEX_DECL_INVALID );
	ASSERT_RETVAL( pEffect, VERTEX_DECL_INVALID );
	if ( eType < VERTEX_DECL_VIRTUAL_TYPES )
		return eType;

	return eType;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

PRESULT dxC_StateGetStats( STATE_SET_TYPE eStat, UINT & nActual, UINT & nTotal )
{
	if ( ! gpStateManager )
		return S_FALSE;
#ifdef ENGINE_TARGET_DX9
	gpStateManager->GetStateStats( eStat, nActual, nTotal );
#else
	nActual = nTotal = 0;
#endif
	return S_OK;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

void dx9_AddStatesTypeStats( WCHAR * pwszStr, UINT * pnTotalChangesPerType, UINT * pnActualChangesPerType )
{
	const int nBufLen = EFFECT_STATE_MANAGER_STATS_LEN;
	WCHAR wszTempStr[ nBufLen ];

#define STAT_NAME(type)						L"[" L##type L"] "
#define STATS_LINE_3(type1, type2, type3)	STAT_NAME(type1) L" %5u,%5u,%5u  " STAT_NAME(type2) L" %5u,%5u,%5u  " STAT_NAME(type3) L" %5u,%5u,%5u\n"
#define STATS_LINE_2(type1, type2)			STAT_NAME(type1) L" %5u,%5u,%5u  " STAT_NAME(type2) L" %5u,%5u,%5u\n"
#define STATS_DATA_STR(type)											\
	pnActualChangesPerType[ type ],										\
	pnTotalChangesPerType [ type ] - pnActualChangesPerType[ type ],		\
	pnTotalChangesPerType [ type ]

	PStrCopy( wszTempStr, pwszStr, nBufLen );
	PStrPrintf( pwszStr, nBufLen, 
		L"%s\n"
		L"[type]   act filt  tot\n"
		STATS_LINE_3( "VS  ", "PS  ", "TEX " )
		STATS_LINE_3( "RS  ", "SS  ", "TSS " )
		STATS_LINE_3( "VSCB", "VSCF", "VSCI" )
		STATS_LINE_3( "PSCB", "PSCF", "PSCI" )
		STATS_LINE_2( "FVF ", "VDEC" )
		STATS_LINE_2( "SSRC", "INDS" )
		STATS_LINE_3( "XFRM", "LENB", "LGHT" )
		STATS_LINE_2( "MTRL", "NPCH" ),
		wszTempStr,
		STATS_DATA_STR( STATE_SET_VERTEX_SHADER ),
		STATS_DATA_STR( STATE_SET_PIXEL_SHADER ),
		STATS_DATA_STR( STATE_SET_TEXTURE ),
		STATS_DATA_STR( STATE_SET_RENDER_STATE ),
		STATS_DATA_STR( STATE_SET_SAMPLER_STATE ),
		STATS_DATA_STR( STATE_SET_TEXTURE_STAGE_STATE ),
		STATS_DATA_STR( STATE_SET_VERTEX_SHADER_CONSTANT_B ),
		STATS_DATA_STR( STATE_SET_VERTEX_SHADER_CONSTANT_F ),
		STATS_DATA_STR( STATE_SET_VERTEX_SHADER_CONSTANT_I ),
		STATS_DATA_STR( STATE_SET_PIXEL_SHADER_CONSTANT_B ),
		STATS_DATA_STR( STATE_SET_PIXEL_SHADER_CONSTANT_F ),
		STATS_DATA_STR( STATE_SET_PIXEL_SHADER_CONSTANT_I ),
		STATS_DATA_STR( STATE_SET_FVF ),
		STATS_DATA_STR( STATE_SET_VERTEX_DECLARATION ),
		STATS_DATA_STR( STATE_SET_STREAM_SOURCE ),
		STATS_DATA_STR( STATE_SET_INDICES ),
		STATS_DATA_STR( STATE_SET_TRANSFORM ),
		STATS_DATA_STR( STATE_LIGHT_ENABLE ),
		STATS_DATA_STR( STATE_SET_LIGHT ),
		STATS_DATA_STR( STATE_SET_MATERIAL ),
		STATS_DATA_STR( STATE_SET_NPATCH_MODE ) );

#undef STATS_LINE_3
#undef STATS_LINE_2
#undef STATS_DATA_STR
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void dx9_AddStatesNonEffectStats( WCHAR * pwszStr, UINT nNonEffectChanges )
{
	const int nBufLen = EFFECT_STATE_MANAGER_STATS_LEN;
	WCHAR wszTempStr[ nBufLen ];

	PStrCopy( wszTempStr, pwszStr, nBufLen );
	PStrPrintf( pwszStr, nBufLen, 
		L"%s\nTotal non-effect state changes: %6u",
		wszTempStr,
		nNonEffectChanges );
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX9
PRESULT dx9_SetStateManagerAll()
{
	// iterate effects and set each state manager
	for ( D3D_EFFECT * pEffect = dxC_EffectGetFirst();
		pEffect;
		pEffect = dxC_EffectGetNext( pEffect ) )
	{
		ASSERT_CONTINUE( pEffect->pD3DEffect || 0 == ( pEffect->dwFlags & EFFECT_FLAG_LOADED ) );
		if ( ! pEffect->pD3DEffect )
			continue;
		V_CONTINUE( pEffect->pD3DEffect->SetStateManager( gpStateManager ) );
	}

	return S_OK;
}
#endif

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

PRESULT dx9_StateManagerDestroy()
{
	if ( gpStateManager )
		gpStateManager->Destroy();
	gpStateManager = NULL;
	return S_OK;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

STATE_MANAGER_TYPE dx9_SetStateManager( STATE_MANAGER_TYPE eTypeOverride )
{
	STATE_MANAGER_TYPE eFinalType = e_GetRenderFlag( RENDER_FLAG_FILTER_STATES ) ? STATE_MANAGER_OPTIMIZE : STATE_MANAGER_TRACK;
	//STATE_MANAGER_TYPE eFinalType = STATE_MANAGER_TRACK;

	if ( eFinalType == STATE_MANAGER_OPTIMIZE )
		DX9_BLOCK( gdwEffectBeginFlags = D3DXFX_DONOTSAVESTATE );  //KMNV shouldn't need any flags in dx10
	else
		gdwEffectBeginFlags = 0;

	if ( eTypeOverride >= 0 )
		eFinalType = eTypeOverride;

	CStateManagerInterface* pManager;
	if ( eFinalType != geStateManagerType )
	{
		if ( gpStateManager )
			gpStateManager->Destroy();
		gpStateManager = NULL;
		pManager = CStateManagerInterface::Create( eFinalType );
	} else
	{
		pManager = gpStateManager;
	}

	if ( pManager )
	{
		geStateManagerType = eFinalType;
	}
	else
	{
		geStateManagerType = STATE_MANAGER_INVALID;
	}

	if ( gpStateManager != pManager )
		gpStateManager = pManager;

	DX9_BLOCK( V( dx9_SetStateManagerAll() ); ) //In dx10 the statemanager is not associated with the effects framework

	return eFinalType;
}

#if defined(ENGINE_TARGET_DX9)
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void dx9_ResetFrameStateStats()
{
	gnStateStatsTotalCalls = 0;
	gnStateStatsNonEffectCalls = 0;
	if ( gpStateManager )
		gpStateManager->ResetStats();
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void dx9_DirtyStateCache()
{
	if ( gpStateManager )
		gpStateManager->DirtyCachedValues();
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
WCHAR * dx9_GetStateStats( int nMaxLen )
{
	const int cnMaxStr = 1024;
	int nMaxStr = min(cnMaxStr, nMaxLen);
	static WCHAR wszStats[ cnMaxStr ];
	if ( gpStateManager )
	{
		WCHAR * pwszStats = NULL;
		pwszStats = gpStateManager->EndFrameStats();
		PStrCopy( wszStats, pwszStats, nMaxStr );
		//pwszStats = wszStats;
	} else
	{
		PStrPrintf( wszStats, nMaxStr, L"State manager not enabled: complete state stats unavailable.\nBasic non-effect renderstate calls: Non-effect: %6d Total: %6d", gnStateStatsNonEffectCalls, gnStateStatsTotalCalls );
		//pwszStats = wszStats;
	}
	static WCHAR wszEffects[ cnMaxStr ];
	V( dx9_GetEffectCacheStatsString( wszEffects, nMaxStr ) );
	PStrCat( wszStats, wszEffects, nMaxStr );
	V( dx9_GetTechniqueStatsString( wszEffects, nMaxStr ) );
	PStrCat( wszStats, wszEffects, nMaxStr );
	return wszStats;
}

PRESULT dx9_VertexDeclarationsRestore()
{
	if ( e_IsNoRender() )
		return S_FALSE;

	VERTEX_SHADER_VERSION eVS;
	PIXEL_SHADER_VERSION ePS;
	dxC_GetCurrentMaxShaderVersions( eVS, ePS );

	for ( int i = 0; i < NUM_VERTEX_DECL_TYPES; i++ )
	{
		VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ i ];
		pTypeData->pVertexDeclaration = NULL;		
		
		if ( !pTypeData->pVertexElements )
			continue;
		
		// CHB 2006.06.21 - Don't create high-end vertex formats on low-end hardware.
		if ( eVS >= pTypeData->nMinimumShaderVersion )
		{
			V_CONTINUE( dxC_GetDevice()->CreateVertexDeclaration( pTypeData->pVertexElements, &pTypeData->pVertexDeclaration ) );
		}
	}

	return S_OK;
}

PRESULT dx9_VertexDeclarationsDestroy()
{
	for ( int i = 0; i < NUM_VERTEX_DECL_TYPES; i++ )
	{
		VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ i ];
		pTypeData->pVertexDeclaration = NULL;
	}

	return S_OK;
}

#elif defined( ENGINE_TARGET_DX10 )
PRESULT dx10_CreateIAObject( VERTEX_DECL_TYPE eType, ID3D10EffectPass* pPass, LPD3DCIALAYOUT* ppD3DIA )
{
	ASSERT_RETINVALIDARG( ppD3DIA );

	VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ eType ];
	ASSERT_RETFAIL( pTypeData );

	D3DC_PASS_DESC Desc;
	V_RETURN( pPass->GetDesc( &Desc ) );

	return dxC_GetDevice()->CreateInputLayout( pTypeData->pVertexElements, pTypeData->nElementCount, Desc.pIAInputSignature, Desc.IAInputSignatureSize, ppD3DIA );
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride )
{
	TRY_STATE_MANAGER( gpStateManager, SetStreamSource( StreamNumber, pStreamData, OffsetInBytes, Stride ) );

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, VERTEX_DECL_TYPE eType, D3DC_FVF dwFVF, UINT Stride, const D3D_EFFECT* pEffect /*= NULL*/ )
{
	if ( eType != VERTEX_DECL_INVALID )
	{
		V_RETURN( dxC_SetVertexDeclaration( eType, pEffect ) );
	}
	else if ( dwFVF )
	{
		V_RETURN( dx9_SetFVF( dwFVF ) );
	}
	else
	{
		ASSERTX_RETFAIL( 0, "No valid VertDecl or FVF in SetStreamSource!" );
	}

	V_RETURN( dxC_SetStreamSource( StreamNumber, pStreamData, OffsetInBytes, Stride ) );

	return S_OK;
}

PRESULT dxC_SetStreamSource( D3D_VERTEX_BUFFER_DEFINITION& tVBDef, UINT OffsetInBytes, const D3D_EFFECT* pEffect /*= NULL*/ )
{
	int nStreamCount = dxC_GetStreamCount( &tVBDef, pEffect );
	for ( int StreamNumber = 0; StreamNumber < nStreamCount; StreamNumber++ )
	{
		if ( dxC_IsPixomaticActive() )
		{
			V( dxC_SetVertexDeclaration( tVBDef.eVertexType, pEffect ) );
			V_RETURN( dxC_PixoSetStream( StreamNumber, (const PIXO_VERTEX_DATA)tVBDef.pLocalData[StreamNumber], OffsetInBytes, dxC_GetVertexSize( StreamNumber, &tVBDef ), tVBDef.nVertexCount ) );
		}
		else
		{
			D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( tVBDef.nD3DBufferID[ StreamNumber ] );
			ASSERT_RETFAIL( pVB );

			//int nReplacement;
			//V_CONTINUE( dxC_VertexBufferShouldSet( pVB, nReplacement ) );
			//if ( nReplacement != INVALID_ID )
			//{
			//	pVB = dxC_VertexBufferGet( nReplacement );
			//	ASSERT_RETFAIL( pVB );
			//}

			V_RETURN( dxC_SetStreamSource( StreamNumber, (LPD3DCVB)pVB->pD3DVertices, OffsetInBytes, tVBDef.eVertexType, tVBDef.dwFVF, dxC_GetVertexSize( StreamNumber, &tVBDef ), pEffect ) );

			V( dxC_VertexBufferDidSet( pVB, tVBDef.nBufferSize[ StreamNumber ] ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_SetFVF( D3DC_FVF dwFVF )
{
	TRY_STATE_MANAGER( gpStateManager, SetFVF( (DWORD)dwFVF ) );

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetIndices( LPD3DCIB pIB, D3DFORMAT ibFormat )
{
#if defined(ENGINE_TARGET_DX9)
	TRY_STATE_MANAGER( gpStateManager, SetIndices( pIB ) );
#elif defined( ENGINE_TARGET_DX10 )
	gpStateManager->SetIndices( pIB, ibFormat );
#endif

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}

PRESULT dxC_SetIndices( const struct D3D_INDEX_BUFFER_DEFINITION & tIBDef, BOOL bForce /*= FALSE*/ )
{
	if ( dxC_IsPixomaticActive() )
	{
		ASSERT_RETFAIL( tIBDef.tFormat == D3DFMT_INDEX16 );
		return dxC_PixoSetIndices( (const PIXO_INDEX_DATA)tIBDef.pLocalData, tIBDef.nBufferSize / sizeof(unsigned short), sizeof(unsigned short) );
	}


	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( tIBDef.nD3DBufferID );
	ASSERT_RETFAIL( pIB );

	//int nReplacement = INVALID_ID;
	//V( bForce || dxC_IndexBufferShouldSet( pIB, nReplacement ) );
	//if ( nReplacement != INVALID_ID )
	//{
	//	pIB = dxC_IndexBufferGet( nReplacement );
	//	ASSERT_RETFAIL( pIB );
	//}

	V_RETURN( dxC_SetIndices( (*pIB).pD3DIndices, tIBDef.tFormat ) );

	V( dxC_IndexBufferDidSet( pIB, tIBDef.nBufferSize ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetTextureConditional ( 
	D3D_MODEL * pModel,
	TEXTURE_TYPE eType,
	int nLOD,	// CHB 2006.11.28
	const EFFECT_TECHNIQUE & tTechnique,
	const D3D_EFFECT * pEffect, 
	EFFECT_PARAM eParam,
	int nStage, 
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh,
	int nMeshIndex,
	BOOL bAllowDbgMipOverride /*= FALSE*/,
	const MATERIAL * pMaterial /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( nMeshIndex < pModelDef->nMeshCount );
	ASSERT_RETINVALIDARG( eType < NUM_TEXTURE_TYPES );
	int nTextureId = dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMeshIndex, eType, nLOD, FALSE, pMaterial );	// CHB 2006.11.28

	V_RETURN( dxC_SetTexture(
		nTextureId,
		tTechnique,
		pEffect,
		eParam,
		nStage,
		pModelDef,
		pMaterial,
		eType,
		bAllowDbgMipOverride ) );

	// adjust addressing mode for diffuse 2 only
	if ( nTextureId != INVALID_ID && eType == TEXTURE_DIFFUSE2 )
	{
		V_RETURN( dx9_SetDiffuse2SamplerStates( (D3DC_TEXTURE_ADDRESS_MODE)pMesh->nDiffuse2AddressMode[ 0 ], (D3DC_TEXTURE_ADDRESS_MODE)pMesh->nDiffuse2AddressMode[ 1 ] ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGetViewportNearFar( BOOL bSkyboxDepth, float & fMinDepth, float & fMaxDepth )
{
	if ( bSkyboxDepth )
	{
		fMinDepth = SKYBOX_FAR_PLANE_PCT;
		fMaxDepth = 1.f;
	}
	else
	{
		fMinDepth = 0.f;
		fMaxDepth = SKYBOX_FAR_PLANE_PCT;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ViewportGetDimensions( D3DC_VIEWPORT & tVP, RENDER_TARGET_INDEX eRT, BOOL bSkyboxDepth /*= FALSE*/, int nSwapChainIndex /*= INVALID_ID*/ )
{
	if ( AppIsHellgate() )
	{
		return dxC_GetFullViewportDesc( tVP, bSkyboxDepth, nSwapChainIndex );
	}

	if ( AppIsTugboat() )
	{
		if( e_GetUICovered() )
		{
		}
		else if( e_GetUICoverageWidth() > 0 )
		{
			int nScreenWidth, nScreenHeight;
			dxC_GetRenderTargetDimensions( nScreenWidth, nScreenHeight, eRT );

			BOUNDING_BOX tViewport;
			if( e_GetUICoveredLeft() )
			{
				tViewport.vMin = VECTOR( e_GetUICoverageWidth(), 0, 0.0f );
				tViewport.vMax = VECTOR( (float)nScreenWidth, (float)nScreenHeight, 1.0f );
			}
			else
			{
				tViewport.vMin = VECTOR( 0, 0, 0.0f );
				tViewport.vMax = VECTOR( (float)nScreenWidth - e_GetUICoverageWidth(), (float)nScreenHeight, 1.0f );
			}
			sGetViewportNearFar( bSkyboxDepth, tViewport.vMin.z, tViewport.vMax.z );
			tVP.TopLeftX	= DWORD( tViewport.vMin.fX );
			tVP.TopLeftY	= DWORD( tViewport.vMin.fY );
			tVP.Width		= DWORD( tViewport.vMax.fX - tViewport.vMin.fX );
			tVP.Height		= DWORD( tViewport.vMax.fY - tViewport.vMin.fY );
			tVP.MinDepth	= tViewport.vMin.fZ;
			tVP.MaxDepth	= tViewport.vMax.fZ;
		}
		else
		{
			return dxC_GetFullViewportDesc( tVP, bSkyboxDepth, nSwapChainIndex );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ResetViewport( BOOL bSkyboxDepth /*= FALSE*/ )
{
	if ( AppIsHellgate() )
	{
		return dxC_ResetFullViewport( bSkyboxDepth );
	}

	if ( AppIsTugboat() )
	{
		D3DC_VIEWPORT tVP;
		dxC_ViewportGetDimensions( tVP, dxC_GetCurrentRenderTarget(), bSkyboxDepth );

		if( e_GetUICovered() )
		{
		}
		else if( e_GetUICoverageWidth() > 0 )
		{
			V( dxC_SetViewport( tVP ) );
			BOUNDING_BOX tViewport = {
				VECTOR( (float)tVP.TopLeftX, (float)tVP.TopLeftY, tVP.MinDepth ),
				VECTOR( (float)( tVP.TopLeftX + tVP.Width ), (float)( tVP.TopLeftY + tVP.Height ), tVP.MaxDepth )
			};
			e_SetViewportOverride( &tViewport );
		}
		else
		{
			V( dxC_SetViewport( tVP ) );
			e_SetViewportOverride( NULL );
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sGetFullWidthHeight( int & nWidth, int & nHeight, int nSwapChainIndex = INVALID_ID )
{
	if ( dxC_IsPixomaticActive() )
	{
		dxC_PixoGetBufferDimensions( nWidth, nHeight );
		return;
	}

	RENDER_TARGET_INDEX eRT = dxC_GetCurrentRenderTarget();

	if ( eRT == RENDER_TARGET_NULL )
	{
		nWidth = 0;
		nHeight = 0;
		return;
	}

	if ( nSwapChainIndex == INVALID_ID )
		nSwapChainIndex = dxC_GetCurrentSwapChainIndex();
	int nCurSwapChainIndex = dxC_GetRenderTargetSwapChainIndex( eRT );
	if ( RENDER_TARGET_INVALID == eRT || ( nCurSwapChainIndex != INVALID_ID && nCurSwapChainIndex != nSwapChainIndex ) )
	{
		eRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
	}

	dxC_GetRenderTargetDimensions( nWidth, nHeight, eRT );

	if ( e_GeneratingCubeMap() )
	{
		nWidth  = e_CubeMapSize();
		nHeight = e_CubeMapSize();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_GetFullViewportDesc( D3DC_VIEWPORT & tVP, BOOL bSkyboxDepth /*= FALSE*/, int nSwapChainIndex /*= INVALID_ID*/ )
{
	int nScreenWidth, nScreenHeight;
	sGetFullWidthHeight( nScreenWidth, nScreenHeight, nSwapChainIndex );

	tVP.TopLeftX = 0;
	tVP.TopLeftY = 0;
	tVP.Width    = nScreenWidth;
	tVP.Height   = nScreenHeight;

	float fMin, fMax;
	sGetViewportNearFar( bSkyboxDepth, fMin, fMax );
	tVP.MinDepth = fMin;
	tVP.MaxDepth = fMax;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ResetFullViewport( BOOL bSkyboxDepth /*= FALSE*/, int nSwapChainIndex /*= INVALID_ID*/ )
{
	D3DC_VIEWPORT tVP;
	V( dxC_GetFullViewportDesc( tVP, bSkyboxDepth ) );
	return dxC_SetViewport( tVP );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetScissorRect ( BOUNDING_BOX & tBBox, UINT index )
{
	int nWidth, nHeight;
	sGetFullWidthHeight( nWidth, nHeight );

	ASSERT_RETINVALIDARG( index == 0 );
	RECT tRect;
	tRect.top    = (LONG)( ( tBBox.vMin.fY * 0.5f + 0.5f ) * nHeight );
	tRect.bottom = (LONG)( ( tBBox.vMax.fY * 0.5f + 0.5f ) * nHeight );
	tRect.left   = (LONG)( ( tBBox.vMin.fX * 0.5f + 0.5f ) * nWidth  );
	tRect.right  = (LONG)( ( tBBox.vMax.fX * 0.5f + 0.5f ) * nWidth  );

	// if the bbox verts are behind the camera, use full-screen scissor rect
	//if ( tBBox.vMin.fZ >= 1.0f || tBBox.vMax.fZ >= 1.0f )
	//{
	//	int a=0;
	//}
	//	dxC_SetScissorRect( NULL );
	//else
	return dxC_SetScissorRect( &tRect );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetClipPlaneRect ( const D3DXVECTOR3 * pvCorners, int nCorners, const D3DXMATRIXA16 * pmatProj, BOOL bCommit )
{
	V( dxC_ResetClipPlanes( FALSE ) );

	// turn off clipping if corner verts are null
	if ( ! pvCorners )
	{
		// unset user clip planes
		if ( bCommit )
		{
			V_RETURN( dxC_CommitClipPlanes() );
		}
		return S_OK;
	}
	ASSERT_RETINVALIDARG( nCorners == 4 );
	ASSERT_RETINVALIDARG( pmatProj );

	PLANE tPlane;
	D3DXVECTOR4 vEyePoint( 0.0f, 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vEyePointView( 0.0f, 0.0f, 0.0f );			// origin in camera space

	// get the camera origin in proj space (it's offset because of near plane)
	D3DXVec3Transform( &vEyePoint, &vEyePointView, pmatProj );

	// points should be arranged as:
	// 1)  xy1		top		left
	// 2)  Xy1		top		right
	// 3)  XY1		bottom	left
	// 4)  xY1		bottom	right
	// ... to ensure proper sign of clip planes

	// sort them now
	// this method requires vaguely squarish clip plane corners (2 corners on each side of center in x and y)
	D3DXVECTOR3 vCenter( 0,0,0 );
	for ( int i = 0; i < 4; i++ )
		vCenter += pvCorners[ i ];
	vCenter *= 0.25f;

	BYTE bySlots[4];	// stores, for each side, which index corner to use for proper signage
	memset( bySlots, -1, sizeof(bySlots) );
	BYTE byOrder[4] = { 0, 2, 3, 1 };
	for ( int i = 0; i < 4; i++ )
	{
		// classify each point with bits -- 0x2 for X, 0x1 for Y
		BYTE bySide = 0;
		if ( pvCorners[ i ].x > vCenter.x )
			bySide |= (BYTE)0x2;
		if ( pvCorners[ i ].y > vCenter.y )
			bySide |= (BYTE)0x1;
		bySlots[ byOrder[ bySide ] ] = i;
	}

	for ( int i = 0; i < 4; i++ )
	{
		PlaneFromPoints( &tPlane, (VECTOR*)&pvCorners[ byOrder[ bySlots[ i ] ] ], (VECTOR*)&pvCorners[ byOrder[ bySlots[ ( i + 1 ) % 4 ] ] ], (VECTOR*)&vEyePoint );
		V( dxC_PushClipPlane( &tPlane ) );
	}

	if ( bCommit )
	{
		V_RETURN( dxC_CommitClipPlanes() );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetClipPlaneRect ( const BOUNDING_BOX * ptBBox, const D3DXMATRIXA16 * pmatProj, BOOL bCommit )
{
	// turn off clipping if bbox is null, or
	// if clip verts contain entire screen or are behind camera
	if ( ptBBox == NULL ||
		 (  ptBBox->vMax.fX >=  1.0f && 
			ptBBox->vMax.fY >=  1.0f && 
			ptBBox->vMin.fX <= -1.0f && 
			ptBBox->vMin.fY <= -1.0f ) ||
		(	ptBBox->vMin.fZ >=  1.0f ||
			ptBBox->vMax.fZ >=  1.0f ) )
	{
		// unset user clip planes
		V_RETURN( dxC_ResetClipPlanes( TRUE ) );
		return S_OK;
	}

	ASSERT_RETINVALIDARG( pmatProj );

	D3DXVECTOR3 vCorner[ 4 ];

	vCorner[ 0 ] = D3DXVECTOR3( ptBBox->vMin.fX, ptBBox->vMin.fY, 1.0f );
	vCorner[ 1 ] = D3DXVECTOR3( ptBBox->vMax.fX, ptBBox->vMin.fY, 1.0f );
	vCorner[ 2 ] = D3DXVECTOR3( ptBBox->vMax.fX, ptBBox->vMax.fY, 1.0f );
	vCorner[ 3 ] = D3DXVECTOR3( ptBBox->vMin.fX, ptBBox->vMax.fY, 1.0f );

	return dxC_SetClipPlaneRect( vCorner, 4, pmatProj, bCommit );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_GetFullScissorRect( E_RECT & tFullRect )
{
	int nWidth, nHeight;
	sGetFullWidthHeight( nWidth, nHeight );

	tFullRect.top    = (LONG) 0;
	tFullRect.bottom = (LONG) nHeight;
	tFullRect.left   = (LONG) 0;
	tFullRect.right  = (LONG) nWidth;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetScissorRect ( const RECT * ptRect, UINT index )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( index == 0 );
	RECT tRect;
	RECT tFullRect;
	BOOL bFullScreen;

    if ( ! ptRect )
	{
		bFullScreen = TRUE;
	}
	else
	{
		tRect.left   = min( ptRect->left, ptRect->right );
		tRect.right  = max( ptRect->left, ptRect->right );
		tRect.top    = min( ptRect->top, ptRect->bottom );
		tRect.bottom = max( ptRect->top, ptRect->bottom );
		bFullScreen = FALSE;
	}

	int nWidth, nHeight;
	sGetFullWidthHeight( nWidth, nHeight );

	tFullRect.top    = (LONG) 0;
	tFullRect.bottom = (LONG) nHeight;
	tFullRect.left   = (LONG) 0;
	tFullRect.right  = (LONG) nWidth;

	if ( !e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) || bFullScreen == TRUE )
		tRect = tFullRect;

	//tRect.top    = min( tRect.top,  tRect.bottom );
	//tRect.left   = min( tRect.left, tRect.right  );
	//tRect.bottom = max( 0,          tRect.bottom );
	//tRect.right  = min( 0,          tRect.right  );

	// stop-gap scissor fixes:
	tRect.left   = max( tRect.left,   tFullRect.left );
	tRect.top    = max( tRect.top,	  tFullRect.top );
	tRect.right  = max( tRect.right,  tRect.left );
	tRect.bottom = max( tRect.bottom, tRect.top );
	tRect.right  = min( tRect.right,  tFullRect.right );
	tRect.bottom = min( tRect.bottom, tFullRect.bottom );

	ASSERT_RETFAIL( tRect.top <= tRect.bottom );
	ASSERT_RETFAIL( tRect.left <= tRect.right );
	ASSERT_RETFAIL( tRect.bottom >= 0 );
	ASSERT_RETFAIL( tRect.right >= 0 );

	DX9_BLOCK( V_RETURN( dxC_GetDevice()->SetScissorRect( &tRect ) ); )
	DX10_BLOCK( dxC_GetDevice()->RSSetScissorRects( 1, &tRect ); )  //Could handle more than 1 in DX10

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static D3DC_VIEWPORT gtViewport;

PRESULT dxC_SetViewport( 
	DWORD dwULX, 
	DWORD dwULY,
	DWORD dwWidth,
	DWORD dwHeight,
	float fMinZ,
	float fMaxZ )
{
	D3DC_VIEWPORT tVP;
	tVP.TopLeftX	= dwULX;
	tVP.TopLeftY	= dwULY;
	tVP.Width		= dwWidth;
	tVP.Height		= dwHeight;
	tVP.MinDepth	= fMinZ;
	tVP.MaxDepth	= fMaxZ;

	return dxC_SetViewport( tVP );
}

PRESULT dxC_SetViewport( const D3DC_VIEWPORT& tVP )
{
	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoSetViewport( &tVP ) );
	}
	else
	{
		DX9_BLOCK( V_RETURN( dxC_GetDevice()->SetViewport( &tVP ) ); )
#if defined( ENGINE_TARGET_DX10 ) 
#	if defined( DX10_USE_MRTS )
		D3DC_VIEWPORT tViewports[] = { tVP, tVP, tVP };
		dxC_GetDevice()->RSSetViewports( ARRAYSIZE( tViewports ), tViewports );
#	else
		dxC_GetDevice()->RSSetViewports( 1, &tVP );
#	endif
#endif
	}

	// Save off a copy.
	gtViewport = tVP;

	return S_OK;
}

PRESULT dxC_GetViewport( D3DC_VIEWPORT& tVP )
{
	tVP = gtViewport;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetBones(
	const D3D_MODEL * pModel,
	int nMesh,
	int nLOD,	// CHB 2006.11.30
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique ) 
{
	ASSERT_RETINVALIDARG( pEffect );

	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, EFFECT_PARAM_BONES );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
		{
			float * pfBones = e_ModelGetBones ( pModel->nId, nMesh, nLOD );	// CHB 2006.11.30
			if (!pfBones)
			{
				return S_FALSE;
			}
			V_RETURN( dx9_EffectSetRawValue( pEffect, tTechnique, EFFECT_PARAM_BONES, pfBones, 0, 3 * 4 * sizeof(float) * e_AnimatedModelGetMaxBonesPerShader() ) );

#ifdef ENGINE_TARGET_DX10
			BOOL bGetPrevious = TRUE;
			GAME* pGame = AppGetCltGame();
			if ( ! pGame )
				bGetPrevious = FALSE;
			else
			{
				UNIT* pPlayer = GameGetControlUnit( pGame );
				if ( ! pPlayer )
					bGetPrevious = FALSE;
				else
				{
					int nPlayerModelId = c_UnitGetModelId( pPlayer );
					bGetPrevious = pModel->nId != nPlayerModelId;
				}
			}

			pfBones = e_ModelGetBones ( pModel->nId, nMesh, nLOD, bGetPrevious );	// AE 2007.07.10
			if (!pfBones)
			{
				return S_FALSE;
			}
			V_RETURN( dx9_EffectSetRawValue( pEffect, tTechnique, EFFECT_PARAM_BONES_PREV, pfBones, 0, 3 * 4 * sizeof(float) * e_AnimatedModelGetMaxBonesPerShader() ) );
#endif
		}
	} 

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_SetBonesEx(
	const D3D_MODEL * pModel,
	int nMesh,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const float * pfBones ) 
{
	if (!pfBones)
	{
		return S_FALSE;
	}

	ASSERT_RETINVALIDARG( pEffect );
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, EFFECT_PARAM_BONES );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
		{
			V_RETURN( dx9_EffectSetRawValue( pEffect, tTechnique, EFFECT_PARAM_BONES, pfBones, 0, 3 * 4 * sizeof(float) * e_AnimatedModelGetMaxBonesPerShader() ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetTexture(
	D3D_TEXTURE * pTexture,
	const D3D_EFFECT * pEffect,
	D3DC_EFFECT_VARIABLE_HANDLE hParam,
	int nStage,
	BOOL bAllowDbgMipOverride )
{
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETINVALIDARG( ( dxC_IsPixomaticActive() && pTexture->ptSysmemTexture ) || pTexture->pD3DTexture );

	int nReplacement = INVALID_ID;

	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoSetTexture( (TEXTURE_TYPE)pTexture->nType, pTexture ) );
	}
	else
	{
		LPD3DCBASETEXTURE pBaseTexture = pTexture->pD3DTexture;

#ifdef ENGINE_TARGET_DX10
		LPD3DCSHADERRESOURCEVIEW pSRV = pTexture->GetShaderResourceView();
#endif


		// Throttle the number of "new" textures we set to prevent render stalls.
		V( dxC_TextureShouldSet( pTexture, nReplacement ) );
		if ( nReplacement != INVALID_ID )
		{
			ONCE
			{
				D3D_TEXTURE * pReplacementTexture = dxC_TextureGet( nReplacement );
				ASSERT_BREAK( pReplacementTexture );
				pBaseTexture = pReplacementTexture->pD3DTexture;
#ifdef ENGINE_TARGET_DX10
				pSRV = pReplacementTexture->GetShaderResourceView();
#endif
			}
		}


		if ( bAllowDbgMipOverride && e_GetRenderFlag( RENDER_FLAG_SHOW_MIP_LEVELS ) )
		{
			V( dx9_GetDebugMipTextureOverride( pTexture, &pBaseTexture ) );
		}


		V_RETURN( dx9_VerifyValidTextureSet( pBaseTexture ) );
		if ( TRUE ) 
		{
			ASSERT_RETINVALIDARG( pEffect );
			ASSERT_RETINVALIDARG( hParam );
			V_RETURN( dx9_EffectSetTexture( pEffect, hParam, DX9_BLOCK( pBaseTexture ) DX10_BLOCK( pSRV ) ) );
		} 
		else if ( nStage >= 0 ) 
		{
			V_RETURN( dxC_SetTexture( nStage, DX9_BLOCK( pBaseTexture ) DX10_BLOCK( pSRV ) ) );
		}
	}

	if ( nReplacement == INVALID_ID )
	{
		V( dxC_TextureDidSet( pTexture ) );
		V( dx9_ReportTextureUse( pTexture ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetTexture ( 
	int nTextureId, 
	const EFFECT_TECHNIQUE & tTechnique,
	const D3D_EFFECT * pEffect, 
	EFFECT_PARAM eParam, 
	int nStage, 
	const D3D_MODEL_DEFINITION * pModelDef /*= NULL*/,
	const MATERIAL * pMaterial /*= NULL*/,
	TEXTURE_TYPE eDefaultType /*= TEXTURE_NONE*/,
	BOOL bAllowDbgMipOverride /*= FALSE*/ )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( ! dxC_IsPixomaticActive() )
	{
		if ( ! ( EFFECT_INTERFACE_ISVALID( hParam ) || ( ! TRUE && nStage >= 0 ) ) )
			return S_FALSE;
	}

	if ( nTextureId == INVALID_ID )
	{
		// if the hParam is valid and we get here, try to set the default texture for this type

		int nDefTexID;
		if ( AppIsTugboat() 
			 && ( eDefaultType == TEXTURE_LIGHTMAP )
			 && ( pEffect->dwFlags & EFFECT_FLAG_FORCE_LIGHTMAP ) )
		{
			// Mythos relies on a specific fallback lightmap color
			nDefTexID = e_GetUtilityTexture( TEXTURE_RGBA_7F );
		}
		else
		{
			nDefTexID = e_GetDefaultTexture( eDefaultType );
		}


		if ( nDefTexID != TEXTURE_NONE )
		{
			D3D_TEXTURE * pDefTex = dxC_TextureGet( nDefTexID );
			ASSERTV_RETFAIL( pDefTex && ( ( dxC_IsPixomaticActive() && pDefTex->ptSysmemTexture ) || pDefTex->pD3DTexture ), "Default texture missing!\n\n%s", e_GetTextureTypeName( eDefaultType ) );
			V_RETURN( sSetTexture( pDefTex, pEffect, hParam, nStage, bAllowDbgMipOverride ) );
		}
		return S_FALSE;
	} 
	else 
	{
		D3D_TEXTURE* pTexture = dxC_TextureGet( nTextureId );
		if (!pTexture)
		{
			static BOOL bAsserted = FALSE;
			if ( ! bAsserted )
			{
				ASSERTV_MSG( "Error: texture mysteriously disappeared!\n\nID: %d\n\nName: %s\n\nMaterial Name: %s\n\nParam: %s\n\nModel: %s",
					nTextureId,
					e_GetTextureTypeName( eDefaultType ),
					pMaterial ? pMaterial->tHeader.pszName : "",
					gptEffectParamChart[ eParam ].pszName,
					pModelDef->tHeader.pszName );
				bAsserted = TRUE;
			}
		}

		ASSERTONCE_RETVAL(pTexture, E_FAIL);
		ASSERT_RETFAIL( dxC_IsPixomaticActive() || ( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM ) == 0 );

		BOOL bTextureReady = ( dxC_IsPixomaticActive() && pTexture->ptSysmemTexture ) || pTexture->pD3DTexture;

		if ( bTextureReady && ! e_GetDefaultTextureOverride( (TEXTURE_TYPE) pTexture->nType ) ) 
		{
			// Common case.  The texture is present and no override has been requested.
			V_RETURN( sSetTexture( pTexture, pEffect, hParam, nStage, bAllowDbgMipOverride ) );
		} else
		{
			D3D_TEXTURE * pShowTex = NULL;
			if ( ! bTextureReady && pTexture->nDefinitionId != INVALID_ID )
			{
				// if texture and definition loaded, but texture not yet created, create it
				TEXTURE_DEFINITION * pTexDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
				if ( S_OK == dx9_TextureCreateIfReady( pTexture, pTexDef ) )
					pShowTex = pTexture;
			}

			if ( ! pShowTex )
			{
				// set default texture for this texture type
				int nDefTexID = INVALID_ID;

				if ( nDefTexID == INVALID_ID )
					nDefTexID = e_GetDefaultTexture( eDefaultType );

				if ( nDefTexID == INVALID_ID )
					return S_FALSE;

				D3D_TEXTURE * pDefTex = dxC_TextureGet( nDefTexID );
				ASSERT_RETFAIL( pDefTex );
				pShowTex = pDefTex;
			}

			V_RETURN( sSetTexture( pShowTex, pEffect, hParam, nStage, bAllowDbgMipOverride ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

HRESULT dxC_UpdateDebugRenderStates( )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) )
	{
		V_RETURN( dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE ) );
	}

	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoUpdateDebugStates() );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_PushClipPlane( const D3DXVECTOR3 * pVec1, const D3DXVECTOR3 * pVec2, const D3DXVECTOR3 * pVec3 )
{
	if ( gtClipPlanes.nCount >= MAX_CLIP_PLANES )
		return S_FALSE;
	ASSERT_RETINVALIDARG( pVec1 && pVec2 && pVec3 );

	PLANE tPlane;
	PlaneFromPoints( &tPlane, (VECTOR*)pVec1, (VECTOR*)pVec2, (VECTOR*)pVec3 );
	V( dxC_PushClipPlane( &tPlane ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_PushClipPlane( const PLANE * pPlane )
{
	if ( gtClipPlanes.nCount >= MAX_CLIP_PLANES )
		return S_FALSE;
	ASSERT_RETINVALIDARG( pPlane );

	gtClipPlanes.tPlanes[ gtClipPlanes.nCount++ ] = *pPlane;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_PopClipPlane()
{
	if ( gtClipPlanes.nCount <= 0 )
		return S_FALSE;

	gtClipPlanes.nCount--;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_ResetClipPlanes( BOOL bCommit )
{
	gtClipPlanes.nCount = 0;
	gtClipPlanes.dwFlags = 0;
	if ( bCommit )
	{
		V_RETURN( dxC_CommitClipPlanes() );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_CommitClipPlanes()
{
	int nCount = gtClipPlanes.nCount;

	// CHB 2006.10.02 - For now, don't commit clip planes on hardware that doesn't support it.
	// We'll ultimately likely need a solution for portals on 1.1 though.
#if defined(ENGINE_TARGET_DX9)
	{
		int nMax = dx9_CapGetValue(DX9CAP_MAX_USER_CLIP_PLANES);
		nCount = min(nCount, nMax);
	}
#endif

	// set the clip planes on the d3d device

	gtClipPlanes.dwFlags = 0;
	
#if defined(ENGINE_TARGET_DX9)
	// CHB 2006.10.02
#if 1
	ASSERT(nCount <= 6);
	nCount = min(nCount, 6);
	C_ASSERT(D3DCLIPPLANE0 == (1 << 0));
	C_ASSERT(D3DCLIPPLANE1 == (1 << 1));
	C_ASSERT(D3DCLIPPLANE2 == (1 << 2));
	C_ASSERT(D3DCLIPPLANE3 == (1 << 3));
	C_ASSERT(D3DCLIPPLANE4 == (1 << 4));
	C_ASSERT(D3DCLIPPLANE5 == (1 << 5));
	gtClipPlanes.dwFlags = (1 << nCount) - 1;
#else
	switch ( nCount )
	{
	case 6:	gtClipPlanes.dwFlags |= D3DCLIPPLANE5;
	case 5:	gtClipPlanes.dwFlags |= D3DCLIPPLANE4;
	case 4:	gtClipPlanes.dwFlags |= D3DCLIPPLANE3;
	case 3:	gtClipPlanes.dwFlags |= D3DCLIPPLANE2;
	case 2:	gtClipPlanes.dwFlags |= D3DCLIPPLANE1;
	case 1:	gtClipPlanes.dwFlags |= D3DCLIPPLANE0;
	}
#endif

		for ( int i = 0; i < nCount; i++ )
		{
			V_RETURN( dxC_GetDevice()->SetClipPlane( i, (float*)&gtClipPlanes.tPlanes[ i ] ) );
		}

		V_RETURN( dxC_SetRenderState( D3DRS_CLIPPLANEENABLE, gtClipPlanes.dwFlags ) );
#endif // ENGINE_TARGET_DX9
	DX10_BLOCK
	(
		;//KMNV TODO: Need to implement
	)

	if ( !e_GetRenderFlag( RENDER_FLAG_CLIP_ENABLED ) )
	{
		gtClipPlanes.dwFlags = 0;
	}

	return S_OK;
}	

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static LPD3DCIALAYOUT sGetVertexDeclaration( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect )
{
	if ( eType == VERTEX_DECL_INVALID )
		return NULL;
	if ( pEffect )
		eType = sResolveVertDeclType( eType, pEffect );

	ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_VERTEX_DECL_TYPES ) );

	VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ eType ];
	ASSERT( pTypeData );

	return pTypeData->pVertexDeclaration;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

static PIXO_VERTEX_DECL sGetPixoVertexDecl( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect )
{
	if ( eType == VERTEX_DECL_INVALID )
		return NULL;
	if ( pEffect )
		eType = sResolveVertDeclType( eType, pEffect );

	ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_VERTEX_DECL_TYPES ) );

	VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ eType ];
	ASSERT( pTypeData );

	return pTypeData->pPixoVertexDecl;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetVertexDeclaration( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect )
{
#ifdef ENGINE_TARGET_DX9
	if ( dxC_IsPixomaticActive() )
	{
		return dxC_PixoSetStreamDefinition( sGetPixoVertexDecl( eType, pEffect ) );
	}

	LPD3DCIALAYOUT pIALayout = sGetVertexDeclaration( eType, pEffect );
	if( !pIALayout )
		return E_FAIL;

	TRY_STATE_MANAGER( gpStateManager, SetVertexDeclaration( pIALayout ) );
#elif defined(ENGINE_TARGET_DX10)
	TRY_STATE_MANAGER( gpStateManager, SetInputLayoutType( eType )  );
#endif
	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const D3DC_INPUT_ELEMENT_DESC * dxC_GetVertexElements( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect )
{
	if ( eType == VERTEX_DECL_INVALID )
		return NULL;
	if ( pEffect )
		eType = sResolveVertDeclType( eType, pEffect );
	ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_VERTEX_DECL_TYPES ) );
	VERTEX_TYPE_DATA * pTypeData = &gpVertexTypeData[ eType ];
	return pTypeData->pVertexElements;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

UINT dxC_GetVertexSize( int nStream, const D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef, const struct D3D_EFFECT * pEffect )
{
	ASSERT_RETZERO( pVertexBufferDef );
	ASSERT_RETZERO( nStream < DXC_MAX_VERTEX_STREAMS );
	int nVertexSize = 0;

	VERTEX_DECL_TYPE eType = pVertexBufferDef->eVertexType;
	D3DC_FVF dwFVF = pVertexBufferDef->dwFVF;

	if ( eType != VERTEX_DECL_INVALID )
	{
		if ( pEffect )
			eType = sResolveVertDeclType( eType, pEffect );

		ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_VERTEX_DECL_TYPES ) );
		VERTEX_TYPE_DATA* pTypeData = &gpVertexTypeData[ eType ];
		ASSERT_RETZERO( nStream < pTypeData->nStreamCount );

		if ( nStream < 0 )
		{
			// Return total size
			int nVertexSize = 0;
			for ( int i = 0; i < DXC_MAX_VERTEX_STREAMS; i++ )
				nVertexSize += pTypeData->nVertexSize[ i ];
		}
		else
			nVertexSize = pTypeData->nVertexSize[ nStream ];		
	}
	else if ( dwFVF != D3DC_FVF_NULL )
	{
		DX9_BLOCK( nVertexSize = D3DXGetFVFVertexSize( dwFVF ); )
	}
	else
	{
		DX10_BLOCK( nVertexSize = pVertexBufferDef->nBufferSize[ nStream ] / pVertexBufferDef->nVertexCount; )
	}
	
	return nVertexSize;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

UINT dxC_GetStreamCount( const D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef, const struct D3D_EFFECT* pEffect )
{
	ASSERT_RETZERO( pVertexBufferDef );

	int nStreamCount = 0;

	VERTEX_DECL_TYPE eType = pVertexBufferDef->eVertexType;
	D3DC_FVF dwFVF = pVertexBufferDef->dwFVF;

	if ( pEffect )
		eType = sResolveVertDeclType( eType, pEffect );

	if ( eType != VERTEX_DECL_INVALID )
	{
		ASSERT_RETNULL( IS_VALID_INDEX( eType, NUM_VERTEX_DECL_TYPES ) );
		VERTEX_TYPE_DATA* pTypeData = &gpVertexTypeData[ eType ];

		nStreamCount = pTypeData->nStreamCount;
	}
	else if ( dwFVF != D3DC_FVF_NULL )
	{
		DX9_BLOCK( nStreamCount = 1; )
	}
	else
	{
		DX10_BLOCK(	nStreamCount = 1; )
	}
	
	return nStreamCount;
}
