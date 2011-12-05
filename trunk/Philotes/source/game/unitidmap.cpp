//----------------------------------------------------------------------------
// unitidmap.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "unitidmap.h"

#include "game.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define UNIT_ID_MAP_BUFFER_SIZE		8


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct UNIT_ID_MAP_NODE
{
	int						m_nAlias;
	union
	{
		DWORD				m_idUnit;
		UNIT_ID_MAP_NODE*	m_pNext;
	};
};

struct UNIT_ID_MAP
{
	GAME*					game;
	UNIT_ID_MAP_NODE**		m_pNodes;
	UNIT_ID_MAP_NODE**		m_pReverse;
	UNIT_ID_MAP_NODE*		m_pGarbage;

	int						m_nCurrentIdx;
	int						m_nCount;
	int						m_nSize;

	BOOL					m_bNodesSorted;
	BOOL					m_bReverseSorted;
	BOOL					m_bSortOnInsert;
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UNIT_ID_MAP* UnitIdMapInit(
	GAME* game)
{
	ASSERT_RETNULL(game);

	UNIT_ID_MAP* idmap = (UNIT_ID_MAP*)GMALLOCZ(game, sizeof(UNIT_ID_MAP));
	idmap->game = game;
	idmap->m_bNodesSorted = TRUE;
	idmap->m_bReverseSorted = TRUE;
	return idmap;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitIdMapGetCount(
	UNIT_ID_MAP* idmap)
{
	ASSERT_RETZERO(idmap);
	return idmap->m_nCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitIdMapFree(
	UNIT_ID_MAP* idmap)
{
	if (!idmap)
	{
		return;
	}
	
	if (idmap->m_pNodes)
	{
		for (int ii = 0; ii < idmap->m_nCount; ii++)
		{
			ASSERT_RETURN(idmap->m_pNodes[ii]);
			GFREE(idmap->game, idmap->m_pNodes[ii]);
		}
		GFREE(idmap->game, idmap->m_pNodes);
	}
	if (idmap->m_pReverse)
	{
		GFREE(idmap->game, idmap->m_pReverse);
	}
	UNIT_ID_MAP_NODE* curr = idmap->m_pGarbage;
	while (curr)
	{
		UNIT_ID_MAP_NODE* next = curr->m_pNext;
		GFREE(idmap->game, curr);
		curr = next;
	}

	GFREE(idmap->game, idmap);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitIdMapReset(
	UNIT_ID_MAP* idmap,
	BOOL bSortOnInsert)
{
	ASSERT_RETURN(idmap);
	// put everything on garbage
	if (idmap->m_pNodes)
	{
		for (int ii = 0; ii < idmap->m_nCount; ii++)
		{
			UNIT_ID_MAP_NODE* curr = idmap->m_pNodes[ii];
			curr->m_pNext = idmap->m_pGarbage;
			idmap->m_pGarbage = curr;
		}
		memclear(idmap->m_pNodes, idmap->m_nCount * sizeof(UNIT_ID_MAP_NODE*));
	}

	// clear aliases in garbage
	int alias = 0;
	UNIT_ID_MAP_NODE* curr = idmap->m_pGarbage;
	while (curr)
	{
		UNIT_ID_MAP_NODE* next = curr->m_pNext;
		curr->m_nAlias = alias;
		alias++;
		curr = next;
	}
	idmap->m_nCount = 0;
	idmap->m_bNodesSorted = TRUE;
	idmap->m_bReverseSorted = TRUE;
	idmap->m_bSortOnInsert = bSortOnInsert;
}


//----------------------------------------------------------------------------
// return index or insertion point (first larger selector)
//----------------------------------------------------------------------------
inline int sUnitIdMapFindNode(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	if (idmap->m_nCount <= 0)
	{
		return 0;
	}

	UNIT_ID_MAP_NODE** min = idmap->m_pNodes;
	UNIT_ID_MAP_NODE** max = min + idmap->m_nCount;
	UNIT_ID_MAP_NODE** ii = min + (max - min) / 2;

	while (max > min)
	{
		if ((*ii)->m_idUnit > idUnit)
		{
			max = ii;
		}
		else if ((*ii)->m_idUnit < idUnit)
		{
			min = ii + 1;
		}
		else
		{
			return (int)(ii - idmap->m_pNodes);
		}
		ii = min + (max - min) / 2;
	}
	return (int)(max - idmap->m_pNodes);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUnitIdMapSortNodesCompare(
	const void* a,
	const void* b)
{
	UNIT_ID_MAP_NODE** A = (UNIT_ID_MAP_NODE**)a;
	UNIT_ID_MAP_NODE** B = (UNIT_ID_MAP_NODE**)b;

	if ((*A)->m_idUnit < (*B)->m_idUnit)
	{
		return -1;
	}
	else if ((*A)->m_idUnit > (*B)->m_idUnit)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitIdMapSortNodes(
	UNIT_ID_MAP* idmap)
{
	if (idmap->m_nCount <= 0)
	{
		idmap->m_bNodesSorted = TRUE;
		return;
	}
	if (idmap->m_bNodesSorted)
	{
		return;
	}
	qsort(idmap->m_pNodes, idmap->m_nCount, sizeof(UNIT_ID_MAP_NODE*), sUnitIdMapSortNodesCompare);
	idmap->m_bNodesSorted = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sUnitIdMapGetOrAddSortOnInsert(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	ASSERT_RETINVALID(idmap);

	UnitIdMapSortNodes(idmap);

	int ii = sUnitIdMapFindNode(idmap, idUnit);
	if (!(ii < idmap->m_nCount && idmap->m_pNodes[ii]->m_idUnit == idUnit))
	{
		if (idmap->m_nCount >= idmap->m_nSize)
		{
			idmap->m_nSize += UNIT_ID_MAP_BUFFER_SIZE;
			idmap->m_pNodes = (UNIT_ID_MAP_NODE**)GREALLOC(idmap->game, idmap->m_pNodes, idmap->m_nSize * sizeof(UNIT_ID_MAP_NODE*));
			idmap->m_pReverse = (UNIT_ID_MAP_NODE**)GREALLOC(idmap->game, idmap->m_pReverse, idmap->m_nSize * sizeof(UNIT_ID_MAP_NODE*));
		}
		if (ii < idmap->m_nCount)
		{
			MemoryMove(idmap->m_pNodes + ii + 1, (idmap->m_nSize - ii - 1) * sizeof(UNIT_ID_MAP_NODE*), idmap->m_pNodes + ii, (idmap->m_nCount - ii) * sizeof(UNIT_ID_MAP_NODE*));
		}

		if (idmap->m_pGarbage)
		{
			idmap->m_pNodes[ii] = idmap->m_pGarbage;
			idmap->m_pGarbage = idmap->m_pGarbage->m_pNext;
		}
		else
		{
			idmap->m_pNodes[ii] = (UNIT_ID_MAP_NODE*)GMALLOCZ(idmap->game, sizeof(UNIT_ID_MAP_NODE));
			idmap->m_pNodes[ii]->m_nAlias = idmap->m_nCount;
		}
		idmap->m_pNodes[ii]->m_idUnit = idUnit;
		idmap->m_pReverse[idmap->m_nCount] = idmap->m_pNodes[ii];
		idmap->m_bReverseSorted = FALSE;
		idmap->m_nCount++;
	}
	return idmap->m_pNodes[ii]->m_nAlias;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sUnitIdMapGetOrAddUnsorted(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	ASSERT_RETINVALID(idmap);

	if (idmap->m_nCount >= idmap->m_nSize)
	{
		idmap->m_nSize += UNIT_ID_MAP_BUFFER_SIZE;
		idmap->m_pNodes = (UNIT_ID_MAP_NODE**)GREALLOC(idmap->game, idmap->m_pNodes, idmap->m_nSize * sizeof(UNIT_ID_MAP_NODE*));
		idmap->m_pReverse = (UNIT_ID_MAP_NODE**)GREALLOC(idmap->game, idmap->m_pReverse, idmap->m_nSize * sizeof(UNIT_ID_MAP_NODE*));
	}
	int ii = idmap->m_nCount;
	if (idmap->m_pGarbage)
	{
		idmap->m_pNodes[ii] = idmap->m_pGarbage;
		idmap->m_pGarbage = idmap->m_pGarbage->m_pNext;
	}
	else
	{
		idmap->m_pNodes[ii] = (UNIT_ID_MAP_NODE*)GMALLOCZ(idmap->game, sizeof(UNIT_ID_MAP_NODE));
	}
	idmap->m_pNodes[ii]->m_nAlias = idmap->m_nCount;
	idmap->m_pNodes[ii]->m_idUnit = idUnit;
	idmap->m_pReverse[idmap->m_nCount] = idmap->m_pNodes[ii];
	idmap->m_bReverseSorted = FALSE;
	idmap->m_bNodesSorted = FALSE;
	idmap->m_nCount++;
	return idmap->m_pNodes[ii]->m_nAlias;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitIdMapGetOrAdd(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	ASSERT_RETINVALID(idmap);

	if (idmap->m_bSortOnInsert)
	{
		return sUnitIdMapGetOrAddSortOnInsert(idmap, idUnit);
	}
	else
	{
		return sUnitIdMapGetOrAddUnsorted(idmap, idUnit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitIdMapFindByUnitID(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	ASSERT_RETINVALID(idmap);

	UnitIdMapSortNodes(idmap);

	int ii = sUnitIdMapFindNode(idmap, idUnit);
	if (!(ii < idmap->m_nCount && idmap->m_pNodes[ii]->m_idUnit == idUnit))
	{
		return INVALID_ID;
	}
	return idmap->m_pNodes[ii]->m_nAlias;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int sUnitIdMapFindReverse(
	UNIT_ID_MAP* idmap,
	int nAlias)
{
	if (idmap->m_nCount <= 0)
	{
		return 0;
	}

	UNIT_ID_MAP_NODE** min = idmap->m_pReverse;
	UNIT_ID_MAP_NODE** max = min + idmap->m_nCount;
	UNIT_ID_MAP_NODE** ii = min + (max - min) / 2;

	while (max > min)
	{
		if ((*ii)->m_nAlias > nAlias)
		{
			max = ii;
		}
		else if ((*ii)->m_nAlias < nAlias)
		{
			min = ii + 1;
		}
		else
		{
			return (int)(ii - idmap->m_pReverse);
		}
		ii = min + (max - min) / 2;
	}
	return (int)(max - idmap->m_pReverse);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUnitIdMapSortReverseCompare(
	const void* a,
	const void* b)
{
	UNIT_ID_MAP_NODE** A = (UNIT_ID_MAP_NODE**)a;
	UNIT_ID_MAP_NODE** B = (UNIT_ID_MAP_NODE**)b;

	if ((*A)->m_nAlias < (*B)->m_nAlias)
	{
		return -1;
	}
	else if ((*A)->m_nAlias > (*B)->m_nAlias)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitIdMapSortReverse(
	UNIT_ID_MAP* idmap)
{
	if (idmap->m_nCount <= 0)
	{
		idmap->m_bReverseSorted = TRUE;
		return;
	}
	if (idmap->m_bReverseSorted)
	{
		return;
	}
	qsort(idmap->m_pReverse, idmap->m_nCount, sizeof(UNIT_ID_MAP_NODE*), sUnitIdMapSortReverseCompare);
	idmap->m_bReverseSorted = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UnitIdMapFindByAlias(
	UNIT_ID_MAP* idmap,
	int nAlias)
{
	UnitIdMapSortReverse(idmap);
	int ii = sUnitIdMapFindReverse(idmap, nAlias);
	if (!(ii < idmap->m_nCount && idmap->m_pReverse[ii]->m_nAlias == nAlias))
	{
		return INVALID_ID;
	}
	ASSERT_RETINVALID(idmap->m_pReverse[ii]);
	return idmap->m_pReverse[ii]->m_idUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitIdMapRemove(
	UNIT_ID_MAP* idmap,
	UNITID idUnit)
{
	ASSERT_RETURN(idmap);

	idmap->m_bSortOnInsert = FALSE;

	UnitIdMapSortNodes(idmap);
	int ii = sUnitIdMapFindNode(idmap, idUnit);
	if (!(ii < idmap->m_nCount && idmap->m_pNodes[ii]->m_idUnit == idUnit))
	{
		return;
	}
	UNIT_ID_MAP_NODE* curr = idmap->m_pNodes[ii];
	ASSERT_RETURN(curr && curr->m_idUnit == idUnit);

	if (ii < idmap->m_nCount - 1)
	{
		MemoryMove(idmap->m_pNodes, (idmap->m_nSize) * sizeof(UNIT_ID_MAP_NODE*), idmap->m_pNodes + 1, (idmap->m_nCount - ii - 1) * sizeof(UNIT_ID_MAP_NODE*));
	}

	UnitIdMapSortReverse(idmap);
	ii = sUnitIdMapFindReverse(idmap, curr->m_nAlias);
	ASSERT(ii < idmap->m_nCount && idmap->m_pReverse[ii]->m_nAlias == curr->m_nAlias);

	if (ii < idmap->m_nCount - 1)
	{
		MemoryMove(idmap->m_pReverse, (idmap->m_nSize) * sizeof(UNIT_ID_MAP_NODE*), idmap->m_pReverse + 1, (idmap->m_nCount - ii - 1) * sizeof(UNIT_ID_MAP_NODE*));
	}
	idmap->m_nCount--;
	curr->m_pNext = idmap->m_pGarbage;
	idmap->m_pGarbage = curr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitIdMapRemove(
	UNIT_ID_MAP* idmap,
	int nAlias)
{
	ASSERT_RETURN(idmap);

	idmap->m_bSortOnInsert = FALSE;

	UnitIdMapSortReverse(idmap);
	int ii = sUnitIdMapFindReverse(idmap, nAlias);
	if (!(ii < idmap->m_nCount && idmap->m_pReverse[ii]->m_nAlias == nAlias))
	{
		return;
	}
	UNIT_ID_MAP_NODE* curr = idmap->m_pReverse[ii];
	ASSERT_RETURN(curr && curr->m_nAlias == nAlias);

	if (ii < idmap->m_nCount - 1)
	{
		MemoryMove(idmap->m_pReverse, (idmap->m_nSize) * sizeof(UNIT_ID_MAP_NODE*), idmap->m_pReverse + 1, (idmap->m_nCount - ii - 1) * sizeof(UNIT_ID_MAP_NODE*));
	}

	UnitIdMapSortNodes(idmap);
	ii = sUnitIdMapFindNode(idmap, curr->m_idUnit);
	ASSERT(ii < idmap->m_nCount && idmap->m_pNodes[ii]->m_idUnit == curr->m_idUnit);

	if (ii < idmap->m_nCount - 1)
	{
		MemoryMove(idmap->m_pNodes, (idmap->m_nSize) * sizeof(UNIT_ID_MAP_NODE*), idmap->m_pNodes + 1, (idmap->m_nCount - ii - 1) * sizeof(UNIT_ID_MAP_NODE*));
	}

	idmap->m_nCount--;
	curr->m_pNext = idmap->m_pGarbage;
	idmap->m_pGarbage = curr;
}




