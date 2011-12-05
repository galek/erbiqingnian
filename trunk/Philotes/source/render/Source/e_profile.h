//----------------------------------------------------------------------------
// e_profile.h
//
// - Header for profile/perf functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_PROFILE__
#define __E_PROFILE__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum METRICS_GROUP
{
	METRICS_GROUP_INVALID = -1,
	METRICS_GROUP_ALL = METRICS_GROUP_INVALID,
	//
	METRICS_GROUP_ANIMATED = 0,
	METRICS_GROUP_BACKGROUND,
	METRICS_GROUP_SHADOW,
	METRICS_GROUP_ZPASS,
	METRICS_GROUP_UI,
	METRICS_GROUP_PARTICLE,
	METRICS_GROUP_EFFECTS,
	METRICS_GROUP_DEBUG,
	METRICS_GROUP_UNKNOWN,
	//
	NUM_METRICS_GROUPS
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct BATCH_DEBUG_INFO
{
	unsigned int nTotalBatches;
	unsigned int nBatches[ NUM_METRICS_GROUPS ];

	unsigned int nRoomsInFrustum;
	unsigned int nRoomsVisible;
	unsigned int nParticleSystemsVisible;

	unsigned int nTotalPolygons;
	unsigned int nPolygons[ NUM_METRICS_GROUPS ];
};

struct ENGINE_MEMORY
{
	DWORD dwTextures;
	DWORD dwTextureBytes;

	DWORD dwVBuffers;
	DWORD dwVBufferBytes;

	DWORD dwIBuffers;
	DWORD dwIBufferBytes;

	void Zero()		{ ZeroMemory(this, sizeof(ENGINE_MEMORY) ); }
	DWORD Total()	{ return dwTextureBytes + dwVBufferBytes + dwIBufferBytes; }
	void DebugTrace( const char * pszHdr )
	{
		DebugString( OP_DEBUG, "%s-- Textures (%4d): %10d bytes\n-- VBuffers (%4d): %10d bytes\n-- IBuffers (%4d): %10d bytes\n-- Total:           %10d bytes\n",
			pszHdr ? pszHdr : "",
			dwTextures,
			dwTextureBytes,
			dwVBuffers,
			dwVBufferBytes,
			dwIBuffers,
			dwIBufferBytes,
			Total() );
	}
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern BATCH_DEBUG_INFO gtBatchInfo;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void e_ReportPolygons( unsigned int nCount, METRICS_GROUP eGroup )
{
	gtBatchInfo.nPolygons[ eGroup ]	+= nCount;
	gtBatchInfo.nTotalPolygons		+= nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_ReportBatch( METRICS_GROUP eGroup )
{
	gtBatchInfo.nBatches[ eGroup ]++;
	gtBatchInfo.nTotalBatches++;

#if ISVERSION(DEVELOPMENT)
	extern int gnDebugTraceBatches;
	extern int gnDebugTraceBatchesGroup;
	if ( gnDebugTraceBatches >= 0
		&& ( gnDebugTraceBatchesGroup == INVALID_ID || gnDebugTraceBatchesGroup == eGroup ) )
	{
		trace( "Batch: Group %d\n", eGroup );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_DebugTraceBatches( METRICS_GROUP eGroup = METRICS_GROUP_ALL )
{
#if ISVERSION(DEVELOPMENT)
	extern int gnDebugTraceBatches;
	extern int gnDebugTraceBatchesGroup;
	gnDebugTraceBatches = 1;
	gnDebugTraceBatchesGroup = eGroup;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_MetricsIncRoomsInFrustum()
{
	gtBatchInfo.nRoomsInFrustum++;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

int e_MetricsGetTotalBatchCount();
void e_GetBatchStats(
	int * pnTotalBatches,
	int * pnAnimatedBatches = NULL,
	int * pnRoomsInFrustum = NULL,
	int * pnRoomsVisible = NULL,
	//int * pnRoomBatches = NULL,
	int * pnParticleBatches = NULL,
	int * pnParticleSystemsVisible = NULL,
	int * pnUIBatches = NULL,
	int * pnZBatches = NULL,
	int * pnShadowBatches = NULL );
unsigned int e_MetricsGetBatchCount( METRICS_GROUP eGroup );
unsigned int e_MetricsGetTotalPolygonCount();
unsigned int e_MetricsGetPolygonCount( METRICS_GROUP eGroup );
void e_GetBatchString( WCHAR * pszString, int nSize );
void e_GetStatesStatsString( WCHAR * pszString, int nSize );
void e_GetStatsString( WCHAR * pszString, int nSize );
void e_GetHashMetricsString( WCHAR * pszString, int nSize );
void e_ZeroRenderMetrics();
void e_ProfileSetMarker( const WCHAR * pwszString, BOOL bForceNewFrame = FALSE );
void e_ToggleTrackResourceStats();
PRESULT e_EngineMemoryDump( BOOL bOutputTrace = TRUE, DWORD* pdwBytesTotal = NULL );

#endif // __E_PROFILE__
