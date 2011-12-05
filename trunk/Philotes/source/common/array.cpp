//-----------------------------------------------------------------------------
// array.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// INCLUDE
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------
#define DEFAULT_ENTRIES_IN_LIST 4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CArrayList::CArrayList(
	ArrayListType eType,
	UINT nBytesPerEntry) :
	m_eArrayListType(eType),
	m_pData(NULL),
	m_nBytesPerEntry(nBytesPerEntry),
	m_nEntriesCount(0),
	m_nEntriesAllocated(0)
{
    if (eType == AL_REFERENCE)
	{
        m_nBytesPerEntry = sizeof(void*);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CArrayList::Init(
	ArrayListType eType,
	UINT nBytesPerEntry)
{
	m_eArrayListType = eType;
	m_pData = NULL;
	m_nBytesPerEntry = nBytesPerEntry;
	m_nEntriesCount = 0;
	m_nEntriesAllocated = 0;
	if (eType == AL_REFERENCE)
	{
		m_nBytesPerEntry = sizeof(void*);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CArrayList::~CArrayList(
	void)
{
    if (m_pData != NULL)
	{
        FREE(NULL, m_pData);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PRESULT CArrayList::Add(
	void * pEntry
#if DEBUG_MEMORY_ALLOCATIONS
	, const char * file,
	unsigned int line
#endif
	)
{

	if (m_nBytesPerEntry == 0)
	{      
		return E_FAIL;
	}
    if (m_pData == NULL || m_nEntriesCount + 1 > m_nEntriesAllocated)
    {
        if (m_nEntriesAllocated == 0)
		{
			m_nEntriesAllocated = DEFAULT_ENTRIES_IN_LIST;
			m_pData = MALLOCFL(NULL, m_nBytesPerEntry * m_nEntriesAllocated, file, line);
		}
        else
		{
			m_nEntriesAllocated *= 2;
			m_pData = REALLOCFL(NULL, m_pData, m_nBytesPerEntry * m_nEntriesAllocated, file, line);
		}
    }

    if (m_eArrayListType == AL_VALUE)
	{
        memcpy((BYTE*)m_pData + (m_nEntriesCount * m_nBytesPerEntry), pEntry, m_nBytesPerEntry);
	}
    else
	{
        *(((void**)m_pData) + m_nEntriesCount) = pEntry;
	}
    m_nEntriesCount++;
    return S_OK;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CArrayList::Remove(
	UINT nEntry)
{
	if (m_pData == NULL)
	{
		return;
	}

    m_nEntriesCount--;

    BYTE* pData = (BYTE*)m_pData + (nEntry * m_nBytesPerEntry);

    ASSERT(MemoryMove(pData, (m_nEntriesAllocated - nEntry) * m_nBytesPerEntry, pData + m_nBytesPerEntry, (m_nEntriesCount - nEntry) * m_nBytesPerEntry));
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void* CArrayList::GetPtr(
	UINT nEntry)
{
	if (m_pData == NULL)
	{
		return NULL;
	}

    if (m_eArrayListType == AL_VALUE)
	{
        return (BYTE*)m_pData + (nEntry * m_nBytesPerEntry);
	}
    else
	{
        return *(((void**)m_pData) + nEntry);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const void* CArrayList::GetPtr(
	UINT nEntry) const
{
	if (m_pData == NULL)
	{
		return NULL;
	}

    if (m_eArrayListType == AL_VALUE)
	{
        return (BYTE*)m_pData + (nEntry * m_nBytesPerEntry);
	}
    else
	{
        return *(((void**)m_pData) + nEntry);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL CArrayList::Contains(
	void* pEntryData)
{
	// if data is sorted, we can optimize!!!
    if (m_eArrayListType == AL_VALUE)
    {
		for (UINT ii = 0; ii < m_nEntriesCount; ii++)
		{
            if (memcmp(GetPtr(ii), pEntryData, m_nBytesPerEntry) == 0)
			{
                return TRUE;
			}
		}
	}
	else
	{
		for (UINT ii = 0; ii < m_nEntriesCount; ii++)
		{
            if (GetPtr(ii) == pEntryData)
			{
                return TRUE;
			}
		}
	}
    return FALSE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL CArrayList::Sort (
	int (__cdecl *fnCompare)(const void *, const void *))
{
	if (m_pData == NULL)
	{
		return TRUE;
	}

	if (m_eArrayListType == AL_VALUE)
	{
		qsort(m_pData, m_nEntriesCount, m_nBytesPerEntry, fnCompare);
		return TRUE;
	}

	ASSERT(0);
	return FALSE; // didn't solve this one...
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CArrayList::Destroy(
	void)
{
	if (m_pData)
	{
		FREE(NULL, m_pData);
	}
	m_pData = NULL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LRUList::Init(
	void)
{
	m_LRU_HeadTail.m_LRU_Next = m_LRU_HeadTail.m_LRU_Prev = &m_LRU_HeadTail;
	m_LRU_HeadTail.m_LRU_Tick = 0;
	m_LRU_HeadTail.m_LRU_Cost = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LRUList::Add(
	LRUNode * item,
	DWORD cost)
{
	ASSERT_RETURN(item && item->m_LRU_Next == NULL && item->m_LRU_Prev == NULL && item->m_LRU_Tick == 0);

	item->m_LRU_Cost = cost;
	item->m_LRU_Tick = m_LRU_HeadTail.m_LRU_Tick;
	
	item->m_LRU_Prev = &m_LRU_HeadTail;
	item->m_LRU_Next = m_LRU_HeadTail.m_LRU_Next;
	item->m_LRU_Prev->m_LRU_Next = item;
	item->m_LRU_Next->m_LRU_Prev = item;

	m_LRU_HeadTail.m_LRU_Cost += cost;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LRUList::Remove(
	LRUNode * item)
{
	ASSERT_RETURN(item && item->m_LRU_Next != NULL && item->m_LRU_Prev != NULL);

	item->m_LRU_Prev->m_LRU_Next = item->m_LRU_Next;
	item->m_LRU_Next->m_LRU_Prev = item->m_LRU_Prev;
	item->m_LRU_Prev = item->m_LRU_Next = NULL;

	m_LRU_HeadTail.m_LRU_Cost -= item->m_LRU_Cost;
	item->m_LRU_Cost = 0;
	item->m_LRU_Tick = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LRUList::Touch(
	LRUNode * item)
{
	item->m_LRU_Tick = m_LRU_HeadTail.m_LRU_Tick;
	// unlink
	item->m_LRU_Prev->m_LRU_Next = item->m_LRU_Next;
	item->m_LRU_Next->m_LRU_Prev = item->m_LRU_Prev;
	// link
	item->m_LRU_Prev = &m_LRU_HeadTail;
	item->m_LRU_Next = m_LRU_HeadTail.m_LRU_Next;
	item->m_LRU_Prev->m_LRU_Next = item;
	item->m_LRU_Next->m_LRU_Prev = item;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LRUList::Update(
	DWORD tick)
{
	m_LRU_HeadTail.m_LRU_Tick = tick;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LRUNode* LRUList::GetHeadTail()
{
	return &m_LRU_HeadTail;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LRUNode* LRUList::GetFirstNode()
{
	return m_LRU_HeadTail.m_LRU_Next;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LRUList::GetTotal(
	void)
{
	return m_LRU_HeadTail.m_LRU_Cost;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void sTestLoops(
	CArrayIndexed<float>& tTestArray)
{
	// make sure that there isn't an infinite loop
	int nId = tTestArray.GetFirst();
	if (nId == INVALID_ID)
	{
		return;
	}

	int ii;
	for (ii = 0; ii < 1000; ii++)
	{
		nId = tTestArray.GetNextId(nId);
		if (nId != INVALID_ID)
		{
			float* pfCurrent = tTestArray.Get(nId);
			ASSERT_RETURN(pfCurrent != NULL);
			ASSERT(*pfCurrent == 1.0f + 100.0f * nId);
		}
		else
		{
			break;
		}
	}
	ASSERT(ii < 1000);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void testArrayIndexed()
{
	enum
	{
		TEST_LENGTH = 10
	};

	CArrayIndexed<float> tTestArray;
	ArrayInit(tTestArray, NULL, TEST_LENGTH);
	int nNewIndex = 12;

	// add a node, and then remove it.  repeat
	float* pfNew = tTestArray.Add(&nNewIndex);
	*pfNew = 1.0f + 100.0f * nNewIndex;
	tTestArray.Remove(nNewIndex);
	sTestLoops(tTestArray);

	pfNew = tTestArray.Add(&nNewIndex);
	*pfNew = 1.0f + 100.0f * nNewIndex;
	tTestArray.Remove(nNewIndex);
	sTestLoops(tTestArray);

	tTestArray.Clear();

	// fill an array of values
	for (int ii = 0; ii < TEST_LENGTH; ii++)
	{
		pfNew = tTestArray.Add(&nNewIndex);
		ASSERT(nNewIndex == ii);
		*pfNew = 1.0f + 100.0f * ii;
		sTestLoops(tTestArray);
	}

	// add an extra one, and make sure it comes back null
	pfNew = tTestArray.Add(&nNewIndex);
	ASSERT(pfNew == NULL);

	// verify that the values are still there
	for (int ii = 0; ii < TEST_LENGTH; ii++)
	{
		pfNew = tTestArray.Get(ii);
		ASSERT(*pfNew == 1.0f + 100.0f * ii);
	}

	// now remove a few of them
	tTestArray.Remove(1);
	tTestArray.Remove(2);
	tTestArray.Remove(0);

	// now add some new ones
	pfNew = tTestArray.Add(&nNewIndex);
	*pfNew = 1.0f + 100.0f * nNewIndex;
	pfNew = tTestArray.Add(&nNewIndex);
	*pfNew = 1.0f + 100.0f * nNewIndex;
	pfNew = tTestArray.Add(&nNewIndex);
	*pfNew = 1.0f + 100.0f * nNewIndex;
	
	// add an extra one, and make sure it comes back null
	pfNew = tTestArray.Add(&nNewIndex);
	ASSERT(pfNew == NULL);

	// loop forward through the current nodes
	int nId = tTestArray.GetFirst();
	ASSERT(tTestArray.Get(nId));
	for (int ii = 0; ii < TEST_LENGTH - 1; ii++)
	{
		nId = tTestArray.GetNextId(nId);
		float * pfCurrent = tTestArray.Get(nId);
		ASSERT(pfCurrent != NULL);
		ASSERT(*pfCurrent == 1.0f + 100.0f * nId);
	}

	// loop backward through the current nodes
	ASSERT(tTestArray.Get(nId));
	for (int ii = 0; ii < TEST_LENGTH - 1; ii++)
	{
		nId = tTestArray.GetPrevId(nId);
		float * pfCurrent = tTestArray.Get(nId);
		ASSERT(pfCurrent != NULL);
		ASSERT(*pfCurrent == 1.0f + 100.0f * nId);
	}

	tTestArray.Clear();
	sTestLoops(tTestArray);
}
