//----------------------------------------------------------------------------
// e_region.cpp
//
// - Implementation for region/culling interface code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_automap.h"
#include "e_region.h"
#include "e_dpvs_priv.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

DWORD gdwVisibilityToken = 0;

CHash<REGION>	gtRegions;
static int		gnRegionNextId = 0;

int				gnCurrentRegion = 0;

CellMap*			g_Cells					= NULL;
static int			gnNextCellID			= 0x40000000;
PhysicalPortalMap*	g_PhysicalPortals		= NULL;
static int			gnNextPhysicalPortalID	= 1;
VirtualPortalMap*	g_VirtualPortals		= NULL;
static int			gnNextVirtualPortalID	= 1;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void CULLING_PHYSICAL_PORTAL::Destroy()
{
#ifdef ENABLE_DPVS
	if ( pInternalPortal )
		pInternalPortal->release();
#endif
	pInternalPortal = NULL;
}

void CULLING_VIRTUAL_PORTAL::Destroy()
{
#ifdef ENABLE_DPVS
	if ( pInternalPortal )
		pInternalPortal->release();
#endif
	pInternalPortal = NULL;
}

void CULLING_CELL::Destroy()
{
#ifdef ENABLE_DPVS
	if ( pInternalCell )
		pInternalCell->release();
#endif
	pInternalCell = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_UpdateVisibilityToken()
{
	gdwVisibilityToken++;
	if ( gdwVisibilityToken == 0 )
	{
		// could reset all model visibility tokens to 0 here
		gdwVisibilityToken = 1;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRegionCleanup( REGION * pRegion )
{
	ASSERT_RETINVALIDARG( pRegion );
	V( e_CellRemove( pRegion->nCullCell ) );
	pRegion->nCullCell = INVALID_ID;
	V( e_AutomapDestroy( pRegion->nAutomapID ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RegionsInit()
{
#if !ISVERSION(SERVER_VERSION)
	HashInit( gtRegions, NULL, 2 );

	// always have at least 1 region around (global default region)
	//V_RETURN( e_RegionAdd( 0 ) );
	
	ASSERT_RETFAIL( NULL == g_Cells );
	g_Cells	= MEMORYPOOL_NEW(g_StaticAllocator) CellMap(CellMap::key_compare(), CellMap::allocator_type(g_StaticAllocator));
	ASSERT_RETVAL( g_Cells, E_OUTOFMEMORY );

	ASSERT_RETFAIL( NULL == g_PhysicalPortals );
	g_PhysicalPortals = MEMORYPOOL_NEW(g_StaticAllocator) PhysicalPortalMap(PhysicalPortalMap::key_compare(), PhysicalPortalMap::allocator_type(g_StaticAllocator));
	ASSERT_RETVAL( g_PhysicalPortals, E_OUTOFMEMORY );

	ASSERT_RETFAIL( NULL == g_VirtualPortals );
	g_VirtualPortals = MEMORYPOOL_NEW(g_StaticAllocator) VirtualPortalMap(VirtualPortalMap::key_compare(), VirtualPortalMap::allocator_type(g_StaticAllocator));
	ASSERT_RETVAL( g_VirtualPortals, E_OUTOFMEMORY );

	e_AutomapsInit();

#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sHashClearRegions()
{
#if !ISVERSION(SERVER_VERSION)
	for ( REGION * pRegion = HashGetFirst( gtRegions );
		pRegion;
		pRegion = HashGetNext( gtRegions, pRegion ) )
	{
		// any per-region cleanup goes here
		sRegionCleanup( pRegion );
	}

	if ( g_Cells )
	{
		for ( CellMap::iterator i = g_Cells->begin();
			i != g_Cells->end();
			++i)
		{
			CULLING_CELL * pCell = &(i->second);
			pCell->Destroy();
		}
		g_Cells->clear();		
	}

	if ( g_PhysicalPortals )
	{
		for ( PhysicalPortalMap::iterator i = g_PhysicalPortals->begin();
			i != g_PhysicalPortals->end();
			++i)
		{
			CULLING_PHYSICAL_PORTAL * pPortal = &(i->second);
			pPortal->Destroy();
		}
		g_PhysicalPortals->clear();		
	}

	if ( g_VirtualPortals )
	{
		for ( VirtualPortalMap::iterator i = g_VirtualPortals->begin();
			i != g_VirtualPortals->end();
			++i)
		{
			CULLING_VIRTUAL_PORTAL * pPortal = &(i->second);
			pPortal->Destroy();
		}
		g_VirtualPortals->clear();
	}

	HashClear( gtRegions );
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RegionsDestroy()
{
#if !ISVERSION(SERVER_VERSION)
	e_AutomapsDestroy();

	sHashClearRegions();

	HashFree( gtRegions );

	if ( g_Cells )
	{
		MEMORYPOOL_PRIVATE_DELETE( g_StaticAllocator, g_Cells, CellMap);
		g_Cells = NULL;
	}

	if ( g_PhysicalPortals )
	{
		MEMORYPOOL_PRIVATE_DELETE( g_StaticAllocator, g_PhysicalPortals, PhysicalPortalMap);
		g_PhysicalPortals = NULL;
	}

	if ( g_VirtualPortals )
	{
		MEMORYPOOL_PRIVATE_DELETE( g_StaticAllocator, g_VirtualPortals, VirtualPortalMap);
		g_VirtualPortals = NULL;
	}
#endif // !ISVERSION(SERVER_VERSION)

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RegionsReset()
{
#if !ISVERSION(SERVER_VERSION)
	sHashClearRegions();

	// always have at least 1 region around (global default region)
	V_RETURN( e_RegionAdd( 0 ) );

#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_SetRegion( int nRegion )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(nRegion >= 0, S_FALSE);

	if ( gnCurrentRegion != nRegion )
		e_AutomapsMarkDirty();

	gnCurrentRegion = nRegion;
#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

REGION * e_GetCurrentRegion()
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETNULL( gnCurrentRegion >= 0 );
	REGION * pRegion = e_RegionGet( gnCurrentRegion );
	//ASSERT_RETNULL( pRegion );
	if ( ! pRegion )
		trace( "Region %d is NULL!\n", gnCurrentRegion );
	return pRegion;
#else
	return NULL;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetCurrentRegionID()
{
	ASSERT_RETINVALID( gnCurrentRegion >= 0 );
	ASSERT_RETINVALID( e_RegionGet( gnCurrentRegion ) );
	return gnCurrentRegion;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RegionSetOffset(
	int nRegion,
	const VECTOR * pVector )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( pVector );
	REGION * pRegion = e_RegionGet( nRegion );
	ASSERT_RETURN( pRegion );
	pRegion->vOffset = *pVector;

	CULLING_CELL * pCell = e_CellGet( pRegion->nCullCell );

	if ( pCell && pCell->pInternalCell )
	{
		MATRIX mWorld;
		MatrixIdentity( &mWorld );
		//*(VECTOR*)&mWorld.m[3][0] = *pVector;
		V( e_DPVS_CellSetWorldMatrix( *pCell, mWorld ) );
	}
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RegionGetCell(
	int nRegion,
	int & nCell )
{
#if !ISVERSION(SERVER_VERSION)
	REGION * pRegion = e_RegionGet( nRegion );
	if ( ! pRegion )
	{
		nCell = INVALID_ID;
		return S_FALSE;
	}
	nCell = pRegion->nCullCell;
#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RegionGetOffset(
	int nRegion,
	VECTOR * pVector )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( pVector );
	REGION * pRegion = e_RegionGet( nRegion );
	ASSERT_RETURN( pRegion );
	*pVector = pRegion->vOffset;
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------s
//----------------------------------------------------------------------------

PRESULT e_RegionAdd( int nRegionID )
{
#if !ISVERSION(SERVER_VERSION)
	REGION * pRegion = HashGet( gtRegions, nRegionID );
	if ( pRegion )
		return S_FALSE;

	pRegion = HashAdd( gtRegions, nRegionID );
	ASSERT_RETFAIL( pRegion );

	const VECTOR vZero = VECTOR( 0, 0, 0 );

	pRegion->dwFlagBits				= 0;
	pRegion->vOffset				= vZero;
	pRegion->nEnvironmentDef		= INVALID_ID;
	pRegion->nPrevEnvironmentDef	= INVALID_ID;
	pRegion->bSavePrevEnvironmentDef = FALSE;
	pRegion->tBBox.vMin				= vZero;
	pRegion->tBBox.vMax				= vZero;
	//pRegion->pEnvironmentDef		= NULL;
	//pRegion->pPrevEnvironmentDef	= NULL;
	pRegion->nCullCell				= INVALID_ID;
	pRegion->nAutomapID				= INVALID_ID;

	V( e_CellCreate( pRegion->nCullCell, nRegionID ) );
	V_RETURN( e_AutomapCreate( nRegionID ) );
	pRegion->nAutomapID = nRegionID;
#endif // !ISVERSION(SERVER_VERSION)

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RegionRemove( int nRegionID )
{
#if !ISVERSION(SERVER_VERSION)
	REGION * pRegion = HashGet( gtRegions, nRegionID );
	if ( pRegion )
	{
		V( sRegionCleanup( pRegion ) );
	}

	HashRemove( gtRegions, nRegionID );
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sCellCreate( int nCellID, const MATRIX * pmWorld, int nRegion )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFAIL( g_Cells );
	ASSERT_RETINVALIDARG( nCellID != INVALID_ID );

	CULLING_CELL& tCell = (*g_Cells)[static_cast<DWORD>(nCellID)];
	tCell.Init();
	if ( pmWorld )
		tCell.mWorld = *pmWorld;
	else
		MatrixIdentity( &tCell.mWorld );
	tCell.nRegion = nRegion;
	V_RETURN( e_DPVS_CreateCullingCell( tCell ) );	

#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellCreate( int & nCellID, int nRegion, const MATRIX * pmWorld /*= NULL*/ )
{
#if !ISVERSION(SERVER_VERSION)
	nCellID = INVALID_ID;
	if ( ! g_Cells )
		return S_FALSE;

	if ( gnNextCellID == -1 )
		gnNextCellID = 0;

	V_RETURN( sCellCreate( gnNextCellID, pmWorld, nRegion ) );

	nCellID = gnNextCellID;
	gnNextCellID++;

#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellCreateWithID( int nCellID, int nRegion, const MATRIX * pmWorld /*= NULL*/ )
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! g_Cells )
		return S_FALSE;

	ASSERT_RETINVALIDARG( nCellID != INVALID_ID );
	DWORD dwKey = static_cast<DWORD>(nCellID);
	CellMap::iterator i = g_Cells->find( dwKey );
	ASSERTV_RETFAIL( i == g_Cells->end(), "e_CellCreate failed: Cell ID %d already exists in the map!", nCellID );

	V_RETURN( sCellCreate( nCellID, pmWorld, nRegion ) );

#endif // !ISVERSION(SERVER_VERSION)
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellRemove( int nCellID )
{
	ASSERT_RETINVALIDARG( nCellID != INVALID_ID );

	CellMap::iterator i = g_Cells->find( static_cast<DWORD>(nCellID) );
	if ( i == g_Cells->end() )
		return S_FALSE;

	(i->second).Destroy();
	g_Cells->erase( i );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellCreatePhysicalPortal(		__out int & nPortalID,
									    int nFromCellID,
										int nToCellID,
										const VECTOR * pvCorners,
										int nCorners,
										const VECTOR & vNormal,
										const MATRIX * pmWorldInv /*= NULL*/ )
{
	if ( ! g_PhysicalPortals )
		return S_FALSE;

	ASSERT( nPortalID == INVALID_ID );

	nPortalID = INVALID_ID;
	CULLING_PHYSICAL_PORTAL tPortal;
	tPortal.Init();

	CULLING_CELL * pFromCell = e_CellGet( nFromCellID );
	ASSERT_RETFAIL( pFromCell );
	CULLING_CELL * pToCell = e_CellGet( nToCellID );
	ASSERT_RETFAIL( pToCell );

	trace( "## Creating portal: %2d->%2d\n", nFromCellID, nToCellID );

	V_RETURN( e_DPVS_CreateCullingPhysicalPortal(
		tPortal,
		pvCorners,
		nCorners,
		vNormal,
		pFromCell,
		pToCell,
		pmWorldInv ) );

	tPortal.nSourceCellID = nFromCellID;
	tPortal.nTargetCellID = nToCellID;

	// add it
	nPortalID = gnNextPhysicalPortalID;
	gnNextPhysicalPortalID++;
	(*g_PhysicalPortals)[ static_cast<DWORD>(nPortalID) ] = tPortal;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellCreatePhysicalPortalPair(	__out int & nPortal1ID,
										__out int & nPortal2ID,
									    int nCellID1,
										int nCellID2,
										const VECTOR * pvCorners1,
										const VECTOR * pvCorners2,
										int nCorners,
										const VECTOR & vNormal1,
										const VECTOR & vNormal2,
										const MATRIX * pmWorldInv1 /*= NULL*/,
										const MATRIX * pmWorldInv2 /*= NULL*/ )
{
	if ( ! g_PhysicalPortals )
		return S_FALSE;

	ASSERT( nPortal1ID == INVALID_ID );

	int nPortalID[2] = { nPortal1ID, nPortal2ID };
	CULLING_PHYSICAL_PORTAL tPortal[2];

	for ( int i = 0; i < 2; ++i )
	{
		nPortalID[i] = INVALID_ID;
		tPortal[i].Init();
	}

	int * pnCellID[2] = { &nCellID1, &nCellID2 };
	CULLING_CELL * pCell[2];

	for ( int i = 0; i < 2; ++i )
	{
		pCell[i] = e_CellGet( *pnCellID[i] );
		ASSERT_RETFAIL( pCell[i] );
	}

	trace( "## Creating portal: %2d<->%2d\n", *pnCellID[0], *pnCellID[1] );

	V_RETURN( e_DPVS_CreateCullingPhysicalPortalPair(
		tPortal[0],
		tPortal[1],
		pvCorners1,
		pvCorners2,
		nCorners,
		vNormal1,
		vNormal2,
		pCell[0],
		pCell[1],
		pmWorldInv1,
		pmWorldInv2 ) );

	tPortal[0].nSourceCellID = *pnCellID[0];
	tPortal[0].nTargetCellID = *pnCellID[1];
	tPortal[1].nSourceCellID = *pnCellID[1];
	tPortal[1].nTargetCellID = *pnCellID[0];

	// add it
	nPortal1ID = gnNextPhysicalPortalID;
	gnNextPhysicalPortalID++;
	(*g_PhysicalPortals)[ static_cast<DWORD>(nPortal1ID) ] = tPortal[0];

	nPortal2ID = gnNextPhysicalPortalID;
	gnNextPhysicalPortalID++;
	(*g_PhysicalPortals)[ static_cast<DWORD>(nPortal2ID) ] = tPortal[1];

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellRemovePhysicalPortal( int nPortalID )
{
	ASSERT_RETINVALIDARG( nPortalID != INVALID_ID );

	PhysicalPortalMap::iterator i = g_PhysicalPortals->find( static_cast<DWORD>(nPortalID) );
	if ( i == g_PhysicalPortals->end() )
		return S_FALSE;

	(i->second).Destroy();
	g_PhysicalPortals->erase( i );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT		e_CellCreateVirtualPortal(	__out int & nPortalID,
									    int nFromCellID,
										const VECTOR * pvCorners,
										int nCorners,
										const VECTOR & vNormal )
{
	if ( ! g_VirtualPortals )
		return S_FALSE;

	ASSERT( nPortalID == INVALID_ID );

	nPortalID = INVALID_ID;
	CULLING_VIRTUAL_PORTAL tPortal;
	tPortal.Init();

	CULLING_CELL * pFromCell = e_CellGet( nFromCellID );
	ASSERT_RETFAIL( pFromCell );

	//trace( "## Creating virtual portal: %2d->%2d\n", nFromCellID );

	V_RETURN( e_DPVS_CreateCullingVirtualPortal(
		tPortal,
		pvCorners,
		nCorners,
		vNormal,
		pFromCell,
		NULL ) );

	V( e_DPVS_VirtualPortalSetWarpMatrix( tPortal ) );

	// add it
	nPortalID = gnNextVirtualPortalID;
	gnNextVirtualPortalID++;
	(*g_VirtualPortals)[ static_cast<DWORD>(nPortalID) ] = tPortal;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellRemoveVirtualPortal( int nPortalID )
{
	ASSERT_RETINVALIDARG( nPortalID != INVALID_ID );

	VirtualPortalMap::iterator i = g_VirtualPortals->find( static_cast<DWORD>(nPortalID) );
	if ( i == g_VirtualPortals->end() )
		return S_FALSE;

	(i->second).Destroy();
	g_VirtualPortals->erase( i );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CellFindSourcePhysicalPortals(
	int nCellID,
	int * pnPortalIDs,
	int & nCount,
	int nMaxPortals )
{
	ASSERT_RETINVALIDARG( pnPortalIDs );

	nCount = 0;
	for ( PhysicalPortalMap::iterator i = g_PhysicalPortals->begin();
		i != g_PhysicalPortals->end();
		++i )
	{
		CULLING_PHYSICAL_PORTAL * pPortal = &i->second;
		if ( pPortal->nSourceCellID != nCellID )
			continue;

		ASSERTX_BREAK( (nCount + 1) <= nMaxPortals, "Found too many physical portals in this cell!  Need to up the max count." );

		pnPortalIDs[nCount] = (int)i->first;
		nCount++;
	}

	return S_OK;
}