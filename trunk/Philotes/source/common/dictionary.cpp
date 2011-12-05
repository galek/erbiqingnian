//----------------------------------------------------------------------------
// dictionary.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "dictionary.h"


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// dictionary with int key
//----------------------------------------------------------------------------
struct INT_DICT_NODE
{
	int						m_nKey;
	void *					m_pData;
};


struct INT_DICTIONARY
{
	INT_DICT_NODE *			m_pNodes;
	int						m_nCount;		// number of elements in m_nCount
	int						m_nSize;		// size of m_pNodes buffer

	int						m_nBufferSize;	// amount to increment m_nSize by

	BOOL					m_bSorted;		// is the data sorted?

	FP_INT_DICT_DELETE *	m_fpDelete;		// call when deleting node
};


//----------------------------------------------------------------------------
// dictionary with char* key
//----------------------------------------------------------------------------
struct STR_DICT_NODE
{
	char *					m_pszKey;
	void *					m_pData;
};


struct STR_DICTIONARY
{
	STR_DICT_NODE*			m_pNodes;
	int						m_nCount;		// number of elements in m_nCount
	int						m_nSize;		// size of m_pNodes buffer

	int						m_nBufferSize;	// amount to increment m_nSize by
	BOOL					m_bSorted;		// is the data sorted?
	BOOL					m_bCopyKey;		// make copy of key on insertion

	FP_STR_DICT_DELETE *	m_fpDelete;		// call when deleting node
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INT_DICTIONARY * IntDictionaryInit(
	int nBufferSize,
	FP_INT_DICT_DELETE * fpDelete)
{
	INT_DICTIONARY * dictionary = (INT_DICTIONARY *)MALLOCZ(NULL, sizeof(INT_DICTIONARY));

	ASSERT(nBufferSize > 0);
	dictionary->m_nBufferSize = MAX(nBufferSize, 1);
	dictionary->m_bSorted = TRUE;
	dictionary->m_fpDelete = fpDelete;

	return dictionary;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IntDictionaryFree(
	INT_DICTIONARY * dictionary)
{
	ASSERT_RETURN(dictionary);

	if (dictionary->m_fpDelete)
	{
		for (int ii = 0; ii < dictionary->m_nCount; ii++)
		{
			dictionary->m_fpDelete(dictionary->m_pNodes[ii].m_pData);
		}
	}
	if (dictionary->m_pNodes)
	{
		FREE(NULL, dictionary->m_pNodes);
	}
	FREE(NULL, dictionary);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int IntDictionaryCompare(
	const void * a,
	const void * b)
{
	INT_DICT_NODE * A = (INT_DICT_NODE *)a;
	INT_DICT_NODE * B = (INT_DICT_NODE *)b;

	if (A->m_nKey < B->m_nKey)
	{
		return -1;
	}
	else if (A->m_nKey > B->m_nKey)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IntDictionarySort(
	INT_DICTIONARY * dictionary)
{
	ASSERT_RETURN(dictionary);

	qsort(dictionary->m_pNodes, dictionary->m_nCount, sizeof(INT_DICT_NODE), IntDictionaryCompare);

	dictionary->m_bSorted = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IntDictionaryAdd(
	INT_DICTIONARY * dictionary,
	int key,
	void * data)
{
	ASSERT_RETURN(dictionary);

	if (dictionary->m_nCount >= dictionary->m_nSize)
	{
		dictionary->m_nSize += dictionary->m_nBufferSize;
		dictionary->m_pNodes = (INT_DICT_NODE *)REALLOC(NULL, dictionary->m_pNodes, dictionary->m_nSize * sizeof(INT_DICT_NODE));
	}
	dictionary->m_pNodes[dictionary->m_nCount].m_nKey = key;
	dictionary->m_pNodes[dictionary->m_nCount].m_pData = data;
	dictionary->m_nCount++;
	dictionary->m_bSorted = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int IntDictionaryGetCount(
	INT_DICTIONARY * dictionary)
{
	return dictionary->m_nCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * IntDictionaryGet(
	INT_DICTIONARY * dictionary,
	int index)
{
	ASSERT_RETNULL(dictionary);
	ASSERT_RETNULL(index >= 0 && index < dictionary->m_nCount);
	return dictionary->m_pNodes[index].m_pData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * IntDictionaryFind(
	INT_DICTIONARY * dictionary,
	int key)
{
	ASSERT_RETNULL(dictionary);

	if (!dictionary->m_bSorted)
	{
		IntDictionarySort(dictionary);
	}

	INT_DICT_NODE keynode;
	keynode.m_nKey = key;
	INT_DICT_NODE * node = (INT_DICT_NODE *)bsearch(&keynode, dictionary->m_pNodes, dictionary->m_nCount, sizeof(INT_DICT_NODE), IntDictionaryCompare);
	if (!node)
	{
		return NULL;
	}
	return node->m_pData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STR_DICTIONARY* StrDictionaryInit(
	int nBufferSize,
	BOOL bCopyKey,
	FP_STR_DICT_DELETE * fpDelete)
{
	STR_DICTIONARY * dictionary = (STR_DICTIONARY *)MALLOCZ(NULL, sizeof(STR_DICTIONARY));

	if (dictionary != 0)
	{
		ASSERT(nBufferSize > 0);
		dictionary->m_nBufferSize = MAX(nBufferSize, 1);
		dictionary->m_bSorted = TRUE;
		dictionary->m_bCopyKey = bCopyKey;
		dictionary->m_fpDelete = fpDelete;
	}

	return dictionary;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STR_DICTIONARY * StrDictionaryInit(
	const STR_DICT * init_array,
	BOOL bCopyKey,
	int offset)
{
	ASSERT_RETNULL(init_array);

	const STR_DICT * cur = init_array;
	while (cur->str)
	{
		cur++;
	}
	int count = (int)(cur - init_array);

	STR_DICTIONARY * dictionary = StrDictionaryInit(count, bCopyKey);
	cur = init_array;
	while (cur->str)
	{
		StrDictionaryAdd(dictionary, cur->str, CAST_TO_VOIDPTR(cur->value + offset));
		cur++;
	}
	return dictionary;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STR_DICTIONARY * StrDictionaryInit(
	const STR_DICT * init_array,
	unsigned int max_size,
	BOOL bCopyKey,
	int offset)
{
	ASSERT_RETNULL(init_array);
	ASSERT_RETNULL(max_size > 0);

	const STR_DICT * cur = init_array;
	unsigned int count = 0;
	while (count < max_size && cur->str)
	{
		++cur;
		++count;
	}

	STR_DICTIONARY * dictionary = StrDictionaryInit(count, bCopyKey);
	cur = init_array;

	unsigned int secondcount = 0;
	while (cur->str && secondcount < count)
	{
		StrDictionaryAdd(dictionary, cur->str, CAST_TO_VOIDPTR(cur->value + offset));
		cur++;
		secondcount++;
	}
	return dictionary;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StrDictionaryFree(
	STR_DICTIONARY * dictionary)
{
	ASSERT_RETURN(dictionary);

	if (dictionary->m_fpDelete)
	{
		for (int ii = 0; ii < dictionary->m_nCount; ii++)
		{
			dictionary->m_fpDelete(dictionary->m_pNodes[ii].m_pszKey, dictionary->m_pNodes[ii].m_pData);
		}
	}
	if (dictionary->m_bCopyKey)
	{
		for (int ii = 0; ii < dictionary->m_nCount; ii++)
		{
			FREE(NULL, dictionary->m_pNodes[ii].m_pszKey);
		}
	}
	if (dictionary->m_pNodes)
	{
		FREE(NULL, dictionary->m_pNodes);
	}
	FREE(NULL, dictionary);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int StrDictionaryCompare(
	const void * a,
	const void * b)
{
	STR_DICT_NODE * A = (STR_DICT_NODE *)a;
	STR_DICT_NODE * B = (STR_DICT_NODE *)b;

	return PStrCmp(A->m_pszKey, B->m_pszKey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StrDictionarySort(
	STR_DICTIONARY * dictionary)
{
	ASSERT_RETURN(dictionary);

	qsort(dictionary->m_pNodes, dictionary->m_nCount, sizeof(STR_DICT_NODE), StrDictionaryCompare);

	dictionary->m_bSorted = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StrDictionaryAdd(
	STR_DICTIONARY * dictionary,
	const char * key,
	void * data)
{
	ASSERT_RETURN(dictionary);

	if (dictionary->m_nCount >= dictionary->m_nSize)
	{
		dictionary->m_nSize += dictionary->m_nBufferSize;
		dictionary->m_pNodes = (STR_DICT_NODE *)REALLOC(NULL, dictionary->m_pNodes, dictionary->m_nSize * sizeof(STR_DICT_NODE));
	}
	if (dictionary->m_bCopyKey)
	{
		int len = PStrLen(key) + 1;
		dictionary->m_pNodes[dictionary->m_nCount].m_pszKey = (char *)MALLOC(NULL, len);
		PStrCopy(dictionary->m_pNodes[dictionary->m_nCount].m_pszKey, key, len);
	}
	else
	{
		dictionary->m_pNodes[dictionary->m_nCount].m_pszKey = (char *)key;
	}
	dictionary->m_pNodes[dictionary->m_nCount].m_pData = data;
	dictionary->m_nCount++;
	dictionary->m_bSorted = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StrDictionaryGetCount(
	const STR_DICTIONARY * dictionary)
{
	return dictionary->m_nCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * StrDictionaryGet(
	const STR_DICTIONARY * dictionary,
	int index)
{
	ASSERT_RETNULL(dictionary);
	ASSERT_RETNULL(index >= 0 && index < dictionary->m_nCount);
	return dictionary->m_pNodes[index].m_pData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * StrDictionaryGetStr(
	const STR_DICTIONARY * dictionary,
	int index)
{
	ASSERT_RETNULL(dictionary);
	ASSERT_RETNULL(index >= 0 && index < dictionary->m_nCount);
	return dictionary->m_pNodes[index].m_pszKey;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * StrDictionaryFind(
	STR_DICTIONARY * dictionary,
	const char * key,
	BOOL * found)
{
	if (!dictionary)
	{
		if (found)
		{
			*found = FALSE;
		}
		return NULL;
	}
	if (key == NULL)
	{
		if (found)
		{
			*found = FALSE;
		}
		return NULL;
	}

	if (!dictionary->m_bSorted)
	{
		StrDictionarySort(dictionary);
	}

	STR_DICT_NODE keynode;
	keynode.m_pszKey = (char *)key;
	STR_DICT_NODE * node = (STR_DICT_NODE *)bsearch(&keynode, dictionary->m_pNodes, dictionary->m_nCount, sizeof(STR_DICT_NODE), StrDictionaryCompare);
	if (!node)
	{
		if (found)
		{
			*found = FALSE;
		}
		return NULL;
	}
	if (found)
	{
		*found = TRUE;
	}
	return node->m_pData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StrDictionaryFindIndex(
	STR_DICTIONARY * dictionary,
	const char * key)
{
	if (!dictionary)
	{
		return INVALID_ID;
	}

	if (!dictionary->m_bSorted)
	{
		StrDictionarySort(dictionary);
	}

	STR_DICT_NODE keynode;
	keynode.m_pszKey = (char *)key;
	STR_DICT_NODE * node = (STR_DICT_NODE *)bsearch(&keynode, dictionary->m_pNodes, dictionary->m_nCount, sizeof(STR_DICT_NODE), StrDictionaryCompare);
	if (!node)
	{
		return INVALID_ID;
	}
	return (int)(node - dictionary->m_pNodes);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * StrDictionaryFindNameByValue(
	const STR_DICTIONARY * dictionary,
	void * data)
{
	if (!dictionary)
	{
		return NULL;
	}

	for (int ii = 0; ii < dictionary->m_nCount; ii++)
	{
		if (dictionary->m_pNodes[ii].m_pData == data)
		{
			return dictionary->m_pNodes[ii].m_pszKey;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * StrDictGet(
	STR_DICT * dictionary,
	int index)
{
	ASSERTX_RETNULL( dictionary, "Expected dictionary" );
	
	for (int i = 0; dictionary[ i ].str; ++i)
	{
		const STR_DICT *pDict = &dictionary[ i ];
		if (pDict->value == index)
		{
			return pDict->str;
		}
	}
	return NULL;	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StrDictFind(
	STR_DICT * dictionary,
	const char * key,
	BOOL bIgnoreCase /*= TRUE*/)
{
	ASSERTX_RETNULL( dictionary, "Expected dictionary" );
	
	for (int i = 0; dictionary[ i ].str != NULL; ++i)
	{
		const STR_DICT *pDict = &dictionary[ i ];
		if (bIgnoreCase)
		{
			if (PStrICmp(pDict->str, key) == 0)
				return pDict->value;
		}
		else
		{
			if (PStrCmp(pDict->str, key) == 0)
				return pDict->value;
		}		
	}
	
	return -1;
};
