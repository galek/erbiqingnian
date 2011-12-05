//----------------------------------------------------------------------------
// proxmap.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// TODO: cache closest node
// TODO: insertsorted, movesorted implementation
// TODO: search along longest axis
// TODO: edge-to-edge closest function for SphereMap
// TODO: hierarchichal map implementation
// TODO: box implementation
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "proxmap.h"

#define QUERY_DEBUG

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static float Square(
	float x)
{
	return x * x;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static float GetDistanceSq(
	float deltax,
	float deltay,
	float deltaz)
{
	return deltax * deltax + deltay * deltay + deltaz * deltaz;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE PointMap::Insert(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z)
{
	UNREFERENCED_PARAMETER(flags);

	PointNode * node = GetNewNode();
	node->id = id;
	node->c[X_AXIS] = x;
	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;

	AddNewNodeToMapUnsorted(node);

	return nodes.GetIndexByPtr(node);
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Insert()
// ---------------------------------------------------------------------------
PROXNODE PointMap::InsertSorted(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z)
{
	return Insert(id, flags, x, y, z);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis>
BOOL PointMap::CheckMoveAxisChange(
	PointNode * node,
	float newc)
{
	float oldc = node->c[axis];
	node->c[axis] = newc;

	if (!sorted[axis])
	{
		return FALSE;
	}

	if (newc == oldc)
	{
		return FALSE;
	}

	if (node->pos[axis] > list[axis])
	{
		PointNode * peek = *(node->pos[axis] - 1);
		if (newc < peek->c[axis])
		{
			return TRUE;
		}
	}
	if (node->pos[axis] < list[axis] + count - 1)
	{
		PointNode * peek = *(node->pos[axis] + 1);
		if (newc > peek->c[X_AXIS])
		{
			return TRUE;
		}
	}

	return FALSE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void PointMap::Move(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	PointNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	if (CheckMoveAxisChange<X_AXIS>(node, x))
	{
		sorted[X_AXIS] = FALSE;
	}
	if (CheckMoveAxisChange<Y_AXIS>(node, y))
	{
		sorted[Y_AXIS] = FALSE;
	}

	node->c[Z_AXIS] = z;
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Move()
// ---------------------------------------------------------------------------
void PointMap::MoveSorted(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	Move(nodeid, x, y, z);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis>
int PointCompare(
	const void * a,
	const void * b)
{
	ProxNode ** A = (ProxNode **)a;
	ProxNode ** B = (ProxNode **)b;

	if ((*A)->c[axis] < (*B)->c[axis])
	{
		return -1;
	}
	if ((*A)->c[axis] > (*B)->c[axis])
	{
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis> 
void PointMap::SortAxis(
	void)
{
	if (sorted[axis])
	{
		return;
	}

	// for now implement using qsort, note probably better algos for already sorted lists
	qsort(list[axis], count, sizeof(PointNode *), PointCompare<axis>);

	PointNode ** cur = list[axis];
	PointNode ** end = cur + count;
	while (cur < end)
	{
		(*cur)->pos[axis] = cur;
		cur++;
	}

	sorted[axis] = TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void PointMap::Sort(
	void)
{
	SortAxis<X_AXIS>();
	SortAxis<Y_AXIS>();
}


// ---------------------------------------------------------------------------
// todo: for cache reasons, this may not be the best search pattern
// ---------------------------------------------------------------------------
PointNode * PointMap::GetClosestNode(
	PointNode * node,
	float max)
{
	#define CCPoint(ptr, ax, st, dir, ch)		{ PointNode * n = *ptr; \
													if ((n->c[ax] - pt[ax]) * (n->c[ax] - pt[ax]) > bestdist) { \
														state |= st; } \
													else { \
														float dist = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]); \
														if (dist < bestdist) { \
															bestnode = n; \
															bestdist = dist; } \
														ptr += dir; if (ptr ch) state |= st; } }

	#define CCPtLeft()							CCPoint(ptr_xl, X_AXIS, 0x1, -1, < list[X_AXIS])
	#define CCPtRight()							CCPoint(ptr_xr, X_AXIS, 0x2, +1, >= end_x)
	#define CCPtUp()							CCPoint(ptr_yl, Y_AXIS, 0x4, -1, < list[Y_AXIS])
	#define CCPtDown()							CCPoint(ptr_yr, Y_AXIS, 0x8, +1, >= end_y)

	ASSERT_RETNULL(node);

	Sort();

	float bestdist = max * max;
	PointNode * bestnode = NULL;

	float pt[3] = { node->c[X_AXIS], node->c[Y_AXIS], node->c[Z_AXIS] };
	unsigned int state = 0;
	PointNode ** end_x = list[X_AXIS] + count;
	PointNode ** ptr_xl = node->pos[X_AXIS] - 1;	if (ptr_xl < list[X_AXIS]) state |= 0x1;
	PointNode ** ptr_xr = node->pos[X_AXIS] + 1;	if (ptr_xr >= end_x) state |= 0x2;
	PointNode ** end_y = list[Y_AXIS] + count;
	PointNode ** ptr_yl = node->pos[Y_AXIS] - 1;	if (ptr_yl < list[Y_AXIS]) state |= 0x4;
	PointNode ** ptr_yr = node->pos[Y_AXIS] + 1;	if (ptr_yr >= end_y) state |= 0x8;

	while (1)
	{
		switch (state)
		{
		case  0:	CCPtLeft();		CCPtRight();	CCPtUp();		CCPtDown();		break;
		case  1:					CCPtRight();	CCPtUp();		CCPtDown();		break;
		case  2:	CCPtLeft();						CCPtUp();		CCPtDown();		break;
		case  3:									CCPtUp();		CCPtDown();		break;
		case  4:	CCPtLeft();		CCPtRight();					CCPtDown();		break;
		case  5:					CCPtRight();					CCPtDown();		break;
		case  6:	CCPtLeft();										CCPtDown();		break;
		case  7:													CCPtDown();		break;
		case  8:	CCPtLeft();		CCPtRight();	CCPtUp();						break;
		case  9:					CCPtRight();	CCPtUp();						break;
		case 10:	CCPtLeft();						CCPtUp();						break;
		case 11:									CCPtUp();						break;
		case 12:	CCPtLeft();		CCPtRight();									break;
		case 13:					CCPtRight();									break;
		case 14:	CCPtLeft();														break;
		case 15:	goto _done;
		}
	}

	#undef CCPoint
	#undef CCPtLeft
	#undef CCPtRight
	#undef CCPtUp
	#undef CCPtDown

_done:
	return bestnode;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD PointMap::GetClosest(
	PROXNODE nodeid,
	float max)
{
	PointNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	PointNode * best = GetClosestNode(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD PointMap::GetClosest(
	PROXNODE nodeid,
	float & x,
	float & y,
	float & z,
	float max)
{
	PointNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	PointNode * best = GetClosestNode(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	x = best->c[X_AXIS];
	y = best->c[Y_AXIS];
	z = best->c[Z_AXIS];
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis> 
PointNode * PointMap::GetClosestNodeOnAxis(
	float f)
{
	PointNode ** min = list[axis];
	PointNode ** max = min + count;
	PointNode ** cur = min + count / 2;
	while (max > min)
	{
		float cf = (*cur)->c[axis];

		if (cf > f)
		{
			max = cur;
		}
		else if (cf < f)
		{
			min = cur + 1;
		}
		else
		{
			return *cur;
		}
		cur = min + (max - min) / 2;
	}

	if (max >= list[axis] + count)
	{
		--max;
	}
	return *max;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD PointMap::GetClosest(
	float x,
	float y,
	float z,
	float max)
{
	if (count == 0)
	{
		return INVALID_ID;
	}

	Sort();

	float bestdist = max * max;
	PointNode * bestnode = NULL;

	PointNode * centerx = GetClosestNodeOnAxis<X_AXIS>(x);
	PointNode * centery = GetClosestNodeOnAxis<Y_AXIS>(y);

	ASSERT_RETINVALID(centerx);
	ASSERT_RETINVALID(centery);

	#define CCPoint(ptr, ax, st, dir, ch)		{ PointNode * n = *ptr; \
													if ((n->c[ax] - pt[ax]) * (n->c[ax] - pt[ax]) > bestdist) { \
														state |= st; } \
													else { \
														float dist = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]); \
														if (dist < bestdist) { \
															bestnode = n; \
															bestdist = dist; } \
														ptr += dir; if (ptr ch) state |= st; } }

	#define CCPtLeft()							CCPoint(ptr_xl, X_AXIS, 0x1, -1, < list[X_AXIS])
	#define CCPtRight()							CCPoint(ptr_xr, X_AXIS, 0x2, +1, >= end_x)
	#define CCPtUp()							CCPoint(ptr_yl, Y_AXIS, 0x4, -1, < list[Y_AXIS])
	#define CCPtDown()							CCPoint(ptr_yr, Y_AXIS, 0x8, +1, >= end_y)

	float pt[3] = { x, y, z };

	unsigned int state = 0;
	PointNode ** end_x = list[X_AXIS] + count;
	PointNode ** ptr_xl = centerx->pos[X_AXIS];		if (ptr_xl < list[X_AXIS]) state |= 0x1;
	PointNode ** ptr_xr = centerx->pos[X_AXIS] + 1;	if (ptr_xr >= end_x) state |= 0x2;
	PointNode ** end_y = list[Y_AXIS] + count;
	PointNode ** ptr_yl = centery->pos[Y_AXIS];		if (ptr_yl < list[Y_AXIS]) state |= 0x4;
	PointNode ** ptr_yr = centery->pos[Y_AXIS] + 1;	if (ptr_yr >= end_y) state |= 0x8;

	while (1)
	{
		switch (state)
		{
		case  0:	CCPtLeft();		CCPtRight();	CCPtUp();		CCPtDown();		break;
		case  1:					CCPtRight();	CCPtUp();		CCPtDown();		break;
		case  2:	CCPtLeft();						CCPtUp();		CCPtDown();		break;
		case  3:									CCPtUp();		CCPtDown();		break;
		case  4:	CCPtLeft();		CCPtRight();					CCPtDown();		break;
		case  5:					CCPtRight();					CCPtDown();		break;
		case  6:	CCPtLeft();										CCPtDown();		break;
		case  7:													CCPtDown();		break;
		case  8:	CCPtLeft();		CCPtRight();	CCPtUp();						break;
		case  9:					CCPtRight();	CCPtUp();						break;
		case 10:	CCPtLeft();						CCPtUp();						break;
		case 11:									CCPtUp();						break;
		case 12:	CCPtLeft();		CCPtRight();									break;
		case 13:					CCPtRight();									break;
		case 14:	CCPtLeft();														break;
		case 15:	goto _done;
		}
	}

	#undef CCPoint
	#undef CCPtLeft
	#undef CCPtRight
	#undef CCPtUp
	#undef CCPtDown

_done:
	if (!bestnode)
	{
		return INVALID_ID;
	}
	return bestnode->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, int dir>
PointNode * PointMap::QueryAxis(
	PointNode * node,
	PointNode * prev,
	PointNode ** end,
	float range)
{
	float rangesq = range * range;

	float pt[3];
	pt[X_AXIS] = node->c[X_AXIS];
	pt[Y_AXIS] = node->c[Y_AXIS];
	pt[Z_AXIS] = node->c[Z_AXIS];

	PointNode ** ptr = node->pos[axis] + dir;
	while (ptr != end)
	{
		PointNode * n = *ptr;

		if (dir > 0)		// in release, this will get optimized out since dir is constant
		{
			if ((n->c[axis] - pt[axis]) > range)
			{
				break;
			}
		}
		else
		{
			if ((pt[axis] - n->c[axis]) > range)
			{
				break;
			}
		}
		float dist = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
		if (dist <= rangesq) 
		{
			prev->next = n;
			prev = n; 
		}
		ptr += dir;
	}
	return prev;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE PointMap::Query(
	PROXNODE nodeid,
	float range)
{
	PointNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	if (range < 0.0f)
	{
		return NULL;
	}

	Sort();

	PointNode * prev = node;

	prev = QueryAxis<X_AXIS, -1>(node, prev, list[X_AXIS] - 1, range);
	prev = QueryAxis<X_AXIS, +1>(node, prev, list[X_AXIS] + count, range);

	prev->next = NULL;
	if (!node->next)
	{
		return INVALID_ID;
	}
	return nodes.GetIndexByPtr((PointNode *)node->next);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE PointMap::Query(
	float x,
	float y,
	float z,
	float range)
{
	if (count == 0)
	{
		return INVALID_ID;
	}

	if (range < 0.0f)
	{
		return INVALID_ID;
	}

	Sort();

	float pt[3] = { x, y, z };
	float rangesq = range * range;

	PointNode * far_xl = GetClosestNodeOnAxis<X_AXIS>(x - range);
	PointNode * far_xr = GetClosestNodeOnAxis<X_AXIS>(x + range);

	ASSERT_RETINVALID(far_xl);
	ASSERT_RETINVALID(far_xr);

	PointNode ** ptr = far_xl->pos[X_AXIS];
	PointNode ** max = far_xr->pos[X_AXIS];
	
	PointNode * list = NULL;
	PointNode * curr = NULL;

	while (ptr <= max)
	{
		float dist = GetDistanceSq((*ptr)->c[X_AXIS] - pt[X_AXIS], (*ptr)->c[Y_AXIS] - pt[Y_AXIS], (*ptr)->c[Z_AXIS] - pt[Z_AXIS]);
		if (dist <= rangesq) 
		{
			if (curr)
			{
				curr->next = *ptr;
				curr = (PointNode *)(curr->next);
			}
			else
			{
				list = curr = *ptr;
			}
		}
		++ptr;
	}

	if (curr)
	{
		curr->next = NULL;
	}
	return (list ? list->_idx : INVALID_ID);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL PointMap::GetPosition(
	PROXNODE nodeid,
	float & x,
	float & y,
	float & z)
{
	PointNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETFALSE(node);

	x = node->x;
	y = node->y;
	z = node->z;

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE SphereMap::Insert(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z,
	float radius)
{
	UNREFERENCED_PARAMETER(flags);

	SphereNode * node = GetNewNode();
	node->id = id;
	node->c[X_AXIS] = x;
	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;
	node->radius = radius;

	AddNewNodeToMapUnsorted(node);

	return nodes.GetIndexByPtr(node);
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Insert()
// ---------------------------------------------------------------------------
PROXNODE SphereMap::InsertSorted(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z,
	float radius)
{
	return Insert(id, flags, x, y, z, radius);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int edge>
void SphereMap::CheckMoveAxisChanged(
	SphereNode * node,
	float newc)
{
	float oldc = node->c[axis];
	node->c[axis] = newc;

	if (!sorted[smlist])
	{
		return;
	}

	if (newc != oldc)
	{
		if (node->pos[smlist] > list[smlist])
		{
			SphereNode * peek = *(node->pos[smlist] - 1);
			if (newc + node->radius * edge < peek->c[axis] + peek->radius * edge)		// multiply by edge will be optimized out
			{
				sorted[smlist] = FALSE;
				return;
			}
		}
		if (node->pos[smlist] < list[smlist] + count - 1)
		{
			SphereNode * peek = *(node->pos[smlist] + 1);
			if (newc + node->radius * edge > peek->c[smlist] + peek->radius * edge)
			{
				sorted[smlist] = FALSE;
				return;
			}
		}
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SphereMap::Move(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	UNREFERENCED_PARAMETER(y);

	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	CheckMoveAxisChanged<X_AXIS, SM_XAXIS_LEDGE, -1>(node, x);
	CheckMoveAxisChanged<X_AXIS, SM_XAXIS_REDGE, +1>(node, x);

	node->c[Z_AXIS] = z;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SphereMap::Resize(
	PROXNODE nodeid,
	float radius)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	node->radius = radius;

	sorted[SM_XAXIS_LEDGE] = FALSE;
	sorted[SM_XAXIS_REDGE] = FALSE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SphereMap::MoveResize(
	PROXNODE nodeid,
	float x,
	float y,
	float z,
	float radius)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	node->c[X_AXIS] = x;
	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;
	node->radius = radius;

	sorted[SM_XAXIS_LEDGE] = FALSE;
	sorted[SM_XAXIS_REDGE] = FALSE;
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Move()
// ---------------------------------------------------------------------------
void SphereMap::MoveSorted(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	Move(nodeid, x, y, z);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, int edge>
int SphereCompare(
	const void * a,
	const void * b)
{
	SphereNode ** A = (SphereNode **)a;
	SphereNode ** B = (SphereNode **)b;

	if ((*A)->c[axis] + (*A)->radius * edge < (*B)->c[axis] + (*B)->radius * edge)		// edge multiply will be optimized out
	{
		return -1;
	}
	if ((*A)->c[axis] + (*A)->radius * edge > (*B)->c[axis] + (*B)->radius * edge)
	{
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int edge> 
void SphereMap::SortAxis(
	void)
{
	if (sorted[smlist])
	{
		return;
	}

	// for now implement using qsort, note probably better algos for already sorted lists
	qsort(list[smlist], count, sizeof(SphereNode *), SphereCompare<axis, edge>);

	// fix up pos reference in nodes O(N)
	SphereNode ** cur = list[smlist];
	SphereNode ** end = cur + count;
	while (cur < end)
	{
		(*cur)->pos[smlist] = cur;
		cur++;
	}

	sorted[smlist] = TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SphereMap::Sort(
	void)
{
	SortAxis<X_AXIS, SM_XAXIS_LEDGE, -1>();
	SortAxis<X_AXIS, SM_XAXIS_REDGE, +1>();
}


// ---------------------------------------------------------------------------
// compute box-to-box distance using closest feature
// or return center-to-center distance + intersect if boxes intersect
// ---------------------------------------------------------------------------
float SphereMap::GetBoxDistanceSq(
	SphereNode * a,
	SphereNode * b,
	BOOL & intersect)
{
	#define SM_RT(obj, axis)				(obj->c[axis] + obj->radius)
	#define SM_LF(obj, axis)				(obj->c[axis] - obj->radius)

	intersect = FALSE;

	if (a->c[X_AXIS] + a->radius < b->c[X_AXIS] - b->radius)
	{
		if (a->c[Y_AXIS] + a->radius < b->c[Y_AXIS] - b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-LLL to a-RRR
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-LLR to a-RRL
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-LL0 to a-RR0
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius > b->c[Y_AXIS] + b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-LRL to a-RLR
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-LRR to a-RLL
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-LR0 to a-RL0
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else	
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-L0L to a-R0R
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-L0R to a-R0L
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-L00 to a-R00
				return GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0,									 0);
			}
		}
	}
	else if (a->c[X_AXIS] - a->radius > b->c[X_AXIS] + b->radius)
	{
		if (a->c[Y_AXIS] + a->radius < b->c[Y_AXIS] - b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-RLL to a-LRR
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-RLR to a-LRL
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-RL0 to a-LR0
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius > b->c[Y_AXIS] + b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-RRL to a-LLR
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-RRR to a-LLL
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-RR0 to a-LL0
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-R0L to a-L0R
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-R0R to a-L0L
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-R00 to a-L00
				return GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0,									 0);
			}
		}
	}
	else	
	{
		if (a->c[Y_AXIS] + a->radius < b->c[Y_AXIS] - b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-0LL to a-0RR
				return GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-0LR to a-0RL
				return GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-0L0 to a-0R0
				return GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius > b->c[Y_AXIS] + b->radius)
		{
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-0RL to a-0LR
				return GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-0RR to a-0LL
				return GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-0R0 to a-0L0
				return GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else
		{	
			if (a->c[Z_AXIS] + a->radius < b->c[Z_AXIS] - b->radius)
			{	// use b-00L to a-00R
				return GetDistanceSq(								   0,									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius > b->c[Z_AXIS] + b->radius)
			{	// use b-00R to a-00L
				return GetDistanceSq(								   0,									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}			
			else
			{	// 2 boxes intersect, use center-to-center distance
				intersect = TRUE;		
				return GetDistanceSq(a->c[X_AXIS] - b->c[X_AXIS], a->c[Y_AXIS] - b->c[Y_AXIS], a->c[Z_AXIS] - b->c[Z_AXIS]);
			}
		}
	}

	#undef SM_RT
	#undef SM_LF
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir>
SphereNode * SphereMap::GetClosestBoxOnAxis(
	SphereNode * node,
	SphereNode * best,
	SphereNode ** end,
	BOOL & intersect,
	float & bestsq)
{
	SphereNode ** ptr = node->pos[axis] + dir;
	while (ptr != end)
	{
		SphereNode * n = *ptr;

		if (intersect)
		{
			if (n->c[axis] + n->radius * -dir < node->c[axis] + n->radius * dir)	// abort if no longer possible to intersect
			{
				break;
			}
			BOOL isect;
			float distsq = GetBoxDistanceSq(node, n, isect);
			if (isect && distsq < bestsq)
			{
				bestsq = distsq;
				best = n;
			}
		}
		else
		{
			if (Square(node->c[axis] + node->radius - n->c[axis] + n->radius) > bestsq)
			{
				break;
			}
			float distsq = GetBoxDistanceSq(node, n, intersect);
			if (intersect || distsq < bestsq)
			{
				bestsq = distsq;
				best = n;
			}
		}

		ptr += dir;
	}

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
SphereNode * SphereMap::GetClosestBox(
	SphereNode * node,
	float max)
{
	ASSERT_RETNULL(node);

	Sort();

	BOOL intersect = FALSE;
	float bestsq = max * max;

	SphereNode * best;
	best = GetClosestBoxOnAxis<X_AXIS, SM_XAXIS_LEDGE, -1>(node, NULL, list[X_AXIS] - 1, intersect, bestsq);
	best = GetClosestBoxOnAxis<X_AXIS, SM_XAXIS_REDGE, 1>(node, best, list[X_AXIS] + count, intersect, bestsq);

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD SphereMap::GetClosestBox(
	PROXNODE nodeid,
	float max)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	SphereNode * best = GetClosestBox(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD SphereMap::GetClosestBox(
	PROXNODE nodeid,
	float & x,
	float & y,
	float & z,
	float max)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	SphereNode * best = GetClosestBox(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	x = best->c[X_AXIS];
	y = best->c[Y_AXIS];
	z = best->c[Z_AXIS];
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir>
SphereNode * SphereMap::GetClosestSphereOnAxis(
	SphereNode * node,
	SphereNode * best,
	SphereNode ** end,
	BOOL & intersect,
	float & bestdist)
{
	float pt[3] = { node->x, node->y, node->z };
	float radius = node->radius;
	float bestsq = Square(bestdist);

	SphereNode ** ptr = node->pos[axis] + dir;
	while (ptr != end)
	{
		SphereNode * n = *ptr;

		if (intersect)
		{
			if (n->c[axis] + n->radius * -dir < pt[axis] + radius * dir)	// abort if no longer possible to intersect
			{
				break;
			}
			float centdistsq = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
			if (centdistsq < Square(radius + n->radius))
			{
				if (centdistsq < bestsq)
				{
					bestsq = centdistsq;
					best = n;
				}
			}
		}
		else
		{
			if (Square(pt[axis] + radius - n->c[axis] + n->radius) > bestsq)	// abort if no longer possible to get closer
			{
				break;
			}
			float centdistsq = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
			if (centdistsq < Square(bestdist + radius + n->radius))
			{
				bestdist = sqrtf(centdistsq) - radius - n->radius;
				if (bestdist <= 0.0f)
				{
					bestdist = 0.0f;
					bestsq = centdistsq;
					intersect = TRUE;
				}
				else
				{
					bestsq = Square(bestdist);
				}
				best = n;
			}
		}
		ptr += dir;
	}

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
SphereNode * SphereMap::GetClosestSphere(
	SphereNode * node,
	float max,
	float * dist)
{
	ASSERT_RETNULL(node);

	Sort();

	BOOL intersect = FALSE;
	float bestdist = max;

	SphereNode * best;
	best = GetClosestSphereOnAxis<X_AXIS, SM_XAXIS_LEDGE, -1>(node, NULL, list[X_AXIS] - 1, intersect, bestdist);
	best = GetClosestSphereOnAxis<X_AXIS, SM_XAXIS_REDGE, 1>(node, best, list[X_AXIS] + count, intersect, bestdist);

	if (dist)
	{
		*dist = bestdist;
	}

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD SphereMap::GetClosestSphere(
	PROXNODE nodeid,
	float max,
	float * dist)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	SphereNode * best = GetClosestSphere(node, max, dist);
	if (!best)
	{
		return INVALID_ID;
	}
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir> 
SphereNode * SphereMap::GetClosestNodeOnAxis(
	float f)
{
	SphereNode ** min = list[smlist];
	SphereNode ** max = min + count;
	SphereNode ** cur = min + count / 2;
	while (max > min)
	{
		float cf = (*cur)->c[axis] + (*cur)->radius * dir;

		if (cf > f)
		{
			max = cur;
		}
		else if (cf < f)
		{
			min = cur + 1;
		}
		else
		{
			return *cur;
		}
		cur = min + (max - min) / 2;
	}

	if (max >= list[axis] + count)
	{
		--max;
	}
	return *max;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, int dir>
SphereNode * SphereMap::QueryAxis(
	SphereNode * node,
	SphereNode * prev,
	SphereNode ** end,
	float range)
{
	float pt[3];
	pt[X_AXIS] = node->c[X_AXIS];
	pt[Y_AXIS] = node->c[Y_AXIS];
	pt[Z_AXIS] = node->c[Z_AXIS];
	float radius = node->radius;

	SphereNode ** ptr = node->pos[axis] + dir;
	while (ptr != end)
	{
		SphereNode * n = *ptr;

		if (((n->c[axis] + n->radius * -dir) - (pt[axis] + radius * dir)) * dir > range)
		{
			break;
		}
		float dist = GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
		if (dist < Square(range + radius + n->radius)) 
		{
			prev->next = n;
			prev = n; 
		}
		ptr += dir;
	}
	return prev;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE SphereMap::Query(
	PROXNODE nodeid,
	float range)
{
	SphereNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	if (range < 0.0f)
	{
		return INVALID_ID;
	}

	Sort();

	SphereNode * prev = node;

	prev = QueryAxis<X_AXIS, -1>(node, prev, list[X_AXIS] - 1, range);
	prev = QueryAxis<X_AXIS, +1>(node, prev, list[X_AXIS] + count, range);

	prev->next = NULL; 
	if (!node->next)
	{
		return INVALID_ID;
	}
	return nodes.GetIndexByPtr((SphereNode *)node->next);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE SphereMap::Query(
	const VECTOR & p,
	float range)
{
	if (range < 0.0f)
	{
		return INVALID_ID;
	}

	Sort();

	SphereNode * far_xl = GetClosestNodeOnAxis<X_AXIS, SM_XAXIS_LEDGE, -1>(p.x - range); // find leftmost sphere whose left edge is closest to x - range
	SphereNode * far_xr = GetClosestNodeOnAxis<X_AXIS, SM_XAXIS_REDGE, +1>(p.x + range); // find rightmost sphere whose right edge is closest to x + range

	ASSERT_RETINVALID(far_xl);
	ASSERT_RETINVALID(far_xr);

	SphereNode ** ptr = far_xl->pos[X_AXIS];
	SphereNode ** max = far_xr->pos[X_AXIS];
	
	SphereNode * list = NULL;
	SphereNode *& curr = list;

	while (ptr <= max)
	{
		SphereNode * n = *ptr;

		float cdistsq = GetDistanceSq(n->c[X_AXIS] - p.x, n->c[Y_AXIS] - p.y, n->c[Z_AXIS] - p.z);
		if (cdistsq < Square(range + n->radius)) 
		{
			curr = *ptr;
			curr = (SphereNode *)(curr->next);
		}
		++ptr;
	}

	if (curr)
	{
		curr->next = NULL;
	}
	return (list ? list->_idx : INVALID_ID);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE SphereMap::Query(
	float x,
	float y,
	float z,
	float range)
{
	return Query(VECTOR(x, y, z), range);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE BoxMap::Insert(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z,
	float xradius,
	float yradius,
	float zradius)
{
	UNREFERENCED_PARAMETER(flags);

	BoxNode * node = GetNewNode();
	node->id = id;
	node->c[X_AXIS] = x;
	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;
	node->radius[X_AXIS] = xradius;
	node->radius[Y_AXIS] = yradius;
	node->radius[Z_AXIS] = zradius;

	AddNewNodeToMapUnsorted(node);

	return nodes.GetIndexByPtr(node);
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Insert()
// ---------------------------------------------------------------------------
PROXNODE BoxMap::InsertSorted(
	DWORD id,
	DWORD flags,
	float x,
	float y,
	float z,
	float xradius,
	float yradius,
	float zradius)
{
	return Insert(id, flags, x, y, z, xradius, yradius, zradius);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int edge>
void BoxMap::CheckMoveAxisChanged(
	BoxNode * node,
	float newc)
{
	float oldc = node->c[axis];
	node->c[axis] = newc;

	if (!sorted[smlist])
	{
		return;
	}

	if (newc == oldc)
	{
		return;
	}

	if (node->pos[smlist] > list[smlist])
	{
		BoxNode * peek = *(node->pos[smlist] - 1);
		if (newc + node->radius[axis] * edge < peek->c[axis] + peek->radius[axis] * edge)		// multiply by edge will be optimized out
		{
			sorted[smlist] = FALSE;
			return;
		}
	}
	if (node->pos[smlist] < list[smlist] + count - 1)
	{
		BoxNode * peek = *(node->pos[smlist] + 1);
		if (newc + node->radius[axis] * edge > peek->c[smlist] + peek->radius[axis] * edge)
		{
			sorted[smlist] = FALSE;
			return;
		}
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMap::Move(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	CheckMoveAxisChanged<X_AXIS, SM_XAXIS_LEDGE, -1>(node, x);
	CheckMoveAxisChanged<X_AXIS, SM_XAXIS_REDGE, +1>(node, x);

	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;
}


// ---------------------------------------------------------------------------
// todo: implement this.  for now, just call Move()
// ---------------------------------------------------------------------------
void BoxMap::MoveSorted(
	PROXNODE nodeid,
	float x,
	float y,
	float z)
{
	Move(nodeid, x, y, z);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMap::Resize(
	PROXNODE nodeid,
	float xradius,
	float yradius,
	float zradius)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	node->radius[X_AXIS] = xradius;
	node->radius[Y_AXIS] = yradius;
	node->radius[Z_AXIS] = zradius;

	sorted[SM_XAXIS_LEDGE] = FALSE;
	sorted[SM_XAXIS_REDGE] = FALSE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMap::MoveResize(
	PROXNODE nodeid,
	float x,
	float y,
	float z,
	float xradius,
	float yradius,
	float zradius)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETURN(node);

	node->c[X_AXIS] = x;
	node->c[Y_AXIS] = y;
	node->c[Z_AXIS] = z;
	node->radius[X_AXIS] = xradius;
	node->radius[Y_AXIS] = yradius;
	node->radius[Z_AXIS] = zradius;

	sorted[SM_XAXIS_LEDGE] = FALSE;
	sorted[SM_XAXIS_REDGE] = FALSE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, int edge>
int BoxCompare(
	const void * a,
	const void * b)
{
	BoxNode ** A = (BoxNode **)a;
	BoxNode ** B = (BoxNode **)b;

	if ((*A)->c[axis] + (*A)->radius[axis] * edge < (*B)->c[axis] + (*B)->radius[axis] * edge)		// edge multiply will be optimized out
	{
		return -1;
	}
	if ((*A)->c[axis] + (*A)->radius[axis] * edge > (*B)->c[axis] + (*B)->radius[axis] * edge)
	{
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int edge> 
void BoxMap::SortAxis(
	void)
{
	if (sorted[smlist])
	{
		return;
	}

	// for now implement using qsort, note probably better algos for already sorted lists
	qsort(list[smlist], count, sizeof(BoxNode *), BoxCompare<axis, edge>);

	// fix up pos reference in nodes O(N)
	BoxNode ** cur = list[smlist];
	BoxNode ** end = cur + count;
	while (cur < end)
	{
		(*cur)->pos[smlist] = cur;
		cur++;
	}

	sorted[smlist] = TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMap::Sort(
	void)
{
	SortAxis<X_AXIS, SM_XAXIS_LEDGE, -1>();
	SortAxis<X_AXIS, SM_XAXIS_REDGE, +1>();
}


// ---------------------------------------------------------------------------
// compute box-to-box distance using closest feature
// or return center-to-center distance + intersect if boxes intersect
// ---------------------------------------------------------------------------
float BoxMap::GetBoxDistanceSq(
	BoxNode * a,
	BoxNode * b,
	BOOL & intersect)
{
	#define SM_RT(obj, axis)				(obj->c[axis] + obj->radius[axis])
	#define SM_LF(obj, axis)				(obj->c[axis] - obj->radius[axis])

	intersect = FALSE;

	if (a->c[X_AXIS] + a->radius[X_AXIS] < b->c[X_AXIS] - b->radius[X_AXIS])
	{
		if (a->c[Y_AXIS] + a->radius[Y_AXIS] < b->c[Y_AXIS] - b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-LLL to a-RRR
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-LLR to a-RRL
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-LL0 to a-RR0
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius[Y_AXIS] > b->c[Y_AXIS] + b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-LRL to a-RLR
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-LRR to a-RLL
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-LR0 to a-RL0
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else	
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-L0L to a-R0R
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-L0R to a-R0L
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-L00 to a-R00
				return ::GetDistanceSq(SM_LF(b, X_AXIS) - SM_RT(a, X_AXIS),									0,									 0);
			}
		}
	}
	else if (a->c[X_AXIS] - a->radius[X_AXIS] > b->c[X_AXIS] + b->radius[X_AXIS])
	{
		if (a->c[Y_AXIS] + a->radius[Y_AXIS] < b->c[Y_AXIS] - b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-RLL to a-LRR
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-RLR to a-LRL
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-RL0 to a-LR0
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius[Y_AXIS] > b->c[Y_AXIS] + b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-RRL to a-LLR
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-RRR to a-LLL
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-RR0 to a-LL0
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS), SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-R0L to a-L0R
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-R0R to a-L0L
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-R00 to a-L00
				return ::GetDistanceSq(SM_RT(b, X_AXIS) - SM_LF(a, X_AXIS),									0,									 0);
			}
		}
	}
	else	
	{
		if (a->c[Y_AXIS] + a->radius[Y_AXIS] < b->c[Y_AXIS] - b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-0LL to a-0RR
				return ::GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-0LR to a-0RL
				return ::GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-0L0 to a-0R0
				return ::GetDistanceSq(								   0, SM_LF(b, Y_AXIS) - SM_RT(a, Y_AXIS),									 0);
			}
		}
		else if (a->c[Y_AXIS] - a->radius[Y_AXIS] > b->c[Y_AXIS] + b->radius[Y_AXIS])
		{
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-0RL to a-0LR
				return ::GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-0RR to a-0LL
				return ::GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS), SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}
			else
			{	// use b-0R0 to a-0L0
				return ::GetDistanceSq(								   0, SM_RT(b, Y_AXIS) - SM_LF(a, Y_AXIS),									 0);
			}
		}
		else
		{	
			if (a->c[Z_AXIS] + a->radius[Z_AXIS] < b->c[Z_AXIS] - b->radius[Z_AXIS])
			{	// use b-00L to a-00R
				return ::GetDistanceSq(								   0,									0, SM_LF(b, Z_AXIS) - SM_RT(a, Z_AXIS));
			}
			else if (a->c[Z_AXIS] - a->radius[Z_AXIS] > b->c[Z_AXIS] + b->radius[Z_AXIS])
			{	// use b-00R to a-00L
				return ::GetDistanceSq(								   0,									0, SM_RT(b, Z_AXIS) - SM_LF(a, Z_AXIS));
			}			
			else
			{	// 2 boxes intersect, use center-to-center distance
				intersect = TRUE;		
				return ::GetDistanceSq(a->c[X_AXIS] - b->c[X_AXIS], a->c[Y_AXIS] - b->c[Y_AXIS], a->c[Z_AXIS] - b->c[Z_AXIS]);
			}
		}
	}

	#undef SM_RT
	#undef SM_LF
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
float BoxMap::GetDistanceSq(
	PROXNODE nodeid1,
	PROXNODE nodeid2,
	BOOL & intersect)
{
	intersect = FALSE;

	BoxNode * node1 = nodes.GetByIndex(nodeid1);
	ASSERT_RETVAL(node1, INVALID_FLOAT);

	BoxNode * node2 = nodes.GetByIndex(nodeid2);
	ASSERT_RETVAL(node1, INVALID_FLOAT);

	return GetBoxDistanceSq(node1, node2, intersect);
}


// ---------------------------------------------------------------------------
// arvo's
// ---------------------------------------------------------------------------
float BoxMap::GetBoxDistanceSq(
	BoxNode * node,
	const VECTOR & p,
	float r)
{
	UNREFERENCED_PARAMETER(r);
    float d = 0; 

	if (p.x  < node->c[0] - node->radius[0])
	{
		float s = p.x - (node->c[0] - node->radius[0]);
		d += s * s;
	}
	else if (p.x  > node->c[0] + node->radius[0])
	{
		float s = p.x  - (node->c[0] + node->radius[0]);
		d += s * s;
	}

	if (p.y < node->c[1] - node->radius[1])
	{
		float s = p.y - (node->c[1] - node->radius[1]);
		d += s * s;
	}
	else if (p.y > node->c[1] + node->radius[1])
	{
		float s = p.y - (node->c[1] + node->radius[1]);
		d += s * s;
	}

	if (p.z < node->c[2] - node->radius[2])
	{
		float s = p.z - (node->c[2] - node->radius[2]);
		d += s * s;
	}
	else if (p.z > node->c[2] + node->radius[2])
	{
		float s = p.z - (node->c[2] + node->radius[2]);
		d += s * s;
	}

	return d;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir>
BoxNode * BoxMap::GetClosestBoxOnAxis(
	BoxNode * node,
	BoxNode * best,
	BoxNode ** end,
	BOOL & intersect,
	float & bestsq)
{
	BoxNode ** ptr = node->pos[axis] + dir;
	while (ptr != end)
	{
		BoxNode * n = *ptr;

		if (intersect)
		{
			if (n->c[axis] + n->radius[axis] * -dir < node->c[axis] + n->radius[axis] * dir)	// abort if no longer possible to intersect
			{
				break;
			}
			BOOL isect;
			float distsq = GetBoxDistanceSq(node, n, isect);
			if (isect && distsq < bestsq)
			{
				bestsq = distsq;
				best = n;
			}
		}
		else
		{
			if (Square(node->c[axis] + node->radius[axis] - n->c[axis] + n->radius[axis]) > bestsq)
			{
				break;
			}
			float distsq = GetBoxDistanceSq(node, n, intersect);
			if (intersect || distsq < bestsq)
			{
				bestsq = distsq;
				best = n;
			}
		}

		ptr += dir;
	}

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BoxNode * BoxMap::GetClosestBox(
	BoxNode * node,
	float max)
{
	ASSERT_RETNULL(node);

	Sort();

	BOOL intersect = FALSE;
	float bestsq = max * max;

	BoxNode * best;
	best = GetClosestBoxOnAxis<X_AXIS, SM_XAXIS_LEDGE, -1>(node, NULL, list[X_AXIS] - 1, intersect, bestsq);
	best = GetClosestBoxOnAxis<X_AXIS, SM_XAXIS_REDGE, 1>(node, best, list[X_AXIS] + count, intersect, bestsq);

	return best;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD BoxMap::GetClosestBox(
	PROXNODE nodeid,
	float max)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	BoxNode * best = GetClosestBox(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD BoxMap::GetClosestBox(
	PROXNODE nodeid,
	float & x,
	float & y,
	float & z,
	float max)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	BoxNode * best = GetClosestBox(node, max);
	if (!best)
	{
		return INVALID_ID;
	}
	x = best->c[X_AXIS];
	y = best->c[Y_AXIS];
	z = best->c[Z_AXIS];
	return best->id;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir>
BoxNode * BoxMap::QueryAxis(
	BoxNode * node,
	BoxNode * prev,
	BoxNode ** begin,
	BoxNode ** end,
	float range)
{
	const float eps = 0.001f;

	float pt[3];
	pt[X_AXIS] = node->c[X_AXIS];
	pt[Y_AXIS] = node->c[Y_AXIS];
	pt[Z_AXIS] = node->c[Z_AXIS];
	float radius[3];
	radius[X_AXIS] = node->radius[X_AXIS];
	radius[Y_AXIS] = node->radius[Y_AXIS];
	radius[Z_AXIS] = node->radius[Z_AXIS];

	BoxNode ** ptr = node->pos[smlist] - dir;
	while (ptr != begin)
	{
		BoxNode * n = *ptr;

		if (((n->c[axis] + n->radius[axis] * -dir) - (pt[axis] + radius[axis] * -dir)) > eps * -dir )
		{
			break;
		}
		float dist = ::GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
		if (dist < Square(range + radius[axis] + n->radius[axis])) 
		{
			prev->next = n;
			prev = n; 
		}
		ptr -= dir;
	}


	ptr = node->pos[smlist] + dir;
	while (ptr != end)
	{
		BoxNode * n = *ptr;

		if (((n->c[axis] + n->radius[axis] * -dir) - (pt[axis] + radius[axis] * dir)) * dir > range)
		{
			break;
		}
		float dist = ::GetDistanceSq(n->c[X_AXIS] - pt[X_AXIS], n->c[Y_AXIS] - pt[Y_AXIS], n->c[Z_AXIS] - pt[Z_AXIS]);
		if (dist < Square(range + radius[axis] + n->radius[axis])) 
		{
			prev->next = n;
			prev = n; 
		}
		ptr += dir;
	}
	return prev;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE BoxMap::Query(
	PROXNODE nodeid,
	float range)
{
	BoxNode * node = nodes.GetByIndex(nodeid);
	ASSERT_RETINVALID(node);

	if (range < 0.0f)
	{
		return NULL;
	}

	Sort();

	BoxNode * prev = node;

	prev = QueryAxis<X_AXIS, SM_XAXIS_REDGE, -1>(node, prev, list[SM_XAXIS_REDGE] + count, list[SM_XAXIS_REDGE] - 1,     range);
	prev = QueryAxis<X_AXIS, SM_XAXIS_LEDGE, +1>(node, prev, list[SM_XAXIS_LEDGE] - 1,     list[SM_XAXIS_LEDGE] + count, range);

	prev->next = NULL; 
	if (!node->next)
	{
		return INVALID_ID;
	}
	return nodes.GetIndexByPtr((BoxNode *)node->next);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int axis, unsigned int smlist, int dir> 
BoxNode * BoxMap::GetClosestNodeOnAxis(
	float f)
{
	BoxNode ** min = list[smlist];
	BoxNode ** max = min + count;
	BoxNode ** cur = min + count / 2;
	while (max > min)
	{
		float cf = (*cur)->c[axis] + (*cur)->radius[axis] * dir;

		if (cf > f)
		{
			max = cur;
		}
		else if (cf < f)
		{
			min = cur + 1;
		}
		else
		{
			return *cur;
		}
		cur = min + (max - min) / 2;
	}

	if (max >= list[axis] + count)
	{
		--max;
	}
	return *max;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PROXNODE BoxMap::Query(
	const VECTOR & p,
	float range)
{
	if (range < 0.0f)
	{
		return NULL;
	}
	float rangesq = range * range;

	Sort();

	BoxNode * far_xl = GetClosestNodeOnAxis<X_AXIS, SM_XAXIS_LEDGE, -1>(p.x - range);	// find leftmost box whose left edge is closest to x - range
	BoxNode * far_xr = GetClosestNodeOnAxis<X_AXIS, SM_XAXIS_REDGE, +1>(p.x + range); // find rightmost box whose right edge is closest to x + range

	ASSERT_RETINVALID(far_xl);
	ASSERT_RETINVALID(far_xr);

	BoxNode ** ptr = far_xl->pos[X_AXIS];
	BoxNode ** max = far_xr->pos[X_AXIS];
	
	BoxNode * list = NULL;
	BoxNode * curr = NULL;

	while (ptr <= max)
	{
		float distsq = GetBoxDistanceSq(*ptr, p, range);
		if (distsq <= rangesq) 
		{
			if (curr)
			{
				curr->next = *ptr;
				curr = (BoxNode *)(curr->next);
			}
			else
			{
				list = curr = *ptr;
			}
		}
		++ptr;
	}

	if (curr)
	{
		curr->next = NULL;
	}
	return (list ? list->_idx : INVALID_ID);
}


#if ISVERSION(DEBUG_VERSION)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void PointMapUnitTest(
	void)
{
	static const unsigned int COUNT = 100;
	static const unsigned int ITER = 1000;

	struct TESTSTRUCT
	{
		PROXNODE	proxid;
		DWORD		nodeid;
		float		x, y, z;
	};
	TESTSTRUCT testarray[COUNT * 2];
	unsigned int count = 0;

	RAND rand;
	RandInit(rand, GetTickCount());

	trace("testing PointMap:\n");

	PointMap map;
	map.Init(NULL);

	// add the count to map
	for (unsigned int ii = 0; ii < COUNT; ++ii)
	{
		testarray[ii].nodeid = ii;
		testarray[ii].x = (float)RandGetNum(rand, 0, 99);
		testarray[ii].y = (float)RandGetNum(rand, 0, 99);
		testarray[ii].z = (float)RandGetNum(rand, 0, 99);
		testarray[ii].proxid = map.Insert(testarray[ii].nodeid, 0, testarray[ii].x, testarray[ii].y, testarray[ii].z);
		trace("add to proxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d\n", testarray[ii].nodeid, testarray[ii].proxid,
			(int)testarray[ii].x, (int)testarray[ii].y, (int)testarray[ii].z);
		++count;
	}

	DWORD nodeidViewPoint = COUNT/2;
	PROXNODE proxidViewPoint = testarray[nodeidViewPoint].proxid;

	for (unsigned int ii = COUNT; ii < COUNT * 2; ++ii)
	{
		testarray[ii].nodeid = INVALID_ID;
		testarray[ii].proxid = INVALID_ID;
	}

	// do queries from ITER random locations
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		float radius = (float)RandGetNum(rand, 1, 25);
		float radiussq = radius * radius;
		float x = (float)RandGetNum(rand, 0, 99);
		float y = (float)RandGetNum(rand, 0, 99);
		float z = (float)RandGetNum(rand, 0, 99);
		trace("[%4d]  query:  x: %2d  y: %2d  z: %2d  r: %2d  ", ii, (int)x, (int)y, (int)z, (int)radius);
		PROXNODE idClosest = map.GetClosest(x, y, z, radius);
		float bestsq = radiussq;
		if (idClosest != INVALID_ID)
		{
			float cx, cy, cz;
			ASSERT(map.GetPosition(idClosest, cx, cy, cz));
			bestsq = GetDistanceSq(x - cx, y - cy, z - cz);
		}
		PROXNODE idQuery = map.Query(x, y, z, radius);
		PROXNODE idCurr = idQuery;
		int found = 0;

		while (idCurr != INVALID_ID)
		{
			float qx, qy, qz;
			ASSERT(map.GetPosition(idCurr, qx, qy, qz));
			float distsq = GetDistanceSq(x - qx, y - qy, z - qz);
			ASSERT(distsq <= radiussq);
			++found;

			// check that it isn't closer than the closest
			ASSERT(idClosest != INVALID_ID || distsq == radiussq);
			ASSERT(distsq >= bestsq);
			
			idCurr = map.GetNext(idCurr);
		}
		trace("found: %2d  (", found);
		idCurr = idQuery;
		while (idCurr != INVALID_ID)
		{
			trace("%d", idCurr);
			if (idCurr == idClosest)
			{
				trace("*");
			}
			idCurr = map.GetNext(idCurr);
			if (idCurr != INVALID_ID)
			{
				trace(", ");
			}
		}
		trace(")\n");

		// make sure there are exactly that many nodes within the radius
		for (unsigned int jj = 0; jj < COUNT; ++jj)
		{
			if (GetDistanceSq(x - testarray[jj].x, y - testarray[jj].y, z - testarray[jj].z) <= radiussq)
			{
				--found;
			}
		}
		ASSERT(found == 0);
	}

	// add and delete nodes
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		BOOL add = RandGetNum(rand, (count > 0 ? 0 : 1), (count < COUNT ? 1 : 0));
		if (add)
		{
			ASSERT(count < ITER);
			unsigned int idx = RandGetNum(rand, 0, COUNT * 2 - count - 1);
			for (unsigned jj = 0; jj < COUNT * 2; ++jj)
			{
				if (testarray[jj].nodeid != INVALID_ID)
				{
					continue;
				}
				if (idx > 0)
				{
					--idx;
					continue;
				}
				ASSERT(testarray[jj].proxid == INVALID_ID);
				testarray[jj].nodeid = jj;
				testarray[jj].x = (float)RandGetNum(rand, 0, 99);
				testarray[jj].y = (float)RandGetNum(rand, 0, 99);
				testarray[jj].z = (float)RandGetNum(rand, 0, 99);
				testarray[jj].proxid = map.Insert(testarray[jj].nodeid, 0, testarray[jj].x, testarray[jj].y, testarray[jj].z);
				trace("add to proxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d\n", testarray[jj].nodeid, testarray[jj].proxid,
					(int)testarray[jj].x, (int)testarray[jj].y, (int)testarray[jj].z);
				++count;

				// for each node, do a query and make sure it's in there once and only once
				for (unsigned int kk = 0; kk < COUNT * 2; ++kk)
				{
					if (kk == nodeidViewPoint)
					{
						continue;
					}
					unsigned int goal = 1;
					if (testarray[kk].nodeid == INVALID_ID)
					{
						goal = 0;
						ASSERT(testarray[kk].proxid == INVALID_ID);
					}
					else
					{
						ASSERT(testarray[kk].proxid != INVALID_ID);
					}
					unsigned int found = 0;
					ASSERT(testarray[nodeidViewPoint].proxid != INVALID_ID);
					PROXNODE idQuery = map.Query(proxidViewPoint, 1000.0f);
					while (idQuery != INVALID_ID)
					{
						if (kk == map.GetId(idQuery))
						{
							found++;
						}
						idQuery = map.GetNext(idQuery);
					}
					ASSERT(found == goal);
				}
				break;
			}
		}
		else
		{
			// pick a random one and delete it
			unsigned int idx = RandGetNum(rand, 0, count - 1);
			for (unsigned jj = 0; jj < COUNT * 2; ++jj)
			{
				if (testarray[jj].nodeid == INVALID_ID)
				{
					continue;
				}
				if (idx > 0)
				{
					--idx;
					continue;
				}
				if (jj == nodeidViewPoint)
				{
					break;
				}
				ASSERT(testarray[jj].proxid != INVALID_ID);
				trace("delete proxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d\n", testarray[jj].nodeid, testarray[jj].proxid,
					(int)testarray[jj].x, (int)testarray[jj].y, (int)testarray[jj].z);
				map.Delete(testarray[jj].proxid);
				testarray[jj].nodeid = INVALID_ID;
				testarray[jj].proxid = INVALID_ID;
				--count;

				// for each node, do a query and make sure it's in there once and only once
				for (unsigned int kk = 0; kk < COUNT * 2; ++kk)
				{
					if (kk == nodeidViewPoint)
					{
						continue;
					}
					unsigned int goal = 1;
					if (testarray[kk].nodeid == INVALID_ID)
					{
						ASSERT(testarray[kk].proxid == INVALID_ID);
						goal = 0;
					}
					else
					{
						ASSERT(testarray[kk].proxid != INVALID_ID);
					}
					unsigned int found = 0;
					ASSERT(testarray[nodeidViewPoint].proxid != INVALID_ID);
					PROXNODE idQuery = map.Query(proxidViewPoint, 1000.0f);
					while (idQuery != INVALID_ID)
					{
						if (kk == map.GetId(idQuery))
						{
							found++;
						}
						idQuery = map.GetNext(idQuery);
					}
					ASSERT(found == goal);
				}
				break;
			}
		}
	}

	map.Free();
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMapUnitTest(
	void)
{
	static const unsigned int COUNT = 100;
	static const unsigned int ITER = 1000;

	struct TESTSTRUCT
	{
		PROXNODE	proxid;
		DWORD		nodeid;
		float		x, y, z;
		float		xr, yr, zr;
	};
	TESTSTRUCT testarray[COUNT * 2];
	unsigned int count = 0;

	RAND rand;
	RandInit(rand, GetTickCount());

	trace("testing BoxMap:\n");

	BoxMap map;
	map.Init(NULL);

	// add the count to map
	for (unsigned int ii = 0; ii < COUNT; ++ii)
	{
		testarray[ii].nodeid = ii;
		testarray[ii].x = (float)RandGetNum(rand, 0, 99);
		testarray[ii].y = (float)RandGetNum(rand, 0, 99);
		testarray[ii].z = (float)RandGetNum(rand, 0, 99);
		testarray[ii].xr = (float)RandGetNum(rand, 1, 6);
		testarray[ii].yr = (float)RandGetNum(rand, 1, 6);
		testarray[ii].zr = (float)RandGetNum(rand, 1, 6);
		testarray[ii].proxid = map.Insert(testarray[ii].nodeid, 0, testarray[ii].x, testarray[ii].y, testarray[ii].z,
			testarray[ii].xr, testarray[ii].yr, testarray[ii].zr);
		trace("add to boxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d  xr:%3d  yr:%3d  zr:%3d\n", testarray[ii].nodeid, testarray[ii].proxid,
			(int)testarray[ii].x, (int)testarray[ii].y, (int)testarray[ii].z, 
			(int)testarray[ii].xr, (int)testarray[ii].yr, (int)testarray[ii].zr);
		++count;
	}

	DWORD nodeidViewPoint = COUNT/2;
	PROXNODE proxidViewPoint = testarray[nodeidViewPoint].proxid;

	for (unsigned int ii = COUNT; ii < COUNT * 2; ++ii)
	{
		testarray[ii].nodeid = INVALID_ID;
		testarray[ii].proxid = INVALID_ID;
	}

	// add and delete nodes
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		BOOL add = RandGetNum(rand, (count > 0 ? 0 : 1), (count < COUNT ? 1 : 0));
		if (add)
		{
			ASSERT(count < ITER);
			unsigned int idx = RandGetNum(rand, 0, COUNT * 2 - count - 1);
			for (unsigned jj = 0; jj < COUNT * 2; ++jj)
			{
				if (testarray[jj].nodeid != INVALID_ID)
				{
					continue;
				}
				if (idx > 0)
				{
					--idx;
					continue;
				}
				ASSERT(testarray[jj].proxid == INVALID_ID);
				testarray[jj].nodeid = jj;
				testarray[jj].x = (float)RandGetNum(rand, 0, 99);
				testarray[jj].y = (float)RandGetNum(rand, 0, 99);
				testarray[jj].z = (float)RandGetNum(rand, 0, 99);
				testarray[jj].xr = (float)RandGetNum(rand, 1, 6);
				testarray[jj].yr = (float)RandGetNum(rand, 1, 6);
				testarray[jj].zr = (float)RandGetNum(rand, 1, 6);
				testarray[jj].proxid = map.Insert(testarray[jj].nodeid, 0, testarray[jj].x, testarray[jj].y, testarray[jj].z,
					testarray[jj].xr, testarray[jj].yr, testarray[jj].zr);
				trace("add to boxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d  xr:%3d  yr:%3d  zr:%3d\n", 
					testarray[jj].nodeid, testarray[jj].proxid,
					(int)testarray[jj].x, (int)testarray[jj].y, (int)testarray[jj].z,
					(int)testarray[jj].xr, (int)testarray[jj].yr, (int)testarray[jj].zr);
				++count;

				// for each node, do a query and make sure it's in there once and only once
				for (unsigned int kk = 0; kk < COUNT * 2; ++kk)
				{
					if (kk == nodeidViewPoint)
					{
						continue;
					}
					unsigned int goal = 1;
					if (testarray[kk].nodeid == INVALID_ID)
					{
						goal = 0;
						ASSERT(testarray[kk].proxid == INVALID_ID);
					}
					else
					{
						ASSERT(testarray[kk].proxid != INVALID_ID);
					}
					unsigned int found = 0;
					ASSERT(testarray[nodeidViewPoint].proxid != INVALID_ID);
					PROXNODE idQuery = map.Query(proxidViewPoint, 1000.0f);
					while (idQuery != INVALID_ID)
					{
						if (kk == map.GetId(idQuery))
						{
							found++;
						}
						idQuery = map.GetNext(idQuery);
					}
					ASSERT(found == goal);
				}
				break;
			}
		}
		else
		{
			// pick a random one and delete it
			unsigned int idx = RandGetNum(rand, 0, count - 1);
			for (unsigned jj = 0; jj < COUNT * 2; ++jj)
			{
				if (testarray[jj].nodeid == INVALID_ID)
				{
					continue;
				}
				if (idx > 0)
				{
					--idx;
					continue;
				}
				if (jj == nodeidViewPoint)
				{
					break;
				}
				ASSERT(testarray[jj].proxid != INVALID_ID);
				trace("delete boxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d  xr:%3d  yr:%3d  zr:%3d\n", 
					testarray[jj].nodeid, testarray[jj].proxid,
					(int)testarray[jj].x, (int)testarray[jj].y, (int)testarray[jj].z,
					(int)testarray[jj].xr, (int)testarray[jj].yr, (int)testarray[jj].zr);
				map.Delete(testarray[jj].proxid);
				testarray[jj].nodeid = INVALID_ID;
				testarray[jj].proxid = INVALID_ID;
				--count;

				// for each node, do a query and make sure it's in there once and only once
				for (unsigned int kk = 0; kk < COUNT * 2; ++kk)
				{
					if (kk == nodeidViewPoint)
					{
						continue;
					}
					unsigned int goal = 1;
					if (testarray[kk].nodeid == INVALID_ID)
					{
						ASSERT(testarray[kk].proxid == INVALID_ID);
						goal = 0;
					}
					else
					{
						ASSERT(testarray[kk].proxid != INVALID_ID);
					}
					unsigned int found = 0;
					ASSERT(testarray[nodeidViewPoint].proxid != INVALID_ID);
					PROXNODE idQuery = map.Query(proxidViewPoint, 1000.0f);
					while (idQuery != INVALID_ID)
					{
						if (kk == map.GetId(idQuery))
						{
							found++;
						}
						idQuery = map.GetNext(idQuery);
					}
					ASSERT(found == goal);
				}
				break;
			}
		}
	}

	map.Free();
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void BoxMapValidityTest(
	void)
{
	// add 27 boxes, each radius RADIUS, centered at 0,0, linear distance DIST apart
	static const float DIST = 3.0f;
	static const float RADIUS = 1.0f;

	struct TESTSTRUCT
	{
		PROXNODE	proxid;
		DWORD		nodeid;
		float		x, y, z;
		float		xr, yr, zr;
	};
	TESTSTRUCT testarray[27];

	trace("testing BoxMap:\n");

	BoxMap map;
	map.Init(NULL);

	for (int kk = -1; kk <= 1; ++kk)
	{
		for (int jj = -1; jj <= 1; ++jj)
		{
			for (int ii = -1; ii <= 1; ++ii)
			{
				unsigned int idx = (kk + 1) * 9 + (jj + 1) * 3 + ii + 1;

				testarray[idx].nodeid = idx;
				testarray[idx].x = ii * DIST;
				testarray[idx].y = jj * DIST;
				testarray[idx].z = kk * DIST + (idx == 13 ? 0.0f : -0.5f);
				testarray[idx].xr = RADIUS;
				testarray[idx].yr = RADIUS;
				testarray[idx].zr = RADIUS;
				testarray[idx].proxid = map.Insert(testarray[idx].nodeid, 0, testarray[idx].x, testarray[idx].y, testarray[idx].z,
					testarray[idx].xr, testarray[idx].yr, testarray[idx].zr);
				trace("add to boxmap:  id:%4d  proxid:%4d  x:%3d  y:%3d  z:%3d  xr:%3d  yr:%3d  zr:%3d\n", 
					testarray[idx].nodeid, testarray[idx].proxid,
					(int)testarray[idx].x, (int)testarray[idx].y, (int)testarray[idx].z, 
					(int)testarray[idx].xr, (int)testarray[idx].yr, (int)testarray[idx].zr);
			}
		}
	}

	// now run through all boxes and output distance to center one
	trace("\n");
	for (int kk = -1; kk <= 1; ++kk)
	{
		for (int jj = -1; jj <= 1; ++jj)
		{
			for (int ii = -1; ii <= 1; ++ii)
			{
				unsigned int idx = (kk + 1) * 9 + (jj + 1) * 3 + ii + 1;
				BOOL intersect;
				trace("node: %2d  (%2d, %2d, %2d)",
					idx, (int)testarray[idx].x, (int)testarray[idx].y, (int)testarray[idx].z);
				float distsq = map.GetDistanceSq(testarray[idx].proxid, testarray[13].proxid, intersect);
				trace("  distsq(%c): %5.2f\n",
					(intersect ? '*' : ' '), distsq);
			}
		}
	}

	// now go through and get successive queries of boxes within a radius
	trace("\n");
	VECTOR p(0.0f, 0.0f, 0.0f);
	for (int rr = 1; rr < 6; rr++)
	{
		trace("query [%d]: ", rr);
		PROXNODE idQuery = map.Query(p, (float)rr);
		while (idQuery != INVALID_ID)
		{
			trace("%d, ", idQuery);
			idQuery = map.GetNext(idQuery);
		}
		trace("\n");
	}

	map.Free();
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ProxMapUnitTest(
	void)
{
	trace("running ProxMapUnitTest()\n");

//	PointMapUnitTest();
//	BoxMapUnitTest();
	BoxMapValidityTest();
}


#endif // ISVERSION(DEBUG_VERISON)