//----------------------------------------------------------------------------
// bitset.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "bitmanip.h"

namespace FSCommon
{

// Class CBitSet
//
template<unsigned int BIT_COUNT>
class CBitSet
{

	public:
	
		CBitSet(bool initializeToZero = true);
		
	public:
		
		bool			Test(int index);
		void			Set(int index);
		void			Set();
		void			Reset(unsigned int index);
		void			Reset();
		int				GetFirstSetBitIndex();
			

	private:
	
		DWORD			m_Bits[BIT_COUNT/BITS_PER_DWORD];

};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CBitSet Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
CBitSet<BIT_COUNT>::CBitSet(bool initializeToZero = true)
{
	initializeToZero ? Reset() : Set();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Test
//
// Parameters
//	index : The index of the bit to test
//
// Returns
//	true if the bit specified at the index is set, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
bool CBitSet<BIT_COUNT>::Test(int index)
{
	if(index <= BIT_COUNT)
	{
		return (TESTBIT(m_Bits, index) == TRUE);		
	}
	else
	{
		return false;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Set
//
// Parameters
//	index : The index of the bit to set
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
void CBitSet<BIT_COUNT>::Set(int index)
{
	if(index >= 0 && index <= BIT_COUNT)
	{
		SETBIT(m_Bits, index);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Set
//
// Remarks
//	Sets all bits to 1
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
void CBitSet<BIT_COUNT>::Set()
{
	memset(m_Bits, 0xFF, sizeof(m_Bits));						
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Reset
//
// Parameters
//	index : The index of the bit to reset
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
void CBitSet<BIT_COUNT>::Reset(unsigned int index)
{
	if(index >= 0 && index <= BIT_COUNT)
	{
		CLEARBIT(m_Bits, index);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Reset
//
// Remarks
//	Sets all bits to 0
//
/////////////////////////////////////////////////////////////////////////////
template<unsigned int BIT_COUNT>
void CBitSet<BIT_COUNT>::Reset()
{
	memset(m_Bits, 0, sizeof(m_Bits));			
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFirstSetBitIndex
//
// Returns
//	The offset into the bit set of the first set bit.  -1 if there
//	are none.
//
/////////////////////////////////////////////////////////////////////////////				
template<unsigned int BIT_COUNT>
int CBitSet<BIT_COUNT>::GetFirstSetBitIndex()
{
	int bit = -1;
	
	// Iterate over all the DWORDs in the free set
	//
	for(unsigned int i = 0; i < arrsize(m_Bits); ++i)
	{
		if(m_Bits[i] != 0)
		{
			bit = (i * BITS_PER_DWORD) + LEAST_SIGNIFICANT_BIT((unsigned int)m_Bits[i]);
			break;
		}
	}			
	
	if(bit > BIT_COUNT)
	{
		return -1;
	}
	
	return bit;
}	


} // end namespace FSCommon
