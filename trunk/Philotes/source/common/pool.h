//----------------------------------------------------------------------------
// pool.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memorymanager_i.h"

#pragma warning(push)
#pragma warning(disable:4127)

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Class CPool
//
// Remarks
//	The pool takes a type T of objects which will be allocated in chunks
//	called buckets.
//
//	Objects of type T must have a "Next" member
//
//	Allocations come from the default memory allocator through the MALLOC
//	and FREE macros unless the "allocator" parameter is specified in the 
//	constructor.
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
class CPool
{
	protected:
	
		static const unsigned int cDefaultAlignment = 16;

	public:
	
		CPool(IMemoryAllocator* allocator = NULL, unsigned int alignment = cDefaultAlignment);
		virtual ~CPool();
	
	public:
	
		unsigned int		Init(void* memory, unsigned int size);
		void				SetAllocator(IMemoryAllocator* allocator);
		bool				SetBucketSize(unsigned int bucketSize);
		T*					Get();
		void				Put(T* item);
		void				PutAll();
		bool				FreeAll();
		unsigned int		GetUsedCount();
		unsigned int		GetFreeCount();	
			
	protected:
	
		// Override these methods for custom allocation
		//
		virtual void*		InternalAlloc(MEMORY_SIZE size);
		virtual void		InternalFree(void* ptr);

	protected:
	
		// The system page size
		//
		unsigned int		m_PageSize;

		// Allocator that will be used for the pool memory
		//
		IMemoryAllocator*	m_Allocator;
		unsigned int		m_AlignedItemSize;
		
		unsigned int		m_UsedCount;
		unsigned int		m_FreeCount;
	
		// This protects the internals from concurrent access since
		// multiple threads could be accessing the lists
		//
		CCritSectLite		m_CS;
	
		// The number of nodes per bucket
		//
		unsigned int		m_BucketSize;
	
		// Nodes are allocated in chunks called buckets.  Nodes are not 
		// linked when they are in the bucket, but do get linked when
		// they are released into the free list.  This adds performance when
		// allocating a new bucket.  This pointer is to the bucket, which is 
		// also a node.  The first node in each bucket is used to link
		// allocated buckets
		//
		T*					m_BucketList;

		// This is another bucket list that is used when calling PutAll which resets
		// all the buckets.  If there are more than one, they are put on this list.
		// When a new bucket list is needed, this list is checked first
		//
		T*					m_FreeBucketList;
		
		// This is an alternate form of memory usage where the caller specifies
		// a block of memory that the pool will use
		//
		T*					m_Memory;
		
		bool				m_InternalAllocation;
		
		// This is the index into the bucket of the next free node.  This 
		// number is incremented when a node is taken from the bucket.
		// When it reaches the bucket size, it means a new bucket needs
		// to be allocated
		//
		unsigned int		m_BucketFreeIndex;
		
		// When nodes get released to the pool, they end up in this linked
		// list of LIFO nodes
		//
		T*					m_FreeList;		
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CPool Constructor
//
// Parameters
//	allocator : [optional]  If specified, determines the allocator used when the pool
//		needs to expand
//	alignment : [optional] If specified, determines the alignment of each object
//		within the pool.  Item size is increased to be a multiple of the 
//		alignment.
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
CPool<T, MT_SAFE>::CPool(IMemoryAllocator* allocator, unsigned int alignment) :
	m_BucketSize(2),
	m_BucketList(NULL),
	m_FreeBucketList(NULL),
	m_FreeList(NULL),
	m_Allocator(allocator),
	m_AlignedItemSize(0),
	m_Memory(NULL),
	m_UsedCount(0),
	m_FreeCount(0),
	m_InternalAllocation(false)
{
	if(MT_SAFE)
	{
		m_CS.Init();
	}
	
	m_PageSize = IMemoryManager::GetInstance().GetPageSize();

	// Increase the item size so that is is aligned
	//
	m_AlignedItemSize = sizeof(T);
	m_AlignedItemSize += (alignment - (m_AlignedItemSize % alignment)) % alignment;

	SetBucketSize(m_PageSize / m_AlignedItemSize);	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CPool Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
CPool<T, MT_SAFE>::~CPool()
{
	FreeAll();

	if(MT_SAFE)
	{
		m_CS.Free();
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	memory : A pointer to the memory that will be used for the pool
//	size : The number of bytes memory points to
//
// Returns
//	The count of items created in the memory
//
// Remarks
//	This is an optional call only necessary when passing in memory.  
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, bool MT_SAFE>
unsigned int CPool<T, MT_SAFE>::Init(void* memory, unsigned int size)
{
	DBG_ASSERT(memory);
	DBG_ASSERT(size >= m_AlignedItemSize);

	m_Memory = (T*)memory;
	
	unsigned int itemCount = size / m_AlignedItemSize;
	SetBucketSize(itemCount);
	
	m_BucketList = (T*)memory;
	m_BucketFreeIndex = 0;
	
	m_FreeCount += itemCount;
	
	return itemCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetAllocator
// 
// Parameters
//	allocator : The allocator that will be used when the pool must expand
//
// Remarks
//	This method must not be called after the pool has expanded
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, bool MT_SAFE>
void CPool<T, MT_SAFE>::SetAllocator(IMemoryAllocator* allocator)
{
	if(m_InternalAllocation == false)
	{
		m_Allocator = allocator;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetBucketSize
//
// Parameters
//	bucketSize : The number of items in each bucket
//
// Returns
//	true if the method succeeds, false otherwise
//
// Remarks
//  The method will fail if a Get() was ever called since the bucketSize
//	will be different
//	
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
bool CPool<T, MT_SAFE>::SetBucketSize(unsigned int bucketSize)
{
	// The bucket size cannot be changed once some memory has
	// been allocated
	//
	if(m_BucketList)
	{
		return false;
	}
	
	// Since the first node in the bucket is used to link buckets,
	// we need the bucket size to be at least 2
	//
	m_BucketSize = MAX<unsigned int>(bucketSize, 2);
	
	// Setting the free index out of range will force a bucket allocation
	// on the next Get() call
	//
	m_BucketFreeIndex = m_BucketSize;	
	
	return true;
}



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Get
//
// Returns
//	Returns a pointer to the next T in the pool.  It will allocate 
//	a new bucket of T's if necessary.  Can return NULL if out of memory
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
T* CPool<T, MT_SAFE>::Get()
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

	T* node = NULL;
	
	// Grab a node from the free list if it's available
	//
	if(m_FreeList)
	{
		node = m_FreeList;
		m_FreeList = node->Next;
		node->Next = NULL;
		
		++m_UsedCount;
		--m_FreeCount;
	}
	// Grab a node from the free bucket
	//
	else
	{	
		// If there are free nodes left in the bucket
		//
		if(m_BucketFreeIndex < m_BucketSize)
		{
			node = (T*)((BYTE*)m_BucketList + (m_BucketFreeIndex * m_AlignedItemSize));
			++m_BucketFreeIndex;
			
			++m_UsedCount;
			--m_FreeCount;			
		}
		// Otherwise allocate a new bucket
		//
		else
		{
			// Grab a free bucket if possible
			//
			if(m_FreeBucketList)
			{
				node = (T*)m_FreeBucketList;
				m_FreeBucketList = node->Next;
			}
			else
			{
				node = (T*)InternalAlloc(m_BucketSize * m_AlignedItemSize);
			}

			// Handle out of memory
			//
			if(node == NULL)
			{
				return NULL;
			}
			
			// The first node of each bucket is used to link each bucket
			// so when the pool needs to cleanup, it can traverse the buckets
			//
			node->Next = m_BucketList;
			m_BucketList = node;
			
			// Use the second node in the bucket
			//
			node = (T*)((BYTE*)m_BucketList + m_AlignedItemSize);
			
			// Make the third node in the bucket the next available node
			//
			m_BucketFreeIndex = 2;
			
			++m_UsedCount;
			m_FreeCount += (m_BucketSize - 2);
		}	
	}
	
	return node;	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Put
//
// Parameters
//	item : A pointer to the pool item to return to the pool
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
void CPool<T, MT_SAFE>::Put(T* item)
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	// Add the item to the front of the free list
	//
	item->Next = m_FreeList;
	m_FreeList = item;
	
	--m_UsedCount;
	++m_FreeCount;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PutAll
//
// Remarks
//	This method resets all items as if every item had been Put back into the pool.
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
void CPool<T, MT_SAFE>::PutAll()
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

	// Reset the used/free counts
	//
	m_FreeCount += m_UsedCount;
	m_UsedCount = 0;

	// Reset the free list
	//
	m_FreeList = NULL;

	// Move any buckets into the free bucket list
	//
	T* bucket = m_BucketList;
	while(bucket)
	{
		T* nextBucket = bucket->Next;
	
		if(bucket != m_Memory)
		{
			bucket->Next = m_FreeBucketList;
			m_FreeBucketList = bucket;
		}

		bucket = nextBucket;
	}
	
	if(m_Memory)
	{
		m_BucketFreeIndex = 0;
		m_BucketList = m_Memory;
	}
	else
	{
		m_BucketFreeIndex = m_BucketSize;
		m_BucketList = NULL;
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FreeAll
//
// Returns
//	true if successful, false otherwise
//
// Remarks
//	Frees all bucket memory
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
bool CPool<T, MT_SAFE>::FreeAll()
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	// Release all the buckets
	//
	while(m_BucketList && m_BucketList != m_Memory)
	{
		T* next = m_BucketList->Next;
	
		InternalFree(m_BucketList);
	
		m_BucketList = next;
	}
	
	m_BucketList = NULL;
	m_BucketFreeIndex = m_BucketSize;
	m_FreeList = NULL;
	
	m_UsedCount = 0;
	m_FreeCount = 0;
	
	return true;
}



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InternalAlloc
//
// Parameters
//	size : The number of bytes of memory to allocate
//
// Returns
//	A pointer to the allocated memory or NULL if an error
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
void* CPool<T, MT_SAFE>::InternalAlloc(MEMORY_SIZE size)
{
	if(m_Allocator)
	{
		m_InternalAllocation = true;
		return m_Allocator->Alloc(MEMORY_FUNC_FILELINE(size, NULL));
	}
	else
	{
		return MALLOC(NULL, size);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InternalFree
//
// Parameters
//	ptr : A pointer to the memory to free
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
void CPool<T, MT_SAFE>::InternalFree(void* ptr)
{
	if(m_Allocator)
	{
		m_Allocator->Free(MEMORY_FUNC_FILELINE(ptr, NULL));
	}
	else
	{
		FREE(NULL, ptr);
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetUsedCount
//
// Returns
//	The number of items in the pool that have been used
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
unsigned int CPool<T, MT_SAFE>::GetUsedCount()
{
	return m_UsedCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFreeCount
//
// Returns
//	The number of items in the pool that have been used
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, bool MT_SAFE>
unsigned int CPool<T, MT_SAFE>::GetFreeCount()
{
	return m_FreeCount;
}



} // end namespace FSCommon

#pragma warning(pop)
