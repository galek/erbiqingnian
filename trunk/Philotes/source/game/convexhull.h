//----------------------------------------------------------------------------
// convexhull.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CONVEXHULL_H_
#define _CONVEXHULL_H_


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct HULL_PLANE
{
	VECTOR								normal;
	float								d;
};


struct FACE : public HULL_PLANE
{
	VECTOR **							vertices;
	unsigned int						vertex_count;
};


struct HULL_AABB
{
	VECTOR								center;
	VECTOR								halfwidth;
};


struct CONVEXHULL
{
	VECTOR *							vertices;
	unsigned int						vertex_count;
	FACE **								faces;
	unsigned int						face_count;
	HULL_AABB							aabb;
};


//----------------------------------------------------------------------------
// Externs
//----------------------------------------------------------------------------

extern const float HULL_EPSILON;

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

CONVEXHULL * HullCreate(
	VECTOR * vList,
	int nNumVertex,
	int * nTriList,
	int nNumTris,
	const char * pszFilename );

void ConvexHullDestroy(
	CONVEXHULL * hull);

BOOL ConvexHullBooleanCollideTest(
	const CONVEXHULL * hullA,
	const CONVEXHULL * hullB);

BOOL ConvexHullAABBCollide(
	const CONVEXHULL * hull,
	const struct BOUNDING_BOX * box,
	float fError = 0.0f );

BOOL ConvexHullPointTest(
	const CONVEXHULL * hull,
	const struct VECTOR * point,
	float fEpsilon = HULL_EPSILON );

BOOL ConvexHullPointTestZIgnored(
	const CONVEXHULL * hull,
	const struct VECTOR * point );

float ConvexHullManhattenDistance(
	const CONVEXHULL * hull,
	const struct VECTOR * point);

float ConvexHullManhattenDistance(
	const CONVEXHULL * hullA,
	const CONVEXHULL * hullB);

void ConvexHullForcePointInside(
	const CONVEXHULL * hull,
	VECTOR & point);

#endif
