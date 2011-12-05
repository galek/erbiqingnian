//----------------------------------------------------------------------------
// e_portal.h
//
// - Header for portal functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_PORTAL__
#define __E_PORTAL__

#include "boundingbox.h"
#include "e_region.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// squarish portals
#define PORTAL_NUM_CORNERS		4

#define PORTAL_FLAG_ACTIVE				MAKE_MASK(0)
//#define PORTAL_FLAG_VISIBLE				MAKE_MASK(1)
//#define PORTAL_FLAG_NEEDUPDATE			MAKE_MASK(2)
#define PORTAL_FLAG_SHAPESET			MAKE_MASK(3)
//#define PORTAL_FLAG_TRAVERSED			MAKE_MASK(4)
#define PORTAL_FLAG_NODRAW				MAKE_MASK(5)
#define PORTAL_FLAG_TARGETSET			MAKE_MASK(6)

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
#pragma warning(push)
#pragma warning(disable:4324)	// warning C4324: 'PORTAL' : structure was padded due to __declspec(align())
struct PORTAL
{
	int				nId;
	PORTAL *		pNext;
	PORTAL *		pConnection;						// the other half of this portal

	DWORD			dwFlags;
	int				nRegion;							// in which region does this portal reside?
	//int				nRoomID;							// user payload (eg, room id associated with this side of the portal)
	int				nCellID;

	VECTOR			vCorners[ PORTAL_NUM_CORNERS ];		// corners of the portal object in world space, for each region
	int				nCorners;							// number of corners (quad? tri? pent? hex?)
	int				nEdges[ PORTAL_NUM_CORNERS ][ 2 ];	// index list of edges
	int				nStrip[ PORTAL_NUM_CORNERS ];		// index list to form tristrip

	VECTOR			vCenter;							// computed centroid
	VECTOR			vNormal;							// computed normal
	BOUNDING_BOX	tBBox;								// world-space bounding box of the portal corners

	//CULLING_VIRTUAL_PORTAL tCullingPortal;
	int				nCullingPortalID;

	//BOUNDING_BOX	tScreenBBox;						// screen-space bounding box of the portal corners, set during render
	//VECTOR			vScreenCorners[ PORTAL_NUM_CORNERS ];	// screen-space portal corners

	//MATRIX			mTransform;							// connected space into view (like a normal view matrix)
	//MATRIX			mTransformInverse;					// view into connected space (pushing local vectors through the portal)

	//MATRIX			mView;								// view transform through the portal
	//VECTOR			vEyeLocation;						// eye location in remote portal space
	//VECTOR			vEyeDirection;						// eye direction in remote portal space
	//VECTOR			vEyeLookat;							// eye lookat in remote portal space

	//int				nPortalTextureID;					// texture to which the portal scene is copied
};
#pragma warning(pop)


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_PortalsInit();
PRESULT e_PortalsDestroy();
PRESULT e_PortalsReset();
int e_PortalCreate(
	int nRegion );
void e_PortalSetDestination(
	PORTAL * pPortalFrom,
	PORTAL * pPortalDest );
void e_PortalSetCellID(
	PORTAL * pPortal,
	int nUserData );
PRESULT e_PortalConnect(
	PORTAL * pPortalFrom );
PRESULT e_PortalSetShape(
	PORTAL * pPortal,
	const VECTOR * pvCorners,
	int nCorners,
	const VECTOR * pvNormal );
void e_PortalRemove(
	int nPortalID );
void e_PortalUpdateMatrices(
	PORTAL * pPortal,
	const VECTOR & vEye,
	const VECTOR & vLookat );
void e_PortalsUpdateVisible(
	const VECTOR & vEye,
	const VECTOR & vLookat );
void e_PortalDebugRender(
	int nPortalID );
void e_PortalDebugRenderAll();
void e_DebugDrawPortal( BOUNDING_BOX & tClipBox, DWORD dwColor, int nDepth );
void e_DebugRenderPortals();
PORTAL * e_PortalGetFirstVisible();
PORTAL * e_PortalGetNextVisible( PORTAL * pPortal );
void e_PortalRestore( PORTAL * pPortal );
void e_PortalRelease( PORTAL * pPortal );
PRESULT e_PortalsRestoreAll();
PRESULT e_PortalsReleaseAll();
PRESULT e_PortalStoreRender( int nPortalID );
BOOL e_PortalIsActive( int nPortal );
BOOL e_PortalIsActive( PORTAL * pPortal );
PRESULT e_SetupPortal( int nPortal );
BOOL e_PortalHasFlags( PORTAL * pPortal, DWORD dwFlags );
BOOL e_PortalHasConnection( PORTAL * pPortal );


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline PORTAL * e_PortalGet( int nPortalID )
{
	extern CHash<PORTAL> gtPortals;
	return HashGet( gtPortals, nPortalID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline PORTAL * e_PortalGetFirst()
{
	extern CHash<PORTAL> gtPortals;
	return HashGetFirst( gtPortals );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline PORTAL * e_PortalGetNext( PORTAL * pPortal )
{
	extern CHash<PORTAL> gtPortals;
	return HashGetNext( gtPortals, pPortal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_PortalSetFlags( PORTAL * pPortal, DWORD dwFlags, BOOL bSet )
{
	if ( bSet )
		pPortal->dwFlags |= dwFlags;
	else
		pPortal->dwFlags &= ~(dwFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_PortalSetFlags( int nPortalID, DWORD dwFlags, BOOL bSet )
{
	PORTAL * pPortal = e_PortalGet( nPortalID );
	ASSERT_RETURN( pPortal );
	e_PortalSetFlags( pPortal, dwFlags, bSet );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline DWORD e_PortalGetFlags( PORTAL * pPortal )
{
	ASSERT_RETZERO( pPortal );
	return pPortal->dwFlags;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline DWORD e_PortalGetFlags( int nPortalID )
{
	PORTAL * pPortal = e_PortalGet( nPortalID );
	return e_PortalGetFlags( pPortal );
}


#endif // __E_PORTAL__
