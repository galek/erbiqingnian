//----------------------------------------------------------------------------
// e_region.h
//
// - Header for engine-API region/culling interface code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_REGION__
#define __E_REGION__

#include "e_dpvs.h"
#include <map>
#include "memoryallocator_stl.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define REGION_INVALID			-1
#define REGION_DEFAULT			0

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef std::map<DWORD, struct CULLING_CELL, std::less<DWORD>, FSCommon::CMemoryAllocatorSTL<std::pair<DWORD, struct CULLING_CELL> > > CellMap;
typedef std::map<DWORD, struct CULLING_PHYSICAL_PORTAL, std::less<DWORD>, FSCommon::CMemoryAllocatorSTL<std::pair<DWORD, struct CULLING_PHYSICAL_PORTAL> > > PhysicalPortalMap;
typedef std::map<DWORD, struct CULLING_VIRTUAL_PORTAL, std::less<DWORD>, FSCommon::CMemoryAllocatorSTL<std::pair<DWORD, struct CULLING_VIRTUAL_PORTAL> > > VirtualPortalMap;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct CULLING_PHYSICAL_PORTAL
{
	LP_INTERNAL_PHYSICAL_PORTAL pInternalPortal;
	PLANE						tPlane;
	VECTOR						vCenter;
	float						fRadiusSq;
	int							nSourceCellID;
	int							nTargetCellID;
	void Destroy();
	void Init()				{ pInternalPortal = NULL; nSourceCellID = INVALID_ID; nTargetCellID = INVALID_ID; }
};

struct CULLING_VIRTUAL_PORTAL
{
	LP_INTERNAL_VIRTUAL_PORTAL	pInternalPortal;
	PLANE						tPlane;
	VECTOR						vCenter;
	void Destroy();
	void Init()				{ pInternalPortal = NULL; }
};

struct CULLING_CELL
{
	MATRIX			 mWorld;
	int				 nRegion;
	LP_INTERNAL_CELL pInternalCell;
	void Destroy();
	void Init()				{ MatrixIdentity( &mWorld ); pInternalCell = NULL; }
};

struct REGION
{
	enum FLAGBITS
	{
	};

	int nId;
	REGION * pNext;

	int nCullCell;
	DWORD dwFlagBits;

	VECTOR vOffset;										// world offset to origin of region
	int nEnvironmentDef;								// current environment for this region
	int nPrevEnvironmentDef;							// for transitions
	int nWaitEnvironmentDef;							// waiting for load on this env
	float fEnvTransitionPhase;
	TIME tEnvTransitionBegin;
	BOOL bSavePrevEnvironmentDef;
	BOUNDING_BOX tBBox;							// bounding box for this region
	int nAutomapID;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline DWORD e_GetVisibilityToken()
{
	extern DWORD gdwVisibilityToken;
	return gdwVisibilityToken;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline REGION * e_RegionGet( int nRegionID )
{
#if !ISVERSION(SERVER_VERSION)
	extern CHash<REGION> gtRegions;
	return HashGet( gtRegions, nRegionID );
#else
	UNREFERENCED_PARAMETER(nRegionID);
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline int e_RegionGetCount()
{
#if !ISVERSION(SERVER_VERSION)
	extern CHash<REGION> gtRegions;
	return HashGetCount( gtRegions );
#else
	return 0;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline REGION * e_RegionGetFirst()
{
#if !ISVERSION(SERVER_VERSION)
	extern CHash<REGION> gtRegions;
	return HashGetFirst( gtRegions );
#else
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline REGION * e_RegionGetNext( REGION * pRegion )
{
#if !ISVERSION(SERVER_VERSION)
	extern CHash<REGION> gtRegions;
	return HashGetNext( gtRegions, pRegion );
#else
	UNREFERENCED_PARAMETER(pRegion);
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline CULLING_CELL* e_CellGet( int nCellID )
{
#if !ISVERSION(SERVER_VERSION)
	extern CellMap* g_Cells;
	ASSERT_RETNULL( g_Cells );
	if ( nCellID < 0 )
		return NULL;

	DWORD dwKey = static_cast<DWORD>(nCellID);

	CellMap::iterator i = g_Cells->find( dwKey );
	//ASSERTV_RETNULL( i != g_Cells->end(), "Couldn't find CELL with ID %d", nCellID );
	if ( i == g_Cells->end() )
	{
		return NULL;
	}

	return &(*g_Cells)[ dwKey ];
#else
	UNREFERENCED_PARAMETER(nCellID);
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline CULLING_PHYSICAL_PORTAL* e_PhysicalPortalGet( int nPortalID )
{
#if !ISVERSION(SERVER_VERSION)
	extern PhysicalPortalMap* g_PhysicalPortals;
	ASSERT_RETNULL( g_PhysicalPortals );
	ASSERT_RETNULL( nPortalID >= 0 );

	DWORD dwKey = static_cast<DWORD>(nPortalID);

	PhysicalPortalMap::iterator i = g_PhysicalPortals->find( dwKey );
	ASSERTV_RETNULL( i != g_PhysicalPortals->end(), "Couldn't find CULLING_PHYSICAL_PORTAL with ID %d", nPortalID );
	//if ( i == g_Cells->end() )
	//{
	//	return NULL;
	//}

	return &(*g_PhysicalPortals)[ dwKey ];
#else
	UNREFERENCED_PARAMETER(nPortalID);
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline CULLING_VIRTUAL_PORTAL* e_VirtualPortalGet( int nPortalID )
{
#if !ISVERSION(SERVER_VERSION)
	extern VirtualPortalMap* g_VirtualPortals;
	ASSERT_RETNULL( g_VirtualPortals );
	ASSERT_RETNULL( nPortalID >= 0 );

	DWORD dwKey = static_cast<DWORD>(nPortalID);

	VirtualPortalMap::iterator i = g_VirtualPortals->find( dwKey );
	ASSERTV_RETNULL( i != g_VirtualPortals->end(), "Couldn't find CULLING_VIRTUAL_PORTAL with ID %d", nPortalID );
	//if ( i == g_Cells->end() )
	//{
	//	return NULL;
	//}

	return &(*g_VirtualPortals)[ dwKey ];
#else
	UNREFERENCED_PARAMETER(nPortalID);
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void		e_UpdateVisibilityToken(	);

PRESULT		e_RegionsInit(				);
PRESULT		e_RegionsDestroy(			);
PRESULT		e_RegionsReset(				);

PRESULT		e_SetRegion(				int nRegion );
REGION *	e_GetCurrentRegion(			);
int			e_GetCurrentRegionID(		);
PRESULT		e_RegionAdd(				int nRegionID );
void		e_RegionRemove(				int nRegionID );
void		e_RegionSetOffset(			int nRegion,
										const VECTOR * pVector );
void		e_RegionGetOffset(			int nRegion,
										VECTOR * pVector );
PRESULT		e_RegionGetCell(			int nRegion,
										int & nCell );

PRESULT		e_CellCreate(				int & nCellID, int nRegion, const MATRIX * pmWorld = NULL );
PRESULT		e_CellCreateWithID(			int nCellID, int nRegion, const MATRIX * pmWorld = NULL );
PRESULT		e_CellRemove(				int nCellID );
PRESULT		e_CellCreatePhysicalPortal(	__out int & nPortalID,
									    int nFromCellID,
										int nToCellID,
										const VECTOR * pvCorners,
										int nCorners,
										const VECTOR & vNormal,
										const MATRIX * pmWorldInv = NULL );
PRESULT		e_CellCreatePhysicalPortalPair(	__out int & nPortal1ID,
											__out int & nPortal2ID,
									    	int nCellID1,
											int nCellID2,
											const VECTOR * pvCorners1,
											const VECTOR * pvCorners2,
											int nCorners,
											const VECTOR & vNormal1,
											const VECTOR & vNormal2,
											const MATRIX * pmWorldInv1 = NULL,
											const MATRIX * pmWorldInv2 = NULL );
PRESULT		e_CellRemovePhysicalPortal(	int nPortalID );
PRESULT		e_CellCreateVirtualPortal(	__out int & nPortalID,
									    int nFromCellID,
										const VECTOR * pvCorners,
										int nCorners,
										const VECTOR & vNormal );
PRESULT		e_CellRemoveVirtualPortal( int nPortalID );
PRESULT		e_CellFindSourcePhysicalPortals( int nCellID,
											 __out int * pnPortalIDs,
											 __out int & nCount,
											 int nMaxPortals );

#endif // __E_REGION__
