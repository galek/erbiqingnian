//----------------------------------------------------------------------------
// e_dpvs.h
//
// - Header for engine-API dPVS interface code
//
// NOTE: This file should use no dpvs types or functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_DPVS__
#define __E_DPVS__

#include "e_model.h"

// ---------------------------------
//
//      USE THIS TO MANUALLY DISABLE DPVS ON HELLGATE PROJECT CONFIGURATIONS
//
//#undef ENABLE_DPVS
//
// ---------------------------------

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define DPVS_TRACE_RELEASE

#ifdef ENABLE_DPVS
#	ifdef DPVS_TRACE_RELEASE
#		define DPVS_SAFE_RELEASE( ptr )	{ if ( ptr ) { DPVSTRACE( "DPVS_RELEASE %s: valid %d, new refcount %d, [%s:%d]", #ptr, (ptr)->debugIsValidPointer(ptr), (ptr)->getReferenceCount() - 1, __FILE__, __LINE__ ); (ptr)->release(); (ptr) = NULL; } else { DPVSTRACE( "DPVS_RELEASE %s: NULL NOP, [%s:%d]", #ptr, __FILE__, __LINE__ ); } }
#	else
#		define DPVS_SAFE_RELEASE( ptr )	{ if ( ptr ) { (ptr)->release(); (ptr) = NULL; } }
#	endif
#else
#	define DPVS_SAFE_RELEASE( ptr )	{ (ptr) = NULL; }
#endif

#ifndef ENABLE_DPVS
namespace DPVS
{
	class Cell;
	class Object;
	class Model;
	class Library;
	class Camera;
};
#else
namespace DPVS
{
	class Camera;
	class Cell;
	class PhysicalPortal;
	class VirtualPortal;
};
#endif // ENABLE_DPVS

#define MAX_PORTAL_DEPTH		128

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

#ifdef ENABLE_DPVS
typedef class DPVS::Camera * LP_INTERNAL_CAMERA;
typedef class DPVS::Cell * LP_INTERNAL_CELL;
typedef class DPVS::PhysicalPortal * LP_INTERNAL_PHYSICAL_PORTAL;
typedef class DPVS::VirtualPortal * LP_INTERNAL_VIRTUAL_PORTAL;
#else
typedef void * LP_INTERNAL_CAMERA;
typedef void * LP_INTERNAL_CELL;
typedef void * LP_INTERNAL_PHYSICAL_PORTAL;
typedef void * LP_INTERNAL_VIRTUAL_PORTAL;
#endif

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
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_BackgroundModelFileOccludesVisibility( const char * pszFilePath )
{
	extern PFN_QUERY_BY_FILE gpfn_BGFileOccludesVisibility;
	if ( gpfn_BGFileOccludesVisibility )
		return gpfn_BGFileOccludesVisibility( pszFilePath );
	return FALSE;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_DPVS_Init();
PRESULT e_DPVS_Destroy();

PRESULT e_DPVS_CameraCreate(
	DPVS::Camera *& pDPVSCamera,
	BOOL bOcclusionCulling,
	int nRenderWidth,
	int nRenderHeight );
PRESULT e_DPVS_CameraUpdate(
	DPVS::Camera * pDPVSCamera,
	int nCell,
	const struct CAMERA_INFO * pCameraInfo,
	float fNear,
	float fFar );
PRESULT e_DPVS_CreateCullingCell	( struct CULLING_CELL & tCell );
PRESULT e_DPVS_CellSetWorldMatrix	( struct CULLING_CELL & tCell, const struct MATRIX & mWorld );
PRESULT e_DPVS_CellSetRegion		( struct CULLING_CELL & tCell, int nRegion );
PRESULT e_DPVS_CreateCullingPhysicalPortal(
	struct CULLING_PHYSICAL_PORTAL & tPortal,
	const VECTOR * pvBorders,
	int nBorderCount,
	const VECTOR & vNormal,
	CULLING_CELL* pFromCell,
	CULLING_CELL* pToCell,
	const MATRIX * pmWorldInv = NULL );
PRESULT e_DPVS_CreateCullingPhysicalPortalPair(
	struct CULLING_PHYSICAL_PORTAL & tPortal1,
	struct CULLING_PHYSICAL_PORTAL & tPortal2,
	const VECTOR * pvBorders1,
	const VECTOR * pvBorders2,
	int nBorderCount,
	const VECTOR & vNormal1,
	const VECTOR & vNormal2,
	CULLING_CELL* pFromCell,
	CULLING_CELL* pToCell,
	const MATRIX * pmWorldInv1 = NULL,
	const MATRIX * pmWorldInv2 = NULL );
PRESULT e_DPVS_CreateCullingVirtualPortal(
	struct CULLING_VIRTUAL_PORTAL & tPortal,
	const VECTOR * pvBorders,
	int nBorderCount,
	const VECTOR & vNormal,
	CULLING_CELL* pFromCell,
	const MATRIX * pmWorldInv = NULL );
PRESULT e_DPVS_VirtualPortalSetWarpMatrix(
	struct CULLING_VIRTUAL_PORTAL & tFromPortal );
PRESULT e_DPVS_VirtualPortalSetTarget(
	struct CULLING_VIRTUAL_PORTAL & tFromPortal,
	struct CULLING_VIRTUAL_PORTAL & tToPortal );
PRESULT e_DPVS_PhysicalPortalGetOBB	( struct CULLING_PHYSICAL_PORTAL * pPortal, MATRIX & mOBB );
PRESULT e_DPVS_VirtualPortalGetOBB	( struct CULLING_VIRTUAL_PORTAL * pPortal, MATRIX & mOBB );

	
void e_DPVS_ClearStats();
void e_DPVS_QueryVisibility			( DPVS::Camera * pDPVSCamera );
PRESULT e_DPVS_ModelSetEnabled		( int nModelID, BOOL bEnabled );
BOOL e_DPVS_ModelGetVisible			( int nModelID );

void e_DPVS_MakeRenderList();

void  e_DPVS_GetStats				( WCHAR * pwszStr, int nBufLen );
DWORD e_DPVS_DebugSetDrawFlags		( DWORD dwFlags );
PRESULT e_DPVS_DebugDumpData();

#endif // __E_DPVS__
