//----------------------------------------------------------------------------
// proxmap.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PROXMAP_H_
#define _PROXMAP_H_


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
typedef DWORD									PROXNODE;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
enum
{
	PROX_FLAG_STATIC =							0x01,
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define QUERY_MAX								(1.0e15f)
#define PROX_BUCKET_SIZE						64


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
enum
{
	X_AXIS =									0,
	Y_AXIS =									1,
	Z_AXIS =									2,
};


// ---------------------------------------------------------------------------
// template vector/garbage class
// C needs data members  'C * _next' and 'unsigned int _idx'
// if each node needs to be individual freed, need to modify Free()
// ---------------------------------------------------------------------------
template <class C, unsigned int bufsize>
class VectorGarbage
{
protected:
	// data
	MEMORYPOOL *								mempool;

	C **										buckets;
	unsigned int								count;				// # of allocated objects

	C *											garbage;

	// -----------------------------------------------------------------------
	unsigned int BucketCount(
		void) const
	{
		return (count + bufsize - 1)/bufsize;
	}

	// -----------------------------------------------------------------------
	C * GetFromGarbage(
		void)
	{
		if (!garbage)
		{
			return NULL;
		}

		C * obj = garbage;
		garbage = (C *)(obj->next);
		return obj;
	}

public:
	// -----------------------------------------------------------------------
	void Init(
		MEMORYPOOL * pool)
	{
		mempool = pool;
		buckets = NULL;
		count = 0;
		garbage = NULL;
	}

	// -----------------------------------------------------------------------
	void Free(
		void)
	{
		unsigned int bucketcount = BucketCount();
		ASSERT_RETURN(bucketcount == 0 || buckets);
		for (unsigned int ii = 0; ii < bucketcount; ++ii)
		{
			FREE(mempool, buckets[ii]);
		}
		if (buckets)
		{
			FREE(mempool, buckets);
		}
		buckets = NULL;
		count = 0;
		garbage = NULL;
		mempool = NULL;
	}

	// -----------------------------------------------------------------------
	C * GetByIndex(
		unsigned int index)
	{
#if !ISVERSION(OPTIMIZED_VERSION)
		DBG_ASSERT_RETNULL(index >= 0 && index < count);
		DBG_ASSERT_RETNULL(buckets)
#endif
		C * bucket = buckets[index / bufsize];
		C * node = bucket + index % bufsize;
#if !ISVERSION(OPTIMIZED_VERSION)
		DBG_ASSERT(node && node->_idx == index);
#endif
		return node;
	}

	// -----------------------------------------------------------------------
	unsigned int GetIndexByPtr(
		C * obj)
	{
#if !ISVERSION(OPTIMIZED_VERSION)
		DBG_ASSERT_RETINVALID(obj);
		DBG_ASSERT_RETINVALID(obj->_idx < count);
#endif
		return obj->_idx;
	}

	// -----------------------------------------------------------------------
	// returns new allocated size (or 0 if not resized)
	C *	GetNew(
		unsigned int & resized)
	{
		resized = 0;

		C * obj = GetFromGarbage();
		if (obj)
		{
			return obj;
		}

		// need to allocate a new bucket?
		if (count % bufsize == 0)
		{
			unsigned int newbucketcount = BucketCount() + 1;
			buckets = (C **)REALLOC(mempool, buckets, sizeof(C*) * newbucketcount);
			buckets[newbucketcount - 1] = (C *)MALLOC(mempool, sizeof(C) * bufsize);
			resized = count + bufsize;
		}

		C * bucket = buckets[count / bufsize];
		obj = bucket + count % bufsize;
		obj->_idx = count;
		count++;
		return obj;
	}

	// -----------------------------------------------------------------------
	C * GetNew( 
		void)
	{
		unsigned int resized;
		return GetNew(resized);
	}

	// -----------------------------------------------------------------------
	void Recycle(
		C * obj)
	{
		obj->next = garbage;
		garbage = obj;
	}
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct ProxNode
{
	DWORD										id;					// user provided id
	PROXNODE									_idx;				// index in vector
	union
	{
		struct														// coordinates
		{
			float								x;
			float								y;
			float								z;
		};
		float									c[3];
	};
	ProxNode *									next;				// next (used for garbage list or queries)
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <class C, unsigned int bufsize, unsigned int numaxis>
class ProxMap
{
protected:
	MEMORYPOOL *								mempool;

	VectorGarbage<C, PROX_BUCKET_SIZE>			nodes;

	unsigned int								count;				// # of real nodes

	C **										list[numaxis];		// sorted lists
	BOOL										sorted[numaxis];	// TRUE if list is sorted

	// functions
	// -----------------------------------------------------------------------
	C *  GetNewNode(
		void)
	{
		unsigned int resized;
		C * node = nodes.GetNew(resized);
		if (resized)
		{
			for (unsigned int ii = 0; ii < numaxis; ++ii)
			{
				C ** old = list[ii];
				list[ii] = (C **)REALLOC(mempool, list[ii], sizeof(C *) * resized);

				// iterate all nodes and rebase pointers
				if (list[ii] != old)
				{
					C ** listptr = list[ii];
					for (unsigned int jj = 0; jj < count; ++jj)
					{
						C * node2 = listptr[jj];
						unsigned int offs = (unsigned int)(node2->pos[ii] - old);
						ASSERT(offs < count);
						node2->pos[ii] = listptr + offs;

						unsigned int newoffs = (unsigned int)(node2->pos[ii] - list[ii]);
						ASSERT(newoffs == offs);
					}
				}
			}
		}

		return node;
	}

	// -----------------------------------------------------------------------
	void AddNewNodeToMapUnsorted(
		C * node)
	{
		// insert into sorted lists
		for (unsigned int ii = 0; ii < numaxis; ++ii)
		{
			sorted[ii] = FALSE;
			list[ii][count] = node;
			node->pos[ii] = list[ii] + count;
		}
		count++;
	}

	// -----------------------------------------------------------------------
	void RemoveFromList(
		unsigned int axis,
		C * node)
	{
		ASSERT_RETURN(node->pos[axis]);

		unsigned int offs = (unsigned int)(node->pos[axis] - list[axis]);
		ASSERT(offs < count);
		if (offs < count - 1)
		{
			ASSERT_RETURN(MemoryMove(node->pos[axis], (count - offs) * sizeof(C *), node->pos[axis] + 1, sizeof(C *) * (count - offs - 1)));

			C ** cur = node->pos[axis];
			C ** end = list[axis] + count - 1;
			while (cur < end)
			{
				(*cur)->pos[axis]--;
				cur++;
			}
		}
		node->pos[axis] = NULL;
	}

public:
	// functions
	// -----------------------------------------------------------------------
	void Init(
		MEMORYPOOL * pool)
	{
		mempool = pool;
		nodes.Init(mempool);
		count = 0;
		for (unsigned int ii = 0; ii < numaxis; ++ii)
		{
			sorted[ii] = FALSE;
			list[ii] = NULL;
		}
	}

	// -----------------------------------------------------------------------
	void Free(
		void)
	{
		nodes.Free();
		for (unsigned int ii = 0; ii < numaxis; ++ii)
		{
			sorted[ii] = FALSE;
			FREE(mempool, list[ii]);
			list[ii] = NULL;
		}
		count = 0;
		mempool = NULL;		
	}

	// -----------------------------------------------------------------------
	void Delete(
		PROXNODE nodeid)
	{
		C * node = nodes.GetByIndex(nodeid);
		ASSERT_RETURN(node);

		// remove from lists
		for (unsigned int ii = 0; ii < numaxis; ++ii)
		{
			RemoveFromList(ii, node);
		}
		count--;

		// put on garbage
		nodes.Recycle(node);
	}

	// -----------------------------------------------------------------------
	DWORD GetId(
		PROXNODE nodeid)
	{
		C * node = nodes.GetByIndex(nodeid);
		ASSERT_RETINVALID(node);

		return node->id;
	}

	// -----------------------------------------------------------------------
	PROXNODE GetNext(
		PROXNODE nodeid)
	{
		C * node = nodes.GetByIndex(nodeid);
		ASSERT_RETINVALID(node);

		if (!node->next)
		{
			return INVALID_ID;
		}
		return nodes.GetIndexByPtr((C *)node->next);
	}
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct PointNode : ProxNode
{
	PointNode **								pos[2];			// pointer to position in sorted lists
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class PointMap : public ProxMap <PointNode, PROX_BUCKET_SIZE, 2>
{
private:
	// functions
	template <unsigned int axis> BOOL CheckMoveAxisChange(PointNode * node, float newc);
	template <unsigned int axis> void SortAxis(void);
	void Sort(void);
	PointNode * GetClosestNode(PointNode * node, float max);
	template <unsigned int axis> PointNode * GetClosestNodeOnAxis(float f);
	template <unsigned int axis, int dir> PointNode * QueryAxis(PointNode * node, 
		PointNode * prev, PointNode ** end, float range);

public:
	PROXNODE	Insert(DWORD id, DWORD flags, float x, float y, float z);
	PROXNODE	InsertSorted(DWORD id, DWORD flags, float x, float y, float z);
	void		Move(PROXNODE nodeid, float x, float y, float z);
	void 		MoveSorted(PROXNODE nodeid, float x, float y, float z);
	DWORD		GetClosest(PROXNODE nodeid, float max = QUERY_MAX);
	DWORD		GetClosest(PROXNODE nodeid, float & x, float & y, float & z, float max = QUERY_MAX);
	DWORD		GetClosest(float x, float y, float z, float max = QUERY_MAX);
	PROXNODE	Query(PROXNODE nodeid, float range);
	PROXNODE	Query(float x, float y, float z, float range);
	BOOL		GetPosition(PROXNODE nodeid, float & x, float & y, float & z);
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
enum
{
	SM_XAXIS_LEDGE,
	SM_XAXIS_REDGE,
	SM_LIST_COUNT,
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct SphereNode : ProxNode
{
	float										radius;			// radius

	SphereNode **								pos[SM_LIST_COUNT];		// position in sorted list
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class SphereMap : public ProxMap <SphereNode, PROX_BUCKET_SIZE, SM_LIST_COUNT>
{
private:
	// functions
	template <unsigned int axis, unsigned int smlist, int edge> void CheckMoveAxisChanged(SphereNode * node, float newc);
	template <unsigned int axis, unsigned int smlist, int edge> void SortAxis(void);
	void Sort(void);
	float GetBoxDistanceSq(SphereNode * a, SphereNode * b, BOOL & intersect);
	template <unsigned int axis, unsigned int smlist, int dir> SphereNode * GetClosestBoxOnAxis(
		SphereNode * node, SphereNode * best, SphereNode ** end, BOOL & intersect, float & bestsq);
	SphereNode * GetClosestBox(SphereNode * node, float max);
	template <unsigned int axis, unsigned int smlist, int dir> SphereNode * GetClosestSphereOnAxis(
		SphereNode * node, SphereNode * best, SphereNode ** end, BOOL & intersect, float & bestdist);
	SphereNode * GetClosestSphere(SphereNode * node, float max, float * dist = NULL);
	template <unsigned int axis, int dir> SphereNode * QueryAxis(SphereNode * node, 
		SphereNode * prev, SphereNode ** end, float range);
	template <unsigned int axis, unsigned int smlist, int dir> SphereNode * GetClosestNodeOnAxis(float f);

public:
	PROXNODE	Insert(DWORD id, DWORD flags, float x, float y, float z, float radius);
	PROXNODE	InsertSorted(DWORD id, DWORD flags, float x, float y, float z, float radius);
	void		Move(PROXNODE nodeid, float x, float y, float z);
	void		MoveSorted(PROXNODE nodeid, float x, float y, float z);
	void		Resize(PROXNODE nodeid, float radius);
	void		MoveResize(PROXNODE nodeid, float x, float y, float z, float radius);
	DWORD		GetClosestBox(PROXNODE nodeid, float max = QUERY_MAX);
	DWORD		GetClosestBox(PROXNODE nodeid, float & x, float & y, float & z, float max = QUERY_MAX);
	DWORD		GetClosestSphere(PROXNODE nodeid, float max = QUERY_MAX, float * dist = NULL);
	PROXNODE	Query(PROXNODE nodeid, float range);
	PROXNODE	Query(const VECTOR & p, float range);
	PROXNODE	Query(float x, float y, float z, float range);
	BOOL		GetPosition(PROXNODE nodeid, float & x, float & y, float & z);
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct BoxNode : ProxNode
{
	float										radius[3];				// radius on each axis

	BoxNode **									pos[SM_LIST_COUNT];		// position in sorted list
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class BoxMap : public ProxMap <BoxNode, PROX_BUCKET_SIZE, SM_LIST_COUNT>
{
private:
	// functions
	template <unsigned int axis, unsigned int smlist, int edge> void CheckMoveAxisChanged(BoxNode * node, float newc);
	template <unsigned int axis, unsigned int smlist, int edge> void SortAxis(void);
	void Sort(void);
	float GetBoxDistanceSq(BoxNode * a, BoxNode * b, BOOL & intersect);
	float GetBoxDistanceSq(BoxNode * node, const VECTOR & p, float r);
	template <unsigned int axis, unsigned int smlist, int dir> BoxNode * GetClosestBoxOnAxis(
		BoxNode * node, BoxNode * best, BoxNode ** end, BOOL & intersect, float & bestsq);
	BoxNode * GetClosestBox(BoxNode * node, float max);
	template <unsigned int axis, unsigned int smlist, int dir> BoxNode * QueryAxis(BoxNode * node, 
		BoxNode * prev, BoxNode ** begin, BoxNode ** end, float range);
	template <unsigned int axis, unsigned int smlist, int dir> BoxNode * GetClosestNodeOnAxis(float f);

public:
	PROXNODE	Insert(DWORD id, DWORD flags, float x, float y, float z, float xradius, float yradius, float zradius);
	PROXNODE	InsertSorted(DWORD id, DWORD flags, float x, float y, float z, float xradius, float yradius, float zradius);
	void		Move(PROXNODE nodeid, float x, float y, float z);
	void		MoveSorted(PROXNODE nodeid, float x, float y, float z);
	void		Resize(PROXNODE nodeid, float xradius, float yradius, float zradius);
	void		MoveResize(PROXNODE nodeid, float x, float y, float z, float xradius, float yradius, float zradius);
	DWORD		GetClosestBox(PROXNODE nodeid, float max = QUERY_MAX);
	DWORD		GetClosestBox(PROXNODE nodeid, float & x, float & y, float & z, float max = QUERY_MAX);
	PROXNODE	Query(PROXNODE nodeid, float range);
	PROXNODE	Query(const VECTOR & p, float range);
	BOOL		GetPosition(PROXNODE nodeid, float & x, float & y, float & z);
	float		GetDistanceSq(PROXNODE nodeid1, PROXNODE nodeid2, BOOL & intersect);
};


#if ISVERSION(DEBUG_VERSION)
void ProxMapUnitTest(
	void);
#endif


#endif // _PROXMAP_H_
