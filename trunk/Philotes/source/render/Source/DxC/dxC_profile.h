//----------------------------------------------------------------------------
// dxC_profile.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_PROFILE__
#define __DX9_PROFILE__

#include "e_profile.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#if defined(PIX_PROFILE) && defined(ENGINE_TARGET_DX9)
#define D3D_PROFILE_REGION( pwszName ) D3D_PERF_EVENT __d3d_perf_event_##__LINE__( pwszName );
#define D3D_PROFILE_REGION_STR( pwszName, str ) D3D_PERF_EVENT __d3d_perf_event##__LINE__( pwszName, str );
#define D3D_PROFILE_REGION_INT( pwszName, val ) D3D_PERF_EVENT __d3d_perf_event##__LINE__( pwszName, val );
#define D3D_PROFILE_REGION_INT_STR( pwszName, val, str ) D3D_PERF_EVENT __d3d_perf_event##__LINE__( pwszName, val, str );
#define D3D_PROFILE_MARKER( pwszName ) D3DPERF_SetMarker( cgdwProfileColor, pwszName );
#else
#define D3D_PROFILE_REGION( pwszName ) 
#define D3D_PROFILE_REGION_STR( pwszName, str ) 
#define D3D_PROFILE_REGION_INT( pwszName, val ) 
#define D3D_PROFILE_REGION_INT_STR( pwszName, val, str ) 
#define D3D_PROFILE_MARKER( pwszName ) 
#endif

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


// uncomment to enable perf event traces
//#define DEBUG_PERF_EVENTS

DWORD const cgdwProfileColor = 0xff7f7f7f;
#ifdef ENGINE_TARGET_DX9 //Need dx10 version of PIX
class D3D_PERF_EVENT
{
	int nEventID;
	WCHAR szName[256];
public:
	D3D_PERF_EVENT( LPCWSTR pszName )
	{
		PStrCopy( szName, pszName, 256 );
		nEventID = D3DPERF_BeginEvent( cgdwProfileColor, szName );
#ifdef DEBUG_PERF_EVENTS
		char szOut[ 256 ];
		wsprintf( szOut, "%32ls - Begin(%3d)", szName, nEventID );
		//LogMessage( PERF_LOG, "D3DPerf_Begin %s", szOut );
		OutputDebugString( szOut );
#endif
	}
	D3D_PERF_EVENT( LPCWSTR pszName, LPCWSTR pszStr )
	{
		PStrCopy( szName, pszName, 256 );
		PStrCat( szName, pszStr, 256 );
		nEventID = D3DPERF_BeginEvent( cgdwProfileColor, szName );
#ifdef DEBUG_PERF_EVENTS
		char szOut[ 256 ];
		wsprintf( szOut, "%32ls - Begin(%3d)", szName, nEventID );
		//LogMessage( PERF_LOG, "D3DPerf_Begin %s", szOut );
		OutputDebugString( szOut );
#endif
	}
	D3D_PERF_EVENT( LPCWSTR pszName, int nValue )
	{
		PStrPrintf( szName, 256, L"%s %d", pszName, nValue );
		nEventID = D3DPERF_BeginEvent( cgdwProfileColor, szName );
#ifdef DEBUG_PERF_EVENTS
		char szOut[ 256 ];
		wsprintf( szOut, "%32ls - Begin(%3d)", szName, nEventID );
		//LogMessage( PERF_LOG, "D3DPerf_Begin %s", szOut );
		OutputDebugString( szOut );
#endif
	}
	D3D_PERF_EVENT( LPCWSTR pszName, int nValue, LPCSTR pszStr )
	{
		PStrPrintf( szName, 256, L"%s %d %S", pszName, nValue, pszStr );
		nEventID = D3DPERF_BeginEvent( cgdwProfileColor, szName );
#ifdef DEBUG_PERF_EVENTS
		char szOut[ 256 ];
		wsprintf( szOut, "%32ls - Begin(%3d)", szName, nEventID );
		//LogMessage( PERF_LOG, "D3DPerf_Begin %s", szOut );
		OutputDebugString( szOut );
#endif
	}
	~D3D_PERF_EVENT( void )
	{
		int nEndedEventID = D3DPERF_EndEvent();
		char szOut[ 256 ];
#ifdef DEBUG_PERF_EVENTS
		wsprintf( szOut, "%32ls - Begin(%3d) End(%3d)", szName, nEventID, nEndedEventID );
		//LogMessage( PERF_LOG, "D3DPerf_End   %s", szOut );
		OutputDebugString( szOut );
#else
		wsprintf( szOut, "%ls - Begin(%d) End(%d)", szName, nEventID, nEndedEventID );
#endif
		ASSERTX( nEndedEventID == nEventID, szOut );
	}
};
#endif

#ifdef DEBUG_PERF_EVENTS
#undef DEBUG_PERF_EVENTS
#endif


struct D3DSTATUS
{
	DWORD	dwVidMemTotal;
	DWORD	dwVidMemRenderTargets;
	DWORD	dwVidMemVertexBuffer;
	DWORD	dwVidMemTexture;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern D3DSTATUS gD3DStatus;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_EngineMemoryGetPool( int nPool, ENGINE_MEMORY & tMemory, DWORD* pdwBytesTotal = NULL );
void dx9_TrackD3DResourceManagerStats();

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DX9_QUERY__