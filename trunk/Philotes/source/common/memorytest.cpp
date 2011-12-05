//----------------------------------------------------------------------------
// memorytest.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "memorytest.h"
#include "random.h"
#include "memorymanager_i.h"
#include "memoryallocator_crt.h"
#include "memoryallocator_heap.h"
#include "memoryallocator_pool.h"

namespace FSCommon
{

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int SIZE>
struct MPBTestStack
{
	unsigned int *					list;
	unsigned int					count;

	MPBTestStack(
		void) 
	{ 
		count = 0; 
		list = new unsigned int[SIZE]; 
	}

	~MPBTestStack(
		void) 
	{ 
		delete list; 
	}

	void Push(
		unsigned int val)
	{ 
		list[count++] = val; 
	}

	unsigned int Pop(
		void) 
	{ 
		return list[--count]; 
	}

	void Reverse(
		void)
	{
		if (count <= 0)
		{
			return;
		}
		for (unsigned int ii = 0, jj = count - 1; ii < jj; ++ii, --jj)
		{
			unsigned int temp = list[ii];
			list[ii] = list[jj];
			list[jj] = temp;
		}
	}

	void Shuffle(
		RAND & rand)
	{
		unsigned int ii = 0;
		do
		{
			unsigned int remaining = count - ii;
			if (remaining <= 1)
			{
				break;
			}
			unsigned int jj = ii + RandGetNum(rand, remaining);
			unsigned int temp = list[jj];
			list[jj] = list[ii];
			list[ii] = temp;
		} while (++ii);
	}
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct MPB_TEST
{
	IMemoryAllocator* Allocator;
	volatile long *				lGo;
	volatile long *				lDone;
	unsigned int				index;
};


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sMPBTestGetSize
//
/////////////////////////////////////////////////////////////////////////////
static MEMORY_SIZE sMPBTestGetSize(
	RAND & rand)
{
	/*
	static const unsigned int MAX_BLOCK_SIZE_3 = 26000;
	static const unsigned int MAX_BLOCK_SIZE_3_CHANCE = 1024;
	static const unsigned int MAX_BLOCK_SIZE_2 = 4096;
	static const unsigned int MAX_BLOCK_SIZE_2_CHANCE = 256;
	static const unsigned int MAX_BLOCK_SIZE_1 = 512;
	*/
	
	
	static const unsigned int MAX_BLOCK_SIZE_3 = 4 * MEGA;
	static const unsigned int MAX_BLOCK_SIZE_3_CHANCE = 1024;
	static const unsigned int MAX_BLOCK_SIZE_2 = 2 * MEGA;
	static const unsigned int MAX_BLOCK_SIZE_2_CHANCE = 256;
	static const unsigned int MAX_BLOCK_SIZE_1 = 512 * KILO;

/*	
	static const unsigned int MAX_BLOCK_SIZE_3 = 256000;
	static const unsigned int MAX_BLOCK_SIZE_3_CHANCE = 1024;
	static const unsigned int MAX_BLOCK_SIZE_2 = 68000;
	static const unsigned int MAX_BLOCK_SIZE_2_CHANCE = 256;
	static const unsigned int MAX_BLOCK_SIZE_1 = 24000;
*/	
	

	if (RandGetNum(rand, MAX_BLOCK_SIZE_3_CHANCE) == 0)
	{
		return RandGetNum(rand, MAX_BLOCK_SIZE_2, MAX_BLOCK_SIZE_3);
	}
	if (RandGetNum(rand, MAX_BLOCK_SIZE_2_CHANCE) == 0)
	{
		return RandGetNum(rand, MAX_BLOCK_SIZE_1 + 1, MAX_BLOCK_SIZE_2);
	}
	//return RandGetNum(rand, 1, MAX_BLOCK_SIZE_1);
	return RandGetNum(rand, 4096, MAX_BLOCK_SIZE_1);
}



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemPoolBinTestThread
//
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI MemPoolBinTestThread(void* param)
{
	struct MBTNode
	{
		void *								ptr;
		unsigned int						size;
	};

	MPB_TEST * mpbtest = (MPB_TEST *)param;

	IMemoryAllocator* allocator = mpbtest->Allocator;

	unsigned int index = mpbtest->index;

	RAND rand;
	RandInit(rand, index + 1, index + 1);

	while (*mpbtest->lGo == 0)
	{
		Sleep(0);
	}

	static const unsigned int ITER_ONE = 400;
	static const unsigned int ITER_TWO = 10;
	
	MBTNode * blocks = (MBTNode *)malloc(ITER_ONE * sizeof(MBTNode));

	MPBTestStack<ITER_ONE> freeblocks;
	MPBTestStack<ITER_ONE> unfreed;
	MPBTestStack<ITER_ONE> reallocblocks;

	char debugString[512];
	printf("[%d]  Memory Pool Bin Allocator Test Start\n", index);	
	sprintf_s(debugString, "[%d]  Memory Pool Bin Allocator Test Start\n", index);
	OutputDebugString(debugString);

	DWORD start = GetTickCount();
	// first allocate ITER_ONE blocks
	for (unsigned int ii = 0; ii < ITER_ONE; ++ii)
	{
		MEMORY_SIZE size = sMPBTestGetSize(rand);
		blocks[ii].size = size;
		blocks[ii].ptr = allocator->Alloc(MEMORY_FUNC_FILELINE(size, NULL));
		ASSERT(blocks[ii].ptr);
		memset(blocks[ii].ptr, 1, size);
		unfreed.Push(ii);
	}

	printf("[%d]  Allocate %d blocks:  Time:%d\n", index, ITER_ONE, GetTickCount() - start);
	sprintf_s(debugString, "[%d]  Allocate %d blocks:  Time:%d\n", index, ITER_ONE, GetTickCount() - start);
	OutputDebugString(debugString);

	start = GetTickCount();

	DWORD count = 0;
	// now go through a cycle of free & allocate
	for (unsigned int ii = 0; ii < ITER_TWO; ++ii)
	{
		// free some blocks
		if (unfreed.count > 0)
		{
			unsigned int tofree = RandGetNum(rand, 1, unfreed.count);
			count += tofree;

			unfreed.Shuffle(rand);
			for (unsigned int jj = 0; jj < tofree; ++jj)
			{
				unsigned int index2 = unfreed.Pop();
				allocator->Free(MEMORY_FUNC_FILELINE(blocks[index2].ptr, NULL));
				blocks[index2].ptr = NULL;
				freeblocks.Push(index2);
			}
		}

		// allocate some blocks
		if (freeblocks.count > 0)
		{
			unsigned int toalloc = RandGetNum(rand, 1, freeblocks.count);
			count += toalloc;

			freeblocks.Shuffle(rand);
			for (unsigned int jj = 0; jj < toalloc; ++jj)
			{
				unsigned int index2 = freeblocks.Pop();

				blocks[index2].ptr = allocator->Alloc(MEMORY_FUNC_FILELINE(blocks[index2].size, NULL));
				ASSERT(blocks[index2].ptr);
				memset(blocks[index2].ptr, 1, blocks[index2].size);
				unfreed.Push(index2);
			}
		}

		// realloc some blocks
		if (unfreed.count > 0)
		{
			unsigned int torealloc = RandGetNum(rand, 1, unfreed.count);
			count += torealloc;

			unfreed.Shuffle(rand);
			for (unsigned int jj = 0; jj < torealloc; ++jj)
			{
				unsigned int index2 = unfreed.Pop();

				blocks[index2].size = sMPBTestGetSize(rand);
				blocks[index2].ptr = allocator->Realloc(MEMORY_FUNC_FILELINE(blocks[index2].ptr, blocks[index2].size, NULL));
				ASSERT(blocks[index2].ptr);
				memset(blocks[index2].ptr, 1, blocks[index2].size);
				reallocblocks.Push(index2);
			}
			while (reallocblocks.count)
			{
				unsigned int index2 = reallocblocks.Pop();
				unfreed.Push(index2);
			}
		}
		
		allocator->GetInternals()->MemTrace();
	}

	printf("[%d]  Allocate/Deallocate/Reallocate %d blocks  Time:%d\n", index, count, GetTickCount() - start);
	sprintf_s(debugString, "[%d]  Allocate/Deallocate/Reallocate %d blocks  Time:%d\n", index, count, GetTickCount() - start);
	OutputDebugString(debugString);
	
	start = GetTickCount();
	
	// Free all blocks
	//
	count = unfreed.count;
	while(unfreed.count)
	{
		unsigned int unfreedIndex = unfreed.Pop();
		allocator->Free(MEMORY_FUNC_FILELINE(blocks[unfreedIndex].ptr, NULL));
		blocks[unfreedIndex].ptr = NULL;
	}
	printf("[%d]  Free All: %d blocks  Time:%d\n", index, count, GetTickCount() - start);	
	sprintf_s(debugString, "[%d]  Free All: %d blocks  Time:%d\n", index, count, GetTickCount() - start);
	OutputDebugString(debugString);
	
	if (blocks)
	{
		free(blocks);
	}

	AtomicDecrement(*mpbtest->lDone);

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemoryTest
//
// Parameters
//	allocatorType : The type of the allocator to test
//	threadCount : The number of threads that will be created to use the 
//		allocator
//
// Returns
//	True if the test encounters no errors, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool MemoryTest(MEMORY_ALLOCATOR_TYPE allocatorType, unsigned int threadCount)
{
	IMemoryManager::GetInstance().Init();
	
	IMemoryAllocator* allocator = NULL;
	
	
	switch(allocatorType)
	{
		case MEMORY_ALLOCATOR_POOL:
		{
			CMemoryAllocatorPOOL<true, true, 4096>* poolAllocator = new CMemoryAllocatorPOOL<true, true, 4096>(L"TestPool");
			
			if(poolAllocator->Init(NULL, 0, true, false) == false)
			{
				delete poolAllocator;
				
				ASSERT(0);
				return false;
			}
			
			allocator = poolAllocator;
		
			break;
		}
		case MEMORY_ALLOCATOR_CRT:
		{
#ifdef _DEBUG
			CMemoryAllocatorCRT<true>* crtAllocator = new CMemoryAllocatorCRT<true>(L"TestCRT");
#else		
			CMemoryAllocatorCRT<false>* crtAllocator = new CMemoryAllocatorCRT<false>(L"TestCRT");
#endif	
			
			if(crtAllocator->Init(NULL) == false)
			{
				delete crtAllocator;
				
				ASSERT(0);
				return false;
			}
			
			allocator = crtAllocator;
			
			break;
		}
		case MEMORY_ALLOCATOR_HEAP:
		{
			if(threadCount > 1)
			{
#ifdef _DEBUG			
				CMemoryAllocatorHEAP<true, true, 1024>* heapAllocator = new CMemoryAllocatorHEAP<true, true, 1024>(L"TestHEAP");
#else
				CMemoryAllocatorHEAP<true, false, 1024>* heapAllocator = new CMemoryAllocatorHEAP<true, false, 1024>(L"TestHEAP");
#endif
				
				if(heapAllocator->Init(NULL, 4 * MEGA, 256 * MEGA, 64 * MEGA) == false)
				{
					delete heapAllocator;
					
					ASSERT(0);
					return false;
				}
				
				allocator = heapAllocator;				
			}
			else
			{
#ifdef _DEBUG			
				CMemoryAllocatorHEAP<false, true, 1024>* heapAllocator = new CMemoryAllocatorHEAP<false, true, 1024>(L"TestHEAP");
#else
				CMemoryAllocatorHEAP<false, false, 1024>* heapAllocator = new CMemoryAllocatorHEAP<false, false, 1024>(L"TestHEAP");
#endif
				
				if(heapAllocator->Init(NULL, 4 * MEGA, 256 * MEGA, 64 * MEGA) == false)
				{
					delete heapAllocator;
					
					ASSERT(0);
					return false;
				}
				
				allocator = heapAllocator;				
			}						
		}
	}
	
	IMemoryManager::GetInstance().AddAllocator(allocator);
	
	
	volatile long lGo = 0;
	volatile long lDone = threadCount;

	MPB_TEST test[100];

	for (unsigned int ii = 0; ii < threadCount; ++ii)
	{
		test[ii].Allocator = allocator;
		test[ii].lGo = &lGo;
		test[ii].lDone = &lDone;
		test[ii].index = ii;

		DWORD dwThreadId;
		HANDLE handle = CreateThread(NULL, 0, MemPoolBinTestThread, (void *)&test[ii], 0, &dwThreadId);
		CloseHandle(handle);
	}

	lGo = 1;

	while (lDone != 0)
	{
		Sleep(10);
	}
	
	if(allocator)
	{
		allocator->Term();
		IMemoryManager::GetInstance().RemoveAllocator(allocator);
	}
	delete allocator;
	
	IMemoryManager::GetInstance().Term();

	return true;

}

} // end namespace FSCommon