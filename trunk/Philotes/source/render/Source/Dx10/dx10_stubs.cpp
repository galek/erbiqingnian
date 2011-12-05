//----------------------------------------------------------------------------
// dx10_stubs.cpp
// Stubs for	DX10
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
//Profile stuff, need to get this working with DX10 queries
void e_GetStatesStatsString( WCHAR * pszString, int nSize ){};
void e_GetStatsString( WCHAR * pszString, int nSize ){};
void e_GetHashMetricsString( WCHAR * pszString, int nSize ){};
void e_ProfileSetMarker( const WCHAR * pwszString, BOOL bForceNewFrame ){};
void e_ToggleTrackResourceStats(){};