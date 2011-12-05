//----------------------------------------------------------------------------
// dxC_effect.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "definition_common.h"
#include "appcommontimer.h"
#include "excel.h"
#include "maths.h"
#include "array.h"

#include "dxC_caps.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_model.h"
#include "dxC_drawlist.h"
#include "dxC_profile.h"
#include "dxC_target.h"
#include "dxC_anim_model.h"
#include "e_texture_priv.h"
#include "performance.h"
#include "filepaths.h"
#include "pakfile.h"
#include "perfhier.h"
#include "dxC_effect.h"
#include "dxC_render.h"
#include "dxC_environment.h"
#include "dxC_pixo.h"

#include "e_main.h"
#include "e_shadow.h"
#include "dxC_material.h"
#include "dxC_irradiance.h"
#include "dxC_texture.h"
#include "e_debug.h"
#include "e_hdrange.h"
#include "e_optionstate.h"	// CHB 2007.03.14
#include "fillpak.h"
#include "dx9_shadowtypeconstants.h"	// CHB 2006.11.02
#include "e_automap.h"
#include "e_gamma.h"		// CHB 2007.10.02
#include "dxC_effectssharedconstants.h"

#ifdef ENGINE_TARGET_DX10
#include "dx10_effect.h"
#endif

using namespace FSSE;

//--------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------

#define MODEL_SILHOUETTE_DISTANCE		70.0f

#define EFFECT_LOD_TRANSITION_START		0.5f

//#define TRACE_EFFECT_TIMERS					// general effect load timers
//#define TRACE_EFFECT_RESTORE_TIMERS			// detailed effect restore timers

//--------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// ENUMS
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------------

CHash<D3D_EFFECT>			gtEffects;
int							gnNextEffectID = 1;		// 0 skipped to detect zeroed memory

SPD3DCEFFECTPOOL			gpEffectPool = NULL;

DWORD						gdwEffectBeginFlags = DXC_9_10( D3DXFX_DONOTSAVESTATE, NULL);  //KMNV TODO: Effect begin flags
BOOL						gbNeedShaderCache	= FALSE;

SPD3DCVB		gpAnimatedTriangleVertexBuffer;
SPD3DCVB		gpRigidTriangleVertexBuffer;

int gpnNonMaterialEffects[ NUM_NONMATERIAL_EFFECTS ] = {-1};

// These must be a one-to-one mapping with the param enum
const EFFECT_PARAM_CHART gptEffectParamChart[ NUM_EFFECT_PARAMS ] =
{
#	include "dxC_effect_def.h"
};

#define INCLUDE_TECHFEAT_INT
const TECHNIQUE_INT_CHART gtTechniqueIntChart[ NUM_TECHNIQUE_INT_CHARTS ] =
{
#	include "dxC_effect_feature_def.h"
};
#undef INCLUDE_TECHFEAT_INT


#define INCLUDE_TECHFEAT_FLAG
const TECHNIQUE_FLAG_CHART gtTechniqueFlagChart[] =
{
#	include "dxC_effect_feature_def.h"
};
const int NUM_TECHNIQUE_FLAG_CHARTS = arrsize(gtTechniqueFlagChart);
#undef INCLUDE_TECHFEAT_FLAG


static int sgnDebugTechniqueCacheHits				= 0;
static int sgnDebugTechniqueCompares				= 0;
static int sgnDebugEffectCacheHits					= 0;
static int sgnDebugEffectCompares					= 0;
static int sgnDebugEffectPassCacheHits				= 0;
static int sgnDebugEffectPassCompares				= 0;
static int sgnDebugEffectPassForcedBegins			= 0;
static LPD3DCEFFECT sgpCurrentEffect				= NULL;
static D3DC_TECHNIQUE_HANDLE sghCurrentTechnique	= NULL;
static int sgnCurrentNumPasses						= -1;
static int sgnCurrentPass							= -1;


int gnTechniqueSearchCacheSentinel					= 1; // can increment this to force all technique caches to flush

static TECHNIQUE_STATS	sgtTechniqueStats;


static int sgnValidateTechniques = 0;

#ifdef ENGINE_TARGET_DX9
	static SPDIRECT3DVERTEXSHADER9 sgpDebugVertexShaders[ NUM_DEBUG_VERTEX_SHADERS ];
	static SPDIRECT3DPIXELSHADER9  sgpDebugPixelShaders [ NUM_DEBUG_PIXEL_SHADERS ];
#endif


#if ISVERSION(DEVELOPMENT)
struct TECHNIQUE_REPORT
{
	DWORD dwSelected;
	int nIndex;
};
static resizable_array<TECHNIQUE_REPORT>	gptTechniqueReport;

struct EFFECT_STATS
{
	DWORD			dwShaderTypes[ NUM_SHADER_TYPES ];
	EFFECT_TARGET	eMaxFXTgt;
};
static EFFECT_STATS sgtEffectStats = { {0}, FXTGT_INVALID };
#endif // DEVELOPMENT


#ifdef ENGINE_TARGET_DX10

//we essentially assume every handle is a 4x4 float matrix
#define DATA_CACHE_SIZE 64 
BYTE fDataCache[ NUM_EFFECT_PARAMS * DATA_CACHE_SIZE ] = {0}; 
int nList[NUM_EFFECT_PARAMS] = {0};

static bool sVariableNeedsUpdate( EFFECT_PARAM eParam, const void * pData, UINT nSize )
{
	bool bForce = false;

	// CHB 2007.09.14 - This is constant, so we can safely return without updating the cache.
	if( c_GetToolMode() )  //Issues in hammer with constant caching, should revisit
		return true;

	// CHB 2007.09.14 - This is constant, so we can safely return without updating the cache.
//	if( nSize > DATA_CACHE_SIZE || eParam > NUM_EFFECT_PARAMS )  //64 bytes max
	if( eParam >= NUM_EFFECT_PARAMS )  //64 bytes max
		return true;

	// CHB 2007.09.14 - This is constant, so we can safely return without updating the cache.
	if( !dx10_IsParamGlobal( eParam ) )
		return true;

	// CHB 2007.09.14 - Although the param is 'dirty', we still need to update the local cache.
	if( dx10_IsParamDirty( eParam ) )
		bForce = true;

	// CHB 2007.09.14 - This is not constant, so we need to update the cache.
	// The size can change across calls, like for the array of point light
	// colors, which depends on the number of point lights. While it's true
	// we need to update for all cases where the size is greater than the
	// cache, we still need to update the cache should the next call have a
	// size that fits.
	if( nSize > DATA_CACHE_SIZE )  //64 bytes max
	{
		bForce = true;
	}
	nSize = min(nSize, DATA_CACHE_SIZE);

	BYTE* pCache = &fDataCache[eParam * DATA_CACHE_SIZE];
	if ((!bForce) && ( memcmp( pCache, pData, nSize ) == 0 ))
	{
		return false;
	}

	dx10_DirtyParam( eParam );  //Dirty param, won't be clean until we actually update
	memcpy( pCache, pData, nSize );
	return true;
}
#endif

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_PoolSetMatrix( EFFECT_PARAM eParam, const D3DXMATRIXA16* pMatrix )
{
	if( !sVariableNeedsUpdate( eParam, (BYTE*)pMatrix, sizeof( D3DXMATRIXA16 ) ) )
		return S_OK;

	return dx10_GetGlobalHandle( eParam )->AsMatrix()->SetMatrix( (float*)&pMatrix );
}
#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sUpdateTechniqueSearchCacheSentinel()
{
	gnTechniqueSearchCacheSentinel++;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGetFullShaderPath( TCHAR * fullname, const TCHAR * filename, EFFECT_FOLDER eFolder, EFFECT_SUBFOLDER eSubFolder, const char * pszEngineTgt )
{
	char pszOriginalFileNameWithPath [ DEFAULT_FILE_WITH_PATH_SIZE ];
	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];

	const char * pszPath = NULL;
	switch (eFolder)
	{
	case EFFECT_FOLDER_COMMON:
		pszPath = FILE_PATH_EFFECT_COMMON; break;
	case EFFECT_FOLDER_TUGBOAT:
	case EFFECT_FOLDER_HELLGATE:
		pszPath = FILE_PATH_EFFECT; break;
	default:
		ASSERTX_RETFAIL( 0, "Bad folder passed in to sGetFullShaderPath!" );
	}
	PStrCopy( pszOriginalFileNameWithPath, pszPath, DEFAULT_FILE_WITH_PATH_SIZE );	

	PStrForceBackslash( pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
	PStrPrintf( szPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszOriginalFileNameWithPath, pszEngineTgt );
	PStrForceBackslash( szPath, DEFAULT_FILE_WITH_PATH_SIZE );

	// CHB 2007.01.02 - PS 1.x shaders split out due to compiler change.
#ifdef ENGINE_TARGET_DX9
	// CHB 2007.05.07 - Get correct subdirectory based on shader version setting.
//	if (dx9_CapGetValue(DX9CAP_MAX_PS_VERSION) < D3DPS_VERSION(2, 0))
	if (dxC_GetCurrentMaxEffectTarget() == FXTGT_SM_11 || eSubFolder == EFFECT_SUBFOLDER_1X )
	{
		PStrCat(szPath, "1x\\", _countof(szPath));
	}
#endif

	PStrPrintf( pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, filename );

	FileGetFullFileName(fullname, pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE);

	return S_OK;
}

PRESULT dxC_GetFullShaderPath(	TCHAR * fullname, const TCHAR * filename, EFFECT_FOLDER eFolder, EFFECT_SUBFOLDER eSubFolder )
{
	V_RETURN( sGetFullShaderPath( fullname, filename, eFolder, eSubFolder, ENGINE_TARGET_FOLDER ) );

#if 0 && defined( ENGINE_TARGET_DX10 )
	if( !FileExists( fullname ) )  //This wasn't a DX10 effect so go for DX9
	{
		V_RETURN( sGetFullShaderPath( fullname, filename, eFolder, eSubFolder, "Dx9", bFromSource ) );
	}
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
// Helpers
//-------------------------------------------------------------------------------------------------
PRESULT dxC_EffectGetTechniqueDesc( LPD3DCEFFECT pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, D3DC_TECHNIQUE_DESC* pDesc)
{
	DX9_BLOCK( return pEffect->GetTechniqueDesc( hTechnique, pDesc ); )
	DX10_BLOCK( return hTechnique->GetDesc( pDesc ); )
}

PRESULT dxC_EffectGetTechniqueDesc( const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, D3DC_TECHNIQUE_DESC* pDesc)
{
	return dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTechnique, pDesc );
}

D3DC_TECHNIQUE_HANDLE dxC_EffectGetTechniqueByIndex( LPD3DCEFFECT pEffect, UINT iIndex )
{
	DX9_BLOCK( return pEffect->GetTechnique( iIndex ); )
	DX10_BLOCK( return pEffect->GetTechniqueByIndex( iIndex ); )
}

PRESULT dxC_EffectGetPassDesc( LPD3DCEFFECT pEffect, D3DC_EFFECT_PASS_HANDLE hPass, D3DC_PASS_DESC* pDesc)
{
	DX9_BLOCK( return pEffect->GetPassDesc( hPass, pDesc ); )
	DX10_BLOCK( return hPass->GetDesc( pDesc ); )
}

PRESULT dxC_EffectGetPassDesc( const D3D_EFFECT * pEffect, D3DC_EFFECT_PASS_HANDLE hPass, D3DC_PASS_DESC* pDesc)
{
	return dxC_EffectGetPassDesc( pEffect->pD3DEffect, hPass, pDesc );
}


D3DC_EFFECT_PASS_HANDLE dxC_EffectGetPassByIndex( LPD3DCEFFECT pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, UINT iIndex)
{
	DX9_BLOCK( return pEffect->GetPass( hTechnique, iIndex ) ; )
	DX10_BLOCK( return hTechnique->GetPassByIndex( iIndex) ; )
}

PRESULT dxC_CreateEffectFromFile(
	LPCTSTR pSrcFile,
	LPD3DCEFFECT * ppEffect,
	LPD3DCBLOB * ppCompilationErrors )
{
	ASSERTX_RETINVALIDARG( ppEffect, "No LPD3DCEFFECT!");
	ASSERTX_RETFAIL( gpEffectPool, "Where's the effect pool?!" );

#if defined(ENGINE_TARGET_DX9)	
	V_RETURN( D3DXCreateEffectFromFile(dxC_GetDevice(),
		pSrcFile,
		NULL, // CONST D3DC_SHADER_MACRO* pDefines,   
		NULL, // LPD3DXINCLUDE pInclude,
		NULL,
		gpEffectPool,
		ppEffect,
		ppCompilationErrors ) );

#elif defined(ENGINE_TARGET_DX10)

	extern D3D10_SHADER_MACRO g_pFX10Defines[];
	V_DO_FAILED( D3DX10CreateEffectFromFile(
		pSrcFile,
		g_pFX10Defines,
		NULL, 
		NULL,
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, //HLSL Flags: Don't need any because compilation is offline
		D3D10_EFFECT_COMPILE_CHILD_EFFECT,
		dxC_GetDevice(),
		gpEffectPool,
		NULL,  //KMNV TODO THREADPUMP
		ppEffect,
		ppCompilationErrors, 
		NULL ) )
	{
#ifdef FX10_ATTEMPT_BACKWARDS_COMPATIBLITLY
		V_RETURN( D3DX10CreateEffectFromFile(
			pSrcFile,
			g_pFX10Defines,
			NULL, 
			NULL,
			NULL,
			D3D10_EFFECT_COMPILE_CHILD_EFFECT,
			dxC_GetDevice(),
			gpEffectPool,
			NULL,  //KMNV TODO THREADPUMP
			ppEffect,
			ppCompilationErrors,
			NULL ) );
#endif
	}
#endif
	return S_OK;
}

PRESULT dxC_CompileEffectFromFile(
	LPCTSTR pSrcFile,
	LPD3DCBLOB* ppCompiledEffect,
	LPD3DCBLOB* ppCompilationErrors )
{
	ASSERTX_RETINVALIDARG( ppCompiledEffect, "No LPD3DCBLOB!");

#if defined(ENGINE_TARGET_DX9)
	return E_FAIL;
#elif defined(ENGINE_TARGET_DX10)
	extern D3D10_SHADER_MACRO g_pFX10Defines[];
	V_DO_FAILED( D3DX10CompileFromFile( 
		pSrcFile,
		g_pFX10Defines,
		NULL,
		NULL,
		NULL,
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY,
		D3D10_EFFECT_COMPILE_CHILD_EFFECT,
		NULL,  //KMNV TODO THREADPUMP
		ppCompiledEffect,
		ppCompilationErrors,
		NULL ) )
	{
#ifdef FX10_ATTEMPT_BACKWARDS_COMPATIBLITLY
		V_RETURN( D3DX10CompileFromFile( 
			pSrcFile,
			g_pFX10Defines,
			NULL,
			NULL,
			NULL,
			NULL,
			D3D10_EFFECT_COMPILE_CHILD_EFFECT,
			NULL,  //KMNV TODO THREADPUMP
			ppCompiledEffect,
			ppCompilationErrors,
			NULL ) );
#endif
	}
	return S_OK;
#endif
}

PRESULT dxC_CreateEffectFromFileInMemory(
	void * pFileData,
	int nDataLen,
	LPCTSTR pFilename,
	LPD3DCEFFECT * ppEffect,
	LPD3DCBLOB * ppCompilationErrors )
{
	ASSERTX_RETINVALIDARG( ppEffect, "No LPD3DCEFFECT!");
	ASSERTX_RETFAIL( gpEffectPool, "Where's the effect pool?!" );

#if defined(ENGINE_TARGET_DX9)
	V_RETURN( D3DXCreateEffect(
		dxC_GetDevice(),
		pFileData,
		nDataLen,
		NULL, // CONST D3DC_SHADER_MACRO* pDefines,   
		NULL, // LPD3DXINCLUDE pInclude,
		NULL,
		gpEffectPool,
		ppEffect,
		ppCompilationErrors ) );

#elif defined(ENGINE_TARGET_DX10)
	extern D3D10_SHADER_MACRO g_pFX10Defines[];

	V_DO_FAILED( D3DX10CreateEffectFromMemory(
		pFileData, 
		nDataLen,
		pFilename,
		g_pFX10Defines,
		NULL, 
		NULL,
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, //HLSL Flags: Don't need any because compilation is offline
		D3D10_EFFECT_COMPILE_CHILD_EFFECT,
		dxC_GetDevice(),
		gpEffectPool,
		NULL,  //KMNV TODO THREADPUMP
		ppEffect,
		ppCompilationErrors,
		NULL ) )
	{
#ifdef FX10_ATTEMPT_BACKWARDS_COMPATIBLITLY
		V_RETURN( D3DX10CreateEffectFromMemory(
			pFileData, 
			nDataLen,
			pFilename,
			g_pFX10Defines, 
			NULL, 
			NULL,
			NULL,
			D3D10_EFFECT_COMPILE_CHILD_EFFECT,
			dxC_GetDevice(),
			gpEffectPool,
			NULL,  //KMNV TODO THREADPUMP
			ppEffect,
			ppCompilationErrors, 
			NULL ) );
#endif
	}
#endif
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sEffectFileError( const char * pszFileName, const char * pszErrors )
{
	ASSERTV_MSG( "Error with effect file: %s :\n%s", pszFileName, pszErrors ? pszErrors : "" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sEffectFileError( const char * pszFileName, const LPD3DCBLOB pErrorBuffer )
{
	char * pszErrors = NULL;
	if ( pErrorBuffer )
	{
		pszErrors = (char *) pErrorBuffer->GetBufferPointer();
	}
	sEffectFileError( pszFileName, pszErrors );
}

//----------------------------------------------------------------------------
// these are the glue functions defining the effect_target to shader version relationship
//----------------------------------------------------------------------------

EFFECT_TARGET dxC_GetEffectTargetFromShaderVersions( VERTEX_SHADER_VERSION tVS, PIXEL_SHADER_VERSION tPS )
{
	if ( tVS >= VS_4_0 &&
		 tPS >= PS_4_0 )
		return FXTGT_SM_40;

	if ( tVS >= VS_3_0 &&
		 tPS >= PS_3_0 )
		return FXTGT_SM_30;

	if ( tVS >= VS_2_0 &&
		 tPS >= PS_2_0 )
	{
		if ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID )
			return FXTGT_SM_20_LOW;
		return FXTGT_SM_20_HIGH;
	}

	if ( tVS >= VS_1_1 &&
		 tPS >= PS_1_1 )
		return FXTGT_SM_11;

	return FXTGT_FIXED_FUNC;
}

void dxC_GetShaderVersionsFromEffectTarget( EFFECT_TARGET eTarget, VERTEX_SHADER_VERSION & tVS, PIXEL_SHADER_VERSION & tPS )
{
	switch( eTarget )
	{
	case FXTGT_SM_40:		tVS = VS_4_0;
							tPS = PS_4_0; return;
	case FXTGT_SM_30:		tVS = VS_3_0;
							tPS = PS_3_0; return;
	case FXTGT_SM_20_LOW:	tVS = VS_2_a;
							tPS = PS_2_a; return;
	case FXTGT_SM_20_HIGH:	tVS = VS_2_a;
							tPS = PS_2_a; return;
	case FXTGT_SM_11:		tVS = VS_1_1;
							tPS = PS_1_1; return;
	case FXTGT_FIXED_FUNC:	tVS = VS_NONE;
							tPS = PS_NONE; return;
	default:
		ASSERTV_MSG( "Invalid effect target: %d", eTarget );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_GetCurrentMaxShaderVersions( VERTEX_SHADER_VERSION & eVS, PIXEL_SHADER_VERSION & ePS )
{
	const OptionState * pState = e_OptionState_GetActive();
	EFFECT_TARGET eTarget = (EFFECT_TARGET)pState->get_nMaxEffectTarget();
	dxC_GetShaderVersionsFromEffectTarget( eTarget, eVS, ePS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEffectMeetsCaps(const EFFECT_DEFINITION * pEffectDef )
{
	BOOL bValid = TRUE;

	// if this effect uses static branching, we check caps against the effect reqs
	if ( (DWORD)pEffectDef->nStaticBranchDepthVS > dx9_CapGetValue( DX9CAP_MAX_VS_STATIC_BRANCH ) &&
		 (DWORD)pEffectDef->nStaticBranchDepthPS > dx9_CapGetValue( DX9CAP_MAX_PS_STATIC_BRANCH ) )
		bValid = FALSE;

	return bValid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EFFECT_TARGET dxC_GetCurrentMaxEffectTarget()
{
	const OptionState * pState = e_OptionState_GetActive();
	return (EFFECT_TARGET)pState->get_nMaxEffectTarget();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_GetEffectFromIndex( int nEffectIndexLine, EFFECT_TARGET eEffectTarget )
{
	if ( nEffectIndexLine == INVALID_ID )
		return INVALID_ID;
	const EFFECT_INDEX * pEffectIndex = (const EFFECT_INDEX *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS, nEffectIndexLine ) ;
	ASSERTV_RETINVALID( pEffectIndex, "Couldn't find effect index referenced in line %d", nEffectIndexLine );


	int nEffectFileLine = INVALID_ID;
	ASSERT_RETNULL( IS_VALID_INDEX( eEffectTarget, NUM_EFFECT_TARGETS ) );
	nEffectFileLine = pEffectIndex->pnFXOIndex[ eEffectTarget ];


	if ( nEffectFileLine == INVALID_ID )
		return INVALID_ID;
	const EFFECT_DEFINITION * pEffectDef = (EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectFileLine ) ;
	ASSERTV_RETINVALID( pEffectDef, "Couldn't find effect file referenced in line %d", nEffectFileLine );

	if ( ! sEffectMeetsCaps( pEffectDef ) )
		return INVALID_ID;

	return nEffectFileLine;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_GetBestEffectFromIndex( int nEffectIndexLine )
{
	if ( nEffectIndexLine == INVALID_ID )
		return INVALID_ID;

	EFFECT_TARGET eTarget = dxC_GetCurrentMaxEffectTarget();
	ASSERT_RETNULL( IS_VALID_INDEX( eTarget, NUM_EFFECT_TARGETS ) );

	int nEffectFileLine = INVALID_ID;
	while ( nEffectFileLine == INVALID_ID && eTarget > FXTGT_INVALID )
	{
		nEffectFileLine = dx9_GetEffectFromIndex( nEffectIndexLine, eTarget );
		((int&)eTarget)--;
	}

	return nEffectFileLine;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sLoadShaderType( SHADER_TYPE_DEFINITION * pShaderTypeDef, int nShaderType )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	int nEffectIndexLine = pShaderTypeDef->nEffectIndex[ nShaderType ];
	if ( nEffectIndexLine == INVALID_ID )
		return S_FALSE;

	int nEffectFileLine = dx9_GetBestEffectFromIndex( nEffectIndexLine );

	const EFFECT_INDEX * pEffectIndex = (const EFFECT_INDEX *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS, nEffectIndexLine );
	ASSERT_RETFAIL( pEffectIndex );
	ASSERT_RETFAIL( !pEffectIndex->bRequired || nEffectFileLine != INVALID_ID );
	if ( ! pEffectIndex->bRequired && nEffectFileLine == INVALID_ID )
		return S_FALSE;

	int nEffectID;
	V_RETURN( dx9_EffectNew( &nEffectID, nEffectFileLine ) );
	ASSERT_RETFAIL( nEffectID != INVALID_ID );

	// clean up unused effects
	//if ( nEffectID != pShaderTypeDef->nEffectID[ nShaderType ] && pShaderTypeDef->nEffectID[ nShaderType ] != INVALID_ID )
	//	dx9_EffectRemove( nEffectID );
	pShaderTypeDef->nEffectID[ nShaderType ] = nEffectID;
	pShaderTypeDef->nEffectFile[ nShaderType ] = nEffectFileLine;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_LoadShaderType( const char * pszShaderName, int nShaderType )
{
	ASSERT_RETINVALIDARG( pszShaderName );
	ASSERT_RETINVALIDARG( nShaderType >= 0 && nShaderType < NUM_SHADER_TYPES );

	SHADER_TYPE_DEFINITION * pShaderTypeDef = (SHADER_TYPE_DEFINITION *)ExcelGetDataByStringIndexNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, pszShaderName );
	ASSERT_RETFAIL( pShaderTypeDef );

	return sLoadShaderType( pShaderTypeDef, nShaderType );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_LoadShaderType( int nShaderTypeID, int nShaderType )
{
	ASSERT_RETINVALIDARG( nShaderTypeID != INVALID_ID );
	ASSERT_RETINVALIDARG( nShaderType >= 0 && nShaderType < NUM_SHADER_TYPES );

	SHADER_TYPE_DEFINITION * pShaderTypeDef = (SHADER_TYPE_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, nShaderTypeID );
	ASSERT_RETFAIL( pShaderTypeDef );

	return sLoadShaderType( pShaderTypeDef, nShaderType );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_LoadAllShadersOfType( int nShaderType )
{
	ASSERT_RETINVALIDARG( nShaderType >= 0 && nShaderType < NUM_SHADER_TYPES );

#if ISVERSION(DEVELOPMENT)
	sgtEffectStats.dwShaderTypes[nShaderType]++;
	sgtEffectStats.eMaxFXTgt = dxC_GetCurrentMaxEffectTarget();
#endif // DEVELOPMENT

	unsigned int nRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS);
	for (unsigned int i = 0; i < nRows; i++)
	{
		SHADER_TYPE_DEFINITION * pShaderTypeDef = (SHADER_TYPE_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, i );
		ASSERT_CONTINUE( pShaderTypeDef );

		int nEffectIndex = pShaderTypeDef->nEffectIndex[ nShaderType ];
		pShaderTypeDef->nEffectFile[ nShaderType ] = dx9_GetBestEffectFromIndex( nEffectIndex );

		if ( pShaderTypeDef->nEffectFile[ nShaderType ] != INVALID_ID )
		{
			V_CONTINUE( dx9_EffectNew( &pShaderTypeDef->nEffectID[ nShaderType ], pShaderTypeDef->nEffectFile[ nShaderType ] ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_NotifyEffectsDeviceLost()
{
#ifdef ENGINE_TARGET_DX9

	for ( D3D_EFFECT * pEffect = HashGetFirst( gtEffects );
		pEffect;
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		if ( !pEffect->pD3DEffect )
			continue;
		V( pEffect->pD3DEffect->OnLostDevice() );
	}

	for ( int i = 0; i < NUM_NONMATERIAL_EFFECTS; i++ )
	{
		int nEffectID = dx9_EffectGetNonMaterialID( i );
		if ( nEffectID == INVALID_ID )
			continue;
		D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
		ASSERT_CONTINUE( pEffect );
		if ( ! pEffect->pD3DEffect )
			continue;
		V( pEffect->pD3DEffect->OnLostDevice() );
	}
#endif // DX9

	//No lost device in DX10

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_NotifyEffectsDeviceReset()
{
	sUpdateTechniqueSearchCacheSentinel();

#ifdef ENGINE_TARGET_DX9

	for ( D3D_EFFECT * pEffect = HashGetFirst( gtEffects );
		pEffect;
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		if ( !pEffect->pD3DEffect )
			continue;
		V( pEffect->pD3DEffect->OnResetDevice() );
	}

	for ( int i = 0; i < NUM_NONMATERIAL_EFFECTS; i++ )
	{
		int nEffectID = dx9_EffectGetNonMaterialID( i );
		if ( nEffectID == INVALID_ID )
			continue;
		D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
		ASSERT_CONTINUE( pEffect );
		if ( ! pEffect->pD3DEffect )
			continue;
		V( pEffect->pD3DEffect->OnResetDevice() );
	}
#endif

	//No reset device in DX10

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
bool dx9_EffectHasAnnotation(const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName)
{
	D3DC_EFFECT_VARIABLE_HANDLE hAnnotation = NULL;
	DX9_BLOCK( hAnnotation = pEffect->pD3DEffect->GetAnnotationByName(hTechnique, pszName); )
	DX10_BLOCK( hAnnotation = hTechnique->GetAnnotationByName( pszName ); )
	return EFFECT_INTERFACE_ISVALID( hAnnotation );
}

template<class T>
static
T dx9_EffectGetAnnotationIntegerDefault(const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName, T nValue)
{
	D3DC_EFFECT_VARIABLE_HANDLE hAnnotation = NULL;
	DX9_BLOCK( hAnnotation = pEffect->pD3DEffect->GetAnnotationByName(hTechnique, pszName); )
	DX10_BLOCK( hAnnotation = hTechnique->GetAnnotationByName( pszName ); )

	if( EFFECT_INTERFACE_ISVALID( hAnnotation ) )
	{
		int nTemp;
		DX9_BLOCK( V(pEffect->pD3DEffect->GetValue(hAnnotation, &nTemp, sizeof( int ))); )
		DX10_BLOCK( V( hAnnotation->GetRawValue( &nTemp, 0, sizeof( int ) ) ); )
		nValue = static_cast<T>(nTemp);
	}
	return nValue;
}

int dx9_EffectGetAnnotation ( const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName )
{
	return dx9_EffectGetAnnotationIntegerDefault(pEffect, hTechnique, pszName, static_cast<int>(0));
}

static
bool dx9_EffectGetAnnotationBool(const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName)
{
	int nFlag = dx9_EffectGetAnnotation(pEffect, hTechnique, pszName);
	ASSERT((nFlag == 0) || (nFlag == 1));
	return !!nFlag;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if 0	// CHB 2006.06.14 - Currently unused.
static
char * dx9_EffectGetAnnotationString( const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName )
{
	D3DC_EFFECT_VARIABLE_HANDLE hAnnotation = pEffect->pD3DEffect->GetAnnotationByName( hTechnique, pszName );
	char * pszValue = NULL;
	if ( hAnnotation )
		HRVERIFY( pEffect->pD3DEffect->GetString( hAnnotation, (LPCSTR*)&pszValue ) );
	return pszValue;
}
#endif	// CHB 2006.06.14

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
VERTEX_SHADER_VERSION dx9_EffectGetVertexShaderVersion(const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique)
{
	// CHB 2006.06.15 - Temp. testing code.
#if 0
	D3DC_EFFECT_PASS_HANDLE hPass = dxC_GetPassByIndex( hTechnique, 0);
	if (hPass != 0) {
		D3DC_PASS_DESC Desc;
		if (SUCCEEDED(pEffect->pD3DEffect->GetPassDesc(hPass, &Desc))) {
			if ((Desc.pPixelShaderFunction != 0) || (Desc.pVertexShaderFunction != 0)) {
				int i = 0;
			}
		}
	}
#endif

	VERTEX_SHADER_VERSION nVSVersion = dx9_EffectGetAnnotationIntegerDefault ( pEffect, hTechnique, "VSVersion", VS_INVALID );
	if ((VS_NONE <= nVSVersion) && (nVSVersion < NUM_VERTEX_SHADER_VERSIONS)) {
		return nVSVersion;
	}
	// If you get this assert, you probably need to add this to your technique definition:
	// < int VSVersion = VS_x_x; int PSVersion = PS_x_x; >
	ASSERTX(false, "Out-of-range vertex shader version found in effect technique!");
	return VS_INVALID;
}

static
PIXEL_SHADER_VERSION dx9_EffectGetPixelShaderVersion(const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique)
{
	PIXEL_SHADER_VERSION nPSVersion = dx9_EffectGetAnnotationIntegerDefault ( pEffect, hTechnique, "PSVersion", PS_INVALID );
	if ((PS_NONE <= nPSVersion) && (nPSVersion < NUM_PIXEL_SHADER_VERSIONS)) {
		return nPSVersion;
	}
	// If you get this assert, you probably need to add this to your technique definition:
	// < int VSVersion = VS_x_x; int PSVersion = PS_x_x; >
	ASSERTX(false, "Out-of-range pixel shader version found in effect technique!");
	return PS_INVALID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX9  //Techniques aren't "set" in dx10
static BOOL sIsEffectAndTechniqueAlreadySet( const LPD3DCEFFECT pEffect, const D3DC_TECHNIQUE_HANDLE hTechnique )
{
	ASSERT_RETFALSE( pEffect && hTechnique );

	sgnDebugEffectCompares++;
	if ( pEffect != sgpCurrentEffect || ! e_GetRenderFlag( RENDER_FLAG_FILTER_EFFECTS ) )
		return FALSE;
	sgnDebugEffectCacheHits++;

	sgnDebugTechniqueCompares++;
	if ( sghCurrentTechnique == hTechnique && e_GetRenderFlag( RENDER_FLAG_FILTER_EFFECTS ) )
	{
		sgnDebugTechniqueCacheHits++;
		return TRUE;
	}
	return FALSE;
}
#endif 

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetTechnique( const LPD3DCEFFECT pEffect, const D3DC_TECHNIQUE_HANDLE hTechnique, UINT * pnNumPasses )
{
	ASSERT_RETZERO( pEffect && hTechnique );

	PRESULT pr = S_FALSE;

#if defined(ENGINE_TARGET_DX9)
	if ( ! sIsEffectAndTechniqueAlreadySet( pEffect, hTechnique ) )
	{
		// need to change effect?
		if ( sgpCurrentEffect && sgnCurrentPass >= 0 )
		{
			V( sgpCurrentEffect->EndPass() );
		}
		if ( sgpCurrentEffect /* && sgpCurrentEffect != pEffect */ )
		{
			V( sgpCurrentEffect->End() );
		}

		V_RETURN( pEffect->SetTechnique( hTechnique ) );
		V_RETURN( pEffect->Begin( (UINT*)&sgnCurrentNumPasses, gdwEffectBeginFlags ) );

		sgpCurrentEffect	= pEffect;
		sghCurrentTechnique = hTechnique;
		sgnCurrentPass		= -1;  // force next beginpass to go through

		pr = S_OK;
	}
#elif defined(ENGINE_TARGET_DX10)
	//Just get number of passes, the technique is set by applying the pass right before the DP call
	D3D10_TECHNIQUE_DESC techDesc;
	V_RETURN( hTechnique->GetDesc( &techDesc ) );
	sgnCurrentNumPasses = techDesc.Passes;
	pr = S_OK;
#endif

	if ( pnNumPasses )
		*pnNumPasses = sgnCurrentNumPasses;

	return pr; 
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectTechniqueSetParamFlags ( const D3D_EFFECT * pEffect, EFFECT_TECHNIQUE & tTechnique )
{
	ASSERT_RETINVALIDARG( pEffect );

	ZeroMemory( tTechnique.dwParamFlags, sizeof(tTechnique.dwParamFlags) );

#ifdef ENGINE_TARGET_DX9
	for ( int i = 0; i < NUM_EFFECT_PARAMS; i++ )
	{
		if ( EFFECT_INTERFACE_ISVALID( pEffect->phParams[ i ] ) && 
			( pEffect->pD3DEffect->IsParameterUsed(  pEffect->phParams[ i ], tTechnique.hHandle ) )
			|| i <= MAX_MUST_HAVE_EFFECT_PARAM )																		// MAX_MUST_HAVE_EFFECT_PARAM == NUM_EFFECT_PARAMS
		{
			SETBIT( tTechnique.dwParamFlags, i, 1 );
		}
	}
#elif defined( ENGINE_TARGET_DX10 )
	V_RETURN( dx10_EffectFillParams( pEffect, tTechnique ) );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sTechniqueMeetsCaps( D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique )
{
	// the first valid technique should be the one with all the most demanding options

	// find the matching best supported shader profile

	return S_OK;
}

static PRESULT sValidateTechnique(D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique )
{
#if defined(ENGINE_TARGET_DX9)
	return pEffect->pD3DEffect->ValidateTechnique( hTechnique );
#elif defined(ENGINE_TARGET_DX10)
	return S_OK;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sFindFirstTechnique( D3D_EFFECT * pEffect, D3DC_EFFECT_DESC & tEffectDesc, D3DC_TECHNIQUE_HANDLE & hTechniqueFirst )
{
	TIMER_START( "Find first tech" )

	hTechniqueFirst = NULL;

	//{
	//	TIMER_START( "Find next valid" )
	//	HRVERIFY( pEffect->pD3DEffect->FindNextValidTechnique( NULL, &hTechniqueFirst ) );
	//	UINT nMs = (UINT)TIMER_END();
	//	trace( "### %5d ms in Effect FindNextValidTech : %s\n", nMs, pEffect->pszFXFileName );
	//}
	hTechniqueFirst = dxC_EffectGetTechniqueByIndex( pEffect->pD3DEffect, 0 );
	if ( ! EFFECT_INTERFACE_ISVALID( hTechniqueFirst ) )
		return -1;

	D3DC_TECHNIQUE_DESC tTechDesc;
	ZeroMemory( &tTechDesc, sizeof( tTechDesc ) );
	V( dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTechniqueFirst, &tTechDesc ) );
	D3DC_PASS_DESC tPassDesc;
	for ( UINT i = 0; i < tTechDesc.Passes; i++ )
	{
		D3DC_EFFECT_PASS_HANDLE hPass = dxC_EffectGetPassByIndex( pEffect->pD3DEffect, hTechniqueFirst, i );
		V( dxC_EffectGetPassDesc( pEffect->pD3DEffect, hPass, &tPassDesc) );
	}

	//return hTechniqueFirst;

	// CHB 2007.07.13
	//bool bValidate = (pEffect->dwFlags & EFFECT_FLAG_KEEP_INVALID_TECHS) == 0;
	bool bValidate = false;

	const OptionState * pState = e_OptionState_GetActive();
	EFFECT_TARGET eTarget = (EFFECT_TARGET)pState->get_nMaxEffectTarget();
	VERTEX_SHADER_VERSION nMaxVSVersion;
	PIXEL_SHADER_VERSION  nMaxPSVersion;
	dxC_GetShaderVersionsFromEffectTarget( eTarget, nMaxVSVersion, nMaxPSVersion );

	VERTEX_SHADER_VERSION nVSVersionFailed = VS_INVALID;
	PIXEL_SHADER_VERSION  nPSVersionFailed = PS_INVALID;

	int nTechnique;
	for ( nTechnique = 0; nTechnique < (int)tEffectDesc.Techniques; nTechnique++ )
	{
		hTechniqueFirst = dxC_EffectGetTechniqueByIndex( pEffect->pD3DEffect, nTechnique );
		ASSERT_CONTINUE( hTechniqueFirst );

		VERTEX_SHADER_VERSION nVSVersion = dx9_EffectGetVertexShaderVersion(pEffect, hTechniqueFirst);
		PIXEL_SHADER_VERSION nPSVersion = dx9_EffectGetPixelShaderVersion(pEffect, hTechniqueFirst);

#if defined(ENGINE_TARGET_DX10)
		//In DX10 the shader version is all we need to "validate"
		if( nVSVersion == nMaxVSVersion && nPSVersion == nMaxPSVersion )
			break;
	}
#elif defined(ENGINE_TARGET_DX9)
		if ((nVSVersion == VS_INVALID) || (nPSVersion == PS_INVALID))
			continue;

		D3DC_TECHNIQUE_DESC tTechDesc;
		V( dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTechniqueFirst, &tTechDesc ) );
		D3DC_PASS_DESC tPassDesc;
		for ( UINT i = 0; i < tTechDesc.Passes; i++ )
		{
			D3DC_EFFECT_PASS_HANDLE hPass = dxC_EffectGetPassByIndex( pEffect->pD3DEffect, hTechniqueFirst , i );
			V( dxC_EffectGetPassDesc( pEffect->pD3DEffect, hPass, &tPassDesc ) );
		}

		if ( S_OK != sTechniqueMeetsCaps( pEffect, hTechniqueFirst ) )
			continue;

		// skip techniques until we hit ones that work
		if ( nVSVersion == nVSVersionFailed && nPSVersion == nPSVersionFailed )
			continue;

		if ( bValidate )
		{
#ifdef TRACE_EFFECT_TIMERS
			trace( "### Validating %s -- %4d : %s\n",
				pEffect->pszFXFileName,
				nTechnique,
				tTechDesc.Name );
#endif // TRACE_EFFECT_TIMERS

			sgnValidateTechniques++;
			V_DO_FAILED( sValidateTechnique( pEffect, hTechniqueFirst ) )
			{
				// CHB 2006.06.25 - Check to see if the technique should have validated.
				// If this assertion trips, the technique was expected to work on the
				// given hardware version level, but it apparently does not.
				ASSERTV( (nVSVersion > nMaxVSVersion) || (nPSVersion > nMaxPSVersion), "Effect should work on this HW level, but does not validate!\n\n%s", pEffect->pszFXFileName );

				nVSVersionFailed = nVSVersion;
				nPSVersionFailed = nPSVersion;
				continue;
			}
		}

		//if ( ! sTechniqueMeetsCaps( pEffect, hTechniqueFirst ) )
		//	continue;

		// when we've gotten to the "correct" shader level techniques
		if ( nVSVersion <= nMaxVSVersion && nPSVersion <= nMaxPSVersion )
			break;
	}
#endif
	if ( ! hTechniqueFirst || nTechnique >= (int)tEffectDesc.Techniques )
	{
		nTechnique = 0;
		hTechniqueFirst = dxC_EffectGetTechniqueByIndex( pEffect->pD3DEffect, nTechnique );
		ASSERT_RETVAL( hTechniqueFirst, -1 );
		V_DO_FAILED( sValidateTechnique( pEffect, hTechniqueFirst ) )
		{
			//ASSERTV_MSG( "Failed to find any validating techniques in effect file: \n%s", pEffect->pszFXFileName );
			hTechniqueFirst = NULL;
			return -1;
		}
		sgnValidateTechniques++;
	}

	{
#ifdef TRACE_EFFECT_RESTORE_TIMERS
		UINT nMs = (UINT)TIMER_END();
		D3DC_TECHNIQUE_DESC tTechDesc;
		V( dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTechniqueFirst, &tTechDesc ) );
		trace( "### %5d ms in Effect find first tech : %s\n   Technique[%4d] : %s\n", nMs, pEffect->pszFXFileName, nTechnique, tTechDesc.Name );
#endif // TRACE_EFFECT_RESTORE_TIMERS
	}

	return nTechnique;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

/*
	CHB 2006.10.11

	Situation: A shader technique compiles without issue for a given
	shader model level. However, the technique fails to validate at
	runtime on some hardware of said shader model level or better.

	Goal: Determine the reason the shader does not validate on some
	hardware.

	Problem: There are several complications:

	-	Even using the debug DirectX runtime with maximum validation,
		ID3DXEffect::ValidateTechnique() does not output any useful
		diagnostic information.

	-	Useful diagnostic information is output only when attempting
		to draw something with an invalid shader.

	-	At the point of validation, it may not be convenient to draw.
		Therefore, a dummy draw to test the shader cannot be easily
		contrived.

	-	Thus we are left to rely on runtime coverage (use) of the
		shader.

	-	The shader technique selection code is designed to be
		failsafe and avoid the use of techniques that fail
		validation.

	-	With some effort it may be possible to construct a separate
		off-screen device for a test render of the technique.

	Solution:

	1.	Enable use of the debug DirectX runtime, with maximum
		validation and the break-on-error option.

	2.	Enable the technique verification code by defining the
		preprocessor symbol ENABLE_TECHNIQUE_VERIFICATION.

	3.	Enable the technique verification code at run time by setting
		s_nEnableTechniqueVerification to the value that represents
		your hardware.

	4.	Shader model compatible techniques that fail validation will
		will output a debug message and cause an assert.

	5.	Following the assert, a flag will be set that permits
		enumeration and selection of the (invalid) technique.

	6.	Run the code until something attempts to draw using the
		technique of interest.

	7.	The DirectX runtime will dump diagnostic information to
		debug output and halt.

	Notes: If a shader technique fails validation, the first thing
	you should do is check whether or not the number of constants
	registers used exceeds the device's limits -- even if Direc3D's
	diagnostic message says something else!
*/

#if defined(_DEBUG) && defined(ENGINE_TARGET_DX9)
	#define ENABLE_TECHNIQUE_VERIFICATION 1
#endif

#ifdef ENABLE_TECHNIQUE_VERIFICATION

static VERTEX_SHADER_VERSION s_nEnableTechniqueVerification = VS_1_1;	// generally too slow on other models for everyday use

static
PRESULT sValidateAllTechniques(D3D_EFFECT * pEffect)
{
	// Our current hardware capability.
	VERTEX_SHADER_VERSION nMaxVSVersion;
	PIXEL_SHADER_VERSION nMaxPSVersion;
	dxC_GetCurrentMaxShaderVersions( nMaxVSVersion, nMaxPSVersion );

	// Only perform validation for hardware of the chosen model or less.
	if (nMaxVSVersion > s_nEnableTechniqueVerification)
	{
		return S_OK;
	}

	// Turn down the warning level, otherwise technique validation
	// bogs down in warnings.
	typedef void (__cdecl DebugSetLevel_type)(BOOL, int);
	DebugSetLevel_type * pDebugSetLevel = 0;
	{
		HMODULE hMod = ::GetModuleHandleA("d3d9d.dll");
		if (hMod != 0)
		{
			pDebugSetLevel = reinterpret_cast<DebugSetLevel_type *>(::GetProcAddress(hMod, "DebugSetLevel"));
		}
		if (pDebugSetLevel != 0)
		{
			(*pDebugSetLevel)(true, 0);
		}
	}

	//
	D3DC_EFFECT_DESC tEffectDesc;
	V( pEffect->pD3DEffect->GetDesc( &tEffectDesc ) );
	int nTechniques = tEffectDesc.Techniques;

	for (int i = 0; i < nTechniques; ++i)
	{
		// Get the technique.
		D3DC_TECHNIQUE_HANDLE hTechnique = dxC_EffectGetTechniqueByIndex(pEffect->pD3DEffect, i);
		if (hTechnique == 0)
		{
			continue;
		}

		// Is it an appropriate version?
		VERTEX_SHADER_VERSION nVSVersion = dx9_EffectGetVertexShaderVersion(pEffect, hTechnique);
		PIXEL_SHADER_VERSION nPSVersion = dx9_EffectGetPixelShaderVersion(pEffect, hTechnique);
		if ((nVSVersion > nMaxVSVersion) || (nPSVersion > nMaxPSVersion))
		{
			continue;
		}

		// Should work on this platform.
		V_DO_FAILED( sValidateTechnique(pEffect, hTechnique) )
		{
			D3DC_TECHNIQUE_DESC tTechDesc;
			V( dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTechnique, &tTechDesc ) );
			trace(__FUNCTION__ ": WARNING: technique %s(%s) failed to validate.\n", tTechDesc.Name, pEffect->pszFXFileName);

			// Check for an annotation permitting silent failure
			// for a specific shader model or lower.
			int nAllowInvalidVS = dx9_EffectGetAnnotationIntegerDefault(pEffect, hTechnique, "AllowInvalidVS", static_cast<int>(VS_NONE));
			if (nMaxVSVersion > nAllowInvalidVS)
			{
				//
				// Note: Hitting this assertion means that a shader
				// technique that's shader model compatible with your
				// video card failed validation for use on your video
				// card. It's likely a situation that someone will want
				// to look at.
				//
				// There will be two possible outcomes:
				//
				//	1.	Adjust the shader technique (or add a new one) to
				//		make it compatible with your video card.
				//
				//	2.	Accept that the shader may not be compatible with
				//		some hardware, and add an "AllowInvalidVS"
				//		annotation to the shader to prevent this assert
				//		from firing in the future.
				//
				// See the large comment preceeding this function for
				// more information.
				//
				ASSERTV(false, "Technique failed validation:\nEffect    %s\nTechnique %s", pEffect->pszFXFileName, tTechDesc.Name );

				//
				// Sigh... no easy way to tell WHY it didn't validate.
				//
				// This flag tells later code not to discard the
				// technique just because it doesn't validate. That
				// way, when the technique is selected for drawing,
				// DirectX will stop with a message describing the
				// problem with the technique.*
				//
				// * Note that DirectX will stop with a diagnostic
				// message for techniques having an invalid vertex
				// shader. However, if the vertex shader is valid
				// but the pixel shader is invalid, the draw will
				// occur and the invalid pixel shader is silently
				// ignored. The result may be untextured polys,
				// for example.
				//
				pEffect->dwFlags |= EFFECT_FLAG_KEEP_INVALID_TECHS;
			}
		}
	}

	// Restore original warning level.
	if (pDebugSetLevel != 0)
	{
		UINT nLevel = ::GetProfileIntA("Direct3D", "debug", 0);
		(*pDebugSetLevel)(true, nLevel);
	}

	return S_OK;
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sTechniqueAnnotationSetFlag(
	D3D_EFFECT * pEffect,
	D3DC_TECHNIQUE_HANDLE hTechnique,
	const char * pszName,
	DWORD & dwTechniqueMask,
	DWORD dwFlag )
{
	int nValue = dx9_EffectGetAnnotation ( pEffect, hTechnique, pszName );
	if ( nValue != 0 )
		dwTechniqueMask |= dwFlag;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sEffectCreate(
	D3D_EFFECT * pEffect,
	void * pFileData,
	int nDataLen )
{
	ASSERT_RETINVALIDARG( pEffect );

	if ( ( pEffect->dwFlags & EFFECT_FLAG_COMPILED ) == 0 )
	{
		ASSERT_RETINVALIDARG( pFileData );
		ASSERT_RETINVALIDARG( nDataLen > 0 );

		TIMER_START( "create effect from file" )

		SPD3DCBLOB pErrorBuffer;

		if( !gpEffectPool )
		{
#if defined(ENGINE_TARGET_DX9)
			V( D3DXCreateEffectPool( &gpEffectPool ) );
#elif defined( ENGINE_TARGET_DX10 )
			// cycle state manager to re-create the global effect pool and fry cache
			dx9_SetStateManager( STATE_MANAGER_TRACK );
			dx9_SetStateManager();
#endif
		}

		V_DO_FAILED( dxC_CreateEffectFromFileInMemory(
			pFileData,
			nDataLen,
			pEffect->pszFXFileName,
			&pEffect->pD3DEffect.p,
			&pErrorBuffer ) )
		{
			sEffectFileError( pEffect->pszFXFileName, pErrorBuffer );
			return E_FAIL;
		}
		ASSERT_RETFAIL( pEffect->pD3DEffect );
		pEffect->dwFlags |= EFFECT_FLAG_COMPILED;
#ifdef ENGINE_TARGET_DX9 //No statemanager in DX10
		if ( gpStateManager )
		{
			V( pEffect->pD3DEffect->SetStateManager( gpStateManager ) );
		}
#endif

#ifdef TRACE_EFFECT_RESTORE_TIMERS
		UINT nMs = (UINT)TIMER_END();
		trace( "### %5d ms in Effect create from file: %s\n", nMs, pEffect->pszFXFileName );
#endif // TRACE_EFFECT_RESTORE_TIMERS
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sTechniqueFeaturesAreValid( TECHNIQUE_FEATURES & tFeatures )
{
	// validate any feature setting to see if it is desirable on this hardware

	int nShadowType = (int)tFeatures.nInts[ TF_INT_MODEL_SHADOWTYPE ];
	if (    (nShadowType != SHADOW_TYPE_NONE) 
		 && (nShadowType != SHADOW_TYPE_ANY) 
		 && (nShadowType != e_GetActiveShadowType()) )
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sFreeEffectTechniques( D3D_EFFECT * pEffect )
{
	if ( pEffect->ptTechniques )
	{
#ifdef ENGINE_TARGET_DX10
		for( int i = 0; i < pEffect->nTechniques; ++i )
			if( pEffect->ptTechniques[ i ].ptPasses )
			{ FREE( NULL, pEffect->ptTechniques[ i ].ptPasses ); }
#endif
			FREE( NULL, pEffect->ptTechniques );
			pEffect->ptTechniques = NULL;
	}

	if ( pEffect->pnTechniquesIndex )
	{
		FREE( NULL, pEffect->pnTechniquesIndex );
		pEffect->pnTechniquesIndex = NULL;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAllocateTechniqueArray(
	D3D_EFFECT * pEffect,
	D3DC_EFFECT_DESC & tEffectDesc )
{
	ASSERT_RETINVALIDARG( pEffect );

	sFreeEffectTechniques( pEffect );

	// At first, allocate space for all the techniques.  If we filter some out, we will realloc later.
	pEffect->nTechniques = (int)tEffectDesc.Techniques;
	pEffect->ptTechniques = ( EFFECT_TECHNIQUE * ) MALLOCZ( NULL, pEffect->nTechniques * sizeof( EFFECT_TECHNIQUE ) );
	ASSERT_RETVAL( pEffect->ptTechniques, E_OUTOFMEMORY );
	pEffect->pnTechniquesIndex = (int*) MALLOC( NULL, pEffect->nTechniques * sizeof(int) );
	ASSERT_RETVAL( pEffect->pnTechniquesIndex, E_OUTOFMEMORY );
	for ( int i = 0; i < pEffect->nTechniques; i++ )
		pEffect->pnTechniquesIndex[i] = i;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sTechniquesCompare( void * pContext, const void * pE1, const void * pE2 )
{
	const EFFECT_TECHNIQUE * pTechs = reinterpret_cast<const EFFECT_TECHNIQUE*>(pContext);
	int nIndex1 = *( reinterpret_cast<const int *>(pE1) );
	int nIndex2 = *( reinterpret_cast<const int *>(pE2) );

	DWORD dwCost1 = pTechs[ nIndex1 ].nCost;
	DWORD dwCost2 = pTechs[ nIndex2 ].nCost;
	if ( dwCost1 < dwCost2 )
		return -1;
	if ( dwCost1 > dwCost2 )
		return 1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sTechniqueFeaturesGetCost( const TECHNIQUE_FEATURES & tFeatures )
{
	int nCost = 0;
	for ( int nInt = 0; nInt < NUM_TECHNIQUE_INT_CHARTS; nInt++ )
	{
		if ( tFeatures.nInts[ nInt ] > 0 )
		{
			if ( gtTechniqueIntChart[nInt].nCostBase >= 0 )
				nCost += gtTechniqueIntChart[nInt].nCostBase;
			nCost += gtTechniqueIntChart[nInt].nCostPer * tFeatures.nInts[ nInt ];
		}
	}

	for ( int nFlag = 0; nFlag < NUM_TECHNIQUE_FLAG_CHARTS; nFlag++ )
	{
		if ( tFeatures.dwFlags & gtTechniqueFlagChart[nFlag].dwFlag )
			if ( gtTechniqueFlagChart[nFlag].nCost >= 0 )
				nCost += gtTechniqueFlagChart[nFlag].nCost;
	}

	return nCost;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sTechniqueFeaturesAdjustRelativeCost( int nCurCost, const TECHNIQUE_FEATURES & tCurFeatures, const TECHNIQUE_FEATURES & tBaseFeatures )
{
	for ( int nInt = 0; nInt < NUM_TECHNIQUE_INT_CHARTS; nInt++ )
	{
		if ( gtTechniqueIntChart[nInt].bMatchMin )
		{
			// penalize the cost by every value under the min desired
			int nDiff = ( tBaseFeatures.nInts[nInt] - tCurFeatures.nInts[nInt] );
			nDiff = MAX( nDiff, 0 );
			// double cost to be below
			nCurCost += 2 * gtTechniqueIntChart[nInt].nCostPer * nDiff;
		}
	}

	return nCurCost;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sTechniqueFeaturesGetDifference( const TECHNIQUE_FEATURES & tBaseFeatures, const TECHNIQUE_FEATURES & tTestFeatures, DWORD & dwReturn )
{
	DWORD dwMax = UINT_MAX;
	DWORD dwDiff = 0;

	for ( int nInt = 0; nInt < NUM_TECHNIQUE_INT_CHARTS; nInt++ )
	{
		if ( tBaseFeatures.nInts[nInt] != tTestFeatures.nInts[nInt] )
		{
			// if this int has a -1 as its base cost, return S_FALSE -- means no substitutions allowed
			if ( gtTechniqueIntChart[ nInt ].nCostBase == -1 )
				return S_FALSE;

			// if test features have an int value the base features are missing, set maxdiff
			if ( tBaseFeatures.nInts[nInt] == 0 )
			{
				dwReturn = dwMax;
				goto finish;
			}

			if ( ! gtTechniqueIntChart[ nInt ].bMatchMin 
				 || tBaseFeatures.nInts[ nInt ] > tTestFeatures.nInts[ nInt ] )
			{
				dwDiff++;
			}
		}
	}

	for ( int nFlag = 0; nFlag < NUM_TECHNIQUE_FLAG_CHARTS; nFlag++ )
	{
		if (	( tBaseFeatures.dwFlags & gtTechniqueFlagChart[nFlag].dwFlag )
			 !=	( tTestFeatures.dwFlags & gtTechniqueFlagChart[nFlag].dwFlag ) )
		{
			// if this flag has a -1 as its cost, return S_FALSE -- means no substitutions allowed
			if ( gtTechniqueFlagChart[ nFlag ].nCost == -1 )
				return S_FALSE;

			// if test features have a flag the base features are missing, set maxdiff
			//if ( 0 == ( tBaseFeatures.dwFlags & gtTechniqueFlagChart[nFlag].dwFlag ) )
			//{
			//	dwReturn = dwMax;
			//	goto finish;
			//}

			dwDiff++;
		}
	}

	dwReturn = dwDiff;

finish:
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFillTechniqueArray(
	D3D_EFFECT * pEffect,
	D3DC_EFFECT_DESC & tEffectDesc,
	EFFECT_DEFINITION * pEffectDef )
{
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETFAIL( pEffect->nTechniques >= (int)tEffectDesc.Techniques );

	D3DC_TECHNIQUE_HANDLE hTech;
	int nTechFirst = sFindFirstTechnique( pEffect, tEffectDesc, hTech );


	if ( nTechFirst < 0 || ! EFFECT_INTERFACE_ISVALID( hTech ) )
	{
		// CHB 2006.09.15 - Note to Kevin: Commenting this back out for the
		// moment, otherwise we get an error dialog for pre-3.0 cards.
		// If this was enabled for a specific purpose, we'll have to work
		// something out here.
#ifdef ENGINE_TARGET_DX10
		ASSERTV_MSG( "Couldn't find any valid techniques in effect!\n\n%s", pEffect->pszFXFileName );
#endif
		return S_FALSE;
	}

	//ZeroMemory( pEffect->ptTechniques, pEffect->nTechniques * sizeof( EFFECT_TECHNIQUE ) );

	D3DC_TECHNIQUE_DESC tTechniqueDescription;
	ZeroMemory( &tTechniqueDescription, sizeof( D3DC_TECHNIQUE_DESC ) );
	V( dxC_EffectGetTechniqueDesc( pEffect->pD3DEffect, hTech, &tTechniqueDescription ) );
	LogMessage( PERF_LOG, "Effect %s using first technique %s, index %d", pEffect->pszFXFileName, tTechniqueDescription.Name, nTechFirst );


	// loop through the techniques and figure out features for each
	VERTEX_SHADER_VERSION nVSVersionStart = dx9_EffectGetVertexShaderVersion(pEffect, hTech);
	VERTEX_SHADER_VERSION nVSVersionCurrent = VS_NONE;	// CHB 2006.06.14 - Was 0.
	PIXEL_SHADER_VERSION nPSVersionStart = dx9_EffectGetPixelShaderVersion(pEffect, hTech);
	PIXEL_SHADER_VERSION nPSVersionCurrent = PS_NONE;	// CHB 2006.06.14 - Was 0.

	// CHB 2007.07.13
	//bool bValidate = (pEffect->dwFlags & EFFECT_FLAG_KEEP_INVALID_TECHS) == 0;
	BOOL bValidate = FALSE;

	int nTechniques = tEffectDesc.Techniques;
	int nValidTechniques = 0;

	for ( int nTech = nTechFirst; nTech < nTechniques; ++nTech )
	{

		hTech = dxC_EffectGetTechniqueByIndex( pEffect->pD3DEffect, nTech );
		if ( ! EFFECT_INTERFACE_ISVALID( hTech ) )
			continue;

		if ( S_OK != sTechniqueMeetsCaps( pEffect, hTech ) )
			continue;

		if ( bValidate )
		{
			sgnValidateTechniques++;
			// CHB 2007.07.13 - If technique validation fails, omit
			// it from the list silently.  It might be a 2.b shader
			// on a 2.0 card, for instance.
			if ( sValidateTechnique( pEffect, hTech ) != S_OK )
			{
				continue;
			}
		}

		nVSVersionCurrent = dx9_EffectGetVertexShaderVersion( pEffect, hTech );
		nPSVersionCurrent = dx9_EffectGetPixelShaderVersion( pEffect, hTech );
		if (    ( ! pEffectDef || ! TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_LOAD_ALL_TECHNIQUES ) )
			 && ( (nVSVersionCurrent != nVSVersionStart) || (nPSVersionCurrent != nPSVersionStart) ) )
		{
			continue;
		}


		// Valid technique!  Add it to the list and parse it for features.

		EFFECT_TECHNIQUE * pTech = &pEffect->ptTechniques[ nValidTechniques ];
		pTech->nCost = 0;
		pTech->tFeatures.Reset();

		// Int features
		ZeroMemory( pTech->tFeatures.nInts, sizeof(int) * NUM_TECHNIQUE_INT_CHARTS );
		for ( int nInt = 0; nInt < NUM_TECHNIQUE_INT_CHARTS; nInt++ )
		{
			pTech->tFeatures.nInts[ nInt ] = dx9_EffectGetAnnotation( pEffect, hTech, gtTechniqueIntChart[ nInt ].pszName );
		}

		// Flag features
		pTech->tFeatures.dwFlags = 0;
		for ( int nFlag = 0; nFlag < NUM_TECHNIQUE_FLAG_CHARTS; nFlag++ )
		{
			int nValue = dx9_EffectGetAnnotation( pEffect, hTech, gtTechniqueFlagChart[ nFlag ].pszName );
			pTech->tFeatures.dwFlags |= nValue ? gtTechniqueFlagChart[nFlag].dwFlag : 0;
		}


		pTech->nCost = sTechniqueFeaturesGetCost( pTech->tFeatures );


		// Check the features
		if ( ! sTechniqueFeaturesAreValid( pTech->tFeatures ) )
			continue;


		// Commit the valid technique
		nValidTechniques++;
		pTech->hHandle = hTech;

		// Pass effectdef flags along
		if ( pEffectDef )
		{
			for ( int nFlag = 0; nFlag < NUM_TECHNIQUE_FLAG_CHARTS; ++nFlag )
			{
				if ( pTech->tFeatures.dwFlags & gtTechniqueFlagChart[nFlag].dwFlag
					 && gtTechniqueFlagChart[nFlag].dwEffectDefFlagBit >= 0 )
				{
					SETBIT( pEffectDef->dwFlags, gtTechniqueFlagChart[nFlag].dwEffectDefFlagBit, TRUE );
				}
			}
		}

		// set the param mask for the technique
		V( dx9_EffectTechniqueSetParamFlags( pEffect, *pTech ) );

		// Dirty the flags to force an update
		DX10_BLOCK( dx10_SetParamDirtyFlags( pTech->dwParamFlags ); )
	}


	ASSERTV( nValidTechniques > 0, "After validation, no techniques remain in effect %s!", pEffect->pszFXFileName );


	// resize the array to the new length to save memory
	pEffect->nTechniques = nValidTechniques;

	pEffect->ptTechniques = ( EFFECT_TECHNIQUE * ) REALLOC( NULL, pEffect->ptTechniques, pEffect->nTechniques * sizeof( EFFECT_TECHNIQUE ) );
	ASSERT_RETVAL( pEffect->ptTechniques, E_OUTOFMEMORY );
	pEffect->pnTechniquesIndex = (int*) REALLOC( NULL, pEffect->pnTechniquesIndex, pEffect->nTechniques * sizeof(int) );
	ASSERT_RETVAL( pEffect->pnTechniquesIndex, E_OUTOFMEMORY );
	for ( int i = 0; i < pEffect->nTechniques; i++ )
		pEffect->pnTechniquesIndex[i] = i;

	// sort the technique index list by cost
	qsort_s( pEffect->pnTechniquesIndex, pEffect->nTechniques, sizeof(int), sTechniquesCompare, pEffect->ptTechniques );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectGetTechniqueByIndex(
	D3D_EFFECT * pEffect,
	int nIndex,
	EFFECT_TECHNIQUE * & pTechniqueOut )
{
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( nIndex, pEffect->nTechniques ) );
	int nTech = pEffect->pnTechniquesIndex[ nIndex ];
	ASSERT_RETFAIL( IS_VALID_INDEX( nTech, pEffect->nTechniques ) );
	pTechniqueOut = &pEffect->ptTechniques[ nTech ];

	sgtTechniqueStats.nGets++;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sEffectMarkTechniqueSelect( D3D_EFFECT * pEffect, int nIndex )
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETURN( pEffect );
	ASSERT_RETURN( IS_VALID_INDEX( nIndex, pEffect->nTechniques ) );

	EFFECT_TECHNIQUE * ptTech = NULL;

	V_DO_SUCCEEDED( dxC_EffectGetTechniqueByIndex( pEffect, nIndex, ptTech ) )
	{
		ptTech->tStats.dwSelected++;
	}
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectGetTechniqueByFeatures(
	D3D_EFFECT * pEffect,
	const TECHNIQUE_FEATURES & tFeatures,
	EFFECT_TECHNIQUE ** ppOutTechnique )
{
	ASSERT_RETINVALIDARG( ppOutTechnique );
	int nIndex;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeatures, nIndex ) );
	V_RETURN( dxC_EffectGetTechniqueByIndex( pEffect, nIndex, *ppOutTechnique ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectGetTechniqueByFeatures(
	D3D_EFFECT * pEffect,
	const TECHNIQUE_FEATURES & tDesiredFeatures,
	int & nIndex )
{
	ASSERT_RETINVALIDARG( pEffect );

	sgtTechniqueStats.nSearch++;

	// Search the index array by increasing cost.  The array is sorted low-cost to high-cost.
	// Stop when we find an exact match or the cost exceeds the original features.
	// As we walk, store the best match so far.  The best match is defined as the lowest-cost,
	// most-matching feature set.

	int nBestMatch = -1;
	DWORD dwBestMatchDiff = UINT_MAX;
	int nBestMatchCost = INT_MAX;

	int nDesiredFeaturesCost = sTechniqueFeaturesGetCost( tDesiredFeatures );
	int i;
	for ( i = 0; i < pEffect->nTechniques; ++i )
	{
		int nTech = pEffect->pnTechniquesIndex[i];
		ASSERT_CONTINUE( IS_VALID_INDEX( nTech, pEffect->nTechniques ) );
		EFFECT_TECHNIQUE * pTechnique = &pEffect->ptTechniques[ nTech ];

		//if ( pTechnique->nCost > nDesiredFeaturesCost )
		//	break;

		if ( ((const TECHNIQUE_FEATURES&)pTechnique->tFeatures) == tDesiredFeatures )
		{
			nBestMatch = i;
			ASSERT( pTechnique->nCost == nDesiredFeaturesCost );
			break;
		}

		DWORD dwDiff;
		PRESULT pr = sTechniqueFeaturesGetDifference( tDesiredFeatures, pTechnique->tFeatures, dwDiff );
		if ( pr != S_OK )
			continue;
		if ( dwDiff > dwBestMatchDiff )
			continue;

		int nCost = sTechniqueFeaturesGetCost( pTechnique->tFeatures );

		nCost = sTechniqueFeaturesAdjustRelativeCost( nCost, pTechnique->tFeatures, tDesiredFeatures );

		// same diff, higher cost == skip
		if ( dwDiff == dwBestMatchDiff )
			if ( nCost > nBestMatchCost )
				continue;

		dwBestMatchDiff = dwDiff;
		nBestMatchCost = nCost;
		nBestMatch = i;
	}

	//ASSERT_DO( i < pEffect->nTechniques && nBestMatch != -1 )
	if ( i >= pEffect->nTechniques || nBestMatch < 0 )
	{
		// didn't find a technique to match!  Use the first one in the list (lowest cost)
		if ( nBestMatch < 0 )
			nBestMatch = 0;

		sgtTechniqueStats.nSearchFailed++;
	}

	nIndex = nBestMatch;

	sEffectMarkTechniqueSelect( pEffect, nIndex );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT dxC_GetEffectAndTechniqueByRef(
	const EFFECT_TECHNIQUE_REF * pRef,
	D3D_EFFECT ** ppOutEffect,
	EFFECT_TECHNIQUE ** ppOutTechnique )
{
	ASSERT_RETINVALIDARG( pRef );
	ASSERT_RETINVALIDARG( ppOutEffect );
	ASSERT_RETINVALIDARG( ppOutTechnique );
	*ppOutEffect = dxC_EffectGet( pRef->nEffectID );
	if ( ! *ppOutEffect )
	{
		*ppOutTechnique = NULL;
		return S_FALSE;
	}

	V_RETURN( dxC_EffectGetTechniqueByIndex( *ppOutEffect, pRef->nIndex, *ppOutTechnique ) );
	ASSERT_RETFAIL( *ppOutTechnique );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------

static PRESULT sTechniqueCacheUpdate(
	const EFFECT_TECHNIQUE_REF & tRef,
	EFFECT_TECHNIQUE_CACHE * ptCache,
	int nFromIndex = -1 )
{
	ASSERT_RETINVALIDARG( ptCache );

	if ( nFromIndex == -1 )
	{
		// add this to the head, pushing back
		MemoryMove(
			&ptCache->tRefs[ 1 ],
			sizeof(EFFECT_TECHNIQUE_REF) * ( NUM_CACHED_EFFECT_TECHNIQUES - 1 ),
			&ptCache->tRefs[ 0 ],
			sizeof(EFFECT_TECHNIQUE_REF) * ( NUM_CACHED_EFFECT_TECHNIQUES - 1 )	);
	}
	else
	{
		// move this to the head, pushing back to the former location
		MemoryMove(
			&ptCache->tRefs[ 1 ],
			sizeof(EFFECT_TECHNIQUE_REF) * ( NUM_CACHED_EFFECT_TECHNIQUES - 1 ),
			&ptCache->tRefs[ 0 ],
			sizeof(EFFECT_TECHNIQUE_REF) * ( nFromIndex ) );
	}

	ptCache->tRefs[ 0 ] = tRef;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectGetTechnique(
	const int nEffectID,
	const TECHNIQUE_FEATURES & tFeatures,
	EFFECT_TECHNIQUE ** ppOutTechnique,
	EFFECT_TECHNIQUE_CACHE * ptCache /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( nEffectID != INVALID_ID );
	ASSERT_RETINVALIDARG( ppOutTechnique );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );

	int nIndex = -1;
	int nFromCache = -1;

	if ( ! e_GetRenderFlag( RENDER_FLAG_TECHNIQUES_CACHE ) )
		ptCache = NULL;

	if ( ptCache && ptCache->nSentinel != gnTechniqueSearchCacheSentinel )
	{
		// If the global sentinel has changed, flush the cache and update the sentinel.
		ptCache->Reset();
		ptCache->nSentinel = gnTechniqueSearchCacheSentinel;
	}
	else if ( ptCache )
	{
		for ( int i = 0; i < NUM_CACHED_EFFECT_TECHNIQUES; ++i )
		{
			if (    ptCache->tRefs[i].nEffectID == nEffectID 
				 && ptCache->tRefs[i].tFeatures == tFeatures )
			{
				// Found! Refresh the cache
				nFromCache = i;
				nIndex = ptCache->tRefs[i].nIndex;

				sgtTechniqueStats.nCacheHits++;

				goto found;
			}
		}
	}

	V_RETURN( dxC_EffectGetTechniqueByFeatures(
		pEffect,
		tFeatures,
		nIndex ) );


found:

	ASSERT_RETFAIL( IS_VALID_INDEX( nIndex, pEffect->nTechniques ) );
	V_RETURN( dxC_EffectGetTechniqueByIndex( pEffect, nIndex, *ppOutTechnique ) );

	// if it wasn't in the cache or wasn't at the head of it, at least
	if ( ptCache && nFromCache != 0 )
	{
		// Cache the reference with the parameters used to search, to avoid frequent re-searching.
		EFFECT_TECHNIQUE_REF tRef;
		tRef.Reset();
		tRef.nEffectID = nEffectID;
		tRef.tFeatures = tFeatures;
		tRef.nIndex    = nIndex;
		V( sTechniqueCacheUpdate( tRef, ptCache, nFromCache ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectCreateFromFileInMemory( D3D_EFFECT * pEffect, EFFECT_DEFINITION * pEffectDef, void * pFileData, int nDataLen )
{
	TIMER_START( pEffect->pszFXFileName );

#ifdef TRACE_EFFECT_TIMERS
	trace( "### Begin EffectCreateFromFileInMemory: %s\n", pEffect->pszFXFileName );
#endif // TRACE_EFFECT_TIMERS

	DebugString( OP_LOW, LOADING_STRING(effect), pEffect->pszFXFileName );


	
	// set up the appropriate vertex declaration for the "validate technique call"
#ifdef ENGINE_TARGET_DX9  //No validation in DX10
	if ( pEffectDef && pEffectDef->eVertexFormat != VERTEX_DECL_INVALID )
	{
		V( dxC_SetVertexDeclaration( pEffectDef->eVertexFormat, pEffect ) );
	} else
	{
		// set a REALLY simple FVF
		V( dx9_SetFVF( VERTEX_ONLY_FORMAT ) );
	}
#endif


	V_RETURN( sEffectCreate( pEffect, pFileData, nDataLen ) );
	ASSERTV_RETFAIL( pEffect->pD3DEffect, pEffect->pszFXFileName );

	e_UpdateGlobalFlags();


	// figure out what params are used in the effect
	ASSERT_RETFAIL( sizeof( gptEffectParamChart ) / sizeof( EFFECT_PARAM_CHART ) == NUM_EFFECT_PARAMS );

	{
		TIMER_START( "get param by name" )

		for ( int i = 0; i < NUM_EFFECT_PARAMS; i++ )
		{
			pEffect->phParams[ i ] 
				= dxC_EffectGetParameterByName( pEffect, gptEffectParamChart[ i ].pszName );
		}

#ifdef TRACE_EFFECT_RESTORE_TIMERS
		UINT nMs = (UINT)TIMER_END();
		trace( "### %5d ms in Effect get param by name: %s\n", nMs, pEffect->pszFXFileName );
#endif // TRACE_EFFECT_RESTORE_TIMERS
	} 


	// CHB 2006.10.11
#ifdef ENABLE_TECHNIQUE_VERIFICATION
	V( sValidateAllTechniques(pEffect) );
#endif



	D3DC_EFFECT_DESC tEffectDesc;
	V_RETURN( pEffect->pD3DEffect->GetDesc( &tEffectDesc ) );

	V_RETURN( sAllocateTechniqueArray( pEffect, tEffectDesc ) );
	V_RETURN( sFillTechniqueArray( pEffect, tEffectDesc, pEffectDef ) );	

	UINT nMs = (UINT)TIMER_END();
#ifdef TRACE_EFFECT_TIMERS
	trace( "### %5d ms in EffectCreateFromFileInMemory: %s\n", nMs, pEffect->pszFXFileName );
#endif // TRACE_EFFECT_TIMERS

	sgnValidateTechniques = 0;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
D3D_EFFECT * dxC_EffectFindNext( const char * pszFileName, D3D_EFFECT * pLastEffect /*= NULL*/ )
{
	D3D_EFFECT * pEffect;
	if ( ! pLastEffect )
		pEffect = HashGetFirst( gtEffects );
	else
		pEffect = HashGetNext( gtEffects, pLastEffect );

	for ( ;
		pEffect; 
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		if ( PStrICmp ( pEffect->pszFXFileName, pszFileName, MAX_XML_STRING_LENGTH ) == 0 )
			return pEffect;
	}

	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL dxC_EffectIsLoaded( const D3D_EFFECT * pEffect )
{
	if ( dxC_IsPixomaticActive() )
		return !! pEffect;
	return BOOL( pEffect && pEffect->pD3DEffect && TESTBIT_DWORD(pEffect->dwFlags, EFFECT_FLAG_LOADED) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sEffectFileLoadedCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

#if 0	// CHB 2007.01.03
	if (data.m_IOSize == 0) 
	{
		return S_FALSE;
	}
#endif

	void *pData = spec->buffer;
	EFFECT_FILE_CALLBACKDATA * pCallbackData = (EFFECT_FILE_CALLBACKDATA *)spec->callbackData;
	ASSERT_RETFAIL( pCallbackData );

	int nEffectID = pCallbackData->nEffectID;
	int nEffectDef = pCallbackData->nEffectDef;

	if ((data.m_IOSize == 0) || (pData == 0))
	{
		// CHB 2007.01.03 - put a breakpoint here to catch missing effect files.
		trace("Effect file not found: %s\n", spec->filename);
		//ASSERTV_MSG( "File does not exist:\n%s", spec->filename);
		return S_FALSE;
	}

	LPD3DCEFFECT pOldD3DEffect;

	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_GOTO( pEffect, _end );
	ASSERT( pEffect->nEffectDefId == INVALID_ID || pEffect->nEffectDefId == nEffectDef );
	EFFECT_DEFINITION * pEffectDef = NULL;
	if ( nEffectDef != INVALID_ID )
	{
		pEffectDef = (EFFECT_DEFINITION*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectDef );
		ASSERT_GOTO( pEffectDef, _end );
	}

	// perhaps we had a debug default set up; save it in case the real effect restore fails
	pOldD3DEffect = pEffect->pD3DEffect;
	pEffect->pD3DEffect = NULL;

	PRESULT pr = E_FAIL;
	if ( ! e_IsNoRender() )
	{
		// in case the device was lost, try to reset it before proceeding
		//V_SAVE_GOTO( pr, _end, e_CheckValid() );

		//V_SAVE_GOTO( pr, _end, dx9_EffectCreateFromFileInMemory( pEffect, pEffectDef, pData, data.m_IOSize ) );
		NEEDS_DEVICE( pr, dx9_EffectCreateFromFileInMemory( pEffect, pEffectDef, pData, data.m_IOSize ) );
		if ( FAILED(pr) )
			goto _end;

		if ( pEffect && pEffect->pD3DEffect )
			SETBIT(pEffect->dwFlags, EFFECT_FLAG_LOADED, TRUE);
		else
			pEffect->pD3DEffect = pOldD3DEffect;
	}

	pr = S_OK;
_end:
	;
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_EffectLoad( int nEffectID, EFFECT_FOLDER eFolder, int nEffectDefID, BOOL bForceSync, BOOL bForceDirect )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETINVALIDARG( pEffect->pszFXFileName[0] );
	EFFECT_DEFINITION * pEffectDef = NULL;
	if ( nEffectDefID != INVALID_ID )
	{
		pEffectDef = (EFFECT_DEFINITION*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectDefID );
		ASSERT_RETFAIL( pEffectDef );
	}

	// figure out the proper folder
	if ( eFolder == EFFECT_FOLDER_FROM_DEF )
	{
		if ( pEffectDef )
			eFolder = pEffectDef->eFolder;
		else
			eFolder = EFFECT_FOLDER_DEFAULT;
	}

	if ( ! IS_VALID_INDEX( eFolder, NUM_EFFECT_FOLDERS ) )
		eFolder = EFFECT_FOLDER_DEFAULT;

	if ( pEffectDef )
		pEffectDef->eFolder = eFolder;

	EFFECT_SUBFOLDER eSubFolder = EFFECT_SUBFOLDER_NOTDEFINED;
	if ( pEffectDef )
		eSubFolder = pEffectDef->eSubFolder;

	EFFECT_FILE_CALLBACKDATA * pCallbackData = (EFFECT_FILE_CALLBACKDATA *) MALLOC( NULL, sizeof( EFFECT_FILE_CALLBACKDATA ) );
	ASSERT_RETVAL( pCallbackData, E_OUTOFMEMORY );
	pCallbackData->nEffectID = nEffectID;
	pCallbackData->nEffectDef = nEffectDefID;

	TCHAR szFullFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	const char * pszFXFileName = pEffectDef ? pEffectDef->pszFXFileName : pEffect->pszFXFileName;
	V( dxC_GetFullShaderPath( szFullFilename, pszFXFileName, eFolder, eSubFolder ) );
	
	DECLARE_LOAD_SPEC( spec, szFullFilename );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	spec.flags |= PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;	// CHB 2007.01.02
	if ( bForceDirect )
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	spec.priority = ASYNC_PRIORITY_EFFECTS;
	spec.fpLoadingThreadCallback = sEffectFileLoadedCallback;
	spec.callbackData = pCallbackData;

#if ISVERSION(DEVELOPMENT)
	if (AppCommonIsAnyFillpak())
	{
		spec.flags |= PAKFILE_LOAD_DONT_LOAD_FROM_PAK;
	}
#endif

	if ( bForceSync )
	{
#ifdef TRACE_EFFECT_TIMERS
		trace( "### Effect Load Sync: %s\n", pEffect->pszFXFileName );
#endif // TRACE_EFFECT_TIMERS

		PakFileLoadNow(spec);
	} 
	else 
	{
#ifdef TRACE_EFFECT_TIMERS
		trace( "### Effect Load Async: %s\n", pEffect->pszFXFileName );
#endif // TRACE_EFFECT_TIMERS

		PakFileLoad(spec);
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_EffectNew( int* pnEffectID, int nEffectFileLine, int nTechniqueGroup, BOOL bForceSync )
{
	ASSERT_RETINVALIDARG( pnEffectID );
	*pnEffectID = INVALID_ID;
	ASSERT_RETINVALIDARG( nEffectFileLine != INVALID_ID );

	const EFFECT_DEFINITION * pEffectDef = NULL;
	pEffectDef = (const EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectFileLine );
	ASSERT_RETINVALIDARG( pEffectDef );
	return dx9_EffectNew( pnEffectID, pEffectDef->pszFXFileName, pEffectDef->eFolder, nEffectFileLine, nTechniqueGroup, bForceSync );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectNew( int* pnEffectID, const char * pszFileName, EFFECT_FOLDER eFolder, int nEffectFileLine, int nTechniqueGroup, BOOL bForceSync )
{
	ASSERT_RETINVALIDARG( pnEffectID );
	*pnEffectID = INVALID_ID;
	ASSERT_RETINVALIDARG( pszFileName );

	//PRESULT pr;

	if ( ! e_IsNoRender() )
	{
		//V_SAVE( pr, e_CheckValid() );
		//if ( pr != S_OK )
		//	return S_FALSE;
	}

	char szFileName[ DEFAULT_FILE_SIZE ];
	PStrCopy( szFileName, pszFileName, DEFAULT_FILE_SIZE );
	PStrLower( szFileName, DEFAULT_FILE_SIZE );

	// first see if we already have this effect (compare filename AND definition line/id)
	D3D_EFFECT * pExistingEffect = dxC_EffectFindNext( szFileName );
	while ( pExistingEffect )
	{
		if ( pExistingEffect->nEffectDefId == nEffectFileLine )
		{
			*pnEffectID = pExistingEffect->nId;
			return S_OK;
		}

		pExistingEffect = dxC_EffectFindNext( szFileName, pExistingEffect );
	}

	int nEffectDefId = nEffectFileLine;
	if ( nEffectDefId == INVALID_ID )
	{
		// look for the effect in the excel table
		nEffectDefId = ExcelGetLineByStringIndex( EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, 1, pszFileName );
	}
	EFFECT_DEFINITION * pEffectDef = NULL;
	if ( nEffectDefId != INVALID_ID )
	{
		pEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectDefId );
	}

	// make a new effect
	D3D_EFFECT * pEffect = HashAdd( gtEffects, gnNextEffectID );
	ASSERT_RETFAIL( pEffect );
	gnNextEffectID++;
	PStrCopy ( pEffect->pszFXFileName, szFileName, MAX_XML_STRING_LENGTH );

	pEffect->nTechniqueGroup = (nTechniqueGroup == INVALID_ID && pEffectDef) ? pEffectDef->nTechniqueGroup : nTechniqueGroup;

	// CHB 2007.03.19 - Coming back to bite you, Chris.
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_LIGHTMAP ) )
		pEffect->dwFlags |= EFFECT_FLAG_FORCE_LIGHTMAP;

	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_EMISSIVE_DIFFUSE ) )
		pEffect->dwFlags |= EFFECT_FLAG_EMISSIVEDIFFUSE;


	// go and find the backup effect - used for LOD
	pEffect->nEffectDefId = nEffectDefId;
	if ( pEffectDef )
	{
		pEffectDef->nD3DEffectId = pEffect->nId;

		BOOL bStaticBranching = pEffectDef->nStaticBranchDepthVS || pEffectDef->nStaticBranchDepthPS;

		// get the distance LOD effect out of the effect_files sheet

		if ( pEffectDef->nLODEffectIndex != INVALID_ID && pEffectDef->fRangeToLOD > 0.f )
		{
			pEffectDef->nLODEffectDef = dx9_GetBestEffectFromIndex( pEffectDef->nLODEffectIndex );
			EFFECT_DEFINITION * pLODEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pEffectDef->nLODEffectDef );
			if ( pLODEffectDef->nD3DEffectId == INVALID_ID )
			{
				V( dx9_EffectNew( &pLODEffectDef->nD3DEffectId, pLODEffectDef->pszFXFileName, pLODEffectDef->eFolder, pEffectDef->nLODEffectDef, nTechniqueGroup, bForceSync ) ); 
				// This line makes no sense.  Overzealous sanity checking?
				//pEffect = HashGet( gtEffects, pEffect->nId );
			}
			pEffectDef->nLODD3DEffectId = pLODEffectDef->nD3DEffectId;
			pEffect->nLODEffect = pLODEffectDef->nD3DEffectId;
		} else {
			pEffect->nLODEffect = INVALID_ID;
		}
		pEffect->fRangeToLOD = pEffectDef->fRangeToLOD;

		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_CAST_SHADOW ) )
			pEffect->dwFlags |= EFFECT_FLAG_CASTSHADOW;
		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_RECEIVE_SHADOW ) )
			pEffect->dwFlags |= EFFECT_FLAG_RECEIVESHADOW;
		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ANIMATED ) )
			pEffect->dwFlags |= EFFECT_FLAG_ANIMATED;
		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_RENDER_TO_Z ) )
			pEffect->dwFlags |= EFFECT_FLAG_RENDERTOZ;
		if ( bStaticBranching )
			pEffect->dwFlags |= EFFECT_FLAG_STATICBRANCH;
	} else {
		pEffect->nLODEffect = INVALID_ID;
		pEffect->fRangeToLOD = 10000.0f;
	}

	if ( dxC_IsPixomaticActive() )
	{
		*pnEffectID = pEffect->nId;
		return S_OK;
	}

	V_RETURN( dx9_EffectLoad( pEffect->nId, eFolder, nEffectDefId, bForceSync ) );

	*pnEffectID = pEffect->nId;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectRemove ( int nEffectID )
{
	sUpdateTechniqueSearchCacheSentinel();

	D3D_EFFECT * pEffect = HashGet( gtEffects, nEffectID );
	if ( ! pEffect )
		return S_FALSE;

	sFreeEffectTechniques( pEffect );

	pEffect->pD3DEffect = NULL;
	pEffect->dwFlags &= ~EFFECT_FLAG_COMPILED;

	HashRemove( gtEffects, nEffectID );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectsInit()
{
	HashInit( gtEffects, NULL, 20 );

	for (int i = 0; i < NUM_NONMATERIAL_EFFECTS; ++i)
	{
		gpnNonMaterialEffects[i] = INVALID_ID;
	}

#if ISVERSION(DEVELOPMENT)
	gptTechniqueReport.Init( NULL );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectsDestroy()
{
#if ISVERSION(DEVELOPMENT)
	gptTechniqueReport.Destroy();
#endif

	V( dx9_EffectsCleanup( FALSE ) );

	for ( D3D_EFFECT * pEffect = HashGetFirst( gtEffects );
		pEffect;
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		sFreeEffectTechniques( pEffect );
	}

	HashFree( gtEffects );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_EffectsCleanup( BOOL bSkipGeneral /*= FALSE*/ )
{
	sUpdateTechniqueSearchCacheSentinel();

	sgpCurrentEffect = NULL;

	for ( D3D_EFFECT * pEffect = HashGetFirst( gtEffects );
		pEffect;
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		if ( bSkipGeneral && pEffect->nEffectDefId != INVALID_ID )
			continue;
		pEffect->pD3DEffect = NULL;
		pEffect->dwFlags &= ~EFFECT_FLAG_COMPILED;
	}

	gpEffectPool = NULL;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectsLoadAll( BOOL bForceSync /*= FALSE*/, BOOL bForceDirect /*= FALSE*/ )
{
	for ( D3D_EFFECT * pEffect = HashGetFirst( gtEffects );
		pEffect;
		pEffect = HashGetNext( gtEffects, pEffect ) )
	{
		if ( pEffect->pD3DEffect )
			continue;
		V_CONTINUE( dx9_EffectLoad( pEffect->nId, EFFECT_FOLDER_FROM_DEF, pEffect->nEffectDefId, bForceSync, bForceDirect ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadersReloadAll( BOOL bForceSyncLoad /*= FALSE*/ )
{
	V_RETURN( dx9_EffectsCleanup() );
	V_RETURN( dx9_EffectsLoadAll( bForceSyncLoad, TRUE ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectSetMatrix ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXMATRIXA16 * pMatrix )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetMatrix( hParam, pMatrix ) ); )
#ifdef ENGINE_TARGET_DX10
		if( !sVariableNeedsUpdate( eParam, (BYTE*)pMatrix, sizeof( D3DXMATRIXA16 ) ) )
			return S_OK;

		V_RETURN( hParam->AsMatrix()->SetMatrix( (float*) pMatrix ) );
#endif
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetMatrix ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXMATRIXA16 * pMatrix )
{
	return sEffectSetMatrix( pEffect, tTechnique, eParam, pMatrix );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetMatrix ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const MATRIX * pMatrix )
{
	return sEffectSetMatrix( pEffect, tTechnique, eParam, (D3DXMATRIXA16*)pMatrix );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect,
	D3DC_EFFECT_VARIABLE_HANDLE eParam, 
	const D3DXVECTOR4 * pVector )
{
	DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( eParam, pVector ) ); )
	DX10_BLOCK( V_RETURN( eParam->AsVector()->SetFloatVector( (float*) pVector ) ); )

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pVector )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
#ifdef ENGINE_TARGET_DX10
		if( !sVariableNeedsUpdate( eParam, (BYTE*)pVector, sizeof( D3DXVECTOR4 ) ) )
			return S_OK;
#endif

		return dx9_EffectSetVector( pEffect, hParam, pVector );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const VECTOR4 * pVector )
{
	return dx9_EffectSetVector( pEffect, tTechnique, eParam, (D3DXVECTOR4*)pVector );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//void dx9_EffectSetVector ( 
//	const D3D_EFFECT * pEffect, 
//	const EFFECT_TECHNIQUE & tTechnique, 
//	const char * pszParam,
//	const D3DXVECTOR4 * pVector )
//{
//	HRVERIFY( pEffect->pD3DEffect->SetVector( pszParam, pVector ) );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//void dx9_EffectSetVector ( 
//	const D3D_EFFECT * pEffect, 
//	const EFFECT_TECHNIQUE & tTechnique, 
//	const char * pszParam,
//	const VECTOR4 * pVector )
//{
//	HRVERIFY( pEffect->pD3DEffect->SetVector( pszParam, (D3DXVECTOR4*)pVector ) );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sCvtColor( D3DXVECTOR4* pNew, DWORD dwColor, BOOL bNoSRGBConvert )
{
	pNew->x = (float)( ( dwColor >> 16 ) & 0xff ) / 255.0f;
	pNew->y = (float)( ( dwColor >> 8  ) & 0xff ) / 255.0f;
	pNew->z = (float)( ( dwColor       ) & 0xff ) / 255.0f;
	pNew->w = (float)( ( dwColor >> 24 ) & 0xff ) / 255.0f;
	if ( ! bNoSRGBConvert && e_ColorsTranslateSRGB() )
	{
		V( e_SRGBToLinear( (VECTOR*)pNew ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sCvtColor( D3DXVECTOR4* pColor, BOOL bNoSRGBConvert )
{
	if ( ! bNoSRGBConvert && e_ColorsTranslateSRGB() )
	{
		V( e_SRGBToLinear( (VECTOR*)pColor ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pColor,
	BOOL bNoSRGBConvert /*= FALSE*/ )
{
	D3DXVECTOR4 vColor = *pColor;
	sCvtColor( &vColor, bNoSRGBConvert );
	return dx9_EffectSetVector( pEffect, tTechnique, eParam, (D3DXVECTOR4*)&vColor );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	DWORD dwColor,
	BOOL bNoSRGBConvert /*= FALSE*/ )
{
	D3DXVECTOR4 vVector;
	sCvtColor( &vVector, dwColor, bNoSRGBConvert );
	return dx9_EffectSetVector( pEffect, tTechnique, eParam, (D3DXVECTOR4*)&vVector );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	const char * pszParam,
	DWORD dwColor,
	BOOL bNoSRGBConvert /*= FALSE*/ )
{
	D3DXVECTOR4 vVector;
	sCvtColor( &vVector, dwColor, bNoSRGBConvert );
	DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( pszParam, (D3DXVECTOR4*)&vVector ) ); )
	DX10_BLOCK( V_RETURN( pEffect->pD3DEffect->GetVariableByName( pszParam )->AsVector()->SetFloatVector( (float*) &vVector ) ); )  //KMNV: TODO this is slow
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetVectorArray ( 
	const D3D_EFFECT * pEffect, 
	D3DC_EFFECT_VARIABLE_HANDLE hParam, 
	const D3DXVECTOR4 * pVector,
	int nCount)
{
	DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVectorArray( hParam, pVector, nCount ) ); )
	DX10_BLOCK( V_RETURN( hParam->AsVector()->SetFloatVectorArray( (float*) pVector, 0, nCount ) ); )
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetVectorArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pVector,
	int nCount)
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
#ifdef ENGINE_TARGET_DX10
		if( !sVariableNeedsUpdate( eParam, (BYTE*)pVector, nCount * sizeof(*pVector) ) )
			return S_OK;
#endif
		return dx9_EffectSetVectorArray( pEffect, hParam, pVector, nCount ) ;
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetTexture ( 
   const D3D_EFFECT * pEffect,
   D3DC_EFFECT_VARIABLE_HANDLE hTexture,
   const LPD3DCBASESHADERRESOURCEVIEW pTexture )
{
	DX9_BLOCK( V( dx9_VerifyValidTextureSet( pTexture ) ); )//Can't do this in DX10 because we're dealing with RTVs
	DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetTexture( hTexture, pTexture ) ); )
	DX10_BLOCK( V_RETURN( hTexture->AsShaderResource()->SetResource( pTexture ) ); )
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetTexture ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const LPD3DCBASESHADERRESOURCEVIEW pTexture )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		return dx9_EffectSetTexture( pEffect, hParam, pTexture );
	}
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//void dx9_EffectSetFloat ( 
//	const D3D_EFFECT * pEffect, 
//	const EFFECT_TECHNIQUE & tTechnique, 
//	const char * pszParam,
//	float fValue )
//{
//	HRVERIFY( pEffect->pD3DEffect->SetFloat( pszParam, fValue ) );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectSetFloatArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nCount )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetFloatArray( hParam, pfValue, nCount ) ); )
		// Why doesn't this take a const float*?
#ifdef ENGINE_TARGET_DX10
		if( !sVariableNeedsUpdate( eParam, (BYTE*)pfValue, nCount * sizeof(*pfValue) ) )
			return S_OK;

		V_RETURN( hParam->AsScalar()->SetFloatArray( (float*)pfValue, 0, nCount ) );
#endif
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetFloatArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nCount )
{
	return sEffectSetFloatArray( pEffect, tTechnique, eParam, pfValue, nCount );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectSetInt ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	int nValue )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetInt( hParam, nValue ) ); )
		DX10_BLOCK( V_RETURN( hParam->AsScalar()->SetInt( nValue ) ); )
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetInt ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	int nValue )
{
#ifdef ENGINE_TARGET_DX10
	if( !sVariableNeedsUpdate( eParam, (BYTE*)&nValue, sizeof( int ) ) )
		return S_OK;
#endif

	return sEffectSetInt( pEffect, tTechnique, eParam, nValue );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectSetIntArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const int * pnValue,
	int nCount )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetIntArray( hParam, pnValue, nCount ) ); )
		// Why doesn't this take a const int*?
		DX10_BLOCK( V_RETURN( hParam->AsScalar()->SetIntArray( (int*)pnValue, 0, nCount ) ) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetIntArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const int * pnValue,
	int nCount )
{
#ifdef ENGINE_TARGET_DX10
	if( !sVariableNeedsUpdate( eParam, (BYTE*)pnValue, nCount * sizeof( int ) ) )
		return S_OK;
#endif

	return sEffectSetIntArray( pEffect, tTechnique, eParam, pnValue, nCount );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectSetBool ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	BOOL bValue )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetBool( hParam, bValue ) ); )
		DX10_BLOCK( V_RETURN( hParam->AsScalar()->SetBool( bValue ) ); )
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetBool ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	BOOL bValue )
{
#ifdef ENGINE_TARGET_DX10
	if( !sVariableNeedsUpdate( eParam, (BYTE*)&bValue, sizeof( BOOL ) ) )
		return S_OK;
#endif

	return sEffectSetBool( pEffect, tTechnique, eParam, bValue );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void sCheckParamBounds(const D3D_EFFECT * pEffect, D3DC_EFFECT_VARIABLE_HANDLE hParam, UINT nBytes)
{
#if ISVERSION(DEVELOPMENT)

	#if defined(ENGINE_TARGET_DX9)
	D3DXPARAMETER_DESC Desc;
	V(pEffect->pD3DEffect->GetParameterDesc(hParam, &Desc));
	ASSERT(nBytes <= Desc.Bytes);
	#elif defined(ENGINE_TARGET_DX10)
	#endif

#else
	REF(pEffect);
	REF(hParam);
	REF(nBytes);
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetRawValue ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nOffset,
	int nSize)
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		ASSERT( 0 == ( nSize % ( 4 * sizeof(float) ) ) );
		sCheckParamBounds(pEffect, hParam, nSize);
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetRawValue( hParam, pfValue, nOffset, nSize ) ); )

#ifdef ENGINE_TARGET_DX10
		if( dx10_IsParamGlobal( eParam ) )
			dx10_DirtyParam( eParam );  //Can't trust that raw values will be exactly the same as values set via other methods

		//if( !sVariableNeedsUpdate( eParam, (BYTE*)&pfValue, nSize ) )
		//	return S_OK;

		V_RETURN( hParam->SetRawValue( (void*)pfValue, nOffset, nSize ) );
#endif
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetValue ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nSize)
{
#ifdef ENGINE_TARGET_DX10
	return dx9_EffectSetRawValue( pEffect, tTechnique, eParam, pfValue, 0, nSize );
#else
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetValue( hParam, pfValue, nSize ) ); )
	}
	return S_OK;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectsCloseActive()
{
	// close any active effects
#ifdef ENGINE_TARGET_DX9
	if ( sgpCurrentEffect && sgnCurrentPass >= 0 )
	{
		V( sgpCurrentEffect->EndPass() );
	}
	if ( sgpCurrentEffect )
	{
		V( sgpCurrentEffect->End() );
	}
#endif

	sgpCurrentEffect			= NULL;
	sghCurrentTechnique			= NULL;
	sgnCurrentPass				= -1;
	sgnCurrentNumPasses			= -1;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ResetEffectCache()
{
	sgnDebugTechniqueCacheHits	= 0;
	sgnDebugTechniqueCompares	= 0;
	sgnDebugEffectCacheHits		= 0;
	sgnDebugEffectCompares		= 0;
	sgnDebugEffectPassCompares	= 0;
	sgnDebugEffectPassCacheHits = 0;
	sgnDebugEffectPassForcedBegins = 0;

	V_RETURN( dx9_EffectsCloseActive() );

	ZeroMemory( &sgtTechniqueStats, sizeof(TECHNIQUE_STATS) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GetEffectCacheStatsString( WCHAR * szStats, int nBufLen )
{
	ASSERT_RETINVALIDARG( szStats );
	PStrPrintf( szStats, nBufLen,
		L"\n\n"
		L"Type    Actual  Calls  Filt  Forced\n"
		L"Effect  %-7d %-7d %-7d\n"
		L"Technq  %-7d %-7d %-7d\n"
		L"Pass    %-7d %-7d %-7d %-7d\n",
		sgnDebugEffectCompares - sgnDebugEffectCacheHits,
		sgnDebugEffectCompares,
		sgnDebugEffectCacheHits,
		sgnDebugTechniqueCompares - sgnDebugTechniqueCacheHits,
		sgnDebugTechniqueCompares,
		sgnDebugTechniqueCacheHits,
		sgnDebugEffectPassCompares - sgnDebugEffectPassCacheHits,
		sgnDebugEffectPassCompares,
		sgnDebugEffectPassCacheHits,
		sgnDebugEffectPassForcedBegins );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GetTechniqueStatsString( WCHAR * szStats, int nBufLen )
{
	ASSERT_RETINVALIDARG( szStats );
	PStrPrintf( szStats, nBufLen,
		L"\n\n"
		L"Get Technique: %d\n"
		L"Cache Hits:    %d\n"
		L"Searches:      %d\n"
		L"Search Failed: %d\n"
		,
		sgtTechniqueStats.nGets,
		sgtTechniqueStats.nCacheHits,
		sgtTechniqueStats.nSearch,
		sgtTechniqueStats.nSearchFailed
		);
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GetTechniqueFeaturesString( WCHAR * szStr, int nBufLen, const TECHNIQUE_FEATURES & tFeats )
{
	ASSERT_RETINVALIDARG( szStr );

	const int cnBufLen = 256;
	WCHAR szItem[ cnBufLen ];

	szStr[0] = NULL;

	for ( int i = 0; i < NUM_TECHNIQUE_INT_CHARTS; i++ )
	{
		PStrPrintf( szItem, cnBufLen, L"  %18S: %d\n", gtTechniqueIntChart[i].pszName, tFeats.nInts[i] );
		PStrCat( szStr, szItem, nBufLen );
	}
	for ( int i = 0; i < NUM_TECHNIQUE_FLAG_CHARTS; i++ )
	{
		PStrPrintf( szItem, cnBufLen, L"  %18S: %d\n", gtTechniqueFlagChart[i].pszName, !!(tFeats.dwFlags & gtTechniqueFlagChart[i].dwFlag) );
		PStrCat( szStr, szItem, nBufLen );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GetTechniqueRefFeaturesString( WCHAR * szStr, int nBufLen, const EFFECT_TECHNIQUE_REF * ptRef )
{
	ASSERT_RETINVALIDARG( szStr );
	ASSERT_RETINVALIDARG( ptRef );

	D3D_EFFECT * pEffect = dxC_EffectGet( ptRef->nEffectID );
	if ( ! pEffect )
		return S_FALSE;

	const int cnBufLen = 256;
	WCHAR szItem[ cnBufLen ];

	PStrPrintf( szStr, nBufLen, L"  %-18s: %4s %4s %4s\n", L"      NAME", L"REQ", L"ACT", L"COST" );

	const TECHNIQUE_FEATURES * ptReq = &ptRef->tFeatures;
	EFFECT_TECHNIQUE * pTech = NULL;
	V_RETURN( dxC_EffectGetTechniqueByIndex( pEffect, ptRef->nIndex, pTech ) );
	ASSERT_RETFAIL( pTech );
	const TECHNIQUE_FEATURES * ptAct = &pTech->tFeatures;

	for ( int i = 0; i < NUM_TECHNIQUE_INT_CHARTS; i++ )
	{
		PStrPrintf( szItem, cnBufLen, L"  %18S: %4d %4d %4d + %d\n",
			gtTechniqueIntChart[i].pszName,
			ptReq->nInts[i],
			ptAct->nInts[i],
			gtTechniqueIntChart[i].nCostBase,
			gtTechniqueIntChart[i].nCostPer );
		PStrCat( szStr, szItem, nBufLen );
	}
	for ( int i = 0; i < NUM_TECHNIQUE_FLAG_CHARTS; i++ )
	{
		PStrPrintf( szItem, cnBufLen, L"  %18S: %4d %4d %4d\n",
			gtTechniqueFlagChart[i].pszName,
			!!(ptReq->dwFlags & gtTechniqueFlagChart[i].dwFlag),
			!!(ptAct->dwFlags & gtTechniqueFlagChart[i].dwFlag),
			gtTechniqueFlagChart[i].nCost );
		PStrCat( szStr, szItem, nBufLen );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetFullEffectInfoString( int nEffectID )
{
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	if ( ! pEffect )
		return S_FALSE;

	trace( 
		"Effect: [%d] %s\n"
		"Techniques: %d\n"
		,
		nEffectID,
		pEffect->pszFXFileName,
		pEffect->nTechniques );

	const int cnInfoLen = 512;
	char szInfo[ cnInfoLen ];

	const int cnFeatLen = 512;
	WCHAR wszFeat[ cnFeatLen ];

	for ( int nTech = 0; nTech < pEffect->nTechniques; nTech++ )
	{
		EFFECT_TECHNIQUE & tTech = pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ nTech ] ];

		V_CONTINUE( dx9_GetTechniqueFeaturesString( wszFeat, cnFeatLen, tTech.tFeatures ) );

		PStrPrintf( szInfo, cnInfoLen,
			"%4d: Cost %4d   File index %4d\n"
			"%S\n"
			,
			nTech,
			tTech.nCost,
			pEffect->pnTechniquesIndex[ nTech ],
			wszFeat
			);

		trace( szInfo );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetEffectListString( char * szStr, int nBufLen )
{
	ASSERT_RETINVALIDARG( szStr );

	szStr[0] = NULL;

	const int cnInfoLen = 512;
	char szInfo[ cnInfoLen ];


	for ( D3D_EFFECT * pEffect = dxC_EffectGetFirst();
		pEffect;
		pEffect = dxC_EffectGetNext( pEffect ) )
	{
		PStrPrintf( szInfo, cnInfoLen, 
			"Effect: [%3d] %s\n"
			,
			pEffect->nId,
			pEffect->pszFXFileName);

		PStrCat( szStr, szInfo, nBufLen );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline PRESULT sEffectBeginPass( const LPD3DCEFFECT pEffect, int nPass, BOOL bForce )
{
#ifdef ENGINE_TARGET_DX9
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETFAIL( pEffect == sgpCurrentEffect );
	ASSERT_RETINVALIDARG( nPass >= 0 );

	PERF( DRAW_FX_BEGINPASS );

	if ( bForce )
		sgnDebugEffectPassForcedBegins++;

	sgnDebugEffectPassCompares++;
	if ( !bForce && ( nPass == sgnCurrentPass ) 
	   && e_GetRenderFlag( RENDER_FLAG_FILTER_EFFECTS ) )
	{
		sgnDebugEffectPassCacheHits++;
		return S_FALSE;
	}

	// need to end current pass?
	if ( sgnCurrentPass >= 0 )
	{
		V_RETURN( pEffect->EndPass() );
	}

	ASSERT_RETFAIL( nPass < sgnCurrentNumPasses );
	V_RETURN( pEffect->BeginPass( (UINT)nPass ) );
#endif

	sgnCurrentPass = nPass;

	return S_OK;
}

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_EffectGrabPasses( LPD3DCEFFECT pD3DEffect, EFFECT_TECHNIQUE** ppTech )
{
	//In dx10 we cache all accessible pass handles
	//this aides in IA construction later as well as
	//reduces calls into the runtime
	ASSERT_RETINVALIDARG( ppTech && *ppTech );
	EFFECT_TECHNIQUE* pTech = *ppTech;

	D3DC_TECHNIQUE_DESC techDesc;
	V( dxC_EffectGetTechniqueDesc( pD3DEffect, pTech->hHandle, &techDesc ) );

	pTech->nPasses = techDesc.Passes;
	pTech->ptPasses = ( EFFECT_PASS* ) MALLOCZ( NULL, pTech->nPasses * sizeof( EFFECT_PASS ) );
	ASSERT_RETFAIL( pTech->ptPasses );

	for( int i = 0; i < pTech->nPasses; ++i )
		pTech->ptPasses[ i ].hHandle = dxC_EffectGetPassByIndex( pD3DEffect, pTech->hHandle, i );

	return S_OK;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectBeginPass( D3D_EFFECT * pEffect, int nPass, EFFECT_DEFINITION* pEffectDef )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETINVALIDARG( nPass >= 0 );

	BOOL bForce = DXC_9_10( FALSE, TRUE );
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_STATE_FROM_EFFECT ) )
		bForce = TRUE;		

	V_RETURN( sEffectBeginPass( pEffect->pD3DEffect, nPass, bForce ) );

#ifdef ENGINE_TARGET_DX10
	ASSERTX( pEffect->ptCurrentTechnique, "No technique set!" );
	if( !pEffect->ptCurrentTechnique->ptPasses )
		dx10_EffectGrabPasses( pEffect->pD3DEffect, &pEffect->ptCurrentTechnique );

	ASSERT_RETFAIL( pEffect->ptCurrentTechnique->ptPasses );

	pEffect->ptCurrentPass = &pEffect->ptCurrentTechnique->ptPasses[ nPass ];

	ASSERT_RETFAIL( pEffect->ptCurrentPass );
#endif

	// If we are in "full alpha" mode, set the alpha opaque floor to 1.0
	if ( e_GetRenderFlag( RENDER_FLAG_ONE_BIT_ALPHA ) && pEffect->ptCurrentTechnique )
	{
		V( dx9_EffectSetFloat( pEffect, *pEffect->ptCurrentTechnique, EFFECT_PARAM_ALPHA_MIN_OPAQUE, 1.f ) );
	}
	else
	{
		V( dx9_EffectSetFloat( pEffect, *pEffect->ptCurrentTechnique, EFFECT_PARAM_ALPHA_MIN_OPAQUE, DEFAULT_GLOW_MIN ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetTechnique( D3D_EFFECT * pEffect, EFFECT_TECHNIQUE * ptTechnique, UINT * pnNumPasses )
{
	if ( dxC_IsPixomaticActive() )
	{
		if ( pnNumPasses )
			*pnNumPasses = 1;
		return S_FALSE;
	}

	ASSERT_RETINVALIDARG( pEffect && ptTechnique );
	DX10_BLOCK( ASSERT_RETINVALIDARG( EFFECT_INTERFACE_ISVALID( ptTechnique->hHandle ) ) );

	pEffect->ptCurrentTechnique = ptTechnique;

#if 0	//Prints the technique name whenever SetTechnique is called
	D3DC_TECHNIQUE_DESC techDesc;
	dxC_EffectGetTechniqueDesc( pEffect, ptTechnique->hHandle, &techDesc );
	LogMessage( "Effect %s using technique %s", pEffect->pszFXFileName, techDesc.Name );
#endif

	return sSetTechnique( pEffect->pD3DEffect, pEffect->ptCurrentTechnique->hHandle, pnNumPasses );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static DWORD sGetTechniqueFlags(
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,	// CHB 2007.09.25
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	const MATERIAL * pMaterial,
	const D3D_EFFECT * pEffect,
	int nLOD )
{
	DWORD dwFlags = pMesh->dwTechniqueFlags;
	if ( pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks )
		dwFlags |= pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMesh ];	// CHB 2006.12.08, 2006.11.28
	//if ( pEffect && pEffect->dwFlags & EFFECT_FLAG_FORCE_LIGHTMAP )
	//	dwFlags |= TECHNIQUE_FLAG_MODEL_LIGHTMAP;
	if ( pMaterial->dwFlags & MATERIAL_FLAG_SPECULAR )
		dwFlags |= TECHNIQUE_FLAG_MODEL_SPECULAR;
	//if ( e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
	//	dwFlags |= TECHNIQUE_FLAG_MODEL_DEBUGFLAG;
	//if ( e_GetRenderFlag( RENDER_FLAG_SHADOWS_ENABLED ) && e_ShadowsActive() &&
	//	! ( pModel->dwFlags & MODEL_FLAG_NOSHADOW ) && sShadowingCurrentRoom( pModel ) )
	if ( e_ShadowsEnabled() && e_ShadowsActive() )
		dwFlags |= TECHNIQUE_FLAG_MODEL_SHADOW;
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NOSHADOW ) )
		dwFlags &= ~(TECHNIQUE_FLAG_MODEL_SHADOW);
	if ( 0 == ( pEffect->dwFlags & EFFECT_FLAG_RECEIVESHADOW ) )
		dwFlags &= ~(TECHNIQUE_FLAG_MODEL_SHADOW);
	if ( pMaterial->dwFlags & MATERIAL_FLAG_NO_RECEIVE_SHADOW )
		dwFlags &= ~(TECHNIQUE_FLAG_MODEL_SHADOW);
	if ( e_GetForceGeometryNormals() )
		dwFlags &= ~ TECHNIQUE_FLAG_MODEL_NORMALMAP;
	if ( !e_OptionState_GetActive()->get_bSpecular() )
		dwFlags &= ~ TECHNIQUE_FLAG_MODEL_SPECULAR;
	if (    pMaterial->dwFlags & MATERIAL_FLAG_CUBE_ENVMAP
		 && pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_ENABLED )
		dwFlags |= TECHNIQUE_FLAG_MODEL_CUBE_ENVMAP;
	if (	pMaterial->dwFlags & MATERIAL_FLAG_SPHERE_ENVMAP
		 && pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_ENABLED )
		dwFlags |= TECHNIQUE_FLAG_MODEL_SPHERE_ENVMAP;
	if ( pMaterial->dwFlags & MATERIAL_FLAG_SCROLLING_ENABLED )
		dwFlags |= TECHNIQUE_FLAG_MODEL_SCROLLUV;
	if ( pMaterial->dwFlags & MATERIAL_FLAG_SUBSURFACE_SCATTERING )
		dwFlags |= TECHNIQUE_FLAG_MODEL_SCATTERING;

	// CHB 2007.09.25 - disable cube mapping if no specular map present.
	if (!dx9_ModelHasSpecularMap(pModel, pModelDef, pMesh, nMesh, pMaterial, nLOD))
	{
		dwFlags &= ~TECHNIQUE_FLAG_MODEL_CUBE_ENVMAP;
		// CML 2007.10.12 - Disable specular lighting if no specular map is present.
		dwFlags &= ~TECHNIQUE_FLAG_MODEL_SPECULAR;
	}

	// CHB 2006.08.05
	Features Feats;
	e_FeaturesGetEffective(pModel->nId, Feats);
	if (Feats.GetFlag(FEATURES_FLAG_SPHERICAL_HARMONICS)) {
		dwFlags |= TECHNIQUE_FLAG_MODEL_SPHERICAL_HARMONICS;
	}
	if (!Feats.GetFlag(FEATURES_FLAG_SPECULAR)) {
		dwFlags &= ~TECHNIQUE_FLAG_MODEL_SPECULAR;
	}

	//ASSERT( dwFlags < TECHNIQUE_FLAG_MODEL_MAX );
	return dwFlags;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_SetStaticBranchParamsFlags(
	DWORD & dwBranches,
	const MATERIAL * pMaterial, 
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh, 
	int nMesh,
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique,
	int nShaderType,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	int nLOD )	// CHB 2006.11.28
{
	DWORD dwFlags = sGetTechniqueFlags( pModel, pModelDef, pMesh, nMesh, pMaterial, pEffect, nLOD );	// CHB 2007.09.25, 2006.11.28
	float fDirLightIntensity[ NUM_ENV_DIR_LIGHTS ];
	for ( int i = 0; i < NUM_ENV_DIR_LIGHTS; i++ )
		fDirLightIntensity[ i ] = e_GetDirLightIntensity( pEnvDef, (ENV_DIR_LIGHT_TYPE)i );
	BOOL bDirLight1 = ( fDirLightIntensity[ ENV_DIR_LIGHT_PRIMARY_DIFFUSE   ] > 1e-5f ) || ( fDirLightIntensity[ ENV_DIR_LIGHT_PRIMARY_SPECULAR  ] > 1e-5f );
	BOOL bDirLight2 = ( fDirLightIntensity[ ENV_DIR_LIGHT_SECONDARY_DIFFUSE ] > 1e-5f );

	dwBranches = 0;

	// set all bools, every time
	// CHB 2006.11.28
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_SPECULAR ) )    && ( dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_SPECULAR, nLOD, FALSE, pMaterial ) != INVALID_ID ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SPECULAR )					: 0;
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_DIFFUSEMAP2 ) ) && ( dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_DIFFUSE2, nLOD, FALSE, pMaterial ) != INVALID_ID ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIFFUSEMAP2 )				: 0;
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_NORMALMAP ) )   && ( dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_NORMAL, nLOD, FALSE, pMaterial   ) != INVALID_ID ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_NORMALMAP )					: 0;

	// We do this twice for shaders using the old convention of self-illum and lightmap sharing the lightmap channel - they both share a static branch right now :/
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_LIGHTMAP ) )    && ( dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_LIGHTMAP, nLOD, FALSE, pMaterial ) != INVALID_ID  ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_LIGHTMAP )					: 0;
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_SELFILLUM ) )   && ( dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_SELFILLUM, nLOD, FALSE, pMaterial ) != INVALID_ID ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_LIGHTMAP )					: 0;

	/*dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_OPACITYMAP ) )  && ( dx9_ModelGetTexture( pModel, pModelDef, nMesh, TEXTURE_OPACITY, nLOD, FALSE, pMaterial  ) != INVALID_ID ) )	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_OPACITYMAP )					: 0;*/
	//dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_DEBUGFLAG ) ) )																					? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DEBUGFLAG )					: 0;
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_SHADOW ) ) )																					? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SHADOWMAP )					: 0;
	dwBranches |= ( ( 0 != ( dwFlags & TECHNIQUE_FLAG_MODEL_SKINNED ) ) )																					? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SKINNED )					: 0;
	dwBranches |= ( bDirLight1 )																															? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIRLIGHT )					: 0;
	dwBranches |= ( bDirLight2 )																															? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIRLIGHT2 )					: 0;
	dwBranches |= ( ( 0 != ( pMaterial->dwFlags & MATERIAL_FLAG_CUBE_ENVMAP   ) ) && ( 0 != ( pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_ENABLED ) ) )		? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_CUBE_ENVMAP )				: 0;
	dwBranches |= ( ( 0 != ( pMaterial->dwFlags & MATERIAL_FLAG_SPHERE_ENVMAP ) ) && ( 0 != ( pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_ENABLED ) ) )		? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SPHERE_ENVMAP )				: 0;
	dwBranches |= ( FALSE )																																	? EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SCROLLLIGHTMAP )				: 0;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_SetStaticBranchParamsFromFlags(
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique,
	DWORD dwBranches )
{
	// set all bools, every time
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SPECULAR,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SPECULAR ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_DIFFUSEMAP2,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIFFUSEMAP2 ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_NORMALMAP,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_NORMALMAP ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_LIGHTMAP,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_LIGHTMAP ) ) ) );
	/*V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_OPACITYMAP,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_OPACITYMAP ) ) ) );*/
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_DEBUGFLAG,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DEBUGFLAG ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SHADOWMAP,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SHADOWMAP ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SHADOWMAPDEPTH,				0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SHADOWMAPDEPTH ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SKINNED,						0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SKINNED ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_DIRLIGHT,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIRLIGHT ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_DIRLIGHT2,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_DIRLIGHT2 ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_CUBE_ENVMAP,					0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_CUBE_ENVMAP ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SPHERE_ENVMAP,				0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SPHERE_ENVMAP ) ) ) );
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_BRANCH_SCROLLLIGHTMAP,				0 != ( dwBranches & EFFECT_MAKE_BRANCH_MASK( EFFECT_PARAM_BRANCH_SCROLLLIGHTMAP ) ) ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sToolLoadShaderNow(
	D3D_EFFECT *& pEffect,
	const MATERIAL * pMaterial,
	int nShaderType )
{
	if ( ! pEffect )
	{
		V_RETURN( dx9_LoadShaderType( pMaterial->pszShaderName, nShaderType ) );
		pEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
	}
	if ( ! pEffect )
	{
		char szError[ 512 ];
		PStrPrintf( szError, 512, "ShaderName: [%s]  ShaderType: [%d]", pMaterial->pszShaderName, nShaderType );
		ASSERTX_RETFAIL( pEffect, szError );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ToolLoadShaderNow(
	D3D_EFFECT *& pEffect,
	const MATERIAL * pMaterial,
	int nShaderType )
{
	if ( ! c_GetToolMode() )
		return S_FALSE;
	if ( pEffect )
		return S_OK;
	ASSERT_RETINVALIDARG( pMaterial );
	return sToolLoadShaderNow( pEffect, pMaterial, nShaderType );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGetEffectLOD(
	int & nEffectID,
	D3D_EFFECT *& pEffect,
	float & fTransitionStr,
	float fDistanceToEyeSquared,
	const OptionState * pOptionState = NULL,
	int nRecursions = 0 )
{
	// Prevent recursion loops.
	ASSERT_RETFAIL( nRecursions < 16 );

	BOOL bLODRange = FALSE;
	float fRangeToLOD = pEffect->fRangeToLOD;
	if ( ! AppIsTugboat() )
	{
		if ( pOptionState )
			fRangeToLOD = pOptionState->get_fEffectLODScale() * fRangeToLOD + pOptionState->get_fEffectLODBias();
		if ( fDistanceToEyeSquared > fRangeToLOD * fRangeToLOD )
			bLODRange = TRUE;
	}

	BOOL bUseLOD = FALSE;
	if ( bLODRange )
		bUseLOD = TRUE;
	if ( e_InCheapMode() )
		bUseLOD = TRUE;
	if ( e_GetRenderFlag( RENDER_FLAG_LOD_EFFECTS_ONLY) )
		bUseLOD = TRUE;
	if ( pEffect->nLODEffect == INVALID_ID )
		bUseLOD = FALSE;
	if ( ! e_GetRenderFlag( RENDER_FLAG_LOD_EFFECTS ) )
		bUseLOD = FALSE;
	if ( bUseLOD )
	{
		int nLODEffectID = pEffect->nLODEffect;
		D3D_EFFECT * pLODEffect = dxC_EffectGet( pEffect->nLODEffect );
		if ( dxC_EffectIsLoaded( pLODEffect ) )
		{
			nEffectID = nLODEffectID;
			pEffect   = pLODEffect;

			float fDist = 0.f;
			if ( fDistanceToEyeSquared > 0.f )
				fDist = sqrtf( fDistanceToEyeSquared );
			fDist -= fRangeToLOD;
			fDist *= fDist;
			V_RETURN( sGetEffectLOD( nEffectID, pEffect, fTransitionStr, fDist, pOptionState, nRecursions + 1 ) );

			return S_OK;
		}
	}

	if ( ! AppIsTugboat() )
	{
		// if there is a backup effect	
		if ( ! bLODRange && fRangeToLOD > 0.f && pEffect->nLODEffect != INVALID_ID )
		{
			float fTransDist = fRangeToLOD * EFFECT_LOD_TRANSITION_START;
			if ( fDistanceToEyeSquared > fTransDist * fTransDist )
			{
				// return a transition-to-LOD value (1.0 == current, 0.0 == LOD)
				fTransitionStr = 1.0f - ( sqrtf( fDistanceToEyeSquared ) - fTransDist ) / ( fRangeToLOD - fTransDist );
			}
		}
	}

	return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GetEffectAndTechnique( 
	const MATERIAL * pMaterial, 
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,	// CHB 2007.09.25
	const D3D_MESH_DEFINITION * pMesh, 
	int nMesh,
	DWORD dwMeshFlagsDoDraw,
	int nPointLights, 
	int nSpecularLights, 
	float fDistanceToEyeSquared, 
	EFFECT_DEFINITION * pEffectDef,
	D3D_EFFECT ** ppEffect, 
	EFFECT_TECHNIQUE ** pptTechnique,
	float & fTransitionStr,
	int nShaderType,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	DWORD * pdwFinalFlags,
	int nLOD )	// CHB 2006.11.28
{
	if ( dwMeshFlagsDoDraw & MESH_FLAG_SILHOUETTE )
	{
		int nEffectId = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SILHOUETTE );
		*ppEffect = dxC_EffectGet( nEffectId );
		ASSERT_RETFAIL( *ppEffect );
		*pptTechnique = &(*ppEffect)->ptTechniques[ (*ppEffect)->pnTechniquesIndex[ 0 ] ];
		return S_OK;
	}

	*ppEffect = NULL;

	int nEffectID = dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	// if the shader type isn't loaded yet, take care of it now
	V_RETURN( dxC_ToolLoadShaderNow( pEffect, pMaterial, nShaderType ) );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;
	ASSERT_RETFAIL( pEffect );


	if ( dxC_IsPixomaticActive() )
	{
		*ppEffect = pEffect;
		return S_OK;
	}


	BOOL bUseLOD = TRUE;
	if ( pEnvDef && ( pEnvDef->dwFlags & ENVIRONMENT_FLAG_NO_LOD_EFFECTS ) )
		bUseLOD = FALSE;
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NO_LOD_EFFECTS ) )
		bUseLOD = FALSE;

	fTransitionStr = 1.f;
	if ( bUseLOD )
	{
		const OptionState * pOptionState = e_OptionState_GetActive();
		V_RETURN( sGetEffectLOD( nEffectID, pEffect, fTransitionStr, fDistanceToEyeSquared, pOptionState ) );
	}

	ASSERTX_RETFAIL( pEffect->pD3DEffect, pEffect->pszFXFileName );

	*ppEffect = pEffect;
	DWORD dwFlags = sGetTechniqueFlags( pModel, pModelDef, pMesh, nMesh, pMaterial, pEffect, nLOD );	// CHB 2007.09.25, CHB 2006.11.28



	if ( pEffectDef &&
		 dwFlags & TECHNIQUE_FLAG_MODEL_32BYTE_VERTEX &&
		 pEffectDef->eVertexFormat == VERTEX_DECL_ANIMATED )
	{
		D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
		ASSERTV_MSG( "Detected invalid shader type in a material for this model:\nMaterial: %s\nModel: %s",
			pMaterial ? pMaterial->tHeader.pszName : "unknown",
			pModelDef ? pModelDef->tHeader.pszName : "unknown" );
		*ppEffect = NULL;
		return E_FAIL;
	}


	// CHB 2006.08.07
	Features Feats;
	e_FeaturesGetEffective(pModel->nId, Feats);
	nPointLights = Feats.GetFlag(FEATURES_FLAG_POINT_LIGHTS) ? nPointLights : 0;
	int nShadowType = SHADOW_TYPE_NONE;
	if ( dwFlags & TECHNIQUE_FLAG_MODEL_SHADOW && e_ShadowsActive() )
		nShadowType = e_GetActiveShadowType();


	EFFECT_TECHNIQUE* ptTechniqueToUse = NULL;

	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	if ( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_MODEL )
	{
		tFeat.nInts[ TF_INT_MODEL_POINTLIGHTS ] = nPointLights;
		tFeat.nInts[ TF_INT_MODEL_SHADOWTYPE  ] = nShadowType;
		tFeat.dwFlags = dwFlags;
	}
	else if ( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_GENERAL )
	{
		tFeat.nInts[ TF_INT_INDEX ] = 0;
	}
	V_RETURN( dxC_EffectGetTechnique( nEffectID, tFeat, &ptTechniqueToUse, pMesh->ptTechniqueCache ) );



	if ( ptTechniqueToUse )
	{
		*pptTechnique = ptTechniqueToUse;
		ASSERTX_RETFAIL( (*pptTechnique)->hHandle, pEffect->pszFXFileName );
	}
	else
	{
		(*pptTechnique)->hHandle = NULL;
	}

	if ( pdwFinalFlags )
		*pdwFinalFlags = dwFlags;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_GetCurrentShaderType(
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const DRAWLIST_STATE * pState,
	const D3D_MODEL * pModel )
{
	if ( ! pEnvDef )
		pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return SHADER_TYPE_INVALID;
	int nShaderType = pEnvDef ? pEnvDef->nLocation : SHADER_TYPE_INVALID;
	if ( pState && pState->nOverrideShaderType != SHADER_TYPE_INVALID )
		nShaderType = pState->nOverrideShaderType;
	return nShaderType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectCreateNonMaterialEffects()
{
	const OptionState * pState = e_OptionState_GetActive();

	bool b20OrBetter = e_GetMaxDetail() > FXTGT_SM_11;

	// CHB 2007.01.02
	for (int i = 0; i < NUM_NONMATERIAL_EFFECTS; ++i)
	{
		gpnNonMaterialEffects[i] = INVALID_ID;
	}

	V( dx9_EffectCreateDebugShaders() );

	int nSilhouetteFileLine		= ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, "Silhouette_FXO" );
	int nOcclusionFileLine		= ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, e_GetMaxDetail() > FXTGT_SM_20_LOW ? "Occlusion_FXO" : "Occlusion_FXO_11" );
	int nMoviePlayerFileLine	= ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, b20OrBetter ? "MoviePlayer_FXO" : "MoviePlayer11_FXO" );
	
	// CHB 2006.07.14 - Choose the shadow map effect based on the
	// number of bones supported by the shader. Currently the effect
	// includes both rigid and animated shaders which is why this is
	// necessary.

	// Check bone counts to determine which shader to use.
	bool bHighBones = e_AnimatedModelGetMaxBonesPerShader() > MAX_BONES_PER_SHADER_ON_DISK;

	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_UI 		], "UI.fxo",							EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );

	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SHADOW  	], bHighBones ? "ShadowMap.fxo" : "ShadowMap11.fxo", EFFECT_FOLDER_COMMON, INVALID_ID, TECHNIQUE_GROUP_SHADOW ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SHADOW_BLOB	], "ShadowBlob.fxo",				EFFECT_FOLDER_COMMON, INVALID_ID,				TECHNIQUE_GROUP_SHADOW ) );

	// CHB 2007.01.02
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_OVERLAY 		], "Overlay.fxo",					EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_ZBUFFER 		], "_ZBuffer.fxo",					EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_COMBINE 		], "CombineLayers.fxo",				EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_BLUR ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SIMPLE  	 	], "SimpleNoTexture.fxo",			EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SILHOUETTE 	], "Silhouette.fxo",				EFFECT_FOLDER_FROM_DEF,	nSilhouetteFileLine,	TECHNIQUE_GROUP_GENERAL ) );

	if ( nOcclusionFileLine != INVALID_ID )
	{
		V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_OCCLUDED	], "OcclusionMap.fxo",				EFFECT_FOLDER_FROM_DEF,	nOcclusionFileLine,		TECHNIQUE_GROUP_GENERAL ) );
	}

	if ( nMoviePlayerFileLine != INVALID_ID )
	{
		V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_MOVIE	], "MoviePlayer.fxo",					EFFECT_FOLDER_FROM_DEF,	nMoviePlayerFileLine,	TECHNIQUE_GROUP_GENERAL ) );
	}

	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SCREEN_EFFECT	], "ScreenEffects.fxo",				EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_LIST ) );

	// CHB 2007.08.30 - Now do blur on 1.1.
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_BLUR    			], "Blurr.fxo",				EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_BLUR ) );

	if ( b20OrBetter )
	{
#ifdef ENGINE_TARGET_DX10
		//V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_SHADOW_MIPGENERATOR ], "DX10ShadowMipGenerator.fxo",		EFFECT_FOLDER_HELLGATE,		INVALID_ID,			TECHNIQUE_GROUP_GENERAL ) );
		//V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_FIXED_FUNC_EMULATOR ], "DX10FixedFuncEmulator.fxo",		EFFECT_FOLDER_HELLGATE,		INVALID_ID,			TECHNIQUE_GROUP_GENERAL ) );
#endif // DX10

		V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_GAUSSIAN			], "Gaussian.fxo",						EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_BLUR ) );
	}

#if ISVERSION(DEVELOPMENT)
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_OBSCURANCE			], "Obscurance.fxo",				EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	if ( b20OrBetter )
	{
		V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_DEBUG   			], "Debug.fxo",				EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	}
	if ( e_GetMaxDetail() >= FXTGT_SM_30 )
	{
		V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_HDR				], "HDR.fxo",					EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	}
#endif // DEVELOPMENT

#ifdef ENABLE_NEW_AUTOMAP

	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_AUTOMAP_GENERATE  	 	], "AutomapGenerate.fxo",		EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );
	V( dx9_EffectNew( &gpnNonMaterialEffects[ NONMATERIAL_EFFECT_AUTOMAP_RENDER  	 	], "AutomapRender.fxo",			EFFECT_FOLDER_COMMON,	INVALID_ID,				TECHNIQUE_GROUP_GENERAL ) );

#endif

	// CHB 2007.10.02
	if (e_CanDoManualGamma(pState))
	{
		V( dx9_EffectNew( &gpnNonMaterialEffects[NONMATERIAL_EFFECT_GAMMA], "WindowGamma.fxo", EFFECT_FOLDER_COMMON, INVALID_ID, TECHNIQUE_GROUP_GENERAL ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectCreateDebugShaders()
{
#if ISVERSION(DEVELOPMENT)
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	const int cnBufLen = 2048;
	SPD3DCBLOB pErrorBuffer;

#ifdef ENGINE_TARGET_DX9
	ONCE
	{
		const char szProfileVS[ cnBufLen ] = 
			"float4 VS_PROFILE(														\n"
			"			in float2 Position : POSITION,								\n"
			"			out float4 c0 : COLOR0,										\n"
			"			out float4 c1 : COLOR1,										\n"
			"			out float4 t0 : TEXCOORD0,									\n"
			"			out float4 t1 : TEXCOORD1,									\n"
			"			out float4 t2 : TEXCOORD2,									\n"
			"			out float4 t3 : TEXCOORD3,									\n"
			"			out float4 t4 : TEXCOORD4,									\n"
			"			out float4 t5 : TEXCOORD5,									\n"
			"			out float4 t6 : TEXCOORD6,									\n"
			"			out float4 t7 : TEXCOORD7									\n"
			"				) : POSITION											\n"
			"{																		\n"
			"	t0 = ( Position.xy * float2( 0.5f, -0.5f ) + 0.5f ).xyxy;			\n"
			"	t1 = 1.f - t0;														\n"
			"	t2 = t0 * 2;														\n"
			"	t3 = t1 * 2;														\n"
			"	t4 = t0 + t1;														\n"
			"	t5 = t2 + t3;														\n"
			"	t6 = t2 + t1;														\n"
			"	t7 = t0 + t3;														\n"
			"	c0 = float4(1,0,0,1);												\n"
			"	c1 = float4(0,0,1,1);												\n"
			"	return float4( Position.xy, 0.0f, 1.0f );							\n"
			"}";
		SPD3DCBLOB pProfileVSCompiled;

		V_DO_FAILED( D3DXCompileShader(
			szProfileVS,
			PStrLen( szProfileVS, cnBufLen ),
			NULL,
			NULL,
			"VS_PROFILE",
			"vs_1_1",
			D3DXSHADER_SKIPVALIDATION,
			&pProfileVSCompiled,
			&pErrorBuffer,
			NULL ) )
		{
			if ( pErrorBuffer )
				ErrorDialog( "Error compiling debug shader:\n\n%s", (char*)pErrorBuffer->GetBufferPointer() );
			else
				ErrorDialog( "Error compiling debug shader:\n\n(no errorbuffer output)" );
			break;
		}
		ASSERT_BREAK( pProfileVSCompiled != NULL );
		ASSERT_BREAK( pProfileVSCompiled->GetBufferPointer() != NULL );
		ASSERT_BREAK( pProfileVSCompiled->GetBufferSize() > 0 );

		V_RETURN( dxC_GetDevice()->CreateVertexShader(
			(DWORD*)pProfileVSCompiled->GetBufferPointer(),
			&sgpDebugVertexShaders[ DEBUG_VS_PROFILE ] ) );

	}
#endif // DX9

#endif // DEVELOPMENT

	return S_OK;

//	const int cnBufLen = 1024;
//	SPD3DCBLOB pErrorBuffer;
//
//#ifdef ENGINE_TARGET_DX9
//	ONCE
//	{
//		const char szWireframeShader[ cnBufLen ] = 
//			"float4 PS_WIREFRAME() : COLOR { return float4( 0.5f, 0.5f, 1.0f, 1.0f ); }";
//		SPD3DCBLOB pWireframeCompiled;
//
//		V_RETURN( D3DXCompileShader(
//			szWireframeShader,
//			PStrLen( szWireframeShader, cnBufLen ),
//			NULL,
//			NULL,
//			"PS_WIREFRAME",
//			"ps_1_1",
//			D3DXSHADER_SKIPVALIDATION,
//			&pWireframeCompiled,
//			&pErrorBuffer,
//			NULL ) );
//		ASSERT_BREAK( pErrorBuffer == NULL );
//		ASSERT_BREAK( pWireframeCompiled != NULL );
//		ASSERT_BREAK( pWireframeCompiled->GetBufferPointer() != NULL );
//		ASSERT_BREAK( pWireframeCompiled->GetBufferSize() > 0 );
//
//		V_RETURN( dx9_GetDevice()->CreatePixelShader(
//			(DWORD*)pWireframeCompiled->GetBufferPointer(),
//			&sgpDebugPixelShaders[ DEBUG_PS_WIREFRAME ] ) );
//
//	}
//#endif
//
//	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetDebugVertexShader( DEBUG_VERTEX_SHADER eShader )
{
#ifdef ENGINE_TARGET_DX9
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eShader, NUM_DEBUG_VERTEX_SHADERS ) );
	if ( ! sgpDebugVertexShaders[ eShader ] )
		return S_FALSE;

	V( dxC_GetDevice()->SetVertexShader( sgpDebugVertexShaders[ eShader ] ) );
#endif
	return S_OK;
}

PRESULT dx9_EffectSetDebugPixelShader( DEBUG_PIXEL_SHADER eShader )
{
#ifdef ENGINE_TARGET_DX9
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eShader, NUM_DEBUG_PIXEL_SHADERS ) );
	if ( ! sgpDebugPixelShaders[ eShader ] )
		return S_FALSE;

	V( dxC_GetDevice()->SetPixelShader( sgpDebugPixelShaders[ eShader ] ) );
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float sUpdateMeshScrollPhase( float & fMeshPhase, float fElapsed, float fRate, float fMod = 1.f )
{
	fMeshPhase += fElapsed * fRate;
	fMeshPhase = FMODF( fMeshPhase, fMod );
	return fMeshPhase;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sUpdateAndGetScrollPhaseForChannel( D3D_MODEL * pModel, int nCurMesh, int nMeshCount, int nLOD, MATERIAL * pMaterial, int nChannel, float * pfOutPhaseU, float * pfOutPhaseV )
{
	TIME tCurTime = e_GetCurrentTimeRespectingPauses();

	float * pfPhaseU;
	float * pfPhaseV;

	if ( pMaterial->dwFlags & MATERIAL_FLAG_SCROLL_GLOBAL_PHASE )
	{
		if ( tCurTime > pMaterial->tScrollLastUpdateTime )
		{
			// update material scroll phases

			TIMEDELTA tDelta = tCurTime - pMaterial->tScrollLastUpdateTime;
			float fSecs = tDelta / (float)MSECS_PER_SEC;

			for ( int nUpdateChannel = 0; nUpdateChannel < NUM_MATERIAL_SCROLL_CHANNELS; ++nUpdateChannel )
			{
				pfPhaseU = &pMaterial->fScrollPhaseU[ nUpdateChannel ];
				pfPhaseV = &pMaterial->fScrollPhaseV[ nUpdateChannel ];

				sUpdateMeshScrollPhase( *pfPhaseU, fSecs, pMaterial->fScrollRateU[ nUpdateChannel ] );
				sUpdateMeshScrollPhase( *pfPhaseV, fSecs, pMaterial->fScrollRateV[ nUpdateChannel ] );
			}

			pMaterial->tScrollLastUpdateTime = tCurTime;
		}

		*pfOutPhaseU = pMaterial->fScrollPhaseU[ nChannel ];
		*pfOutPhaseV = pMaterial->fScrollPhaseV[ nChannel ];
	} else
	{
		D3D_MODEL_DEFDATA * pDefData = &pModel->tModelDefData[ nLOD ];
		if ( tCurTime > pDefData->tScrollLastUpdateTime )
		{
			// update all model scroll phases for all channels

			TIMEDELTA tDelta = tCurTime - pDefData->tScrollLastUpdateTime;
			float fSecs = tDelta / (float)MSECS_PER_SEC;

			for ( int nMesh=0; nMesh < nMeshCount; ++nMesh )
			{
				for ( int nUpdateChannel = 0; nUpdateChannel < NUM_MATERIAL_SCROLL_CHANNELS; ++nUpdateChannel )
				{
					pfPhaseU = &pDefData->pvScrollingUVPhases[ nMesh * NUM_MATERIAL_SCROLL_CHANNELS + nUpdateChannel ].x;
					pfPhaseV = &pDefData->pvScrollingUVPhases[ nMesh * NUM_MATERIAL_SCROLL_CHANNELS + nUpdateChannel ].y;

					sUpdateMeshScrollPhase( *pfPhaseU, fSecs, pMaterial->fScrollRateU[ nUpdateChannel ] );
					sUpdateMeshScrollPhase( *pfPhaseV, fSecs, pMaterial->fScrollRateV[ nUpdateChannel ] );
				}
			}

			pDefData->tScrollLastUpdateTime = tCurTime;
		}

		*pfOutPhaseU = pDefData->pvScrollingUVPhases[ nCurMesh * NUM_MATERIAL_SCROLL_CHANNELS + nChannel ].x;
		*pfOutPhaseV = pDefData->pvScrollingUVPhases[ nCurMesh * NUM_MATERIAL_SCROLL_CHANNELS + nChannel ].y;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectSetScrollingTextureParams(
	D3D_MODEL * pModel,
	int nLOD,
	int nMesh,
	int nMeshCount,
	MATERIAL * pMaterial,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique ) 
{
	ASSERT_RETFAIL( pEffect );
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, EFFECT_PARAM_SCROLL_TEXTURES );
	if ( ! EFFECT_INTERFACE_ISVALID( hParam ) )
		return S_FALSE;

	// if not already initialized, do so now
	V_RETURN( e_ModelInitScrollingMaterialPhases( pModel->nId, FALSE ) );

	// tile_u, tile_v, phase_u, phase_v
	struct ScrollData
	{
		float fTileU;
		float fTileV;
		float fPhaseU;
		float fPhaseV;
	};
	ScrollData pfScrollData[ NUM_MATERIAL_SCROLL_CHANNELS ] = {0};
	//memset( pfScrollData, 0, sizeof(pfScrollData) );

	if ( ! IS_VALID_INDEX( nLOD, LOD_COUNT ) )
		return S_FALSE;

	for ( int nChannel = 0; nChannel < NUM_MATERIAL_SCROLL_CHANNELS; ++nChannel )
	{
		pfScrollData[ nChannel ].fTileU = pMaterial->fScrollTileU[ nChannel ];
		pfScrollData[ nChannel ].fTileV = pMaterial->fScrollTileV[ nChannel ];
		sUpdateAndGetScrollPhaseForChannel( pModel, nMesh, nMeshCount, nLOD, pMaterial, nChannel, &pfScrollData[ nChannel ].fPhaseU, &pfScrollData[ nChannel ].fPhaseV );
	}

	//dx9_EffectSetFloat( pEffect, tTechnique, "gfNormalPower", sinf( fPhase ) );

	// Internally calls dx9_EffectSetRawValue in DX10
	V_RETURN( dx9_EffectSetValue( pEffect, tTechnique, EFFECT_PARAM_SCROLL_TEXTURES, (float*)pfScrollData, 4 * sizeof(float) * NUM_MATERIAL_SCROLL_CHANNELS ) );
	//dx9_EffectSetRawValue( pEffect, tTechnique, EFFECT_PARAM_SCROLL_TEXTURES, (float*)&pfScrollData, 0, 4 * sizeof(float) * NUM_MATERIAL_SCROLL_CHANNELS );

	V_RETURN( dx9_EffectSetFloatArray( pEffect, tTechnique, EFFECT_PARAM_SCROLL_ACTIVE, pMaterial->fScrollAmt, NUM_SCROLLING_SAMPLER_TYPES ) );

	V_RETURN( dx9_ResetSamplerStates() );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O > PRESULT sEffectSetSHCoefs (
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	SH_COEFS_RGB<O> * pSHCoefs );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<> PRESULT sEffectSetSHCoefs<3> (
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	SH_COEFS_RGB<3> * pSHCoefs )
{
	// Order 3 - 9 coefs

	ASSERT_RETFAIL( pEffect );
	if ( ! dx9_EffectUsesSH_O3( pEffect, tTechnique ) )
		return S_FALSE;

	ASSERT_RETFAIL( pSHCoefs->nOrder <= MAX_SUPPORTED_SH_ORDER );

	float* fLight[3] = { pSHCoefs->pfRed, pSHCoefs->pfGreen, pSHCoefs->pfBlue };

	// Lighting environment coefficients
	D3DXVECTOR4 vCoefficients[3];

	// These constants are described in the article by Peter-Pike Sloan titled 
	// "Efficient Evaluation of Irradiance Environment Maps" in the book 
	// "ShaderX 2 - Shader Programming Tips and Tricks" by Wolfgang F. Engel.
	static const float s_fSqrtPI = (sqrtf(PI));
	const float fC0 = 1.0f/(2.0f*s_fSqrtPI);
	const float fC1 = sqrtf(3.0f)/(3.0f*s_fSqrtPI);
	const float fC2 = sqrtf(15.0f)/(8.0f*s_fSqrtPI);
	const float fC3 = sqrtf(5.0f)/(16.0f*s_fSqrtPI);
	const float fC4 = 0.5f*fC2;


	int iChannel;
	for( iChannel=0; iChannel<3; iChannel++ )
	{
		vCoefficients[iChannel].x = -fC1*fLight[iChannel][3];
		vCoefficients[iChannel].y = -fC1*fLight[iChannel][1];
		vCoefficients[iChannel].z =  fC1*fLight[iChannel][2];
		vCoefficients[iChannel].w =  fC0*fLight[iChannel][0] - fC3*fLight[iChannel][6];
	}

	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAR, &vCoefficients[0] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAG, &vCoefficients[1] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAB, &vCoefficients[2] ) );

	for( iChannel=0; iChannel<3; iChannel++ )
	{
		vCoefficients[iChannel].x =      fC2*fLight[iChannel][4];
		vCoefficients[iChannel].y =     -fC2*fLight[iChannel][5];
		vCoefficients[iChannel].z = 3.0f*fC3*fLight[iChannel][6];
		vCoefficients[iChannel].w =     -fC2*fLight[iChannel][7];
	}

	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CBR, &vCoefficients[0] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CBG, &vCoefficients[1] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CBB, &vCoefficients[2] ) );

	vCoefficients[0].x = fC4*fLight[0][8];
	vCoefficients[0].y = fC4*fLight[1][8];
	vCoefficients[0].z = fC4*fLight[2][8];
	vCoefficients[0].w = 1.0f;					// unused in the shader

	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CC, &vCoefficients[0] ) );

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<> PRESULT sEffectSetSHCoefs<2> (
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	SH_COEFS_RGB<2> * pSHCoefs )
{
	// Order 2 -- 4 coefs

	ASSERT_RETFAIL( pEffect );
	if ( ! dx9_EffectUsesSH_O2( pEffect, tTechnique ) )
		return S_FALSE;

	ASSERT_RETFAIL( pSHCoefs->nOrder <= MAX_SUPPORTED_SH_ORDER );

	float* fLight[3] = { pSHCoefs->pfRed, pSHCoefs->pfGreen, pSHCoefs->pfBlue };

	// Lighting environment coefficients
	D3DXVECTOR4 vCoefficients[3];

	// These constants are described in the article by Peter-Pike Sloan titled 
	// "Efficient Evaluation of Irradiance Environment Maps" in the book 
	// "ShaderX 2 - Shader Programming Tips and Tricks" by Wolfgang F. Engel.
	static const float s_fSqrtPI = ((float)sqrtf(PI));
	const float fC0 = 1.0f/(2.0f*s_fSqrtPI);
	const float fC1 = sqrtf(3.0f)/(3.0f*s_fSqrtPI);

	int iChannel;
	for( iChannel=0; iChannel<3; iChannel++ )
	{
		vCoefficients[iChannel].x = -fC1*fLight[iChannel][3];
		vCoefficients[iChannel].y = -fC1*fLight[iChannel][1];
		vCoefficients[iChannel].z =  fC1*fLight[iChannel][2];
		vCoefficients[iChannel].w =  fC0*fLight[iChannel][0];
	}

	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAR, &vCoefficients[0] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAG, &vCoefficients[1] ) );
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SH_CAB, &vCoefficients[2] ) );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_EffectGetScaledMeshLights(
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	struct MODEL_RENDER_LIGHT_DATA & tLights,
	struct MESH_RENDER_LIGHT_DATA & tMeshLights )
{
	// a number of point lights are sometimes applied per-vert or per-pixel -- skip them in SH
	ASSERT_RETFAIL( pEffect );
	ASSERT_RETFAIL( pEffectDef );

	tMeshLights.nRealLights		= 0;
	tMeshLights.nSplitLights	= 0;
	//tMeshLights.nSHLights		= 0;
	tMeshLights.fSplitRealStr	= 0.f;
	tMeshLights.fSplitSHStr		= 0.f;

	if (    ! dx9_EffectUsesSH_O2( pEffect, tTechnique ) 
		 && ! dx9_EffectUsesSH_O3( pEffect, tTechnique ) )
	{
		return S_FALSE;
	}

	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_BG_SH_COEFS ) )
		return S_FALSE;
	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS ) )
		return S_FALSE;

	if ( 0 >= tLights.ModelPoint.nLights )
		return S_FALSE;

	int nMaxRealLights = tTechnique.tFeatures.nInts[ TF_INT_MODEL_POINTLIGHTS ];
	if ( 0 >= nMaxRealLights )
		return S_OK;

	// only (nMaxRealLights - 1) slots are for full real lights -- the last is for a split light
	nMaxRealLights -= 1;

	if ( tLights.ModelPoint.nLights <= nMaxRealLights )
		return S_OK;

	// specify one split light if there are more total lights than the shader can handle
	tMeshLights.nSplitLights++;

	// the rest are full real shader lights
	tMeshLights.nRealLights = nMaxRealLights;

	// use the next light to get the relative strengths for the split light (for smooth fade purposes)
	int nSplitLightIndex = tMeshLights.nRealLights;

	// get the strengths -- color.w * getfalloff()
	// split strength relative to the two surrounding lights
	float fRealStr	= 1.f;
	float fSHStr	= 0.f;
	if ( ( nSplitLightIndex + 1 ) < tLights.ModelPoint.nLights )
	{
		fSHStr = tLights.pfModelPointLightLODStrengths[ nSplitLightIndex + 1 ];
	}

	float fSplitStr = tLights.pfModelPointLightLODStrengths[ nSplitLightIndex ];
	ASSERT( fSplitStr >= fSHStr );

	if ( fSplitStr > fSHStr && nSplitLightIndex == 0 )
	{
		// if there aren't any real lights, this is simple
		tMeshLights.fSplitRealStr = SATURATE( fSplitStr - fSHStr );
	}
	else if ( nSplitLightIndex > 0 )
	{
		// if there is a real light above the split light, there's a ceiling for a lerp
		fRealStr = tLights.pfModelPointLightLODStrengths[ nSplitLightIndex - 1 ];
		ASSERT( fSplitStr <= fRealStr );
		if ( ( fSHStr - fRealStr ) != 0.f ) 
			tMeshLights.fSplitRealStr = MAP_VALUE_TO_RANGE( fSplitStr, fSHStr, fRealStr, 0.f, 1.f );
		else
			tMeshLights.fSplitRealStr = 0.f;
	}
	else
	{
		tMeshLights.fSplitRealStr = 0.f;
	}

	tMeshLights.fSplitSHStr = 1.f - tMeshLights.fSplitRealStr;

	//keytrace( "real %d, split %d, realstr %3.2f, splitstr %3.2f, shstr %3.2f, split_real %3.1f, split_sh %3.1f\n",
	//	tMeshLights.nRealLights, tMeshLights.nSplitLights, fRealStr, fSplitStr, fSHStr, tMeshLights.fSplitRealStr, tMeshLights.fSplitSHStr );

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sEffectGetDirLightColor(
	VECTOR4 & vColor,
	ENV_DIR_LIGHT_TYPE eDir,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const D3D_MODEL * pModel,
	float fTransitionStr,
	DWORD dwFlags,
	BOOL bForceCurrentEnv )
{
	ASSERT_RETINVALIDARG( pEnvDef );
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETINVALIDARG( pEffectDef );

	e_GetDirLightColor( pEnvDef, eDir, &vColor, bForceCurrentEnv );

	if ( AppIsTugboat() )
	{
		//vLightDirectionalColor[ i ] *= e_GetDirLightIntensity( pEnvDef, (ENV_DIR_LIGHT_TYPE)i, bForceCurrentEnv );// * fModifier;
		float fIntensity = e_GetDirLightIntensity( pEnvDef, eDir, bForceCurrentEnv );// * fModifier;

		if ( dwFlags & EFFECT_SET_DIR_DONT_MULTIPLY_INTENSITY )
			vColor.w = fIntensity;
		else
			vColor *= fIntensity;

		if ( pModel )
		{
			if( pModel->bFullbright )
			{
				vColor *= 4.0f * ( 1.0f - pModel->HitColor );
				//vColor.x += 4.0f;
			}
			vColor *= pModel->AmbientLight;
			vColor.x += pModel->HitColor;			
		}
	}
	else
	{
		float fModifier = 1.f;
		//if ( eDir == ENV_DIR_LIGHT_PRIMARY_SPECULAR && pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_LOD_TRANS_SPECULAR ) && fTransitionStr < 1.f )
		//	fModifier = fTransitionStr;

		float fIntensity = e_GetDirLightIntensity( pEnvDef, eDir, bForceCurrentEnv ) * fModifier;

		if ( dwFlags & EFFECT_SET_DIR_DONT_MULTIPLY_INTENSITY )
			vColor.w = fIntensity;
		else
			vColor *= fIntensity;
	}

	if ( e_ColorsTranslateSRGB() )
	{
		V( e_SRGBToLinear( (VECTOR3*)&vColor ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sEffectGetDirLightVector(
	VECTOR4 & vLightDir,
	ENV_DIR_LIGHT_TYPE eDir,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const MATRIX * pmTransform = NULL,
	BOOL bNegateForShader = TRUE,
	BOOL bForceCurrentEnv = FALSE )
{
	float fNegate = 1.f;
	if ( eDir == ENV_DIR_LIGHT_SECONDARY_DIFFUSE && pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_DIR1_OPPOSITE_DIR0 )
	{
		eDir = ENV_DIR_LIGHT_PRIMARY_DIFFUSE;
		fNegate = -1.f;
	}

	e_GetDirLightVector( pEnvDef, eDir, (VECTOR3*)&vLightDir, bForceCurrentEnv );
	if ( pmTransform )
		MatrixMultiplyNormal( (VECTOR3*)&vLightDir, (VECTOR3*)&vLightDir, pmTransform );

	VectorNormalize( *(VECTOR3*)&vLightDir );

	// negate to simpify dot product math on shader
	if ( bNegateForShader )
		fNegate *= -1.f;

	// in case this is the secondary light and we want the opposite of the primary
	vLightDir *= fNegate;
	vLightDir.w = 1.f;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template< int O > PRESULT sEffectSetSHLightingParams(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const SH_COEFS_RGB<O> * ptPointCoefs,
	const SH_COEFS_RGB<O> & tBaseCoefs,
	const MESH_RENDER_LIGHT_DATA * ptMeshLights,
	BOOL bForceCurrentEnv )
{
	// a number of point lights are sometimes applied per-vert or per-pixel -- skip them in SH
	ASSERT_RETFAIL( pEffectDef );
	ASSERT_RETFAIL( ptMeshLights );

	int nStartLights = 0;

	BOOL bNormalMap			= !!( tTechnique.tFeatures.dwFlags & TECHNIQUE_FLAG_MODEL_NORMALMAP );
	BOOL bWorldSpaceLights	= !!( tTechnique.tFeatures.dwFlags & TECHNIQUE_FLAG_MODEL_WORLDSPACELIGHTS );
	BOOL bSkipLights = bNormalMap && ! bWorldSpaceLights;

	if ( bSkipLights )
	{
		// if normal mapped and pixel shader in tangent space, send down the top x diffuse lights separately
		nStartLights = tTechnique.tFeatures.nInts[ TF_INT_MODEL_POINTLIGHTS ];
		ASSERT( nStartLights >= ( ptMeshLights->nRealLights + ptMeshLights->nSplitLights ) );
	}

	SH_COEFS_RGB<O> tCoefs;
	dx9_SHCoefsInit(&tCoefs);	// CHB 2007.09.04 - Fixes "city block fully lit" issue

	// debug
	//int nStartLights = 0;
	BOOL bUseBGCoefs = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_BG_SH_COEFS );

	if ( !bUseBGCoefs )
	{
		tCoefs = tBaseCoefs;	// CHB 2007.09.04 - Fixes "city block fully lit" issue

		// add in the non-per-vert/pixel lights that aren't in the base coefs
		for ( int i = nStartLights; i < MAX_POINT_LIGHTS_PER_EFFECT; i++ )
		{
			V( dx9_SHCoefsAdd( &tCoefs, &tCoefs, &ptPointCoefs[ i ] ) );
		}

		// add the scaled SH-portion of the split light
		if ( bSkipLights && ptMeshLights->nSplitLights > 0 )
		{
			// this will end up getting translated into tangent space for the ps
			ASSERT( ptMeshLights->nSplitLights == 1 );
			SH_COEFS_RGB<O> tSplitCoefs;
			V( dx9_SHCoefsScale( &tSplitCoefs, &ptPointCoefs[ ptMeshLights->nRealLights ], ptMeshLights->fSplitSHStr ) );
			V( dx9_SHCoefsAdd( &tCoefs, &tCoefs, &tSplitCoefs ) );
		}
	}


	// scale point light intensities only (not background env map or dir light intensities)
	V( dx9_SHCoefsScale( &tCoefs, &tCoefs, LIGHT_INTENSITY_MDR_SCALE ) );


	if ( pEnvDef )
	{
		BOOL bLinear = e_ColorsTranslateSRGB();

		// from states, etc.
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_HAS_BASE_SH ) )
		{
			V( dx9_SHCoefsAdd( &tCoefs, &tCoefs, dxC_ModelGetBaseSHCoefs<O>( pModel, bLinear ) ) );
		}

		// background coefs
		SH_COEFS_RGB<O> tEnvCoefs;
		if ( bUseBGCoefs && pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS )
		{
			V( dx9_SHCoefsScale( &tEnvCoefs, dx9_EnvGetBackgroundSHCoefs<O> ( pEnvDef, bLinear ), pEnvDef->fBackgroundSHIntensity ) );
			V( dx9_SHCoefsAdd( &tCoefs, &tCoefs, &tEnvCoefs ) );
		}
		if ( ! bUseBGCoefs && pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS )
		{
			V( dx9_SHCoefsScale( &tEnvCoefs, dx9_EnvGetAppearanceSHCoefs<O> ( pEnvDef, bLinear ), pEnvDef->fAppearanceSHIntensity ) );
			V( dx9_SHCoefsAdd( &tCoefs, &tCoefs, &tEnvCoefs ) );
		}
	}

	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_DIRECTIONAL_IN_SH ) )
	{
		// add diffuse directional light
		for ( int i = 0; i < NUM_ENV_DIR_LIGHTS; i++ )
		{
			ENV_DIR_LIGHT_TYPE eDir = (ENV_DIR_LIGHT_TYPE)i;
			if ( eDir != ENV_DIR_LIGHT_PRIMARY_DIFFUSE && eDir != ENV_DIR_LIGHT_SECONDARY_DIFFUSE )
				continue;


			// get color
			VECTOR4 vColor;
			V_CONTINUE( sEffectGetDirLightColor( vColor, eDir, pEnvDef, pEffect, pEffectDef, pModel, 1.f, 0, bForceCurrentEnv ) );

			// get direction
			VECTOR4 vLightDir;
			V_CONTINUE( sEffectGetDirLightVector( vLightDir, eDir, pEnvDef, NULL, FALSE, bForceCurrentEnv ) );

			V( dx9_SHCoefsAddDirectionalLight( &tCoefs, vColor, *(VECTOR3*)&vLightDir ) );
		}
	}


	V_RETURN( sEffectSetSHCoefs( pEffect, tTechnique, &tCoefs ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectSetSHLightingParams(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const struct MODEL_RENDER_LIGHT_DATA * ptLights,
	const struct MESH_RENDER_LIGHT_DATA * ptMeshLights,
	BOOL bForceCurrentEnv /*= FALSE*/ )
{
	if ( dx9_EffectUsesSH_O2( pEffect, tTechnique ) )
	{
		V_RETURN( sEffectSetSHLightingParams( pModel, pEffect, pEffectDef, tTechnique, pEnvDef,
			ptLights->tSHCoefsPointLights_O2, ptLights->tSHCoefsBase_O2, ptMeshLights, bForceCurrentEnv ) );
	}

	if ( dx9_EffectUsesSH_O3( pEffect, tTechnique ) )
	{
		V_RETURN( sEffectSetSHLightingParams( pModel, pEffect, pEffectDef, tTechnique, pEnvDef,
			ptLights->tSHCoefsPointLights_O3, ptLights->tSHCoefsBase_O3, ptMeshLights, bForceCurrentEnv ) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectSetHemiLightingParams(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef )
{
	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_HEMI_LIGHT_COLORS ) )
	{
		VECTOR4 vColors[2];
		e_GetHemiLightColors( pEnvDef, vColors );
		// pre-multiply the intensity to save shader work
		*(VECTOR3*)&vColors[0].x *= vColors[0].w;
		*(VECTOR3*)&vColors[1].x *= vColors[1].w;
		V_RETURN( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_HEMI_LIGHT_COLORS, (D3DXVECTOR4*)vColors, 2 ) );
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetDirectionalLightingParams(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * const NOALIAS pEffect,
	const EFFECT_DEFINITION * const NOALIAS pEffectDef,
	const MATRIX * const NOALIAS pmWorldToObject,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * const NOALIAS pEnvDef,
	DWORD dwFlags,
	float fTransitionStr,
	BOOL bForceCurrentEnv )
{
	EFFECT_PARAM tSpaces[ NUM_COORDINATE_SPACES ] =
	{
		EFFECT_PARAM_DIRLIGHTDIR_OBJECT,
		EFFECT_PARAM_DIRLIGHTDIR_WORLD,
	};

	const MATRIX * pmMats[ NUM_COORDINATE_SPACES ] = { pmWorldToObject, NULL };

	VECTOR4 vDir[ NUM_ENV_DIR_LIGHTS ];

	if ( ! TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_DIRECTIONAL_IN_SH ) )
	{
		for ( int nSpace = 0; nSpace < NUM_COORDINATE_SPACES; nSpace++ )
		{
			if ( dx9_EffectIsParamUsed( pEffect, tTechnique, tSpaces[ nSpace ] ) )
			{
				for ( int i = 0; i < NUM_ENV_DIR_LIGHTS; i++ )
				{
					V_CONTINUE( sEffectGetDirLightVector(
						vDir[i],
						(ENV_DIR_LIGHT_TYPE)i,
						pEnvDef,
						pmMats[ nSpace ],
						TRUE,
						bForceCurrentEnv ) );
				}

				V_RETURN( dx9_EffectSetVectorArray( pEffect, tTechnique, tSpaces[ nSpace ], (D3DXVECTOR4*)vDir, NUM_ENV_DIR_LIGHTS ) );
			}
		}

		if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_DIRLIGHTCOLOR ) )
		{
			VECTOR4 vColor[ NUM_ENV_DIR_LIGHTS ];

			for ( int i = 0; i < NUM_ENV_DIR_LIGHTS; i++ )
			{
				V_CONTINUE( sEffectGetDirLightColor( vColor[i], (ENV_DIR_LIGHT_TYPE)i, pEnvDef, pEffect, pEffectDef, pModel, fTransitionStr, dwFlags, bForceCurrentEnv ) );
			}

			V_RETURN( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_DIRLIGHTCOLOR, (D3DXVECTOR4*)vColor, NUM_ENV_DIR_LIGHTS ) );
		}
	}

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_LIGHTAMBIENT ) )
	{
		D3DXVECTOR4 vLightAmbient;
		e_GetAmbientColor( pEnvDef, (VECTOR4*)&vLightAmbient, bForceCurrentEnv );
		vLightAmbient *= e_GetAmbientIntensity( pEnvDef, bForceCurrentEnv );

		if ( AppIsTugboat() )
		{
			if( pModel->bFullbright )
			{
				vLightAmbient *= 2.0f * ( 1.0f - pModel->HitColor );
				//vLightAmbient.x += 4.0f;
			}
			vLightAmbient *= pModel->AmbientLight;
			vLightAmbient.x += pModel->HitColor;
		}

		vLightAmbient.w = 0.f;

		V_RETURN( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_LIGHTAMBIENT, &vLightAmbient) );
	}

	V( dx9_EffectSetHemiLightingParams(
		pEffect,
		tTechnique,
		pEnvDef ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EffectSetSpotlightParams(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const EFFECT_DEFINITION * pEffectDef,
	const MATRIX * pmWorldToObject,
	const D3D_MODEL * pModel,
	const D3D_LIGHT * pLight )
{
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETINVALIDARG( pLight );
	LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
	float fCos = 0.25f;
	if ( pLightDef )
		fCos = pLightDef->fSpotAngleDeg / 180.f;

	struct
	{
		EFFECT_PARAM hPos;
		EFFECT_PARAM hNormal;
		EFFECT_PARAM hFalloff;
	}
	tSpaces[ NUM_COORDINATE_SPACES ] =
	{
		{	EFFECT_PARAM_SPOTLIGHTPOS_OBJECT,	EFFECT_PARAM_SPOTLIGHTNORMAL_OBJECT,	EFFECT_PARAM_SPOTLIGHTFALLOFF_OBJECT	},
		{	EFFECT_PARAM_SPOTLIGHTPOS_WORLD,	EFFECT_PARAM_SPOTLIGHTNORMAL_WORLD,		EFFECT_PARAM_SPOTLIGHTFALLOFF_WORLD		},
	};

	const MATRIX * pmMats[ NUM_COORDINATE_SPACES ] = { pmWorldToObject, NULL };

	const unsigned nSpotLights = 1;

	for ( int nSpace = 0; nSpace < NUM_COORDINATE_SPACES; nSpace++ )
	{
		D3DXVECTOR4 vPos( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.f );
		if ( pmMats[ nSpace ] )
			D3DXVec3Transform( &vPos, (D3DXVECTOR3*)&vPos, (D3DXMATRIXA16*)pmMats[ nSpace ] );

		D3DXVECTOR4 vNormal( pLight->vDirection.x, pLight->vDirection.y, pLight->vDirection.z, 0 );
		if ( pmMats[ nSpace ] )
			D3DXVec3TransformNormal( (D3DXVECTOR3*)&vNormal, (D3DXVECTOR3*)&vNormal, (D3DXMATRIXA16*)pmMats[ nSpace ] );

		D3DXVECTOR4 vFalloff;
		vFalloff.x = pLight->vFalloff.x;	// base brightness
		vFalloff.y = pLight->vFalloff.y;	// linear falloff coefficient
		if ( nSpace == WORLD_SPACE && pModel )
		{
			vFalloff.y *= pModel->vScale.fX;
		}
		// pre-calc for spotlight umbra falloff
		vFalloff.z = fCos - 1.f;
		vFalloff.w = 1.f / fCos;

		V( dx9_EffectSetVectorArray( pEffect, tTechnique, tSpaces[ nSpace ].hPos, &vPos, nSpotLights ) );
		V( dx9_EffectSetVectorArray( pEffect, tTechnique, tSpaces[ nSpace ].hNormal, &vNormal, nSpotLights ) );
		V( dx9_EffectSetVectorArray( pEffect, tTechnique, tSpaces[ nSpace ].hFalloff, &vFalloff, nSpotLights ) );
	}

	D3DXVECTOR4 vColor( pLight->vDiffuse.x, pLight->vDiffuse.y, pLight->vDiffuse.z, pLight->vDiffuse.w );
	V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPOTLIGHTCOLOR, &vColor, nSpotLights ) );

	if ( pLightDef && e_TextureIsLoaded( pLightDef->nSpotUmbraTextureID, TRUE ) )
	{
		//D3D_TEXTURE * pTexture = dxC_TextureGet( pLightDef->nSpotUmbraTextureID );
		//gpDebugTexture = pTexture->pD3DTexture;
		V( dxC_SetTexture(
			pLightDef->nSpotUmbraTextureID,
			tTechnique,
			pEffect,
			EFFECT_PARAM_SPECULARLOOKUP,
			INVALID_ID ) );
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_EffectSetFogParameters( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	float fScale )
{
	ASSERT_RETINVALIDARG( pEffect );

	float fFogNear, fFogFar;
	e_GetFogDistances( fFogNear, fFogFar );
	fScale = 1.f / fScale;
	fFogNear *= fScale;  // get fog plane distances into object scale (not world scale)
	fFogFar  *= fScale;
	V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_FOGDISTANCEMAX, fFogFar ) );
	V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_FOGDISTANCEMIN, fFogNear ) );


	// don't need these with shaders

	// we output 1..0 fog distances in our shaders  -- should probably output real ranges for fixed-func compat
	//float fFar = 0.5f;
	//HRVERIFY( dxC_SetRenderState( D3DRS_FOGEND, *(DWORD*)&fFar ) );
	//float fNear = 1.0f;
	//HRVERIFY( dxC_SetRenderState( D3DRS_FOGSTART, *(DWORD*)&fNear ) );


	DWORD dwFogColor = e_GetFogColor();
	D3DXVECTOR4 vFogColor(
		float( ( dwFogColor & 0x00ff0000 ) >> 16 ) / 255.0f,
		float( ( dwFogColor & 0x0000ff00 ) >>  8 ) / 255.0f,
		float( ( dwFogColor & 0x000000ff )       ) / 255.0f,
		0.0f );
	V( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_FOGCOLOR, &vFogColor ) );
	//dxC_SetRenderState( D3DRS_FOGCOLOR, dwFogColor );
	//dxC_SetRenderState( D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR );

	BOOL bFogEnabled = e_GetFogEnabled();
	// set fog factor 0.f to enable fog, 1.f to disable
	V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_FOG_FACTOR, bFogEnabled ? 0.f : 1.f ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_EffectSetScreenSize( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	RENDER_TARGET_INDEX nOverrideRT /*= RENDER_TARGET_INVALID*/ )
{
	ASSERT_RETINVALIDARG( pEffect );
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, EFFECT_PARAM_SCREENSIZE );
	if ( ! EFFECT_INTERFACE_ISVALID( hParam ) )
		return S_FALSE;

	D3DXVECTOR4 vSize;
	int nWidth, nHeight;
	if ( nOverrideRT == RENDER_TARGET_INVALID )
		nOverrideRT = dxC_GetCurrentRenderTarget();
	DX9_BLOCK( ASSERT_RETFAIL( nOverrideRT != RENDER_TARGET_NULL ) );  //Can't have a NULL RT with DX9

	V( dxC_GetRenderTargetDimensions( nWidth, nHeight, nOverrideRT ) );
	vSize.x = (FLOAT)nWidth;
	vSize.y = (FLOAT)nHeight;
	vSize.z = 1.f / nWidth;
	vSize.w = 1.f / nHeight;

	dx9_EffectSetVector( pEffect, hParam, &vSize );
	//DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( hParam, &vSize ) ); )
	//DX10_BLOCK( V_RETURN( hParam->AsVector()->SetFloatVector( (float*) &vSize ) ); )
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_EffectSetViewport( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	RENDER_TARGET_INDEX nOverrideRT /*= RENDER_TARGET_INVALID*/ )
{
	ASSERT_RETINVALIDARG( pEffect );
	const EFFECT_PARAM efParam = EFFECT_PARAM_VIEWPORT;
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, efParam );
	if ( ! EFFECT_INTERFACE_ISVALID( hParam ) )
		return S_FALSE;

 	D3DXVECTOR4 vSize;

	BOUNDING_BOX* pViewportOverride = e_GetViewportOverride();
	if( pViewportOverride )
	{
		vSize.x = pViewportOverride->vMin.fX;
		vSize.y = pViewportOverride->vMin.fY;
		vSize.z = pViewportOverride->vMax.fX;
		vSize.w = pViewportOverride->vMax.fY;
	}
	else
	{
		int nWidth, nHeight;
		if ( nOverrideRT == RENDER_TARGET_INVALID )
			nOverrideRT = dxC_GetCurrentRenderTarget();
		V( dxC_GetRenderTargetDimensions( nWidth, nHeight, nOverrideRT ) );
		vSize.x = 0;
		vSize.y = 0;
		vSize.z = (FLOAT)nWidth;
		vSize.w = (FLOAT)nHeight;
	}

	dx9_EffectSetVector( pEffect, tTechnique, efParam, &vSize );
	//DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( hParam, &vSize ) ); )
	//DX10_BLOCK( V_RETURN( hParam->AsVector()->SetFloatVector( (float*) &vSize ) ); )
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_EffectSetPixelAccurateOffset( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique )
{
	ASSERT_RETINVALIDARG( pEffect );
	const EFFECT_PARAM efParam = EFFECT_PARAM_PIXEL_ACCURATE_OFFSET;
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, efParam );
	if ( ! EFFECT_INTERFACE_ISVALID( hParam ) )
		return S_FALSE;

	D3DXVECTOR4 vOffset;
	int nWidth, nHeight;
	V( dxC_GetRenderTargetDimensions( nWidth, nHeight, dxC_GetCurrentRenderTarget() ) );
	vOffset.x = DXC_9_10( 0.5f, 0.0f) / nWidth;
	vOffset.y = DXC_9_10( 0.5f, 0.0f) / nHeight;
	vOffset.z = 0.f;
	vOffset.w = 0.f;

	dx9_EffectSetVector( pEffect, tTechnique, efParam, &vOffset );
	//DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( hParam, &vOffset ) ); )
	//DX10_BLOCK( V_RETURN( hParam->AsVector()->SetFloatVector( (float*) &vOffset ) ); )
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_EffectSetShadowSize( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	int nWidth, 
	int nHeight )
{
	ASSERT_RETINVALIDARG( pEffect );
	const EFFECT_PARAM efParam = EFFECT_PARAM_SHADOWSIZE;
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, efParam );
	if ( ! EFFECT_INTERFACE_ISVALID( hParam ) )
		return S_FALSE;

	D3DXVECTOR4 vSize;
	vSize.x = (FLOAT)nWidth;
	vSize.y = (FLOAT)nHeight;
	vSize.z = 1.f / nWidth;
	vSize.w = 1.f / nHeight;

	dx9_EffectSetVector( pEffect, tTechnique, efParam, &vSize );
	//DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetVector( hParam, &vSize ) ); )
	//DX10_BLOCK( V_RETURN( hParam->AsVector()->SetFloatVector( (float*) &vSize ) ); )
	
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_EffectsLoadAll()
{
	// load all effects, from both code and the spreadsheet -- mainly for fillpak
	V( dx9_EffectCreateNonMaterialEffects() );

	int nRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES );
	for ( int i = 0; i < nRows; i++ )
	{
		EFFECT_DEFINITION * pDef = (EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, i );
		ASSERT_CONTINUE( pDef );
		int nEffectID;
		V_CONTINUE( dx9_EffectNew( &nEffectID, i ) );
		ASSERT( nEffectID != INVALID_ID );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
static void sEffectFillpakCallback(
	const FILE_ITERATE_RESULT * resultFileFind,
	void * userData)
{
	ASSERT_RETURN(resultFileFind && resultFileFind->szFilenameRelativepath);

	trace( "Fillpak Effect: %s\n", resultFileFind->szFilenameRelativepath );

	DECLARE_LOAD_SPEC( spec, resultFileFind->szFilenameRelativepath );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	//spec.flags |= PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;	// CHB 2007.01.02
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	spec.priority = ASYNC_PRIORITY_EFFECTS;
	spec.fpLoadingThreadCallback = NULL;
	spec.callbackData = NULL;

	PakFileLoadNow(spec);
}*/


void sEffectFillpakCallback(
	LPCSTR filename)
{
	DECLARE_LOAD_SPEC(spec, filename);
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	//spec.flags |= PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;	// CHB 2007.01.02
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	spec.priority = ASYNC_PRIORITY_EFFECTS;
	spec.fpLoadingThreadCallback = NULL;
	spec.callbackData = NULL;

	PakFileLoadNow(spec);
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sEffectFillMinpakCallback(
	const FILE_ITERATE_RESULT * resultFileFind,
	void * userData)
{
	ASSERT_RETURN(resultFileFind && resultFileFind->szFilenameRelativepath);

	trace( "Fillpak Effect: %s\n", resultFileFind->szFilenameRelativepath );

	DECLARE_LOAD_SPEC( spec, resultFileFind->szFilenameRelativepath );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	//spec.flags |= PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;	// CHB 2007.01.02
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_PAK;
	spec.priority = ASYNC_PRIORITY_EFFECTS;
	spec.fpLoadingThreadCallback = NULL;
	spec.callbackData = NULL;

	PakFileLoadNow(spec);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_EffectsFillpak()
{
	FillPak_LoadEffects();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_EffectsFillMinpak()
{
	FilesIterateInDirectory( FILE_PATH_EFFECT,		  "*.fxo", sEffectFillMinpakCallback, NULL, TRUE);
	FilesIterateInDirectory( FILE_PATH_EFFECT_COMMON, "*.fxo", sEffectFillMinpakCallback, NULL, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace
{

class LocalEventHandler : public OptionState_EventHandler
{
	public:
		virtual void QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut) override;
};

void LocalEventHandler::QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut)
{
	// See if any changes were made.
#ifdef ENGINE_TARGET_DX10
	if (   pOldState->get_bDX10ScreenFX() != pNewState->get_bDX10ScreenFX()
		|| pOldState->get_bFluidEffects() != pNewState->get_bFluidEffects() )
	{
		// This will force a re-creation of the render targets.
		nActionFlagsOut |= SETTING_NEEDS_RESET;
	}
#endif	// DX10

	bool bChanged =
		(pOldState->get_nMaxEffectTarget() != pNewState->get_nMaxEffectTarget());
	if (!bChanged)
	{
		return;
	}

	//
#if 0
	nActionFlagsOut |= SETTING_NEEDS_RESET;
	nActionFlagsOut |= SETTING_NEEDS_FXRELOAD;
#endif
	nActionFlagsOut |= SETTING_NEEDS_APP_EXIT;
}

};	// anonymous namespace

//-------------------------------------------------------------------------------------------------

PRESULT dxC_Effect_RegisterEventHandler(void)
{
	return e_OptionState_RegisterEventHandler(new LocalEventHandler);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)

static PRESULT sAddStringToReport( char *& pBuf, int & nBufAlloc, int & nBufUsed, char * pszFmt, ... )
{
	va_list args;
	va_start(args, pszFmt);

	const int cnMaxLineLen = 512;
	char szLine[ cnMaxLineLen ];
	PStrVprintf( szLine, cnMaxLineLen, pszFmt, args );

	int nLen = PStrLen( szLine );
	if ( nBufUsed + nLen >= nBufAlloc )
	{
		// double size on each realloc
		nBufAlloc *= 2;
		pBuf = (char*) REALLOC( NULL, pBuf, nBufAlloc );
		ASSERT_RETVAL( pBuf, E_OUTOFMEMORY );
	}
	PStrCat( pBuf, szLine, nBufAlloc );
	nBufUsed += nLen;

	return S_OK;
}

static void sPrintToLine( char *& szCur, int & nMaxLenCur, const char * pszFmt, ... )
{
	va_list args;
	va_start(args, pszFmt);

	int nDiff = PStrVprintf( szCur, nMaxLenCur, pszFmt, args );
	szCur += nDiff;
	nMaxLenCur -= nDiff;
	nMaxLenCur = MAX(0, nMaxLenCur);
}

static PRESULT sDumpStats( char *& pBuf, int & nBufAlloc, int & nBufUsed )
{
	const int cnMaxLineLen = 512;
	char szLine[ cnMaxLineLen ];
	int nMaxLenCur = cnMaxLineLen;
	char * szCur = szLine;

	sPrintToLine( szCur, nMaxLenCur, "%s,%d\r\n", "Max FXTGT", (int)sgtEffectStats.eMaxFXTgt );

	for ( int i = 0; i < _countof(sgtEffectStats.dwShaderTypes); ++i )
	{
		sPrintToLine( szCur, nMaxLenCur, "%s,%d\r\n", e_LevelLocationGetName( i ), sgtEffectStats.dwShaderTypes[i] );
	}

	return sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "%s\r\n", szLine );
}

static PRESULT sDumpHeaders( char *& pBuf, int & nBufAlloc, int & nBufUsed )
{
	const int cnMaxLineLen = 512;
	char szLine[ cnMaxLineLen ] = "";

	PStrCat( szLine, "Effect,Index,Selected,", cnMaxLineLen );

	for ( int i = 0; i < NUM_TECHNIQUE_INT_CHARTS; ++i )
	{
		PStrCat( szLine, gtTechniqueIntChart[i].pszName, cnMaxLineLen );
		PStrCat( szLine, ",", cnMaxLineLen );
	}

	for ( int i = 0; i < NUM_TECHNIQUE_FLAG_CHARTS; ++i )
	{
		PStrCat( szLine, gtTechniqueFlagChart[i].pszName, cnMaxLineLen );
		PStrCat( szLine, ",", cnMaxLineLen );
	}

	return sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "%s\r\n", szLine );
}

static PRESULT sDumpTechnique( const D3D_EFFECT * pEffect, int nTech, char *& pBuf, int & nBufAlloc, int & nBufUsed )
{
	const EFFECT_TECHNIQUE * ptTechnique = &pEffect->ptTechniques[ nTech ];

	const int cnMaxLineLen = 512;
	char szLine[ cnMaxLineLen ];
	int nMaxLenCur = cnMaxLineLen;
	char * szCur = szLine;

	sPrintToLine( szCur, nMaxLenCur, "%s,%d,%d,", pEffect->pszFXFileName, nTech, ptTechnique->tStats.dwSelected );

	for ( int i = 0; i < NUM_TECHNIQUE_INT_CHARTS; ++i )
	{
		sPrintToLine( szCur, nMaxLenCur, "%d,", ptTechnique->tFeatures.nInts[i] );
	}

	for ( int i = 0; i < NUM_TECHNIQUE_FLAG_CHARTS; ++i )
	{
		sPrintToLine( szCur, nMaxLenCur, "%d,", !!(ptTechnique->tFeatures.dwFlags & gtTechniqueFlagChart[i].dwFlag) );
	}

	return sAddStringToReport( pBuf, nBufAlloc, nBufUsed, "%s\r\n", szLine );
}

static int sCompareTechniqueReports( const void *p1, const void * p2 )
{
	const TECHNIQUE_REPORT * pE1 = static_cast<const TECHNIQUE_REPORT *>( p1 );
	const TECHNIQUE_REPORT * pE2 = static_cast<const TECHNIQUE_REPORT *>( p2 );

	if ( pE1->dwSelected > pE2->dwSelected )
		return -1;
	if ( pE1->dwSelected < pE2->dwSelected )
		return 1;
	return 0;
}

static PRESULT sReportEffectFile( const D3D_EFFECT * pEffect, char *& pBuf, int & nBufAlloc, int & nBufUsed )
{
	V_RETURN( gptTechniqueReport.Resize( pEffect->nTechniques ) );

	for ( int i = 0; i < pEffect->nTechniques; ++i )
	{
		gptTechniqueReport[i].nIndex     = i;
		gptTechniqueReport[i].dwSelected = pEffect->ptTechniques[i].tStats.dwSelected;
	}

	qsort( (void*)(TECHNIQUE_REPORT*)gptTechniqueReport, pEffect->nTechniques, sizeof(TECHNIQUE_REPORT), sCompareTechniqueReports );


	for ( int i = 0; i < pEffect->nTechniques; ++i )
	{
		V_RETURN( sDumpTechnique( pEffect, gptTechniqueReport[i].nIndex, pBuf, nBufAlloc, nBufUsed ) );
	}

	return S_OK;
}

PRESULT e_DumpEffectReport()
{
	OS_PATH_CHAR szFilename[ MAX_PATH ];
	PStrPrintf( szFilename, MAX_PATH, OS_PATH_TEXT("%s%s"), FilePath_GetSystemPath( FILE_PATH_LOG ), OS_PATH_TEXT("effect_report.csv") );


	int nBufAlloc = 1024;
	char * pBuf   = (char*) MALLOC( NULL, nBufAlloc );
	ASSERT( pBuf );
	pBuf[ 0 ]     = NULL;
	int nBufUsed  = 1;	// for the '\0'

	V_RETURN( sDumpStats( pBuf, nBufAlloc, nBufUsed ) );
	V_RETURN( sDumpHeaders( pBuf, nBufAlloc, nBufUsed ) );

	for ( D3D_EFFECT * pEffect = dxC_EffectGetFirst();
		pEffect;
		pEffect = dxC_EffectGetNext( pEffect ) )
	{
		V_BREAK( sReportEffectFile( pEffect, pBuf, nBufAlloc, nBufUsed ) );
	}

	if ( pBuf )
	{
		pBuf[ nBufUsed-1 ] = NULL;
		FileDelete( szFilename );
		FileSave( szFilename, pBuf, nBufUsed );
		FREE( NULL, pBuf );
	}

	return S_OK;
}
#undef ADDLINE

#else

#endif // DEVELOPMENT