//----------------------------------------------------------------------------
// uix_nametable.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "nametable.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define NAMETABLE_HASHSIZE				256
#define NAMETABLE_BUFFERSIZE			255


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NAMETABLE_NODE
{
	const char *			m_pName;
	int						m_nType;
	NAMETABLE_NODE *		m_pNext;
	void *					m_Data;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NAMETABLE_NODEPOOL
{
	NAMETABLE_NODE			m_Nodes[NAMETABLE_BUFFERSIZE];
	NAMETABLE_NODEPOOL *	m_pNext;
};


//----------------------------------------------------------------------------
// NAMETABLE
// name table.  the data element can be either a pointer or an id depending
// on what we need for safety
//----------------------------------------------------------------------------
struct NAMETABLE
{
public:
	NAMETABLE_NODE *		m_Hash[NAMETABLE_HASHSIZE];

	NAMETABLE_NODE *		m_pGarbage;

	NAMETABLE_NODEPOOL *	m_pNodePool;
	unsigned int			m_nFirstUnused;

	BOOL					m_bFreeData;		// should nametable free data?
};


//----------------------------------------------------------------------------
// INTERNAL FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int ComputeKey(
	const char * str)
{
	unsigned int key = 0;
	while (*str)
	{
		key = (key << 1) + *str;
		str++;
	}
	return (key % NAMETABLE_HASHSIZE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static NAMETABLE_NODE * GetEmptyNode(
	NAMETABLE * nametable)
{
	NAMETABLE_NODE * node = NULL;

	if (nametable->m_pGarbage)
	{
		node = nametable->m_pGarbage;
		nametable->m_pGarbage = node->m_pNext;
	}
	else if (nametable->m_nFirstUnused == 0 || nametable->m_nFirstUnused >= NAMETABLE_BUFFERSIZE)
	{
		NAMETABLE_NODEPOOL * pool = (NAMETABLE_NODEPOOL *)MALLOC(NULL, sizeof(NAMETABLE_NODEPOOL));
		pool->m_pNext = nametable->m_pNodePool;
		nametable->m_pNodePool = pool;
		nametable->m_nFirstUnused = 1;
		node = pool->m_Nodes;
	}
	else
	{
		ASSERT_RETNULL(nametable->m_pNodePool);
		node = nametable->m_pNodePool->m_Nodes + nametable->m_nFirstUnused;
		nametable->m_nFirstUnused++;
	}
	memclear(node, sizeof(NAMETABLE_NODE));
	return node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void RecycleNode(
	NAMETABLE * nametable,
	NAMETABLE_NODE * node)
{
	node->m_Data = NULL;

	node->m_pNext = nametable->m_pGarbage;
	nametable->m_pGarbage = node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AddToHash(
	NAMETABLE * nametable,
	NAMETABLE_NODE * node)
{
	unsigned int key = ComputeKey(node->m_pName);
	node->m_pNext = nametable->m_Hash[key];
	nametable->m_Hash[key] = node;
}


//----------------------------------------------------------------------------
// look up a name, fills in type if given, returns data associated with the name
//----------------------------------------------------------------------------
static void * NameLookup(
	NAMETABLE * nametable,
	const char * name,
	int * type = NULL)
{
	unsigned int key = ComputeKey(name);
	NAMETABLE_NODE * node = nametable->m_Hash[key];
	while (node)
	{
		if (PStrCmp(name, node->m_pName) == 0)
		{
			if (type)
			{
				*type = node->m_nType;
			}
			return node->m_Data;
		}
		node = node->m_pNext;
	}
	return NULL;
}


//----------------------------------------------------------------------------
// returns FALSE if name is already registered
//----------------------------------------------------------------------------
static BOOL RegisterName(	
	NAMETABLE * nametable,
	const char * name,
	int type,
	void * data)
{
	unsigned int key = ComputeKey(name);
	NAMETABLE_NODE * node = nametable->m_Hash[key];
	while (node)
	{
		if (PStrCmp(name, node->m_pName) == 0)
		{
			return FALSE;
		}
		node = node->m_pNext;
	}
	node = GetEmptyNode(nametable);
	node->m_pName = name;
	node->m_nType = type;
	node->m_Data = data;
	node->m_pNext = nametable->m_Hash[key];
	nametable->m_Hash[key] = node;
	return TRUE;
}


//----------------------------------------------------------------------------
// asserts if type or data doesn't match
//----------------------------------------------------------------------------
static BOOL UnregisterName(
	NAMETABLE * nametable,
	const char * name,
	int type,
	void * data)
{
	unsigned int key = ComputeKey(name);
	NAMETABLE_NODE* prev = NULL;
	NAMETABLE_NODE* node = nametable->m_Hash[key];
	while (node)
	{
		if (PStrCmp(name, node->m_pName) == 0)
		{
			ASSERT(type == node->m_nType);
			ASSERT(data == node->m_Data);
			if (type != node->m_nType || data != node->m_Data)
			{
				return FALSE;
			}
			if (prev)
			{
				prev->m_pNext = node->m_pNext;
			}
			else
			{
				nametable->m_Hash[key] = node->m_pNext;
			}

			if (nametable->m_bFreeData)
			{
				FREE(NULL, node->m_Data);
				node->m_Data = NULL;
			}

			RecycleNode(nametable, node);
			return TRUE;
		}
		prev = NULL;
		node = node->m_pNext;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// changes name to be unique, maxlen is string length
//----------------------------------------------------------------------------
void MakeUniqueName(
	NAMETABLE * nametable,
	char * name,
	int maxlen)
{
	int key = ComputeKey(name);
	int len = PStrLen(name);
	int add = 0;
	ASSERT(len + 5 < maxlen);

	BOOL bDupe = FALSE;
	do
	{
		bDupe = FALSE;

		NAMETABLE_NODE* node = nametable->m_Hash[key];
		while (node)
		{
			if (PStrCmp(name, node->m_pName, maxlen) == 0)
			{
				bDupe = TRUE;
				add++;
				ASSERT(add <= 9999);
				PIntToStr(name + len, maxlen - len, add);
				break;
			}
			node = node->m_pNext;
		}
	} while (bDupe);
}


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameTableFree(
	NAMETABLE * nametable)
{
	if (nametable)
	{
		if (nametable->m_bFreeData)
		{
			for (int ii = 0; ii < NAMETABLE_HASHSIZE; ii++)
			{
				NAMETABLE_NODE* node = nametable->m_Hash[ii];
				while (node)
				{
					if (node->m_Data)
					{
						FREE(NULL, node->m_Data);
					}
					node = node->m_pNext;
				}
			}
		}
		while (nametable->m_pNodePool)
		{
			NAMETABLE_NODEPOOL* next = nametable->m_pNodePool->m_pNext;
			FREE(NULL, nametable->m_pNodePool);
			nametable->m_pNodePool = next;
		}
		FREE(NULL, nametable);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NAMETABLE * NameTableInit(
	BOOL bFreeData)
{
	NAMETABLE * nametable = (NAMETABLE *)MALLOCZ(NULL, sizeof(NAMETABLE));

	nametable->m_bFreeData = bFreeData;

	return nametable;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * NameTableLookup(
	NAMETABLE * nametable,	
	const char * name,
	int * type)
{
	ASSERT_RETNULL(nametable);
	return NameLookup(nametable, name, type);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NameTableRegister(	
	NAMETABLE * nametable,	
	const char * name,
	int type,
	void * data)
{
	ASSERT_RETFALSE(nametable);
	return RegisterName(nametable, name, type, data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NameTableUnregister(
	NAMETABLE * nametable,	
	const char * name,
	int type,
	void * data)
{
	ASSERT_RETFALSE(nametable);
	return UnregisterName(nametable, name, type, data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameTableMakeUniqueName(
	NAMETABLE * nametable,	
	char * name,
	int maxlen)
{
	ASSERT_RETURN(nametable);
	return MakeUniqueName(nametable, name, maxlen);
}
