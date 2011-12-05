//----------------------------------------------------------------------------
// picker_common.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "picker_common.h"


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PickerInit(
	struct MEMORYPOOL * pool,
	PICKERS *& pickers)
{
	if (!pickers)
	{
		pickers = (PICKERS *)MALLOCZ(pool, sizeof(PICKERS));
		pickers->cur = -1;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PickerFree(
	struct MEMORYPOOL * pool,
	PICKERS *& pickers)
{
	if (!pickers)
	{
		return;
	}
	for (int ii = 0; ii < PICKER_STACK_MAX; ii++)
	{
		PICKER * picker = pickers->picker + ii;
		if (picker->m_pList)
		{
			FREE(pool, picker->m_pList);
		}
	}
	FREE(pool, pickers);
	pickers = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseSingleWeightIdx(
	PICKER * picker,
	RAND & rand)
{
	if (picker->m_nCurrent <= 0 || picker->m_nTotalChance <= 0)
	{
		return INVALID_INDEX;
	}
	ASSERT_RETVAL(picker->m_pList && picker->m_nCurrent <= picker->m_nListSize, INVALID_INDEX);
	return RandGetNum(rand, picker->m_nCurrent);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseSingleWeight(
	PICKER * picker,
	RAND & rand)
{
	int roll = sPickerChooseSingleWeightIdx(picker, rand);
	if (roll < 0 || roll >= picker->m_nCurrent)
	{
		return INVALID_INDEX;
	}
	PICK_NODE * ii = picker->m_pList + roll;
	return ii->m_nIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseIdxI(
	PICKER * picker,
	RAND & rand)
{
	if (picker->m_nCurrent <= 0 || picker->m_nTotalChance <= 0)
	{
		return INVALID_INDEX;
	}
	ASSERT_RETVAL(picker->m_pList && picker->m_nCurrent <= picker->m_nListSize, INVALID_INDEX);
	DWORD total = picker->m_nTotalChance;
	DWORD roll = RandGetNum(rand, total);

	PICK_NODE * min = picker->m_pList;
	PICK_NODE * max = picker->m_pList + picker->m_nCurrent;
	PICK_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		if (ii->m_nChance > roll)
		{
			max = ii;
		}
		else if (ii->m_nChance < roll)
		{
			min = ii + 1;
		}
		else
		{
			// return NEXT item after ii
			ii = ii + 1;
			ASSERT_RETVAL(ii < picker->m_pList + picker->m_nCurrent, INVALID_INDEX);
			ASSERT_RETVAL(roll < ii->m_nChance, INVALID_INDEX);
			return SIZE_TO_INT(ii - picker->m_pList);
		}
		ii = min + (max - min) / 2;
	}
	// return NEXT item after max
	ii = max;
	ASSERT_RETVAL(ii >= picker->m_pList && ii < picker->m_pList + picker->m_nCurrent, INVALID_INDEX);
	ASSERT_RETVAL(roll < ii->m_nChance, INVALID_INDEX);
	return SIZE_TO_INT(ii - picker->m_pList);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseI(
	PICKER * picker,
	RAND & rand)
{
	if (picker->m_nCurrent <= 0 || picker->m_nTotalChance <= 0)
	{
		return INVALID_INDEX;
	}
	int ii = sPickerChooseIdxI(picker, rand);
	ASSERT_RETVAL(ii >= 0 && ii < picker->m_nCurrent, INVALID_INDEX);
	PICK_NODE * choice = picker->m_pList + ii;
	return choice->m_nIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseIdxF(
	PICKER * picker,
	RAND & rand)
{
	if (picker->m_nCurrent <= 0 || picker->m_fTotalChance <= EPSILON)
	{
		return INVALID_INDEX;
	}
	ASSERT_RETVAL(picker->m_pList && picker->m_nCurrent <= picker->m_nListSize, INVALID_INDEX);
	float total = picker->m_fTotalChance;
	float roll = RandGetFloat(rand, total);

	PICK_NODE * min = picker->m_pList;
	PICK_NODE * max = picker->m_pList + picker->m_nCurrent;
	PICK_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		if (ii->m_fChance > roll)
		{
			max = ii;
		}
		else if (ii->m_fChance < roll)
		{
			min = ii + 1;
		}
		else
		{
			// return NEXT item after ii
			ii = ii + 1;
			if (ii >= picker->m_pList + picker->m_nCurrent)
			{
				ii = &picker->m_pList[picker->m_nCurrent-1];
			}
			return SIZE_TO_INT(ii - picker->m_pList);
		}
		ii = min + (max - min) / 2;
	}
	// return NEXT item after max
	ii = max;
	ASSERT_RETVAL(ii >= picker->m_pList && ii < picker->m_pList + picker->m_nCurrent, INVALID_INDEX);
	return SIZE_TO_INT(ii - picker->m_pList);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseF(
	PICKER * picker,
	RAND & rand)
{
	if (picker->m_nCurrent <= 0 || picker->m_fTotalChance <= EPSILON)
	{
		return INVALID_INDEX;
	}
	int ii = sPickerChooseIdxF(picker, rand);
	ASSERT_RETVAL(ii >= 0 && ii < picker->m_nCurrent, INVALID_INDEX);
	PICK_NODE * choice = picker->m_pList + ii;
	return choice->m_nIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChoose(
	PICKER * picker,
	RAND & rand)
{
	ASSERT_RETVAL(picker, INVALID_INDEX);
	int idx = INVALID_INDEX;

	if (picker->m_bSingleWeight)
	{
		idx = sPickerChooseSingleWeight(picker, rand);
	}
	else if (picker->m_bFloatWeights)
	{
		idx = sPickerChooseF(picker, rand);
	}
	else
	{
		idx = sPickerChooseI(picker, rand);
	}

	return idx;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PickerChoose(
	PICKERS * pickers,
	RAND & rand)
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETVAL(picker, INVALID_INDEX);
	return sPickerChoose(picker, rand);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPickerChooseRemove(
	PICKER * picker,
	RAND & rand)
{
	ASSERT_RETVAL(picker, INVALID_INDEX);

	int idx = INVALID_INDEX;

	if (picker->m_bSingleWeight)
	{
		idx = sPickerChooseSingleWeightIdx(picker, rand);
	}
	else if (picker->m_bFloatWeights)
	{
		idx = sPickerChooseIdxF(picker, rand);
	}
	else
	{
		idx = sPickerChooseIdxI(picker, rand);
	}

	// GS - 3/15/2007
	// This used to be an ASSERT, but really, it's okay not to do that, particularly if the picker is empty when you're doing this.
	if(idx < 0 || idx >= picker->m_nCurrent)
		return INVALID_INDEX;

	PICK_NODE * choice = picker->m_pList + idx;
	int retval = choice->m_nIndex;

	// do the remove part
	if (picker->m_bFloatWeights)
	{
		float weight = choice->m_fChance - (idx > 0 ? picker->m_pList[idx-1].m_fChance : 0);

		if (idx < picker->m_nCurrent - 1)
		{
			ASSERT_RETVAL(MemoryMove(picker->m_pList + idx, (picker->m_nListSize - idx) * sizeof(PICK_NODE), picker->m_pList + idx + 1, sizeof(PICK_NODE) * (picker->m_nCurrent - idx - 1)),INVALID_INDEX);
		}
		picker->m_nCurrent--;
		picker->m_fTotalChance -= weight;
		PICK_NODE * cur = picker->m_pList + idx;
		for (int ii = idx; ii < picker->m_nCurrent; ii++, cur++)
		{
			cur->m_fChance -= weight;
		}
	}
	else
	{
		int weight = choice->m_nChance - (idx > 0 ? picker->m_pList[idx-1].m_nChance : 0);

		if (idx < picker->m_nCurrent - 1)
		{
			ASSERT_RETVAL(MemoryMove(picker->m_pList + idx, (picker->m_nListSize - idx) * sizeof(PICK_NODE), picker->m_pList + idx + 1, sizeof(PICK_NODE) * (picker->m_nCurrent - idx - 1)),INVALID_INDEX);
		}
		picker->m_nCurrent--;
		picker->m_nTotalChance -= weight;
		PICK_NODE * cur = picker->m_pList + idx;
		for (int ii = idx; ii < picker->m_nCurrent; ii++, cur++)
		{
			cur->m_nChance -= weight;
		}
	}

	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PickerChooseRemove(
	PICKERS * pickers,
	RAND & rand)
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETVAL(picker, INVALID_INDEX);
	return sPickerChooseRemove(picker, rand);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PickerGetCount(
	PICKERS * pickers)
{
	PICKER * picker = PickerGetCurrent(pickers);
	ASSERT_RETVAL(picker, INVALID_INDEX);

	return picker->m_nCurrent;
}
