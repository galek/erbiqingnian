//----------------------------------------------------------------------------
// memorypoolbin.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memorypoolbucket.h"

#define VALIDATE_BIN		0  // Set to 1 to allow validation for debugging

#pragma warning(push)
#pragma warning(disable:4127)
#pragma warning(push)
#pragma warning(disable:6011)


namespace FSCommon
{


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryPoolBin
//
// Remarks
//	A bin is a list of buckets which all have the same block size.
//
//	The list is sorted in one of two ways depending on if Init is called
//	with optimizeForTrim set to true or false.
//
//	Trimming involves removing those buckets from the list that have no
//	outstanding allocations from them.  This allows the total bin size to stay
//	trim.  Buckets are sorted in the list so that buckets with free blocks
//	are always moved to the beginning of the list and buckets with no free
//	blocks are moved to the back of the list.  This gains cache locality since
//	the most recently freed memory locations will be the first to be allocated.
//
//	In order to further reduce the size of the bin, the bucket list can be
//	sorted sequentially according to the number of free blocks so that the 
//	bucket with the least amount of free blocks is always allocated from.  
//	This means that over time, it is more likely that buckets will become
//	completely free and be released from the bin list.
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
class CMemoryPoolBin
{
	
	public:
	
		void								Init(bool optimizeForTrim);
		void								Term();
		bool								CalcInfo(MEMORY_SIZE& in_size, MEMORY_SIZE in_granularity, MEMORY_SIZE in_bucketsize, MEMORY_SIZE maxAllocationSize);
		BYTE*								GetBlock(CMemoryPoolBucket<MT_SAFE>* bucket, MEMORY_SIZE externalSize);
		void								PutBlock(void* ptr, CMemoryPoolBucket<MT_SAFE>* bucket);
		void								Lock();
		void								UnLock();
		void								AddBucket(CMemoryPoolBucket<MT_SAFE>* bucket, bool useFront);
		void								RemoveBucket(CMemoryPoolBucket<MT_SAFE>* bucket);
				
	private:
	
		void								SortBucket(CMemoryPoolBucket<MT_SAFE>* bucket);
		void								MoveToTail(CMemoryPoolBucket<MT_SAFE>* bucket);
		void								MoveToHead(CMemoryPoolBucket<MT_SAFE>* bucket);
		void								Validate();
	
	public:
		
		// A list of CMemoryPoolBuckets sorted by the free block count (increasing)
		//
		CMemoryPoolBucket<MT_SAFE>*			m_Head;
		CMemoryPoolBucket<MT_SAFE>*			m_Tail;
		CMemoryPoolBucket<MT_SAFE>*			m_FirstNonEmptyBucket;

		// All buckets in the bin contain blocks of this size
		//
		MEMORY_SIZE							m_BlockSize; // max allocation size for this bin

		// The size in bytes that will be allocated for each bucket in the bin.  This is a 
		// multiple of the allocation granularity which is a multiple of the page size.
		//
		MEMORY_SIZE							m_BucketSize;	// total size of a bucket

		// The number of blocks that fit in each bucket in the bin
		//
		unsigned int						m_BlockCountMax;
		
		// The number of free blocks in all buckets
		//
		unsigned int						m_FreeBlocks;
		
		// The number of used blocks in all buckets
		//
		unsigned int						m_UsedBlocks;		

		// In order to be able to return the size of an allocation, each block within the bucket
		// has a corresponding external allocation size stored after the block data itself.  The 
		// size of the size field depends on the size of the block, using the least amount of 
		// bits necessary.  This member stores the number of bits.
		//
		unsigned int						m_SizeBits; // bits needed to store the size
	
		// Used when manipulating bins
		//
		CCriticalSection					m_CS;
		
		// The number of allocated buckets in the bin
		//
		unsigned int						m_BucketCount;
		unsigned int						m_BucketCountPeak;
		
		// If true, causes the buckets to get sorted by free block count for 
		// better trimming
		//
		bool								m_OptimizeForTrim;
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	optimizeForTrim : If true, makes the bin's buckets get sorted by free
//		block size so allocations come from the bucket with the least
//		free blocks.  This should help out with trimming, but decrease
//		performance.
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::Init(bool optimizeForTrim)
{
	if (MT_SAFE)
	{
		m_CS.Init();
	}
	
	m_OptimizeForTrim = optimizeForTrim;
	m_Head = NULL;
	m_Tail = NULL;
	m_FirstNonEmptyBucket = NULL;
	m_BlockSize = 0;
	m_BucketSize = 0;
	m_BlockCountMax = 0;
	m_SizeBits = 0;
	m_FreeBlocks = 0;
	m_UsedBlocks = 0;
	m_BucketCount = 0;
	m_BucketCountPeak = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::Term()
{
	m_BlockSize = 0;
	m_BucketSize = 0;
	m_BlockCountMax = 0;
	m_SizeBits = 0;	
	m_FreeBlocks = 0;
	m_UsedBlocks = 0;
	m_BucketCount = 0;
	m_BucketCountPeak = 0;
	
	// Terminate all buckets
	//
	while(m_Head)
	{
		m_Head->Term();		
		m_Head = m_Head->Next;
	}
	
	m_Head = NULL;
	m_Tail = NULL;	
	m_FirstNonEmptyBucket = NULL;

	if (MT_SAFE)
	{
		m_CS.Free();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AddBucket
//
// Parameters
//	bucket : A pointer to the bucket to be added to the bin's bucket list
//	useFront : If true, adds the bucket to the front of the list, otherwise
//		adds the bucket to the back
//
// Remarks
//	buckets only get added to the bin's bucket list when there are no more available blocks
//	in the bin's currently allocated buckets. 
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::AddBucket(CMemoryPoolBucket<MT_SAFE>* bucket, bool useFront)
{
	++m_BucketCount;	
	m_BucketCountPeak = MAX<unsigned int>(m_BucketCountPeak, m_BucketCount);

	// Add the bucket to the front of the list
	//
	if(useFront)
	{
		if(m_Head)
		{
			bucket->Next = m_Head;
			bucket->Next->Prev = bucket;
		}
		else
		{
			bucket->Next = NULL;
		}
		m_Head = bucket;
		bucket->Prev = NULL;

		if(m_Tail == NULL)
		{
			m_Tail = bucket;
		}	
	}
	// Add the bucket to the tail of the list
	//
	else
	{
		if(m_Tail)
		{
			m_Tail->Next = bucket;
			bucket->Prev = m_Tail;
		}
		else
		{
			bucket->Prev = NULL;
		}
		m_Tail = bucket;
		bucket->Next = NULL;
		
		if(m_Head == NULL)
		{
			m_Head = bucket;
		}
	}
	
	m_FirstNonEmptyBucket = bucket;
	DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());
	
	m_FreeBlocks += m_BlockCountMax;	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveBucket
//
// Parameters
//	bucket : A pointer to the bucket to be removed from the bin's bucket list
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::RemoveBucket(CMemoryPoolBucket<MT_SAFE>* bucket)
{
	--m_BucketCount;

	// Adjust head
	//
	if(m_Head == bucket)
	{
		m_Head = bucket->Next;
	}
	
	// Adjust tail
	//
	if(m_Tail == bucket)
	{
		m_Tail = bucket->Prev;
	}
	
	// Adjust previous
	//
	if(bucket->Prev)
	{
		bucket->Prev->Next = bucket->Next;
	}
	
	// Adjust next
	//
	if(bucket->Next)
	{
		bucket->Next->Prev = bucket->Prev;
	}
	
	if(m_FirstNonEmptyBucket == bucket)
	{
		m_FirstNonEmptyBucket = m_FirstNonEmptyBucket->Next;
	
		if(m_FirstNonEmptyBucket && m_FirstNonEmptyBucket->GetFreeBlockCount() == 0)
		{
			m_FirstNonEmptyBucket = NULL;
		}
		
		DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());
	}
	
	m_FreeBlocks -= m_BlockCountMax;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SortBucket
//
// Parameters
//	bucket : A pointer to the bucket to be sorted in the bin's bucket list
//
// Remarks
//	This only gets called when a block is freed, so the bucket's free block
//	count will always be increasing
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::SortBucket(CMemoryPoolBucket<MT_SAFE>* bucket)
{	
	CMemoryPoolBucket<MT_SAFE>* nextBucket = bucket->Next;
	CMemoryPoolBucket<MT_SAFE>* insertAfterBucket = NULL;

	while(nextBucket)
	{
		if(bucket->GetFreeBlockCount() > nextBucket->GetFreeBlockCount())
		{
			insertAfterBucket = nextBucket;
			nextBucket = nextBucket->Next;
		}		
		else
		{
			break;
		}
	}
	
	if(m_FirstNonEmptyBucket == NULL || bucket->GetFreeBlockCount() <= m_FirstNonEmptyBucket->GetFreeBlockCount())
	{
		m_FirstNonEmptyBucket = bucket;
	}
	DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());

	if(insertAfterBucket)
	{
		// Pull the bucket out of the list
		//
		if(m_Head == bucket)
		{
			m_Head = bucket->Next;
		}
		
		if(m_Tail == bucket)
		{
			m_Tail = bucket->Prev;
		}
		
		if(bucket->Prev)
		{
			bucket->Prev->Next = bucket->Next;
		}
		
		if(bucket->Next)
		{
			bucket->Next->Prev = bucket->Prev;
		}	
		
		// Insert the bucket after the insertAfterBucket
		//
		if(m_Tail == insertAfterBucket)
		{
			m_Tail = bucket;
			bucket->Next = NULL;
		}
		
		if(insertAfterBucket->Next)
		{
			insertAfterBucket->Next->Prev = bucket;
			bucket->Next = insertAfterBucket->Next;
		}
		
		insertAfterBucket->Next = bucket;
		bucket->Prev = insertAfterBucket;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CalcInfo
//
// Parameters
//	in_size : [in/out] Contains the bucket block size of the last bin that was created.  Is set
//		to the size of the next bucket to create.
//	in_granularity : The number of bytes to increment "size" by when determining successive
//		bucket block sizes
//	in_bucketsize : The number of bytes that the next bucket will allocate for it's block memory
//	maxAllocationSize : Specifies the maximum block size allowed for a bin.  Zero indicates no
//		max block size.
//
// Returns
//	true is the bin should be created given the returned in_size value, false if the bin
//	should not be created due to the fact that the next definition bucket block size is
//	a better fit or an error has been detected.
//
// Remarks
//	This method calculates the block size and count for any bucket created for the bin.  It 
//	takes into account overhead needed to store the external allocation sizes within the
//	bucket's block memory. 
//
//	If the bin measurements are such that the next higher block sized bin would hold the same
//	amount of blocks in the same amount of memory, the method uses the next higher 
//	measurements instead.
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
bool CMemoryPoolBin<MT_SAFE>::CalcInfo(MEMORY_SIZE& in_size, MEMORY_SIZE in_granularity, MEMORY_SIZE in_bucketsize, MEMORY_SIZE maxAllocationSize)
{
	ASSERT_RETFALSE(in_granularity > 0);
	ASSERT_RETFALSE(in_bucketsize > 0);

	// Increment the size of the previous bin to get the current bin's bucket block size
	//
	in_size += in_granularity;

	// Figure out a loose estimate at the count not taking into account any size array overhead
	//
	unsigned int count = in_bucketsize / in_size;
	ASSERT_RETFALSE(count > 0);

	// At the end of the bucket's block memory is stored an array of external allocation
	// sizes (one for each block).  These sizes only need as many bits as necessary to store
	// the maximum size of a block (-1 because we never have size 0 and can use 0 to represent 1).
	// Figure out how many bits are necessary for each size value
	//
	unsigned int in_sizebits = BITS_TO_STORE(in_size - 1);

	// If the number of blocks is greater than will fit in the bucket's fixed-size external size array
	//
	if(count > CMemoryPoolBucket<MT_SAFE>::cBucketSizeArraySize)
	{	
		// Reduce the count to make room for the external allocation size array at the back of the bucket memory
		//
		while(count * in_size + ((in_sizebits * (count - CMemoryPoolBucket<MT_SAFE>::cBucketSizeArraySize)) + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE > in_bucketsize)
		{
			--count;
		}
		ASSERT_RETFALSE(count > 0);
	}

	MEMORY_SIZE increment = in_granularity;

	// Figure out if the next potential bin size would fit in the same amount of memory with the same number
	// of blocks.  If so, create the bin based on that next size
	//
	while (1)
	{
		MEMORY_SIZE nextBucketBlockSize = in_size + increment;		
		
		// If there is a max allocation size and the next bucket block size is over the max
		//
		if(maxAllocationSize != 0 && nextBucketBlockSize > maxAllocationSize)
		{
			break;
		}
				
		unsigned int nextBucketBlockSizeBits = BITS_TO_STORE(nextBucketBlockSize - 1);
		unsigned int nextBucketSizeArraySize = 0;
		
		if(count > CMemoryPoolBucket<MT_SAFE>::cBucketSizeArraySize)
		{
			nextBucketSizeArraySize = ((nextBucketBlockSizeBits * (count - CMemoryPoolBucket<MT_SAFE>::cBucketSizeArraySize)) + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE;
		}
		
		// If the next bucket block size wouldn't fix in the same amount of memory with the same number of blocks
		//
		if(((count * nextBucketBlockSize) + nextBucketSizeArraySize) > in_bucketsize)
		{
			// Stop skipping bucket sizes and use the current settings for the bin
			//
			break;
		}
		
		in_size = nextBucketBlockSize;
		in_sizebits = nextBucketBlockSizeBits;
		ASSERT(in_size <= (MEMORY_SIZE)(1 << in_sizebits));
	}

	m_BlockSize = in_size;
	m_BucketSize = in_bucketsize;
	m_BlockCountMax = count;
	m_SizeBits = in_sizebits;	
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBlock
//
// Parameters
//	bucket : A pointer to the bucket to get the block from
//	externalSize : The size of the external allocation request for which 
//		an unused block is being retrieved.
//
// Returns
//	A pointer to the beginning of the unused block or NULL if not available
//
// Remarks
//	This function will allocate new virtual memory if necessary through the creation
//	of an additional bucket.
//
/////////////////////////////////////////////////////////////////////////////
template <bool MT_SAFE>
BYTE* CMemoryPoolBin<MT_SAFE>::GetBlock(CMemoryPoolBucket<MT_SAFE>* bucket, MEMORY_SIZE externalSize)
{
	DBG_ASSERT(bucket);
	DBG_ASSERT(externalSize);

	// Retrieve the index of the next block in the bucket available
	// for external allocation.  This also marks it as used.
	//
	BYTE* internalAllocationPtr = bucket->GetBlock(externalSize);

	if(internalAllocationPtr)
	{		
		if(bucket->GetFreeBlockCount() == 0)
		{
			m_FirstNonEmptyBucket = m_FirstNonEmptyBucket->Next;	
					
			if(m_OptimizeForTrim == false)
			{					
				MoveToTail(bucket);
				
				if(m_FirstNonEmptyBucket && m_FirstNonEmptyBucket->GetFreeBlockCount() == 0)
				{
					m_FirstNonEmptyBucket = NULL;
				}			
			}
			
			DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());			
		}
				
		--m_FreeBlocks;
		++m_UsedBlocks;
	}
	
#if VALIDATE_BIN	
	Validate();
#endif	
	
	
	return internalAllocationPtr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PutBlock
//
// Parameters
//	ptr : The pointer to put back into the bucket
//	bucket : The bucket which the ptr belongs to
//
/////////////////////////////////////////////////////////////////////////////
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::PutBlock(void* ptr, CMemoryPoolBucket<MT_SAFE>* bucket)
{	
	DBG_ASSERT(ptr);
	DBG_ASSERT(bucket);

	bucket->PutBlock(ptr);
	
	--m_UsedBlocks;
	++m_FreeBlocks;
	
	if(m_OptimizeForTrim)
	{
		if(m_FirstNonEmptyBucket && (bucket->GetFreeBlockCount() <= m_FirstNonEmptyBucket->GetFreeBlockCount()))
		{
			m_FirstNonEmptyBucket = m_FirstNonEmptyBucket->Next;
		}
		DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());
		
		SortBucket(bucket);
	}
	// Move the bucket to the head of the bin's bucket list to increase
	// memory locality
	//					
	else
	{
		MoveToHead(bucket);
		m_FirstNonEmptyBucket = m_Head;
		
		DBG_ASSERT(m_FirstNonEmptyBucket == NULL || m_FirstNonEmptyBucket->GetFreeBlockCount());
	}
	
#if VALIDATE_BIN	
	Validate();
#endif
	
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Lock
//
/////////////////////////////////////////////////////////////////////////////
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::Lock()
{
	if(MT_SAFE)
	{
		m_CS.Enter();
	}
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	UnLock
//
/////////////////////////////////////////////////////////////////////////////
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::UnLock()
{
	if(MT_SAFE)
	{
		m_CS.Leave();
	}
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MoveToTail
//
// Parameters
//	bucket : A pointer to the bucket to be moved to the back of the bucket list
//		This bucket must currently be the head of the bin's bucket list.
//
// Remarks
//	Buckets get added to the tail of the bin's bucket list when all blocks have been externally
//	allocated from the bucket.
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::MoveToTail(CMemoryPoolBucket<MT_SAFE>* bucket)
{
	DBG_ASSERT(m_Head == bucket);
	if (m_Tail == bucket)
	{
		return;
	}
	ASSERT_RETURN(bucket->Next);

	// Head
	//
	m_Head = bucket->Next;
	
	// Next
	//
	bucket->Next->Prev = NULL;
	
	// Bucket
	//
	bucket->Next = NULL;
	
	// Tail
	//
	bucket->Prev = m_Tail;	
	m_Tail->Next = bucket;
	m_Tail = bucket;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MoveToHead
//
// Parameters
//	bucket : A pointer to the bucket to be moved to the head of the list
//
// Remarks
//	Buckets get moved to the head of the list when an external allocation is released back
//	into the bin.  Moving the bucket to the head of the list provides some cache locality.
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::MoveToHead(CMemoryPoolBucket<MT_SAFE>* bucket)
{
	if(bucket == m_Head)
	{
		return;
	}
	
	if(bucket->Next)
	{
		DBG_ASSERT(m_Tail != bucket);
		bucket->Next->Prev = bucket->Prev;
	}
	else
	{
		DBG_ASSERT(m_Tail == bucket);
		m_Tail = bucket->Prev;
	}
	ASSERT_RETURN(bucket->Prev);
	bucket->Prev->Next = bucket->Next;

	bucket->Prev = NULL;
	bucket->Next = m_Head;
	if (bucket->Next)
	{
		bucket->Next->Prev = bucket;
	}
	m_Head = bucket;
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Validate
//
/////////////////////////////////////////////////////////////////////////////	
template <bool MT_SAFE>
void CMemoryPoolBin<MT_SAFE>::Validate()
{
	unsigned int prevFreeBlockCount = 0;
	CMemoryPoolBucket<MT_SAFE>* bucket = m_Head;
	bool foundFirstNonEmptyBucket = false;
	
	
	while(bucket)
	{
		if(foundFirstNonEmptyBucket == false && bucket->GetFreeBlockCount())
		{
			DBG_ASSERT(m_FirstNonEmptyBucket == bucket);
		
			foundFirstNonEmptyBucket = true;
		}
	
		DBG_ASSERT(bucket->GetFreeBlockCount() >= prevFreeBlockCount);
		
		prevFreeBlockCount = bucket->GetFreeBlockCount();		
		bucket = bucket->Next;
	}		
	
}
	
				


} // end namespace FSCommon

#pragma warning(pop)
#pragma warning(pop)