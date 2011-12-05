//----------------------------------------------------------------------------
// collision.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "collision.h"
#include "units.h" // also includes game.h
#include "skills.h"
#include "complexmaths.h"
#include "unit_priv.h"
#include "level.h"
#include "quadtree.h"
#include "e_model.h"
#include "quests.h"
#include "pets.h"
#include "teams.h"
#include "prime.h"
#include "objects.h"
#include "unitmodes.h"
#include "vector.h"
#include "facelist.h"

#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif

//#define DRB_NEW_COLLIDE_ALL		1

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define COLLIDE_MIN_DIST				0.0f
#define MIN_STEP_ADD					0.001f
#define MAX_CLOSE_UNITS					100

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_NEW_COLLIDE_ALL

enum COLLIDE_ALL_RESULT_TYPE
{
	COLLIDE_ALL_RESULT_NONE = 0,
	COLLIDE_ALL_RESULT_GEOMETRY,
	COLLIDE_ALL_RESULT_UNIT,
};

struct COLLIDE_ALL_DATA
{
	// source
	UNIT *	unit;
	float	radius;
	float	height;
	VECTOR	start;
	VECTOR	end;
	VECTOR	velocity;
	float	distance;
	float	original_distance;
	int		num_rooms;
	ROOM *	room_list[MAX_ROOMS_CONNECTED];
	int		num_units;
	UNIT *	unit_list[MAX_CLOSE_UNITS];
	// results
	COLLIDE_ALL_RESULT_TYPE result_type;
	float	result_distance;
	VECTOR	result_intersection;
};

#endif

// Class CFaceBoundsRetriever
//
class CFaceBoundsRetriever
{
	
	public:
	
		const BOUNDING_BOX& operator()(unsigned int faceIndex) const
		{		
			return *m_FaceList->GetBounds(faceIndex);
		}
		
	public:
		
		CFaceList* m_FaceList;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct LEVEL_COLLISION
{
	CFaceList*							pFaceList;
	CQuadtree<unsigned int, CFaceBoundsRetriever>* pQuadTree;
	SIMPLE_DYNAMIC_ARRAY<ROOMID>		tRoomList;
	SIMPLE_DYNAMIC_ARRAY<ROOMID>		tRoomPropList;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AABBComputeFromPointList(
	AABB* aabb,
	const VECTOR* pointlist,
	int count)
{
	VECTOR vMin(INFINITY, INFINITY, INFINITY);
	VECTOR vMax(-INFINITY, -INFINITY, -INFINITY);
	for (int ii = 0; ii < count; ii++)
	{
		const VECTOR* vec = pointlist;
		if (vec->fX < vMin.fX)
		{
			vMin.fX = vec->fX;
		}
		if (vec->fX > vMax.fX)
		{
			vMax.fX = vec->fX;
		}
		if (vec->fY < vMin.fY)
		{
			vMin.fY = vec->fY;
		}
		if (vec->fY > vMax.fY)
		{
			vMax.fY = vec->fY;
		}
		if (vec->fZ < vMin.fZ)
		{
			vMin.fZ = vec->fZ;
		}
		if (vec->fZ > vMax.fZ)
		{
			vMax.fZ = vec->fZ;
		}
		pointlist++;
	}
	aabb->fRadius[X] = (vMax.fX - vMin.fX) / 2.0f;
	aabb->fRadius[Y] = (vMax.fY - vMin.fY) / 2.0f;
	aabb->fRadius[Z] = (vMax.fZ - vMin.fZ) / 2.0f;

	aabb->vCenter.fX = vMin.fX + aabb->fRadius[X];
	aabb->vCenter.fY = vMin.fY + aabb->fRadius[Y];
	aabb->vCenter.fZ = vMin.fZ + aabb->fRadius[Z];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AABBComputeFromPointList(
	AABB* aabb,
	const VECTOR** pointlist,
	int count)
{
	VECTOR vMin(INFINITY, INFINITY, INFINITY);
	VECTOR vMax(-INFINITY, -INFINITY, -INFINITY);
	for (int ii = 0; ii < count; ii++)
	{
		const VECTOR* vec = *pointlist;
		if (vec->fX < vMin.fX)
		{
			vMin.fX = vec->fX;
		}
		if (vec->fX > vMax.fX)
		{
			vMax.fX = vec->fX;
		}
		if (vec->fY < vMin.fY)
		{
			vMin.fY = vec->fY;
		}
		if (vec->fY > vMax.fY)
		{
			vMax.fY = vec->fY;
		}
		if (vec->fZ < vMin.fZ)
		{
			vMin.fZ = vec->fZ;
		}
		if (vec->fZ > vMax.fZ)
		{
			vMax.fZ = vec->fZ;
		}
		pointlist++;
	}
	aabb->fRadius[X] = (vMax.fX - vMin.fX) / 2.0f;
	aabb->fRadius[Y] = (vMax.fY - vMin.fY) / 2.0f;
	aabb->fRadius[Z] = (vMax.fZ - vMin.fZ) / 2.0f;

	aabb->vCenter.fX = vMin.fX + aabb->fRadius[X];
	aabb->vCenter.fY = vMin.fY + aabb->fRadius[Y];
	aabb->vCenter.fZ = vMin.fZ + aabb->fRadius[Z];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelCollisionInit(
	GAME* game,
	LEVEL * level)
{
	ASSERT_RETURN(game && level);
	LEVEL_COLLISION * lvlcol = (LEVEL_COLLISION *)GMALLOCZ(game, sizeof(LEVEL_COLLISION));
	level->m_Collision = lvlcol;
	ASSERT_RETURN(lvlcol);

	if (AppIsTugboat())
	{
		ArrayInit(lvlcol->tRoomList, GameGetMemPool(game), 4);
		ArrayInit(lvlcol->tRoomPropList, GameGetMemPool(game), 4);
		
		lvlcol->pFaceList = MEMORYPOOL_NEW(GameGetMemPool(game)) CFaceList(GameGetMemPool(game));
			
		lvlcol->pQuadTree = (CQuadtree<unsigned int,CFaceBoundsRetriever >*)GMALLOC( game, sizeof(CQuadtree<unsigned int,CFaceBoundsRetriever >));
		CFaceBoundsRetriever boundsRetriever;
		boundsRetriever.m_FaceList = lvlcol->pFaceList;
		lvlcol->pQuadTree->Init( level->m_BoundingBox, 8.0f, boundsRetriever, GameGetMemPool( game ) );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelCollisionFree(
	GAME* game,
	LEVEL * level)
{
	ASSERT_RETURN(game && level);
	LEVEL_COLLISION* lvlcol = level->m_Collision;
	if (!lvlcol)
	{
		return;
	}
	if( lvlcol->pQuadTree )
	{
		lvlcol->pQuadTree->Free();
		GFREE( game, lvlcol->pQuadTree );
		lvlcol->pQuadTree = NULL;
	}

	if( lvlcol->pFaceList )
	{
		MEMORYPOOL_DELETE(GameGetMemPool(game), lvlcol->pFaceList);
		lvlcol->pFaceList = NULL;
	}

	lvlcol->tRoomList.Destroy();
	lvlcol->tRoomPropList.Destroy();
	GFREE(game, lvlcol);
	level->m_Collision = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sVertexCompare(
	VECTOR* a,
	VECTOR* b)
{
	float dx = a->fX - b->fX;
	if (dx < -EPSILON)
	{
		return -1;
	}
	else if (dx > EPSILON)
	{
		return 1;
	}
	
	float dy = a->fY - b->fY;
	if (dy < -EPSILON)
	{
		return -1;
	}
	else if (dy > EPSILON)
	{
		return 1;
	}

	float dz = a->fZ - b->fZ;
	if (dz < -EPSILON)
	{
		return -1;
	}
	else if (dz > EPSILON)
	{
		return 1;
	}

	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelCollisionAddVertex(
	GAME * game,
	LEVEL_COLLISION * lvlcol,
	VECTOR * vertex)
{
	if (!AppIsTugboat())
	{
		return;
	}
	ASSERT_RETURN(lvlcol);
	ASSERT_RETURN(lvlcol->pFaceList);
	lvlcol->pFaceList->AddVertex( *vertex );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelCollisionAddFace(
	GAME * game,
	LEVEL_COLLISION * lvlcol,
	ROOM * room,
	const ROOM_CPOLY * cpoly,
	VECTOR ** vmap,
	int VertexOffset,
	int nMaterialIndex = 1 )
{
	if (!AppIsTugboat())
	{
		return;
	}
	ASSERT_RETURN(lvlcol);

	ASSERT_RETURN(lvlcol->pFaceList);
	unsigned int faceIndex = lvlcol->pFaceList->AddFace( cpoly->pPt3->nIndex + VertexOffset,cpoly->pPt2->nIndex + VertexOffset, cpoly->pPt1->nIndex + VertexOffset, nMaterialIndex);
	
	// Add the face's bounding box to the level's quadtree
	//
	const BOUNDING_BOX& bBox = *lvlcol->pFaceList->GetBounds(faceIndex);
	lvlcol->pQuadTree->Add(faceIndex, bBox);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelCollisionAddRoom(
	GAME * game, 
	LEVEL * level,
	ROOM * room)
{
	if (!AppIsTugboat())
	{
		return;
	}
	const ROOM_INDEX * pRoomIndex = RoomGetRoomIndex( game, room );
	if (pRoomIndex == NULL)
	{
		return;
	}
	if (pRoomIndex->bNoCollision)
	{
		return;
	}

	ASSERT_RETURN(game && level && room);
	LEVEL_COLLISION * lvlcol = level->m_Collision;
	ASSERT_RETURN(lvlcol);
	ROOM_DEFINITION * roomdef = room->pDefinition;
	ASSERT_RETURN(roomdef);

	VECTOR ** vmap = NULL;
	int OriginalVertices = 0;

	int rooms = lvlcol->tRoomList.Count();

	// don't add rooms already added - we don't take 'em out.
	for( int i = 0; i < rooms; i++ )
	{
		if( lvlcol->tRoomList[i] == room->idRoom )
		{
			return;
		}
	}
	ArrayAddItem(lvlcol->tRoomList, room->idRoom);
	// add points
	int point_count = roomdef->nRoomPointCount;
	LOCAL_OR_ALLOC_BUFFER<VECTOR, 8192, FALSE> vbuf(GameGetMemPool(game), point_count);

	for (int ii = 0; ii < point_count; ii++)
	{
		vbuf[ii] = roomdef->pRoomPoints[ii].vPosition;
	}
	// do transforms
	const MATRIX* matrix = &room->tWorldMatrix;
	for (int ii = 0; ii < point_count; ii++)
	{
		MatrixMultiplyCoord(&vbuf[ii], &vbuf[ii], matrix);
	}
	OriginalVertices = lvlcol->pFaceList->GetVertexCount();
	for (int ii = 0; ii < point_count; ii++)
	{
		sLevelCollisionAddVertex(game, lvlcol, &vbuf[ii]);
	}
	// add faces
	int poly_count = roomdef->nRoomCPolyCount;
	for (int ii = 0; ii < poly_count; ii++)
	{
		
		const ROOM_COLLISION_MATERIAL_INDEX * pMaterialIndex = GetMaterialFromIndex(roomdef, ii);
		
		sLevelCollisionAddFace(game, lvlcol, room, roomdef->pRoomCPolys + ii, vmap, OriginalVertices,  pMaterialIndex->nMaterialIndex + 1);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelCollisionSetRoomPopulated( 
	LEVEL* level,
	ROOM * room)
{
	if (AppIsTugboat())
	{
		if( !LevelCollisionRoomPopulated( level, room ) )
		{
			LEVEL_COLLISION* lvlcol = level->m_Collision;
			ASSERT_RETURN(lvlcol);
			ArrayAddItem(lvlcol->tRoomPropList, room->idRoom);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool LevelCollisionRoomPopulated( LEVEL* level,
								 ROOM* room )
{
	if (AppIsTugboat())
	{
		LEVEL_COLLISION* lvlcol = level->m_Collision;
		ASSERT_RETTRUE(lvlcol);


		int rooms = lvlcol->tRoomPropList.Count();

		// don't add rooms already added - we don't take 'em out.
		for( int i = 0; i < rooms; i++ )
		{
			if( lvlcol->tRoomPropList[i] == room->idRoom )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelCollisionAddProp(
	GAME * game, 
	LEVEL * level,
	ROOM_DEFINITION * roomdef,
	ROOM_LAYOUT_GROUP* pLayoutGroup,
	MATRIX * matrix,
	BOOL bUseMatID /* = FALSE */)
{
	if (!AppIsTugboat())
	{
		return;
	}

	ASSERT_RETURN(game && level && roomdef);
	LEVEL_COLLISION* lvlcol = level->m_Collision;
	ASSERT_RETURN(lvlcol);

	VECTOR** vmap = NULL;
	int OriginalVertices = 0;

	// add points
	int point_count = roomdef->nRoomPointCount;
	LOCAL_OR_ALLOC_BUFFER<VECTOR, 8192, FALSE> vbuf(GameGetMemPool(game), point_count);
	for (int ii = 0; ii < point_count; ii++)
	{
		vbuf[ii] = roomdef->pRoomPoints[ii].vPosition;
	}
	// do transforms
	for (int ii = 0; ii < point_count; ii++)
	{
		MatrixMultiplyCoord(&vbuf[ii], &vbuf[ii], matrix);
	}
	OriginalVertices = lvlcol->pFaceList->GetVertexCount();
	for (int ii = 0; ii < point_count; ii++)
	{
		sLevelCollisionAddVertex(game, lvlcol, &vbuf[ii]);
	}


	// add faces
	int poly_count = roomdef->nRoomCPolyCount;

	for (int ii = 0; ii < poly_count; ii++)
	{
		const ROOM_COLLISION_MATERIAL_INDEX * pMaterialIndex = GetMaterialFromIndex(roomdef, ii);

		sLevelCollisionAddFace(game, lvlcol, NULL, roomdef->pRoomCPolys + ii, vmap, OriginalVertices, bUseMatID ? pMaterialIndex->nMaterialIndex + 1 : PROP_MATERIAL);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sAngleOk( float fX1, float fY1, float fX2, float fY2 )
{
	float fVal = ( fX1 * fX2 ) + ( fY1 * fY2 );
	if ( fVal <= 0 )
		return TRUE;
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define Z_RANGE_ERROR		0.01f

static BOOL sInZRange( float z, float z1, float z2 )
{
	if ( ( z >= ( z1 - Z_RANGE_ERROR ) ) && ( z <= ( z2 + Z_RANGE_ERROR ) ) )
	{
		return TRUE;
	}
	if ( ( z >= ( z2 - Z_RANGE_ERROR ) ) && ( z <= ( z2 + Z_RANGE_ERROR ) ) )
	{
		return TRUE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL DistancePointLineXY( VECTOR * pOrig, VECTOR * pStart, VECTOR * pEnd, float * pfDist, DWORD dwFlags )
{
	if ( ( dwFlags & DISTANCE_POINT_FLAGS_WITH_Z_RANGE ) && !sInZRange( pOrig->fZ, pStart->fZ, pEnd->fZ ) )
	{
		return FALSE;
	}

	VECTOR edge;
	VectorSubtract( edge, *pEnd, *pStart );
	float fLineMagSq = ( edge.fX * edge.fX ) + ( edge.fY * edge.fY );
	if ( fLineMagSq == 0.0f )
	{
		*pfDist = 0.0f;
		return FALSE;
	}
	
	float fU =  ( ( (pOrig->fX - pStart->fX) * (pEnd->fX - pStart->fX) ) +
		          ( (pOrig->fY - pStart->fY) * (pEnd->fY - pStart->fY) ) ) / fLineMagSq;

	// Check if point falls within the line segment
	if ( fU < 0.0f || fU > 1.0f )
		return FALSE;

	VECTOR Intersection;
	Intersection.fX = pStart->fX + ( fU * edge.fX );
	Intersection.fY = pStart->fY + ( fU * edge.fY );
	Intersection.fZ = pOrig->fZ;

	VectorSubtract( Intersection, *pOrig, Intersection );
	*pfDist = VectorLength( Intersection );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL DistancePointLine3D( VECTOR * pOrig, VECTOR * pStart, VECTOR * pEnd, float * pfDist, VECTOR * pIntersection, DWORD dwFlags )
{
	ASSERT_RETFALSE(pOrig && pStart && pEnd && pfDist);
	if ( ( dwFlags & DISTANCE_POINT_FLAGS_WITH_Z_RANGE ) && !sInZRange( pOrig->fZ, pStart->fZ, pEnd->fZ ) )
	{
		return FALSE;
	}

	VECTOR edge;
	VectorSubtract( edge, *pEnd, *pStart );
	float fLineMagSq = VectorLengthSquared( edge );
	if ( fLineMagSq == 0.0f )
	{
		return FALSE;
	}
	//ASSERTX_RETFALSE( fLineMagSq != 0.0f, "Divide by 0" );

	float fU =  ( ( ( pOrig->fX - pStart->fX ) * ( pEnd->fX - pStart->fX ) ) +
		( ( pOrig->fY - pStart->fY ) * ( pEnd->fY - pStart->fY ) ) +
		( ( pOrig->fZ - pStart->fZ ) * ( pEnd->fZ - pStart->fZ ) ) ) /
		(fLineMagSq);

	// Check if point falls within the line segment
	if ( fU < 0.0f || fU > 1.0f )
		return FALSE;

	VECTOR Intersection;
	Intersection.fX = pStart->fX + ( fU * edge.fX );
	Intersection.fY = pStart->fY + ( fU * edge.fY );
	Intersection.fZ = pStart->fZ + ( fU * edge.fZ );

	VectorSubtract( Intersection, *pOrig, Intersection );
	*pfDist = VectorLength( Intersection );
	if ( pIntersection )
		*pIntersection = Intersection;

	return TRUE;
}

//----------------------------------------------------------------------------
// moller trombore
// todo: dir is (0, 0, 0)  or  unit vector (so far (0, 0, 1) or (0, 0, -1)
//----------------------------------------------------------------------------
static BOOL CRayIntersectTriangle( 
	const VECTOR & origin,
	const VECTOR & dir,
	const VECTOR & vVert0,
	const VECTOR & vVert1,
	const VECTOR & vVert2,
	float * pfDist,
	float * pfUi,
	float * pfVi)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	float lensq = VectorLengthSquared(dir);
	if (lensq < -EPSILON || lensq > EPSILON)
	{
		ASSERT(lensq >= 1.0f - 0.01f && lensq <= 1.0f + 0.01f);
	}
#endif

	VECTOR edge1, edge2, tvec, pvec, qvec;

	// find vectors for two edges sharing vert0
	VectorSubtract(edge1, vVert1, vVert0);
	VectorSubtract(edge2, vVert2, vVert0);

	// begin calculating determinant - also used to calculate U parameter
	VectorCross(pvec, dir, edge2);

	// if determinant is near zero, ray lies in plane of triangle
	float fDet = VectorDot(edge1, pvec);

	if (fDet < EPSILON)
	{
		return FALSE;
	}

	// calculate distance from vert0 to ray origin
	VectorSubtract(tvec, origin, vVert0);

	// calculate U parameter and test bounds
	*pfUi = VectorDot(tvec, pvec);
	if (*pfUi < 0.0f || *pfUi > fDet)
	{
		return FALSE;
	}

	// prepare to test V parameter
	VectorCross(qvec, tvec, edge1);

	// calculate V parameter and test bounds
	*pfVi = VectorDot(dir, qvec);
	if (*pfVi < 0.0f || *pfUi + *pfVi > fDet)
	{
		return FALSE;
	}

	// calculate t, ray intersects triangle
	float fInvDet = 1.0f / fDet;
	*pfDist = VectorDot(edge2, qvec) * fInvDet;
	*pfUi *= fInvDet;
	*pfVi *= fInvDet;
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sCylinderIntersectEdge( VECTOR * pOrigin, float fMin, float fMax, float fRadius, VECTOR & e1, VECTOR & e2, VECTOR & vNormal, float * dist )
{
	VECTOR ctop = *pOrigin;
	VECTOR cbottom = *pOrigin;
	ctop.fZ += fMax;
	cbottom.fZ += fMin;

	VECTOR top;
	VECTOR bottom;

	if ( e1.fZ > e2.fZ )
	{
		top = e1;
		bottom = e2;
	}
	else
	{
		top = e2;
		bottom = e1;
	}

	if ( ctop.fZ < bottom.fZ )
	{
		return FALSE;
	}

	if ( cbottom.fZ > top.fZ )
	{
		return FALSE;
	}

	float distsq1, distsq2;
	VECTOR dir1, dir2;
	if ( ctop.fZ < top.fZ )
	{
		float zpercent = ( top.fZ - ctop.fZ ) / ( top.fZ - bottom.fZ );
		float x = ( ( bottom.fX - top.fX ) * zpercent ) + top.fX;
		float y = ( ( bottom.fY - top.fY ) * zpercent ) + top.fY;
		distsq1 = ( ctop.fX - x ) * ( ctop.fX - x ) + ( ctop.fY - y ) * ( ctop.fY - y );
		dir1.fX = ctop.fX - x;
		dir1.fY = ctop.fY - y;
	}
	else
	{
		distsq1 = ( ctop.fX - top.fX ) * ( ctop.fX - top.fX ) + ( ctop.fY - top.fY ) * ( ctop.fY - top.fY );
		dir1.fX = ctop.fX - top.fX;
		dir1.fY = ctop.fY - top.fY;
	}

	if ( cbottom.fZ > bottom.fZ )
	{
		float zpercent = ( top.fZ - cbottom.fZ ) / ( top.fZ - bottom.fZ );
		float x = ( ( bottom.fX - top.fX ) * zpercent ) + top.fX;
		float y = ( ( bottom.fY - top.fY ) * zpercent ) + top.fY;
		distsq2 = ( cbottom.fX - x ) * ( cbottom.fX - x ) + ( cbottom.fY - y ) * ( cbottom.fY - y );
		dir2.fX = cbottom.fX - x;
		dir2.fY = cbottom.fY - y;
	}
	else
	{
		distsq2 = ( cbottom.fX - bottom.fX ) * ( cbottom.fX - bottom.fX ) + ( cbottom.fY - bottom.fY ) * ( cbottom.fY - bottom.fY );
		dir2.fX = cbottom.fX - bottom.fX;
		dir2.fY = cbottom.fY - bottom.fY;
	}

	if ( ( distsq1 > ( fRadius * fRadius ) ) && ( distsq2 > ( fRadius * fRadius ) ) )
	{
		return FALSE;
	}

	if ( distsq1 < distsq2 )
	{
		ASSERT( distsq1 <= fRadius * fRadius );
		dir1.fZ = 0.0f;
		VectorNormalize( dir1 );
		if ( vNormal.fX * dir1.fX + vNormal.fY * dir1.fY < 0 )
		{
			return FALSE;
		}
		*dist = sqrtf( distsq1 );
	}
	else
	{
		ASSERT( distsq2 <= fRadius * fRadius );
		dir2.fZ = 0.0f;
		VectorNormalize( dir2 );
		if ( vNormal.fX * dir2.fX + vNormal.fY * dir2.fY < 0 )
		{
			return FALSE;
		}
		*dist = sqrtf( distsq2 );
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sMakeBoundingBox( BOUNDING_BOX * pBox, const VECTOR & vCenter, float fRadius, float fHeight )
{
	pBox->vMin.fX = vCenter.fX - fRadius;
	pBox->vMin.fY = vCenter.fY - fRadius;
	pBox->vMin.fZ = vCenter.fZ + MAX_UP_DELTA;
	pBox->vMax = pBox->vMin;

	VECTOR vNew;
	vNew.fX = vCenter.fX + fRadius;
	vNew.fY = vCenter.fY + fRadius;
	vNew.fZ = vCenter.fZ + fHeight;

	BoundingBoxExpandByPoint( pBox, &vNew );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define SPEED_BUFFER_MULTIPLIER 1.01f 
static void sClipNewPositionToSpeed ( VECTOR * pvPositionNew, const VECTOR * pvPositionPrev, float fSpeed )
{
	VECTOR vDelta;
	VectorSubtract( vDelta, *pvPositionNew, *pvPositionPrev );
	float fDistanceSquared = VectorLengthSquared( vDelta );
	if ( fDistanceSquared > SPEED_BUFFER_MULTIPLIER * fSpeed * fSpeed )
	{// make sure that you have not moved too far from your previous location
		float fDeltaLength = sqrt( fDistanceSquared );
		if ( fDeltaLength && fSpeed )
		{
			float fDeltaMultiplier = fSpeed / fDeltaLength;
			vDelta.fX *= fDeltaMultiplier;
			vDelta.fY *= fDeltaMultiplier;
			vDelta.fZ *= fDeltaMultiplier;
		}
		VectorAdd( *pvPositionNew, *pvPositionPrev, vDelta );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
typedef BOOL FP_COLLISION_ITERATE_CALLBACK(ROOM_CPOLY* poly, void* data);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL RoomIterateCollisionNode(
	ROOM * room,
	COLLISION_GRID_NODE * node,
	WORD iter,
	int nCollisionType,
	FP_COLLISION_ITERATE_CALLBACK * fpCallback,
	void * data)
{
	ROOM_DEFINITION * def = room->pDefinition;

	ASSERT_RETFALSE(node);
	for (unsigned int nn = 0; nn < node->nCount[nCollisionType]; ++nn)
	{
		unsigned int index = node->pnPolyList[nCollisionType][nn];
#if	!ISVERSION(OPTIMIZED_VERSION)
		ASSERT(index < (unsigned int)def->nRoomCPolyCount);
#endif
		if (room->pwCollisionIter[index] == iter)
		{
			continue;
		}
		room->pwCollisionIter[index] = iter;
		ROOM_CPOLY * poly = def->pRoomCPolys + index;
		if (fpCallback(poly, data))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WORD RoomGetCollisionIter(
	ROOM * room)
{
	WORD iter = ++room->wCollisionIter;
	if (iter == 0)
	{
		iter = 1;

		if (room->pwCollisionIter)
		{
			unsigned int cpolycount = room->pDefinition->nRoomCPolyCount;
			for (unsigned int ii = 0; ii < cpolycount; ++ii)
			{
				room->pwCollisionIter[ii] = 0;
			}
		}
	}
	return iter;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomIterateCollisionBox(
	ROOM * room,
	float fX1,
	float fY1,
	float fX2,
	float fY2,
	int nCollisionType,
	FP_COLLISION_ITERATE_CALLBACK* fpCallback,
	void * data)
{
	ASSERT_RETFALSE(room && fpCallback);
	ROOM_DEFINITION * def = room->pDefinition;
	ASSERT_RETFALSE(def);

	ASSERT_RETFALSE(def->pCollisionGrid);

	int gx1 = (int)((fX1 - def->fCollisionGridX) / PATHING_MESH_SIZE);
	int gy1 = (int)((fY1 - def->fCollisionGridY) / PATHING_MESH_SIZE);
	gx1 = PIN(gx1, 0, def->nCollisionGridWidth - 1);
	gy1 = PIN(gy1, 0, def->nCollisionGridHeight - 1);
	int gx2 = (int)((fX2 - def->fCollisionGridX) / PATHING_MESH_SIZE);
	int gy2 = (int)((fY2 - def->fCollisionGridY) / PATHING_MESH_SIZE);
	gx2 = PIN(gx2, 0, def->nCollisionGridWidth - 1);
	gy2 = PIN(gy2, 0, def->nCollisionGridHeight - 1);

	WORD iter = RoomGetCollisionIter(room);
	for (int jj = gy1; jj <= gy2; ++jj)
	{
		for (int ii = gx1; ii <= gx2; ++ii)
		{
			COLLISION_GRID_NODE * node = def->pCollisionGrid + jj * def->nCollisionGridWidth + ii;
			if (RoomIterateCollisionNode(room, node, iter, nCollisionType, fpCallback, data))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_COLLIDE_RAY_TEST
{
	ROOM*	room;
	VECTOR	vStart;
	VECTOR	vDirection;
	float	fMinDist;
	BOOL	bCollide;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sRoomIterateCollisionRayDiagonal(
	ROOM * room,
	COLLISION_GRID_NODE * node,
	int x,
	int y,
	int dx,
	int dy,
	float fX1,
	float fY1,
	float fX2,
	float fY2,
	int nCollisionType,
	FP_COLLISION_ITERATE_CALLBACK * fpCallback,
	void * data)
{
	WORD iter = RoomGetCollisionIter(room);

	ROOM_DEFINITION * def = room->pDefinition;
	ASSERT_RETFALSE(def);

#ifdef _DEBUG
	float sx = def->fCollisionGridX + (float)x * PATHING_MESH_SIZE;
	float sy = def->fCollisionGridY + (float)y * PATHING_MESH_SIZE;
	ASSERT(fX1 >= sx && fX1 <= sx + PATHING_MESH_SIZE && fY1 >= sy && fY1 <= sy + PATHING_MESH_SIZE);
#endif

	COLLISION_GRID_NODE * lowerbound = def->pCollisionGrid;
	COLLISION_GRID_NODE * upperbound = lowerbound + def->nCollisionGridWidth * def->nCollisionGridHeight;

	float fSXOffs = 0.0f;
	float fSYOffs = 0.0f;
	if (dx >= 0)
	{
		fSXOffs = PATHING_MESH_SIZE;
	}
	if (dy >= 0)
	{
		fSYOffs = PATHING_MESH_SIZE;
	}
	int span = (dy < 0 ? -def->nCollisionGridWidth : def->nCollisionGridWidth);

	while (node >= lowerbound && node < upperbound)
	{
		if (RoomIterateCollisionNode(room, node, iter, nCollisionType, fpCallback, data))
		{
			return TRUE;
		}

		float sx = def->fCollisionGridX + (float)x * PATHING_MESH_SIZE;
		float sy = def->fCollisionGridY + (float)y * PATHING_MESH_SIZE;

		float vix, viy;
		BOOL bVIntersect = VLineIntersect2D(sy, sy + PATHING_MESH_SIZE, sx + fSXOffs, fX1, fY1, fX2, fY2, &vix, &viy);
		float hix, hiy;
		BOOL bHIntersect = HLineIntersect2D(sx, sx + PATHING_MESH_SIZE, sy + fSYOffs, fX1, fY1, fX2, fY2, &hix, &hiy);
		if (!bVIntersect && !bHIntersect)
		{
			ASSERT(fX2 >= sx - EPSILON && fX2 <= sx + PATHING_MESH_SIZE + EPSILON && fY2 >= sy - EPSILON && fY2 <= sy + PATHING_MESH_SIZE + EPSILON);
			break;
		}
		ASSERT(!(bVIntersect && bHIntersect));
		if (bVIntersect && bHIntersect)
		{
			if ((vix-fX1)*(vix-fX1) + (viy-fY1)*(viy-fY1) < (hix-fX1)*(hix-fX1) + (hiy-fY1)*(hiy-fY1))
			{
				bVIntersect = FALSE;
			}
			else
			{
				bHIntersect = FALSE;
			}
		}
		if (bVIntersect)
		{
			x += dx;
			node += dx;
		}
		else
		{
			y += dy;
			node += span;
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sRoomIterateCollisionRayStraight(
	ROOM * room, 
	COLLISION_GRID_NODE * node,
	COLLISION_GRID_NODE * end,
	int inc,
	int nCollisionType,
	FP_COLLISION_ITERATE_CALLBACK * fpCallback,
	void * data)
{
	ROOM_DEFINITION * def = room->pDefinition;
	ASSERT_RETFALSE(def);

	WORD iter = RoomGetCollisionIter(room);

	for (;node != end; node += inc)
	{
#ifdef _DEBUG
		ROOM_COLLIDE_RAY_TEST * Data = (ROOM_COLLIDE_RAY_TEST *)data;

		ASSERT(node >= def->pCollisionGrid && node < def->pCollisionGrid + def->nCollisionGridWidth * def->nCollisionGridHeight);
		int gx = (int)(node - def->pCollisionGrid) % def->nCollisionGridWidth;
		int gy = (int)(node - def->pCollisionGrid) / def->nCollisionGridWidth;
		ASSERT(gx >= 0 && gx < def->nCollisionGridWidth);
		ASSERT(gy >= 0 && gy < def->nCollisionGridHeight);
		float sx = def->fCollisionGridX + (float)gx * PATHING_MESH_SIZE;
		float sy = def->fCollisionGridY + (float)gy * PATHING_MESH_SIZE;
		if (node != end)
		{
			if (inc == 1)
			{
				ASSERT(VLineIntersect2D(sy, sy + PATHING_MESH_SIZE, sx + PATHING_MESH_SIZE, Data->vStart.fX, Data->vStart.fY, Data->vStart.fX + Data->vDirection.fX, Data->vStart.fY + Data->vDirection.fY));
			}
			else if (inc == -1)
			{
				ASSERT(VLineIntersect2D(sy, sy + PATHING_MESH_SIZE, sx, Data->vStart.fX, Data->vStart.fY, Data->vStart.fX + Data->vDirection.fX, Data->vStart.fY + Data->vDirection.fY));
			}
			else if (inc > 1)
			{
				ASSERT(HLineIntersect2D(sx, sx + PATHING_MESH_SIZE, sy + PATHING_MESH_SIZE, Data->vStart.fX, Data->vStart.fY, Data->vStart.fX + Data->vDirection.fX, Data->vStart.fY + Data->vDirection.fY));
			}
			else
			{
				ASSERT(HLineIntersect2D(sx, sx + PATHING_MESH_SIZE, sy, Data->vStart.fX, Data->vStart.fY, Data->vStart.fX + Data->vDirection.fX, Data->vStart.fY + Data->vDirection.fY));
			}
		}
#endif
		if (RoomIterateCollisionNode(room, node, iter, nCollisionType, fpCallback, data))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomIterateCollisionRay(
	ROOM * room,
	VECTOR & vStart,
	VECTOR & vEnd,
	int nCollisionType,
	FP_COLLISION_ITERATE_CALLBACK * fpCallback,
	void * data)
{
	ASSERT_RETFALSE(room && fpCallback);
	ROOM_DEFINITION * def = room->pDefinition;
	ASSERT_RETFALSE(def);

	if (!ClipLine(vStart.fX, vStart.fY, vEnd.fX, vEnd.fY, def->fCollisionGridX, def->fCollisionGridY, 
		def->fCollisionGridX + def->nCollisionGridWidth * PATHING_MESH_SIZE, def->fCollisionGridY + def->nCollisionGridHeight * PATHING_MESH_SIZE))
	{
		return FALSE;
	}

	int gx1 = (int)((vStart.fX - def->fCollisionGridX) / PATHING_MESH_SIZE);
	int gy1 = (int)((vStart.fY - def->fCollisionGridY) / PATHING_MESH_SIZE);
	int gx2 = (int)((vEnd.fX - def->fCollisionGridX) / PATHING_MESH_SIZE);
	int gy2 = (int)((vEnd.fY - def->fCollisionGridY) / PATHING_MESH_SIZE);
	gx1 = PIN(gx1, 0, def->nCollisionGridWidth - 1);
	gy1 = PIN(gy1, 0, def->nCollisionGridHeight - 1);
	gx2 = PIN(gx2, 0, def->nCollisionGridWidth - 1);
	gy2 = PIN(gy2, 0, def->nCollisionGridHeight - 1);
	int dx = gx2 - gx1;
	int dy = gy2 - gy1;
	int dir = (dy == 0 ? 1 : (dy < 0 ? 0 : 2)) * 3 + (dx == 0 ? 1 : (dx < 0 ? 0 : 2));

	COLLISION_GRID_NODE * node = def->pCollisionGrid + gy1 * def->nCollisionGridWidth + gx1;
	COLLISION_GRID_NODE * end = def->pCollisionGrid + gy2 * def->nCollisionGridWidth + gx2;
	int span = def->nCollisionGridWidth;
	
	switch (dir)
	{
	case 0:
		return (sRoomIterateCollisionRayDiagonal(room, node, gx1, gy1, -1, -1, vStart.fX, vStart.fY, vEnd.fX, vEnd.fY, nCollisionType, fpCallback, data));
	case 1:
		return (sRoomIterateCollisionRayStraight(room, node, end, -span, nCollisionType, fpCallback, data));
	case 2:
		return (sRoomIterateCollisionRayDiagonal(room, node, gx1, gy1, 1, -1, vStart.fX, vStart.fY, vEnd.fX, vEnd.fY, nCollisionType, fpCallback, data));
	case 3:
		return (sRoomIterateCollisionRayStraight(room, node, end, -1, nCollisionType, fpCallback, data));
	case 4:
		return (RoomIterateCollisionNode(room, node, RoomGetCollisionIter(room), nCollisionType, fpCallback, data));
	case 5:
		return (sRoomIterateCollisionRayStraight(room, node, end, 1, nCollisionType, fpCallback, data));
	case 6:
		return (sRoomIterateCollisionRayDiagonal(room, node, gx1, gy1, -1, 1, vStart.fX, vStart.fY, vEnd.fX, vEnd.fY, nCollisionType, fpCallback, data));
	case 7:
		return (sRoomIterateCollisionRayStraight(room, node, end, span, nCollisionType, fpCallback, data));
	case 8:
		return (sRoomIterateCollisionRayDiagonal(room, node, gx1, gy1, 1, 1, vStart.fX, vStart.fY, vEnd.fX, vEnd.fY, nCollisionType, fpCallback, data));
	}

	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sRoomIterateCollisionRayTestCallback(
	ROOM_CPOLY * poly,
	void * data)
{
	ROOM_COLLIDE_RAY_TEST* Data = (ROOM_COLLIDE_RAY_TEST*)data;

	VECTOR vDirNormal = Data->vDirection;
	VectorNormalize(vDirNormal);
	float fDist, fU, fV;
	BOOL bHit = RayIntersectTriangle(&Data->vStart, &vDirNormal, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition, &fDist, &fU, &fV);
	if (bHit)
	{
		if ( fDist < Data->fMinDist && fDist > COLLIDE_MIN_DIST )
		{
			Data->bCollide = TRUE;
			Data->fMinDist = fDist;
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RoomIterateCollisionRayTest(
	UNIT* unit,
	ROOM* room,
	VECTOR& vPosition,
	VECTOR& vDirection)
{
	ASSERT_RETURN(unit && room);

	ROOM_COLLIDE_RAY_TEST data;
	data.room = room;
	data.vStart = vPosition + VECTOR(0.0f, 0.0f, 1.0f);
	data.vDirection = vDirection * 1000.0f;
	data.fMinDist = 100.0f;
	data.bCollide = FALSE;

	VECTOR vec = data.vStart + data.vDirection;
	RoomIterateCollisionRay(room, data.vStart, vec, COLLISION_WALL, sRoomIterateCollisionRayTestCallback, (void*)&data);
	if (data.bCollide)
	{
		trace("test: %0.4f\n", data.fMinDist);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_COLLIDE_WALL_DATA
{
	ROOM*				room;
	BOUNDING_BOX*		ptBoxInRoom;
	float				fRadius;
	float				fRadiusSquared;
	float				fHeight;

	// new collision vars
	BOOL				first;
	BOOL				abort;
	BOOL				nearest_embedded;
	VECTOR				source_point;
	float				nearest_distance;
	VECTOR				nearest_polygon_intersection_point;
	ROOM_CPOLY *		nearest_poly;
	VECTOR				velocity;
	float				distance;		// length of velocity vector
	DWORD				flags;
	int					num_spheres;
	BOOL				result_wall;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float sDistancePointPlane( VECTOR p, VECTOR plane_point0, VECTOR plane_normal, VECTOR * out )
{
	VECTOR delta = p - plane_point0;
	float sn = -VectorDot( plane_normal, delta );
	float sd = VectorDot( plane_normal, plane_normal );
	float sb = sn / sd;

	out->x = ( plane_normal.x * sb ) + p.x;
	out->y = ( plane_normal.y * sb ) + p.y;
	out->z = ( plane_normal.z * sb ) + p.z;

	delta = p - *out;
	return sqrtf( VectorDot( delta, delta ) );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL sSameSide( VECTOR p1, VECTOR p2, VECTOR a, VECTOR b )
{
	VECTOR delta_ba = b - a;
	VECTOR delta_p1a = p1 - a;
	VECTOR delta_p2a = p2 - a;

	VECTOR cp1, cp2;
	VectorCross( cp1, delta_ba, delta_p1a );
	VectorCross( cp2, delta_ba, delta_p2a );

	if ( VectorDot( cp1, cp2 ) >= 0.0f )
		return TRUE;

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define COLLIDE_EPSILON			( 0.001f )
#define COS_60DEGREES			0.5f

//-------------------------------------------------------------------------------------------------

struct ROOM_COLLIDE_SPHERE_DATA 
{
	VECTOR			vResultDir;		// resultant direction to travel toward (the edge) or out of the collision
	float			fResultDist;	// resultant distance
	ROOM_CPOLY *	pResultPoly;	// polygon collision occured with
	VECTOR			vIntersection;	// intersection point
	BOOL			bEmbedded;		// was it an embedded collision
};

//-------------------------------------------------------------------------------------------------

enum
{
	NCF_NONE			= 0,
	NCF_NO_Z			= MAKE_MASK( 1 ),
	NCF_WALLS_ONLY		= MAKE_MASK( 2 ),
	NCF_FLOORS_ONLY		= MAKE_MASK( 3 ),
	NCF_CEILINGS_ONLY	= MAKE_MASK( 4 ),
	NCF_Z_ADJUST		= MAKE_MASK( 5 ),

	NCF_CLEAR_Z			= ~( NCF_NO_Z ),
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sAdjustSphereZ( int index, VECTOR & vStart, VECTOR & vEnd, DWORD & flags, float radius, float height )
{
	flags &= NCF_CLEAR_Z;
	switch ( index )
	{
	case 0:
		// first sphere at + radius from feet
		vStart.z += radius + COLLIDE_EPSILON;
		vEnd.z += radius + COLLIDE_EPSILON;
		flags |= NCF_NONE;
		break;
	case 1:
		// second sphere in middle
		vStart.z += height * 0.5f;
		vEnd.z += height * 0.5f;
		flags |= NCF_NO_Z;
		break;
	case 2:
		// last sphere at top - radius of sphere
		vStart.z += height - radius;
		vEnd.z += height - radius;
		flags |= NCF_NO_Z;
		break;
	default:
		ASSERTX( ( index >=0 ) && ( index <= 2 ), "Invalid sphere index for collision (0-2)" );
		break;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sTranslateProp( PROP_NODE * prop, ROOM * room )
{
	ASSERT( prop->pCPoints == NULL );
	ASSERT( prop->pCPolys == NULL );
	ROOM_POINT * srcpt = prop->m_pDefinition->pRoomPoints;
	ROOM_POINT * destpt = ( ROOM_POINT * )GMALLOCZ( RoomGetGame( room ), sizeof( ROOM_POINT ) * prop->m_pDefinition->nRoomPointCount );;
	prop->pCPoints = destpt;
	for ( int ii = 0; ii < prop->m_pDefinition->nRoomPointCount; ii++, srcpt++, destpt++ )
	{
		MatrixMultiply( &destpt->vPosition, &srcpt->vPosition, &prop->m_matrixPropToRoom );
		destpt->nIndex = srcpt->nIndex;
	}
	ROOM_CPOLY * srcpoly = prop->m_pDefinition->pRoomCPolys;
	ROOM_CPOLY * destpoly = ( ROOM_CPOLY * )GMALLOCZ( RoomGetGame( room ), sizeof( ROOM_CPOLY ) * prop->m_pDefinition->nRoomCPolyCount );
	prop->pCPolys = destpoly;
	for ( int i = 0; i < prop->m_pDefinition->nRoomCPolyCount; i++, srcpoly++, destpoly++ )
	{
		destpoly->pPt1 = &prop->pCPoints[srcpoly->pPt1->nIndex];
		destpoly->pPt2 = &prop->pCPoints[srcpoly->pPt2->nIndex];
		destpoly->pPt3 = &prop->pCPoints[srcpoly->pPt3->nIndex];
		if ( i == 0 )
		{
			destpoly->tBoundingBox.vMin = destpoly->pPt1->vPosition;
			destpoly->tBoundingBox.vMax = destpoly->pPt1->vPosition;
		}
		else
		{
			BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt1->vPosition );
		}
		BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt2->vPosition );
		BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt3->vPosition );
		MatrixMultiplyNormal( &destpoly->vNormal, &srcpoly->vNormal, &prop->m_matrixPropToRoom );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideProp(
	ROOM* room,
	PROP_NODE* prop,
	FP_COLLISION_ITERATE_CALLBACK* fpCallback,
	void * data)
{
	// check if we collide in prop-space
	ROOM_COLLIDE_WALL_DATA * Data = ( ROOM_COLLIDE_WALL_DATA * )data;
	VECTOR vPropPosition = VECTOR( prop->m_matrixPropToRoom._41, prop->m_matrixPropToRoom._42, prop->m_matrixPropToRoom._43 );
	float fDistSq = VectorDistanceSquared( Data->source_point, vPropPosition );
	float fMin = prop->m_pDefinition->fRoomCRadius + Data->fRadius + Data->distance;
	if ( fDistSq > fMin * fMin )
	{
		return FALSE;
	}

	// crude collided... now lets find out more...
	if ( ( prop->pCPoints == NULL ) || ( prop->pCPolys == NULL ) )
	{
		sTranslateProp( prop, room );
	}

	ROOM_CPOLY* cpoly = prop->pCPolys;

	for (int ii = 0; ii < prop->m_pDefinition->nRoomCPolyCount; ii++, cpoly++)
	{
		if (fpCallback(cpoly, data))
		{
			return TRUE;
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------------------

float distance_point_plane( VECTOR point, VECTOR plane_point, VECTOR plane_normal )
{
	VECTOR delta = point - plane_point;
	return VectorDot( plane_normal, delta );
}

//-------------------------------------------------------------------------------------------------

float intersect( VECTOR point, VECTOR direction, VECTOR plane_point, VECTOR plane_normal )
{
	VECTOR w = point - plane_point;
	float n = -VectorDot( plane_normal, w );
	float d = VectorDot( plane_normal, direction );
	return n / d;
}

//-------------------------------------------------------------------------------------------------

float intersect_sphere( VECTOR sphere_center, float sphere_radius, VECTOR point, VECTOR dir )
{
	VECTOR dst = point - sphere_center;
	float b = VectorDot( dst, dir );
	float c = VectorDot( dst, dst ) - ( sphere_radius * sphere_radius );
	float d = ( b * b ) - c;

	if ( d < 0.0f )
		return -1.0f;

	return -b - sqrtf( d );
} 

//-------------------------------------------------------------------------------------------------

BOOL same_side( VECTOR & p1, VECTOR & p2, VECTOR & a, VECTOR & b )
{
	VECTOR delta_ba = b - a;
	VECTOR delta_p1a = p1 - a;
	VECTOR delta_p2a = p2 - a;

	VECTOR cp1, cp2;
	VectorCross( cp1, delta_ba, delta_p1a );
	VectorCross( cp2, delta_ba, delta_p2a );

	if ( VectorDot( cp1, cp2 ) >= 0.0f )
		return TRUE;

	return FALSE;
}

//-------------------------------------------------------------------------------------------------

BOOL point_in_polygon( VECTOR & point, VECTOR & p0, VECTOR & p1, VECTOR & p2 )
{
	if ( same_side( point, p0, p1, p2 ) && same_side( point, p1, p0, p2 ) && same_side( point, p2, p0, p1 ) )
		return TRUE;

	return FALSE;
}

//-------------------------------------------------------------------------------------------------

VECTOR distance_point_line( VECTOR & point, VECTOR & line0, VECTOR & line1 )
{
	// Determine t (the length of the VECTOR from ‘a’ to ‘p’)
	VECTOR c = point - line0;
	VECTOR v = line1 - line0;
	float d = VectorLength( v );
	VectorNormalize( v );
	float t = VectorDot( v, c );

	// Check to see if ‘t’ is beyond the extents of the line segment
	if ( t < 0.0f ) return line0;
	if ( t > d ) return line1;

	// Return the point between ‘a’ and ‘b’
	v *= t;
	return line0 + v;
}

//-------------------------------------------------------------------------------------------------

VECTOR closest_point_on_triangle( VECTOR & point, VECTOR & p0, VECTOR & p1, VECTOR & p2 )
{
	VECTOR p01 = distance_point_line( point, p0, p1 );
	VECTOR p12 = distance_point_line( point, p1, p2 );
	VECTOR p20 = distance_point_line( point, p2, p0 );
	float dsq01 = VectorDistanceSquared( point, p01 );
	float dsq12 = VectorDistanceSquared( point, p12 );
	float dsq20 = VectorDistanceSquared( point, p20 );

	float min = dsq01;
	VECTOR ret = p01;
	if ( dsq12 < min )
	{
		min = dsq12;
		ret = p12;
	}
	if ( dsq20 < min )
	{
		min = dsq20;
		ret = p20;
	}
	return ret;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL sRoomCollideWallsSphereCallback(
	ROOM_CPOLY* poly,
	void* data)
{
	ROOM_COLLIDE_WALL_DATA * Data = ( ROOM_COLLIDE_WALL_DATA * )data;
	BOUNDING_BOX& tBoxInRoom = *(Data->ptBoxInRoom);
	float flWalkableZNormal = CollisionWalkableZNormal();
	BOOL bWall = ( poly->vNormal.z < flWalkableZNormal && poly->vNormal.z > -flWalkableZNormal );
	BOOL bFloor = ( poly->vNormal.z >= flWalkableZNormal );
	BOOL bCeiling = ( poly->vNormal.z <= 0.0f );
	// is this floors only and this poly is considered a wall?
	if ( ( Data->flags & NCF_FLOORS_ONLY ) && !bFloor )
		return FALSE;
	// is this walls only and this poly is considered a floor?
	if ( ( Data->flags & NCF_WALLS_ONLY ) && !bWall )
		return FALSE;
	if ( ( Data->flags & NCF_CEILINGS_ONLY ) && !bCeiling )
		return FALSE;
	if ( BoundingBoxCollide( &tBoxInRoom, &poly->tBoundingBox ) )
	{
		if ( VectorIsZero( poly->vNormal ) )
			return FALSE;

		// only facing against our direction...
		VECTOR travel_direction = Data->velocity;
		VectorNormalize( travel_direction );
		float facing = VectorDot( travel_direction, poly->vNormal );
		if ( facing >= 0.0f && facing <= 1.0f )
			return FALSE;

		// player is made up of 3 spheres ( top, mid, bottom )
		for ( int i = 0; i < Data->num_spheres; i++ )
		{
			VECTOR vStart = Data->source_point;
			VECTOR vEnd = Data->source_point + Data->velocity;
			if ( Data->flags & NCF_Z_ADJUST )
			{
				sAdjustSphereZ( i, vStart, vEnd, Data->flags, Data->fRadius, Data->fHeight );
			}

			// Determine the distance from the plane to the source
			float distance = distance_point_plane( vStart, poly->pPt1->vPosition, poly->vNormal );

			// only polys in front of us...
			if ( distance < 0.0f )
				continue;

			// Is the plane embedded?
			if ( distance + COLLIDE_EPSILON < Data->fRadius )
			{
				// Calculate the plane intersection point
				VECTOR delta = poly->vNormal * distance;
				VECTOR intersection_point = vStart - delta;

				// is the intersection point within the poly?
				if ( !point_in_polygon( intersection_point, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition ) )
				{
					// get the closest point on the poly to the original plane intersection point
					intersection_point = closest_point_on_triangle( intersection_point, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition );
				}

				VECTOR closest_direction = intersection_point - vStart;
				float d = VectorLength( closest_direction );
				if ( d + COLLIDE_EPSILON < Data->fRadius )
				{
					// Embedded... deal with it
					// closest embedded?
					if ( Data->first || !Data->nearest_embedded || d < Data->nearest_distance )
					{
						Data->nearest_distance = d;
						Data->nearest_poly = poly;
						Data->nearest_polygon_intersection_point.x = intersection_point.x;
						Data->nearest_polygon_intersection_point.y = intersection_point.y;
						if ( Data->flags & NCF_NO_Z )
							Data->nearest_polygon_intersection_point.z = Data->source_point.z;
						else
							Data->nearest_polygon_intersection_point.z = intersection_point.z - ( vStart.z - Data->source_point.z );
						Data->nearest_embedded = TRUE;
						Data->first = FALSE;
						Data->result_wall = bWall;
					}
				}
			}

			if ( !Data->nearest_embedded )
			{
				// Calculate the closest point to the plane and on the edge of the sphere
				VECTOR temp = poly->vNormal * Data->fRadius;
				VECTOR sphere_edge = vStart - temp;

				// Calculate the plane intersection point
				float t = intersect( sphere_edge, Data->velocity, poly->pPt1->vPosition, poly->vNormal );

				// Calculate the plane intersection point
				VECTOR delta = Data->velocity * t;
				VECTOR intersection_point = sphere_edge + delta;

				// is the intersection point within the poly?
				if ( !point_in_polygon( intersection_point, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition ) )
				{
					// get the closest point on the poly to the original plane intersection point
					intersection_point = closest_point_on_triangle( intersection_point, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition );
				}

				// Invert the velocity vector
				VECTOR negative_velocity = -Data->velocity;
				VectorNormalize( negative_velocity );

				// Using the polygonIntersectionPoint, we need to reverse-intersect with the sphere
				float d = intersect_sphere( vStart, Data->fRadius, intersection_point, negative_velocity );

				// Was there an intersection with the sphere?
				if ( ( ( d + COLLIDE_EPSILON ) >= 0.0f ) &&
					 ( ( d + COLLIDE_EPSILON ) <= Data->distance ) )
				{
					// Closest intersection thus far?
					if ( Data->first || d < Data->nearest_distance )
					{
						// save this intersection
						Data->nearest_distance = d;
						Data->nearest_poly = poly;
						Data->nearest_polygon_intersection_point.x = intersection_point.x;
						Data->nearest_polygon_intersection_point.y = intersection_point.y;
						if ( Data->flags & NCF_NO_Z )
							Data->nearest_polygon_intersection_point.z = Data->source_point.z;
						else
							Data->nearest_polygon_intersection_point.z = intersection_point.z - ( vStart.z - Data->source_point.z );
						Data->nearest_embedded = FALSE;
						Data->first = FALSE;
						Data->result_wall = bWall;
					}
				}
			}

		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideWalls(
	UNIT * unit,
	ROOM * room,
	const VECTOR & vPositionPrevInRoom,
	VECTOR & vPositionNewInRoom,
	float fRadius,
	float fHeight)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	ROOM_COLLIDE_WALL_DATA data;
	data.room = room;
	data.fRadius = fRadius;
	data.fRadiusSquared = fRadius * fRadius;
	data.fHeight = fHeight;
	if ( unit && UnitIsPlayer( unit ) )
		data.num_spheres = 3;
	else
		data.num_spheres = 1;

	VECTOR start_point = vPositionPrevInRoom;
	data.source_point = vPositionPrevInRoom;
	data.velocity = vPositionNewInRoom - vPositionPrevInRoom;

	float flWalkableZNormal = CollisionWalkableZNormal();

	BOOL done = FALSE;
	BOOL wall_hit = FALSE;
	int count = 0;
	float original_distance = 1.0f;
	while ( !done )
	{
		// How far do we need to go?
		data.distance = VectorLength( data.velocity );

		if ( count == 0 )
			original_distance = data.distance;

		// Do we need to bother?
		if ( data.distance < COLLIDE_EPSILON )
		{
			vPositionNewInRoom = data.source_point;
			return wall_hit;
		}

		// What's our destination?
		VECTOR destination_point = data.source_point + data.velocity;

		// Determine the nearest collider from the list potentialColliders
		BOUNDING_BOX tBoxInRoom;
		VECTOR vTemp;
		vTemp.x = data.source_point.x - data.fRadius;
		vTemp.y = data.source_point.y - data.fRadius;
		vTemp.z = data.source_point.z - data.fRadius;
		tBoxInRoom.vMin = vTemp;
		tBoxInRoom.vMax = vTemp;
		vTemp += data.velocity;
		BoundingBoxExpandByPoint( &tBoxInRoom, &vTemp );
		vTemp.x = data.source_point.x + data.fRadius;
		vTemp.y = data.source_point.y + data.fRadius;
		vTemp.z = data.source_point.z + data.fHeight + data.fRadius;
		BoundingBoxExpandByPoint( &tBoxInRoom, &vTemp );
		vTemp += data.velocity;
		BoundingBoxExpandByPoint( &tBoxInRoom, &vTemp );
		data.ptBoxInRoom = &tBoxInRoom;

		data.first = TRUE;
		data.flags = NCF_NONE | NCF_Z_ADJUST;
		data.nearest_distance = -1.0;
		data.nearest_poly = NULL;
		data.nearest_polygon_intersection_point;
		data.nearest_embedded = FALSE;
		data.result_wall = FALSE;

		// find nearest
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.x, tBoxInRoom.vMin.y, tBoxInRoom.vMax.x, tBoxInRoom.vMax.y, COLLISION_WALL, sRoomCollideWallsSphereCallback, (void*)&data);
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.x, tBoxInRoom.vMin.y, tBoxInRoom.vMax.x, tBoxInRoom.vMax.y, COLLISION_FLOOR, sRoomCollideWallsSphereCallback, (void*)&data);
		PROP_NODE * prop = room->pProps;
		while (prop)
		{
			sRoomCollideProp(room, prop, sRoomCollideWallsSphereCallback, (void*)&data);
			prop = prop->m_pNext;
		}

		// If we never found a collision, we can safely move to the destination and bail
		if ( data.first )
		{
			vPositionNewInRoom = data.source_point + data.velocity;
			return wall_hit;
		}

		wall_hit |= data.result_wall;

		if ( data.nearest_embedded )
		{
			// push out of sphere if embedded
			VECTOR push_direction = data.source_point - data.nearest_polygon_intersection_point;
			VectorNormalize( push_direction );
			float distance_to_push = data.fRadius - data.nearest_distance;
			push_direction *= distance_to_push;
			data.source_point += push_direction;
			//if ( push_direction.z != 0.0f )
			//	data.source_point.z += COLLIDE_EPSILON;
		}
		else
		{
			// move source point to polygon
			VECTOR v = data.velocity;
			VectorNormalize( v );
			v *= data.nearest_distance;
			data.source_point += v;
		}

		count++;
		if ( count > 5 )
		{
			vPositionNewInRoom.x = start_point.x;
			vPositionNewInRoom.y = start_point.y;
			// let the Z take care of itself later...
			return wall_hit;
		}

		if ( !data.nearest_embedded )
		{
			// Determine the sliding plane (we do this now, because we're about to change sourcePoint)
			VECTOR slide_plane_point = data.nearest_polygon_intersection_point;
			VECTOR slide_plane_normal = data.nearest_polygon_intersection_point - data.source_point;
			if ( !VectorIsZero( slide_plane_normal ) )
			{
				// We now project the destination point onto the sliding plane
				float slide_time = intersect( destination_point, slide_plane_normal, slide_plane_point, slide_plane_normal );

				VECTOR destination_projection_normal = slide_plane_normal * slide_time;
				VECTOR new_destination_point = destination_point + destination_projection_normal;

				// Generate the slide vector, which will become our new velocity vector for the next iteration
				data.velocity = new_destination_point - data.nearest_polygon_intersection_point;

				// did I want to go straight, and now I'm going up hill?
				ASSERT_CONTINUE( data.nearest_poly != NULL );
				if ( ( ( vPositionNewInRoom.z - vPositionPrevInRoom.z ) == 0.0f ) && ( data.velocity.z > 0.0f ) && ( data.nearest_poly->vNormal.z >= flWalkableZNormal ) )
				{
					// use the original distance to calculate the new vector
					VectorNormalize( data.velocity );
					//data.velocity *= ( original_distance * 0.75f );
					data.velocity *= original_distance;
				}
			}
			else
			{
				// stop, there is no normal for sliding
				vPositionNewInRoom = data.source_point;
				return wall_hit;
			}
		}
	}

	// should never get here...
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL sRoomCollideSphereDistance(
	ROOM * room,
	VECTOR & vPositionInRoom,
	VECTOR vTestDirection,
	float fTestDistance,
	float fRadius,
	RCSFLAGS eFlags,
	ROOM_COLLIDE_SPHERE_DATA & result_data )
{
	ASSERT_RETFALSE( room );

	// set up data
	ROOM_COLLIDE_WALL_DATA data;
	data.room = room;
	data.fRadius = fRadius;
	data.fHeight = fRadius;
	data.fRadiusSquared = fRadius * fRadius;

	VECTOR start_point = vPositionInRoom;
	data.source_point = vPositionInRoom;

	// is this really a direction?
	data.velocity = vTestDirection;
	VectorNormalize( data.velocity );
	if ( VectorIsZero( data.velocity ) )
		return FALSE;

	// Do we need to bother?
	if ( fTestDistance < COLLIDE_EPSILON )
		return FALSE;

	data.velocity *= fTestDistance;
	data.distance = fTestDistance;
	data.flags = NCF_NONE;
	if ( ( eFlags & RCSFLAGS_ALL ) == RCSFLAGS_FLOORS_ONLY )
		data.flags |= NCF_FLOORS_ONLY;
	if ( eFlags & RCSFLAGS_Z_ADJUST )
		data.flags |= NCF_Z_ADJUST;
	data.num_spheres = 1;

	// Determine the nearest collider from the list potentialColliders
	BOUNDING_BOX tBoxInRoom;
	VECTOR vTemp = data.source_point;
	if ( eFlags & RCSFLAGS_Z_ADJUST )
		vTemp.z += data.fRadius * 1.1f;
	tBoxInRoom.vMin = vTemp;
	tBoxInRoom.vMax = vTemp;
	vTemp += data.velocity;
	BoundingBoxExpandByPoint( &tBoxInRoom, &vTemp );
	BoundingBoxExpandBySize( &tBoxInRoom, data.fRadius * 1.1f );
	data.ptBoxInRoom = &tBoxInRoom;

	data.first = TRUE;
	data.nearest_distance = -1.0;
	data.nearest_polygon_intersection_point;
	data.nearest_embedded = FALSE;

	// find nearest
	if ( eFlags & RCSFLAGS_WALLS_AND_FLOORS )
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.x, tBoxInRoom.vMin.y, tBoxInRoom.vMax.x, tBoxInRoom.vMax.y, COLLISION_WALL, sRoomCollideWallsSphereCallback, (void*)&data);
	if ( eFlags & RCSFLAGS_FLOORS )
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.x, tBoxInRoom.vMin.y, tBoxInRoom.vMax.x, tBoxInRoom.vMax.y, COLLISION_FLOOR, sRoomCollideWallsSphereCallback, (void*)&data);
	if ( eFlags & RCSFLAGS_CEILINGS )
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.x, tBoxInRoom.vMin.y, tBoxInRoom.vMax.x, tBoxInRoom.vMax.y, COLLISION_CEILING, sRoomCollideWallsSphereCallback, (void*)&data);
	if ( eFlags & RCSFLAGS_PROPS )
	{
		PROP_NODE * prop = room->pProps;
		while (prop)
		{
			BOOL bIgnore = FALSE;
			if ( ( eFlags & RCSFLAGS_EITHER_CAMERA ) && prop->m_pDefinition )
			{
				ROOM_INDEX * pPropIndex = NULL;
				if( prop->m_pDefinition->nRoomExcelIndex >= 0 )
				{
					pPropIndex = (ROOM_INDEX*)ExcelGetData( RoomGetGame( room ), DATATABLE_PROPS, prop->m_pDefinition->nRoomExcelIndex );
				}
				if ( ( eFlags & RCSFLAGS_CAMERA ) && pPropIndex && pPropIndex->b3rdPersonCameraIgnore )
					bIgnore = TRUE;
				if ( ( eFlags & RCSFLAGS_RTS_CAMERA ) && pPropIndex && pPropIndex->bRTSCameraIgnore )
					bIgnore = TRUE;
			}
			if ( !bIgnore )
				sRoomCollideProp(room, prop, sRoomCollideWallsSphereCallback, (void*)&data);
			prop = prop->m_pNext;
		}
	}

	// If we never found a collision, return FALSE
	if ( data.first )
	{
		return FALSE;
	}

	result_data.pResultPoly = data.nearest_poly;

	// calculate embedded return values (sphere was embedded to begin with)
	if ( data.nearest_embedded )
	{
		// push out of sphere if embedded
		result_data.vResultDir = data.source_point - data.nearest_polygon_intersection_point;
		VectorNormalize( result_data.vResultDir );
		result_data.fResultDist = ( data.fRadius - data.nearest_distance ) + COLLIDE_EPSILON;
		result_data.vIntersection = data.nearest_polygon_intersection_point;
		result_data.bEmbedded = TRUE;
		return TRUE;
	}

	// return how far away (and in what dir) the nearest collision is
	result_data.vResultDir = data.velocity;
	VectorNormalize( result_data.vResultDir );
	result_data.fResultDist = data.nearest_distance;
	result_data.vIntersection = data.nearest_polygon_intersection_point;
	result_data.bEmbedded = FALSE;
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideGround(
	UNIT * unit,
	ROOM * room,
	const VECTOR & vOriginalPosition,
	VECTOR & vPositionInRoom,
	float fRadius,
	float fHeight,
	DWORD * pdwMoveResult )
{
	ASSERT_RETFALSE( unit && room );

	// are we still in this room?
	VECTOR vTestPosition = vPositionInRoom;
	if ( !UnitIsOnGround( unit ) )
	{
		if ( UnitGetZVelocity( unit ) >= 0.0f )
			return FALSE;
		vTestPosition.z = vOriginalPosition.z;
	}
	VECTOR vWorldPosition = vTestPosition;
	MatrixMultiply( &vWorldPosition, &vWorldPosition, &room->tWorldMatrix );
	if ( !ConvexHullPointTest( room->pHull, &vWorldPosition ) )
		return FALSE;

	// calculate the legit amount of downward direction we can go...
	float fTestDistance = ( UnitGetVelocity( unit ) / COS_60DEGREES ) * AppCommonGetElapsedTime();
	if ( !UnitIsOnGround( unit ) )
		fTestDistance += vOriginalPosition.z - vPositionInRoom.z + 0.05f;
	if ( fTestDistance <= 0.0f )
		return FALSE;
	VECTOR vTestDirection = VECTOR( 0.0f, 0.0f, -1.0f );
	ROOM_COLLIDE_SPHERE_DATA result_data;
	BOOL bCollide = sRoomCollideSphereDistance( room, vTestPosition, vTestDirection, fTestDistance, fRadius, RCSFLAGS_GROUND_UNIT, result_data );

	if ( bCollide )
	{
		float fWalkable = CollisionWalkableZNormal();
		VECTOR vCollisionDirection;
		if ( result_data.bEmbedded )
			vCollisionDirection = result_data.vResultDir;
		else
			vCollisionDirection = vTestPosition - result_data.vIntersection;
		VectorNormalize( vCollisionDirection );
		float fCollisionDistanceXY = VectorDistanceSquaredXY( result_data.vIntersection, vPositionInRoom );

		// landed
		if ( !UnitIsOnGround( unit ) )
		{
			// only run on the way down...
			if ( UnitGetZVelocity( unit ) <= 0.0f )
			{
				if ( vCollisionDirection.z >= fWalkable )
				{
					// are you close to the center of our sphere and this is a decent surface?
					float fMaxXY = fRadius * 0.5f;
					if ( fCollisionDistanceXY < ( fMaxXY * fMaxXY ) )
					{
						SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
					}
					else
					{
						// calc reverse normal of poly we hit to edge of sphere
						VECTOR vEdge = -result_data.pResultPoly->vNormal * fRadius;
						vEdge += vPositionInRoom;
						// if that is close to where we landed/hit then we are on that surface, otherwise it is smashing into us
						VECTOR vDelta = vEdge - result_data.vIntersection;
						float fTotal = fabsf( vDelta.x ) + fabsf( vDelta.y ) + fabsf( vDelta.z );
						if ( ( fTotal < COLLIDE_EPSILON ) && ( result_data.pResultPoly->vNormal.z >= fWalkable ) )
						{
							SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
						}
					}
				}

				// if we haven't been set down on the ground, make sure we are still moving downwards
				if ( !TESTBIT( pdwMoveResult, MOVERESULT_SETONGROUND ) )
				{
					float fResultZ = vOriginalPosition.z - vPositionInRoom.z;
					float fPartZ = UnitGetZVelocity( unit ) * 0.1f;
					if ( ( fResultZ < COLLIDE_EPSILON ) && ( fPartZ < -0.5f ) )
					{
						SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
					}
				}

				if ( TESTBIT( pdwMoveResult, MOVERESULT_SETONGROUND ) )
				{
					VECTOR vMovement = result_data.vResultDir;
					vMovement *= result_data.fResultDist;
					vPositionInRoom.z = vTestPosition.z + vMovement.z + COLLIDE_EPSILON;
				}
			}
		}
		else
		{
			// walking on ground
			{
				// did I go up?
				float delta_z = vPositionInRoom.z - vOriginalPosition.z;
				// no need to do anything if I'm just going upwards and the wall collision took care of me
				if ( delta_z > COLLIDE_EPSILON )
				{
					if ( result_data.pResultPoly->vNormal.z >= fWalkable )
						return FALSE;
					else
						SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
				}
				else
				{
					float fMaxXY = fRadius * 0.5f;
					if ( fCollisionDistanceXY > ( fMaxXY * fMaxXY ) )
					{
						// more than halfway through my collision radius.... let's see how far down it is...
						fTestDistance = MAX_UP_DELTA;
						MatrixMultiply( &vWorldPosition, &vPositionInRoom, &room->tWorldMatrix );						
						float fDistanceDown = LevelLineCollideLen( UnitGetGame( unit ), UnitGetLevel( unit ), vWorldPosition, vTestDirection, fTestDistance );
						if ( fDistanceDown >= MAX_UP_DELTA )
							SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
					}
				}
				if ( !TESTBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND ) )
				{
					VECTOR vMovement = result_data.vResultDir;
					vMovement *= result_data.fResultDist;
					vPositionInRoom.z = vTestPosition.z + vMovement.z + COLLIDE_EPSILON;
				}
			}
		}

	}
	else
	{
		// didn't hit anything and I was on ground?
		if ( UnitIsOnGround( unit ) )
		{
			SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
		}
	}

	return bCollide;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sNewRoomCollideGround(
	UNIT * unit,
	ROOM * room,
	const VECTOR & vOriginalPosition,
	VECTOR & vPositionInRoom,
	float fRadius,
	float fHeight,
	DWORD * pdwMoveResult )
{
	ASSERT_RETFALSE( unit && room );
	if ( !UnitIsOnGround( unit ) && ( UnitGetZVelocity( unit ) >= 0.0f ) )
		return FALSE;

	// are we still in this room?
	VECTOR vTestPosition = vPositionInRoom;
	if ( !UnitIsOnGround( unit ) )
	{
		vTestPosition.z = vOriginalPosition.z;
	}
	VECTOR vWorldPosition = vTestPosition;
	MatrixMultiply( &vWorldPosition, &vWorldPosition, &room->tWorldMatrix );
	if ( !ConvexHullPointTest( room->pHull, &vWorldPosition ) )
		return FALSE;

	// calculate the legit amount of downward direction we can go...
	float fTestDistance = ( UnitGetVelocity( unit ) / COS_60DEGREES ) * AppCommonGetElapsedTime();
	if ( !UnitIsOnGround( unit ) )
		fTestDistance += vOriginalPosition.z - vPositionInRoom.z + 0.05f;
	if ( fTestDistance <= 0.0f )
		return FALSE;
	VECTOR vTestDirection = VECTOR( 0.0f, 0.0f, -1.0f );
	ROOM_COLLIDE_SPHERE_DATA result_data;
	BOOL bCollide = sRoomCollideSphereDistance( room, vTestPosition, vTestDirection, fTestDistance, fRadius, RCSFLAGS_GROUND_UNIT, result_data );

	if ( bCollide )
	{
		float fWalkable = CollisionWalkableZNormal();
		VECTOR vCollisionDirection;
		if ( result_data.bEmbedded )
			vCollisionDirection = result_data.vResultDir;
		else
			vCollisionDirection = vTestPosition - result_data.vIntersection;
		VectorNormalize( vCollisionDirection );
		float fCollisionDistanceXY = VectorDistanceSquaredXY( result_data.vIntersection, vPositionInRoom );
		
		BOOL bGround = FALSE;		
		if ( result_data.pResultPoly->vNormal.z >= fWalkable )
			bGround = TRUE;
		float fMaxXY = fRadius * 0.5f;
		if ( fCollisionDistanceXY > ( fMaxXY * fMaxXY ) )
		{
			bGround = FALSE;
		}
		else
		{
			// calc reverse normal of poly we hit to edge of sphere
			VECTOR vEdge = -result_data.pResultPoly->vNormal * fRadius;
			vEdge += vPositionInRoom;
			// if that is close to where we landed/hit then we are on that surface, otherwise it is smashing into us
			VECTOR vDelta = vEdge - result_data.vIntersection;
			float fTotal = fabsf( vDelta.x ) + fabsf( vDelta.y ) + fabsf( vDelta.z );
			if ( fTotal < COLLIDE_EPSILON )
			{
				SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
			}
		}

		// landed
		if ( !UnitIsOnGround( unit ) )
		{
			if ( vCollisionDirection.z >= fWalkable )
			{
				// are you close to the center of our sphere and this is a decent surface?
				float fMaxXY = fRadius * 0.5f;
				if ( fCollisionDistanceXY < ( fMaxXY * fMaxXY ) )
				{
					SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
				}
				else
				{
					// calc reverse normal of poly we hit to edge of sphere
					VECTOR vEdge = -result_data.pResultPoly->vNormal * fRadius;
					vEdge += vPositionInRoom;
					// if that is close to where we landed/hit then we are on that surface, otherwise it is smashing into us
					VECTOR vDelta = vEdge - result_data.vIntersection;
					float fTotal = fabsf( vDelta.x ) + fabsf( vDelta.y ) + fabsf( vDelta.z );
					if ( fTotal < COLLIDE_EPSILON )
					{
						SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
					}
				}
			}
			// if we haven't been set down on the ground, make sure we are still moving downwards
			if ( !TESTBIT( pdwMoveResult, MOVERESULT_SETONGROUND ) )
			{
				float fResultZ = result_data.vResultDir.z * result_data.fResultDist;
				float fPartZ = UnitGetZVelocity( unit ) * 0.1f;
				if ( fResultZ > fPartZ )
				{
					SETBIT( pdwMoveResult, MOVERESULT_SETONGROUND );
				}
			}
		}
		// walking on ground
		else
		{
			// sure, i collided, but maybe it was a bit too far, or on a bad surface...
			//if ( !( vCollisionDirection.z >= fWalkable ) )
			//{
			//	SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
			//}
			//else
			{
				// did I go up?
				float delta_z = vPositionInRoom.z - vOriginalPosition.z;
				// no need to do anything if I'm just going upwards and the wall collision took care of me
				if ( delta_z > COLLIDE_EPSILON )
				{
					if ( result_data.pResultPoly->vNormal.z >= fWalkable )
						return FALSE;
					else
						SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
				}
				else
				{
					float fMaxXY = fRadius * 0.5f;
					if ( fCollisionDistanceXY > ( fMaxXY * fMaxXY ) )
					{
						// more than halfway through my collision radius.... let's see how far down it is...
						fTestDistance = MAX_UP_DELTA;
						MatrixMultiply( &vWorldPosition, &vPositionInRoom, &room->tWorldMatrix );						
						float fDistanceDown = LevelLineCollideLen( UnitGetGame( unit ), UnitGetLevel( unit ), vWorldPosition, vTestDirection, fTestDistance );
						if ( fDistanceDown >= MAX_UP_DELTA )
							SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
					}
				}
			}
		}

		//VECTOR vTravelDirection = vPositionInRoom - vOriginalPosition;
		//VectorNormalize( vTravelDirection );
		VECTOR vMovement = result_data.vResultDir;
		vMovement *= result_data.fResultDist;
		vPositionInRoom.z = vTestPosition.z + vMovement.z + COLLIDE_EPSILON;
	}
	else
	{
		// didn't hit anything and I was on ground?
		if ( UnitIsOnGround( unit ) )
		{
			SETBIT( pdwMoveResult, MOVERESULT_TAKEOFFGROUND );
		}
	}

	return bCollide;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_COLLIDE_GROUND_DATA
{
	ROOM*				room;
	const BOUNDING_BOX*	ptBoxInRoom;
	VECTOR*				pvPositionTest;
	VECTOR				vDirectionTest;
	float				fMinDist;
	BOOL				bHitGround;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sRoomCollideGroundCallbackForWalls(
	ROOM_CPOLY* poly,
	void* data)
{
	ROOM_COLLIDE_GROUND_DATA* Data = (ROOM_COLLIDE_GROUND_DATA*)data;
	const BOUNDING_BOX& tBoxInRoom = *(Data->ptBoxInRoom);

	if ((tBoxInRoom.vMin.fX <= poly->tBoundingBox.vMax.fX + EPSILON) &&
		(tBoxInRoom.vMax.fX >= poly->tBoundingBox.vMin.fX - EPSILON) &&
		(tBoxInRoom.vMin.fY <= poly->tBoundingBox.vMax.fY + EPSILON) &&
		(tBoxInRoom.vMax.fY >= poly->tBoundingBox.vMin.fY - EPSILON))
	{
		float fDist, fU, fV;
		BOOL bHit = RayIntersectTriangle(Data->pvPositionTest, &Data->vDirectionTest, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition, &fDist, &fU, &fV);
		if (bHit)
		{
			if (fDist < Data->fMinDist && fDist > COLLIDE_MIN_DIST)
			{
				//trace( "Hit ground. Floor normal = (%4.2f,%4.2f,%4.2f), fDist = %4.2f\n", poly->vNormal.fX, poly->vNormal.fY, poly->vNormal.fZ, fDist );
				//trace( "Hit ground at (%4.2f,%4.2f,%4.2f), fDist = %4.2f\n", Data->pvPositionTest->fX, Data->pvPositionTest->fY, Data->pvPositionTest->fZ + MAX_UP_DELTA, fDist );
				Data->bHitGround = TRUE;
				Data->fMinDist = fDist;
			}
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sRoomCollideGroundCallback(
	ROOM_CPOLY* poly,
	void* data)
{
	ROOM_COLLIDE_GROUND_DATA* Data = (ROOM_COLLIDE_GROUND_DATA*)data;
	const BOUNDING_BOX& tBoxInRoom = *(Data->ptBoxInRoom);

	if ((poly->vNormal.fZ >= CollisionWalkableZNormal()) && 
		(tBoxInRoom.vMin.fX <= poly->tBoundingBox.vMax.fX + EPSILON) &&
		(tBoxInRoom.vMax.fX >= poly->tBoundingBox.vMin.fX - EPSILON) &&
		(tBoxInRoom.vMin.fY <= poly->tBoundingBox.vMax.fY + EPSILON) &&
		(tBoxInRoom.vMax.fY >= poly->tBoundingBox.vMin.fY - EPSILON))
	{
		float fDist, fU, fV;
		BOOL bHit = RayIntersectTriangle(Data->pvPositionTest, &Data->vDirectionTest, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition, &fDist, &fU, &fV);
		if (bHit)
		{
			if (fDist < Data->fMinDist && fDist > COLLIDE_MIN_DIST)
			{
				//trace( "Hit ground. Floor normal = (%4.2f,%4.2f,%4.2f), fDist = %4.2f\n", poly->vNormal.fX, poly->vNormal.fY, poly->vNormal.fZ, fDist );
				//trace( "Hit ground at (%4.2f,%4.2f,%4.2f), fDist = %4.2f\n", Data->pvPositionTest->fX, Data->pvPositionTest->fY, Data->pvPositionTest->fZ + MAX_UP_DELTA, fDist );
				Data->bHitGround = TRUE;
				Data->fMinDist = fDist;
			}
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline void sRoomCollideGroundRepositionZ(
	UNIT* unit,
	const VECTOR& vPositionPrevInRoom,
	VECTOR& vPositionNewInRoom,
	float fMinDist,
	DWORD dwMoveFlags,
	DWORD* pdwMoveResult)
{
	// either set to falling or position z to ground level
	BOOL bSetOnGround = FALSE;
	BOOL bTakeOffGround = FALSE;
	
	float fUpDelta = MAX_UP_DELTA;
	if ( unit && !UnitIsOnGround( unit ) )
		fUpDelta = PLAYER_HEIGHT;
		
	if (!(dwMoveFlags & MOVEMASK_FORCEGROUND))
	{
		// don't put me on the floor, unless I am going straight or downwards
		if (unit)
		{
			if (UnitGetZVelocity( unit ) <= 0.0f)
			{
				float fDelta;
				// what direction am I heading?
				if ( UnitIsOnGround( unit ))
				{
					fDelta = fMinDist - fUpDelta - 0.005f;
				}
				else
				{
					fDelta = fMinDist + ( vPositionNewInRoom.fZ - vPositionPrevInRoom.fZ ) - fUpDelta - 0.05f;
				}
				if (fDelta < 0.0f)
				{
					vPositionNewInRoom.fZ -= fDelta;	// make certain we're above ground!
					//trace( "Up - PrevZ = %4.2f, NewZ = %4.2f, fZDir = %4.2f, fDelta = %4.2f\n", vPositionPrevInRoom.fZ, vPositionNewInRoom.fZ, fZDir, fDelta );
					bSetOnGround = TRUE;
				}
				else
				{
					// was I on the ground and now I am falling?
					if (UnitIsOnGround( unit ))
					{
						// is it a small fall?
						if (fDelta < fUpDelta)
						{
							vPositionNewInRoom.fZ -= fDelta;
							//trace( "Down - PrevZ = %4.2f, NewZ = %4.2f, fZDir = %4.2f, fDelta = %4.2f\n", vPositionPrevInRoom.fZ, vPositionNewInRoom.fZ, fZDir, fDelta );
							bSetOnGround = TRUE;
						}
						else
						{
							bTakeOffGround = TRUE;
						}
					}
				}
			}
			// I'm on the way up, but I may be going into a ramp
			else
			{
				if ( fMinDist < fUpDelta )
					vPositionNewInRoom.fZ -= fMinDist - fUpDelta - 0.005f;
			}
		}
	}
	else
	{
		vPositionNewInRoom.fZ -= fMinDist - fUpDelta - 0.005f;	// make certain we're above ground!
		if (unit && !UnitIsOnGround( unit ))
		{
			bSetOnGround = TRUE;
		}
	}
	if (bSetOnGround)
	{
		SETBIT(pdwMoveResult, MOVERESULT_SETONGROUND);
	}
	else if (bTakeOffGround)
	{
		SETBIT(pdwMoveResult, MOVERESULT_TAKEOFFGROUND);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideCheckGroundFlags(
	const DWORD dwMoveFlags,
	const DWORD* pdwMoveResult)
{
	return TESTBIT(pdwMoveResult, MOVERESULT_SETONGROUND) && (dwMoveFlags & MOVEMASK_FORCEGROUND);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL old_sRoomCollideGround(
	GAME* game,
	UNIT* unit,
	ROOM*& room,
	int nCollisionType, 
	const VECTOR& vPositionPrev,
	VECTOR& vPositionTest,
	VECTOR& vPositionPrevInRoom,
	VECTOR& vPositionNewInRoom,
	BOUNDING_BOX& tBoxInRoom,
	DWORD dwMoveFlags, 
	float fRadius,
	float fHeight,
	DWORD* pdwMoveResult)
{
	if (unit && (UnitIsOnGround( unit ) || sRoomCollideCheckGroundFlags(dwMoveFlags, pdwMoveResult)))
	{
		vPositionTest.fZ = vPositionNewInRoom.fZ + MAX_UP_DELTA;
	}
	else
	{
		vPositionTest.fZ = vPositionPrevInRoom.fZ + MAX_UP_DELTA;
	}
	if (!BoundingBoxTestPoint(&room->pDefinition->tBoundingBox, &vPositionTest))
	{
		VECTOR vWorldPosition;
		MatrixMultiply(&vWorldPosition, &vPositionTest, &room->tWorldMatrix);
		ROOM* room_to_check = RoomGetFromPosition(game, room, &vWorldPosition);
		if (!room_to_check)
		{
			room_to_check = room;
		}
		if (room_to_check != room)
		{	// switch rooms
			MatrixMultiply(&vPositionTest, &vWorldPosition, &room_to_check->tWorldMatrixInverse);
			MatrixMultiply(&vWorldPosition, &vPositionNewInRoom, &room->tWorldMatrix);
			MatrixMultiply(&vPositionNewInRoom, &vWorldPosition, &room_to_check->tWorldMatrixInverse);
			MatrixMultiply(&vPositionPrevInRoom, &vPositionPrev, &room_to_check->tWorldMatrixInverse);
			sMakeBoundingBox(&tBoxInRoom, vPositionNewInRoom, fRadius, fHeight);
			room = room_to_check;
		}
	}

	ROOM_COLLIDE_GROUND_DATA data;
	data.room = room;
	data.ptBoxInRoom = &tBoxInRoom;
	data.pvPositionTest = &vPositionTest;
	data.vDirectionTest = VECTOR(0.0f, 0.0f, -1.0f);
	data.bHitGround = TRUE;

	VECTOR vDelta = vPositionNewInRoom - vPositionPrevInRoom;
	float fDist = VectorLength( vDelta );
	if ( fDist == 0.0f )
		return FALSE;

	float fStep = 0.0f;
	ASSERT_RETFALSE(fRadius >= 0.1);
	float fHalfRadius = fRadius * 0.5f;
	float fStepAdd = fHalfRadius / fDist;

	if ( fStepAdd < MIN_STEP_ADD )
	{
		fStepAdd = MIN_STEP_ADD;
		fHalfRadius = fStepAdd * fDist;
		if ( unit )
		{
			trace( "Unit class = %i, fRadius = %e, fDist = %e\n ", UnitGetClass( unit ), fRadius, fDist );
		}
	}

	while ( ( fStep < 1.0f ) && data.bHitGround )
	{
		if ( fDist > fHalfRadius )
		{
			fStep += fStepAdd;
			fDist -= fHalfRadius;
		}
		else
		{
			fStep = 1.0f;
		}
		vPositionTest.fX = vPositionPrevInRoom.fX + ( vDelta.fX * fStep );
		vPositionTest.fY = vPositionPrevInRoom.fY + ( vDelta.fY * fStep );
		if (unit && (UnitIsOnGround( unit ) || sRoomCollideCheckGroundFlags(dwMoveFlags, pdwMoveResult)))
		{
			vPositionTest.fZ = vPositionNewInRoom.fZ + MAX_UP_DELTA;
		}
		else
		{
			// use highest point
			if ( vPositionNewInRoom.fZ > vPositionPrevInRoom.fZ )
				vPositionTest.fZ = vPositionNewInRoom.fZ + PLAYER_HEIGHT;
			else
				vPositionTest.fZ = vPositionPrevInRoom.fZ + PLAYER_HEIGHT;
		}

		data.fMinDist = ((dwMoveFlags & MOVEMASK_FLY) != 0) ? MAX_UP_DELTA : 1000.0f;		
		data.bHitGround = FALSE;
		RoomIterateCollisionBox(room, tBoxInRoom.vMin.fX, tBoxInRoom.vMin.fY, tBoxInRoom.vMax.fX, tBoxInRoom.vMax.fY, nCollisionType, sRoomCollideGroundCallback, (void*)&data);

		if (!data.bHitGround)
		{
			RoomIterateCollisionBox(room, tBoxInRoom.vMin.fX, tBoxInRoom.vMin.fY, tBoxInRoom.vMax.fX, tBoxInRoom.vMax.fY, COLLISION_WALL, sRoomCollideGroundCallbackForWalls, (void*)&data);
			if (!data.bHitGround)
			{
				//trace( "Didn't hit ground at (%4.2f,%4.2f,%4.2f)\n", vPositionTest.fX, vPositionTest.fY, vPositionTest.fZ );
				return FALSE;
			}
		}

		sRoomCollideGroundRepositionZ(unit, vPositionPrevInRoom, vPositionNewInRoom, data.fMinDist, dwMoveFlags, pdwMoveResult);

	}
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_COLLIDE_CEILING_DATA
{
	ROOM*				room;
	const BOUNDING_BOX*	ptBoxInRoom;
	VECTOR*				pvPositionTest;
	VECTOR				vDirectionTest;
	float				fMinDist;
	BOOL				bHitCeiling;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sRoomCollideCeilingCallback(
	ROOM_CPOLY* poly,
	void* data)
{
	ROOM_COLLIDE_CEILING_DATA* Data = (ROOM_COLLIDE_CEILING_DATA*)data;
	const BOUNDING_BOX& tBoxInRoom = *(Data->ptBoxInRoom);

	if ((poly->vNormal.fZ <= 0.0f) &&
		(tBoxInRoom.vMin.fX <= poly->tBoundingBox.vMax.fX) &&
		(tBoxInRoom.vMax.fX >= poly->tBoundingBox.vMin.fX) &&
		(tBoxInRoom.vMin.fY <= poly->tBoundingBox.vMax.fY) &&
		(tBoxInRoom.vMax.fY >= poly->tBoundingBox.vMin.fY))
	{
		float fDist, fU, fV;
		BOOL bHit = RayIntersectTriangle(Data->pvPositionTest, &Data->vDirectionTest, poly->pPt1->vPosition, poly->pPt2->vPosition, poly->pPt3->vPosition, &fDist, &fU, &fV);
		if (bHit)
		{
			if (fDist < Data->fMinDist && fDist > 0.0f)
			{
				Data->bHitCeiling = TRUE;
				Data->fMinDist = fDist;
			}
		}
	}
	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideCeiling(
	ROOM* room,
	VECTOR& vPositionTest,
	BOUNDING_BOX& tBoxInRoom,
	float& fMinDist)
{
	ROOM_COLLIDE_CEILING_DATA data;
	data.room = room;
	data.ptBoxInRoom = &tBoxInRoom;
	data.pvPositionTest = &vPositionTest;
	data.vDirectionTest = VECTOR(0.0f, 0.0f, 1.0f);
	data.fMinDist = fMinDist;
	data.bHitCeiling = FALSE;

	RoomIterateCollisionBox(room, tBoxInRoom.vMin.fX, tBoxInRoom.vMin.fY, tBoxInRoom.vMax.fX, tBoxInRoom.vMax.fY, COLLISION_CEILING, sRoomCollideCeilingCallback, (void*)&data);

	fMinDist = data.fMinDist;
	return data.bHitCeiling;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollidePropGround(
	FP_COLLISION_ITERATE_CALLBACK* fpCallback,
	UNIT* unit,
	ROOM* room,
	PROP_NODE* prop,
	const VECTOR & vPositionPrev,
	VECTOR & vPositionTest,
	VECTOR & vPositionPrevInRoom,
	VECTOR & vPositionNewInRoom,
	BOUNDING_BOX& tBoxInRoom,
	DWORD dwMoveFlags,
	float fRadius,
	float fHeight,
	DWORD* pdwMoveResult,
	float* pfMinDist)
{
	// check if we collide in prop-space
	VECTOR vPropPosition = VECTOR( prop->m_matrixPropToRoom._41, prop->m_matrixPropToRoom._42, prop->m_matrixPropToRoom._43 );
	float fDistSq = VectorDistanceSquared( vPositionTest, vPropPosition );
	float fMin = prop->m_pDefinition->fRoomCRadius + fRadius;
	if ( fDistSq > fMin * fMin )
	{
		return FALSE;
	}

	VECTOR vPositionTestProp = vPositionTest;
	if (unit && (UnitIsOnGround( unit ) || sRoomCollideCheckGroundFlags(dwMoveFlags, pdwMoveResult)))
	{
		vPositionTestProp.fZ = vPositionNewInRoom.fZ + MAX_UP_DELTA;
	}
	else
	{
		// use highest point
		if ( vPositionNewInRoom.fZ > vPositionPrevInRoom.fZ )
			vPositionTestProp.fZ = vPositionNewInRoom.fZ + PLAYER_HEIGHT;
		else
			vPositionTestProp.fZ = vPositionPrevInRoom.fZ + PLAYER_HEIGHT;
	}

	BOOL bHitGround = FALSE;
	float fMinDist = ((dwMoveFlags & MOVEMASK_FLY) != 0) ? MAX_UP_DELTA : 1000.0f;

	ROOM_COLLIDE_GROUND_DATA data;
	data.room = room;
	data.ptBoxInRoom = &tBoxInRoom;
	data.pvPositionTest = &vPositionTestProp;
	data.vDirectionTest = VECTOR(0.0f, 0.0f, -1.0f);
	data.fMinDist = fMinDist;
	data.bHitGround = FALSE;

	if ( ( prop->pCPoints == NULL ) || ( prop->pCPolys == NULL ) )
	{
		sTranslateProp( prop, room );
	}

	ROOM_CPOLY* cpoly = prop->pCPolys;

	for (int ii = 0; ii < prop->m_pDefinition->nRoomCPolyCount; ii++, cpoly++)
	{
		if (fpCallback(cpoly, &data))
		{
			break;
		}
		if (data.bHitGround)
		{
			*pfMinDist = data.fMinDist;
			bHitGround = TRUE;
		}
	}

	return bHitGround;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline BOOL sRoomCollideGroundProps(
	GAME* game,
	UNIT* unit,
	ROOM*& room,
	const VECTOR & vPositionPrev,
	VECTOR & vPositionTest,
	VECTOR & vPositionPrevInRoom,
	VECTOR & vPositionNewInRoom,
	BOUNDING_BOX & tBoxInRoom,
	DWORD dwMoveFlags, 
	float fRadius,
	float fHeight,
	DWORD* pdwMoveResult)
{
	BOOL bHitGround = FALSE;

	PROP_NODE* prop = room->pProps;
	while (prop)
	{

		float fMinDist = 0.0f;
		if (sRoomCollidePropGround(sRoomCollideGroundCallback, unit, room, prop, vPositionPrev, vPositionTest, vPositionPrevInRoom, vPositionNewInRoom, tBoxInRoom, dwMoveFlags, fRadius, fHeight, pdwMoveResult, &fMinDist))
		{
			bHitGround = TRUE;
			sRoomCollideGroundRepositionZ(unit, vPositionPrevInRoom, vPositionNewInRoom, fMinDist, dwMoveFlags, pdwMoveResult);
		}
		prop = prop->m_pNext;
	}

	return bHitGround;
}

//-------------------------------------------------------------------------------------------------
// unit can be NULL!!!
//-------------------------------------------------------------------------------------------------
static BOOL sRoomCollide(
	GAME* game, 
	ROOM* room,
	UNIT* unit,
	const VECTOR& vPositionPrev,
	VECTOR* pvPositionNew,
	BOOL bMoving,
	float fRadius,
	float fHeight,
	DWORD dwMoveFlags,
	DWORD* pdwMoveResult)
{
	ASSERT_RETFALSE(game && room);
	ASSERT_RETFALSE(room->pDefinition);

	// move the input position into the room's model coords
	VECTOR vPositionNewInRoom;
	MatrixMultiply(&vPositionNewInRoom, pvPositionNew, &room->tWorldMatrixInverse);
	VECTOR vPositionPrevInRoom;
	MatrixMultiply(&vPositionPrevInRoom, &vPositionPrev, &room->tWorldMatrixInverse);

	// am I going anywhere?
	VECTOR vMoveDelta = vPositionNewInRoom - vPositionPrevInRoom;
	if ( VectorIsZeroEpsilon( vMoveDelta ) )
		return FALSE;

	VECTOR vPositionTest = vPositionNewInRoom;
	BOOL bCollide = FALSE;

	if ( !( dwMoveFlags & MOVEMASK_FLOOR_ONLY ) &&
		 !( dwMoveFlags & MOVEMASK_NOT_WALLS ) )
	{
		if(!unit || (unit && !UnitTestFlag(unit, UNITFLAG_WALLWALK)))
		{
			// if moving x,y or in the air, then check walls, etc. Otherwise just check floor
			if ( bMoving || ( unit && !UnitIsOnGround(unit) ) )
			{
				bCollide = sRoomCollideWalls(unit, room, vPositionPrevInRoom, vPositionNewInRoom, fRadius, fHeight);
				if (bCollide)
				{
					SETBIT(pdwMoveResult, MOVERESULT_COLLIDE);
				}
			}
		}
	}

	if( AppIsHellgate() )
	{

		if ( unit && UnitIsA( unit, UNITTYPE_PLAYER ) )
		{
			sRoomCollideGround( unit, room, vPositionPrevInRoom, vPositionNewInRoom, fRadius, fHeight, pdwMoveResult );
		}
		else
		{
			// old method of checking the ground
			BOUNDING_BOX tBoxInRoom;
			sMakeBoundingBox(&tBoxInRoom, vPositionNewInRoom, fRadius, fHeight);
			BOUNDING_BOX temp;
			sMakeBoundingBox( &temp, vPositionPrevInRoom, fRadius, fHeight );
			BoundingBoxExpandByPoint( &tBoxInRoom, &temp.vMin );
			BoundingBoxExpandByPoint( &tBoxInRoom, &temp.vMax );

			VECTOR vPositionNewTemporary = vPositionNewInRoom;
			if (!sRoomCollideGroundProps(game, unit, room, vPositionPrev, vPositionTest, vPositionPrevInRoom, vPositionNewInRoom, tBoxInRoom, dwMoveFlags, fRadius, fHeight, pdwMoveResult))
			{
				old_sRoomCollideGround(game, unit, room, COLLISION_FLOOR, vPositionPrev, vPositionTest, vPositionPrevInRoom, vPositionNewInRoom, tBoxInRoom, dwMoveFlags, fRadius, fHeight, pdwMoveResult);
			}
			else
			{
				old_sRoomCollideGround(game, unit, room, COLLISION_FLOOR, vPositionPrev, vPositionTest, vPositionPrevInRoom, vPositionNewTemporary, tBoxInRoom, dwMoveFlags, fRadius, fHeight, pdwMoveResult);
				if(vPositionNewTemporary.fZ >= vPositionNewInRoom.fZ)
				{
					vPositionNewInRoom = vPositionNewTemporary;
				}
			}
		}

		// check ceiling
		if ((dwMoveFlags & MOVEMASK_FLY) || (unit && !UnitIsOnGround( unit ) && (UnitGetZVelocity(unit) > 0.0f)))
		{
			BOUNDING_BOX tBoxInRoom;
			sMakeBoundingBox(&tBoxInRoom, vPositionNewInRoom, fRadius, fHeight);
			BOUNDING_BOX temp;
			sMakeBoundingBox( &temp, vPositionPrevInRoom, fRadius, fHeight );
			BoundingBoxExpandByPoint( &tBoxInRoom, &temp.vMin );
			BoundingBoxExpandByPoint( &tBoxInRoom, &temp.vMax );

			vPositionTest = vPositionNewInRoom;
			float fMinDist = fHeight;
			BOOL bHitCeiling = sRoomCollideCeiling(room, vPositionTest, tBoxInRoom, fMinDist);
			if (bHitCeiling)
			{
				SETBIT(pdwMoveResult, MOVERESULT_COLLIDE_CEILING);
				vPositionNewInRoom.fZ += fMinDist - (fHeight + 0.05f);
				if(!(dwMoveFlags & MOVEMASK_NOT_REAL_STEP))
				{
					if (unit && !UnitIsOnGround( unit ))
					{
						float fNewZVelocity = 0.0f;
						if(UnitTestFlag(unit, UNITFLAG_BOUNCE_OFF_CEILING))
						{
							fNewZVelocity = -0.1f;
						}
						UnitSetZVelocity( unit, fNewZVelocity );
					}
				}
			}
		}

	}
	MatrixMultiply(pvPositionNew, &vPositionNewInRoom, &room->tWorldMatrix);

	return bCollide;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sCollideUnit( const UNIT * monster, const VECTOR * pvPositionPrev, VECTOR * pvPositionNew, float fRadius )
{
	if (AppIsTugboat())
	{
		VECTOR Closest;
		ClosestPointOnLine( *pvPositionPrev, *pvPositionNew, monster->vPosition, Closest );

		VECTOR delta;
		delta.fX = Closest.fX - monster->vPosition.fX;
		delta.fY = Closest.fY - monster->vPosition.fY;

		float distance = ( delta.fX * delta.fX ) + ( delta.fY * delta.fY );

		float total_radius = ( fRadius + UnitGetCollisionRadius( monster ) );// * 0.6f;
		float total_radius_squared = total_radius * total_radius;


		if ( distance > ( total_radius_squared ) )
			return FALSE;

		//delta = monster->vPosition - Closest;
		VECTOR Impact = monster->vPosition - *pvPositionPrev;
		if( VerifyFloat( delta.fX, 1 ) != delta.fX &&
			VerifyFloat( delta.fY, 1 ) != delta.fY )
		{
			delta = -Impact;
		}
		Impact.fZ = 0;
		delta.fZ = 0;
		VectorNormalize( delta );
		VectorNormalize( Impact );
		pvPositionNew->fX = monster->vPosition.fX - Impact.fX * ( UnitGetCollisionRadius( monster ) );
		pvPositionNew->fY = monster->vPosition.fY - Impact.fY * ( UnitGetCollisionRadius( monster ) );
		pvPositionNew->fX += delta.fX * ( fRadius + .05f );
		pvPositionNew->fY += delta.fY * ( fRadius + .05f );
		delta = *pvPositionNew - monster->vPosition;
		delta.fZ = 0;
		VectorNormalize( delta );
		delta *= ( total_radius + .01f );
		pvPositionNew->fX = monster->vPosition.fX + delta.fX;
		pvPositionNew->fY = monster->vPosition.fY + delta.fY;
	}

	if (AppIsHellgate())
	{
		VECTOR delta;
		delta.fX = pvPositionNew->fX - monster->vPosition.fX;
		delta.fY = pvPositionNew->fY - monster->vPosition.fY;

		float distance = ( delta.fX * delta.fX ) + ( delta.fY * delta.fY );

		float total_radius = ( fRadius + UnitGetCollisionRadius( monster ) ) * 0.7f;

		if ( distance > ( total_radius * total_radius ) )
			return FALSE;

		// we have collided, back up off the unit
		float length = sqrtf( distance );
		float bump = ( total_radius + 0.05f ) - length;
		if ( ( bump > 0 ) && ( length > 0 ) )
		{
			bump /= length;
			pvPositionNew->fX += delta.fX * bump;
			pvPositionNew->fY += delta.fY * bump;
		}
		else
		{
			*pvPositionNew = *pvPositionPrev;
		}
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef DRB_NEW_COLLIDE_ALL
static void sCollideAllUnitTest( COLLIDE_ALL_DATA & tData )
{
	ASSERT_RETURN( AppIsHellgate() );

	if ( tData.num_units <= 0 )
		return;

	VECTOR test_point;
	test_point.x = tData.start.x;
	test_point.y = tData.start.y;
	test_point.z = 0.0f;
	VECTOR test_dir;
	test_dir.x = tData.end.x - tData.start.x;
	test_dir.y = tData.end.y - tData.start.y;
	test_dir.z = 0.0f;
	float travel_distance_squared = ( test_dir.x * test_dir.x ) + ( test_dir.y * test_dir.y );
	float travel_distance = sqrtf( travel_distance_squared );
	test_dir.x /= travel_distance;
	test_dir.y /= travel_distance;

	for ( int i = 0; i < tData.num_units; i++ )
	{
		// distance between us
		float dist_sq_xy = VectorDistanceSquaredXY( tData.start, tData.unit_list[i]->vPosition );
		// take off our radius ( the 70% multiplier has been in forever... it was strictly for good looks )
		float total_radius = ( tData.radius + UnitGetCollisionRadius( tData.unit_list[i] ) ) * 0.7f;
		float total_radius_squared = total_radius * total_radius;
		dist_sq_xy -= total_radius_squared;

		// is there any chance of collision?
		if ( dist_sq_xy < travel_distance_squared )
		{
			if ( dist_sq_xy < total_radius_squared )
			{
				// embedded
				tData.result_distance = 0.0f;
				tData.result_intersection = tData.unit_list[i]->vPosition;
				tData.result_type = COLLIDE_ALL_RESULT_UNIT;
			}
			else
			{
				// check for a collision
				VECTOR center;
				center.x = tData.unit_list[i]->vPosition.x;
				center.y = tData.unit_list[i]->vPosition.y;
				center.z = 0.0f;
				float distance = intersect_sphere( center, total_radius, test_point, test_dir );
				if ( ( distance >= 0.0f ) && ( distance < tData.result_distance ) )
				{
					tData.result_distance = distance;
					tData.result_intersection = tData.unit_list[i]->vPosition;
					tData.result_type = COLLIDE_ALL_RESULT_UNIT;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sCollideAllRoomsTest( COLLIDE_ALL_DATA & tData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sHandleCollideAllUnits( COLLIDE_ALL_DATA & tData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sHandleCollideAllGeometry( COLLIDE_ALL_DATA & tData )
{
}

#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define UNIT_TEST_BUFFER	0.3f
#define CLOSE_IS_THIS_XY	0.0f
#define CLOSE_IS_THIS_Z		2.0f


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RoomCollide( 
	UNIT * unit, 
	const VECTOR & vPositionPrev, 
	const VECTOR & vVelocity, 
	VECTOR * pvPositionNew, 
	float fRadius, 
	float fHeight,
	DWORD dwMoveFlags,
	COLLIDE_RESULT * result)
{
	ASSERT_RETURN(result);

	result->flags = 0;
	result->unit = NULL;

	float fSpeedSquared = VectorLengthSquared(vVelocity);
	float fSpeed = sqrt(fSpeedSquared);

	ASSERTX(fRadius > 0.0f, "Trying to collide without a collision radius.");

	// -- Find rooms to test --
	// Keep track of what rooms you tested as you go
	// use bounding boxes since they are fast and simple
	int nRoomsCloseCount = 0;
	ROOM * ppRoomsClose[ MAX_ROOMS_CONNECTED ];
	BOUNDING_BOX tFatUnitBox;
	sMakeBoundingBox( & tFatUnitBox, vPositionPrev, fRadius + fSpeed + UNIT_TEST_BUFFER, fHeight );
	BoundingBoxExpandByPoint( & tFatUnitBox, pvPositionNew );
	ROOM * pCenterRoom = UnitGetRoom( unit );
	ASSERT_RETURN(pCenterRoom);

	int close_count = 0;
	UNIT * close_monsters[ MAX_CLOSE_UNITS ];

	float fCloseRange = fRadius;
	if ( dwMoveFlags & MOVEMASK_USE_DESIRED_MELEE_RANGE )
	{
		const UNIT_DATA * pUnitData = unit ? UnitGetData( unit ) : NULL;
		fCloseRange += pUnitData ? pUnitData->fMeleeRangeDesired : 0.0f;
	} else {
		fCloseRange += CLOSE_IS_THIS_XY;
	}

	int adjacentRoomCount = RoomGetAdjacentRoomCount(pCenterRoom);
	for (int ii = -1; ii < adjacentRoomCount; ++ii)
	{
		ROOM * pRoomToTest;
		if (ii == -1)
		{
			pRoomToTest = pCenterRoom;
		}
		else
		{
			pRoomToTest = RoomGetAdjacentRoom(pCenterRoom, ii);
		}

		if (! pRoomToTest)
			continue;

		ASSERT( pRoomToTest->pHull );
		if ( pRoomToTest->pHull && ConvexHullAABBCollide( pRoomToTest->pHull, & tFatUnitBox ) )
		{
			BOOL bAdd = TRUE;
			for ( int i = 0; bAdd && ( i < nRoomsCloseCount ); i++ )
			{
				if ( ppRoomsClose[i] == pRoomToTest )
					bAdd = FALSE;
			}
			if ( bAdd )
			{
				ppRoomsClose[ nRoomsCloseCount ] = pRoomToTest;
				nRoomsCloseCount++;
				ASSERT ( nRoomsCloseCount <= MAX_ROOMS_CONNECTED );
			}
		}

		if ( dwMoveFlags & MOVEMASK_SOLID_MONSTERS )
		{
			for ( int target_type = TARGET_GOOD; target_type <= TARGET_DEAD; target_type++ )
			{
				for ( UNIT * monster = pRoomToTest->tUnitList.ppFirst[target_type]; monster; monster = monster->tRoomNode.pNext )
				{	
					if( target_type == TARGET_DEAD &&
						!UnitDataTestFlag(monster->pUnitData, UNIT_DATA_FLAG_COLLIDE_WHEN_DEAD))
					{
						continue;
					}
					if ( UnitGetGenus( monster ) == GENUS_MONSTER ||
							UnitIsA( monster, UNITTYPE_BLOCKING ) )
					{
						if ( monster == unit )
							continue;

						if ((UnitGetGenus( monster ) == GENUS_MONSTER) && (UnitGetTargetType(unit) == TARGET_GOOD) && ((monster->dwTestCollide & TARGETMASK_GOOD) == 0 ))
							continue;

						float close_delta = fCloseRange + UnitGetCollisionRadius( monster );
						float delta = fabs( pvPositionNew->x - monster->vPosition.x );
						if ( delta >= close_delta )
							continue;
						delta = fabs( pvPositionNew->y - monster->vPosition.y );
						if ( delta >= close_delta )
							continue;

						if(dwMoveFlags & MOVEMASK_ONLY_ONGROUND_SOLID &&
							!UnitIsOnGround(monster))
							continue;

						if (UnitIsA(monster, UNITTYPE_NOTARGET))
							continue;

						if ( UnitGetStat(monster, STATS_DONT_TARGET) )
							continue;

						// don't collide with Tugboat Doors
						if( ObjectIsDoor( monster ) )
						{
							continue;
						}

						if(PetIsPet(monster) && TestFriendly(UnitGetGame(unit), monster, unit))
							continue;

						delta = fabs( pvPositionNew->z - monster->vPosition.z );
						if ( delta < CLOSE_IS_THIS_Z )
						{
							// this unit is close enough to worry about
							if ( close_count < MAX_CLOSE_UNITS )
							{
								close_monsters[ close_count ] = monster;
								close_count++;
							}
						}
					}
				}
			}
		}
	}

#ifdef DRB_NEW_COLLIDE_ALL
	if ( AppIsHellgate() )
	{
		COLLIDE_ALL_DATA tData;
		tData.unit = unit;
		tData.radius = fRadius;
		tData.height = fHeight;
		tData.start = vPositionPrev;
		tData.end = *pvPositionNew;
		tData.velocity = *pvPositionNew - vPositionPrev;
		tData.distance = VectorLength( tData.velocity );
		tData.original_distance = tData.distance;
		tData.num_rooms = nRoomsCloseCount;
		for ( int i = 0; i < nRoomsCloseCount; i++ )
			tData.room_list[i] = ppRoomsClose[i];
		tData.num_units = close_count;
		for ( int i = 0; i < close_count; i++ )
			tData.unit_list[i] = close_monsters[i];

		int count = 0;
		while ( tData.distance > COLLIDE_EPSILON )
		{
			tData.result_type = COLLIDE_ALL_RESULT_NONE;
			tData.result_distance = tData.distance;

			// find closest collision
			sCollideAllUnitTest( tData );
			sCollideAllRoomsTest( tData );

			// handle collision
			if ( tData.result_type == COLLIDE_ALL_RESULT_NONE )
			{
				tData.end = tData.start + tData.velocity;
			}
			else if ( tData.result_type == COLLIDE_ALL_RESULT_UNIT )
			{
				sHandleCollideAllUnits( tData );
			}
			else
			{
				sHandleCollideAllGeometry( tData );
			}

			// calculate new distance left, etc...
			count++;
			if ( count > 5 )
			{
				tData.end = tData.start;	// abort
			}
			tData.velocity = tData.end - tData.start;
			tData.distance = VectorLength( tData.velocity );
		}
		*pvPositionNew = tData.end;
	}
#else
	{
		const int MAX_BOUNCE_PASSES = 2;
		BOOL bMoving = ABS(fSpeed) > EPSILON;

		// -- Bounce off of room collision --
		BOOL bBounceRoom = TRUE;
		BOOL bBounceUnit = TRUE;
		int nBounce;
		VECTOR vPreviousTemp = vPositionPrev;

		for ( nBounce = 0; ( nBounce <= MAX_BOUNCE_PASSES ) && ( bBounceRoom || bBounceUnit ); nBounce++ )
		{
			// if necessary, check the units again - we might have ping-ponged back into them
			if ( !nBounce || bBounceRoom || bBounceUnit )
			{
				bBounceUnit = FALSE;
				for ( int ii = 0; ii < close_count; ii++ )
				{
					BOOL bCollide = sCollideUnit( close_monsters[ ii ], &vPreviousTemp, pvPositionNew, fCloseRange );
					if ( bCollide )
						result->unit = close_monsters[ii];

					bBounceUnit |= bCollide;

					if( bCollide )
					{
						SETBIT(result->flags, MOVERESULT_COLLIDE_UNIT);
					}
				}
			}
			// if necessary, check the rooms again - we might have ping-ponged back into them
			if ( !nBounce || bBounceRoom || bBounceUnit )
			{
				bBounceRoom = FALSE;
				if ( !( dwMoveFlags & MOVEMASK_NO_GEOMETRY ) )
				{
					for ( int ii = 0; ii < nRoomsCloseCount; ii ++ )
					{
						BOOL bCollide = sRoomCollide(UnitGetGame(unit), ppRoomsClose[ ii ], unit, vPreviousTemp, pvPositionNew, bMoving, fRadius, fHeight, dwMoveFlags, &result->flags);
						bBounceRoom |= bCollide;
					}
				}
			}
			if ( AppIsHellgate() && ( bBounceRoom || bBounceUnit ) )
			{
				vPreviousTemp = *pvPositionNew;
			}
		}

		if ( AppIsHellgate()  && ( nBounce > MAX_BOUNCE_PASSES ) && ( bBounceRoom || bBounceUnit ) )
		{
			// good god... just stop it if I can
			if ( unit && UnitIsOnGround( unit ) )
			{
				*pvPositionNew = vPositionPrev;
			}
			else
			{
				pvPositionNew->fX = vPositionPrev.fX;
				pvPositionNew->fY = vPositionPrev.fY;
			}
		}
	}
#endif

	// -- Update the unit's room --
	// you have to do this afterwards in case a player moved in and out rooms during collision testing
	if ((dwMoveFlags & MOVEMASK_TEST_ONLY) == 0)
	{
		// set the new position
		UnitSetPosition(unit, *pvPositionNew);
		VECTOR vPositionNewLifted = *pvPositionNew;
		vPositionNewLifted.fZ += 0.05f;// lift the position slightly so that you don't fall through the floor
		for (int ii = 0; ii < nRoomsCloseCount; ii++)
		{
			if ( ConvexHullPointTest( ppRoomsClose[ii]->pHull, & vPositionNewLifted ) )
			{
				UnitUpdateRoom(unit, ppRoomsClose[ii], TRUE);
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomCollide( 
	UNIT* pUnit, 
	SIMPLE_DYNAMIC_ARRAY< ROOM* >*	PreSortedRooms,
	const VECTOR& vPositionPrev, 
	const VECTOR& vVelocity, 
	VECTOR* pvPositionNew, 
	float fRadius, 
	float fHeight,
	DWORD dwMoveFlags,
	COLLIDE_RESULT * pResult)
{
	const int MAX_BOUNCE_PASSES = 1;
	SIMPLE_DYNAMIC_ARRAY< ROOM* >*	ActiveRooms = PreSortedRooms;
	SIMPLE_DYNAMIC_ARRAY< ROOM* >	Rooms;
	ArrayInit(Rooms, GameGetMemPool(UnitGetGame(pUnit)), 2);
	if( !ActiveRooms )
	{
		ActiveRooms = &Rooms;
	}
	ASSERT_RETURN( pResult );

	pResult->flags = 0;
	pResult->unit = NULL;

	float fSpeedSquared = VectorLengthSquared(vVelocity);
	float fSpeed = sqrt(fSpeedSquared);
	BOOL bMoving = ABS(fSpeed) > EPSILON;

	VECTOR vTargetPosition = *pvPositionNew;

	ASSERTX(fRadius > 0.0f, "Trying to collide without a collision radius.");

	// -- Find rooms to test --
	// Keep track of what rooms you tested as you go
	// use bounding boxes since they are fast and simple
	int nRoomsCloseCount = 0;
	BOUNDING_BOX tFatUnitBox;
	BOUNDING_BOX tFatUnitBox2;
	// TRAVIS: This used to expand the bounding box by the new position, but that wouldn't
	// include padding for radius, etc. Added a bbox merge to take care of this
	sMakeBoundingBox( & tFatUnitBox, vPositionPrev, fRadius + fSpeed + UNIT_TEST_BUFFER, fHeight );
	sMakeBoundingBox( & tFatUnitBox2, *pvPositionNew, fRadius + fSpeed + UNIT_TEST_BUFFER, fHeight );
	BoundingBoxExpandByBox( & tFatUnitBox, & tFatUnitBox2 );

	int close_count = 0;
	UNIT * close_monsters[ MAX_CLOSE_UNITS ];
	LEVEL* pLevel = UnitGetLevel( pUnit );

	if( !PreSortedRooms )
	{
		ArrayClear(*ActiveRooms);
	
		pLevel->m_pQuadtree->GetItems( tFatUnitBox, (*ActiveRooms) );
		
	}
	nRoomsCloseCount = (int)(*ActiveRooms).Count();

	for( int j = 0; j < nRoomsCloseCount; j++ )
	{
		ROOM * pRoomToTest = (*ActiveRooms)[j];

		if ( dwMoveFlags & MOVEMASK_DOORS )
		{
			for ( int target_type = TARGET_OBJECT; target_type <= TARGET_DEAD; target_type++ )
			{
				for ( UNIT * monster = pRoomToTest->tUnitList.ppFirst[target_type]; monster; monster = monster->tRoomNode.pNext )
				{
					if( target_type == TARGET_DEAD &&
						!UnitDataTestFlag(monster->pUnitData, UNIT_DATA_FLAG_COLLIDE_WHEN_DEAD))
					{
						continue;
					}
					if ( monster != pUnit )
					{
						if( !UnitIsA( monster, UNITTYPE_BLOCKING_OPENABLE ) )
						{
							continue;
						}

						if( !ObjectIsDoor( monster ) )
						{
							continue;
						}

						if( UnitTestMode( monster, MODE_OPENED ) ||
							UnitTestMode( monster, MODE_OPEN ) )
						{
							continue;
						}
						
						// this unit is close enough to worry about
						if ( close_count < MAX_CLOSE_UNITS )
						{
							close_monsters[ close_count ] = monster;
							close_count++;
						}

					}
				}
			}
		}
	}

	// -- Bounce off of room collision --
	BOOL bBounceRoom = FALSE;
	BOOL bBounceUnit = ( dwMoveFlags & MOVEMASK_DOORS );
	int nBounce;
	VECTOR vPreviousTemp = vPositionPrev;
	for ( nBounce = 0; ( nBounce < MAX_BOUNCE_PASSES ) && ( bBounceRoom || bBounceUnit ); nBounce++ )
	{
		// if necessary, check the rooms again - we might have ping-ponged back into them
		if (  !( dwMoveFlags & MOVEMASK_NOT_WALLS ) && ( !nBounce || bBounceRoom || bBounceUnit ) )
		{
			bBounceRoom = FALSE;
			if ( !( dwMoveFlags & MOVEMASK_NO_GEOMETRY ) )
			{
				for ( int ii = 0; ii < nRoomsCloseCount; ii ++ )
				{
					BOOL bCollide = sRoomCollide(UnitGetGame(pUnit), (*ActiveRooms)[ ii ], pUnit, vPreviousTemp, pvPositionNew, bMoving, fRadius, fHeight, dwMoveFlags, &pResult->flags);
					bBounceRoom |= bCollide;
				}
			}
		}
		// if necessary, check the units again - we might have ping-ponged back into them
		if ( !nBounce || bBounceRoom || bBounceUnit )
		{
			bBounceUnit = FALSE;
			for ( int ii = 0; ii < close_count; ii++ )
			{
				BOOL bCollide = sCollideUnit( close_monsters[ ii ], &vPreviousTemp, pvPositionNew, fRadius );
				if ( bCollide )
					pResult->unit = close_monsters[ii];
				bBounceUnit = bCollide;
				if( bCollide )
				{
					SETBIT(pResult->flags, MOVERESULT_COLLIDE_UNIT);
				}
			}
		}
		if ( bBounceRoom || bBounceUnit )
		{
			vPreviousTemp = *pvPositionNew;
		}
	}
	
	// -- Update the unit's room --
	// you have to do this afterwards in case a player moved in and out rooms during collision testing
	if ((dwMoveFlags & MOVEMASK_TEST_ONLY) == 0)
	{
		VECTOR vPositionNewLifted = *pvPositionNew;
		vPositionNewLifted.fZ += 0.05f;// lift the position slightly so that you don't fall through the floor
		for (int ii = 0; ii < nRoomsCloseCount; ii++)
		{
			if ( ConvexHullPointTest( (*ActiveRooms)[ii]->pHull, & vPositionNewLifted ) )
			{
				UnitUpdateRoom(pUnit, (*ActiveRooms)[ii], TRUE);
				break;
			}
		}
	}
	else if ((dwMoveFlags & MOVEMASK_UPDATE_POSITION) == 0)
	{
		*pvPositionNew = vTargetPosition;
	}
	Rooms.Destroy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomTestPos(
	GAME* game,
	ROOM* room,
	const VECTOR& vPositionPrev,
	VECTOR* pvPositionNew,
	float fRadius,
	float fHeight,
	DWORD dwFlags)
{
	VECTOR vDelta = *pvPositionNew - vPositionPrev;
	BOOL bMoving = !VectorIsZero(vDelta);
	DWORD dwMoveResult = 0;
	return sRoomCollide(game, room, NULL, vPositionPrev, pvPositionNew, bMoving, fRadius, fHeight, dwFlags, &dwMoveResult);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sGetRoomsInteresectBox(
	ROOM* center,
	ROOM** roomlist,
	int maxcount,
	BOUNDING_BOX& box)
{
	roomlist[0] = center;
	int count = 1;

	int adjacentRoomCount = RoomGetAdjacentRoomCount(center);
	for (int ii = 0; ii < adjacentRoomCount; ii++)
	{
		ROOM * room = RoomGetAdjacentRoom(center, ii);
		if (!room)
		{
			continue;
		}
		if ( ConvexHullAABBCollide( room->pHull, &box ) )
		{
			ASSERT_RETZERO(count < maxcount);
			roomlist[count] = room;
			count++;
		}
	}
	return count;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define SCAN_RADIUS			1.0f

struct ROOM_COLLIDE_SCAN_DATA
{
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sCollisionScanRoomCallback(
	ROOM_CPOLY* poly,
	void* data)
{
//	ROOM_COLLIDE_SCAN_DATA* Data = (ROOM_COLLIDE_SCAN_DATA*)data;

//	const BOUNDING_BOX& tBoxInRoom = *(Data->ptBoxInRoom);


	return FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sCollisionScanRoom(
	GAME* game, 
	ROOM* room,
	const VECTOR& position,
	float radius,
	float height)
{
	ASSERT_RETURN(game && room);
	ASSERT_RETURN(room->pDefinition);

	// move the input position into the room's model coords
	VECTOR roompos;
	MatrixMultiply(&roompos, &position, &room->tWorldMatrixInverse);

	BOUNDING_BOX tAreaBox;
	sMakeBoundingBox(&tAreaBox, roompos, radius + SCAN_RADIUS, height);

	ROOM_COLLIDE_SCAN_DATA data;
	RoomIterateCollisionBox(room, tAreaBox.vMin.fX, tAreaBox.vMin.fY, tAreaBox.vMax.fX, tAreaBox.vMax.fY, COLLISION_WALL, sCollisionScanRoomCallback, (void*)&data);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CollisionScan(
	GAME* game,
	ROOM* room,
	const VECTOR& position,
	float radius,
	float height)
{
	ASSERT_RETURN(game && room);

	BOUNDING_BOX tAreaBox;
	sMakeBoundingBox(&tAreaBox, position, radius + SCAN_RADIUS, height);

	ROOM* roomlist[MAX_ROOMS_CONNECTED];
	int roomcount = sGetRoomsInteresectBox(room, roomlist, MAX_ROOMS_CONNECTED, tAreaBox);

	for (int ii = 0; ii < roomcount; ii++)
	{
		sCollisionScanRoom(game, roomlist[ii], position, radius, height);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillIsValidTarget( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pTestUnit, 
	int nSkill, 
	UNIT * pWeapons[MAX_WEAPON_LOCATIONS_PER_SKILL],
	BOOL & bIsInRange )
{
	BOOL bHasWeapon = FALSE;
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		if ( ! pWeapons[ i ] )
			continue;
		bHasWeapon = TRUE;
		if ( SkillIsValidTarget( pGame, pUnit, pTestUnit, pWeapons[ i ], nSkill, TRUE, &bIsInRange ) )
		{
			return TRUE;
		}
	}

	if ( ! bHasWeapon )
		return SkillIsValidTarget( pGame, pUnit, pTestUnit, NULL, nSkill, TRUE, &bIsInRange );

	return FALSE;
}

#define SELECTION_ERROR		0.1f

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* SelectTarget(
	GAME* game,
	UNIT* unit,
	DWORD dwFlags,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fRange,
	BOOL * pbIsInRange,
	float* pfDistance,
	VECTOR* pvNormal,
	int nSkill,
	UNIT * pWeapon,
	UNITID idPrevious,
	UNITTYPE * pUnitType,
	float* pfLevelLineCollideResults )
{
	if (!game || !unit || !UnitGetRoom(unit))
	{
		if (pfDistance)
		{
			*pfDistance = fRange;
		}
		return NULL;
	}

	if ( ( dwFlags & SELECTTARGET_FLAG_RESELECT_PREVIOUS ) && ( idPrevious == INVALID_ID ) )
	{
		if ( pfDistance )
			*pfDistance = fRange;
		return NULL;
	}

	LEVEL * pLevel = UnitGetLevel( unit );

	enum UNIT_SELECTION_TYPE
	{
		UNIT_SELECTION_SLIM,
		UNIT_SELECTION_FAT,
		UNIT_SELECTION_COUNT
	};
	UNIT* selected_unit[ UNIT_SELECTION_COUNT ] = { NULL, NULL };
	BOOL bIsInRange[ UNIT_SELECTION_COUNT ] = { FALSE, FALSE };
	if ( pbIsInRange )
		*pbIsInRange = FALSE;
	float fMaxLen = 0;
	if ( pfLevelLineCollideResults && *pfLevelLineCollideResults >= 0 )
		fMaxLen = *pfLevelLineCollideResults;
	else
	{
		fMaxLen = LevelLineCollideLen(game, pLevel, vPosition, vDirection, fRange, pvNormal);

		if(!(dwFlags & SELECTTARGET_FLAG_DONT_RETURN_SELECTION_ERROR))
		{
			fMaxLen += SELECTION_ERROR;
		}

		if ( pfLevelLineCollideResults )
		{
			*pfLevelLineCollideResults = fMaxLen;
		}
	}

	CRayIntersector tRayIntersector( vPosition, vDirection );

	// used in point line calcs
	float fLengthSq = fMaxLen * fMaxLen;
	float fClosestSq[ UNIT_SELECTION_COUNT ] = { fLengthSq, fLengthSq };

	int nSkillRight = INVALID_ID;
	int nSkillLeft  = INVALID_ID;
	UNIT * pWeaponsRight[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UNIT * pWeaponsLeft [ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	if ( nSkill == INVALID_ID )
	{
		nSkillRight = UnitGetStat(unit, STATS_SKILL_RIGHT);
		nSkillLeft  = UnitGetStat(unit, STATS_SKILL_LEFT);
		UnitGetWeapons( unit, nSkillRight, pWeaponsRight, FALSE );
		UnitGetWeapons( unit, nSkillLeft,  pWeaponsLeft,  FALSE );
	} else {
		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );
		if ( pSkillData )
		{
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) )
				dwFlags |= SELECTTARGET_FLAG_CHECK_LOS_TO_AIM_POS;
		}
	}

	BOOL bTestFattenedUnitsOnly = dwFlags & SELECTTARGET_FLAG_FATTEN_UNITS;
	if ( TEST_MASK( dwFlags, SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT ) )
		bTestFattenedUnitsOnly = FALSE;

	VECTOR vFaceDirectionNoZ = UnitGetFaceDirection( unit, FALSE );
	if ( dwFlags & SELECTTARGET_FLAG_ONLY_IN_FRONT_OF_UNIT )
	{
		vFaceDirectionNoZ.fZ = 0.0f;
		VectorNormalize( vFaceDirectionNoZ );
	}

	// this needs to be optimized to only search through rooms that matter
	for (ROOM* room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ))
	{
		for (int nTargetList = TARGET_COLLISIONTEST_START; nTargetList < NUM_TARGET_TYPES; nTargetList++)
		{
			for (UNIT* pTestUnit = room->tUnitList.ppFirst[nTargetList]; pTestUnit; pTestUnit = pTestUnit->tRoomNode.pNext)
			{
				UNIT_SELECTION_TYPE eSelectionType = UNIT_SELECTION_SLIM;

				if (pTestUnit == unit)
				{
					continue;
				}

				if ( ( dwFlags & SELECTTARGET_FLAG_RESELECT_PREVIOUS ) && ( UnitGetId( pTestUnit ) != idPrevious ) )
				{
					continue;
				}

				if ( ( dwFlags & SELECTTARGET_FLAG_DONT_RESELECT_PREVIOUS ) && ( UnitGetId( pTestUnit ) == idPrevious ) )
				{
					continue;
				}

				// for now, only select monsters or players
				const UNIT_DATA * pTestUnitData = UnitGetData( pTestUnit );

				if(!pUnitType)
				{
					BOOL bIsMonster = UnitIsA(pTestUnit, UNITTYPE_MONSTER);
					BOOL bIsPlayer = UnitIsA(pTestUnit, UNITTYPE_PLAYER);
					BOOL bIsOpenable = UnitIsA(pTestUnit, UNITTYPE_OBJECT) && 
						UnitCanInteractWith(pTestUnit, unit) &&
						UnitDataTestFlag(pTestUnitData, UNIT_DATA_FLAG_INTERACTIVE) && 
						!IsUnitDeadOrDying(pTestUnit);
					if (!bIsMonster && !bIsPlayer && !bIsOpenable)
					{
						BOOL bNoTarget = UnitIsA(pTestUnit, UNITTYPE_NOTARGET);
						if (!(dwFlags & SELECTTARGET_FLAG_ALLOW_NOTARGET) || !bNoTarget)
						{
							continue;
						}
					}
				}
				else
				{
					BOOL bIsCorrectType = UnitIsA(pTestUnit, *pUnitType);
					if(!bIsCorrectType)
					{
						continue;
					}
				}

				// is it the closest unit we are dealing with?
				float fTestRangeSquared = 0.0f;
				if (! UnitIsInRange( pTestUnit, vPosition, fRange, &fTestRangeSquared ) )
					continue;
				if (   fTestRangeSquared > fClosestSq[ UNIT_SELECTION_SLIM ]
					&& fTestRangeSquared > fClosestSq[ UNIT_SELECTION_FAT ] )
					continue;

				if ((dwFlags & SELECTTARGET_FLAG_IGNORE_DIRECTION) == 0)
				{
					VECTOR vDirToTestUnit = UnitGetCenter( pTestUnit ) - vPosition;
					float fDistToTestUnit = VectorNormalize( vDirToTestUnit );
					if ( fDistToTestUnit > 1.0f && VectorDot( vDirToTestUnit, vDirection ) < 0 )
						continue;

					//BOOL bFlyingUnit = TESTBIT( pTestUnit->pdwMovementFlags, MOVEFLAG_FLY );
					
					BOOL bBoundingBoxIntersected = FALSE;
					for ( int nSelectionType = 0; nSelectionType < UNIT_SELECTION_COUNT; nSelectionType++ )
					{
						if ( bTestFattenedUnitsOnly && nSelectionType == UNIT_SELECTION_SLIM )
							continue;

						if ( fTestRangeSquared > fClosestSq[ nSelectionType ] )
							continue;

						if ( ! TEST_MASK( dwFlags, SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT ) 
							&& nSelectionType == UNIT_SELECTION_FAT )
							continue;

						BOUNDING_BOX box;
						// create a pseudo bounding box / axis alligned
						float fRadius = UnitGetCollisionRadius( pTestUnit );
						float fHeight = UnitGetCollisionHeight( pTestUnit );
						if ( ! UnitDataTestFlag( pTestUnitData, UNIT_DATA_FLAG_DONT_FATTEN_COLLISION_FOR_SELECTION )
							&& nSelectionType == UNIT_SELECTION_FAT )
						{
							fRadius *= 1.5f;	
							fHeight *= 8.0f;
						} 
						box.vMin.fX = pTestUnit->vPosition.fX - fRadius;
						box.vMin.fY = pTestUnit->vPosition.fY - fRadius;
						box.vMin.fZ = pTestUnit->vPosition.fZ;
						//if ( bFlyingUnit )
						//	box.vMin.fZ -= fHeight / 2.0f;
						//else 
						if ( nSelectionType == UNIT_SELECTION_FAT )
							box.vMin.fZ -= fHeight / 4.0f;
						box.vMax = box.vMin;
						VECTOR vTemp;
						vTemp.fX = pTestUnit->vPosition.fX + fRadius;
						vTemp.fY = pTestUnit->vPosition.fY + fRadius;
						vTemp.fZ = box.vMin.fZ + fHeight;

						BoundingBoxExpandByPoint( &box, &vTemp );

						if ( !BoundingBoxIntersectRay( box, tRayIntersector, fMaxLen ) )
							continue;
						else
						{
							bBoundingBoxIntersected = TRUE;
							eSelectionType = (UNIT_SELECTION_TYPE)nSelectionType;
							break;
						}
					}

					if ( ! bBoundingBoxIntersected )
						continue;
				}
								
				BOOL bIsInSkillRange = FALSE;
				if ( nSkill != INVALID_ID )
				{
					if ( ! SkillIsValidTarget( game, unit, pTestUnit, pWeapon, nSkill, TRUE, &bIsInSkillRange, FALSE, TRUE ) )
						continue;
				}
				else 
				{
					if (!sSkillIsValidTarget( game, unit, pTestUnit, nSkillRight, pWeaponsRight, bIsInSkillRange ) &&
						!sSkillIsValidTarget( game, unit, pTestUnit, nSkillLeft,  pWeaponsLeft,  bIsInSkillRange ) &&
						!UnitCanInteractWith( pTestUnit, unit ))
					continue;
					if ( UnitCanInteractWith( pTestUnit, unit ) )
					{
						bIsInSkillRange = VectorDistanceSquared( UnitGetPosition(unit), UnitGetPosition(pTestUnit) ) <= ( UNIT_INTERACT_DISTANCE_SQUARED );
					}
				}

				if ( ! bIsInSkillRange && pbIsInRange && bIsInRange[ eSelectionType ] )
					continue;

				if ( dwFlags & SELECTTARGET_FLAG_CHECK_LOS_TO_AIM_POS && 
					 !UnitDataTestFlag( pTestUnitData, UNIT_DATA_FLAG_SELECT_TARGET_IGNORES_AIM_POS ))
				{
					VECTOR vAimPoint = UnitGetAimPoint( pTestUnit );
					VECTOR vDirToAimPoint = vAimPoint - vPosition;
					float fDistanceToAimPoint = VectorNormalize( vDirToAimPoint );
					float fCollisionCheck = LevelLineCollideLen( game, pLevel, vPosition, vDirToAimPoint, fDistanceToAimPoint);
					if ( fCollisionCheck < fDistanceToAimPoint )
					{
						if ( IS_SERVER( game ) )
							continue;

						// On the client, check if the aim point, calculated
						// with an offset position on the server, is obstructed.
						// If the client used an animated bone position for 
						// the aim point previously, it may be clipping
						// through collision in the world. -cmarch
						vAimPoint = UnitGetAimPoint( pTestUnit, TRUE );
						vDirToAimPoint = vAimPoint - vPosition;
						fDistanceToAimPoint = VectorNormalize( vDirToAimPoint );
						fCollisionCheck = LevelLineCollideLen( game, pLevel, vPosition, vDirToAimPoint, fDistanceToAimPoint);
						if ( fCollisionCheck < fDistanceToAimPoint )
							continue;
					}
				}

				if ( dwFlags & SELECTTARGET_FLAG_ONLY_IN_FRONT_OF_UNIT )
				{
					VECTOR vTestPos = UnitGetPosition( pTestUnit );
					VECTOR vDirToTestPos = vTestPos - UnitGetPosition( unit );
					vDirToTestPos.fZ = 0.0f;
					VectorNormalize( vDirToTestPos );
					float fDot = VectorDot( vDirToTestPos, vFaceDirectionNoZ );
					if ( fDot < 0.15f )
						continue;
				}
				
				bIsInRange[ eSelectionType ] = bIsInSkillRange;
				selected_unit[ eSelectionType ] = pTestUnit;
				fClosestSq[ eSelectionType ] = fTestRangeSquared;
			}
		}
	}

	UNIT_SELECTION_TYPE eBestSelectionType = UNIT_SELECTION_SLIM;

	for ( int nSelectionType = 0; nSelectionType < UNIT_SELECTION_COUNT; nSelectionType++ )
	{
		if ( selected_unit[ nSelectionType ] )
		{
			eBestSelectionType = (UNIT_SELECTION_TYPE)nSelectionType;
			break;
		}
	}

	if ( pbIsInRange )
		*pbIsInRange = bIsInRange[ eBestSelectionType ];

	// need something faster than sqrt.
	if ( pfDistance )
		*pfDistance = sqrt( fClosestSq[ eBestSelectionType ] );

	if ( pvNormal && ( selected_unit[ eBestSelectionType ] || fMaxLen == fRange )) 
	{
		pvNormal->fX = -vDirection.fX;
		pvNormal->fY = -vDirection.fY;
		pvNormal->fZ = -vDirection.fZ;
	} 

	return selected_unit[ eBestSelectionType ];
}


//----------------------------------------------------------------------------
// why is this function here?  it seems like you could change selecttarget to get it to do this job too... just extra code to maintain... - Tyler
//----------------------------------------------------------------------------
UNIT* SelectDebugTarget(
	GAME* game,
	UNIT* unit,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fRange)
{
	UNIT* selected_unit = NULL;
	LEVEL * pLevel = UnitGetLevel( unit );
	float fMaxLen = LevelLineCollideLen(game, pLevel, vPosition, vDirection, fRange, NULL) + 1.0f;

	// used in point line calcs
	VECTOR vEndPoint;
	vEndPoint.fX = (vDirection.fX * fMaxLen) + vPosition.fX;
	vEndPoint.fY = (vDirection.fY * fMaxLen) + vPosition.fY;
	vEndPoint.fZ = (vDirection.fZ * fMaxLen) + vPosition.fZ;

	VECTOR vDelta;
	VectorSubtract(vDelta, vEndPoint, vPosition);
	float fLengthSq = VectorLengthSquared(vDelta);
	float f2dLengthSq = (vDelta.fX * vDelta.fX) + (vDelta.fY * vDelta.fY);
	REF(f2dLengthSq);
	float fClosestSq = fLengthSq;

	// this needs to be optimized to only search through rooms that matter
	for (ROOM* room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ))
	{
		for (int target_list = TARGET_NONE; target_list < NUM_TARGET_TYPES; target_list++)
		{
			for (UNIT* test_unit = room->tUnitList.ppFirst[target_list]; test_unit; test_unit = test_unit->tRoomNode.pNext)
			{
				if (test_unit == unit)
				{
					continue;
				}

				// is it the closest unit we are dealing with?
				float dist_sq = VectorDistanceSquared(vPosition, UnitGetPosition(test_unit));
				if (dist_sq > fClosestSq)
				{
					continue;
				}

				BOUNDING_BOX box;
				float fRadius = UnitGetCollisionRadius( test_unit ) * 1.10f;	// 10% selection buffer lovin
				float fHeight = UnitGetCollisionHeight( test_unit ) * 1.10f;
				box.vMin.fX = test_unit->vPosition.fX - fRadius;
				box.vMin.fY = test_unit->vPosition.fY - fRadius;
				box.vMin.fZ = test_unit->vPosition.fZ;
				box.vMax = box.vMin;
				{
					VECTOR vTemp;
					vTemp.fX = test_unit->vPosition.fX + fRadius;
					vTemp.fY = test_unit->vPosition.fY + fRadius;
					vTemp.fZ = test_unit->vPosition.fZ + fHeight;
					BoundingBoxExpandByPoint(&box, &vTemp);
				}

				// find center point
				VECTOR vCenter = box.vMax + box.vMin;
				VectorScale(vCenter, vCenter, 0.5f);

				// find closest point from our selection ray to this unit's center of their bounding box

				// I have no idea where the bug is in this code. I have looked at it for 2 days
				// and can't figure it out, so I put back the unoptimized vector-point distance
				// routine and will figure this out later - drb
				float fU = (((vCenter.fX - vPosition.fX) * vDelta.fX) +
							 ((vCenter.fY - vPosition.fY) * vDelta.fY) +
							 ((vCenter.fZ - vPosition.fZ) * vDelta.fZ)) /
							 fLengthSq;

				// Check if point falls within the line segment
				if (fU < 0.0f || fU > 1.0f)
				{
					continue;
				}

				VECTOR vClosest;
				vClosest.fX = vPosition.fX + (fU * vDelta.fX);
				vClosest.fY = vPosition.fY + (fU * vDelta.fY);
				vClosest.fZ = vPosition.fZ + (fU * vDelta.fZ);

				// check if that point is within the bounding box
				if (!BoundingBoxTestPoint(&box, &vClosest))
				{
					continue;
				}
				
				selected_unit = test_unit;
				fClosestSq = dist_sq;
			}
		}
	}

	return selected_unit;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/*
static float sRoomLineCollideLen( 
	ROOM *pRoom, 
	const VECTOR & vPositionIn, 
	const VECTOR & vDirectionIn, 
	float fMaxLen, 
	VECTOR * pvNormal )
{
	ASSERT( pRoom );
	ASSERT( pRoom->pDefinition );
	ROOM_DEFINITION *pRDefinition = pRoom->pDefinition;

	// move the input position and direction into the room's model coords
	VECTOR vPosition;
	VECTOR vDirection;
	MatrixMultiply		( & vPosition,  & vPositionIn, & pRoom->tWorldMatrixInverse );
	MatrixMultiplyNormal( & vDirection, & vDirectionIn,& pRoom->tWorldMatrixInverse );

	// compute endpoint for the ray
	ASSERT( pRDefinition );
	VECTOR vDirectionScaled;
	VectorScale( vDirectionScaled, vDirection, fMaxLen );
	VECTOR vRayEnd;
	VectorAdd( vRayEnd, vPosition, vDirectionScaled );

	hkShapeRayCastInput tRayCastInput;
	tRayCastInput.m_from.set( vPosition.fX, vPosition.fY, vPosition.fZ );
	tRayCastInput.m_to  .set( vRayEnd.fX,	vRayEnd.fY,	  vRayEnd.fZ );
	tRayCastInput.m_rayShapeCollectionFilter = NULL;
	hkShapeRayCastOutput tRayCastOutput;

	// cast the ray
	pRDefinition->pHavokShape->castRay( tRayCastInput, tRayCastOutput );

	// change the normal back into world coordinates
	if ( tRayCastOutput.hasHit() ) 
	{
		if ( pvNormal )
		{
			pvNormal->fX = tRayCastOutput.m_normal.getSimdAt( 0 );
			pvNormal->fY = tRayCastOutput.m_normal.getSimdAt( 1 );
			pvNormal->fZ = tRayCastOutput.m_normal.getSimdAt( 2 );
			MatrixMultiplyNormal( pvNormal, pvNormal, &pRoom->tWorldMatrix );
		} 
	} else {
		ASSERT_RETZERO( tRayCastOutput.m_hitFraction == 1.0f );
	}
	return tRayCastOutput.m_hitFraction * fMaxLen;
}
// */

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// #ifdef HAMMER
static float sRoomLineCollideLenGeometry(
	ROOM *pRoom, 
	const VECTOR & vPositionIn, 
	const VECTOR & vDirectionIn,
	int & nModelHitId,
	float fMaxLen, 
	VECTOR * pvNormal,
	int nNoCollideCount,
	int *pnNoCollideModelIds)
{
	ASSERT( pRoom );
	ASSERT( pRoom->pDefinition );
	ROOM_DEFINITION *pRDefinition = pRoom->pDefinition;
	REF(pRDefinition);
	nModelHitId = INVALID_ID;

	extern BOOL e_ModelDefinitionCollide(int, const VECTOR&, const VECTOR&, int*, int*, VECTOR*, float*, float*, VECTOR*, float*, BOOL);

	for ( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
	{
		int nModelID = pRoom->pnRoomModels[ i ];
		if ( nModelID == INVALID_ID )
			continue;

		BOOL bSkip = FALSE;
		for(int j=0; j<nNoCollideCount; j++)
		{
			if (nModelID == pnNoCollideModelIds[j])
			{
				bSkip = TRUE;
				break;
			}
		}
		if (bSkip)
		{
			continue;
		}

		int nModelDefinition = e_ModelGetDefinition(nModelID);
		const MATRIX * pModelWorld = e_ModelGetWorldScaled(nModelID);
		MATRIX tModelInverse;
		MatrixInverse(&tModelInverse, pModelWorld);
		VECTOR vPosition = vPositionIn;
		VECTOR vDirection = vDirectionIn;
		MatrixMultiply(&vPosition, &vPosition, &tModelInverse);
		MatrixMultiplyNormal(&vDirection, &vDirection, &tModelInverse);
		VectorNormalize(vDirection);
		float fModelMaxLen = fMaxLen;
		VECTOR vModelPositionOnMesh;
		VECTOR vModelNormal;
		VECTOR vPositionOnMesh;
		BOOL bFound = e_ModelDefinitionCollide(nModelDefinition,
												vPosition,
												vDirection,
												NULL, NULL,
												&vModelPositionOnMesh,
												NULL, NULL,
												&vModelNormal,
												&fModelMaxLen,
												FALSE);

		if (!bFound)
		{
			continue;
		}
		
		nModelHitId = nModelID; 
		VECTOR vScale = e_ModelGetScale(nModelHitId);
		//This only works because it's in object space and not world space
		vModelPositionOnMesh = vModelPositionOnMesh * vScale;
		VECTOR tmpPos = vPosition * vScale;
		//end scale code
		fModelMaxLen = VectorLength(tmpPos - vModelPositionOnMesh);
		if (fModelMaxLen < fMaxLen)
		{
			fMaxLen = fModelMaxLen;
			if (pvNormal)
			{
				*pvNormal = vModelNormal;
				MatrixMultiplyNormal(pvNormal, pvNormal, pModelWorld);
			}
		}
	}

	return fMaxLen;
}
//-------------------------------------------------------------------------------------------------
//For hammer layout item selection through holodeck
//-------------------------------------------------------------------------------------------------
// #ifdef HAMMER
static int sRoomLineCollideModelId(
	ROOM *pRoom, 
	const VECTOR & vPositionIn, 
	const VECTOR & vDirectionIn, 
	float fMaxLen, 
	VECTOR * pvNormal,
	int nNoCollideCount,
	int *pnNoCollideModelIds)
{
	ASSERT( pRoom );
	ASSERT( pRoom->pDefinition );
	ROOM_DEFINITION *pRDefinition = pRoom->pDefinition;
	REF(pRDefinition);

	extern BOOL e_ModelDefinitionCollide(int, const VECTOR&, const VECTOR&, int*, int*, VECTOR*, float*, float*, VECTOR*, float*, BOOL);

	int nClosestModelId = INVALID_ID;
	float fClosestLen = fMaxLen;

	for ( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
	{
		int nModelID = pRoom->pnRoomModels[ i ];
		if ( nModelID == INVALID_ID )
			continue;

		BOOL bSkip = FALSE;
		for(int j=0; j<nNoCollideCount; j++)
		{
			if (nModelID == pnNoCollideModelIds[j])
			{
				bSkip = TRUE;
				break;
			}
		}
		if (bSkip)
		{
			continue;
		}

		int nModelDefinition = e_ModelGetDefinition(nModelID);
		const MATRIX * pModelWorld = e_ModelGetWorldScaled(nModelID);
		MATRIX tModelInverse;
		MatrixInverse(&tModelInverse, pModelWorld);
		VECTOR vPosition = vPositionIn;
		VECTOR vDirection = vDirectionIn;
		MatrixMultiply(&vPosition, &vPosition, &tModelInverse);
		MatrixMultiplyNormal(&vDirection, &vDirection, &tModelInverse);
		VectorNormalize(vDirection);
		float fModelMaxLen = fMaxLen;
		VECTOR vModelPositionOnMesh;
		VECTOR vModelNormal;
		VECTOR vPositionOnMesh;
		BOOL bFound = e_ModelDefinitionCollide(nModelDefinition,
												vPosition,
												vDirection,
												NULL, NULL,
												&vModelPositionOnMesh,
												NULL, NULL,
												&vModelNormal,
												&fModelMaxLen,
												FALSE);

		if (bFound)
		{
			if(fModelMaxLen < fClosestLen)
			{
				fClosestLen = fModelMaxLen;
				nClosestModelId = nModelID;
			}
		}
	}

	return nClosestModelId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float LevelLineCollideGeometry(
	ROOM * pRoom,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	int & nModelHitId,
	float fMaxLen,
	VECTOR * pvNormal,
	int nNoCollideCount,
	int *pnNoCollideModelIds)
{
	if (pRoom)
	{
		// if the room is valid, then run the static room collision test
		float fLength = fMaxLen;
		fLength = sRoomLineCollideLenGeometry(pRoom, vPosition, vDirection,nModelHitId,fLength, pvNormal, nNoCollideCount, pnNoCollideModelIds);
		return fLength;
	}
	else
	{
		// if the room is null, then just test against the Z = 0 plane
		if ( vDirection.fZ == 0.0f )
			return fMaxLen;

		float fDistToZero = - vPosition.fZ / vDirection.fZ;
		if ( pvNormal )
			*pvNormal = VECTOR( 0, 0, 1 );
		return MAX( MIN( fDistToZero, fMaxLen ), 0.0f );
	}
}

//-------------------------------------------------------------------------------------------------
//For hammer layout item selection through holodeck
//-------------------------------------------------------------------------------------------------
int LevelLineCollideModelId(
	ROOM * pRoom,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	VECTOR * pvNormal,
	int nNoCollideCount,
	int *pnNoCollideModelIds)
{
	if (pRoom)
	{
		// if the room is valid, then run the static room collision test
		float fLength = fMaxLen;
		return sRoomLineCollideModelId(pRoom, vPosition, vDirection, fLength, pvNormal, nNoCollideCount, pnNoCollideModelIds);
	}

	return INVALID_ID;
}

#if HELLGATE_ONLY
//----------------------------------------------------------------------------
//class used for LevelLineCollideAll()
//----------------------------------------------------------------------------
class CollisionHitCollector : public hkRayHitCollector
{
public:
	CollisionHitCollector(
		GAME * pGame,
		LEVEL * pLevel,
		const VECTOR & vPosition,
		const VECTOR & vDirection,
		CollisionCallback callbackFunc,
		void * userdata) : 
			cbFunc(callbackFunc), data(userdata), 
				vPosition(vPosition), vDirection(vDirection), 
				pGame(pGame), pLevel(pLevel), hkRayHitCollector() {}

	void addRayHit (const hkCdBody &cdBody, const hkShapeRayCastCollectorOutput &hitInfo )
	{
		const hkShape * pShape = cdBody.getShape();
		hkShapeType eShapeType = pShape ? pShape->getType() : HK_SHAPE_INVALID;
		if(hitInfo.hasHit() && cbFunc && eShapeType == HK_SHAPE_TRIANGLE)
		{
			VECTOR vLocation = vPosition + hitInfo.m_hitFraction * vDirection;
			ROOM * pRoom = RoomGetFromPosition(pGame, pLevel, &vLocation);
			ROOM_INDEX * pRoomIndex = NULL;
			//*
			const hkCollidable * pCollidable = cdBody.getRootCollidable();
			if(pCollidable)
			{
				hkRigidBody * pRigidBody = hkGetRigidBody(pCollidable);
				if(pRigidBody)
				{
					pRoomIndex = (ROOM_INDEX *)pRigidBody->getUserData();
				}
			}
			// */
			/*
			const hkCdBody * pParent = &cdBody;
			while(pParent && !pRoomIndex)
			{
				pParent = pParent->getParent();
				const hkCollidable * pCollidable = reinterpret_cast<const hkCollidable*>(pParent);
				if(pCollidable)
				{
					hkRigidBody * pRigidBody = hkGetRigidBody(pCollidable);
					if(pRigidBody)
					{
						pRoomIndex = (ROOM_INDEX *)pRigidBody->getUserData();
					}
				}
			}
			// */
			(*cbFunc)(pRoomIndex, pRoom, vLocation, cdBody.getShapeKey(), hitInfo.m_hitFraction, data);
		}
	}

private:
	CollisionCallback cbFunc;
	void * data;
	const VECTOR vPosition;
	const VECTOR vDirection;
	GAME * pGame;
	LEVEL * pLevel;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelLineCollideAll(
	GAME* pGame,
	LEVEL * pLevel,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	CollisionCallback callbackFunc,
	void * userdata)
{
	ASSERT_RETURN(pGame);
	if (!pLevel)
	{
		return;
	}

	// compute endpoint for the ray
	VECTOR vDirectionScaled;
	VectorScale( vDirectionScaled, vDirection, fMaxLen );
	VECTOR vRayEnd;
	VectorAdd( vRayEnd, vPosition, vDirectionScaled );

	hkWorldRayCastInput tRayCastInput;
	tRayCastInput.m_from.set( vPosition.fX, vPosition.fY, vPosition.fZ );
	tRayCastInput.m_to  .set( vRayEnd.fX,	vRayEnd.fY,	  vRayEnd.fZ );
	tRayCastInput.m_enableShapeCollectionFilter = FALSE;
	tRayCastInput.m_filterInfo = 0;
	CollisionHitCollector tRayCastOutput(pGame, pLevel, vPosition, vDirection, callbackFunc, userdata);

	pLevel->m_pHavokWorld->castRay(tRayCastInput, tRayCastOutput);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientControlUnitLineCollideAll(
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	CollisionCallback callbackFunc,
	void * userdata)
{
	GAME * pGame = AppGetCltGame();
	if(!pGame)
		return;

	UNIT * pPlayer = GameGetControlUnit(pGame);
	if(!pPlayer)
		return;

	LEVEL * pLevel = UnitGetLevel(pPlayer);
	if(!pLevel)
		return;

#if HELLGATE_ONLY
	if (AppIsHellgate())
	{
		LevelLineCollideAll(pGame, pLevel, vPosition, vDirection, fMaxLen, callbackFunc, userdata);
	}
#endif
	if( AppIsTugboat() )
	{
		VECTOR vCollisionNormal;
		int FaceMaterial;
		EVENT_DATA *pEventData = (EVENT_DATA*)userdata;
		float fCollideDistance = LevelLineCollideLen( pGame, pLevel, vPosition, vDirection, fMaxLen, FaceMaterial, &vCollisionNormal);
		if( fCollideDistance != fMaxLen )
		{
			FaceMaterial -= 1;
			const ROOM_COLLISION_MATERIAL_DATA * pMaterialData = FindMaterialWithIndex(FaceMaterial);
			if(!pMaterialData)
				return;

			int nMaterialIndexMapped = INVALID_ID;
			if(pMaterialData->nMapsTo >= 0)
			{
				const ROOM_COLLISION_MATERIAL_DATA * pMaterialDataMapsTo = (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(NULL, DATATABLE_MATERIALS_COLLISION, pMaterialData->nMapsTo);

				if(pMaterialDataMapsTo)
				{
					nMaterialIndexMapped = GetIndexOfMaterial( pMaterialDataMapsTo );
				}
				else
				{
					nMaterialIndexMapped = GetIndexOfMaterial( pMaterialData );
				}
			}
			else
			{
				nMaterialIndexMapped = GetIndexOfMaterial( pMaterialData );
			}


			*(int *)pEventData->m_Data1 = nMaterialIndexMapped;
		}
		else
		{
			*(int *)pEventData->m_Data1 = INVALID_ID;
		}


	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelSortFaces( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen )
{
	ASSERT_RETURN(pLevel);

	VECTOR vDestination = vPosition + vDirection * fMaxLen;
	BOUNDING_BOX BoundingBox;
	BoundingBox.vMin = vPosition;
	BoundingBox.vMax = vPosition;
	ExpandBounds( BoundingBox.vMin, BoundingBox.vMax, vDestination );
	pLevel->m_Collision->pQuadTree->GetItems( BoundingBox, SortedFaces );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal,
	int nIgnoreMaterial )
{
	int FaceMaterial;
	return LevelLineCollideLen( pGame, pLevel, SortedFaces, vPosition, vDirection, fMaxLen, FaceMaterial, pvNormal, nIgnoreMaterial );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen,
	int& FaceMaterial,
	VECTOR* pvNormal,
	int nIgnoreMaterial )
{
	ASSERT_RETZERO(pLevel);
	VECTOR ImpactPoint;
	VECTOR vDestination = vPosition + vDirection * fMaxLen;
	float fBestDistance = INFINITY;
	BOUNDING_BOX BoundingBox;
	BoundingBox.vMin = vPosition;
	BoundingBox.vMax = vPosition;
	ExpandBounds( BoundingBox.vMin, BoundingBox.vMax, vDestination );
	
	if( pLevel->m_Collision->pFaceList->RayCollision( SortedFaces, vPosition, vDestination, BoundingBox, ImpactPoint, pvNormal, FaceMaterial, fBestDistance, nIgnoreMaterial ) )
	{
		return VectorLength( ImpactPoint - vPosition );
	}
	else
	{
		return fMaxLen;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelLineCollideLenCollideUnitsTestRoom(
	ROOM *pRoom,
	void *pCallbackData )
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pCallbackData);

	EVENT_DATA * pEventData = (EVENT_DATA *)pCallbackData;
	CRayIntersector * pRayIntersector = (CRayIntersector *)pEventData->m_Data1;
	float fMaxLen = EventParamToFloat(pEventData->m_Data2);
	ROOM ** pRoomList = (ROOM **)pEventData->m_Data3;
	const int MAX_ROOMS = (const int)pEventData->m_Data4;
	int * pnCurrentRoomCount = (int *)pEventData->m_Data5;
	ASSERT_RETURN(pRayIntersector);
	ASSERT_RETURN(pRoomList);
	ASSERT_RETURN(MAX_ROOMS > 0);
	ASSERT_RETURN(pnCurrentRoomCount);

	if(*pnCurrentRoomCount >= MAX_ROOMS)
	{
		*pnCurrentRoomCount = MAX_ROOMS;
		return;
	}

	const BOUNDING_BOX * pBox = RoomGetBoundingBoxWorldSpace(pRoom);
	if (BoundingBoxIntersectRay(*pBox, *pRayIntersector, fMaxLen))
	{
		pRoomList[*pnCurrentRoomCount] = pRoom;
		*pnCurrentRoomCount++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sLevelLineCollideLenCollideUnitsTestUnit(
	UNIT *pUnit,
	void *pCallbackData )
{
	ASSERT_RETVAL(pUnit, RIU_CONTINUE);
	ASSERT_RETVAL(pCallbackData, RIU_CONTINUE);

	if(!ObjectIsBlocking(pUnit) && !ObjectIsWarp(pUnit))
	{
		return RIU_CONTINUE;
	}

	EVENT_DATA * pEventData = (EVENT_DATA *)pCallbackData;

	VECTOR * pvRayStart = (VECTOR *)pEventData->m_Data1;
	VECTOR * pvRayEnd = (VECTOR *)pEventData->m_Data2;
	float * pfMaxLen = (float *)pEventData->m_Data3;
	ASSERT_RETVAL(pvRayStart, RIU_CONTINUE);
	ASSERT_RETVAL(pvRayEnd, RIU_CONTINUE);
	ASSERT_RETVAL(pfMaxLen, RIU_CONTINUE);

	VECTOR vPointAlongRay = *pvRayEnd;
	if(RayIntersectsUnitCylinder(pUnit, *pvRayStart, *pvRayEnd, 0.0f, 0.0f, &vPointAlongRay))
	{
		float fLengthAlongRay = VectorLength(vPointAlongRay - *pvRayStart);
		if(fLengthAlongRay < *pfMaxLen)
		{
			*pfMaxLen = fLengthAlongRay;
		}
	}

	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sLevelLineCollideLenCollideUnits(
	LEVEL * pLevel,
	const VECTOR & vPosition,
	const VECTOR & vDirection,
	float fMaxLen)
{
	ASSERT_RETVAL(pLevel, fMaxLen);

	const int MAX_ROOMS = 64;
	ROOM * pRoomList[MAX_ROOMS];
	int nRoomCount = 1;
	pRoomList[0] = RoomGetFromPosition(LevelGetGame(pLevel), pLevel, &vPosition);
	// First find all the rooms to search
	CRayIntersector tRayIntersector( vPosition, vDirection );
	LevelIterateRooms(pLevel, sLevelLineCollideLenCollideUnitsTestRoom, &EVENT_DATA((DWORD_PTR)&tRayIntersector, EventFloatToParam(fMaxLen), (DWORD_PTR)pRoomList, MAX_ROOMS, (DWORD_PTR)&nRoomCount));

	float fMaxLengthFromUnits = fMaxLen;
	VECTOR vRayEnd = vPosition + fMaxLen * vDirection;
	// Then find all the units in all of the rooms to search
	for(int i=0; i<nRoomCount; i++)
	{
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		RoomIterateUnits(pRoomList[i], eTargetTypes, sLevelLineCollideLenCollideUnitsTestUnit, &EVENT_DATA((DWORD_PTR)&vPosition, (DWORD_PTR)&vRayEnd, (DWORD_PTR)&fMaxLengthFromUnits));
	}
	return fMaxLengthFromUnits;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal,
	DWORD dwFlags,
	int nIgnoreMaterial )
{
	ASSERT_RETVAL(pGame, fMaxLen);
	ASSERT_RETVAL(pLevel, fMaxLen);

	if (AppIsTugboat() && pLevel->m_Collision )
	{
		SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
		ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);

		BOUNDING_BOX BoundingBox;
		BoundingBox.vMin = vPosition;
		BoundingBox.vMax = vPosition;
		VECTOR vDestination = vPosition + vDirection * fMaxLen;
		ExpandBounds( BoundingBox.vMin, BoundingBox.vMax, vDestination );
		pLevel->m_Collision->pQuadTree->GetItems( BoundingBox, SortedFaces );
		float Result = LevelLineCollideLen( pGame, pLevel, SortedFaces, vPosition, vDirection, fMaxLen, pvNormal, nIgnoreMaterial );
		SortedFaces.Destroy();
		return Result;
	}

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		float fMaxLengthToUse = fMaxLen;
		if(TESTBIT(dwFlags, LCF_CHECK_OBJECT_CYLINDER_COLLISION_BIT))
		{
			fMaxLengthToUse = sLevelLineCollideLenCollideUnits(pLevel, vPosition, vDirection, fMaxLen);
		}
		return HavokLineCollideLen( pLevel->m_pHavokWorld, vPosition, vDirection, fMaxLengthToUse, pvNormal );
	}
#endif

	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	int& FaceMaterial,
	VECTOR* pvNormal,
	DWORD dwFlags,
	int nIgnoreMaterial )
{
	ASSERT_RETVAL(pGame, fMaxLen);
	ASSERT_RETVAL(pLevel, fMaxLen);

	if (AppIsTugboat())
	{
		SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
		ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);

		BOUNDING_BOX BoundingBox;
		BoundingBox.vMin = vPosition;
		BoundingBox.vMax = vPosition;
		VECTOR vDestination = vPosition + vDirection * fMaxLen;
		ExpandBounds( BoundingBox.vMin, BoundingBox.vMax, vDestination );
		pLevel->m_Collision->pQuadTree->GetItems( BoundingBox, SortedFaces );
		float Result = LevelLineCollideLen( pGame, pLevel, SortedFaces, vPosition, vDirection, fMaxLen, FaceMaterial, pvNormal, nIgnoreMaterial );
		SortedFaces.Destroy();
		return Result;
	}

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		float fMaxLengthToUse = fMaxLen;
		if(TESTBIT(dwFlags, LCF_CHECK_OBJECT_CYLINDER_COLLISION_BIT))
		{
			fMaxLengthToUse = sLevelLineCollideLenCollideUnits(pLevel, vPosition, vDirection, fMaxLen);
		}
		return HavokLineCollideLen( pLevel->m_pHavokWorld, vPosition, vDirection, fMaxLen, pvNormal );
	}
#endif

	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float ClientControlUnitLineCollideLen( 
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal)
{
	GAME * pGame = AppGetCltGame();
	if(!pGame)
		return fMaxLen;

	UNIT * pPlayer = GameGetControlUnit(pGame);
	if(!pPlayer)
		return fMaxLen;

	LEVEL * pLevel = UnitGetLevel(pPlayer);
	if(!pLevel)
		return fMaxLen;

	return LevelLineCollideLen(pGame, pLevel, vPosition, vDirection, fMaxLen, pvNormal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClearLineOfSightBetweenPositions(
	LEVEL *pLevel,
	const VECTOR &vPosFrom,
	const VECTOR &vPosTo)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// we will start at a and go to b
	VECTOR vDirectionTo = vPosTo - vPosFrom;
	float flMaxDistance = VectorNormalize( vDirectionTo );

	// collide 	
	GAME *pGame = LevelGetGame( pLevel );
	float flDistToCollide = LevelLineCollideLen( 
		pGame,
		pLevel,
		vPosFrom,
		vDirectionTo,
		flMaxDistance);
			
	const float flSlop = 0.5f;
	if (flDistToCollide < (flMaxDistance - flSlop))
	{
		return FALSE;
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClearLineOfSightBetweenUnits(
	UNIT *pUnitFrom,
	UNIT *pUnitTo)
{
	ASSERTX_RETFALSE( pUnitFrom, "Expected unit from" );
	ASSERTX_RETFALSE( pUnitTo, "Expected unit to" );
	
	// must be on save level
	ASSERTX_RETFALSE( UnitsAreInSameLevel( pUnitFrom, pUnitTo ), "Units must be in same level" );
	LEVEL *pLevel = UnitGetLevel( pUnitFrom );

	// check distance to center of units
	VECTOR vPosFrom = UnitGetCenter( pUnitFrom );
	VECTOR vPosTo = UnitGetCenter( pUnitTo );
	return ClearLineOfSightBetweenPositions( pLevel, vPosFrom, vPosTo );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct LEVEL_SPHERE_COLLIDE_DATA
{
	// testing data
	BOUNDING_BOX	tBox;
	VECTOR			vStart;
	VECTOR			vDirection;
	float			fLength;
	float			fRadius;
	RCSFLAGS		eFlags;
	// collision result info
	float			fDistanceToFirstCollision;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLevelSphereCollideLenRoom(
	ROOM *pRoom,
	void *pCallbackData )
{
	LEVEL_SPHERE_COLLIDE_DATA * data = ( LEVEL_SPHERE_COLLIDE_DATA * )pCallbackData;

	const BOUNDING_BOX * pBox = RoomGetBoundingBoxWorldSpace( pRoom );
	if ( BoundingBoxCollide( pBox, &data->tBox ) )
	{
		ROOM_COLLIDE_SPHERE_DATA results;
		VECTOR vPositionInRoom, vDirectionInRoom;
		MatrixMultiply( &vPositionInRoom, &data->vStart, &pRoom->tWorldMatrixInverse );
		VECTOR vEnd = data->vStart + data->vDirection;
		VECTOR vEndInRoom;
		MatrixMultiply( &vEndInRoom, &vEnd, &pRoom->tWorldMatrixInverse );
		vDirectionInRoom = vEndInRoom - vPositionInRoom;
		if ( sRoomCollideSphereDistance( pRoom, vPositionInRoom, vDirectionInRoom, data->fLength, data->fRadius, data->eFlags, results ) )
		{
			float fDistance = results.fResultDist;
			if ( results.bEmbedded )
				fDistance = 0.0f;
			if ( fDistance < data->fDistanceToFirstCollision )
				data->fDistanceToFirstCollision = results.fResultDist;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelSphereCollideLen(
	struct GAME * pGame, 
	struct LEVEL * pLevel,
	const VECTOR & vPosition, 
	const VECTOR & vDirection, 
	float fRadius,
	float fMaxLen,
	RCSFLAGS eFlags )
{
	LEVEL_SPHERE_COLLIDE_DATA data;
	data.tBox.vMin = vPosition;
	data.tBox.vMax = vPosition;
	VECTOR vEnd = vDirection * fMaxLen;
	vEnd += vPosition;
	BoundingBoxExpandByPoint( &data.tBox, &vEnd );
	BoundingBoxExpandBySize( &data.tBox, fRadius );
	data.vStart = vPosition;
	data.vDirection = vDirection;
	data.fRadius = fRadius;
	data.fLength = fMaxLen;
	data.fDistanceToFirstCollision = fMaxLen;
	data.eFlags = eFlags;

	LevelIterateRooms( pLevel, sLevelSphereCollideLenRoom, (void *)&data );

	return data.fDistanceToFirstCollision;
}
