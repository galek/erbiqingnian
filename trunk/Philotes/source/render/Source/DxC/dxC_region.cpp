//----------------------------------------------------------------------------
// dxC_region.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_debug.h"
#include "e_primitive.h"

#include "dxC_region.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

extern CellMap*				g_Cells;
extern PhysicalPortalMap*	g_PhysicalPortals;
extern VirtualPortalMap*	g_VirtualPortals;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

PRESULT dxC_CellDebugRenderAll()
{
#if ISVERSION(DEVELOPMENT)
	if ( ! g_Cells )
		return S_FALSE;

	for ( CellMap::iterator i = g_Cells->begin();
		i != g_Cells->end();
		++i )
	{
		CULLING_CELL * pCell = &(i->second);
		if ( ! pCell->pInternalCell )
			continue;
		//MATRIX mCell;
		//pCell->pInternalCell->getWorldToCellMatrix( (Matrix4x4&)mCell );
		V( e_DebugRenderAxes( pCell->mWorld, 0.5f, NULL ) );
	}
#endif
	return S_OK;
}

PRESULT dxC_PortalDebugRenderAll()
{
#if ISVERSION(DEVELOPMENT)
	if ( ! g_PhysicalPortals || ! g_VirtualPortals )
		return S_FALSE;

	for ( PhysicalPortalMap::iterator i = g_PhysicalPortals->begin();
		i != g_PhysicalPortals->end();
		++i )
	{
		CULLING_PHYSICAL_PORTAL * pPortal = &(i->second);
		if ( ! pPortal->pInternalPortal )
			continue;
#ifdef ENABLE_DPVS
		MATRIX mOBB;
		V( e_DPVS_PhysicalPortalGetOBB( pPortal, mOBB ) );
		V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, &mOBB, 0xff3fff3f ) );
#endif
	}
#endif
	return S_OK;
}