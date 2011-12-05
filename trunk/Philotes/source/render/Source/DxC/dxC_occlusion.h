//----------------------------------------------------------------------------
// dxC_occlusion.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_OCCLUSION__
#define __DXC_OCCLUSION__

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define OCCLUSION_BUFFERS			1

// below this distance from eye, don't allow occlusion - helps to clear up ugly near-user popping
#define MIN_OCCLUSION_DISTANCE		10.0f

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct D3D_OCCLUSION
{
	SPD3DCQUERY									pD3DQuery[ OCCLUSION_BUFFERS ];
	int											pnStates [ OCCLUSION_BUFFERS ];
	int											pnQueue  [ OCCLUSION_BUFFERS ];
	DX9_BLOCK( DWORD ) DX10_BLOCK( UINT64 )		dwPixelsDrawn;

	// debug
	int											pnFrames [ OCCLUSION_BUFFERS ];
	int											pnType   [ OCCLUSION_BUFFERS ];
};

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

enum OCCLUSION_TYPE
{
	OCCLUSION_TYPE_NONE  = -1,
	OCCLUSION_TYPE_MODEL = 0,
	OCCLUSION_TYPE_BBOX,
	NUM_OCCLUSION_TYPES
};

enum OCCLUSION_STATE
{
	OCCLUSION_STATE_INVALID = -2,
	OCCLUSION_STATE_INIT = -1,
	OCCLUSION_STATE_ISSUING,
	OCCLUSION_STATE_PENDING,
	OCCLUSION_STATE_CHECKED,
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern int gnOcclusionQueriesRendering;
extern int gnOcclusionQueriesWaiting;

extern const char cgszOcclusionType[ NUM_OCCLUSION_TYPES ][ 16 ];

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline const char * dx9_GetOcclusionTypeString( OCCLUSION_TYPE eType )
{
	if ( eType >= 0 && eType < NUM_OCCLUSION_TYPES )
		return cgszOcclusionType[ eType ];
	return "None";
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void dx9_DumpQueryReportNewFrame();
void dx9_DumpQueryEndFrame();
void dx9_OcclusionGetCounts( int & rnObjectsCurrent, int & rnObjectsTotal, int & rnRendering, int & rnWaiting );
PRESULT dx9_ReleaseAllOcclusionQueries();
PRESULT dx9_RestoreAllOcclusionQueries();

PRESULT dx9_CheckOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDefaultPixels = 0, BOOL bFlush = FALSE );
PRESULT dx9_BeginOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugType = -1, int nDebugModelID = INVALID_ID );
PRESULT dx9_EndOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID = INVALID_ID );
PRESULT dx9_RestoreOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID = INVALID_ID );
PRESULT dx9_ReleaseOcclusionQuery( D3D_OCCLUSION * pOcclusion, int nDebugModelID = INVALID_ID );
PRESULT dx9_ClearOcclusionQueries( D3D_OCCLUSION * pOcclusion, int nDebugModelID = INVALID_ID );
PRESULT dx9_SetOccluded( D3D_OCCLUSION * pOcclusion, BOOL bOccluded, int nDebugModelID = INVALID_ID );
BOOL dx9_GetOccluded( D3D_OCCLUSION * pOcclusion );


PRESULT dxC_OcclusionQueryCreate( LPD3DCQUERY * ppQuery );
PRESULT dxC_OcclusionQueryBegin( SPD3DCQUERY pQuery );
PRESULT dxC_OcclusionQueryEnd( SPD3DCQUERY pQuery );
PRESULT dxC_OcclusionQueryGetData( SPD3DCQUERY & pQuery, D3DC_QUERY_DATA & dwData, BOOL bFlush = FALSE );


#endif //__DXC_OCCLUSION__