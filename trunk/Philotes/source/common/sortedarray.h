//----------------------------------------------------------------------------
// sortedarray.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once 

#include "critsect.h"
#include "memorymanager_i.h"

#pragma warning(push)
#pragma warning(disable:4127)

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CSortedArray
//
// Parameters
//	K : The Key type for each item
//	T : The type of each item in the array
//  MT_SAFE : Determines if locks are taken when modifying the array
//
// Remarks
//	Stores copies of the key and value in nodes that are allocated through
//	the default allocator via MALLOC and FREE macros.
//
//  Although the insert and remove operations can be thread-safe, none of the
//	retrieval operations are thread safe since a pointer returned could be
//	moved or deleted from another thread.  Store pointers as the type T for 
//	multi-threaded use.
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>
class CSortedArray
{

	public:
	
		// ARRAY_NODE
		//
		struct ARRAY_NODE
		{
			K		Key;
			T		Value;
		};

	public:
	
		CSortedArray(IMemoryAllocator* allocator = NULL, bool sortIncreasing = true);
		virtual ~CSortedArray();

	public:
	
		void						Init(void* memory, unsigned int maxCount, unsigned int growCount);
		bool						Init(unsigned int initialCount, unsigned int growCount);
		void						SetAllocator(IMemoryAllocator* allocator);
		bool						Insert(const K& key, const T& value, unsigned int* index = NULL);
		bool						Remove(const K& key);
		bool						RemoveAt(const unsigned int index);
		void						RemoveAll();
		T*							Find(const K& key, unsigned int* index = NULL);
		T*							Find(const T& value, unsigned int* index = NULL, K** key = NULL);
		T*							FindOrBefore(const K& key, unsigned int* index = NULL);
		bool						FindOrBefore(const K& key, T& item, unsigned int* index = NULL);
		T*							FindOrAfter(const K& key, unsigned int* index = NULL);
		bool						FindOrAfter(const K& key, T& item, unsigned int* index = NULL);		
		bool						FreeAll();
		inline unsigned int			GetCount();
		K*							GetKey(unsigned int index);
		unsigned int				Sort(unsigned int index, const K& newKey);
		inline T*					operator[](unsigned int index);
		bool						Verify();
		
	protected:
	
		virtual void*				InternalAlloc(MEMORY_SIZE size);
		virtual void				InternalFree(void* ptr);
		bool						RemoveAt(const unsigned int index, bool useLock);		
		ARRAY_NODE*					Find(const K& key, ARRAY_NODE** itrFinal = NULL);
		bool						GrowAtIndex(unsigned int growIndex);

		// Comparison operators
		//
		bool						IsLower(const K& left, const K& right);
		bool						IsHigher(const K& left, const K& right);

	protected:

		CReaderWriterLock_NS_FAIR	m_CS;
		
		IMemoryAllocator*			m_Allocator;
		bool						m_SortIncreasing;
	
		ARRAY_NODE*					m_Memory;	// sorted array of ARRAY_NODE
		unsigned int				m_NodeCountMax;	// allocated size of list (in number of elements)
		unsigned int				m_NodeCount;	// number of external elements in the list
		unsigned int				m_GrowCount;
		
		bool						m_InternalAllocation;
};


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CSortedArray Constructor
// 
// Parameters
//	allocator : [optional]  If specified, determines the allocator used
//  sortIncreasing : [optional] Specifies whether the items are sorted
//		in increasing or decreasing order
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>		
CSortedArray<K, T, MT_SAFE>::CSortedArray(IMemoryAllocator* allocator, bool sortIncreasing) :
	m_Allocator(allocator),
	m_GrowCount(0),
	m_Memory(NULL),
	m_NodeCount(0),
	m_NodeCountMax(0),
	m_InternalAllocation(false),
	m_SortIncreasing(sortIncreasing)
{
	if(MT_SAFE)
	{
		m_CS.Init();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CSortedArray Destructor
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>				
CSortedArray<K, T, MT_SAFE>::~CSortedArray()
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
//	memory : A pointer to the memory that will be used for the sorted array
//	maxCount : The number of array nodes that memory points to.  Only used
//		when memory is not NULL.
//	growCount : Determines the number of nodes that the array will increase by.
//		Can be zero if the array should not grow.
//
// Remarks
//	Used when the class is a wrapper for already allocated memory.  The
//	memory must also account for the key as well.  
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>		
void CSortedArray<K, T, MT_SAFE>::Init(void* memory, unsigned int maxCount, unsigned int growCount)
{
	DBG_ASSERT(m_Memory == NULL);
	
	if(memory)
	{
		m_Memory = (ARRAY_NODE*)(memory);
		m_NodeCountMax = maxCount;
		m_NodeCount = 0;
	}
	
	m_GrowCount = growCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	initialCount : The number of item to initialize the array with
//	growCount : Determines the number of nodes that the array will increase by.
//		Can be zero if the array should not grow.
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>		
bool CSortedArray<K, T, MT_SAFE>::Init(unsigned int initialCount, unsigned int growCount)
{
	DBG_ASSERT(m_Memory == NULL);
	
	// Allocate memory for the node array
	//
	m_Memory = (ARRAY_NODE*)InternalAlloc(sizeof(ARRAY_NODE) * initialCount);	

	if(m_Memory == NULL)
	{
		return false;
	}
		
	m_NodeCount = 0;
	m_NodeCountMax = initialCount;
	m_GrowCount = growCount;		
	
	return true;
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
template<typename K, typename T, bool MT_SAFE>		
void CSortedArray<K, T, MT_SAFE>::SetAllocator(IMemoryAllocator* allocator)
{
	if(m_InternalAllocation == false)
	{
		m_Allocator = allocator;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Insert
//
// Parameters
//	key : The key by which the value will be inserted into the array
//	value : The value to add into the array
//  index : [out/optional] A pointer that will be set to the index of the item
//	 insertion point.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::Insert(const K& key, const T& value, unsigned int* index)
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE* itrFinal = NULL;
	Find(key, &itrFinal);
	
	unsigned int insertionIndex = (MEMORY_SIZE)(reinterpret_cast<BYTE*>(itrFinal) - reinterpret_cast<BYTE*>(m_Memory)) / sizeof(ARRAY_NODE);
	bool hasGrown = false;								
	if(m_NodeCount >= m_NodeCountMax)
	{		
		hasGrown = GrowAtIndex(insertionIndex);
		
		if(hasGrown == false && m_Memory == NULL)
		{
			return false; // Handle out of memory
		}		
	}
	
	if(insertionIndex < m_NodeCountMax)
	{
		// Create some space for the node by moving all the nodes
		// after it by one.
		//
		if(insertionIndex < m_NodeCount && hasGrown == false)
		{
			memmove(m_Memory + insertionIndex + 1, m_Memory + insertionIndex, sizeof(ARRAY_NODE) * MIN<unsigned int>((m_NodeCount - insertionIndex), m_NodeCountMax - (insertionIndex + 1)));
		}

		if(m_NodeCount < m_NodeCountMax)
		{
			++m_NodeCount;
		}
	
		m_Memory[insertionIndex].Key = key;
		m_Memory[insertionIndex].Value = value;	

		if(index)
		{
			*index = insertionIndex;
		}
		
		return true;
	}
	else
	{
		return false;
	}
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Remove
//
// Parameters
//	key : The key for the node to remove
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::Remove(const K& key)
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE** itrFinal = NULL;
	ARRAY_NODE* node = Find(key, itrFinal);
	
	if(node == NULL)
	{
		return false;
	}
	
	unsigned int nodeIndex = (unsigned int)(node - m_Memory);
	
	return RemoveAt(nodeIndex, false);
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveAt
//
// Parameters
//	index : The index of the item to remove
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::RemoveAt(const unsigned int index)
{
	return RemoveAt(index, true);	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveAll
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>						
void CSortedArray<K, T, MT_SAFE>::RemoveAll()
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);

	m_NodeCount = 0;
}



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	key : The key for the item to find
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	A pointer to the value if found or NULL if not found
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
T* CSortedArray<K, T, MT_SAFE>::Find(const K& key, unsigned int* index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	ARRAY_NODE* node = Find(key);
	
	if(node)
	{
		if(index)
		{
			*index = (unsigned int)(node - m_Memory);
		}
		return &node->Value;
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	value : The value for the item to find
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//	key : [out/optional] A pointer to a key that will be set to the key of the
//		item if it is found.
//
// Returns
//	A pointer to the value if found or NULL if not found
//
// Remarks
//	It is necessary to iterate through the entire array when the key is
//	not know.
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
T* CSortedArray<K, T, MT_SAFE>::Find(const T& value, unsigned int* index, K** key)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	for(unsigned int i = 0; i < m_NodeCount; ++i)
	{
		ARRAY_NODE* node = &m_Memory[i];
		
		if(node->Value == value)
		{
			if(index)
			{
				*index = (unsigned int)(node - m_Memory);
			}
			
			if(key)
			{
				*key = &node->Key;
			}
		
			return &node->Value;
		}
	}
	
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FindOrBefore
//
// Parameters
//	key : The key find
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	A pointer to the value equal or just before the key
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
T* CSortedArray<K, T, MT_SAFE>::FindOrBefore(const K& key, unsigned int* index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE* itrFinal = NULL;
	ARRAY_NODE* node = Find(key, &itrFinal);
	
	// If the key was found
	//
	if(node)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(node - m_Memory);
		}
		return &node->Value;
	}
	// Otherwise itrFinal points to the value just greater than
	// the key.  If it isn't the first key, return the value just
	// before itrFinal
	// 
	else if(itrFinal > m_Memory)
	{
		// Get the node just before
		//
		itrFinal = itrFinal - 1;
		
		if(index)
		{
			*index = static_cast<unsigned int>(itrFinal - m_Memory);		
		}
		return &itrFinal->Value;
	}
	
	return NULL;
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FindOrBefore
//
// Parameters
//	key : The key find
//  item : [out] A reference to an item that will be set to the found item
//		if it exists
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	True if the item was found for the key and item is valid, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::FindOrBefore(const K& key, T& item, unsigned int* index = NULL)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE* itrFinal = NULL;
	ARRAY_NODE* node = Find(key, &itrFinal);
	
	// If the key was found
	//
	if(node)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(node - m_Memory);
		}
		
		item = node->Value;
		return true;		
	}
	// Otherwise itrFinal points to the value just greater than
	// the key.  If it isn't the first key, return the value just
	// before itrFinal
	// 
	else if(itrFinal > m_Memory)
	{
		// Get the node just before
		//
		itrFinal = itrFinal - 1;
		
		if(index)
		{
			*index = static_cast<unsigned int>(itrFinal - m_Memory);		
		}
		
		item = itrFinal->Value;
		return true;		
	}
	
	return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FindOrAfter
//
// Parameters
//	key : The key find
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	A pointer to the value equal to or after the key
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
T* CSortedArray<K, T, MT_SAFE>::FindOrAfter(const K& key, unsigned int* index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE* itrFinal = NULL;
	ARRAY_NODE* node = Find(key, &itrFinal);
	
	// If the key was found
	//
	if(node)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(node - m_Memory);	
		}
		return &node->Value;
	}
	// Otherwise itrFinal points to the value just greater than
	// the key.  If it isn't the end of the list, return that value
	// 
	else if(itrFinal < m_Memory + m_NodeCount)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(itrFinal - m_Memory);			
		}
		return &itrFinal->Value;
	}
	
	return NULL;
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FindOrAfter
//
// Parameters
//	key : The key find
//  item : [out] A reference to an item that will be set to the found item
//		if it exists
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the sorted array for the index to not change
//
// Returns
//	True if the item was found for the key and item is valid, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::FindOrAfter(const K& key, T& item, unsigned int* index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	ARRAY_NODE* itrFinal = NULL;
	ARRAY_NODE* node = Find(key, &itrFinal);
	
	// If the key was found
	//
	if(node)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(node - m_Memory);	
		}
		
		item = node->Value;
		return true;
		
	}
	// Otherwise itrFinal points to the value just greater than
	// the key.  If it isn't the end of the list, return that value
	// 
	else if(itrFinal < m_Memory + m_NodeCount)
	{
		if(index)
		{
			*index = static_cast<unsigned int>(itrFinal - m_Memory);			
		}
		
		item = itrFinal->Value;
		return true;		
	}
	
	return false;
}			

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FreeAll
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::FreeAll()
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);

	if(m_Memory && m_InternalAllocation)
	{				
		// Release the memory for the nodes
		//
		InternalFree(m_Memory);
	}
	m_Memory = NULL;
	m_InternalAllocation = false;
	m_NodeCount = 0;
	m_NodeCountMax = 0;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetCount
//
// Returns
//	The number of items in the array
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
unsigned int CSortedArray<K, T, MT_SAFE>::GetCount()
{
	return m_NodeCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetKey
//
// Returns
//	A pointer to the key at the specified position or NULL if the index
//	is out of bounds.
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
K* CSortedArray<K, T, MT_SAFE>::GetKey(unsigned int index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	if(index < m_NodeCount)
	{
		return &m_Memory[index].Key;
	}
	else
	{
		return NULL;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Sort
//
// Parameters
//	index : The index of the item that will be bubble sorted to its proper position
//	newKey : The new key to assign to the indexed item before sorting
//
// Returns
//	The new index of the item.  If the index is out of bounds, it just returns
//	the index and does nothing.
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
unsigned int CSortedArray<K, T, MT_SAFE>::Sort(unsigned int index, const K& newKey)
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);

	if(index >= m_NodeCount)
	{
		return index;
	}
	
	// Give the item a new key
	//
	m_Memory[index].Key = newKey;
	
	do 
	{
		if(index > 0 && IsHigher(m_Memory[index - 1].Key, m_Memory[index].Key))
		{
			// The indexed item is too high and needs to be bubbled down
			//	
			ARRAY_NODE lowerNode = m_Memory[index - 1];
			m_Memory[index - 1] = m_Memory[index];
			m_Memory[index] = lowerNode;			
			
			--index;
		}
		else if(index < m_NodeCount - 1 && IsLower(m_Memory[index + 1].Key, m_Memory[index].Key))
		{
			// The indexed item is too low and needs to be bubbled up
			//
			ARRAY_NODE higherNode = m_Memory[index + 1];
			m_Memory[index + 1] = m_Memory[index];
			m_Memory[index] = higherNode;			
			
			++index;
		}
		else
		{
			// No more sorting needs to be done
			//
			break;
		}
	
	} while(index > 0 && index < m_NodeCount - 1);	
	
	return index;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	operator[]
//
// Returns
//	A pointer to the item at the specified position or NULL if the index
//	is out of bounds.
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
T* CSortedArray<K, T, MT_SAFE>::operator[](unsigned int index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	if(index < m_NodeCount)
	{
		return &m_Memory[index].Value;
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Verify
//
// Returns
//	True if the array's key's are sorted correctly, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::Verify()
{
	for(unsigned int i = 1; i < m_NodeCount; ++i)
	{
		ARRAY_NODE* prevNode = &m_Memory[i - 1];
		ARRAY_NODE* node = &m_Memory[i];
		
		if(IsHigher(prevNode->Key, node->Key))
		{
			return false;
		}
	}
	
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
template<typename K, typename T, bool MT_SAFE>						
void* CSortedArray<K, T, MT_SAFE>::InternalAlloc(MEMORY_SIZE size)
{
	m_InternalAllocation = true;
	
	if(m_Allocator)
	{
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
template<typename K, typename T, bool MT_SAFE>						
void CSortedArray<K, T, MT_SAFE>::InternalFree(void* ptr)
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
//	RemoveAt
//
// Parameters
//	index : The index of the item to remove
//  useLock : Specifies whether to use the internal lock
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::RemoveAt(const unsigned int index, bool useLock)
{
	RWFairWriteAutoLock lock(useLock && MT_SAFE ? &m_CS : NULL);

	// If the node isn't the last node
	//
	if(index < m_NodeCount - 1)
	{
		// Move all the nodes above it down one
		//
		memmove(m_Memory + index, m_Memory + index + 1, sizeof(ARRAY_NODE) * (m_NodeCount - index - 1));
	}
	
	--m_NodeCount;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	key : The key for the node to find
//	itrFinal : [out] A reference to a pointer that will be set to the final
//		iterator value if the key was not found.  If the key is before
//		all keys in the array, itrFinal will point to the beginning of
//		the array.  If the key is after all keys in the array, itrFinal
//		will point past the end of the array
//
// Returns
//	A pointer to the node if the key was found, NULL if the key was not
//	found.  If the key was found, itrFinal will also point to the node
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
typename CSortedArray<K, T, MT_SAFE>::ARRAY_NODE* CSortedArray<K, T, MT_SAFE>::Find(const K& key, ARRAY_NODE** itrFinal = NULL)
{
	ARRAY_NODE* begin = m_Memory;
	ARRAY_NODE* end = m_Memory + m_NodeCount;
	ARRAY_NODE* itr = NULL;
	
	// Do a binary search through the array
	//			
	while(begin < end)
	{
		// Set itr to half way between being and end
		//			
		itr = begin + ((end - begin) / 2);
		
		if(IsLower(key, itr->Key))
		{
			end = itr;
		}
		else if(IsHigher(key, itr->Key))
		{
			begin = itr + 1;
		}
		else
		{
			// The key was found in the array
			//	
			if(itrFinal)
			{
				*itrFinal = itr;
			}			
			return itr;
		}
	}
	
	// The key was not found
	//
	if(itrFinal)
	{
		*itrFinal = end;
	}			

	return NULL;
}			

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GrowAtIndex
//
// Parameters
//	growIndex : The index of the node at which pointer all nodes are shifted
//		down by one, including the index node
//
// Returns
//	false if the method fails, true otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::GrowAtIndex(unsigned int growIndex)
{
	if(m_GrowCount == 0)
	{
		return false;
	}

	// Figure out the new max node count
	//
	unsigned int newCountMax = m_NodeCountMax + m_GrowCount;
	
	// Allocate new memory for the node array
	//
	bool wasInternalAllocation = m_InternalAllocation;
	ARRAY_NODE* newMemory = (ARRAY_NODE*)InternalAlloc(sizeof(ARRAY_NODE) * newCountMax);
	if(newMemory == NULL)
	{
		return false;
	}			
							
	if(m_Memory)
	{
		// Copy memory that comes before the grow index
		//
		if (growIndex)
		{
			memcpy(newMemory, m_Memory, sizeof(ARRAY_NODE) * growIndex);
		}
		// Copy memory start at the grow index and onwards one node forward
		//
		if (growIndex < m_NodeCount)
		{
			memcpy(newMemory + growIndex + 1, m_Memory + growIndex, sizeof(ARRAY_NODE) * (m_NodeCount - growIndex));
		}
		
		if(wasInternalAllocation)
		{
			InternalFree(m_Memory);
		}
	}
	
	m_Memory = newMemory;	
	m_NodeCountMax = newCountMax;

	return true;
}		

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	IsLower
//
// Parameters
//	left : A reference to the left operand to compare
//	right : A reference to the right operand to compare
//
// Returns
//	True if the left operand should be placed at the lower index in the
//	sorted array than the right operand
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::IsLower(const K& left, const K& right)
{
	if(m_SortIncreasing)
	{
		return left < right;
	}
	else
	{
		return left > right;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	IsHigher
//
// Parameters
//	left : A reference to the left operand to compare
//	right : A reference to the right operand to compare
//
// Returns
//	True if the left operand should be placed at the higher index in the
//	sorted array than the right operand
//
/////////////////////////////////////////////////////////////////////////////
template<typename K, typename T, bool MT_SAFE>						
bool CSortedArray<K, T, MT_SAFE>::IsHigher(const K& left, const K& right)
{
	if(m_SortIncreasing)
	{
		return left > right;
	}
	else
	{
		return left < right;
	}
}

} // end namespace FSCommon

#pragma warning(pop)