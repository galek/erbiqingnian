//----------------------------------------------------------------------------
// uix_scheduler.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "uix_scheduler.h"
#include "perfhier.h"
#include "dbg_file_line_table.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define UIX_EVENT_POOL_SIZE			1024			// size of event pool
#define UIX_EVENT_HASH_SIZE			512				// size of event hash
#define UIX_EVENT_RESOLUTION		32				// bin size in msecs
#define UIX_SCHEDULE_BINS			256				// number of bins


// #define _UI_SCHEDULER_TRACE

//----------------------------------------------------------------------------
// MAIN OBJECT
//----------------------------------------------------------------------------
class UI_SCHEDULER		*g_pScheduler;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int GetScheduleBin(
	TIME time)
{
	return (int)((time/UIX_EVENT_RESOLUTION) % UIX_SCHEDULE_BINS);
}


//----------------------------------------------------------------------------
// STRUCT
//----------------------------------------------------------------------------
struct UIX_EVENT
{
	DWORD			m_dwTicket;
	UIX_EVENT*		m_pHashNext;
	UIX_EVENT*		m_pHashPrev;

	UIX_EVENT*		m_pNext;
	UIX_EVENT*		m_pPrev;

	TIME			m_tiTime;
	FP_CEVENT		m_fpCallback;
	CEVENT_DATA		m_Data;

#ifdef _DEBUG
	const char*		file;
	int				line;
#endif
};

struct UIX_EVENT_POOL
{
	UIX_EVENT		m_Pool[UIX_EVENT_POOL_SIZE];
	UIX_EVENT_POOL*	m_pNext;
};


//----------------------------------------------------------------------------
// UI_SCHEDULER
//----------------------------------------------------------------------------
class UI_SCHEDULER
{
protected:
	UIX_EVENT_POOL* m_pEventPoolFirst;
	UIX_EVENT_POOL* m_pEventPoolLast;
	int				m_nFirstUnused;
	UIX_EVENT*		m_pRecycle;
	DWORD			m_dwCurrentTicket;
	int				m_nCount;
	int				m_nImmediate;
	BOOL			m_bProcessingImmediateList;
	UIX_EVENT*		m_pNextEventToProcess;

	UIX_EVENT*		m_pEventHash[UIX_EVENT_HASH_SIZE];	// track outstanding tickets

	TIME			m_tiLastRuntime;

	UIX_EVENT*		m_pImmediateListFirst;				// list of events to process next
	UIX_EVENT*		m_pImmediateListLast;

	UIX_EVENT*		m_pImmediateWaitFirst;				// list of immediate events to delay to next frame
	UIX_EVENT*		m_pImmediateWaitLast;

	UIX_EVENT*		m_pSchedule[UIX_SCHEDULE_BINS];		// list of events to process per timeslice
#ifdef _DEBUG
	DbgFileLineTable	dbg_fl_tbl;
#endif

	void RecycleEvent(
		UIX_EVENT* event)
	{
		ASSERT(event);
		event->m_fpCallback = NULL;
		if (event->m_pPrev)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("RecycleEvent: connect %d next %d", event->m_pPrev->m_dwTicket, event->m_pNext ? event->m_pNext->m_dwTicket : -1);
#endif
			event->m_pPrev->m_pNext = event->m_pNext;
		}
		if (event->m_pNext)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("RecycleEvent: connect %d prev %d", event->m_pNext->m_dwTicket, event->m_pPrev ? event->m_pPrev->m_dwTicket : -1);
#endif
			event->m_pNext->m_pPrev = event->m_pPrev;
		}
		event->m_pPrev = NULL;
		event->m_pNext = m_pRecycle;
		m_pRecycle = event;
	}
	void AddEventToHash(
		UIX_EVENT* event)
	{
		unsigned int key = event->m_dwTicket % UIX_EVENT_HASH_SIZE;
		event->m_pHashNext = m_pEventHash[key];
		if (event->m_pHashNext)
		{
			event->m_pHashNext->m_pHashPrev = event;
		}
		m_pEventHash[key] = event;
	}
	void RemoveEventFromHash(
		UIX_EVENT* event)
	{
		if (event->m_pHashPrev)
		{
			event->m_pHashPrev->m_pHashNext = event->m_pHashNext;
		}
		else
		{
			unsigned int key = event->m_dwTicket % UIX_EVENT_HASH_SIZE;
			m_pEventHash[key] = event->m_pHashNext;
		}
		if (event->m_pHashNext)
		{
			event->m_pHashNext->m_pHashPrev = event->m_pHashPrev;
		}
		event->m_pHashPrev = NULL;
		event->m_pHashNext = NULL;
	}
	UIX_EVENT* GetEmptyEvent(
		void)
	{
		if (m_pRecycle)
		{
			UIX_EVENT* event = m_pRecycle;
			m_pRecycle = event->m_pNext;
			
			memclear(event, sizeof(UIX_EVENT));
			event->m_dwTicket = ++m_dwCurrentTicket;
			AddEventToHash(event);
			return event;
		}
		// allocate a new event pool if neccessary
		if (!m_pEventPoolLast || m_nFirstUnused >= UIX_EVENT_POOL_SIZE)
		{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			UIX_EVENT_POOL* pool = (UIX_EVENT_POOL*)MALLOC(g_StaticAllocator, sizeof(UIX_EVENT_POOL));
#else		
			UIX_EVENT_POOL* pool = (UIX_EVENT_POOL*)MALLOC(NULL, sizeof(UIX_EVENT_POOL));
#endif
			pool->m_pNext = NULL;
			if (m_pEventPoolLast)
			{
				m_pEventPoolLast->m_pNext = pool;
			}
			else
			{
				m_pEventPoolFirst = pool;
			}
			m_nFirstUnused = 0;
			m_pEventPoolLast = pool;
		}

		UIX_EVENT* event = m_pEventPoolLast->m_Pool + m_nFirstUnused;
		m_nFirstUnused++;
		memclear(event, sizeof(UIX_EVENT));
		event->m_dwTicket = ++m_dwCurrentTicket;
		AddEventToHash(event);
		return event;
	}
	UIX_EVENT* GetEventByTicket(
		DWORD ticket)
	{
		unsigned int key = ticket % UIX_EVENT_HASH_SIZE;
		UIX_EVENT* cur = m_pEventHash[key];
		while (cur)
		{
			if (cur->m_dwTicket == ticket)
			{
				RemoveEventFromHash(cur);
				return cur;
			}
			cur = cur->m_pHashNext;
		}
		return NULL;
	}
	void EventListDebug(
		void)
	{
#ifdef _UI_SCHEDULER_TRACE
		if (m_pImmediateListFirst || m_pImmediateListLast)
		{
			ASSERT(m_pImmediateListFirst && m_pImmediateListLast);
			UIX_EVENT* curr = m_pImmediateListFirst;
			UIX_EVENT* prev = NULL;
			while (curr)
			{
				ASSERT(curr->m_pPrev == prev);
				UIX_EVENT* next = curr->m_pNext;
				if (!next)
				{
					ASSERT(curr == m_pImmediateListLast);
				}
				prev = curr;
				curr = next;
			}

			curr = m_pImmediateListLast;
			UIX_EVENT* next = NULL;
			while (curr)
			{
				ASSERT(curr->m_pNext == next);
				UIX_EVENT* prev = curr->m_pPrev;
				if (!prev)
				{
					ASSERT(curr == m_pImmediateListFirst);
				}
				next = curr;
				curr = prev;
			}
		}
		if (m_pImmediateWaitFirst || m_pImmediateWaitLast)
		{
			ASSERT(m_pImmediateWaitFirst && m_pImmediateWaitLast);
			UIX_EVENT* prev = NULL;
			UIX_EVENT* curr = m_pImmediateWaitFirst;
			while (curr)
			{
				ASSERT(curr->m_pPrev == prev);
				UIX_EVENT* next = curr->m_pNext;
				if (!next)
				{
					ASSERT(curr == m_pImmediateWaitLast);
				}
				prev = curr;
				curr = next;
			}

			curr = m_pImmediateWaitLast;
			UIX_EVENT* next = NULL;
			while (curr)
			{
				ASSERT(curr->m_pNext == next);
				UIX_EVENT* prev = curr->m_pPrev;
				if (!prev)
				{
					ASSERT(curr == m_pImmediateWaitFirst);
				}
				next = curr;
				curr = prev;
			}
		}
#endif
	}
	void EventListTrace(
		void)
	{
#ifdef _UI_SCHEDULER_TRACE
		int tick = 0;
		if (AppGetCltGame())
		{
			tick = GameGetTick(AppGetCltGame());
		}
		LogText(LOG_FILE, "[%d] Immediate List: ", tick);
		UIX_EVENT* curr = m_pImmediateListFirst;
		while (curr)
		{
			LogText(LOG_FILE, "%d, ", curr->m_dwTicket);
			curr = curr->m_pNext;
		}
		LogText(LOG_FILE, "\n");
		LogText(LOG_FILE, "[%d] Immediate Wait: ", tick);
		curr = m_pImmediateWaitFirst;
		while (curr)
		{
			LogText(LOG_FILE, "%d, ", curr->m_dwTicket);
			curr = curr->m_pNext;
		}
		LogText(LOG_FILE, "\n");
#endif
	}
	void RemoveEventFromImmediateListOrWait(
		UIX_EVENT* event)
	{
		EventListDebug();
		if (event->m_pPrev)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("RemoveEvent (%d): connect %d next %d", event->m_dwTicket, event->m_pPrev->m_dwTicket, event->m_pNext ? event->m_pNext->m_dwTicket : -1);
#endif
			event->m_pPrev->m_pNext = event->m_pNext;
		}
		else
		{
			if (m_pImmediateListFirst == event)
			{
#ifdef _UI_SCHEDULER_TRACE
				LogMessage("RemoveEvent (%d): set m_pImmListFirst %d", event->m_dwTicket, event->m_pNext ? event->m_pNext->m_dwTicket : -1);
#endif
				m_pImmediateListFirst = event->m_pNext;
			}
			else if (m_pImmediateWaitFirst == event)
			{
#ifdef _UI_SCHEDULER_TRACE
				LogMessage("RemoveEvent (%d): set m_pImmWiatFirst %d", event->m_dwTicket, event->m_pNext ? event->m_pNext->m_dwTicket : -1);
#endif
				m_pImmediateWaitFirst = event->m_pNext;
			}
		}
		if (event->m_pNext)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("RemoveEvent (%d): connect %d prev %d", event->m_dwTicket, event->m_pNext->m_dwTicket, event->m_pPrev ? event->m_pPrev->m_dwTicket : -1);
#endif
			event->m_pNext->m_pPrev = event->m_pPrev;
		}
		else
		{
			if (m_pImmediateListLast == event)
			{
#ifdef _UI_SCHEDULER_TRACE
				LogMessage("RemoveEvent (%d): set m_pImmListLast %d", event->m_dwTicket, event->m_pPrev ? event->m_pPrev->m_dwTicket : -1);
#endif
				m_pImmediateListLast = event->m_pPrev;
			}
			else if (m_pImmediateWaitLast == event)
			{
#ifdef _UI_SCHEDULER_TRACE
				LogMessage("RemoveEvent (%d): set m_pImmWaitLast %d", event->m_dwTicket, event->m_pPrev ? event->m_pPrev->m_dwTicket : -1);
#endif
				m_pImmediateWaitLast = event->m_pPrev;
			}
		}
		EventListDebug();
		m_nImmediate--;
	}
	void RemoveEventFromSchedule(
		UIX_EVENT* event)
	{
		EventListDebug();
		if (event->m_pPrev)
		{
			event->m_pPrev->m_pNext = event->m_pNext;
		}
		else
		{
			int bin = GetScheduleBin(event->m_tiTime);

			m_pSchedule[bin] = event->m_pNext;
		}
		if (event->m_pNext)
		{
			event->m_pNext->m_pPrev = event->m_pPrev;
		}
		EventListDebug();
		m_nCount--;
	}
	void AddEventToImmediateList(
		UIX_EVENT* event)
	{
		EventListDebug();
		event->m_pPrev = m_pImmediateListLast;
		if (event->m_pPrev)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("AddEventList: connect %d next %d", event->m_pPrev->m_dwTicket, event->m_dwTicket);
#endif
			event->m_pPrev->m_pNext = event;
		}
		else
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("AddEventList: set ImmListFirst %d", event->m_dwTicket);
#endif
			m_pImmediateListFirst = event;
		}
		m_pImmediateListLast = event;
		EventListDebug();
		m_nImmediate++;
	}
	void AddEventToImmediateWait(
		UIX_EVENT* event)
	{
		EventListDebug();
		event->m_pPrev = m_pImmediateWaitLast;
		if (event->m_pPrev)
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("AddEventWait (%d): connect %d next %d", event->m_dwTicket, event->m_pPrev->m_dwTicket, event->m_dwTicket);
#endif
			event->m_pPrev->m_pNext = event;
		}
		else
		{
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("AddEventWait (%d): set ImmWaitFirst %d", event->m_dwTicket, event->m_dwTicket);
#endif
			m_pImmediateWaitFirst = event;
		}
#ifdef _UI_SCHEDULER_TRACE
		LogMessage("AddEventWait (%d): set m_pImmWaitLast %d", event->m_dwTicket, event->m_dwTicket);
#endif
		m_pImmediateWaitLast = event;
		EventListDebug();
		m_nImmediate++;
	}
	void AddEventToSchedule(
		UIX_EVENT* event)
	{
		EventListDebug();

		int bin = GetScheduleBin(event->m_tiTime);

		UIX_EVENT* cur = m_pSchedule[bin];
		UIX_EVENT* prev = NULL;

		while (cur)
		{
			if (cur->m_tiTime > event->m_tiTime)
			{
				break;
			}
			prev = cur;
			cur = cur->m_pNext;;
		}

		if (prev)
		{	
			event->m_pNext = prev->m_pNext;
			event->m_pPrev = prev;
			prev->m_pNext = event;
		}
		else
		{
			event->m_pNext = m_pSchedule[bin];
			m_pSchedule[bin] = event;
		}
		if (event->m_pNext)
		{
			event->m_pNext->m_pPrev = event;
		}
		m_nCount++;

		EventListDebug();
	}

public:
	void Zero(
		void)
	{
		m_pEventPoolFirst = m_pEventPoolLast = NULL;
		m_pRecycle = NULL;
		m_tiLastRuntime = 0;
		m_dwCurrentTicket = 0;
		m_nCount = 0;
		m_nImmediate = 0;
		memclear(m_pEventHash, sizeof(UIX_EVENT*) * UIX_EVENT_HASH_SIZE);
		memclear(m_pSchedule, sizeof(UIX_EVENT*) * UIX_SCHEDULE_BINS);

		m_pImmediateListFirst = NULL;
		m_pImmediateListLast = NULL;

		m_pImmediateWaitFirst = NULL;
		m_pImmediateWaitLast = NULL;

		m_bProcessingImmediateList = FALSE;
		m_pNextEventToProcess = NULL;

#ifdef _DEBUG
		memclear(&dbg_fl_tbl, sizeof(dbg_fl_tbl));
#endif
	}
	UI_SCHEDULER(
		void)
	{
		Zero();
		m_tiLastRuntime = AppCommonGetCurTime();
	}
	~UI_SCHEDULER(
		void)
	{
		Free();
	}
	void Free(
		void)
	{
		UIX_EVENT_POOL* pool = m_pEventPoolFirst;
		while (pool)
		{
			UIX_EVENT_POOL* next = pool->m_pNext;
			
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			FREE(g_StaticAllocator, pool);
#else			
			FREE(NULL, pool);
#endif
			
			pool = next;
		}

#ifdef _DEBUG
		dbg_fl_tbl.CleanUp();
#endif
		Zero();
	}

#ifdef _DEBUG
	DWORD Schedule(
		FP_CEVENT callback,
		const CEVENT_DATA& data,
		const char* file,
		int line)
#else
	DWORD Schedule(
		FP_CEVENT callback,
		const CEVENT_DATA& data)
#endif
	{
		UIX_EVENT* event = GetEmptyEvent();
		event->m_tiTime = 0;
		event->m_fpCallback = callback;
		memcpy(&event->m_Data, &data, sizeof(CEVENT_DATA));

		if (m_bProcessingImmediateList)
		{
			AddEventToImmediateWait(event);
		}
		else
		{
			AddEventToImmediateList(event);
		}

#ifdef _DEBUG
		event->file = file;
		event->line = line;
		dbg_fl_tbl.Add(file, line);
#endif
#ifdef _UI_SCHEDULER_TRACE
		LogMessage("schedule: %d (%I64u)", event->m_dwTicket, event->m_tiTime);
		EventListTrace();
#endif

		return event->m_dwTicket;
	}

#ifdef _DEBUG
	DWORD Schedule(
		TIME & time,								// time 0 => put on immediate list
		FP_CEVENT callback,
		const CEVENT_DATA& data,
		const char* file,
		int line)
#else
	DWORD Schedule(
		TIME & time,								// time 0 => put on immediate list
		FP_CEVENT callback,
		const CEVENT_DATA& data)
#endif
	{
		if (time <= m_tiLastRuntime)
		{
			time = 0;
#ifdef _DEBUG
			return Schedule(callback, data, file, line);
#else
			return Schedule(callback, data);
#endif
		}
		UIX_EVENT* event = GetEmptyEvent();
		event->m_tiTime = time;
		event->m_fpCallback = callback;
		memcpy(&event->m_Data, &data, sizeof(CEVENT_DATA));

		AddEventToSchedule(event);

#ifdef _DEBUG
		event->file = file;
		event->line = line;
		dbg_fl_tbl.Add(file, line);
#endif

		return event->m_dwTicket;
	}

	BOOL IsValidTicket(
		DWORD ticket)
	{
		unsigned int key = ticket % UIX_EVENT_HASH_SIZE;
		UIX_EVENT* cur = m_pEventHash[key];
		while (cur)
		{
			if (cur->m_dwTicket == ticket)
			{
				return TRUE;
			}
			cur = cur->m_pHashNext;
		}
		return FALSE;
	}

	BOOL GetEventData(
		DWORD ticket,
		CEVENT_DATA& eventData)
	{
		UIX_EVENT* event = GetEventByTicket(ticket);
		if (event==NULL)
			return FALSE;

		memcpy(&eventData, &event->m_Data, sizeof eventData);
		return TRUE;
	}

	BOOL Cancel(
		DWORD ticket)
	{
		UIX_EVENT* event = GetEventByTicket(ticket);
		if (!event)
		{
			return FALSE;
		}

#ifdef _UI_SCHEDULER_TRACE
		LogMessage("canceled: %d (%d)", ticket, event->m_tiTime);
		EventListTrace();
#endif

		RemoveEventFromHash(event);

		// remove event from immediate list or schedule table
		if (m_pNextEventToProcess == event)
		{
			m_pNextEventToProcess = event->m_pNext;
		}
		if (event->m_tiTime == 0)
		{
			RemoveEventFromImmediateListOrWait(event);
		}
		else
		{
			RemoveEventFromSchedule(event);
		}
		event->m_pPrev = NULL;
		event->m_pNext = NULL;

#ifdef _DEBUG
		dbg_fl_tbl.Sub(event->file, event->line);
#endif

		RecycleEvent(event);

		return TRUE;
	}

	void Process(
		GAME* game)
	{
		PERF( UI_SCHED_PROCESS );

		TIME last_runtime = m_tiLastRuntime;
		TIME cur_time = AppCommonGetCurTime();
		m_tiLastRuntime = cur_time;

		// run events on immediate list
		m_bProcessingImmediateList = TRUE;
		ASSERT(m_pImmediateWaitFirst == NULL && m_pImmediateWaitLast == NULL);
		UIX_EVENT* cur = m_pImmediateListFirst;
		m_pImmediateListFirst = m_pImmediateListLast = NULL;

		while (cur)
		{
			HITCOUNT( UI_SCHED_PROC_IMMED );

			m_pNextEventToProcess = cur->m_pNext;

			CEVENT_DATA data;
			memcpy(&data, &cur->m_Data, sizeof(CEVENT_DATA));
			FP_CEVENT callback = cur->m_fpCallback;

			RemoveEventFromHash(cur);
			m_nImmediate--;
#ifdef _DEBUG
			dbg_fl_tbl.Sub(cur->file, cur->line);
#endif
#ifdef _UI_SCHEDULER_TRACE
			LogMessage("execute: %d (%d)", cur->m_dwTicket, cur->m_tiTime);
#endif
			RecycleEvent(cur);

			ASSERT(callback);
			callback(game, data, cur->m_dwTicket);

			cur = m_pNextEventToProcess;
		}
		m_bProcessingImmediateList = FALSE;
		ASSERT(m_pImmediateListFirst == NULL && m_pImmediateListLast == NULL);
		m_pImmediateListFirst = m_pImmediateWaitFirst;
		m_pImmediateListLast = m_pImmediateWaitLast;
#ifdef _UI_SCHEDULER_TRACE
		LogMessage("set ImmListFirst: %d  ImmListLast: %d  ImmWaitFirst: 0  ImmWaitLast: 0", m_pImmediateWaitFirst ? m_pImmediateWaitFirst->m_dwTicket : -1, m_pImmediateWaitLast ? m_pImmediateWaitLast->m_dwTicket : -1);
#endif
		m_pImmediateWaitFirst = m_pImmediateWaitLast = NULL;

		// run events from last_runtime to cur_time
		TIME run_time = (last_runtime & (~(UIX_EVENT_RESOLUTION - 1))) + (UIX_EVENT_RESOLUTION - 1);
		if (cur_time < run_time)
		{
			run_time = cur_time;
		}
		int bin = GetScheduleBin(last_runtime);

		while (run_time <= cur_time)
		{
			// process any events scheduled for < run_time from the bin
			while (m_pSchedule[bin])
			{
				HITCOUNT( UI_SCHED_PROC_TIMED );

				UIX_EVENT* cur = m_pSchedule[bin];
				if (cur->m_tiTime > run_time)
				{
					break;
				}
				m_pSchedule[bin] = cur->m_pNext;
				if (cur->m_pNext)
				{
					cur->m_pNext->m_pPrev = NULL;
				}

#ifdef _UI_SCHEDULER_TRACE
				LogMessage("process: %d (%I64u)", cur->m_dwTicket, cur->m_tiTime);
				EventListTrace();
#endif

				CEVENT_DATA data;
				memcpy(&data, &cur->m_Data, sizeof(CEVENT_DATA));
				FP_CEVENT callback = cur->m_fpCallback;

				m_nCount--;
#ifdef _DEBUG
				dbg_fl_tbl.Sub(cur->file, cur->line);
#endif
				RemoveEventFromHash(cur);
				RecycleEvent(cur);
					
				ASSERT(callback);
				callback(game, data, cur->m_dwTicket);
			}

			if (run_time >= cur_time)
			{
				break;
			}
			run_time += UIX_EVENT_RESOLUTION;
			if (cur_time < run_time)
			{
				run_time = cur_time;
			}
			bin = (bin + 1) % UIX_SCHEDULE_BINS;
		}
	}

	TIME GetLastRunTime(
		void)
	{
		return m_tiLastRuntime;
	}

#ifdef _DEBUG
	void GetDebugData(
		EVENT_SYSTEM_DEBUG* debug_data)
	{
		debug_data->num_events = m_nCount;
		debug_data->num_immediate = m_nImmediate;
		debug_data->fltbl = &dbg_fl_tbl;
	}
#endif
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CSchedulerInit(
	void)
{
	delete g_pScheduler;
	g_pScheduler = new UI_SCHEDULER;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CSchedulerFree(
	void)
{
	delete g_pScheduler;
	g_pScheduler = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
DWORD CSchedulerRegisterEventImmDbg(
	FP_CEVENT callback,
	const CEVENT_DATA& data,
	const char* file,
	int line)
#else
DWORD CSchedulerRegisterEventImm(
	FP_CEVENT callback,
	const CEVENT_DATA& data)
#endif
{
	if (!g_pScheduler)
	{
		return 0;
	}
#ifdef _DEBUG
	return g_pScheduler->Schedule(callback, data, file, line);
#else
	return g_pScheduler->Schedule(callback, data);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
DWORD CSchedulerRegisterEventDbg(
	TIME time,
	FP_CEVENT callback,
	const CEVENT_DATA& data,
	const char* file,
	int line)
#else
DWORD CSchedulerRegisterEvent(
	TIME time,
	FP_CEVENT callback,
	const CEVENT_DATA& data)
#endif
{
	if (!g_pScheduler)
	{
		return 0;
	}
#ifdef _DEBUG
	return g_pScheduler->Schedule(time, callback, data, file, line);
#else
	return g_pScheduler->Schedule(time, callback, data);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CSchedulerCancelEvent(
	DWORD& ticket)
{
	ASSERT(g_pScheduler);
	BOOL bReturn = g_pScheduler->Cancel(ticket);
	if (bReturn)
	{
		ticket = INVALID_ID;
	}

	return bReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CSchedulerIsValidTicket(
	DWORD ticket)
{
	ASSERT(g_pScheduler);
	return g_pScheduler->IsValidTicket(ticket);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CSchedulerGetEventData(
	DWORD ticket,
	CEVENT_DATA& eventData)
{
	ASSERT_RETVAL(g_pScheduler, FALSE);
	return g_pScheduler->GetEventData(ticket, eventData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CSchedulerProcess(
	GAME* game)
{
	if (!g_pScheduler)
	{
		return;
	}
	g_pScheduler->Process(game);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TIME CSchedulerGetTime(
	void)
{
	if (!g_pScheduler)
	{
		return TIME_ZERO;
	}
	return g_pScheduler->GetLastRunTime();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
void CSchedulerGetDebugData(
	GAME* game,
	EVENT_SYSTEM_DEBUG* debug_data)
{
	if (!g_pScheduler)
	{
		return;
	}
	g_pScheduler->GetDebugData(debug_data);
}
#endif
