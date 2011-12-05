//----------------------------------------------------------------------------
// dx9_query.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_texture.h"
#include "e_settings.h"
#include "dxC_profile.h"
#include "safe_vector.h"
#include "chb_timing.h"
#include "e_demolevel.h"
#include "dxC_pixo.h"

#include "dxC_query.h"


using namespace FSSE;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

void *	gpTraceQuery = NULL;
int		gnTraceModel = INVALID_ID;
D3D_STAT_QUERIES gtStatQueries;
int gpnStatsToDisplay[] = 
{
	D3DRTYPE_TEXTURE,
	D3DRTYPE_CUBETEXTURE,
	D3DRTYPE_VERTEXBUFFER,
	D3DRTYPE_INDEXBUFFER,
};
const int gnStatsToDisplayCount = arrsize( gpnStatsToDisplay );
char gszStatsToDisplay[][ D3D_QUERY_PROFILE_STR_LEN ] =
{
	"Texture",
	"Cube Texture",
	"Vertex Buffer",
	"Index Buffer"
};
#ifdef ENGINE_TARGET_DX9
D3DDEVINFO_RESOURCEMANAGER		gtD3DStatResManagerDiff;
D3DDEVINFO_RESOURCEMANAGER		gtD3DStatResManagerRaw;
D3DDEVINFO_D3DVERTEXSTATS		gtD3DStatVertexStatsDiff;
D3DDEVINFO_D3DVERTEXSTATS		gtD3DStatVertexStatsRaw;
#endif
safe_vector<D3D_GPU_TIMER> *	gpGPUTimers = NULL;
RunningAverage<DWORD,60>		gtGPUTimerAvgMS;
DWORD							gdwGPUTimerStaleFrames = 0;
RunningAverage<DWORD,60>		gtCPUTimerAvgMS;

DWORD							gdwGPUTimerLastMS = 0;



//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetTraceQuery( unsigned int nAddress )
{
	gpTraceQuery = *(void **)&nAddress;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetTraceModel( int nModelID )
{
	gnTraceModel = nModelID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_GetQueryData( LPD3DCQUERY pQuery, void * pData, int nDataSize )
{
	ASSERT( pQuery );
	int nActualDataSize = pQuery->GetDataSize();
	ASSERT( nDataSize == nActualDataSize );
	PRESULT hrTest = pQuery->GetData( pData, nDataSize, 0 );
	return hrTest;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_RestoreStatQueries()
{
#ifdef ENGINE_TARGET_DX9
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	PRESULT pr;
	// these will return NOT_AVAILABLE in the Release D3D runtime
	V_SAVE( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_VERTEXSTATS,		&gtStatQueries.pVertexStats	   ) );
	V_SAVE( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_RESOURCEMANAGER,	&gtStatQueries.pResourceManager  ) );

	// these require an instrumented driver
	// ... will return NOT_AVAILABLE in non-instrumented drivers
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_BANDWIDTHTIMINGS,	&gtStatQueries.pBandwidthTimings );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_CACHEUTILIZATION,	&gtStatQueries.pCacheUtilization );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_INTERFACETIMINGS,	&gtStatQueries.pInterfaceTimings );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_PIPELINETIMINGS,		&gtStatQueries.pPipelineTimings  );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_VCACHE,				&gtStatQueries.pVCache			  );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_PIXELTIMINGS,		&gtStatQueries.pPixelTimings	  );
	//hResult = dx9_GetDevice()->CreateQuery( D3DQUERYTYPE_VERTEXTIMINGS,		&gtStatQueries.pVertexTimings	  );
#elif defined(ENGINE_TARGET_DX10)
	D3D10_QUERY_DESC Desc;
	Desc.Query = D3D10_QUERY_PIPELINE_STATISTICS;
	Desc.MiscFlags = 0;
	V_RETURN( dxC_GetDevice()->CreateQuery( &Desc, &gtStatQueries.pD3D10PipeLineStatistics ) );
	gtStatQueries.pD3D10PipeLineStatistics->Begin();
	gtStatQueries.bQueryIssued = TRUE;
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_ReleaseStatQueries()
{
#ifdef ENGINE_TARGET_DX9
	gtStatQueries.pBandwidthTimings = NULL;
	gtStatQueries.pCacheUtilization = NULL;
	gtStatQueries.pInterfaceTimings = NULL;
	gtStatQueries.pPipelineTimings  = NULL;
	gtStatQueries.pResourceManager  = NULL;
	gtStatQueries.pVCache			= NULL;
	gtStatQueries.pVertexStats		= NULL;
	gtStatQueries.pVertexTimings	= NULL;
	gtStatQueries.pPixelTimings		= NULL;
#elif defined(ENGINE_TARGET_DX10)
	gtStatQueries.pD3D10PipeLineStatistics = NULL;
#endif

	// also clear any GPU timer queries
	dxC_GPUTimersReleaseAll();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_StatQueriesBeginFrame()
{
#ifdef ENGINE_TARGET_DX9
	// only these stat queries require a beginFrame()
	//if ( sgtStatQueries.pBandwidthTimings )
	//	sgtStatQueries.pBandwidthTimings->Issue( D3DISSUE_BEGIN );
	//if ( sgtStatQueries.pInterfaceTimings )
	//	sgtStatQueries.pInterfaceTimings->Issue( D3DISSUE_BEGIN );
	//if ( sgtStatQueries.pPipelineTimings )
	//	sgtStatQueries.pPipelineTimings->Issue( D3DISSUE_BEGIN );
	//if ( sgtStatQueries.pVertexTimings )
	//	sgtStatQueries.pVertexTimings->Issue( D3DISSUE_BEGIN );
	//if ( sgtStatQueries.pPixelTimings )
	//	sgtStatQueries.pPixelTimings->Issue( D3DISSUE_BEGIN );
	//if ( sgtStatQueries.pCacheUtilization )
	//	sgtStatQueries.pCacheUtilization->Issue( D3DISSUE_BEGIN );

	// gather stats at the beginning of the frame, if active
	if ( gtStatQueries.pResourceManager )
	{
		// double-buffer to compute diffs
		D3DDEVINFO_RESOURCEMANAGER tManager;
		ZeroMemory( &tManager, sizeof(D3DDEVINFO_RESOURCEMANAGER) );
		HRESULT hResManagerResult		= dx9_GetQueryData( gtStatQueries.pResourceManager,	&tManager,	sizeof(tManager) );
		// calc diffs
		for ( int i = 0; i < D3DRTYPECOUNT; i++ )
		{
			D3DRESOURCESTATS & tDiff = gtD3DStatResManagerDiff.stats[ i ];
			D3DRESOURCESTATS & tRaw  = gtD3DStatResManagerRaw.stats[ i ];

#define DIFF_STAT(staat)		tDiff.staat = ( tManager.stats[ i ].staat > tRaw.staat ) ? ( tManager.stats[ i ].staat - tRaw.staat ) : 0

			DIFF_STAT(ApproxBytesDownloaded);
			DIFF_STAT(NumEvicts);
			DIFF_STAT(NumUsed);
			DIFF_STAT(NumUsedInVidMem);
			DIFF_STAT(NumVidCreates);
			DIFF_STAT(TotalBytes);
			DIFF_STAT(TotalManaged);
			DIFF_STAT(WorkingSet);
			DIFF_STAT(WorkingSetBytes);

#undef DIFF_STAT
		}

		dx9_TrackD3DResourceManagerStats();

		gtD3DStatResManagerRaw = tManager;
	}
	if ( gtStatQueries.pVertexStats )
	{
		D3DDEVINFO_D3DVERTEXSTATS tStats;
		ZeroMemory( &tStats, sizeof(D3DDEVINFO_D3DVERTEXSTATS) );
		HRESULT hVertexStatsResult		= dx9_GetQueryData( gtStatQueries.pVertexStats,		&tStats,	sizeof(tStats) );

		D3DDEVINFO_D3DVERTEXSTATS & tDiff = gtD3DStatVertexStatsDiff;
		D3DDEVINFO_D3DVERTEXSTATS & tRaw  = gtD3DStatVertexStatsRaw;

		tDiff = tStats;

#define DIFF_STAT(staat)		tDiff.staat = ( tStats.staat > tRaw.staat ) ? ( tStats.staat - tRaw.staat ) : 0
		DIFF_STAT(NumRenderedTriangles);
#undef DIFF_STAT

		gtD3DStatVertexStatsRaw = tStats;
	}
#elif defined(ENGINE_TARGET_DX10)
	D3D10_QUERY_DATA_PIPELINE_STATISTICS stats;
	if( dx9_GetQueryData( gtStatQueries.pD3D10PipeLineStatistics, (void*)&stats, sizeof( D3D10_QUERY_DATA_PIPELINE_STATISTICS ) ) == S_OK )
	{
		gtStatQueries.pD3D10PipeLineStatistics->Begin();
		gtStatQueries.bQueryIssued = TRUE;
		//Chris: Where do we want these stats?
	}
	
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_StatQueriesEndFrame()
{
#ifdef ENGINE_TARGET_DX9
	// normal driver, debug runtime
	if ( gtStatQueries.pResourceManager )
	{
		V( gtStatQueries.pResourceManager	->Issue( D3DISSUE_END ) );
	}
	if ( gtStatQueries.pVertexStats )
	{
		V( gtStatQueries.pVertexStats		->Issue( D3DISSUE_END ) );
	}

	// instrumented driver
	//if ( gtStatQueries.pBandwidthTimings )
	//	gtStatQueries.pBandwidthTimings->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pInterfaceTimings )
	//	gtStatQueries.pInterfaceTimings->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pPipelineTimings )
	//	gtStatQueries.pPipelineTimings->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pVertexTimings )
	//	gtStatQueries.pVertexTimings->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pPixelTimings )
	//	gtStatQueries.pPixelTimings->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pCacheUtilization )
	//	gtStatQueries.pCacheUtilization->Issue( D3DISSUE_END );
	//if ( gtStatQueries.pVCache )
	//	gtStatQueries.pVCache->Issue( D3DISSUE_END );
#elif defined(ENGINE_TARGET_DX10)
	if( gtStatQueries.pD3D10PipeLineStatistics && gtStatQueries.bQueryIssued )
	{
		gtStatQueries.pD3D10PipeLineStatistics->End();
		gtStatQueries.bQueryIssued = FALSE;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRestoreGPUTimer( D3D_GPU_TIMER & tTimer )
{
	tTimer.bWaiting = FALSE;
	tTimer.bOpen = FALSE;

	tTimer.Release();

	PRESULT pr = S_OK;
#ifdef ENGINE_TARGET_DX9
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_TIMESTAMP,			&tTimer.pTimestampBegin		) );
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_TIMESTAMP,			&tTimer.pTimestampEnd		) );
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_TIMESTAMPDISJOINT,	&tTimer.pTimestampDisjoint	) );
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_TIMESTAMPFREQ,		&tTimer.pTimestampFreq		) );
#elif defined(ENGINE_TARGET_DX10)
	D3D10_QUERY_DESC Desc;
	Desc.MiscFlags = 0;
	Desc.Query = D3D10_QUERY_TIMESTAMP;
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( &Desc, &tTimer.pTimestampBegin ) );
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( &Desc, &tTimer.pTimestampEnd ) );
	Desc.Query = D3D10_QUERY_TIMESTAMP_DISJOINT;
	V_SAVE_ERROR( pr, dxC_GetDevice()->CreateQuery( &Desc, &tTimer.pTimestampDisjoint ) );
#endif
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GPUTimerBeginFrame()
{
	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;

	// lazy create the array if necessary
	if ( ! gpGPUTimers )
	{
		gpGPUTimers = (safe_vector<D3D_GPU_TIMER>*) MALLOC_NEW( NULL, safe_vector<D3D_GPU_TIMER> );
		ASSERT_RETVAL( gpGPUTimers, E_OUTOFMEMORY );
		gtGPUTimerAvgMS.Zero();
		gtCPUTimerAvgMS.Zero();
	}

	D3D_GPU_TIMER * pTimer = NULL;

	// lazy create a new timer if necessary
	for ( safe_vector<D3D_GPU_TIMER>::iterator i = gpGPUTimers->begin();
		i != gpGPUTimers->end();
		++i )
	{
		if ( ! i->bOpen && ! i->bWaiting )
		{
			pTimer = &(*i);
			break;
		}
	}
	if ( pTimer )
	{
		// grab this timer
		if ( ! pTimer->IsValid() )
		{
			V_RETURN( sRestoreGPUTimer( *pTimer ) );
		}
	}
	else
	{
		// create a new timer
		D3D_GPU_TIMER tTimer;
		// CHB 2007.05.28 - Fail silently for the poor folk without queries.
		if (FAILED( sRestoreGPUTimer( tTimer ) ))
		{
			return S_FALSE;
		}
		gpGPUTimers->push_back( tTimer );
		pTimer = &((*gpGPUTimers)[ gpGPUTimers->size() - 1 ] );
	}

	ASSERT_RETFAIL( pTimer && pTimer->IsValid() );

#ifdef ENGINE_TARGET_DX9
	V( pTimer->pTimestampDisjoint->Issue( D3DISSUE_BEGIN ) );
	V( pTimer->pTimestampBegin->Issue( D3DISSUE_END ) );
#else // DX10
	pTimer->pTimestampDisjoint->Begin();
	pTimer->pTimestampBegin->End();
#endif // DX10

	pTimer->bWaiting = FALSE;
	pTimer->bOpen = TRUE;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GPUTimerEndFrame()
{
	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;
	if ( ! gpGPUTimers )
		return S_FALSE;

	// Find the open timer
	D3D_GPU_TIMER * pTimer = NULL;

	for ( safe_vector<D3D_GPU_TIMER>::iterator i = gpGPUTimers->begin();
		i != gpGPUTimers->end();
		++i )
	{
		if ( i->bOpen )
		{
			pTimer = &(*i);
			break;
		}
	}
	if ( pTimer )
	{
		ASSERT( ! pTimer->bWaiting );
		ASSERT_RETFAIL( pTimer->IsValid() );

#ifdef ENGINE_TARGET_DX9
		V( pTimer->pTimestampEnd->Issue( D3DISSUE_END ) );
		V( pTimer->pTimestampFreq->Issue( D3DISSUE_END ) );
		V( pTimer->pTimestampDisjoint->Issue( D3DISSUE_END ) );
#else // DX10
		pTimer->pTimestampEnd->End();
		pTimer->pTimestampDisjoint->End();
#endif // DX10

		pTimer->bOpen = FALSE;
		pTimer->bWaiting = TRUE;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GPUTimerUpdate()
{
	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;
	if ( ! gpGPUTimers )
		return S_FALSE;

	gdwGPUTimerStaleFrames++;

	// Find the waiting timer(s)
	for ( safe_vector<D3D_GPU_TIMER>::iterator i = gpGPUTimers->begin();
		i != gpGPUTimers->end();
		++i )
	{
		if ( i->bOpen || ! i->bWaiting )
			continue;

		D3D_GPU_TIMER * pTimer = &(*i);

		// see if the timer is done yet
		UINT64 qwFreq, qwBegin, qwEnd;
		BOOL bDisjoint;
		PRESULT pr;
#ifdef ENGINE_TARGET_DX9
		pr = pTimer->pTimestampDisjoint->GetData(&bDisjoint, sizeof(BOOL), 0 );
		if ( S_OK != pr )	continue;
		pr = pTimer->pTimestampFreq->GetData(&qwFreq, sizeof(UINT64), 0);
		if ( S_OK != pr )	continue;
		pr = pTimer->pTimestampBegin->GetData(&qwBegin, sizeof(UINT64), 0 );
		if ( S_OK != pr )	continue;
		pr = pTimer->pTimestampEnd->GetData(&qwEnd, sizeof(UINT64), 0 );
		if ( S_OK != pr )	continue;
#else // DX10
		D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tDisjoint;
		pr = pTimer->pTimestampDisjoint->GetData( &tDisjoint, sizeof(D3D10_QUERY_DATA_TIMESTAMP_DISJOINT), 0 );
		if ( S_OK != pr )	continue;
		pr = pTimer->pTimestampBegin->GetData( &qwBegin, sizeof(UINT64), 0 );
		if ( S_OK != pr )	continue;
		pr = pTimer->pTimestampEnd->GetData( &qwEnd, sizeof(UINT64), 0 );
		if ( S_OK != pr )	continue;
		bDisjoint = tDisjoint.Disjoint;
		qwFreq = tDisjoint.Frequency;
#endif // DX10
		pTimer->bWaiting = FALSE;
		if ( bDisjoint )
			continue;
		DWORD dwLastMS = (DWORD)( ( qwEnd - qwBegin ) * 1000.0 / qwFreq );
		gtGPUTimerAvgMS.Push( dwLastMS );
		gdwGPUTimerLastMS = dwLastMS;
		gdwGPUTimerStaleFrames = 0;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GPUTimerGetCounts( DWORD * pdwAvg, DWORD * pdwMax, DWORD & dwStaleFrames )
{
	if ( pdwAvg )	*pdwAvg = 0;
	if ( pdwMax )	*pdwMax = 0;
	dwStaleFrames = 0;

	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;
	if ( ! gpGPUTimers )
		return S_FALSE;

	if ( pdwAvg )	*pdwAvg = gtGPUTimerAvgMS.Avg();
	if ( pdwMax )	*pdwMax = gtGPUTimerAvgMS.Max();
	dwStaleFrames = gdwGPUTimerStaleFrames;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD dxC_GPUTimerGetLast()
{
	return gdwGPUTimerLastMS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GPUTimersReleaseAll()
{
	if ( ! gpGPUTimers )
		return S_FALSE;

	for ( safe_vector<D3D_GPU_TIMER>::iterator i = gpGPUTimers->begin();
		i != gpGPUTimers->end();
		++i )
	{
		(*i).Release();
	}

	FREE_DELETE( NULL, gpGPUTimers, safe_vector<D3D_GPU_TIMER> );
	gpGPUTimers = NULL;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CPUTimerUpdate()
{
	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;

	DWORD dwFrameTimeMS = static_cast<DWORD>(CHB_TimingGetInstantaneous(CHB_TIMING_FRAME) * 1000 + 0.5f);
	gtCPUTimerAvgMS.Push( dwFrameTimeMS );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CPUTimerGetCounts( DWORD * pdwAvg, DWORD * pdwMax )
{
	if ( pdwAvg )	*pdwAvg = 0;
	if ( pdwMax )	*pdwMax = 0;

	if ( ! e_DemoLevelIsActive() && ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return S_FALSE;

	if ( pdwAvg )	*pdwAvg = gtCPUTimerAvgMS.Avg();
	if ( pdwMax )	*pdwMax = gtCPUTimerAvgMS.Max();
	return S_OK;
}
