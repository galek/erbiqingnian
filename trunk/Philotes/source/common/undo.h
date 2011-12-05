//----------------------------------------------------------------------------
// FILE: undo.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNDO_H_
#define _UNDO_H_

#define MAX_NODES 100

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
class UndoEngine
{
public:
	//----------------------------------------------------------------------------
	UndoEngine()
	{
		pCopyCallback = NULL;
		pFreeCallback = NULL;
		pHead = NULL;
		nCreatingUndoPointLock = 0;
	}

	//----------------------------------------------------------------------------
	~UndoEngine()
	{
		Reset();
	}

	//----------------------------------------------------------------------------
	typedef void (*CopyCallback)(T* pFrom, T* pTo);
	typedef void (*FreeCallback)(T* pItem);

	//----------------------------------------------------------------------------
	void Init(CopyCallback pCopy, FreeCallback pFree)
	{
		pCopyCallback = pCopy;
		pFreeCallback = pFree;
	}

	//----------------------------------------------------------------------------
	void Reset()
	{
		if(pHead)
		{
			pHead->pCurrent = NULL;
		}
		RemoveUndoNode();
	}

	//----------------------------------------------------------------------------
	void Undo()
	{
		if(pHead)
		{
			if(pHead->pCurrent)
			{
				pHead->pCurrent = pHead->pCurrent->pPrev;
			}
		}
	}

	//----------------------------------------------------------------------------
	void Redo()
	{
		if(pHead)
		{
			if(pHead->pCurrent)
			{
				if(pHead->pCurrent->pNext)
				{
					pHead->pCurrent = pHead->pCurrent->pNext;
				}
			}
			else
			{
				pHead->pCurrent = pHead;
			}
		}
	}

	//----------------------------------------------------------------------------
	void StartCreateUndoPoint()
	{
		nCreatingUndoPointLock++;
	}

	//----------------------------------------------------------------------------
	void EndCreateUndoPoint()
	{
		if(nCreatingUndoPointLock > 0)
		{
			nCreatingUndoPointLock--;
		}
	}

	//----------------------------------------------------------------------------
	void CreateUndoPoint(T * pItem)
	{
		if(nCreatingUndoPointLock)
		{
			return;
		}

		// Add a new node and increment the current pointer
		AddUndoNode(pItem);
		Redo();
	}

	//----------------------------------------------------------------------------
	T * GetCurrent()
	{
		if(pHead->pCurrent)
		{
			return &pHead->pCurrent->tItem;
		}
		return NULL;
	}

private:
	//----------------------------------------------------------------------------
	struct UNDO_NODE
	{
		// All undo nodes use these
		UNDO_NODE * pNext;
		UNDO_NODE * pPrev;

		// Only the head uses these
		UNDO_NODE * pHead;
		UNDO_NODE * pCurrent;

		// Each undo node stores an entire copy of the item
		T tItem;
	};

//----------------------------------------------------------------------------
// We've gotta cull the old nodes at some point..we're getting crashes since a new node is
//	created everytime the mouse wheel ticks to adjust the spinners.
//----------------------------------------------------------------------------
	void CullOldest()
	{
		int nCount = CountOlderNodes();
		if(nCount > MAX_NODES)
		{
				
			UNDO_NODE * pCurr = pHead;
			pHead = pHead->pNext;
			if(pFreeCallback)
			{
				pFreeCallback(&pCurr->tItem);
			}
			if(pCurr->pNext)
			{
				pCurr->pNext->pPrev = NULL;
				pHead = pCurr->pNext;
			}

			FREE(NULL,pCurr);		



		}


	}

	//----------------------------------------------------------------------------
	// Remove all nodes after the current one
	//----------------------------------------------------------------------------
	void RemoveUndoNode()
	{
		if(!pHead)
		{
			return;
		}

		UNDO_NODE * pCurrent = pHead->pCurrent;
		if(pCurrent)
		{
			pCurrent = pCurrent->pNext;
			if(!pCurrent)
			{
				return;
			}
		}
		else
		{
			// Remove everything, including the head
			pCurrent = pHead;
			pHead = NULL;
		}

		if(pCurrent->pPrev)
		{
			pCurrent->pPrev->pNext = NULL;
		}

		while(pCurrent)
		{
			UNDO_NODE * pNext = pCurrent->pNext;
			if(pFreeCallback)
			{
				pFreeCallback(&pCurrent->tItem);
			}
			FREE(NULL, pCurrent);
			pCurrent = pNext;
		}
	}

	//----------------------------------------------------------------------------
	int CountOlderNodes()
	{
		int nCount = 0;

		UNDO_NODE *pCurr = pHead;
		while(pCurr != pHead->pCurrent)
		{
			nCount++;
			pCurr = pCurr->pNext;
		}

		return nCount;

	}

	//----------------------------------------------------------------------------
	void AddUndoNode(T * pFrom)
	{
		// Delete everything after the current node first
		RemoveUndoNode();

		UNDO_NODE * pInsertionPoint = pHead ? pHead->pCurrent : NULL;
		UNDO_NODE * pNewNode = (UNDO_NODE *)MALLOCZ(NULL, sizeof(UNDO_NODE));

		pNewNode->pNext = NULL;
		pNewNode->pPrev = pInsertionPoint;
		pNewNode->pHead = pHead ? pHead : pNewNode;
		if(pInsertionPoint)
		{
			pInsertionPoint->pNext = pNewNode;
		}

		if(pCopyCallback)
		{
			pCopyCallback(pFrom, &pNewNode->tItem);
		}
		else
		{
			MemoryCopy(&pNewNode->tItem, sizeof(T), pFrom, sizeof(T));
		}

		if(!pHead)
		{
			pHead = pNewNode;
		}

		CullOldest();
	}

	//----------------------------------------------------------------------------
	
	CopyCallback pCopyCallback;
	FreeCallback pFreeCallback;
	UNDO_NODE * pHead;
	int nCreatingUndoPointLock;

};

#endif
