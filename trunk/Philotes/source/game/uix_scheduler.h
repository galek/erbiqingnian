//----------------------------------------------------------------------------
// UI_SCHEDULER.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UI_SCHEDULER_H_
#define _UI_SCHEDULER_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define SCHEDULER_TIME_PER_SECOND 1000


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct CEVENT_DATA
{
	DWORD_PTR	m_Data1;
	DWORD_PTR	m_Data2;
	DWORD_PTR	m_Data3;
	DWORD_PTR	m_Data4;

	CEVENT_DATA() : m_Data1(NULL), m_Data2(NULL), m_Data3(NULL), m_Data4(NULL) {};
	CEVENT_DATA(DWORD_PTR data1) : m_Data1(data1), m_Data2(NULL), m_Data3(NULL), m_Data4(NULL) {};
	CEVENT_DATA(DWORD_PTR data1, DWORD_PTR data2) : m_Data1(data1), m_Data2(data2), m_Data3(NULL), m_Data4(NULL) {};
	CEVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(NULL) {};
	CEVENT_DATA(DWORD_PTR data1, DWORD_PTR data2, DWORD_PTR data3, DWORD_PTR data4) : m_Data1(data1), m_Data2(data2), m_Data3(data3), m_Data4(data4) {};
};

//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef void (*FP_CEVENT)(struct GAME* game, const struct CEVENT_DATA& event_data, DWORD dwTicket);


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void CSchedulerInit(
	void);

void CSchedulerFree(
	void);

void CSchedulerProcess(
	GAME* game);

#ifdef _DEBUG
#define CSchedulerRegisterEventImm(fp, d)		CSchedulerRegisterEventImmDbg(fp, d, #fp, __LINE__)
DWORD CSchedulerRegisterEventImmDbg(
	FP_CEVENT callback,
	const CEVENT_DATA& data,
	const char* file,
	int line);
#else
#define CSchedulerRegisterEventI(fp, d)			CSchedulerRegisterEvent(fp, d)
DWORD CSchedulerRegisterEventImm(
	FP_CEVENT callback,
	const CEVENT_DATA& data);
#endif

#ifdef _DEBUG
#define CSchedulerRegisterEvent(t, fp, d)		CSchedulerRegisterEventDbg(t, fp, d, #fp, __LINE__)
DWORD CSchedulerRegisterEventDbg(
	struct TIME time,
	FP_CEVENT callback,
	const CEVENT_DATA& data,
	const char* file,
	int line);
#else
DWORD CSchedulerRegisterEvent(
	struct TIME time,
	FP_CEVENT callback,
	const CEVENT_DATA& data);
#endif

BOOL CSchedulerCancelEvent(
	DWORD& ticket);

BOOL CSchedulerIsValidTicket(
	DWORD ticket);

BOOL CSchedulerGetEventData(
	DWORD ticket,
	CEVENT_DATA& eventData);

TIME CSchedulerGetTime(
	void);

#ifdef _DEBUG
void CSchedulerGetDebugData(
	struct GAME* game,
	struct EVENT_SYSTEM_DEBUG* debug_data);
#endif


#endif // _UI_SCHEDULER_H_
