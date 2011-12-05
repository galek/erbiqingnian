//----------------------------------------------------------------------------
// accountbadges.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "critsect.h"

namespace FSCommon
{
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define QUEUE_DEBUG						0


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
typedef int QUEUE_COMPARE(void * A, void * B);


// ---------------------------------------------------------------------------
// Enum QUEUE_BEHAVIOR
// ---------------------------------------------------------------------------
enum QUEUE_BEHAVIOR
{
	QB_FIFO,
	QB_LIFO
};


// ---------------------------------------------------------------------------
// critical sectioned queue
// ---------------------------------------------------------------------------
template <QUEUE_BEHAVIOR B, typename T>
class CQueue
{
	public:
	
		CCritSectLite			m_CS;							// critical section
		unsigned int			m_nCount;						// number of items in queue
		T *						m_pHead;						// first item
		T *						m_pTail;						// last item

	public:
	
	// validate integrity of list
#if QUEUE_DEBUG
	void Validate(
		const char * file,
		unsigned int line)
	{
		if (!m_pHead)
		{
			FL_ASSERT(!m_pTail, file, line);
			FL_ASSERT(m_nCount == 0, file, line);
			return;
		}
		FL_ASSERT(m_pTail, file, line);
		T * prev = NULL;
		T * curr = m_pHead;
		unsigned int count = 0;
		while (curr)
		{
			++count;
			prev = curr;
			curr = curr->m_pNext;
		}
		FL_ASSERT(count == m_nCount, file, line);
		FL_ASSERT(prev == m_pTail, file, line);
	}
#define QUEUE_VALIDATE()		Validate(__FILE__, __LINE__)
#else
#define QUEUE_VALIDATE()
#endif

	// get the item at the front of the list
	T *	Get(
		void)
	{
		T * node;
		
		{
			CSLAutoLock autoLock(&m_CS);
			QUEUE_VALIDATE();

			node = m_pHead;
			if (node)
			{
				m_pHead = m_pHead->m_pNext;
				if (!m_pHead)
				{
					m_pTail = NULL;
				}
				node->m_pNext = NULL;
				--m_nCount;
			}

			QUEUE_VALIDATE();
		}

		return node;
	}

	// put an item at the end of the list
	void Put(
		T * node)
	{
		if (!node)
		{
			return;
		}
		node->m_pNext = NULL;

		{
			CSLAutoLock autoLock(&m_CS);
			QUEUE_VALIDATE();

			if (B == QB_FIFO)
			{
				if (m_pTail)
				{
					m_pTail->m_pNext = node;
				}
				else
				{
					m_pHead = node;
				}
				m_pTail = node;
			}
			else
			{			
				node->m_pNext = m_pHead;
				m_pHead = node;

				if(m_pTail == NULL)
				{
					m_pTail = node;
				}
			}						
			
			m_nCount++;

			QUEUE_VALIDATE();
		}		
	}

	// put in order -- assumes list is already ordered
	void PutInOrder(
		T * node,
		QUEUE_COMPARE * fpCompare)
	{
		ASSERT_RETURN(fpCompare);
		if (!node)
		{
			return;
		}
		node->m_pNext = NULL;

		{
			CSLAutoLock autoLock(&m_CS);
			QUEUE_VALIDATE();

			T * prev = NULL;
			T * curr = m_pHead;
			while (curr)
			{
				int cmp = fpCompare(node, curr);
				if (cmp <= 0)
				{
					break;
				}
				prev = curr;
				curr = curr->m_pNext;
			}
			if (prev)
			{
				prev->m_pNext = node;
			}
			else
			{
				m_pHead = node;
			}
			if (curr)
			{
				node->m_pNext = curr;
			}
			else
			{
				m_pTail = node;
			}
			
			m_nCount++;

			QUEUE_VALIDATE();
		}		
	}

	// get all of the items
	T * GetAll(
		T ** tail = NULL)
	{
		T * head;

		{
			CSLAutoLock autoLock(&m_CS);
			QUEUE_VALIDATE();

			head = m_pHead;
			if (tail)
			{
				*tail = m_pTail;
			}
			m_pHead = m_pTail = NULL;
			m_nCount = 0;
		}

		return head;
	}

	// put an entire list (assumes count is valid)
	void PutList(
		T * head,
		T * tail,
		unsigned int count)
	{
		ASSERT_RETURN(BOTH(head, tail));
		if (!head)
		{
			return;
		}

		{
			CSLAutoLock autoLock(&m_CS);
			QUEUE_VALIDATE();

			if (m_pTail)
			{
				m_pTail->m_pNext = head;
				m_pTail = tail;
			}
			else
			{
				m_pHead = head;
				m_pTail = tail;
			}
			m_nCount += count;

			QUEUE_VALIDATE();
		}
	}

	// get the count
	unsigned int GetCount(
		void)
	{
		unsigned int count;

		{
			CSLAutoLock autoLock(&m_CS); // note, this isn't really necessary if we size m_nCount to processor
			count = m_nCount;
		}

		return count;
	}
};

} // end namespace common