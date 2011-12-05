//----------------------------------------------------------------------------
// convexhull.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "boundingbox.h"
#include "convexhull.h"


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define HULL_MAX_FACE_COUNT				32										// assert if more than this number of faces
const float HULL_EPSILON = 0.05f;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline VECTOR normalize(
	VECTOR & v)
{
	float lensq = VectorLengthSquared(v);
	if (lensq < HULL_EPSILON)
	{
		return v;
	}
	float len = sqrtf(lensq);
	//ASSERT(fabs(len) > HULL_EPSILON);
	v /= len;
	return v;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int ConvexHullGetVertexCount(
	const CONVEXHULL * hull)
{
	//ASSERT_RETNULL(hull);
	return hull->vertex_count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const VECTOR * ConvexHullGetVertex(
	const CONVEXHULL * hull,
	int index)
{
	//ASSERT_RETNULL(hull);
	//ASSERT_RETNULL(index >= 0 && index < hull->vertex_count);
	return hull->vertices + index;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int ConvexHullGetFaceCount(
	const CONVEXHULL * hull)
{
	//ASSERT_RETNULL(hull);
	return hull->face_count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const FACE * ConvexHullGetFace(
	const CONVEXHULL * hull,
	int index)
{
	//ASSERT_RETNULL(hull);
	//ASSERT_RETNULL(index >= 0 && index < hull->face_count);
	return hull->faces[index];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL AABBBooleanCollideTest(
	const HULL_AABB * A,
	const HULL_AABB * B)
{
	if (fabs(A->center.fX - B->center.fX) > (A->halfwidth.fX + B->halfwidth.fX - HULL_EPSILON))
	{
		return FALSE;
	}
	if (fabs(A->center.fY - B->center.fY) > (A->halfwidth.fY + B->halfwidth.fY - HULL_EPSILON))
	{
		return FALSE;
	}
	if (fabs(A->center.fZ - B->center.fZ) > (A->halfwidth.fZ + B->halfwidth.fZ - HULL_EPSILON))
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL AABBBooleanCollideTestZIgnored(
	const HULL_AABB * aabb,
	const VECTOR * vector)
{
	if (fabs(aabb->center.fX - vector->fX) > (aabb->halfwidth.fX + HULL_EPSILON))
	{
		return FALSE;
	}
	if (fabs(aabb->center.fY - vector->fY) > (aabb->halfwidth.fY + HULL_EPSILON))
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL AABBBooleanCollideTest(
	const HULL_AABB * aabb,
	const VECTOR * vector)
{
	if (fabs(aabb->center.fX - vector->fX) > (aabb->halfwidth.fX + HULL_EPSILON))
	{
		return FALSE;
	}
	if (fabs(aabb->center.fY - vector->fY) > (aabb->halfwidth.fY + HULL_EPSILON))
	{
		return FALSE;
	}
	if (fabs(aabb->center.fZ - vector->fZ) > (aabb->halfwidth.fZ + HULL_EPSILON))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const HULL_AABB * ConvexHullGetAABB(
	const CONVEXHULL * hull)
{
	//ASSERT_RETNULL(hull);
	return &hull->aabb;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const VECTOR * PlaneGetNormal(
	const HULL_PLANE * plane)
{
	return &plane->normal;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const VECTOR * FaceGetVertex(
	const FACE * face,
	int index)
{
	//ASSERT_RETNULL(face);
	//ASSERT_RETNULL(index >= 0 && index < face->vertex_count);
	return face->vertices[index];
}


//----------------------------------------------------------------------------
// get center point of a convex hull
//----------------------------------------------------------------------------
static inline const VECTOR * ConvexHullGetCenter(
	const CONVEXHULL * hull)
{
	return &hull->aabb.center;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ConvexHullAddPoint(
	CONVEXHULL * hull,
	const VECTOR & point)
{
	ASSERT_RETURN(hull);
	hull->vertices = (VECTOR *)REALLOC(NULL, hull->vertices, (hull->vertex_count + 1) * sizeof(VECTOR));
	hull->vertices[hull->vertex_count] = point;
	hull->vertex_count++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ConvexHullAddPointList(
	CONVEXHULL * hull,
	const VECTOR * pointlist,
	unsigned int count)
{
	ASSERT_RETURN(hull);
	hull->vertices = (VECTOR *)REALLOC(NULL, hull->vertices, (hull->vertex_count + count) * sizeof(VECTOR));
	memcpy(hull->vertices + hull->vertex_count * sizeof(VECTOR), pointlist, count * sizeof(VECTOR));
	hull->vertex_count += count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCompareNormal(
	const VECTOR & A,
	const VECTOR & B)
{
	float diff = VectorDistanceSquared(A, B);
	return (diff <= (HULL_EPSILON * HULL_EPSILON));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static FACE * sConvexHullGetCoplanarFace(
	CONVEXHULL * hull,
	const VECTOR & normal)
{
	for (unsigned int ii = 0; ii < hull->face_count; ++ii)
	{
		FACE * face = hull->faces[ii];
		if (sCompareNormal(face->normal, normal) == TRUE)
		{
			return face;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ConvexHullAddFaceList(
	CONVEXHULL * hull,
	unsigned int * index_list,
	unsigned int count)
{
	ASSERT_RETURN(hull);
	ASSERT_RETURN(index_list && count > 2);
	for (unsigned int ii = 0; ii < count; ii++)
	{
		ASSERT_RETURN(index_list[ii] >= 0 && index_list[ii] < hull->vertex_count);
	}

	VECTOR normal;
	VectorCross(normal, hull->vertices[index_list[1]] - hull->vertices[index_list[0]], hull->vertices[index_list[2]] - hull->vertices[index_list[0]]);
	normalize(normal);

	FACE * face = sConvexHullGetCoplanarFace(hull, normal);
	if (!face)
	{
		face = (FACE *)MALLOCZ(NULL, sizeof(FACE));
		face->normal = normal;
		face->d = VectorDot(face->normal, hull->vertices[index_list[0]]);

		hull->faces = (FACE **)REALLOC(NULL, hull->faces, (hull->face_count + 1) * sizeof(FACE *));
		// we had a report of a crash here, and I don't know how.  So, I added these checks.  -Tyler
		if (!hull->faces)
		{ 
			hull->face_count = 0;
			ASSERT_RETURN(hull->faces);
		}
		hull->faces[hull->face_count] = face;
		hull->face_count++;
	}

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		BOOL found = FALSE;
		for (unsigned int jj = 0; jj < face->vertex_count; ++jj)
		{
			if (hull->vertices[index_list[ii]] == *face->vertices[jj])
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			face->vertices = (VECTOR **)REALLOC(NULL, face->vertices, (face->vertex_count + 1) * sizeof(VECTOR *));
			face->vertices[face->vertex_count] = hull->vertices + index_list[ii];
			++face->vertex_count;
		}
	}

	ASSERT(hull->face_count < HULL_MAX_FACE_COUNT)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ConvexHullCalcAABB(
	CONVEXHULL * hull)
{
	ASSERT_RETURN(hull);
	int vertex_count = ConvexHullGetVertexCount(hull);

	float minx = INFINITY;
	float maxx = -INFINITY;
	float miny = INFINITY;
	float maxy = -INFINITY;
	float minz = INFINITY;
	float maxz = -INFINITY;

	for (int ii = 0; ii < vertex_count; ii++)
	{
		const VECTOR * vertex = ConvexHullGetVertex(hull, ii);
		if (vertex->fX < minx)
		{
			minx = vertex->fX;
		}
		if (vertex->fX > maxx)
		{
			maxx = vertex->fX;
		}
		if (vertex->fY < miny)
		{
			miny = vertex->fY;
		}
		if (vertex->fY > maxy)
		{
			maxy = vertex->fY;
		}
		if (vertex->fZ < minz)
		{
			minz = vertex->fZ;
		}
		if (vertex->fZ > maxz)
		{
			maxz = vertex->fZ;
		}
	}

	hull->aabb.halfwidth.fX = (maxx - minx) / 2.0f;
	hull->aabb.halfwidth.fY = (maxy - miny) / 2.0f;
	hull->aabb.halfwidth.fZ = (maxz - minz) / 2.0f;
	hull->aabb.center.fX = minx + hull->aabb.halfwidth.fX;
	hull->aabb.center.fY = miny + hull->aabb.halfwidth.fY;
	hull->aabb.center.fZ = minz + hull->aabb.halfwidth.fZ;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void FaceDestroy(
	FACE * face)
{
	ASSERT_RETURN(face);
	if (face->vertices)
	{
		FREE(NULL, face->vertices);
	}
	FREE(NULL, face);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConvexHullDestroy(
	CONVEXHULL * hull)
{
	ASSERT_RETURN(hull);

	if (hull->vertices)
	{
		FREE(NULL, hull->vertices);
	}
	if (hull->faces)
	{
		for (unsigned int ii = 0; ii < hull->face_count; ii++)
		{
			FaceDestroy(hull->faces[ii]);
		}
		FREE(NULL, hull->faces);
	}
	FREE(NULL, hull);
}


//----------------------------------------------------------------------------
// get signed distance of a point to a plane.  if the distance is positive,
// the point is on the same side as the normal, if it's negative, the
// point is on the other side of the normal
//----------------------------------------------------------------------------
static inline float PointGetSignedDistanceToPlane(
	const VECTOR * vertex,
	const FACE * face)
{
	return VectorDot(*PlaneGetNormal(face), (*vertex - *FaceGetVertex(face, 0)));
}


//----------------------------------------------------------------------------
// check if all the vertices of B are on the opposite side of any of the
// faces of A.  if there is a face of A where all the vertices of B are
// on the opposite side, then A and B don't intersect
// center points of convex hulls are provided for a quicker test
//----------------------------------------------------------------------------
static inline BOOL sConvexHullBooleanCollideTest(
	const CONVEXHULL * hullA,
	const VECTOR * centerA,
	const CONVEXHULL * hullB,
	const VECTOR * centerB)
{
	BOOL result = FALSE;

	int face_countA = ConvexHullGetFaceCount(hullA);
	for (int ii = 0; !result && ii < face_countA; ii++)
	{
		const FACE * face = ConvexHullGetFace(hullA, ii);
		float center_distB = PointGetSignedDistanceToPlane(centerB, face);
		if (center_distB < -HULL_EPSILON)
		{
			continue;
		}

		result = TRUE;
		int vertex_countB = ConvexHullGetVertexCount(hullB);
		for (int jj = 0; jj < vertex_countB; jj++)
		{
			const VECTOR * vertex = ConvexHullGetVertex(hullB, jj);
			float dist = PointGetSignedDistanceToPlane(vertex, face);
			if (dist < -HULL_EPSILON)
			{
				result = FALSE;
				break;
			}
		}
	}
	return result;
}


//----------------------------------------------------------------------------
// check for collision between two convex hulls, return TRUE if they collide
//----------------------------------------------------------------------------
BOOL ConvexHullBooleanCollideTest(
	const CONVEXHULL * hullA,
	const CONVEXHULL * hullB)
{
	ASSERT_RETFALSE(hullA && hullB);

	// do aligned bounding box test for quick rejection
	if (!AABBBooleanCollideTest(ConvexHullGetAABB(hullA), ConvexHullGetAABB(hullB)))
	{
		return FALSE;
	}

	const VECTOR * centerA = ConvexHullGetCenter(hullA);
	const VECTOR * centerB = ConvexHullGetCenter(hullB);

	if (sConvexHullBooleanCollideTest(hullA, centerA, hullB, centerB))
	{
		return FALSE;
	}
	if (sConvexHullBooleanCollideTest(hullB, centerB, hullA, centerA))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// check if point is in a hull
//----------------------------------------------------------------------------
BOOL ConvexHullPointTestZIgnored(
	const CONVEXHULL * hull,
	const struct VECTOR * point)
{
	VECTOR pointRedone(*point);
	pointRedone.z = 1.0f;
	if (!hull)
	{
		return FALSE;
	}
	if (!AABBBooleanCollideTestZIgnored(ConvexHullGetAABB(hull), &pointRedone))
	{
		return FALSE;
	}
	int face_count = ConvexHullGetFaceCount(hull);
	for (int ii = 0; ii < face_count; ii++)
	{
		const FACE * face = ConvexHullGetFace(hull, ii);
		float dist = PointGetSignedDistanceToPlane(&pointRedone, face);
		if (dist > HULL_EPSILON)
		{
			return FALSE;
		}
	}
	return TRUE;	
}


//----------------------------------------------------------------------------
// check if point is in a hull
//----------------------------------------------------------------------------
BOOL ConvexHullPointTest(
	const CONVEXHULL * hull,
	const VECTOR * point,
	float fEpsilon)				 // = HULL_EPSILON
{
	if (!hull)
	{
		return FALSE;
	}
	if (!AABBBooleanCollideTest(ConvexHullGetAABB(hull), point))
	{
		return FALSE;
	}
	int face_count = ConvexHullGetFaceCount(hull);
	for (int ii = 0; ii < face_count; ii++)
	{
		const FACE * face = ConvexHullGetFace(hull, ii);
		float dist = PointGetSignedDistanceToPlane(point, face);
		if (dist > fEpsilon)
		{
			return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConvexHullAABBCollide(
	const CONVEXHULL * hull,
	const BOUNDING_BOX * box,
	float fError )
{
	if ( ( hull->aabb.center.fX - hull->aabb.halfwidth.fX ) + fError < box->vMax.fX &&
		 ( hull->aabb.center.fY - hull->aabb.halfwidth.fY ) + fError < box->vMax.fY &&
		 ( hull->aabb.center.fZ - hull->aabb.halfwidth.fZ ) + fError < box->vMax.fZ &&
		 ( hull->aabb.center.fX + hull->aabb.halfwidth.fX ) - fError > box->vMin.fX &&
		 ( hull->aabb.center.fY + hull->aabb.halfwidth.fY ) - fError > box->vMin.fY &&
		 ( hull->aabb.center.fZ + hull->aabb.halfwidth.fZ ) - fError > box->vMin.fZ )
		return TRUE;
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConvexHullForcePointInside(
	const CONVEXHULL * hull,
	VECTOR & point)
{
	if (!hull)
	{
		return;
	}
	if (ConvexHullPointTest(hull, &point))
	{
		return;
	}
	float fMinDistance = INFINITY;
	const FACE * pMinFace = NULL;
	int face_count = ConvexHullGetFaceCount(hull);
	for (int ii = 0; ii < face_count; ii++)
	{
		const FACE * face = ConvexHullGetFace(hull, ii);
		float dist = PointGetSignedDistanceToPlane(&point, face);
		if (fabs(dist) < fMinDistance || pMinFace == NULL)
		{
			fMinDistance = fabs(dist);
			pMinFace = face;
		}
	}
	if (pMinFace)
	{
		float dist = PointGetSignedDistanceToPlane(&point, pMinFace);
		point += pMinFace->normal * dist;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONVEXHULL * HullCreate(
	VECTOR * vList,
	int nNumVertex,
	int * nTriList,
	int nNumTris,
	const char * pszFileName)
{
	if (!vList || nNumVertex == 0 || !nTriList || nNumTris == 0)
	{
		ASSERTX(vList, pszFileName);
		ASSERTX(nNumVertex, pszFileName);
		ASSERTX(nTriList, pszFileName);
		ASSERTX(nNumTris, pszFileName);
		return NULL;
	}

	CONVEXHULL * hull = (CONVEXHULL *)MALLOCZ(NULL, sizeof(CONVEXHULL));

	ConvexHullAddPointList(hull, vList, nNumVertex);

	unsigned int * list = (unsigned int *)nTriList;
	for (int ii = 0; ii < nNumTris; ii++)
	{
		ConvexHullAddFaceList(hull, list, 3);
		list += 3;
	}

	ConvexHullCalcAABB(hull);

	return hull;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float ConvexHullManhattenDistance(
	const CONVEXHULL * hull,
	const VECTOR * point) 
{
	float fDistance = 0.0f;

	if ((hull->aabb.center.fX - hull->aabb.halfwidth.fX) > point->fX)
	{
		fDistance += (hull->aabb.center.fX - hull->aabb.halfwidth.fX) - point->fX;
	}
	else if (point->fX > (hull->aabb.center.fX + hull->aabb.halfwidth.fX))
	{
		fDistance += point->fX - (hull->aabb.center.fX + hull->aabb.halfwidth.fX);
	}

	if ((hull->aabb.center.fY - hull->aabb.halfwidth.fY) > point->fY)
	{
		fDistance += (hull->aabb.center.fY - hull->aabb.halfwidth.fY) - point->fY;
	}
	else if (point->fY > (hull->aabb.center.fY + hull->aabb.halfwidth.fY))
	{
		fDistance += point->fY - (hull->aabb.center.fY + hull->aabb.halfwidth.fY);
	}

	if ((hull->aabb.center.fZ - hull->aabb.halfwidth.fZ) > point->fZ)
	{
		fDistance += (hull->aabb.center.fZ - hull->aabb.halfwidth.fZ) - point->fZ;
	}
	else if (point->fZ > (hull->aabb.center.fZ + hull->aabb.halfwidth.fZ))
	{
		fDistance += point->fZ - (hull->aabb.center.fZ + hull->aabb.halfwidth.fZ);
	}

	return fDistance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float ConvexHullManhattenDistance(
	const CONVEXHULL * hullA,
	const CONVEXHULL * hullB) 
{
	float fDistance = 0.0f;

	if ((hullA->aabb.center.fX - hullA->aabb.halfwidth.fX) > (hullB->aabb.center.fX + hullB->aabb.halfwidth.fX))
	{
		fDistance += (hullA->aabb.center.fX - hullA->aabb.halfwidth.fX) - (hullB->aabb.center.fX + hullB->aabb.halfwidth.fX);
	}
	else if ((hullB->aabb.center.fX - hullB->aabb.halfwidth.fX) > (hullA->aabb.center.fX + hullA->aabb.halfwidth.fX))
	{
		fDistance += (hullB->aabb.center.fX - hullB->aabb.halfwidth.fX) - (hullA->aabb.center.fX + hullA->aabb.halfwidth.fX);
	}

	if ((hullA->aabb.center.fY - hullA->aabb.halfwidth.fY) > (hullB->aabb.center.fY + hullB->aabb.halfwidth.fY))
	{
		fDistance += (hullA->aabb.center.fY - hullA->aabb.halfwidth.fY) - (hullB->aabb.center.fY + hullB->aabb.halfwidth.fY);
	}
	else if ((hullB->aabb.center.fY - hullB->aabb.halfwidth.fY) > (hullA->aabb.center.fY + hullA->aabb.halfwidth.fY))
	{
		fDistance += (hullB->aabb.center.fY - hullB->aabb.halfwidth.fY) - (hullA->aabb.center.fY + hullA->aabb.halfwidth.fY);
	}

	if ((hullA->aabb.center.fZ - hullA->aabb.halfwidth.fZ) > (hullB->aabb.center.fZ + hullB->aabb.halfwidth.fZ))
	{
		fDistance += (hullA->aabb.center.fZ - hullA->aabb.halfwidth.fZ) - (hullB->aabb.center.fZ + hullB->aabb.halfwidth.fZ);
	}
	else if ((hullB->aabb.center.fZ - hullB->aabb.halfwidth.fZ) > (hullA->aabb.center.fZ + hullA->aabb.halfwidth.fZ))
	{
		fDistance += (hullB->aabb.center.fZ - hullB->aabb.halfwidth.fZ) - (hullA->aabb.center.fZ + hullA->aabb.halfwidth.fZ);
	}

	return fDistance;
}
