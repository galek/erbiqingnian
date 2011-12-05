//----------------------------------------------------------------------------
// heap.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
// Implements a binary min-heap.

// Takes arbitrary pointers, but requires a function which
// defines a transitive comparison for said arbitrary pointers.
// Create the pointers yourself.  If they're null and your comparison
// function doesn't handle pointer nullness, crashy crash may ensue.

//Note: SIZE should be a power of 2.  Wastes a pointer of memory.
//----------------------------------------------------------------------------
#ifndef _HEAP_H_
#define _HEAP_H_

#define DEFAULT_HEAP_SIZE 256

#define INVALID_HEAP_POSITION 0

//Writing own swap function to avoid troubles with std::swap

template<typename T>
inline void TEMPLATE_SWAP(
	T & a,
	T & b)
{
	T tmp = a;
	a = b;
	b = tmp;
}


template<class T, unsigned int SIZE, BOOL COMPARE(T * a, T * b )>
struct Heap
{
protected:
	virtual void HeapSwap(int nFirstIndex, int nSecondIndex)
	{
		TEMPLATE_SWAP(m_pItems[nFirstIndex], m_pItems[nSecondIndex]);
	}

	void BubbleUp(int nThisNode)
	{
		while(nThisNode != 1)
		{	//try to bubble node upwards
			//break if fail.
			//HeapSwap if successful.
			if(COMPARE(m_pItems[nThisNode], m_pItems[nThisNode/2]) )
			{
				HeapSwap(nThisNode, nThisNode/2);
				//TEMPLATE_SWAP(m_pItems[nThisNode], m_pItems[nThisNode/2]);
				nThisNode /= 2;
			}
			else break;
		}
	}

	void BubbleDown(int nThisNode)
	{
		while(nThisNode*2 < (m_nItemCount-1)) 
		{
			if(COMPARE(m_pItems[nThisNode*2], m_pItems[nThisNode*2+1]) )
			{
				if(!COMPARE(m_pItems[nThisNode*2], m_pItems[nThisNode]))
					break;
				HeapSwap(nThisNode, nThisNode*2);
				//TEMPLATE_SWAP(m_pItems[nThisNode], m_pItems[nThisNode*2]);
				nThisNode = nThisNode*2;
			}
			else
			{
				if(!COMPARE(m_pItems[nThisNode*2+1], m_pItems[nThisNode]))
					break;
				HeapSwap(nThisNode, nThisNode*2+1);
				//TEMPLATE_SWAP(m_pItems[nThisNode], m_pItems[nThisNode*2+1]);
				nThisNode = nThisNode*2+1;
			}
		}
		if(m_nItemCount == 2*nThisNode+1 && COMPARE(m_pItems[nThisNode*2], m_pItems[nThisNode]))
		{
			HeapSwap(nThisNode*2, nThisNode);
			//TEMPLATE_SWAP(m_pItems[nThisNode*2], m_pItems[nThisNode]);
		}

	}

public:
	int m_nItemCount;
	T * m_pItems[SIZE];


	Heap()
	:m_nItemCount(1)
	{}
	
	void Init()
	{
		memclear(m_pItems, sizeof(m_pItems));
		m_nItemCount = 1;
	}
	void Free() {} //I could free all the nodes, but I didn't alloc them in the first place.

	//If you really want to, here's a function to do it.  Note that it doesn't free things
	//that have been popped off the heap already.
	void FreeNodes(MEMORYPOOL * pPool)
	{
		for(int i = 1; i < m_nItemCount; i++) if(m_pItems[i]) FREE(pPool, m_pItems[i]);
		m_nItemCount = 1;
	}

	BOOL AddNode(T* pItem)
	{
		ASSERT_RETFALSE(m_nItemCount < SIZE);
		int nThisNode = m_nItemCount;

		m_pItems[m_nItemCount] = pItem;

		BubbleUp(nThisNode);

		m_nItemCount++;
		return TRUE;
	}

	T* GetMin() //returns lowest node as defined by COMPARE
	{
		return m_pItems[1];
	}

	T* PopMinNode() //returns lowest node as defined by COMPARE and removes top.
	{
		if(m_nItemCount <= 1) return NULL;

		T* toRet = m_pItems[1];
		m_nItemCount--;
		
		//m_pItems[1] = m_pItems[m_nItemCount]; //replacing this with below 2 lines so heapswap is called.
		m_pItems[1] = NULL;
		HeapSwap(1, m_nItemCount);

		//Until we get to the bottom or are stable, TEMPLATE_SWAP the node with its smaller child.
		int nThisNode = 1;
		BubbleDown(nThisNode);
		return toRet;

	}
	int GetItemCount()
	{
		return m_nItemCount - 1; //zeroeth item is empty
	}
	//void AddNodeList(HeapNode *, int nodecount); //Might be faster to multiple add nodes, but not by much.
		//Likely doesn't garner enough speed increase to bother with, so I won't implement it.
		//After all, we still have to get the top element repeatedly.
};

//----------------------------------------------------------------------------
// HeapRestacking
// A derived class of Heap.  You can change the value of a given item and
// call Update(), at which point it rebubbles to the correct place in the
// heap.  Since you would have to know your current heap index to do this,
// this heap also provides a function pointer which is called whenever we
// move an entry, to keep you updated as to the position.
//
// Not threadsafe.
//----------------------------------------------------------------------------
//template<class T>
//typedef void (*fpHandleHeapMove)(void *, int, T*);//context, index, pointer

template<class T, unsigned int SIZE, BOOL COMPARE(T * a, T * b )>
struct HeapRestacking : public Heap<T, SIZE, COMPARE>
{
public:
	void (*fpOnHeapMove)(void *, int, T*);
	void *pContext;

	HeapRestacking()
		:pContext(NULL), fpOnHeapMove(NULL), Heap()
	{}
	HeapRestacking(void *context, void (*fp)(void *, int, T*))
		:pContext(context), fpOnHeapMove(fp), Heap()
	{}

	void Init()
	{
		pContext = NULL;
		fpOnHeapMove = NULL;
		Heap::Init();
	}

	void Update(int nIndex)
	{//Compare node with parent and children, either bubble up or trickle down as needed.
		ASSERT_RETURN(nIndex != 0); //Zeroeth index is empty.
		ASSERT_RETURN(nIndex < SIZE);

		if(nIndex > 1 && COMPARE(m_pItems[nIndex], m_pItems[nIndex/2]))
		{//If the previous item is greater, bubble up.
			HeapSwap(nIndex, nIndex/2);
			nIndex /= 2;
			BubbleUp(nIndex);
		}
		else
		{//See if we need to bubble down
			BubbleDown(nIndex);
		}
	}

	T* PopMinNode()
	{
		T* node = Heap::PopMinNode();
		if(fpOnHeapMove) fpOnHeapMove(pContext, INVALID_HEAP_POSITION, node);
		return node;
	}

	BOOL AddNode(T* pItem)
	{
		if(fpOnHeapMove) fpOnHeapMove(pContext, m_nItemCount, pItem);
		return Heap::AddNode(pItem);
	}
protected:
	virtual void HeapSwap(int nFirstIndex, int nSecondIndex)
	{
		TEMPLATE_SWAP(m_pItems[nFirstIndex], m_pItems[nSecondIndex]);
		if(fpOnHeapMove)
		{
			fpOnHeapMove(pContext, nFirstIndex , m_pItems[nFirstIndex ]);
			fpOnHeapMove(pContext, nSecondIndex, m_pItems[nSecondIndex]);
		}
	}
};

//----------------------------------------------------------------------------
// DEBUG FUNCTIONS
//----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)

template<class T, unsigned int SIZE, BOOL COMPARE(T * a, T * b )>
inline void TraceHeapRestacking(const HeapRestacking<T, SIZE, COMPARE> &heap, void (*fpTraceNode)(T*))
{
	int row = 1;
	while(row < SIZE && row<heap.m_nItemCount)
	{
		for(int i = row; i < row*2 && i < heap.m_nItemCount; i++)
		{
			fpTraceNode(heap.m_pItems[i]);
			if(i*2 < heap.m_nItemCount) if(COMPARE(heap.m_pItems[i*2], heap.m_pItems[i])) DebugBreak();
		}
		fpTraceNode(NULL); //better null check and trace a endl!
		row*= 2;
	}
}

#endif //DEBUG_VERSION


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define HeapInit(heap)						(heap).Init()
#define HeapFree(heap)						(heap).Free()
#define HeapAdd(heap, id)					(heap).AddNode(id)


#endif //_HEAP_H_