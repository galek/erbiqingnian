//----------------------------------------------------------------------------
// e_automap.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_AUTOMAP__
#define __E_AUTOMAP__

#include "e_common.h"

namespace FSSE
{;



//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define ENABLE_NEW_AUTOMAP

#if ISVERSION(SERVER_VERSION)
#	undef ENABLE_NEW_AUTOMAP
#endif

#define AUTOMAP_COMPONENT_NAME		"automap"

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

PRESULT e_AutomapsInit();
PRESULT e_AutomapsDestroy();

PRESULT e_AutomapCreate( int nRegion );
PRESULT e_AutomapDestroy( int nRegion );

PRESULT e_AutomapsMarkDirty();

PRESULT e_AutomapAddTri(
	int nID,
	const VECTOR * pv1,
	const VECTOR * pv2,
	const VECTOR * pv3,
	BOOL bWall );
PRESULT e_AutomapPrealloc(
	int nID,
	int nAddlTris );
PRESULT e_AutomapRenderGenerate(
	int nID );
PRESULT e_AutomapRenderDisplay(
	int nID,
	const E_RECT & tRect,
	DWORD dwColor,
	float fZFinal,
	float fScale );
PRESULT e_AutomapSetDisplay(
	const VECTOR * pvPos,
	const MATRIX * pmRotate,
	float * pfScale );
PRESULT e_AutomapGetItemPos(
	int nID,
	const VECTOR & vWorldPos,
	VECTOR & vNewPos,
	const E_RECT & tRect );
PRESULT e_AutomapSetShowAll(
	int nID,
	BOOL bShowAll );

PRESULT e_AutomapSave(
	class WRITE_BUF_DYNAMIC & fbuf );
PRESULT e_AutomapLoad(
	class BYTE_BUF & buf );


};	// FSSE
#endif // __E_AUTOMAP__