//----------------------------------------------------------------------------
// memoryheapbucket.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "sortedarray.h"
#include "memorymanager_i.h"

#pragma warning(push)
#pragma warning(disable: 4324)

namespace FSCommon
{

static const unsigned int cHeapBlockInvalid = 0xFFFFFFFF;

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryHeapBucket
//
// Parameters
//	BLOCK_COUNT : The number of blocks that will be allocated in the bucket.
//		Determined by the bucket size divided by the min allocation size.
//
// Remarks
//	The bucket only deals with internal allocations, not external.  It does not
//	know about alignment or headers.  That is done by the heap allocator
//
/////////////////////////////////////////////////////////////////////////////	
template<unsigned int BLOCK_COUNT>
class __declspec(align(16)) CMemoryHeapBucket
{

	struct HEAP_BLOCK
	{
		BYTE*						Memory;			// A pointer to the internal allocation
		unsigned int				InternalSize;	// Can be greater than the bucket size if an allocation spans multiple buckets
	};
	
	typedef CSortedArray<unsigned int, HEAP_BLOCK, false> FREE_BLOCK_ARRAY;
	typedef CSortedArray<BYTE*, HEAP_BLOCK, false> USED_BLOCK_ARRAY;

	public:
			
		CMemoryHeapBucket();
		
	public:
	
		bool						Init(BYTE* memory, unsigned int memorySize, MEMORY_SIZE maxAllocationSize, bool freeMemoryOnTerminate);
		bool						Term();
		void						SetNextBucket(CMemoryHeapBucket* next);
		void						SetPrevBucket(CMemoryHeapBucket* prev);
		bool						IsInside(BYTE* ptr);
		MEMORY_SIZE					GetMaxAllocationSize(MEMORY_SIZE* nextBucketFrontAllocationSize);
		BYTE*						GetBlock(MEMORY_SIZE size, MEMORY_SIZE& adjustedSize, MEMORY_SIZE& totalAdjustedSize, bool useFront, CMemoryHeapBucket<BLOCK_COUNT>*& sortBucket, unsigned int& sortBackCount, bool& frontBlockChanged, MEMORY_SIZE& oldFrontBlockSize);
		MEMORY_SIZE					PutBlock(BYTE* externalAllocationPtr, bool useFront, CMemoryHeapBucket<BLOCK_COUNT>*& sortBucket, unsigned int& sortBackCount, bool& frontBlockChanged, MEMORY_SIZE& oldFrontBlockSize);
		BYTE*						GetInternalPointer(BYTE* externalAllocationPtr);
		MEMORY_SIZE					GetInternalSize(BYTE* externalAllocationPtr);

	private:

		void						RemoveFreeBlock(unsigned int blockIndex);
		unsigned int				SortFreeBlock(unsigned int blockIndex, MEMORY_SIZE internalSize);
		unsigned int				InsertFreeBlock(MEMORY_SIZE internalSize, HEAP_BLOCK& heapBlock);

		// Debug
		//
		void						VerifyBlocks();
		
	public:
	
		// Points to the next/prev contiguous memory bucket
		//
		CMemoryHeapBucket*			Prev;
		CMemoryHeapBucket*			Next;
		
	public:	
		
		// An array of free heap blocks sorted increasing by size
		//
		FREE_BLOCK_ARRAY			m_FreeBlocks;
		BYTE						m_FreeBlockMemory[BLOCK_COUNT * (sizeof(HEAP_BLOCK) + sizeof(unsigned int))]; // 384-512	
		
		// An array of used heap blocks sorted increasing by their pointer
		//
		USED_BLOCK_ARRAY			m_UsedBlocks;	
		BYTE						m_UsedBlockMemory[BLOCK_COUNT * (sizeof(HEAP_BLOCK) + sizeof(BYTE*))]; // 384-640
							
		BYTE*						m_Memory;
		unsigned int				m_MemorySize;
		MEMORY_SIZE					m_MaxAllocationSize;		// Max internal allocation request size in bytes
		bool						m_FreeMemoryOnTerminate;		
		
		// If the block at the beginning of the memory is in the free list, this is set to
		// that index, otherwise it is out of bounds
		//
		unsigned int				m_FrontFreeBlockIndex;
		unsigned int				m_BackFreeBlockIndex;
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//  CMemoryHeapBucket Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
CMemoryHeapBucket<BLOCK_COUNT>::CMemoryHeapBucket()
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//  Init
//
// Parameters
//	memory : A pointer to the memory for which the bucket wraps
//	memorySize : The size in bytes of the memory pointed to 
//	maxAllocationSize : The largest internal allocation that will be requested
//		through the GetBlock
//	freeMemoryOnTerminate : Specified whether the memory will be released 
//		back to the system memory allocator when Term is called
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
bool CMemoryHeapBucket<BLOCK_COUNT>::Init(BYTE* memory, unsigned int memorySize, MEMORY_SIZE maxAllocationSize, bool freeMemoryOnTerminate)
{
	m_Memory = memory;
	m_MemorySize = memorySize;
	m_MaxAllocationSize = maxAllocationSize;
	m_FreeMemoryOnTerminate = freeMemoryOnTerminate;

	m_FreeBlocks.Init(m_FreeBlockMemory, BLOCK_COUNT, 0);
	m_UsedBlocks.Init(m_UsedBlockMemory, BLOCK_COUNT, 0);

	Prev = NULL;
	Next = NULL;
	
	// Use a single free block for the entire bucket
	//
	HEAP_BLOCK freeBlock;
	freeBlock.Memory = m_Memory;
	freeBlock.InternalSize = m_MemorySize;
	
	unsigned int freeBlockIndex = cHeapBlockInvalid;	
	if(m_FreeBlocks.Insert(m_MemorySize, freeBlock, &freeBlockIndex) == false)
	{
		Term();
		return false;
	}
	
	m_FrontFreeBlockIndex = freeBlockIndex;
	m_BackFreeBlockIndex = freeBlockIndex;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//  Term
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
bool CMemoryHeapBucket<BLOCK_COUNT>::Term()
{
	if(m_Memory)
	{
		if(m_FreeMemoryOnTerminate)
		{
			// Release the memory to the system memory allocator
			//
			IMemoryManager::GetInstance().GetSystemAllocator()->Free(MEMORY_FUNC_FILELINE(m_Memory, NULL));
		}
		m_Memory = NULL;
	}
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//  SetNextBucket
//
// Parameters
//	next : A pointer to the next bucket
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
void CMemoryHeapBucket<BLOCK_COUNT>::SetNextBucket(CMemoryHeapBucket* next)
{
	Next = next;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//  SetPrevBucket
//
// Parameters
//	prev : A pointer to the previous bucket
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
void CMemoryHeapBucket<BLOCK_COUNT>::SetPrevBucket(CMemoryHeapBucket* prev)
{
	Prev = prev;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	IsInside
//
// Parameters
//	ptr : A pointer to check to see if it is inside the bucket
//
// Returns
//	true if the ptr is inside the bucket, false otherwise
//
/////////////////////////////////////////////////////////////////////////////		
template<unsigned int BLOCK_COUNT>
bool CMemoryHeapBucket<BLOCK_COUNT>::IsInside(BYTE* ptr)
{
	return (ptr >= m_Memory && ptr < m_Memory + m_MemorySize);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetMaxAllocationSize
//
// Parameters
//	nextBucketFrontAllocationSize : If valid, a pointer to the front allocation size of
//		the next bucket
//
// Returns
//	The maximum allocation size from the current block taking into account
//	adjacent buckets
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
MEMORY_SIZE CMemoryHeapBucket<BLOCK_COUNT>::GetMaxAllocationSize(MEMORY_SIZE* nextBucketFrontAllocationSize)
{
	MEMORY_SIZE maxSingleBucketAllocationSize = 0;
	MEMORY_SIZE maxMultipleBucketAllocationSize = 0;
	
	// Get the maximum allocation size out of this bucket
	//		
	if(m_FreeBlocks.GetCount())
	{
		maxSingleBucketAllocationSize = m_FreeBlocks[m_FreeBlocks.GetCount() - 1]->InternalSize;
	}
	
	// Get the maximum allocation size for contiguous buckets
	//
	if(maxSingleBucketAllocationSize < m_MaxAllocationSize && m_BackFreeBlockIndex != cHeapBlockInvalid)
	{
		CMemoryHeapBucket* currentBucket = this;
		maxMultipleBucketAllocationSize += m_FreeBlocks[m_BackFreeBlockIndex]->InternalSize;
		
		// Use the supplied front allocation size if available
		//
		if(nextBucketFrontAllocationSize)
		{
			maxMultipleBucketAllocationSize += *nextBucketFrontAllocationSize;
		}
		// Otherwise compute the front allocation size of the next bucket
		//
		else
		{		
			// Stop traversing when the maximum internal allocation size has been reached
			//
			while(maxMultipleBucketAllocationSize < m_MaxAllocationSize) 
			{
				CMemoryHeapBucket* nextBucket = currentBucket->Next;
							
				// If there is a no next bucket, the max allocation size has been found
				//
				if(nextBucket == NULL)
				{
					break;
				}
				
				// If there is no front free block index for the next bucket, the max allocation size has been found
				//
				if(nextBucket->m_FrontFreeBlockIndex == cHeapBlockInvalid)
				{
					break;
				}

				// Add the size of the next bucket's front free block index
				//
				maxMultipleBucketAllocationSize += nextBucket->m_FreeBlocks[nextBucket->m_FrontFreeBlockIndex]->InternalSize;			
				
				// If the next bucket's front free block doesn't span the entire bucket, the max allocation size has been found
				//
				if(nextBucket->m_FrontFreeBlockIndex != nextBucket->m_BackFreeBlockIndex)
				{
					break;
				}
				
				currentBucket = nextBucket;
			}
		}
	}	
	
	MEMORY_SIZE maxAllocationSize = MAX<MEMORY_SIZE>(maxSingleBucketAllocationSize, maxMultipleBucketAllocationSize);
	
	// Enforce the specified max allocation size if it exists
	//
	if(m_MaxAllocationSize)
	{
		maxAllocationSize = MIN<MEMORY_SIZE>(maxAllocationSize, m_MaxAllocationSize);
	}
	
	return maxAllocationSize;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBlock
//
// Parameters
//	size : The size of the requested allocation
//   adjustedSize : [in/out] Will contain the size of the internal allocation
//		due to minimum allocation size on exit.  Should be set to "size"
//		when called from GetBlock.  It differs from totalAdjustedSize in
//		that is records only forward blocks, whereas totalAdjustedSize
//		will eventually be used by AlignedAlloc to determine the total
//		adjusted size of the entire allocation.
//	totalAdjustedSize : [in/out] Will contain the size of the internal
//		allocation due to minimum allocation size on exit.  Should be set
//		to "size" when called from AlignedAlloc.
//	useFront : Always allocated from the front of the bucket.  This is 
//		specified when a previous bucket is allocating across the bucket
//		boundary
//	sortBucket : [out] A pointer to the bucket which requires sorting.  If NULL,
//		no sorting is required.  It may also be necessary to sort buckets
//		previous to the returned bucket.
//	sortBackCount : [out] A pointer to an int that will be set to indicate
//		whether a sort operation needs to traverse back to previous
//		contiguous buckets.  This is set positive when the front block
//		is returned.  The count indicates how many previous contiguous
//		buckets to traverse when sorting
//	frontBlockChanged : [out] Returns whether or not the front block
//		was changed during the operation.  Used by the heap to determine
//		if further sort operations on previous buckets are required.
//	oldFrontBlockSize : [out] Returns the old front block size.
//
// Returns
//	A pointer to allocated memory
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
BYTE* CMemoryHeapBucket<BLOCK_COUNT>::GetBlock(MEMORY_SIZE size, MEMORY_SIZE& adjustedSize, 
	MEMORY_SIZE& totalAdjustedSize, bool useFront, CMemoryHeapBucket<BLOCK_COUNT>*& sortBucket, 
	unsigned int& sortBackCount, bool& frontBlockChanged, MEMORY_SIZE& oldFrontBlockSize)
{
	DBG_ASSERT(size <= m_MaxAllocationSize);
	DBG_ASSERT(size >= m_MemorySize / BLOCK_COUNT);

	frontBlockChanged = false;

	if(m_FreeBlocks.GetCount() == 0)
	{
		return NULL;
	}
	
	// Save the old front block size for use in sorting the bucket in
	// the heap's free list
	//
	if(m_FrontFreeBlockIndex != cHeapBlockInvalid)
	{
		oldFrontBlockSize = m_FreeBlocks[m_FrontFreeBlockIndex]->InternalSize;
	}

	// Find a block from the free list.  The block could be 
	// equal to or larger than the request size, or it could
	// be smaller if there is a contiguous bucket available
	// for allocation
	//
	unsigned int freeBlockIndex = cHeapBlockInvalid;
	HEAP_BLOCK* freeBlock = NULL;

	// Blocks are requested from the front of the bucket's memory when
	// they are requested from the previous adjacent bucket
	//
	if(useFront)
	{
		DBG_ASSERT(m_FrontFreeBlockIndex < m_FreeBlocks.GetCount());

		freeBlockIndex = m_FrontFreeBlockIndex;
		freeBlock = m_FreeBlocks[m_FrontFreeBlockIndex];

	}
	else
	{
		freeBlock = m_FreeBlocks.FindOrAfter(size, &freeBlockIndex);
	}
	
	// If the free block index is out of range, it means that there 
	// is no block in the bucket large enough to satisfy the allocation request.
	// Since we got into this method, it also means that the last block
	// must span into the next bucket and it's combined size must be large enough
	// to satisfy the allocation request
	//
	if(freeBlock == NULL)
	{
		DBG_ASSERT(m_BackFreeBlockIndex < m_FreeBlocks.GetCount());
		
		freeBlockIndex = m_BackFreeBlockIndex;
		freeBlock = m_FreeBlocks[m_BackFreeBlockIndex];
	}
	MEMORY_SIZE availableSize = freeBlock->InternalSize;

	DBG_ASSERT(freeBlock);
	DBG_ASSERT(freeBlockIndex < m_FreeBlocks.GetCount());
		
	// Store the largest allocation size so we can compare after removing
	// the free block and determine if we need to resort this bucket
	//
	MEMORY_SIZE oldMaxAllocationSize = GetMaxAllocationSize(NULL);
	
	// Create a used block
	//
	HEAP_BLOCK usedBlock = *freeBlock;
	
	// If the requested size would cause a free block to become smaller than the 
	// minimum allocation size
	//
	if(availableSize > size && availableSize - size < m_MemorySize / BLOCK_COUNT)
	{
		// Use the entire free block
		//
		totalAdjustedSize += availableSize - size;
		adjustedSize += availableSize - size;
		size = availableSize;
	}
	else
	{
		// If the requested size spills over into the next bucket and the spill over amount
		// is less than the minimum allocation size, increase it to the minimum allocation size
		//
		if(availableSize < size && size - availableSize < m_MemorySize / BLOCK_COUNT)
		{
			MEMORY_SIZE extraSize = (m_MemorySize / BLOCK_COUNT) - (size - availableSize);
			size += extraSize;
			totalAdjustedSize += extraSize;
			adjustedSize += extraSize;
		}
	
	}
	
	usedBlock.InternalSize = size;
	
	// If the requested size takes up the entire block
	//
	unsigned int remainingSize = 0;
	if(size >= availableSize)
	{				
		// Remove the free block from the list
		//
		RemoveFreeBlock(freeBlockIndex);
		
		// Figure out how many additional bytes we need
		//
		remainingSize = (size - availableSize);
		
		// If additional space is needed from the adjacent bucket
		//
		if(remainingSize)
		{		
			sortBackCount++;
		
			DBG_ASSERT(Next);
			bool nextFrontBlockChanged = false;
			MEMORY_SIZE nextOldFrontBlockSize = 0;
			adjustedSize = remainingSize;			
			BYTE* nextMemory = Next->GetBlock(remainingSize, adjustedSize, totalAdjustedSize, true, sortBucket, sortBackCount, nextFrontBlockChanged, nextOldFrontBlockSize);
			
			usedBlock.InternalSize += adjustedSize - remainingSize;
			adjustedSize = usedBlock.InternalSize;
			
			REF(nextMemory);
			DBG_ASSERT(nextMemory);			
		}			
	}
	// The requested size fits is less than the block amount
	//
	else
	{						
		// Adjust the free block
		//
		freeBlock->InternalSize -= size;

		// Use the front of the free block is this is a cross-bucket allocation
		//
		if(useFront)
		{
			freeBlock->Memory += size;
		}
		// Otherwise use the back of the free block in order to reduce 
		// fragmentation
		else
		{
			usedBlock.Memory = freeBlock->Memory + freeBlock->InternalSize; // since it was just decremented by size
			
			// If the block is being taken from the back of the bucket, set the
			// back block index to invalid
			//
			if(freeBlockIndex == m_BackFreeBlockIndex)
			{
				m_BackFreeBlockIndex = cHeapBlockInvalid;
			}
			
			if(freeBlockIndex == m_FrontFreeBlockIndex)
			{
				frontBlockChanged = true;
			}
		}

		// Resort the item in the array since it's size has changed
		//
		freeBlockIndex = SortFreeBlock(freeBlockIndex, freeBlock->InternalSize);
		
	}

	// Add the used block to the list
	//
	m_UsedBlocks.Insert(usedBlock.Memory, usedBlock);	
	
	// Re-sort the bucket in the heap's free list if the maximum allocation
	// size for the bucket has changed and it wasn't set by the next bucket in a
	// spanning block allocation
	//	
	if(GetMaxAllocationSize(NULL) < oldMaxAllocationSize && sortBucket == NULL)
	{
		sortBucket = this;
	}
	else if(useFront && remainingSize == 0)
	{
		// We would get here if this is the last block in a bucket allocation chain
		// where this bucket doesn't need to be re-sorted, but the previous bucket
		// already incremented the sortBlockCount assuming that this bucket needed to
		// be resorted
		//
		DBG_ASSERT(sortBackCount);
		--sortBackCount;		
	}	
	
	// If we are taking a block from the front of the bucket
	//
	if(usedBlock.Memory == m_Memory)
	{
		// Set the front block index to indicate that it 
		// is unavailable
		//
		m_FrontFreeBlockIndex = cHeapBlockInvalid;
		
		frontBlockChanged = true;
	}
	
#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyBlocks();	
#endif
		
	return usedBlock.Memory;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PutBlock
//
// Parameters
//	externalAllocationPtr : A pointer to the internal allocation
//	useFront : This is specified when a previous bucket is freeing across the bucket
//		boundary.
//	sortBucket : [out] A pointer to the bucket which requires sorting.  If NULL,
//		no sorting is requires.  It may also be necessary to sort buckets
//		previous to the returned bucket.
//	sortBackCount : [out] A pointer to an int that will be set to indicate
//		whether a sort operation needs to traverse back to previous
//		contiguous buckets.  This is set positive when the front block
//		is returned.  The count indicates how many previous contiguous
//		buckets to traverse when sorting
//	frontBlockChanged : [out] Returns whether or not the front block
//		was changed during the operation.  Used by the heap to determine
//		if further sort operations on previous buckets are required.
//	oldFrontBlockSize : [out] Returns the old front block size.
//
// Returns
//	The size of the block which externalAllocationPtr belongs
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
MEMORY_SIZE CMemoryHeapBucket<BLOCK_COUNT>::PutBlock(BYTE* externalAllocationPtr, bool useFront, CMemoryHeapBucket<BLOCK_COUNT>*& sortBucket, unsigned int& sortBackCount, bool& frontBlockChanged, MEMORY_SIZE& oldFrontBlockSize)
{
	// Make sure the block is within the bucket
	//
	if(IsInside(externalAllocationPtr) == false)
	{
		DBG_ASSERT(false);				
		return 0;
	}

	// Search for a used block for the pointer
	//
	unsigned int usedBlockIndex = cHeapBlockInvalid;
	HEAP_BLOCK* usedBlock = m_UsedBlocks.FindOrBefore(externalAllocationPtr, &usedBlockIndex);
	MEMORY_SIZE blockSize = usedBlock->InternalSize;
	DBG_ASSERT(usedBlock);
	DBG_ASSERT(externalAllocationPtr >= usedBlock->Memory && externalAllocationPtr < usedBlock->Memory + usedBlock->InternalSize);
	
	// If the used block is the front of the bucket, it could be that due to alignment and header,
	// the start of the allocation is from the previous bucket
	//
	if(usedBlock->Memory == m_Memory && Prev && Prev->m_BackFreeBlockIndex == cHeapBlockInvalid)
	{
		// See if the previous bucket's back used block extends beyond the bounds
		// of the bucket
		//
		HEAP_BLOCK* prevUsedBlock = Prev->m_UsedBlocks[Prev->m_UsedBlocks.GetCount() - 1];
		
		if(prevUsedBlock && prevUsedBlock->Memory + prevUsedBlock->InternalSize > Prev->m_Memory + Prev->m_MemorySize)
		{
			return Prev->PutBlock(prevUsedBlock->Memory, false, sortBucket, sortBackCount, frontBlockChanged, oldFrontBlockSize);
		}
	}
	
	// Figure out how much if any of the allocation was from the next bucket
	//
	MEMORY_SIZE sizeInNextBucket = 0;

	if((usedBlock->Memory + usedBlock->InternalSize) > (m_Memory + m_MemorySize))
	{
		sizeInNextBucket = static_cast<MEMORY_SIZE>((usedBlock->Memory + usedBlock->InternalSize) - (m_Memory + m_MemorySize));
	}
	
	// Create a free block from the used block
	//
	HEAP_BLOCK freeBlock = *usedBlock;
	freeBlock.InternalSize -= sizeInNextBucket;
	unsigned int freeBlockIndex = cHeapBlockInvalid;
	
	// Store these for later when we determine if the bucket need to be re-sorted in the heap's free list	
	//	
	MEMORY_SIZE oldMaxAllocationSize = GetMaxAllocationSize(NULL);
	
	if(m_FrontFreeBlockIndex != cHeapBlockInvalid)
	{
		oldFrontBlockSize = m_FreeBlocks[m_FrontFreeBlockIndex]->InternalSize;
	}

	// Remove the used block
	//
	m_UsedBlocks.RemoveAt(usedBlockIndex);
			
	// Try to coalesce with an adjacent (in address space) free block
	//
	bool foundAdjacentBlock = false;
	for(unsigned int i = 0; i < m_FreeBlocks.GetCount(); ++i)		
	{
		HEAP_BLOCK* block = m_FreeBlocks[i];
		
		// The free block is after the block (in address space)
		//
		if(block->Memory + block->InternalSize == freeBlock.Memory)
		{
			// Increase the size of the block
			//
			block->InternalSize += freeBlock.InternalSize;
			
			foundAdjacentBlock = true;
			freeBlockIndex = i;
			freeBlock = *block;
			
			break;	
		}
		// The free block is before the block (in address space)
		//
		else if(freeBlock.Memory + freeBlock.InternalSize == block->Memory)
		{
			// Replace the block with the free block
			//
			block->Memory = freeBlock.Memory;
			block->InternalSize += freeBlock.InternalSize;
			
			foundAdjacentBlock = true;
			freeBlockIndex = i;
			freeBlock = *block;
			
			break;
		}
	}
	
	// If an adjacent block was found, see if it can now be coalesced with another block
	//
	if(foundAdjacentBlock)
	{
		// Sort the free blocks since the size of the free block has changed
		//
		freeBlockIndex = SortFreeBlock(freeBlockIndex, freeBlock.InternalSize);		
	
		unsigned int removedBlockIndex = cHeapBlockInvalid;
	
		// Look through the free list for a block that is adjacent to the free block (in address space)
		//
		for(unsigned int i = 0; i < m_FreeBlocks.GetCount(); ++i)
		{
			HEAP_BLOCK* block = m_FreeBlocks[i];
			
			// The free block is after the block (in address space)
			//
			if(block->Memory + block->InternalSize == freeBlock.Memory)
			{
				// Increase the size of the block
				//
				block->InternalSize += freeBlock.InternalSize;
				
				// Remove the free block since it is now accounted for by the
				// block before it
				//				
				freeBlock = *block;					
				m_FreeBlocks.RemoveAt(freeBlockIndex);
				removedBlockIndex = freeBlockIndex;
				
				// If the free block is after the block (in the free block array)
				//
				if(freeBlockIndex < i)
				{
					// Compensate for the removal of the free block 
					//
					freeBlockIndex = i - 1;
				}
				else
				{
					freeBlockIndex = i;
				}										
							
				break;	
			}
			// The free block is before the block (in address space)
			//
			else if(freeBlock.Memory + freeBlock.InternalSize == block->Memory)
			{
				// Increase the size of the free block
				//
				m_FreeBlocks[freeBlockIndex]->InternalSize += block->InternalSize;
				freeBlock.InternalSize += block->InternalSize;

				// Remove the block since it is now accounted for by the free block
				//
				m_FreeBlocks.RemoveAt(i);
				removedBlockIndex = i;					
				
				// If the block is before the free block (in the free block array)
				//
				if(freeBlockIndex > i)
				{
					// Compensate for the removal of the block 
					//
					--freeBlockIndex;
				}																																				
				
				break;
			}				
		}			
		
		// Re-sort the free block in the array if two blocks were merged
		//
		if(removedBlockIndex != cHeapBlockInvalid)
		{			
			// Adjust the front/back block indexes
			//
			if(m_FrontFreeBlockIndex != cHeapBlockInvalid && removedBlockIndex < m_FrontFreeBlockIndex)
			{
				--m_FrontFreeBlockIndex;
			}							
			
			if(m_BackFreeBlockIndex != cHeapBlockInvalid && removedBlockIndex < m_BackFreeBlockIndex)
			{
				--m_BackFreeBlockIndex;
			}
		
			freeBlockIndex = SortFreeBlock(freeBlockIndex, freeBlock.InternalSize);		
		}

	}
	// Otherwise, the free block doesn't coalesce with anything and should just be added
	//
	else
	{
		freeBlockIndex = InsertFreeBlock(freeBlock.InternalSize, freeBlock);
	}
	
	// Check to see if the free block is now the front block
	//
	if(freeBlock.Memory == m_Memory)
	{
		m_FrontFreeBlockIndex = freeBlockIndex;
		
		frontBlockChanged = true;
	}
	else
	{
		frontBlockChanged = false;
	}
	
	// Check to see if the free block is now the back block
	//
	if(freeBlock.Memory + freeBlock.InternalSize == m_Memory + m_MemorySize)
	{
		m_BackFreeBlockIndex = freeBlockIndex;
	}		
	
	// If part of the block was allocated from the next bucket
	//
	if(sizeInNextBucket)
	{
		sortBackCount++;
		
		// Free the block in the next bucket as well
		//
		bool nextFrontBlockChanged = false;
		MEMORY_SIZE nextOldFrontBlockSize = 0;
		Next->PutBlock(m_Memory + m_MemorySize, true, sortBucket, sortBackCount, nextFrontBlockChanged, nextOldFrontBlockSize);
	}
	
	MEMORY_SIZE newMaxAllocationSize = GetMaxAllocationSize(NULL);
	
	if(newMaxAllocationSize > oldMaxAllocationSize && sortBucket == NULL)
	{
		sortBucket = this;
	}
	else if((useFront && sizeInNextBucket == 0) || (oldMaxAllocationSize >= newMaxAllocationSize && sortBackCount))
	{
		// We would get here if this is the last block in a bucket allocation chain
		// where this bucket doesn't need to be re-sorted, but the previous bucket
		// already incremented the sortBlockCount assuming that this bucket needed to
		// be resorted.
		//		
		--sortBackCount;		
	}
	

#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyBlocks();	
#endif

	return blockSize;

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternalPointer
//
// Parameters
//	externalAllocationPtr : A pointer to the external allocation for which
//		the internal allocation pointer will be returned if it exists
//
// Returns
//	A pointer to the internal allocation if it exists, or NULL if it does
//	not
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
BYTE* CMemoryHeapBucket<BLOCK_COUNT>::GetInternalPointer(BYTE* externalAllocationPtr)
{
	// Make sure the pointer is in the bucket
	//
	if(IsInside(externalAllocationPtr) == false)
	{
		return NULL;
	}
	
	// Search for a used block for the pointer
	//	
	HEAP_BLOCK* usedBlock = m_UsedBlocks.FindOrBefore(externalAllocationPtr);
	
	// If the used block is the front of the bucket, it could be that due to alignment and header,
	// the start of the allocation is from the previous bucket
	//
	if(usedBlock->Memory == m_Memory && Prev && Prev->m_BackFreeBlockIndex == cHeapBlockInvalid)
	{
		// See if the previous bucket's back used block extends beyond the bounds
		// of the bucket
		//
		HEAP_BLOCK* prevUsedBlock = Prev->m_UsedBlocks.FindOrBefore(externalAllocationPtr);
		
		if(prevUsedBlock && prevUsedBlock->Memory + prevUsedBlock->InternalSize > Prev->m_Memory + Prev->m_MemorySize)
		{
			return Prev->GetInternalPointer(prevUsedBlock->Memory);
		}
	}

	// Check to see if the pointer is within the block
	//
	if(externalAllocationPtr >= usedBlock->Memory && externalAllocationPtr < usedBlock->Memory + usedBlock->InternalSize)
	{
		return usedBlock->Memory;
	}	
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternalSize
//
// Parameters
//	externalAllocationPtr : A pointer to the external allocation for which
//		the internal allocation size will be returned if it exists
//
// Returns
//	The size of the internal allocation if it exists, or zero if it doesn't
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
MEMORY_SIZE CMemoryHeapBucket<BLOCK_COUNT>::GetInternalSize(BYTE* externalAllocationPtr)
{
	// Make sure the pointer is in the bucket
	//
	if(IsInside(externalAllocationPtr) == false)
	{
		return 0;
	}
	
	// Search for a used block for the pointer
	//	
	HEAP_BLOCK* usedBlock = m_UsedBlocks.FindOrBefore(externalAllocationPtr);
	
	// Validate that the pointer is within the block
	//
	if(usedBlock == NULL || externalAllocationPtr < usedBlock->Memory || externalAllocationPtr >= usedBlock->Memory + usedBlock->InternalSize)
	{
		return 0;
	}
	
	// If the used block is the front of the bucket, it could be that due to alignment and header,
	// the start of the allocation is from the previous bucket
	//
	if(usedBlock->Memory == m_Memory && Prev && Prev->m_BackFreeBlockIndex == cHeapBlockInvalid)
	{
		// See if the previous bucket's back used block extends beyond the bounds
		// of the bucket
		//
		HEAP_BLOCK* prevUsedBlock = Prev->m_UsedBlocks.FindOrBefore(externalAllocationPtr);
		
		if(prevUsedBlock && prevUsedBlock->Memory + prevUsedBlock->InternalSize > Prev->m_Memory + Prev->m_MemorySize)
		{
			return Prev->GetInternalSize(prevUsedBlock->Memory);
		}
	}	

	// Check to see if the pointer is within the block
	//
	if(externalAllocationPtr >= usedBlock->Memory && externalAllocationPtr < usedBlock->Memory + usedBlock->InternalSize)
	{
		return usedBlock->InternalSize;
	}	
	else
	{
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveFreeBlock
//
// Parameters
//	blockIndex : The index of the block to remove from the free block index
//
// Remarks
//	This method will fix up the front and back block indexes as a result of the block index
//	being removed
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
void CMemoryHeapBucket<BLOCK_COUNT>::RemoveFreeBlock(unsigned int blockIndex)
{
	// Remove the free block from the list
	//
	m_FreeBlocks.RemoveAt(blockIndex);
			
	// If the block used to be the front block index, reset the front block index
	//
	if(blockIndex == m_FrontFreeBlockIndex)
	{
		m_FrontFreeBlockIndex = cHeapBlockInvalid;
	}
	
	// If the block used to be the back block index, reset the back block index
	//
	if(blockIndex == m_BackFreeBlockIndex)
	{
		m_BackFreeBlockIndex = cHeapBlockInvalid;
	}
	
	// If the removed block was before the front block index, the front block index will have shifted down one
	//
	if(m_FrontFreeBlockIndex != cHeapBlockInvalid && blockIndex < m_FrontFreeBlockIndex)
	{
		--m_FrontFreeBlockIndex;
	}							

	// If the removed block was before the back block index, the back block index will have shifted down one
	//
	if(m_BackFreeBlockIndex != cHeapBlockInvalid && blockIndex < m_BackFreeBlockIndex)
	{
		--m_BackFreeBlockIndex;
	}		

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SortFreeBlock
//
// Parameters
//	blockIndex : The index of the block to sort in the free block index
//	internalSize : The new size of the block in bytes.  This will also become the new
//		key for the block in the free block list
//
// Remarks
//	This method will fix up the front and back block indexes as a result of the block index
//	being removed
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
unsigned int CMemoryHeapBucket<BLOCK_COUNT>::SortFreeBlock(unsigned int blockIndex, MEMORY_SIZE internalSize)
{
	// Sort the free block at the specified index
	//
	unsigned int oldBlockIndex = blockIndex;																					
	unsigned int newBlockIndex = m_FreeBlocks.Sort(blockIndex, internalSize);			
	
	// The front free block index may have changed as a result of the sort
	//
	if(m_FrontFreeBlockIndex != cHeapBlockInvalid)
	{
		if(oldBlockIndex == m_FrontFreeBlockIndex)
		{
			m_FrontFreeBlockIndex = newBlockIndex;
		}
		else if(oldBlockIndex < m_FrontFreeBlockIndex && newBlockIndex >= m_FrontFreeBlockIndex)
		{
			--m_FrontFreeBlockIndex;	
		}
		else if(oldBlockIndex > m_FrontFreeBlockIndex && newBlockIndex <= m_FrontFreeBlockIndex)
		{
			++m_FrontFreeBlockIndex;
		}				
	}							

	// The back free block index may have changed as a result of the sort
	//
	if(m_BackFreeBlockIndex != cHeapBlockInvalid)
	{
		if(oldBlockIndex == m_BackFreeBlockIndex)
		{
			m_BackFreeBlockIndex = newBlockIndex;
		}
		else if(oldBlockIndex < m_BackFreeBlockIndex && newBlockIndex >= m_BackFreeBlockIndex)
		{
			--m_BackFreeBlockIndex; 
		}
		else if(oldBlockIndex > m_BackFreeBlockIndex && newBlockIndex <= m_BackFreeBlockIndex)
		{
			++m_BackFreeBlockIndex;
		}
	}						

	return newBlockIndex;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InsertFreeBlock
//
// Parameters
//	internalSize : The size in bytes of the block to be inserted (also the key)
//	heapBlock : A reference to the heap block to insert
//
// Returns
//	The index of the heap block after insertion
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
unsigned int CMemoryHeapBucket<BLOCK_COUNT>::InsertFreeBlock(MEMORY_SIZE internalSize, HEAP_BLOCK& heapBlock)
{
	// Insert the heap block into the free block list
	//
	unsigned int newBlockIndex = cHeapBlockInvalid;
	m_FreeBlocks.Insert(internalSize, heapBlock, &newBlockIndex);
	
	// Adjust the front block index since the heap block could have been inserted
	// before the front block
	//
	if(m_FrontFreeBlockIndex != cHeapBlockInvalid && newBlockIndex <= m_FrontFreeBlockIndex)
	{
		++m_FrontFreeBlockIndex;
	}							
	
	// Adjust the back block index since the heap block could have been inserted
	// before the back block
	//
	if(m_BackFreeBlockIndex != cHeapBlockInvalid && newBlockIndex <= m_BackFreeBlockIndex)
	{
		++m_BackFreeBlockIndex;
	}			
	
	return newBlockIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	VerifyBlocks
//
// Remarks
//	Verifies that the used blocks are sorted correctly by increasing memory address.  Also
//	verifies that free blocks are sorted correctly by increasing internal size
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BLOCK_COUNT>
void CMemoryHeapBucket<BLOCK_COUNT>::VerifyBlocks()
{
	MEMORY_SIZE totalSize = 0;

	// Verify used blocks
	//
	BYTE* last = NULL;
	for(unsigned int i = 0; i < m_UsedBlocks.GetCount(); ++i)
	{
		HEAP_BLOCK* block = m_UsedBlocks[i];
		
		MEMORY_SIZE internalBlockSize = block->InternalSize;
		if(block->Memory + block->InternalSize > m_Memory + m_MemorySize)
		{
			internalBlockSize = (MEMORY_SIZE)((m_Memory + m_MemorySize) - block->Memory);
		}
		totalSize += internalBlockSize;
		
		DBG_ASSERT(block->Memory > last);
		DBG_ASSERT(block->InternalSize >= m_MemorySize / BLOCK_COUNT);
		
		last = block->Memory;
	}	
	
	// Verify free blocks
	//
	MEMORY_SIZE lastSize = 0;
	for(unsigned int i = 0; i < m_FreeBlocks.GetCount(); ++i)
	{
		HEAP_BLOCK* block = m_FreeBlocks[i];
		
		totalSize += block->InternalSize;
		
		DBG_ASSERT(block->InternalSize >= lastSize);
		DBG_ASSERT(block->InternalSize >= m_MemorySize / BLOCK_COUNT);
		
		lastSize = block->InternalSize;
	}	
	
	DBG_ASSERT(totalSize == m_MemorySize);

}

} // end namespace FSCommon

#pragma warning(pop)