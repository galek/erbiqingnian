//----------------------------------------------------------------------------
// memorypoolbucket.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "bitset.h"

#pragma warning(push)
#pragma warning(disable:4127)

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Class CMemoryPoolBucket
//
// Remarks
//	A bucket contains an array of same-sized memory blocks.  It also contains
//	a fixed-size array of bits to represent each block's allocation state.
//	The external allocation size of each block is stored in the block memory
//	itself, at the end of the array and only uses as many bits as are
//	necessary to fit the block size.
//
//  No critical section is taken when manipulating the bucket.  Is is assumed
//	that the bucket's bin is protecting the bucket from simultaneous access
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE>
class CMemoryPoolBucket
{
	public:
	
		// This determines the maximum number of blocks per bucket and sizes
		// the free block bit set
		//
		static const unsigned int cMaxAllocationsPerBucket = 1024;
		
		// Each bucket has a fixed size array of external allocation sizes based on this number
		//
		static const unsigned int cBucketSizeArraySize = 16;
	
	public:
	
		bool							Init(BYTE* memory, MEMORY_SIZE memorySize, unsigned int blockSize, unsigned int blockCount, unsigned int sizeEntrySize);
		void							Term();
		inline unsigned int				GetFreeBlockCount();
		bool							IsInside(void* ptr);
		void							PutBlock(void* ptr);
		BYTE*							GetBlock(MEMORY_SIZE externalSize);
		MEMORY_SIZE						GetSize(void* ptr);
			
	private:
	
		void							SetAllocationSize(unsigned int blockIndex, MEMORY_SIZE size);
		inline unsigned int				GetBlockIndex(void* ptr);
		inline BYTE*					GetSizeArray(unsigned int& bytes) const;
		unsigned int					MEM_GET_BBUF_VALUE(BYTE * bbuf, unsigned int bytes, unsigned int offset, unsigned int bits);
		void							MEM_SET_BBUF_VALUE(BYTE * bbuf, unsigned int bytes, unsigned int offset, unsigned int bits, unsigned int val);

	public:

		// This is a bit set where each bit corresponds to a block within the bucket memory.
		// If the bit is set, it means that the block is currently available for external allocation
		//
		CBitSet<cMaxAllocationsPerBucket>	m_FreeSet;
		
		// External sizes are stored here until they overflow in which case they
		// spill over into the memory at the back of the bucket
		//
		MEMORY_SIZE						m_SizeArray[cBucketSizeArraySize];
		
		// For use in a pool
		//
		CMemoryPoolBucket<MT_SAFE>*		Next;
		
		// For use in the bin
		//
		CMemoryPoolBucket<MT_SAFE>*		Prev;
		
		BYTE*							m_Memory;			// pointer to actual data
		MEMORY_SIZE						m_MemorySize;		// The size in bytes of the m_Memory
		
		unsigned int					m_BlockCount;		// The total number of blocks in the bucket
		unsigned int					m_BlockSize;		// The size in bytes of each block in the bucket
		unsigned int					m_SizeEntrySize;	// The number of bits per external allocation size entry
		
		
		unsigned int					m_UsedCount;	// number of allocated blocks
		
#ifdef _WIN64		
		BYTE							m_Padding[4];		// The class must be 16-byte aligned
#endif
	
};


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Parameters
//	memory : A pointer to the memory to use for the bucket
//	memorySize : The number of bytes to allocate
//	blockSize : The size of each block in the bucket
//	blockCount : The number of blocks that fit in the bucket
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
bool CMemoryPoolBucket<MT_SAFE>::Init(BYTE* memory, MEMORY_SIZE memorySize, unsigned int blockSize, unsigned int blockCount, unsigned int sizeEntrySize)
{
	m_MemorySize = memorySize;
	m_Memory = memory;
	
	if(m_Memory == NULL)
	{
		ASSERT(0);
		return false;
	}
	
	Next = NULL;
	Prev = NULL;
	
	m_BlockSize = blockSize;
	m_BlockCount = blockCount;
	m_SizeEntrySize = sizeEntrySize;	
	m_UsedCount = 0;

	// set used flags
	if(m_BlockCount > cMaxAllocationsPerBucket)
	{
		ASSERT(0);
		return false;
	}
	
	m_FreeSet.Set();
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
void CMemoryPoolBucket<MT_SAFE>::Term()
{
	m_Memory = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFreeBlockCount
//
// Returns
//	The number of free blocks in the bucket
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
unsigned int CMemoryPoolBucket<MT_SAFE>::GetFreeBlockCount()
{
	return m_BlockCount - m_UsedCount;
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
template<bool MT_SAFE>
bool CMemoryPoolBucket<MT_SAFE>::IsInside(void* ptr)
{
	BYTE* memory = reinterpret_cast<BYTE*>(ptr);
	
	// Verify that there isn't any bucket corruption going on where someone
	// is releasing a pointer that actually points to the external size data
	//
	DBG_ASSERT(ptr < (m_Memory + (m_BlockCount * m_BlockSize)) || ptr > m_Memory + m_MemorySize);
	
	return (memory >= m_Memory && memory < m_Memory + m_MemorySize);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PutBlock
//
// Parameters
//	ptr : A pointer to the block to mark as unused
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
void CMemoryPoolBucket<MT_SAFE>::PutBlock(void* ptr)
{
	// Verify that there are externally allocated blocks in the bucket
	//
	ASSERT_RETURN(m_UsedCount > 0);

	// Figure out the offset of the block within the bucket's memory
	//
	unsigned int blockIndex = GetBlockIndex(ptr);
	ASSERT_RETURN(blockIndex <= m_BlockCount);

	// Verify that the bit is not currently set, indicating that
	// it is used
	//
	ASSERT_RETURN(m_FreeSet.Test(blockIndex) == false);
	
	// Set the bit, indicating that it is available
	//
	m_FreeSet.Set(blockIndex);

	// Keep track of how many blocks are used in the bucket
	//
	--m_UsedCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBlock
//
// Parameters
//	externalSize : The external allocation size
//
// Returns
//	true if there are more blocks available, false if there are none.
//
// Remarks
//	The method also marks the block as used in the free set
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
BYTE* CMemoryPoolBucket<MT_SAFE>::GetBlock(MEMORY_SIZE externalSize)
{
	// Compute the bit's offset in bit-count from the start of the free set
	//
	int blockIndex = m_FreeSet.GetFirstSetBitIndex();			
	if(blockIndex < 0 || blockIndex >= (int)m_BlockCount)
	{
		return NULL;
	}

	// Clear the bit in the free set to indicate that the block is now used in the bucket
	//
	m_FreeSet.Reset(blockIndex);

	++m_UsedCount;

	// Record the size of the external allocation in the bucket's array
	// of external allocation sizes
	//
	SetAllocationSize(blockIndex, externalSize);

	// Return the pointer to the beginning of the block
	//
	return m_Memory + (blockIndex * m_BlockSize);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAllocationSize
//
// Parameters
//	ptr : A pointer for which to return the external allocation size for
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
MEMORY_SIZE CMemoryPoolBucket<MT_SAFE>::GetSize(void* ptr)
{
	unsigned int blockIndex = GetBlockIndex(ptr);
	ASSERT_RETZERO(m_FreeSet.Test(blockIndex) == false);

	if(blockIndex < cBucketSizeArraySize)
	{
		return m_SizeArray[blockIndex];
	}	
	else
	{
		unsigned int bytes;
		BYTE* sizearray = GetSizeArray(bytes);

		return MEM_GET_BBUF_VALUE(sizearray, bytes, (blockIndex - cBucketSizeArraySize) * m_SizeEntrySize, m_SizeEntrySize) + 1;
	}
}	
	
/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetAllocationSize
//
// Parameters
//	blockIndex : The index of the block within the bucket's memory for which the size will be set
//	size : The size in bytes for which the block's 
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
void CMemoryPoolBucket<MT_SAFE>::SetAllocationSize(unsigned int blockIndex, MEMORY_SIZE size)
{
	if(blockIndex < cBucketSizeArraySize)
	{
		m_SizeArray[blockIndex] = size;	
	}
	else
	{
		// Get a pointer to the beginning of the external allocation array along with the
		// size of the array
		//
		unsigned int bytes;
		BYTE * sizearray = GetSizeArray(bytes);

		// Set the value within the external allocation size array to the size of the actual
		// external allocation
		//
		MEM_SET_BBUF_VALUE(sizearray, bytes, (blockIndex - cBucketSizeArraySize) * m_SizeEntrySize, m_SizeEntrySize, (unsigned int)(size - 1));
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBlockIndex
//
// Parameters
//	ptr : A pointer to the block within the bucket
//
// Returns
//	The index of the block within the bucket's memory
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
unsigned int CMemoryPoolBucket<MT_SAFE>::GetBlockIndex(void* ptr)
{
	// Calculate the distance in bytes from the beginning of the bucket's memory
	// to the supplied pointer
	//
	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	
	MEMORY_SIZE diff = (MEMORY_SIZE)(externalAllocationPtr - m_Memory);			
	ASSERT_RETINVALID(diff < m_MemorySize);

	// Calculate the index of the block
	//
	unsigned int blockIndex = (unsigned int)(diff / m_BlockSize);
	ASSERT_RETINVALID(blockIndex < m_BlockCount);
	
	return blockIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetSizeArray
//
// Parameters
//	bytes : [out] A reference to an int that will be set to the size in bytes of the external
//		allocation array.
//
// Returns
//	A pointer to the beginning of the external allocation size array.  This array of sizes
//	is stored at the end of each bucket's memory and facilitates getting the size from 
//	a pointer allocated from the bucket.
//
/////////////////////////////////////////////////////////////////////////////		
template<bool MT_SAFE>
BYTE* CMemoryPoolBucket<MT_SAFE>::GetSizeArray(unsigned int& bytes) const
{
	DBG_ASSERT(m_BlockCount > cBucketSizeArraySize);

	// Figure out how many bytes are currently reserved in the block's memory for the 
	// external allocation size array
	//
	bytes = ROUNDUP(m_SizeEntrySize * (m_BlockCount - cBucketSizeArraySize), (unsigned int)BITS_PER_BYTE) / BITS_PER_BYTE;

	// Return a pointer to the beginning of the external allocation size array
	//
	return m_Memory + m_MemorySize - bytes;
}				

template<bool MT_SAFE>
unsigned int CMemoryPoolBucket<MT_SAFE>::MEM_GET_BBUF_VALUE(BYTE * bbuf, unsigned int bytes, unsigned int offset, unsigned int bits)
{
	DBG_ASSERT(offset + bits <= bytes * BITS_PER_BYTE);
	REF(bytes);

	unsigned int val = 0;
	unsigned int bitsleft = bits;
	BYTE * buf = bbuf + offset / BITS_PER_BYTE;
	// read leading bits
	unsigned int leadingoffs = (offset % BITS_PER_BYTE);
	unsigned int leadingbits = (BITS_PER_BYTE - leadingoffs) % BITS_PER_BYTE;
	if (leadingoffs != 0)
	{
		unsigned int bitstoread = MIN(leadingbits, bits);
		// clear bits we're about to write
		BYTE mask = (BYTE)((1 << bitstoread) - 1);
		val = (*buf & (mask << leadingoffs)) >> leadingoffs;
		++buf;
		bitsleft -= bitstoread;
	}
	// read whole byte bits
	unsigned int temp = 0;
	switch (bitsleft / BITS_PER_BYTE)
	{
	case 4:
		temp += buf[0];
		temp += buf[1] << 8;
		temp += buf[2] << 16;
		temp += buf[3] << 24;
		return val + (temp << leadingbits);
	case 3:
		temp += buf[0];
		temp += buf[1] << 8;
		temp += buf[2] << 16;
		val += (temp << leadingbits);
		bitsleft -= 24;
		buf += 3;
		break;
	case 2:
		temp += buf[0];
		temp += buf[1] << 8;
		val += (temp << leadingbits);
		bitsleft -= 16;
		buf += 2;
		break;
	case 1:
		temp += buf[0];
		val += (temp << leadingbits);
		bitsleft -= 8;
		buf += 1;
		break;
	case 0:
		break;
	default: __assume(0);
	}
	// read trailing bits
	if (bitsleft)
	{
		unsigned int bitstoread = bitsleft;
		BYTE mask = (BYTE)((1 << bitstoread) - 1);
		temp = (*buf & mask);
		val += (temp << (bits - bitsleft));
	}
	return val;
}

template<bool MT_SAFE>
void CMemoryPoolBucket<MT_SAFE>::MEM_SET_BBUF_VALUE(BYTE * bbuf, unsigned int bytes, unsigned int offset, unsigned int bits, unsigned int val)
{
	DBG_ASSERT(bits == 32 || val < ((unsigned int)1 << bits));
	DBG_ASSERT(offset + bits <= bytes * BITS_PER_BYTE);
	REF(bytes);
	
	unsigned int bitsleft = bits;
	BYTE * buf = bbuf + offset / BITS_PER_BYTE;
	// write leading bits in first byte
	unsigned int leadingoffs = (offset % BITS_PER_BYTE);
	unsigned int leadingbits = BITS_PER_BYTE - leadingoffs;
	if (leadingoffs != 0)
	{
		unsigned int bitstowrite = MIN(leadingbits, bits);
		// clear bits we're about to write
		BYTE mask = (BYTE)((1 << bitstowrite) - 1);
		*buf &= ~(mask << leadingoffs);
		*buf |= ((val & mask) << leadingoffs);
		++buf;
		bitsleft -= bitstowrite;
		val >>= bitstowrite;
	}
	// write whole byte bits
	switch (bitsleft / BITS_PER_BYTE)
	{
	case 4:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		buf[2] = (BYTE)(val >> 16) & 0xff;
		buf[3] = (BYTE)(val >> 24) & 0xff;
		return;
	case 3:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		buf[2] = (BYTE)(val >> 16) & 0xff;
		bitsleft -= 24;
		val >>= 24;
		buf += 3;
		break;
	case 2:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		bitsleft -= 16;
		val >>= 16;
		buf += 2;
		break;
	case 1:
		buf[0] = (BYTE)(val & 0xff);
		bitsleft -= 8;
		val >>= 8;
		buf += 1;
		break;
	case 0:
		break;
	default: __assume(0);
	}
	// write trailing bits
	if (bitsleft)
	{
		unsigned int bitstowrite = bitsleft;
		BYTE mask = (BYTE)((1 << bitstowrite) - 1);
		*buf &= ~mask;
		*buf |= val;
	}
}

} // end namespace FSCommon

#pragma warning(pop)
