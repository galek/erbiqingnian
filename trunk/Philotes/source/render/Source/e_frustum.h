//----------------------------------------------------------------------------
// e_frustum.h
//
// - Header for frustum structures and functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_FRUSTUM__
#define __E_FRUSTUM__

#include "frustum.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum FRUSTUM_PLANES
{
	FRUSTUM_PLANE_NEAR = 0,
	FRUSTUM_PLANE_FAR,
	FRUSTUM_PLANE_LEFT,
	FRUSTUM_PLANE_RIGHT,
	FRUSTUM_PLANE_TOP,
	FRUSTUM_PLANE_BOTTOM,
	NUM_FRUSTUM_PLANES
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

class VIEW_FRUSTUM
{
	MATRIX	matView;
	MATRIX	matProj;

	BOOL	bCurrent;

	PLANE	tPlanes[ NUM_FRUSTUM_PLANES ];
public:
	BOOL IsCurrent()
	{
		return bCurrent;
	}
	void Dirty()
	{
		bCurrent = FALSE;
	}
	void Update( const MATRIX * pmatView, const MATRIX * pmatProj );
	const PLANE * GetPlanes( BOOL bAllowDirty = FALSE );
};


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline VIEW_FRUSTUM * e_GetCurrentViewFrustum()
{
	extern VIEW_FRUSTUM gtViewFrustum;
	return &gtViewFrustum;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_ExtractFrustumPoints(
	VECTOR * pvPoints,
	const BOUNDING_BOX * pScreenBox,
	const MATRIX * pmatTrans );

PRESULT e_ExtractFrustumPoints(
	VECTOR * pvPoints, 
	const MATRIX * pmatView, 
	const MATRIX * pmatProj );

PRESULT e_ExtractFrustumPlanes(
	PLANE * pPlanes,
	const BOUNDING_BOX * pScreenBox,
	const MATRIX * pmatView,
	const MATRIX * pmatProj );

PRESULT e_ExtractFrustumPlanes(
	PLANE * pPlanes, 
	const MATRIX * pmatView, 
	const MATRIX * pmatProj );

void e_MakeFrustumPlanes(
	PLANE * pPlanes, 
	const MATRIX * pmatView, 
	const MATRIX * pmatProj, 
	int nModelID = INVALID_ID );

void e_ExtractAABBBounds( const MATRIX * pmatView, 
						  const MATRIX * pmatProj, 
						  BOUNDING_BOX * BBox );

BOOL e_OBBInFrustum(
	ORIENTED_BOUNDING_BOX pPoints,
	const PLANE * pPlanes );

int	e_IntersectsAABB( BOUNDING_BOX * FrustumBox, BOUNDING_BOX * BBox );	// max AABB bounds of the object to intersect


BOOL e_PointInFrustum(
	VECTOR *pPoint,
	const PLANE * pPlanes,
	float fPad = 0.f );
BOOL e_PointInFrustumCheap(
	VECTOR *pPoint,
	const PLANE * pPlanes,
	float fPad = 0.f );
int e_AABBInFrustum(
	BOUNDING_BOX* BBox,
	const PLANE * pPlanes,
	float fPadZ = 0.f );
int e_AABBXYInFrustum(
	VECTOR *vBBox,
	const PLANE * pPlanes,
	float fPadZ = 0.f );

BOOL e_PrimitiveInFrustum(
	struct PRIMITIVE_INFO * ptInfo,
	PLANE * pPlanes );

#endif // __E_FRUSTUM__