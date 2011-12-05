//----------------------------------------------------------------------------
// e_budget.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "spring.h"
#include "e_texture.h"
#include "appcommon.h"
#include "e_budget.h"
#include "e_optionstate.h"	// CHB 2007.03.07
#include "e_main.h"
#include "e_texture.h"
#include "e_caps.h"
#include "appcommontimer.h"
#include "datatables.h" 
#include "e_settings.h"
#include "prime.h"
#include "game.h"

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
#include "memoryallocatorinternals_i.h"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

TEXTURE_BUDGET gtTextureBudget;
MODEL_BUDGET gtModelBudget;
RESOURCE_REAPER gtReaper;
RESOURCE_UPLOAD_MANAGER gtUploadManager;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------


inline int sGetMIPFactor( TEXTURE_BUDGET_MIP_GROUP ** ppSrcMipRates, TEXTURE_GROUP eGroup, TEXTURE_TYPE eType, float fFactor, float fFactorNormalizer )
{
	int nMipDown = ROUND( ppSrcMipRates[ eGroup ]->fMIP[ eType ] * fFactor * fFactorNormalizer );
	if ( eGroup == TEXTURE_GROUP_UI )
		return MIN( nMipDown, UI_TEXTURE_MAX_MIP_DOWN );
	else
		return MIN( nMipDown, TEXTURE_MAX_MIP_DOWN );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_TextureBudgetGetMIPDown( TEXTURE_GROUP eGroup, TEXTURE_TYPE eType, BOOL bIgnoreReaper /*= FALSE*/ )
{
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( pOverrides && pOverrides->dwFlags & GFX_FLAG_ENABLE_MIP_LOD_OVERRIDES )
	{
		return pOverrides->pnTextureLODDown[ eGroup ][ eType ];
	}
	if ( bIgnoreReaper )
		return gtTextureBudget.nMIPDownUnreaped[ eGroup ][ eType ];
	else
		return gtTextureBudget.nMIPDown[ eGroup ][ eType ];
}

// CHB 2006.10.11
int e_TextureBudgetAdjustWidthAndHeight( TEXTURE_GROUP eGroup, TEXTURE_TYPE eType, int & nWidthInOut, int & nHeightInOut, BOOL bIgnoreReaper /*= FALSE*/ )
{
	int nWidth = nWidthInOut;
	int nHeight = nHeightInOut;

	int nMipDown = e_TextureBudgetGetMIPDown(eGroup, eType, bIgnoreReaper);

	// Minimum dimension of any side of a texture.
	// Anything less than 32 is expected to offer
	// very little or no memory savings.
	int nMinDimension = 64;

	while (nMipDown > 0) {
		int w = e_ResolutionMIP( nWidth, nMipDown );
		int h = e_ResolutionMIP( nHeight, nMipDown );
		if ((w < nMinDimension) || (h < nMinDimension)) {
			--nMipDown;
		}
		else {
			break;
		}
	}


	// Don't mip beyond 1 level any texture which started at or below this resolution.
	int nMinMipDownDimension = 512;


	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( ! pOverrides || 0 == ( pOverrides->dwFlags & GFX_FLAG_ENABLE_MIP_LOD_OVERRIDES ) )
	{
		if ( eGroup == TEXTURE_GROUP_UI )
			nMipDown = MIN( nMipDown, UI_TEXTURE_MAX_MIP_DOWN );
		else
			nMipDown = MIN( nMipDown, TEXTURE_MAX_MIP_DOWN );

		if ( MIN( nWidth, nHeight ) <= nMinMipDownDimension )
			nMipDown = MIN( nMipDown, 1 );
	}

	nMipDown = MAX( nMipDown, 0 );

	nWidthInOut = e_ResolutionMIP( nWidth, nMipDown );
	nHeightInOut = e_ResolutionMIP( nHeight, nMipDown );

	return nMipDown;	// for the terminally curious
}


void e_TextureBudgetDebugOutputMIPLevels( BOOL bConsole, BOOL bTrace, BOOL bLog )
{
	// build output string
	const int cnOutLen = 256;
	char szHead[ cnOutLen ];
	char szInfo[ cnOutLen ];
	char szLines[ NUM_TEXTURE_GROUPS ][ cnOutLen ];
	int * pnMIPDown;

	PStrPrintf( szHead, cnOutLen, "MIP GROUP   DIFF  NORM  SELF  DIF2  SPEC  ENVM  LGHT" );

	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( pOverrides && pOverrides->dwFlags & GFX_FLAG_ENABLE_MIP_LOD_OVERRIDES )
	{
		PStrPrintf( szInfo, cnOutLen, "Texture MIP LOD overrides enabled!" );
		pnMIPDown = &pOverrides->pnTextureLODDown[0][0];
	}
	else
	{
		PStrPrintf( szInfo, cnOutLen, "Texture MIP levels (post-reaper):" );
		pnMIPDown = &gtTextureBudget.nMIPDown[0][0];
	}


	for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
	{
		PStrPrintf( szLines[ g ], cnOutLen, "%-12s%4d  %4d  %4d  %4d  %4d  %4d  %4d",
			e_GetTextureGroupName( (TEXTURE_GROUP)g ),
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_DIFFUSE ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_NORMAL ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_SELFILLUM ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_DIFFUSE2 ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_SPECULAR ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_ENVMAP ],
			pnMIPDown[ g * NUM_TEXTURE_TYPES + TEXTURE_LIGHTMAP ] );
	}

	if ( bConsole )
	{
		DebugString( OP_DEBUG, szInfo );
		DebugString( OP_DEBUG, szHead );
		for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
			DebugString( OP_DEBUG, szLines[ g ] );
	}

	if ( bTrace && ( ! bLog || ! LogGetTrace( LOG_FILE ) ) )
	{
		trace( "%s\n", szInfo );
		trace( "%s\n", szHead );
		for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
			trace( "%s\n", szLines[ g ] );
	}

	if ( bLog )
	{
		LogMessage( szInfo );
		LogMessage( szHead );
		for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
			LogMessage( szLines[ g ] );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_TextureBudgetInit()
{
	ZeroMemory( &gtTextureBudget, sizeof(TEXTURE_BUDGET) );
	gtTextureBudget.fQualityBias = 1.0f;
	return S_OK;
}

PRESULT e_TextureBudgetUpdate()
{
	// No texture budgeting in tool mode!
	if ( c_GetToolMode() )
		return S_FALSE;

	float fMinRate = 100.f;
	TEXTURE_BUDGET_MIP_GROUP * ppMIPRates[ NUM_TEXTURE_GROUPS ];
	for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
	{
		ppMIPRates[ g ] = (TEXTURE_BUDGET_MIP_GROUP *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_BUDGETS_TEXTURE_MIPS, g );
		ASSERTX_RETFAIL( ppMIPRates[ g ], e_GetTextureGroupName( (TEXTURE_GROUP)g ) );
		ASSERTX_RETFAIL( ppMIPRates[ g ]->eGroup == (TEXTURE_GROUP)g, "MIP fallback rate excel line doesn't match expected texture group!" );

		if ( g != TEXTURE_GROUP_UI )
		{
			// Find the minimum MIP rate to normalize the MIP budget factor.
			for ( int t = 0; t < NUM_TEXTURE_TYPES; t++ )
				if ( ppMIPRates[g]->fMIP[t] > 0.f )
					fMinRate = min( fMinRate, ppMIPRates[ g ]->fMIP[ t ] );
		}
	}

	gtTextureBudget.fMIPFactorNormalizer = TEXTURE_MAX_MIP_DOWN / fMinRate;


	float fTextureQuality = e_OptionState_GetActive()->get_fTextureQuality();
	float fTextureQualityUnreaped = fTextureQuality;
	float fMipFactorUnreaped = SATURATE( 1.f - fTextureQualityUnreaped );
	fTextureQuality *= gtTextureBudget.fQualityBias;
	// fMIPFactor is a 0..1 value.
	gtTextureBudget.fMIPFactor = SATURATE( 1.f - fTextureQuality );

	// store calculated rates
	for ( int g = 0; g < NUM_TEXTURE_GROUPS; g++ )
	{
		for ( int t = 0; t < NUM_TEXTURE_TYPES; t++ )
		{
			gtTextureBudget.nMIPDown[ g ][ t ] = sGetMIPFactor( ppMIPRates, 
				(TEXTURE_GROUP)g, (TEXTURE_TYPE)t, gtTextureBudget.fMIPFactor, 
				gtTextureBudget.fMIPFactorNormalizer );	

			gtTextureBudget.nMIPDownUnreaped[ g ][ t ] = sGetMIPFactor( ppMIPRates, 
				(TEXTURE_GROUP)g, (TEXTURE_TYPE)t, fMipFactorUnreaped, 
				gtTextureBudget.fMIPFactorNormalizer );	
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sGetMaxLOD( MODEL_BUDGET_GROUP** ppLODRates, MODEL_GROUP eGroup, float fQualityBias )
{
	return ROUND( MIN( 1.f, fQualityBias * ( 2 - ppLODRates[ eGroup ]->fLODRate ) ) * ( LOD_COUNT - 1 ) );
}

PRESULT e_ModelBudgetInit()
{
	ZeroMemory( &gtModelBudget, sizeof( MODEL_BUDGET ) );
	gtModelBudget.fQualityBias = 1.f;
	e_ModelBudgetUpdate();
	return S_OK;
}

PRESULT e_ModelBudgetUpdate()
{
	MODEL_BUDGET_GROUP* ppLODRates[ NUM_MODEL_GROUPS ];
	for ( int i = 0; i < NUM_MODEL_GROUPS; i++ )
	{
		if ( AppIsHellgate() )
		{
			int nForceLOD = LOD_NONE;
			if ( i == MODEL_GROUP_BACKGROUND )
				nForceLOD = e_OptionState_GetActive()->get_nBackgroundLoadForceLOD();
			else if ( i == MODEL_GROUP_UNITS || i == MODEL_GROUP_WARDROBE || i == MODEL_GROUP_PARTICLE || i == MODEL_GROUP_UI )
				nForceLOD = e_OptionState_GetActive()->get_nAnimatedLoadForceLOD();

			if ( nForceLOD == LOD_NONE )
			{
				ppLODRates[ i ] = (MODEL_BUDGET_GROUP*)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_BUDGETS_MODEL, i );
				ASSERT_RETFAIL( ppLODRates[ i ] );
				ASSERTX_RETFAIL( ppLODRates[ i ]->eGroup == (MODEL_GROUP)i, "LOD rate excel line doesn't match expected texture group!" );
				gtModelBudget.nMaxLOD[ i ] = sGetMaxLOD( ppLODRates, (MODEL_GROUP)i, gtModelBudget.fQualityBias );
			}
			else
			{
				gtModelBudget.nMaxLOD[ i ] = nForceLOD;
			}
		}
		else
			gtModelBudget.nMaxLOD[ i ] = ( LOD_COUNT - 1 );
	}

	return S_OK;
}

int e_ModelBudgetGetMaxLOD( MODEL_GROUP eGroup )
{
	return gtModelBudget.nMaxLOD[ eGroup ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sReaperResetModelDistances()
{
	ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
	ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	while ( pRes && pRes != pHeadTail )
	{
		pRes->fDistance = -1.f;
		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	return S_OK;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sReaperPruneModels( DWORD dwInactiveTime, float fPruneDistance = 50.f )
{
	DWORD dwTime = AppCommonGetRealTime();

	ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
	ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	BOOL bTexturesRemoved = FALSE;
	while ( pRes && pRes != pHeadTail )
	{
		DWORD dwResTime = pRes->GetTime();
		if ( dwTime >= dwResTime && ( dwTime - dwResTime >= dwInactiveTime ) )
		{
			if (   /*ABS( pRes->fQuality - gtModelBudget.fQualityBias ) > 0.01f
				&&*/ pRes->fDistance > fPruneDistance || pRes->fDistance < 0.f )
			{
				if (   e_ModelDefinitionExists( pRes->nId, LOD_LOW ) 
					&& e_ModelDefinitionExists( pRes->nId, LOD_HIGH ) )
				{
					e_ModelDefinitionDestroy( pRes->nId, LOD_HIGH, MODEL_DEFINITION_DESTROY_ALL, TRUE );
					pRes->fQuality = gtModelBudget.fQualityBias;
				}
				//else if ( !e_ModelDefinitionExists( pRes->nId, LOD_LOW ) )
				//{
				//	e_ModelDefinitionSetTextures( pRes->nId, LOD_HIGH, 
				//		TEXTURE_NORMAL, INVALID_ID );
				//	e_ModelDefinitionSetTextures( pRes->nId, LOD_HIGH, 
				//		TEXTURE_SPECULAR, INVALID_ID );
				//	bTexturesRemoved = TRUE;
				//}
			}
		}
		else
			break;

		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	if ( bTexturesRemoved )
		e_TexturesCleanup();

	return S_OK;
}

static PRESULT sReaperReloadModels()
{
	ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
	ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	while ( pRes && pRes != pHeadTail )
	{
		if (   /*ABS( pRes->fQuality - gtModelBudget.fQualityBias ) > 0.01f
			|| */pRes->fDistance <= 50.f && pRes->fDistance > 0.f )
		{
			if ( e_AnimatedModelDefinitionExists( pRes->nId, LOD_LOW ) 
			  && !e_AnimatedModelDefinitionExists( pRes->nId, LOD_HIGH ) )
			{
				V_DO_RESULT( e_AnimatedModelDefinitionReload( pRes->nId, LOD_HIGH ), S_OK )
				{
					pRes->fQuality = gtModelBudget.fQualityBias;
					break;
				}
			}
			else if ( e_ModelDefinitionExists( pRes->nId, LOD_LOW )
				   && !e_ModelDefinitionExists( pRes->nId, LOD_HIGH ) )
			{
				V_DO_RESULT( e_ModelDefinitionReload( pRes->nId, LOD_HIGH ), S_OK )
				{
					pRes->fQuality = gtModelBudget.fQualityBias;
					break;
				}
			}
		}

		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	return S_OK;
}

PRESULT e_MarkEngineResourcesUnloaded()
{
	ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tTextures.GetHeadTail();
	ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	while ( pRes && pRes != pHeadTail )
	{
		CLEARBIT( pRes->dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED );
		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
	pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	while ( pRes && pRes != pHeadTail )
	{
		CLEARBIT( pRes->dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED );
		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	return S_OK;
}


static PRESULT sReaperReloadTextures( DWORD dwInactiveTime, int nMaxReloads = 5 )
{
	DWORD dwTime = AppCommonGetRealTime();
	int nTexturesReloaded = 0;

	ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tTextures.GetHeadTail();
	ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetPrev();
	while ( pRes && pRes != pHeadTail )
	{
		DWORD dwResTime = pRes->GetTime();
		if ( dwTime > dwResTime && ( dwTime - dwResTime > dwInactiveTime ) )
		{			
			// Only update if it is a significant change.
			if ( ABS( pRes->fQuality - gtTextureBudget.fQualityBias ) > 0.1f )
			{
				V( e_TextureReload( pRes->nId ) );
				pRes->fQuality = gtTextureBudget.fQualityBias;
				nTexturesReloaded++;
			}
		}
		else
			break;

		if ( nMaxReloads > 0 && nTexturesReloaded >= nMaxReloads )
			break;

		pRes = (ENGINE_RESOURCE*)pRes->GetPrev();
	}

	return S_OK;
}

static DWORD sReaperGetTrackedResourcesSize( DWORD dwMaxElapsedTime )
{
		DWORD dwTime = AppCommonGetRealTime();
		DWORD dwBytesTotal = 0;

		{
			ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tTextures.GetHeadTail();
			ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetNext();
			while ( pRes && pRes != pHeadTail )
			{
				DWORD dwResTime = pRes->GetTime();
				if ( dwTime > dwResTime && ( dwTime - dwResTime <= dwMaxElapsedTime ) )
				{
					DWORD dwBytes = 0;
					V_GOTO( _next_texture_resource, e_TextureGetSizeInMemory( pRes->nId, &dwBytes ) );
					dwBytesTotal += dwBytes;
				}
				else
					break;

_next_texture_resource:
				pRes = (ENGINE_RESOURCE*)pRes->GetNext();
			}
		}

		{
			ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
			ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)pHeadTail->GetNext();
			while ( pRes && pRes != pHeadTail )
			{
				DWORD dwResTime = pRes->GetTime();
				if ( dwTime > dwResTime && ( dwTime - dwResTime <= dwMaxElapsedTime ) )
				{
					ENGINE_MEMORY tEngineMem;
					tEngineMem.Zero();

					int nLOD = LOD_NONE;
					if ( e_ModelDefinitionExists( pRes->nId, LOD_HIGH )	)
						nLOD = LOD_HIGH;
					else if ( e_ModelDefinitionExists( pRes->nId, LOD_LOW ) )
						nLOD = LOD_LOW;

					if ( nLOD != LOD_NONE )
					{
						V_GOTO( _next_modeldef_resource, 
							e_ModelDefinitionGetSizeInMemory( pRes->nId, &tEngineMem, 
							nLOD, TRUE ) );
					}

					dwBytesTotal += tEngineMem.dwVBufferBytes;
					dwBytesTotal += tEngineMem.dwIBufferBytes;
				}
				else
					break;

_next_modeldef_resource:
				pRes = (ENGINE_RESOURCE*)pRes->GetNext();
			}
		}
		
	return dwBytesTotal;
}

PRESULT e_ReaperInit()
{
	structclear( gtReaper );
	e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL_NONE );

	DWORD dwTime = AppCommonGetRealTime();
	gtReaper.tModels.Init();
	gtReaper.tModels.Update( dwTime );
	gtReaper.tTextures.Init();
	gtReaper.tTextures.Update( dwTime );

	return S_OK;
}

PRESULT e_ReaperReset()
{
	gtModelBudget.fQualityBias = 1.0f;
	V( e_ModelBudgetUpdate() );

	gtTextureBudget.fQualityBias = 1.0f;
	V( e_TextureBudgetUpdate() );

	return S_OK;
}

PRESULT e_ReaperUpdate()
{
	if ( !e_GetRenderFlag( RENDER_FLAG_REAPER_ENABLED ) || c_GetToolMode() || e_IsNoRender() )
		return S_OK;

	DWORD dwTime = AppCommonGetRealTime();
	gtReaper.tModels.Update( dwTime );
	gtReaper.tTextures.Update( dwTime );

	static const DWORD scdwAlertUpdateInterval = 1000;
	BOOL bUpdateAlertLevel = FALSE;

	APP_STATE eAppState = AppGetState();
	GAME* pGame = AppGetCltGame();
	GAME_STATE eGameState = pGame ? GameGetState( pGame ) : GAMESTATE_LOADING;
	BOOL bAppIsLoading = eAppState != APP_STATE_IN_GAME || eGameState != GAMESTATE_RUNNING;
	if ( bAppIsLoading )
		bUpdateAlertLevel = TRUE;

	if ( dwTime >= gtReaper.dwAlertLevelSetTime
	  && dwTime - gtReaper.dwAlertLevelSetTime > scdwAlertUpdateInterval )
	  bUpdateAlertLevel = TRUE;

	static float fVideoMemMultiplier = 1.0f; // 2.0f - default

	DWORD dwEngineTotalBytes = 0;
	DWORD dwEngineTotalMB = 0;
	DWORD dwMaxVideoMemory = 0;
	DWORD dwMaxVideoMemoryAdjusted = 0;
	float fPhysMemUsedPercentage = 0.f;
	float fVidMemUsedPercentage = 0.f;
	float fVAUsedPercentage = 0.f;
	MEMORYSTATUSEX memstat;
	MEM_COUNTER nLargestFreeBlock = 0;

	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) || bUpdateAlertLevel )
	{
		//e_EngineMemoryDump( FALSE, &dwEngineTotalBytes );
		if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) )
		{
			dwEngineTotalBytes = sReaperGetTrackedResourcesSize( 100 );
			dwEngineTotalMB = dwEngineTotalBytes / BYTES_PER_MB;
			dwMaxVideoMemory = e_CapGetValue( CAP_PHYSICAL_VIDEO_MEMORY_MB );
			dwMaxVideoMemoryAdjusted = DWORD( dwMaxVideoMemory * fVideoMemMultiplier );
			fVidMemUsedPercentage = float( dwEngineTotalMB ) / float( dwMaxVideoMemoryAdjusted );
		}

#if USE_MEMORY_MANAGER
		if ( false )
		{
			using namespace FSCommon;
			// CRT allocator is always 0.
			IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator( 0 );
			if( allocator )
			{
				MEMORY_ALLOCATOR_METRICS metrics;
				allocator->GetInternals()->GetMetrics(metrics);
				nLargestFreeBlock = metrics.LargestFreeBlock;
			}
		}
#endif

		memstat.dwLength = sizeof( MEMORYSTATUSEX );
		GlobalMemoryStatusEx(&memstat);
		fVAUsedPercentage = float( memstat.ullTotalVirtual - memstat.ullAvailVirtual ) / float( memstat.ullTotalVirtual );

		if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) )
			fPhysMemUsedPercentage = float( memstat.ullTotalPhys - memstat.ullAvailPhys ) / float( memstat.ullTotalPhys );
	}

	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) )
	{ 
		const int cnSize = 1024;
		WCHAR szInfo[ cnSize ] = L"";

		if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) == 1 )
		{
			e_DebugSetBar( 7, "VB + IB + Tex (MB)", dwEngineTotalMB, 
				dwMaxVideoMemoryAdjusted, int((float)dwMaxVideoMemoryAdjusted * 0.3f), 
				int((float)dwMaxVideoMemoryAdjusted * 0.75f) );
			e_DebugSetBar( 8, "PhysMem (MB)", ( memstat.ullTotalPhys - memstat.ullAvailPhys ) / BYTES_PER_MB,
				memstat.ullTotalPhys / BYTES_PER_MB, int((float)memstat.ullTotalPhys * 0.3f) / BYTES_PER_MB,
				int((float)memstat.ullTotalPhys * 0.90f) / BYTES_PER_MB );
			e_DebugSetBar( 9, "VA (MB)", ( memstat.ullTotalVirtual - memstat.ullAvailVirtual ) / BYTES_PER_MB,
				memstat.ullTotalVirtual / BYTES_PER_MB, int((float)memstat.ullTotalVirtual * 0.3f) / BYTES_PER_MB,
				int((float)memstat.ullTotalVirtual * 0.75f) / BYTES_PER_MB );

			WCHAR szAlertLevels[ NUM_REAPER_ALERT_LEVELS ][ 64 ] =
			{
				L"None",
				L"Low",
				L"Medium",
				L"High",
			};

			PStrPrintf( szInfo, cnSize, 
				L"RESOURCE REAPER\n"
				L"---------------\n"
				L"Alert level: %s\n"
				L"Alert updates @ current level: %d\n"
				L"Alert set time: %u\n"
				L"Current time: %u\n"
				L"Texture Bias: %1.1f\n"
				L"Model Bias: %1.1f\n" 
				L"VidMem used: %3.1f%%\n"
				L"PhysMem used: %3.1f%%\n"
				L"VA used: %3.1f%%\n"
				L"LFB (MB): %4.1f\n",
				szAlertLevels[ gtReaper.eAlertLevel ], 
				gtReaper.nAlertLevelUpdates,
				gtReaper.dwAlertLevelSetTime,
				dwTime,
				gtTextureBudget.fQualityBias,
				gtModelBudget.fQualityBias,
				fVidMemUsedPercentage * 100.f,
				fPhysMemUsedPercentage * 100.f,
				fVAUsedPercentage * 100.f,
				nLargestFreeBlock / (float)MEGA );
			VECTOR vPos = VECTOR( 0.7f, 1.f, 0.f );
			V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_RIGHT, TRUE ) );
		}
		else if ( e_GetRenderFlag( RENDER_FLAG_SHOW_REAPER_INFO ) > 1 )
		{
			ENGINE_RESOURCE* pHeadTail = (ENGINE_RESOURCE*)gtReaper.tModels.GetHeadTail();
			ENGINE_RESOURCE* pRes = (ENGINE_RESOURCE*)gtReaper.tModels.GetFirstNode();
			szInfo[0] = '\0';
			while ( pRes && pRes != pHeadTail )
			{
				const int cnLineSize = 256;
				WCHAR szLRUInfo[cnLineSize] = L"";
				PStrPrintf( szLRUInfo, cnLineSize, L"%d: %u (dist %f)\n", pRes->nId, 
					dwTime - pRes->GetTime(), pRes->fDistance );
				PStrCat( szInfo, szLRUInfo, cnSize );

				pRes = (ENGINE_RESOURCE*)pRes->GetNext();
			}
			VECTOR vPos = VECTOR( 1.f, -1.f, 0.f );
			V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOMRIGHT, UIALIGN_RIGHT, TRUE ) );

			pHeadTail = (ENGINE_RESOURCE*)gtReaper.tTextures.GetHeadTail();
			pRes = (ENGINE_RESOURCE*)gtReaper.tTextures.GetFirstNode();
			szInfo[0] = '\0';
			while ( pRes && pRes != pHeadTail )
			{
				const int cnLineSize = 256;
				WCHAR szLRUInfo[cnLineSize] = L"";
				DWORD dwTimeDiff = dwTime - pRes->GetTime();
				if ( dwTimeDiff < 1000 )
				{
					PStrPrintf( szLRUInfo, cnLineSize, L"%d: %u\n", pRes->nId, dwTimeDiff );
					PStrCat( szInfo, szLRUInfo, cnSize );
				}

				pRes = (ENGINE_RESOURCE*)pRes->GetNext();
			}
			vPos = VECTOR( -1.f, -1.f, 0.f );
			V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOMLEFT, UIALIGN_LEFT, TRUE ) );
		}
	}

	if ( bUpdateAlertLevel )
	{
		float fResourcesUsedPercentage = fVAUsedPercentage; //MAX( fVidMemUsedPercentage, fVAUsedPercentage );
		static int snHighPhysMemAlertCount = 0;
		static const float scfHighPhysMemUsedPercentageDefault = 0.95f;
		static float sfLastHighPhysMemUsedPercentage = scfHighPhysMemUsedPercentageDefault;

#if 0 // AE 2007.05.09 - Disabling physical memory metric; I'm not convinced that it is a good metric.
		if ( fPhysMemUsedPercentage > sfLastHighPhysMemUsedPercentage )
		{
			sfLastHighPhysMemUsedPercentage = fPhysMemUsedPercentage;
			snHighPhysMemAlertCount++;
		}
		else if ( fPhysMemUsedPercentage < scfHighPhysMemUsedPercentageDefault )
		{
			snHighPhysMemAlertCount = 0;
			sfLastHighPhysMemUsedPercentage = scfHighPhysMemUsedPercentageDefault;
		}
#endif

#if 0
		static BOOL sbDumpMemoryOutput = TRUE;
		if ( fResourcesUsedPercentage > 0.85f && sbDumpMemoryOutput	)
		{				
			MemoryTraceAllFLIDX();
			MemoryDumpCRTStats();
			e_EngineMemoryDump( TRUE, NULL );
			e_ModelDumpList();
			e_TextureDumpList();
			e_DumpTextureReport();
		}
#endif

		if ( !gtReaper.bAlertLevelOverride )
		{
			if ( fResourcesUsedPercentage > 0.95f || snHighPhysMemAlertCount > 5 )
				e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL_HIGH );
			//else if ( fResourcesUsedPercentage > 0.85f )
			//	e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL_MEDIUM );
			else if ( fResourcesUsedPercentage > 0.85f )
				e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL_LOW );
			else
				e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL_NONE );
		}

		switch ( gtReaper.eAlertLevel )
		{
		case REAPER_ALERT_LEVEL_NONE:
			{
				if ( gtReaper.nAlertLevelUpdates == 0 )
				{
					gtModelBudget.fQualityBias = 1.0f;
					V( e_ModelBudgetUpdate() );

					gtTextureBudget.fQualityBias = 1.0f;
					V( e_TextureBudgetUpdate() );
				}
				
				if ( ! bAppIsLoading )
				{
					V( sReaperReloadModels() );
					//V( sReaperReloadTextures( 10000 ) );
				}
			}
			break;

		case REAPER_ALERT_LEVEL_LOW:
			{
				if ( ( gtReaper.nAlertLevelUpdates % 5 ) == 0 )
				{
					e_Cleanup();
				}
				//else
				//{
				//	gtTextureBudget.fQualityBias *= 0.9f;
				//	gtTextureBudget.fQualityBias = MAX( gtTextureBudget.fQualityBias, 0.7f );
				//	V( e_TextureBudgetUpdate( FALSE ) );
				//}
			}
			break;

		case REAPER_ALERT_LEVEL_MEDIUM:
			{
				//if ( gtReaper.nAlertLevelUpdates == 0 )
				//{
				//	gtModelBudget.fQualityBias = 0.6f;
				//	V( e_ModelBudgetUpdate() );

				//	gtTextureBudget.fQualityBias = 0.6f;
				//	V( e_TextureBudgetUpdate( FALSE ) );

				//	e_Cleanup();
				//}
				//else
				//{
				//	gtTextureBudget.fQualityBias *= 0.9f;
				//	gtTextureBudget.fQualityBias = MAX( gtTextureBudget.fQualityBias, 0.3f );
				//	V( e_TextureBudgetUpdate( FALSE ) );
				//}

				//V( sReaperPruneModels( 30000 ) );
				//V( sReaperReloadTextures( 30000, -1 ) );
			}
			break;

		case REAPER_ALERT_LEVEL_HIGH:
			{
				if ( gtReaper.nAlertLevelUpdates == 0 )
				{
					gtModelBudget.fQualityBias = 0.4f;
					V( e_ModelBudgetUpdate() );
				
					// Force MIP on textures!
					gtTextureBudget.fQualityBias = 0.5f;
					V( e_TextureBudgetUpdate() );

					e_Cleanup();
				}
				else
				{
					//gtTextureBudget.fQualityBias *= 0.9f;
					//gtTextureBudget.fQualityBias = MAX( gtTextureBudget.fQualityBias, 0.1f );
					//V( e_TextureBudgetUpdate( FALSE ) );
				}

				DWORD dwInactiveTime = 0;
				float fPruneDistance = 0.f;
				if ( !bAppIsLoading )
				{
					fPruneDistance = 50.f;
					//dwInactiveTime = 10000;
				}

				V( sReaperPruneModels( dwInactiveTime, fPruneDistance ) );
				//V( sReaperReloadTextures( dwInactiveTime, -1 ) );

#if ISVERSION( DEVELOPMENT ) && 0
				ASSERTVONCE_DO( fResourcesUsedPercentage <= 0.95f,
					"LOW MEMORY! If you are a tester and received this message, "
					"then come get Amir, so he can debug this on your machine." )
				{				
					MemoryTraceAllFLIDX();
					MemoryDumpCRTStats();
					e_EngineMemoryDump( TRUE, NULL );
					e_ModelDumpList();
					e_TextureDumpList();
					e_DumpTextureReport();
				}
#endif

				if (   !bAppIsLoading 
					&& fResourcesUsedPercentage <= 0.9f
					/*&& gtReaper.nAlertLevelUpdates % 5 == 0 */)
				{
					V( sReaperReloadModels() );
				}
			}
			break;
		}

		if ( gtReaper.bAlertLevelOverride )
			e_ReaperSetAlertLevel( gtReaper.eAlertLevel, TRUE );
	}

	sReaperResetModelDistances();

	return S_OK;
}

void e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL eAlertLevel, BOOL bOverride /*= FALSE*/ )
{
	if ( gtReaper.eAlertLevel == eAlertLevel )
		gtReaper.nAlertLevelUpdates++;
	else
	{
		gtReaper.eAlertLevel = eAlertLevel;
		gtReaper.nAlertLevelUpdates = 0;
	}

	gtReaper.bAlertLevelOverride = bOverride;
	gtReaper.dwAlertLevelSetTime = AppCommonGetRealTime();
}

REAPER_ALERT_LEVEL e_ReaperGetAlertLevel()
{
	return gtReaper.eAlertLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void RESOURCE_UPLOAD_MANAGER::SetLimit( DWORD dwBytes )
{
	dwUploadFrameMaxBytes = dwBytes;
}

BOOL RESOURCE_UPLOAD_MANAGER::CanUpload()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_LIMIT_UPLOADS ) )
		return TRUE;
	if ( ! e_PresentEnabled() )
		return TRUE;
	if ( dwCurFrameBytesTotal > dwUploadFrameMaxBytes )
		return FALSE;
	return TRUE;
}

void RESOURCE_UPLOAD_MANAGER::MarkUpload( DWORD dwBytes, UPLOAD_TYPE eType )
{
	ASSERT_RETURN( IS_VALID_INDEX( eType, NUM_TYPES ) );
	dwCurFrameBytesPerType[eType] += dwBytes;
	dwCurFrameBytesTotal += dwBytes;
}

void RESOURCE_UPLOAD_MANAGER::ZeroCur()
{
	for ( int i = 0; i < NUM_TYPES; i++ )
	{
		dwCurFrameBytesPerType[i] = 0;
	}
	dwCurFrameBytesTotal = 0;
}

void RESOURCE_UPLOAD_MANAGER::NewFrame()
{
	// save the old frame's metrics
	for ( int i = 0; i < NUM_TYPES; i++ )
	{
		tMetrics[i].dwFrameBytes.Push( dwCurFrameBytesPerType[i] );
		tMetrics[i].dwTotalBytes += dwCurFrameBytesPerType[i];
	}	
	ZeroCur();
}

void RESOURCE_UPLOAD_MANAGER::Init()
{
	for ( int i = 0; i < NUM_TYPES; i++ )
	{
		tMetrics[i].dwFrameBytes.Zero();
		tMetrics[i].dwTotalBytes = 0;
	}
	ZeroCur();
}

DWORD RESOURCE_UPLOAD_MANAGER::GetTotal()
{
	DWORD dwTotal = 0;
	for ( int i = 0; i < NUM_TYPES; i++ )
		dwTotal += tMetrics[i].dwTotalBytes;
	return dwTotal;
}

DWORD RESOURCE_UPLOAD_MANAGER::GetAvgRecentForType( UPLOAD_TYPE eType )
{
	ASSERT_RETZERO( IS_VALID_INDEX( eType, NUM_TYPES ) );
	return tMetrics[eType].dwFrameBytes.Avg();
}

DWORD RESOURCE_UPLOAD_MANAGER::GetAvgRecentAll()
{
	DWORD dwAll = 0;
	for ( int i = 0; i < NUM_TYPES; ++i )
		dwAll += GetAvgRecentForType((UPLOAD_TYPE)i);
	return dwAll;
}

DWORD RESOURCE_UPLOAD_MANAGER::GetMaxRecentForType( UPLOAD_TYPE eType )
{
	ASSERT_RETZERO( IS_VALID_INDEX( eType, NUM_TYPES ) );
	return tMetrics[eType].dwFrameBytes.Max();
}

DWORD RESOURCE_UPLOAD_MANAGER::GetMaxRecentAll()
{
	DWORD dwAll = 0;
	for ( int i = 0; i < NUM_TYPES; ++i )
		dwAll += GetMaxRecentForType((UPLOAD_TYPE)i);
	return dwAll;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#undef ROUND
