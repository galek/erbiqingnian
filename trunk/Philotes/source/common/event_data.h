//----------------------------------------------------------------------------
// event_data.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _EVENT_DATA_H_
#define _EVENT_DATA_H_


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct EVENT_DATA
{
	DWORD_PTR	m_Data1;
	DWORD_PTR	m_Data2;
	DWORD_PTR	m_Data3;
	DWORD_PTR	m_Data4;
	DWORD_PTR	m_Data5;
	DWORD_PTR	m_Data6;

	EVENT_DATA() : m_Data1(0), m_Data2(0), m_Data3(0), m_Data4(0), m_Data5(0), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1) : m_Data1(data1), m_Data2(0), m_Data3(0), m_Data4(0), m_Data5(0), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1, DWORD_PTR data2) : m_Data1(data1), m_Data2(data2), m_Data3(0), m_Data4(0), m_Data5(0), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(0), m_Data5(0), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3, DWORD_PTR data4) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(data4), m_Data5(0), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3, DWORD_PTR data4, DWORD_PTR data5) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(data4), m_Data5(data5), m_Data6(0) {};
	EVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3, DWORD_PTR data4, DWORD_PTR data5, DWORD_PTR data6) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(data4), m_Data5(data5), m_Data6(data6) {};

	EVENT_DATA * operator&(void)
	{
		return (EVENT_DATA *)this;
	}
};


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum EVENT_DATA_COMPARE
{
	EDC_PARAM1_BIT,
	EDC_PARAM2_BIT,
	EDC_PARAM3_BIT,
	EDC_PARAM4_BIT,
	EDC_PARAM5_BIT,
	EDC_PARAM6_BIT
};


//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline DWORD_PTR EventFloatToParam(
	float fParam)
{ 
	return (DWORD_PTR)*((DWORD_PTR *)&fParam); 
}

//----------------------------------------------------------------------------
inline float EventParamToFloat(
	DWORD_PTR pParam)
{ 
	return (float)*((float *)&pParam); 
}

//----------------------------------------------------------------------------
inline BOOL EventDataIsEqual(
	const EVENT_DATA * ed1,
	const EVENT_DATA * ed2,
	const DWORD dwEventDataCompare)
{	
	if (TESTBIT(dwEventDataCompare, EDC_PARAM1_BIT) && ed1->m_Data1 != ed2->m_Data1)
	{
		return FALSE;
	}
	if (TESTBIT(dwEventDataCompare, EDC_PARAM2_BIT) && ed1->m_Data2 != ed2->m_Data2)
	{
		return FALSE;
	}
	if (TESTBIT(dwEventDataCompare, EDC_PARAM3_BIT) && ed1->m_Data3 != ed2->m_Data3)
	{
		return FALSE;
	}
	if (TESTBIT(dwEventDataCompare, EDC_PARAM4_BIT) && ed1->m_Data4 != ed2->m_Data4)
	{
		return FALSE;
	}
	if (TESTBIT(dwEventDataCompare, EDC_PARAM5_BIT) && ed1->m_Data5 != ed2->m_Data5)
	{
		return FALSE;
	}
	if (TESTBIT(dwEventDataCompare, EDC_PARAM6_BIT) && ed1->m_Data6 != ed2->m_Data6)
	{
		return FALSE;
	}

	return TRUE;		
}


#endif
