//----------------------------------------------------------------------------
// picker_common.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
// picks an index based on weight, supports either float or int
// call PickerStart, followed by a bunch of PickerAdds, followed by
// PickerChoose.  use PickerStart or PickerStartF for floats
//----------------------------------------------------------------------------
#ifndef _PICKER_COMMON_H_
#define _PICKER_COMMON_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define PICKER_SIZE_INCREMENT				64
#define PICKER_STACK_MAX					6


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define PickerStartCommon(container, n)				MY_PICKER n(container, FALSE);
#define PickerStartCommonF(container, n)			MY_PICKER n(container, TRUE);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct PICK_NODE
{
	int					m_nIndex;
	union
	{
		DWORD			m_nChance;
		float			m_fChance;
	};
};

struct PICKER
{
	PICK_NODE *			m_pList;
	int					m_nListSize;
	int					m_nCurrent;
	union
	{
		DWORD			m_nTotalChance;
		float			m_fTotalChance;
	};
	BOOL				m_bFloatWeights;
	BOOL				m_bSingleWeight;
	union
	{
		DWORD			m_nSingleWeight;
		float			m_fSingleWeight;
	};
};

struct PICKERS
{
	struct PICKER picker[PICKER_STACK_MAX];
	int cur;
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void PickerInit(
	struct MEMORYPOOL * pool,
	PICKERS *& pickers);

void PickerFree(
	struct MEMORYPOOL * pool,
	PICKERS *& pickers);

void PickerResize(
	struct MEMORYPOOL * pPool,
	PICKER * picker,
	int size);

int PickerChoose(
	PICKERS * pickers,
	struct RAND & rand);

int PickerChooseRemove(
	PICKERS * pickers,
	RAND & rand);

int PickerGetCount(
	PICKERS * pickers);


//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PICKER * PickerGetCurrent(
	PICKERS * pickers)
{
	ASSERT_RETNULL(pickers && pickers->cur >= 0 && pickers->cur < PICKER_STACK_MAX);
	return pickers->picker + pickers->cur;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PICKER * PickerGetNew(
	PICKERS * pickers)
{
	ASSERT_RETNULL(pickers && pickers->cur >= -1 && pickers->cur < PICKER_STACK_MAX - 1);
	pickers->cur++;
	return pickers->picker + pickers->cur;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerEnd(
	PICKERS * pickers)
{
	ASSERT_RETURN(pickers && pickers->cur >= 0 && pickers->cur < PICKER_STACK_MAX);
	pickers->cur--;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerResize(MEMORY_FUNCARGS(
	struct MEMORYPOOL * pPool,
	PICKER * picker,
	int size))
{
	ASSERT_RETURN(picker);

	if (picker->m_nListSize >= size)
	{
		return;
	}
	picker->m_nListSize = size;
	picker->m_pList = (PICK_NODE *)REALLOCFL(pPool, picker->m_pList, picker->m_nListSize * sizeof(PICK_NODE), file, line);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerReset(
	PICKER * picker,
	BOOL bFloatWeights)
{
	ASSERT_RETURN(picker);

	picker->m_nCurrent = 0;
	picker->m_nTotalChance = 0;
	picker->m_bFloatWeights = bFloatWeights;
	picker->m_bSingleWeight = TRUE;
	picker->m_nSingleWeight = 0;
	picker->m_fSingleWeight = 0.0f;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void _PickerStart(
	PICKERS * pickers)
{
	PICKER * picker = PickerGetNew(pickers);
	ASSERT_RETURN(picker);
	PickerReset(picker, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void _PickerStartF(
	PICKERS * pickers)
{
	PICKER * picker = PickerGetNew(pickers);
	ASSERT_RETURN(picker);
	PickerReset(picker, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(
	PICKER * picker,
	int	index,
	int weight)
{
	ASSERT_RETURN(picker);
	ASSERT_RETURN(!picker->m_bFloatWeights);
	ASSERT_RETURN(picker->m_pList);
	ASSERT_RETURN(picker->m_nCurrent < picker->m_nListSize);
	if (weight <= 0)
	{
		return;
	}
	picker->m_nTotalChance += (DWORD)weight;
	if (picker->m_bSingleWeight)
	{
		if (picker->m_nCurrent <= 0)
		{
			picker->m_nSingleWeight = (DWORD)weight;
		}
		else if (picker->m_nSingleWeight != (DWORD)weight)
		{
			picker->m_bSingleWeight = FALSE;
		}
	}
	picker->m_pList[picker->m_nCurrent].m_nIndex = index;
	picker->m_pList[picker->m_nCurrent].m_nChance = picker->m_nTotalChance;
	picker->m_nCurrent++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(MEMORY_FUNCARGS(
	PICKERS * pickers,
	struct MEMORYPOOL * pool,
	int	index,
	int weight))
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETURN(picker);

	ASSERT_RETURN(!picker->m_bFloatWeights);
	if (picker->m_nCurrent >= picker->m_nListSize)
	{
		PickerResize(MEMORY_FUNC_PASSFL(pool, picker, picker->m_nCurrent + PICKER_SIZE_INCREMENT));
	}
	
	PickerAdd(picker, index, weight);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(
	PICKER * picker,
	int	index,
	float weight)
{
	ASSERT_RETURN(picker);
	ASSERT_RETURN(picker->m_bFloatWeights);
	ASSERT_RETURN(picker->m_pList);
	ASSERT_RETURN(picker->m_nCurrent < picker->m_nListSize);

	if (weight <= EPSILON)
	{
		return;
	}
	if (picker->m_bSingleWeight)
	{
		if (picker->m_nCurrent <= 0)
		{
			picker->m_fSingleWeight = weight;
		}
		else if (picker->m_fSingleWeight != weight)
		{
			picker->m_bSingleWeight = FALSE;
		}
	}
	picker->m_fTotalChance += weight;
	picker->m_pList[picker->m_nCurrent].m_nIndex = index;
	picker->m_pList[picker->m_nCurrent].m_fChance = picker->m_fTotalChance;
	picker->m_nCurrent++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(MEMORY_FUNCARGS(
	PICKERS * pickers,
	struct MEMORYPOOL * pool,
	int	index,
	float weight))
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETURN(picker);

	ASSERT_RETURN(picker->m_bFloatWeights);
	if (picker->m_nCurrent >= picker->m_nListSize)
	{
		PickerResize(MEMORY_FUNC_PASSFL(pool, picker, picker->m_nCurrent + PICKER_SIZE_INCREMENT));
	}
	PickerAdd(picker, index, weight);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PickerIsEmpty(
	PICKER * picker)
{
	ASSERT_RETVAL(picker, TRUE);

	if (picker->m_nCurrent <= 0)
	{
		return TRUE;
	}

	if (picker->m_bFloatWeights)
	{
		return (picker->m_fTotalChance <= 0);
	}
	else
	{
		return (picker->m_nCurrent <= 0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PickerIsEmpty(
	PICKERS * pickers)
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETVAL(picker, TRUE);

	return PickerIsEmpty(picker);
}


//----------------------------------------------------------------------------
// Helper Class
//----------------------------------------------------------------------------
struct MY_PICKER
{
	PICKERS * m_pPickers;

	MY_PICKER(
		PICKERS * pickers,
		BOOL bFloat) : m_pPickers(pickers)
	{
		if (bFloat)
		{
			_PickerStartF(pickers);
		}
		else
		{
			_PickerStart(pickers);
		}
	}
	~MY_PICKER()
	{
		if(m_pPickers)
		{
			PickerEnd(m_pPickers);
		}
	}
};


#endif  // _PICKER_COMMON_H_

