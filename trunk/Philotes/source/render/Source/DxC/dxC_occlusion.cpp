//----------------------------------------------------------------------------
// dx9_occlusion.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_model.h"
#include "dxC_query.h"
#include "dxC_profile.h"
#include "dxC_occlusion.h"
#include "dxC_caps.h"
#include "dxC_obscurance.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

int gnDumpOcclusionQueries			= -1;

// debug
void * gpLastBeginOcclusionPtr		= NULL;
void * gpLastEndOcclusionPtr		= NULL;
BOOL gbOcclusionQueryActive			= FALSE;

int gnLastBeginOcclusionModelID		= -1;
int gnLastEndOcclusionModelID		= -1;

extern int gnOcclusionQueryObjectsCurrent	= 0;
extern int gnOcclusionQueryObjectsTotal		= 0;
extern int gnOcclusionQueriesRendering		= 0;
extern int gnOcclusionQueriesWaiting		= 0;

const char cgszOcclusionType[ NUM_OCCLUSION_TYPES ][ 16 ] =
{
	"Model",
	"BBox"
};

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

void dx9_OcclusionGetCounts( int & rnObjectsCurrent, int & rnObjectsTotal, int & rnRendering, int & rnWaiting )
{
	rnObjectsCurrent = gnOcclusionQueryObjectsCurrent;
	rnObjectsTotal	 = gnOcclusionQueryObjectsTotal;
	rnRendering		 = gnOcclusionQueriesRendering;
	rnWaiting		 = gnOcclusionQueriesWaiting;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_ReleaseAllOcclusionQueries()
{
	// release all occlusion queries in the game

	// models:
	for ( D3D_MODEL* pModel = dx9_ModelGetFirst(); pModel; pModel = dx9_ModelGetNext( pModel ) )
	{
		V( dx9_ReleaseOcclusionQuery( &pModel->tOcclusion, pModel->nId ) );
	}

#if ISVERSION(DEVELOPMENT)
	V( dxC_ObscuranceDeviceLost() );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RestoreAllOcclusionQueries()
{
	// release all occlusion queries in the game

	// models:
	//for ( D3D_MODEL* pModel = dx9_ModelGetFirst(); pModel; pModel = dx9_ModelGetNext( pModel ) )
	//{
	//	V( dx9_RestoreOcclusionQuery( &pModel->tOcclusion, pModel->nId ) );
	//}

#if ISVERSION(DEVELOPMENT)
	// in general, we don't actually want to restore obscurance queries -- it's only done during asset conversion
	//V( dxC_ObscuranceDeviceReset() );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_DebugDumpOcclusion()
{
	gnDumpOcclusionQueries = -2;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_SetOccluded( D3D_OCCLUSION * pOcclusion, BOOL bOccluded, int nDebugModelID )
{
	TRACE_QUERY_INT( pOcclusion, "Setting occlusion query", bOccluded );

	ASSERT_RETINVALIDARG( pOcclusion );
	if ( ! e_GetRenderFlag( RENDER_FLAG_OCCLUSION_FREEZE ) )
	{
		pOcclusion->dwPixelsDrawn = (DWORD)( ! bOccluded );

		if ( gnDumpOcclusionQueries >= 0 )
		{
			V( dx9_DumpQuery( "Set", nDebugModelID, bOccluded, NULL ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RestoreOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID )
{
	ASSERT_RETINVALIDARG( pOcclusion );

	TRACE_QUERY( pOcclusion, "Releasing occlusion query" );

	for ( int i = 0; i < OCCLUSION_BUFFERS; i++ )
		pOcclusion->pD3DQuery[ i ] = NULL;

	pOcclusion->dwPixelsDrawn = 0;
	pOcclusion->pnQueue[ 0 ] = INVALID_ID;

	for ( int i = 0; i < OCCLUSION_BUFFERS; i++ )
	{
		pOcclusion->pnStates[ i ] = OCCLUSION_STATE_INVALID;
	}

#if ! ISVERSION(DEVELOPMENT)
	if ( AppIsTugboat() )
		return S_FALSE;
#endif

	// -- CHB 2006.06.21 - Occlusion query not available on 1.1.
	// CML 2006.07.16 - Level-based render flags often aren't in the proper state when this function
	//                  is called.  Instead, should check E_CAPS directly.
	//if (!e_GetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED ))
	if ( ! dx9_CapGetValue( DX9CAP_OCCLUSION_QUERIES ) )
	{
		return S_FALSE;
	}

	for ( int i = 0; i < OCCLUSION_BUFFERS; i++ )
	{
		//DebugString( OP_DEBUG, "Restoring occlusion query, index: %d", i );
		V_RETURN( dxC_OcclusionQueryCreate( &pOcclusion->pD3DQuery[i] ) );

		gnOcclusionQueryObjectsCurrent++;
		gnOcclusionQueryObjectsTotal++;

		if ( gnDumpOcclusionQueries >= 0 )
		{
			V( dx9_DumpQuery( "Create", nDebugModelID, i, pOcclusion->pD3DQuery[ i ] ) );
		}

		pOcclusion->pnStates[ i ] = OCCLUSION_STATE_INIT;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ReleaseOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID )
{
	ASSERT_RETINVALIDARG( pOcclusion );
	TRACE_QUERY( pOcclusion, "Releasing occlusion query" );
	for ( int i = 0; i < OCCLUSION_BUFFERS; i++ )
	{
		//DebugString( OP_DEBUG, "Releasing occlusion query" );
		if ( pOcclusion->pD3DQuery[ i ] )
		{
			DX9_BLOCK( V( pOcclusion->pD3DQuery[ i ]->Issue( D3DISSUE_END ) ); )
			DX10_BLOCK( pOcclusion->pD3DQuery[ i ]->End(); )
			gnOcclusionQueryObjectsCurrent--;
			ASSERT( gnOcclusionQueryObjectsCurrent >= 0 );

			if ( gnDumpOcclusionQueries >= 0 )
			{
				V( dx9_DumpQuery( "Release", nDebugModelID, i, pOcclusion->pD3DQuery[ i ] ) );
			}
		}
		pOcclusion->pD3DQuery[ i ] = NULL;
	}

	pOcclusion->dwPixelsDrawn = 0;
	pOcclusion->pnQueue[ 0 ] = INVALID_ID;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_BeginOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugType, int nDebugModelID )
{
#if ! ISVERSION(DEVELOPMENT)
	if ( AppIsTugboat() )
		return S_FALSE;
#endif

	if (!e_GetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED )) {
		return S_FALSE;
	}
	ASSERT_RETINVALIDARG( pOcclusion );

	// how many slots are in use?
	int nSlots;
	for ( nSlots = 0; nSlots < OCCLUSION_BUFFERS; nSlots++ )
		if ( pOcclusion->pnQueue[ nSlots ] == INVALID_ID )
			break;
	if ( nSlots == OCCLUSION_BUFFERS )
	{
		// all available queries have been issued and are still outstanding;
		// don't issue a new one
		return S_FALSE;
	}

	// find which query buffer is unused
	int nQuery = INVALID_ID;
	for ( int i = 0; i < OCCLUSION_BUFFERS; i++ )
	{
		int j;
		for ( j = 0; j < nSlots; j++ )
			if ( pOcclusion->pnQueue[ j ] == i )
				break;
		if ( j == nSlots && pOcclusion->pnStates[ i ] != OCCLUSION_STATE_INVALID )
		{
			nQuery = i;
			break;
		}
	}
	ASSERT_RETFAIL( nQuery != INVALID_ID );

	// insert query buffer into the head
	MemoryMove( &pOcclusion->pnQueue[ 1 ], (OCCLUSION_BUFFERS - 1) * sizeof(int), pOcclusion->pnQueue, sizeof(int) * ( OCCLUSION_BUFFERS - 1 ) );
	pOcclusion->pnQueue[ 0 ] = nQuery;
	ASSERT_RETFAIL( pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] );

	ASSERT( pOcclusion->pnStates[ pOcclusion->pnQueue[ 0 ] ] == OCCLUSION_STATE_INIT ||
		pOcclusion->pnStates[ pOcclusion->pnQueue[ 0 ] ] == OCCLUSION_STATE_CHECKED );

	// debug track occlusion state
	ASSERT( ! gbOcclusionQueryActive );
	gbOcclusionQueryActive  = TRUE;
	ASSERT( gpLastBeginOcclusionPtr == gpLastEndOcclusionPtr );

	DX9_BLOCK( V( pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ]->Issue( D3DISSUE_BEGIN ) ); )
	DX10_BLOCK( pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ]->Begin(); )
	pOcclusion->pnStates [ pOcclusion->pnQueue[ 0 ] ] = OCCLUSION_STATE_ISSUING;
	gnOcclusionQueriesRendering++;
	gnOcclusionQueriesWaiting++;

	//DebugString( OP_DEBUG, "BeginQuery: %d", gnOcclusionQueriesWaiting );

	if ( gnDumpOcclusionQueries >= 0 )
	{
		V( dx9_DumpQuery( "Begin", nDebugModelID, nDebugType, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] ) );
	}

	// debug track occlusion ptrs
	gpLastBeginOcclusionPtr = pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ];

	// debug
	pOcclusion->pnType  [ pOcclusion->pnQueue[ 0 ] ] = nDebugType;
	pOcclusion->pnFrames[ pOcclusion->pnQueue[ 0 ] ] = 0;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_EndOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID )
{
#if ! ISVERSION(DEVELOPMENT)
	if ( AppIsTugboat() )
		return S_FALSE;
#endif

	BOOL bIssuing = FALSE;
	if ( pOcclusion )
		bIssuing = ( pOcclusion->pnStates[ pOcclusion->pnQueue[ 0 ] ] == OCCLUSION_STATE_ISSUING );
	// if Begin didn't issue a query, don't try to end it
	if ( ! bIssuing )
		return S_FALSE;

	BOOL bEnabled = ( e_GetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED ) );
	ASSERT( bEnabled );
	if ( ! bEnabled )
		return S_FALSE;

	// issue an end_query on the query at the head of the queue

	// debug track occlusion state
	ASSERT( gbOcclusionQueryActive );
	gbOcclusionQueryActive  = FALSE;

    ASSERT_RETFAIL( pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] );
	PRESULT pr = S_OK;
	DX9_BLOCK( V_SAVE( pr, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ]->Issue( D3DISSUE_END ) ); )
	DX10_BLOCK( pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ]->End(); )
	pOcclusion->pnStates [ pOcclusion->pnQueue[ 0 ] ] = OCCLUSION_STATE_PENDING;
	gnOcclusionQueriesRendering--;
	ASSERT( gnOcclusionQueriesRendering >= 0 );

	if ( gnDumpOcclusionQueries >= 0 )
	{
		V( dx9_DumpQuery( "End", nDebugModelID, pOcclusion->pnQueue[ 0 ], pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] ) );
	}

	// debug track occlusion ptrs
	ASSERT( gpLastBeginOcclusionPtr == pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] );
	gpLastEndOcclusionPtr   = pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ];
#ifdef ENGINE_TARGET_DX9
	if ( FAILED( pr ) )
	{
		// when this happens, must force device reset
		dx9_SetResetDevice();
	}
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_CheckOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDefaultPixels, BOOL bFlush /*= FALSE*/ )
{
#if ! ISVERSION(DEVELOPMENT)
	if ( AppIsTugboat() )
		return S_FALSE;
#endif

	if ( ! e_GetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED ) )
		return S_FALSE;

	int nDebugModelID = INVALID_ID;
	TRACE_MODEL( nDebugModelID, "Check Occlusion" );

	ASSERT_RETINVALIDARG( pOcclusion );

	if ( pOcclusion->pnQueue[ 0 ] == INVALID_ID || !pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ 0 ] ] )
	{
		// no query to check; use the specified default pixel result
		if ( ! e_GetRenderFlag( RENDER_FLAG_OCCLUSION_FREEZE ) )
			pOcclusion->dwPixelsDrawn = nDefaultPixels;
		return S_FALSE;
	}

	// debug
	for ( int nQuery = 0; nQuery < OCCLUSION_BUFFERS; nQuery++ )
	{
		if ( pOcclusion->pnQueue[ nQuery ] != INVALID_ID )
			pOcclusion->pnFrames[ pOcclusion->pnQueue[ nQuery ] ]++;
	}

	int nFailed = 0;

	for ( int nQuery = 0; nQuery < OCCLUSION_BUFFERS; nQuery++ )
	{
		if ( pOcclusion->pnQueue[ nQuery ] == INVALID_ID )
			break;

		ASSERT( pOcclusion->pnStates[ pOcclusion->pnQueue[ nQuery ] ] == OCCLUSION_STATE_PENDING );

		
		PRESULT pr;
		DWORD dwFlags = bFlush ? D3DGETDATA_FLUSH : 0;

#ifdef ENGINE_TARGET_DX9
		DWORD dwData = 0;
		V_SAVE( pr, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ nQuery ] ]->GetData( &dwData, sizeof(DWORD), dwFlags ) );
#elif defined( ENGINE_TARGET_DX10 )
		UINT64	dwData;
		V_SAVE( pr, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ nQuery ] ]->GetData( &dwData, sizeof(UINT64), dwFlags ) );
#endif
		switch ( pr )
		{
		case S_OK:		// query complete; remove it (and any issued before it) from the queue
			if ( ! e_GetRenderFlag( RENDER_FLAG_OCCLUSION_FREEZE ) )
			{
				//if ( e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
				//{
				//	if ( abs( (int)( pOcclusion->dwPixelsDrawn - dwData ) ) > 50 && pOcclusion->dwPixelsDrawn != 1 )
				//	{
				//		int nRoomID = INVALID_ID;
				//		if ( nDebugModelID != INVALID_ID )
				//		{
				//			D3D_MODEL * pModel = dx9_ModelGet( nDebugModelID );
				//			if ( pModel )
				//				nRoomID = pModel->m_idRoom;
				//		}
				//		DebugString( OP_DEBUG, L"Occlusion change: from %8d to %8d, model %4d, room %3d, type %5S, occl 0x%08p, nqry %d, pqry 0x%08p",
				//			pOcclusion->dwPixelsDrawn, 
				//			dwData, 
				//			nDebugModelID, 
				//			nRoomID, 
				//			dx9_GetOcclusionTypeString( (OCCLUSION_TYPE)pOcclusion->pnType[ pOcclusion->pnQueue[ nQuery ] ] ),
				//			(void*)pOcclusion,
				//			nQuery,
				//			(void*)pOcclusion->pD3DQuery[ nQuery ] );
				//	}
				//}
				pOcclusion->dwPixelsDrawn = dwData;

				TRACE_MODEL_INT( nDebugModelID, "Occlusion Returned", (int)pOcclusion->dwPixelsDrawn );
			}

			for ( int j = nQuery; j < OCCLUSION_BUFFERS; j++ )
			{
				if ( pOcclusion->pnQueue[ j ] == INVALID_ID )
					break;
				pOcclusion->pnStates[ pOcclusion->pnQueue[ j ] ] = OCCLUSION_STATE_CHECKED;
			}

			gnOcclusionQueriesWaiting--;
			ASSERT( gnOcclusionQueriesWaiting >= 0 );

			if ( gnDumpOcclusionQueries >= 0 )
			{
				V( dx9_DumpQuery( "Returned", nDebugModelID, (int)dwData, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ nQuery ] ] ) );
			}

			pOcclusion->pnQueue[ nQuery ] = INVALID_ID;
			goto finalize;
		case S_FALSE:	// hardware query result not ready yet
			TRACE_MODEL( nDebugModelID, "Occlusion Failed" );
			nFailed++;
			if ( gnDumpOcclusionQueries >= 0 )
			{
				V( dx9_DumpQuery( "Failed", nDebugModelID, -1, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ nQuery ] ] ) ); 
			}
			break;
		default:		// error returned, need to re-create query
			TRACE_MODEL( nDebugModelID, "Occlusion Error" );
			V( dx9_RestoreOcclusionQuery( pOcclusion, nDebugModelID ) );
			V( dx9_SetOccluded( pOcclusion, FALSE, nDebugModelID ) );
			if ( gnDumpOcclusionQueries >= 0 )
			{
				V( dx9_DumpQuery( "Error", nDebugModelID, -1, pOcclusion->pD3DQuery[ pOcclusion->pnQueue[ nQuery ] ] ) );
			}
		}
	}

finalize:
	// allow forcing of occlusion pixel result
	int nForce = e_GetRenderFlag( RENDER_FLAG_OCCLUSION_FORCE );
	if ( nForce >= 0 )
	{
		pOcclusion->dwPixelsDrawn = (DWORD)nForce;
	}

	//gtBatchInfo.nFailedOcclusion += nFailed;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_ClearOcclusionQueries( D3D_OCCLUSION * pOcclusion, int nDebugModelID )
{
	ASSERT_RETINVALIDARG( pOcclusion );
	pOcclusion->pnQueue[ 0 ] = INVALID_ID;
	for ( int j = 0; j < OCCLUSION_BUFFERS; j++ )
		pOcclusion->pnStates[ j ] = OCCLUSION_STATE_CHECKED;

	if ( gnDumpOcclusionQueries >= 0 )
	{
		V( dx9_DumpQuery( "Clear", nDebugModelID, 0, NULL ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL dx9_GetOccluded( D3D_OCCLUSION * pOcclusion )
{
#if ! ISVERSION(DEVELOPMENT)
	if ( AppIsTugboat() )
		return S_FALSE;
#endif

	if ( e_GetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED ) == FALSE )
		return FALSE;

	ASSERT_RETFALSE( pOcclusion );
	return (BOOL)( ! pOcclusion->dwPixelsDrawn );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_DumpQueryReportNewFrame()
{
	if ( gnDumpOcclusionQueries == -2 )
	{
		gnDumpOcclusionQueries = 3;
	}
	if ( gnDumpOcclusionQueries >= 0 )
	{
		V( dx9_DumpQuery( "NewFrame", -1, -1, NULL ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_DumpQueryEndFrame()
{
	if ( gnDumpOcclusionQueries == 0 )
	{
		gnDumpOcclusionQueries = -1;
		V( dx9_DumpDrawListBuffer() );
	}
	if ( gnDumpOcclusionQueries > 0 )
		gnDumpOcclusionQueries--;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_OcclusionQueryCreate( LPD3DCQUERY * ppQuery )
{
	ASSERT_RETINVALIDARG( ppQuery );

#ifdef ENGINE_TARGET_DX9

	V_RETURN( dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_OCCLUSION, ppQuery ) );

#elif defined(ENGINE_TARGET_DX10)

	D3D10_QUERY_DESC Desc;
	Desc.Query = D3D10_QUERY_OCCLUSION;
	Desc.MiscFlags = 0;
	V_RETURN( dxC_GetDevice()->CreateQuery( &Desc, ppQuery ) );

#endif

	ASSERT_RETFAIL( ppQuery );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_OcclusionQueryBegin( SPD3DCQUERY pQuery )
{
	ASSERT_RETINVALIDARG( pQuery );
	DX9_BLOCK( V_RETURN( pQuery->Issue( D3DISSUE_BEGIN ) ) );
	DX10_BLOCK( pQuery->Begin() );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_OcclusionQueryEnd( SPD3DCQUERY pQuery )
{
	ASSERT_RETINVALIDARG( pQuery );
	DX9_BLOCK( V_RETURN( pQuery->Issue( D3DISSUE_END ) ) );
	DX10_BLOCK( pQuery->End() );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_OcclusionQueryGetData( SPD3DCQUERY & pQuery, D3DC_QUERY_DATA & tData, BOOL bFlush /*= FALSE*/ )
{
	PRESULT pr;

#ifdef ENGINE_TARGET_DX9
	V_SAVE( pr, pQuery->GetData( &tData, sizeof(D3DC_QUERY_DATA), bFlush ? D3DGETDATA_FLUSH : 0 ) );
#elif defined( ENGINE_TARGET_DX10 )
	V_SAVE( pr, pQuery->GetData( &tData, sizeof(D3DC_QUERY_DATA), bFlush ? 0 : D3D10_ASYNC_GETDATA_DONOTFLUSH ) );
#endif

	switch ( pr )
	{
	case S_OK:		// result returned successfully
		break;
	case S_FALSE:	// hardware query result not ready yet
		break;
	default:
		// error returned, need to re-create query
		pQuery = NULL;
		V_RETURN( dxC_OcclusionQueryCreate( &pQuery ) );
		tData = (D3DC_QUERY_DATA)0;
	}

	return pr;
}