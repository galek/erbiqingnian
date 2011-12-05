//----------------------------------------------------------------------------
// e_profile.cpp
//
// - Implementation for profile/perf functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_profile.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BATCH_DEBUG_INFO gtBatchInfo;

#if ISVERSION(DEVELOPMENT)
int gnDebugTraceBatches = -1;
int gnDebugTraceBatchesGroup = INVALID_ID;
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetBatchString( 
	WCHAR * pszString, 
	int nSize )
{
#define BATCH_LINE( str, grp )	str, gtBatchInfo.nBatches[ grp ], gtBatchInfo.nPolygons[ grp ]

	PStrPrintf( pszString, nSize, 
			L"%10s  %7s %10s\n"		// HEADER
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"
			L"%10s: %7d %10d\n"		// TOTAL
			L"Rooms[ Visible/InFrustum: %3d/%3d (%3.2f) ]\nPartSysVisible: %3d\n"
			,
			L"Group",	L"Batches",		L"Polygons",
			BATCH_LINE( L"ANIMATED",	METRICS_GROUP_ANIMATED ),
			BATCH_LINE( L"BACKGROUND",	METRICS_GROUP_BACKGROUND ),
			BATCH_LINE( L"SHADOW",		METRICS_GROUP_SHADOW ),
			BATCH_LINE( L"ZPASS",		METRICS_GROUP_ZPASS ),
			BATCH_LINE( L"UI",			METRICS_GROUP_UI ),
			BATCH_LINE( L"PARTICLE",	METRICS_GROUP_PARTICLE ),
			BATCH_LINE( L"EFFECTS",		METRICS_GROUP_EFFECTS ),
			BATCH_LINE( L"DEBUG",		METRICS_GROUP_DEBUG ),
			BATCH_LINE( L"UNKNOWN",		METRICS_GROUP_UNKNOWN ),
			L"TOTAL",		gtBatchInfo.nTotalBatches,		gtBatchInfo.nTotalPolygons,
			gtBatchInfo.nRoomsVisible,
			gtBatchInfo.nRoomsInFrustum,
			( gtBatchInfo.nRoomsInFrustum == 0 ) ? 0.f : (float)gtBatchInfo.nRoomsVisible / gtBatchInfo.nRoomsInFrustum,
			gtBatchInfo.nParticleSystemsVisible );

#undef BATCH_LINE
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ZeroRenderMetrics()
{
	ZeroMemory( &gtBatchInfo, sizeof( gtBatchInfo ) );

#if ISVERSION(DEVELOPMENT)
	if ( gnDebugTraceBatches >= 0 )
		gnDebugTraceBatches--;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_MetricsGetTotalBatchCount()
{
	return gtBatchInfo.nTotalBatches;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetBatchStats(
	int * pnTotalBatches,
	int * pnAnimatedBatches,
	int * pnRoomsInFrustum,
	int * pnRoomsVisible,
	//int * pnRoomBatches,
	int * pnParticleBatches,
	int * pnParticleSystemsVisible,
	int * pnUIBatches,
	int * pnZBatches,
	int * pnShadowBatches )
{
	if ( pnTotalBatches )			*pnTotalBatches = gtBatchInfo.nTotalBatches;
	if ( pnAnimatedBatches )		*pnAnimatedBatches = gtBatchInfo.nBatches[ METRICS_GROUP_ANIMATED ];
	if ( pnRoomsInFrustum )			*pnRoomsInFrustum = gtBatchInfo.nRoomsInFrustum;
	if ( pnRoomsVisible )			*pnRoomsVisible = gtBatchInfo.nRoomsVisible;
	//if ( pnRoomBatches )			*pnRoomBatches = gtBatchInfo.nRoomBatches;
	if ( pnParticleBatches )		*pnParticleBatches = gtBatchInfo.nBatches[ METRICS_GROUP_PARTICLE ];
	if ( pnParticleSystemsVisible ) *pnParticleSystemsVisible = gtBatchInfo.nParticleSystemsVisible;
	if ( pnUIBatches )				*pnUIBatches = gtBatchInfo.nBatches[ METRICS_GROUP_UI ];
	if ( pnZBatches )				*pnZBatches = gtBatchInfo.nBatches[ METRICS_GROUP_ZPASS ];
	if ( pnShadowBatches )			*pnShadowBatches = gtBatchInfo.nBatches[ METRICS_GROUP_SHADOW ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

unsigned int e_MetricsGetTotalPolygonCount()
{
	return gtBatchInfo.nTotalPolygons;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

unsigned int e_MetricsGetPolygonCount( METRICS_GROUP eGroup )
{
	ASSERT_RETZERO( IS_VALID_INDEX( eGroup, NUM_METRICS_GROUPS ) );
	return gtBatchInfo.nPolygons[ eGroup ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

unsigned int e_MetricsGetBatchCount( METRICS_GROUP eGroup )
{
	ASSERT_RETZERO( IS_VALID_INDEX( eGroup, NUM_METRICS_GROUPS ) );
	return gtBatchInfo.nBatches[ eGroup ];
}
