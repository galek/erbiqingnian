//----------------------------------------------------------------------------
// dxC_query.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_QUERY__
#define __DXC_QUERY__

#include "e_settings.h"
//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define D3D_QUERY_PROFILE_STR_LEN	32

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#ifdef _DEBUG

#define TRACE_MODEL(nModelID, pszEvent)				dx9_TraceModel( nModelID, pszEvent, __LINE__ )
#define TRACE_MODEL_INT(nModelID, pszEvent, nInt)	dx9_TraceModelInt( nModelID, pszEvent, nInt, __LINE__ )
#define TRACE_QUERY(pQuery, pszEvent)				dx9_TraceQuery( (void*)(pQuery), pszEvent, __LINE__ )
#define TRACE_QUERY_INT(pQuery, pszEvent, nInt)		dx9_TraceQueryInt( (void*)(pQuery), pszEvent, nInt, __LINE__ )

#else

#define TRACE_MODEL(nModelID, pszEvent)				((void)0)
#define TRACE_MODEL_INT(nModelID, pszEvent, nInt)	((void)0)
#define TRACE_QUERY(pQuery, pszEvent)				((void)0)
#define TRACE_QUERY_INT(pQuery, pszEvent, nInt)		((void)0)

#endif

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct D3D_STAT_QUERIES {
#ifdef ENGINE_TARGET_DX9
	SPD3DCQUERY pBandwidthTimings;
	SPD3DCQUERY pCacheUtilization;
	SPD3DCQUERY pInterfaceTimings;
	SPD3DCQUERY pPipelineTimings;
	SPD3DCQUERY pResourceManager;
	SPD3DCQUERY pVCache;
	SPD3DCQUERY pVertexStats;
	SPD3DCQUERY pVertexTimings;
	SPD3DCQUERY pPixelTimings;
#elif defined(ENGINE_TARGET_DX10)
	SPD3DCQUERY pD3D10PipeLineStatistics;
	BOOL bQueryIssued;
#endif
};

struct D3D_GPU_TIMER
{
	BOOL bOpen;
	BOOL bWaiting;
	SPD3DCQUERY pTimestampBegin;
	SPD3DCQUERY pTimestampEnd;
	SPD3DCQUERY pTimestampDisjoint;
#ifdef ENGINE_TARGET_DX9
	SPD3DCQUERY pTimestampFreq;
#endif

	void Release()
	{
		pTimestampBegin = NULL;
		pTimestampEnd = NULL;
		pTimestampDisjoint = NULL;
		DX9_BLOCK( pTimestampFreq = NULL; )
	}

	BOOL IsValid()
	{
		return !!( pTimestampBegin && pTimestampEnd && pTimestampDisjoint	DX9_BLOCK( && pTimestampFreq ) );
	}
};


//struct D3D_GPU_SYNC
//{
//	static const int MAX_QUERIES = 4;
//	enum SYNC_STATE
//	{
//		INACTIVE = 0,
//		CPU_ISSUED,
//		GPU_COMPLETE,
//	};
//	SPD3DCQUERY pQueries[MAX_QUERIES];
//
//	PRESULT Issue();
//	PRESULT GetOutstanding( DWORD & dwOut );
//};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern D3D_STAT_QUERIES gtStatQueries;
extern int gpnStatsToDisplay[];
extern const int gnStatsToDisplayCount;
extern char gszStatsToDisplay[ ][ D3D_QUERY_PROFILE_STR_LEN ];
#if defined(ENGINE_TARGET_DX9)
	extern D3DDEVINFO_RESOURCEMANAGER		gtD3DStatResManagerDiff;
	extern D3DDEVINFO_RESOURCEMANAGER		gtD3DStatResManagerRaw;
	extern D3DDEVINFO_D3DVERTEXSTATS		gtD3DStatVertexStatsDiff;
	extern D3DDEVINFO_D3DVERTEXSTATS		gtD3DStatVertexStatsRaw;
#endif


//----------------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------------

HRESULT dx9_GetQueryData( LPD3DCQUERY pQuery, void * pData, int nDataSize );
void dx9_RestoreStatQueries();
void dx9_ReleaseStatQueries();
void dx9_StatQueriesBeginFrame();
void dx9_StatQueriesEndFrame();
PRESULT dxC_RestoreStatQueries();
PRESULT dxC_ReleaseStatQueries();
void dxC_StatQueriesBeginFrame();
PRESULT dxC_GPUTimerBeginFrame();
PRESULT dxC_GPUTimerEndFrame();
PRESULT dxC_GPUTimerUpdate();
PRESULT dxC_GPUTimerGetCounts( DWORD * pdwAvg, DWORD * pdwMax, DWORD & dwStaleFrames );
DWORD dxC_GPUTimerGetLast();
PRESULT dxC_GPUTimersReleaseAll();
PRESULT dxC_CPUTimerUpdate();
PRESULT dxC_CPUTimerGetCounts( DWORD * pdwAvg, DWORD * pdwMax );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline BOOL dx9_IsTraceQuery(
	void * pQuery )
{
#if ISVERSION(DEVELOPMENT)
	extern void * gpTraceQuery;
	//if ( ! e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
	//	return FALSE;
	if ( gpTraceQuery == NULL || gpTraceQuery != pQuery )
		return FALSE;
	return TRUE;
#else
	return FALSE;
#endif
}

inline void dx9_TraceQuery(
	void * pQuery,
	const char * pszEvent,
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( dx9_IsTraceQuery( pQuery ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE QUERY 0x%08p:   %s", nLine, pQuery, pszEvent );
#endif
}

inline void dx9_TraceQueryInt(
	void * pQuery,
	const char * pszEvent,
	const int nInt,
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( dx9_IsTraceQuery( pQuery ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE QUERY 0x%08p:   %s [ %d ]", nLine, pQuery, pszEvent, nInt );
#endif
}

inline BOOL dx9_IsTraceModel(
	int nModelID )
{
#if ISVERSION(DEVELOPMENT)
	extern int gnTraceModel;
	//if ( ! e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
	//	return FALSE;
	if ( gnTraceModel == INVALID_ID || gnTraceModel != nModelID )
		return FALSE;
	return TRUE;
#else
	return FALSE;
#endif
}

inline void dx9_TraceModel(
	int nModelID, 
	const char * pszEvent, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( dx9_IsTraceModel( nModelID ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE MODEL %4d:   %s", nLine, nModelID, pszEvent );
#endif
}

inline void dx9_TraceModelInt( 
	int nModelID, 
	const char * pszEvent, 
	const int nInt, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( dx9_IsTraceModel( nModelID ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE MODEL %4d:   %s [ %d ]", nLine, nModelID, pszEvent, nInt );
#endif
}


#endif //__DXC_QUERY__