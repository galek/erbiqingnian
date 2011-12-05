//----------------------------------------------------------------------------
// dxC_render.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_RENDER__
#define __DXC_RENDER__

#include "safe_vector.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct D3D_DRAWLIST_COMMAND;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_RenderBoundingBox(
	BOUNDING_BOX * pBBox,
	MATRIX * pmatWorld,
	MATRIX * pmatView,
	MATRIX * pmatProj,
	BOOL bWriteColorAndDepth = TRUE,
	BOOL bTestDepth = TRUE,
	DWORD dwColor = 0x016060ff );
PRESULT dx9_RenderDebugBoxes();
PRESULT dx9_Render2DAABB(
	const BOUNDING_BOX * pBBox,
	BOOL bWriteColorAndDepth,
	BOOL bTestDepth,
	DWORD dwColor = 0x0160ff60 );
PRESULT dx9_RenderToInterfaceTexture(
	MATRIX * pmatView,
	MATRIX * pmatProj,
	VECTOR * pvEyeLocation,
	BOOL bClear = FALSE );
PRESULT dx9_RenderToShadowMaps();
PRESULT dxC_RenderBlobShadows( void * pUserData );
PRESULT dx9_RenderToParticleLightingMap(void);	// CHB 2007.02.17
PRESULT dx9_RenderModelBoundingBox (
	int nDrawList,
	int nModelID,
	BOOL bWriteColorAndDepth = TRUE,
	DWORD dwColor = 0x016060ff );
PRESULT dx9_RenderModelDepth (
	int nDrawList,
	int nModelID );
PRESULT dx9_RenderModelShadow (
	int nDrawList,
	int nData,
	int nModelID );
PRESULT dx9_RenderModel (
	int nDrawList,
	int nModelID );
PRESULT dx9_RenderModel (
	const DRAWLIST_STATE* pState,
	int nModelID );
PRESULT dx9_RenderModelBehind (
	int nDrawList,
	int nModelID );	
PRESULT dx9_RenderModelBehind (
	const DRAWLIST_STATE * pState,
	int nModelID );
PRESULT dx9_RenderLayer( 
	int nLayerID,
	LPD3DCSHADERRESOURCEVIEW pTexture1,
	LPD3DCSHADERRESOURCEVIEW pTexture2,
	DWORD dwColor );

namespace FSSE {
	struct MeshDraw;
	typedef safe_vector<struct MeshDrawSortable> MeshList;
	struct RENDER_CONTEXT;
	struct ModelDraw;
};
using namespace FSSE;
PRESULT dxC_RenderMesh(
	MeshDraw & tMeshDraw );
PRESULT dxC_RenderMeshDepth(
	MeshDraw & tMeshDraw );
PRESULT dxC_RenderMeshBehind(
	MeshDraw & tMeshDraw );

PRESULT dxC_ModelDrawPrepare(
	struct RENDER_CONTEXT & tContext,
	struct ModelDraw & tModelDraw );
PRESULT dxC_ModelDrawAddMesh(
	MeshList & tMeshList,
	struct RENDER_CONTEXT & tContext,
	struct ModelDraw & tModelDraw,
	int nMesh,
	MATERIAL_OVERRIDE_TYPE eMatOverrideType,
	DWORD dwPassFlags );
PRESULT dxC_ModelDrawAddMeshes(
	MeshList & tMeshList,
	struct RENDER_CONTEXT & tContext,
	struct ModelDraw & tModelDraw );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DXC_RENDER__