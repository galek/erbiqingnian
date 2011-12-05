//----------------------------------------------------------------------------
// Prime v2.0
// unitevent.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "units.h"
#if ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#include "unitevent.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define ABSURD_EVENT_HANDLER_COUNT			0
#endif


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct EVENT_HANDLER
{
	EVENT_HANDLER *							m_pNextHandler;
	EVENT_HANDLER *							m_pNextPending;
	FP_UNIT_EVENT_HANDLER					m_fpHandler;

	EVENT_DATA								m_Data;

#if ISVERSION(DEVELOPMENT)
	const char *							file;
#endif

#if GLOBAL_EVENT_HANDLER_TRACKING
	const char *							track_name;
#endif

#if ISVERSION(DEVELOPMENT)
	unsigned int							line;
#endif

#if GLOBAL_EVENT_HANDLER_TRACKING
	unsigned int							track_index;
#endif

	UNIT_EVENT								m_eEvent;

	DWORD									m_dwId;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_EVENT_HANDLER_TRACKING
#define GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS			1024
struct GLOBAL_EVENT_HANDLER_TRACKER
{
	static const unsigned int			INTERVAL = 1000 * 60;	// reset counters every minute
	struct EVENT_NAME_NODE	
	{
		const char *					m_szName;
		unsigned int					m_Index;
	};
	friend int GlobalEventHandlerNameCompare(const void *, const void *);

	volatile long						m_Count[GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS];
	volatile long						m_Total[GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS];
	const char *						m_EventName[GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS];
	CCritSectLite						m_CS;
	EVENT_NAME_NODE						m_Events[GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS];
	volatile long						m_NumEvents;
	
	//------------------------------------------------------------------------
	GLOBAL_EVENT_HANDLER_TRACKER(
		void)
	{
		m_CS.Init();
		m_NumEvents = 0;
		for (unsigned int ii = 0; ii < GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS; ++ii)
		{
			m_Count[ii] = 0;
			m_EventName[ii] = 0;
			m_Events[ii].m_szName = NULL;
			m_Events[ii].m_Index = 0;
		}
	}

	//------------------------------------------------------------------------
	unsigned int RegisterEventName(
		const char * name)
	{
		CSLAutoLock autolock(&m_CS);
		ASSERT_RETINVALID(m_NumEvents < GLOBAL_EVENT_HANDLER_TRACKER_MAX_EVENTS);
		EVENT_NAME_NODE key;
		key.m_szName = name;
		EVENT_NAME_NODE * node = (EVENT_NAME_NODE *)bsearch(&key, m_Events, m_NumEvents, sizeof(EVENT_NAME_NODE), GlobalEventHandlerNameCompare);
		if (node)
		{
			return node->m_Index;
		}
		unsigned int index = m_NumEvents;
		m_Events[index].m_szName = name;
		m_Events[index].m_Index = m_NumEvents;
		m_EventName[index] = name;
		InterlockedIncrement(&m_NumEvents);
		qsort(m_Events, m_NumEvents, sizeof(EVENT_NAME_NODE), GlobalEventHandlerNameCompare);
		return index;
	}

	//------------------------------------------------------------------------
	void Add(
		unsigned int index,
		const char * name)
	{
		ASSERT_RETURN(index < (unsigned int)m_NumEvents);
		ASSERT_RETURN(name == m_EventName[index]);
		InterlockedIncrement(&m_Total[index]);
		InterlockedIncrement(&m_Count[index]);
	}

	//------------------------------------------------------------------------
	void Remove(
		unsigned int index,
		const char * name)
	{
		ASSERT_RETURN(index < (unsigned int)m_NumEvents);
		ASSERT_RETURN(name == m_EventName[index]);
		InterlockedDecrement(&m_Count[index]);
	}

	//------------------------------------------------------------------------
	void Update(
		void)
	{
	}

	//------------------------------------------------------------------------
	void TraceAll(
		void)
	{
		TraceGameInfo("--= GLOBAL EVENT HANDLER TRACKER =--");
		TraceGameInfo("%40s  %20s  %20s", "EVENT HANDLER", "COUNT", "TOTAL");
		unsigned int numevents = m_NumEvents;
		for (unsigned int ii = 0; ii < numevents; ++ii)
		{
			unsigned int total = (unsigned int)m_Total[ii];
			unsigned int count = (unsigned int)m_Count[ii];
			if (total > 0)
			{
				const char * name = m_EventName[ii];
				TraceGameInfo("%40s  %20d  %20d", name, count, total);
			}
		}
	}
};

//----------------------------------------------------------------------------
int GlobalEventHandlerNameCompare(
	const void * a, 
	const void * b)
{
	GLOBAL_EVENT_HANDLER_TRACKER::EVENT_NAME_NODE * A = (GLOBAL_EVENT_HANDLER_TRACKER::EVENT_NAME_NODE *)a;
	GLOBAL_EVENT_HANDLER_TRACKER::EVENT_NAME_NODE * B = (GLOBAL_EVENT_HANDLER_TRACKER::EVENT_NAME_NODE *)b;
	if (A->m_szName < B->m_szName)
	{
		return -1;
	}
	else if (A->m_szName > B->m_szName)
	{
		return 1;
	}
	return 0;
}

GLOBAL_EVENT_HANDLER_TRACKER	g_GlobalEventHandlerTracker;

//----------------------------------------------------------------------------
void GlobalEventHandlerTrackerUpdate(
	void)
{
	g_GlobalEventHandlerTracker.Update();
}

//----------------------------------------------------------------------------
void GlobalEventHandlerTrackerTrace(
	void)
{
	g_GlobalEventHandlerTracker.TraceAll();
}

//----------------------------------------------------------------------------
unsigned int GlobalEventHandlerTrackerRegisterEvent(
	const char * name)
{
	return g_GlobalEventHandlerTracker.RegisterEventName(name);
}

//----------------------------------------------------------------------------
void GlobalEventHandlerTrackerAdd(
	unsigned int index,
	const char * name)
{
	g_GlobalEventHandlerTracker.Add(index, name);
}

//----------------------------------------------------------------------------
void GlobalEventHandlerTrackerRemove(
	unsigned int index,
	const char * name)
{
	g_GlobalEventHandlerTracker.Remove(index, name);
}
#endif


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sTrackingRemoveEventHandler(
	GAME * game,
	UNIT * unit,
	EVENT_HANDLER * handler)
{
	REF(game);
	REF(unit);
	REF(handler);
#if ISVERSION(DEVELOPMENT)
	unit->m_nEventHandlerCount--;
#endif
#if ISVERSION(SERVER_VERSION)
	GameServerContext * server_context = game->pServerContext;
	if (server_context) 
	{
		PERF_ADD64(server_context->m_pPerfInstance, GameServer, GameServerGameEventHandlers, -1);
	}
#endif
#if GLOBAL_EVENT_HANDLER_TRACKING
	GlobalEventHandlerTrackerRemove(handler->track_index, handler->track_name);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitFreeEventHandlers(
	GAME * game,
	UNIT * unit)
{
	DBG_ASSERT(unit->m_nEventHandlerCount == 0);
	ASSERT_RETURN(unit->m_nEventHandlerIterRef <= 0);
	ASSERT_RETURN(unit->m_pEventHandlerPending == NULL);

	for (int ii = 0; ii < NUM_UNIT_EVENTS; ii++)
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[ii];
		ASSERT(curr == NULL);
		while (curr)
		{
			EVENT_HANDLER * next = curr->m_pNextHandler;
			sTrackingRemoveEventHandler(game, unit, curr);
			GFREE(game, curr);
			curr = next;
		}
		unit->m_pEventHandlers[ii] = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitUnregisterAllEventHandlers(
	GAME * game,
	UNIT * unit)
{
	// need to unregister any events on the pending list
	EVENT_HANDLER * curr = NULL;
	EVENT_HANDLER * next = unit->m_pEventHandlerPending;
	while ((curr = next) != NULL)
	{
		next = curr->m_pNextPending;
		curr->m_fpHandler = NULL;
		curr->m_dwId = INVALID_ID;
	}

	for (int ii = 0; ii < NUM_UNIT_EVENTS; ii++)
	{
		if (unit->m_nEventHandlerIterRef <= 0)
		{
			EVENT_HANDLER * curr = NULL;
			EVENT_HANDLER * next = unit->m_pEventHandlers[ii];
			while ((curr = next) != NULL)
			{
				next = curr->m_pNextHandler;
				sTrackingRemoveEventHandler(game, unit, curr);
				GFREE(game, curr);
			}
			unit->m_pEventHandlers[ii] = NULL;
		}
		else
		{
			EVENT_HANDLER * curr = NULL;
			EVENT_HANDLER * next = unit->m_pEventHandlers[ii];
			while ((curr = next) != NULL)
			{
				next = curr->m_pNextHandler;
				if (curr->m_fpHandler != NULL || curr->m_dwId != INVALID_ID)
				{
					curr->m_fpHandler = NULL;
					curr->m_dwId = INVALID_ID;
					curr->m_pNextPending = unit->m_pEventHandlerPending;
					unit->m_pEventHandlerPending = curr;
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void HandleAbsurdEventHandlerCount(
	GAME * game,
	UNIT * unit)
{
	if (game->m_idUnitWithAbsurdNumberOfEventHandlers == INVALID_ID ||
		AppCommonGetCurTime() - game->m_tiUnitWithAbsurdNumberOfEventHandlersTime > 10000)
	{
		game->m_idUnitWithAbsurdNumberOfEventHandlers = UnitGetId(unit);
		game->m_tiUnitWithAbsurdNumberOfEventHandlersTime = AppCommonGetCurTime();
	}
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD sUnitRegisterEventHandler(
	GAME * game,
#if GLOBAL_EVENT_HANDLER_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if ISVERSION(DEVELOPMENT)
	const char * file,
	unsigned int line,
#endif
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler,
	const EVENT_DATA * data)
{
	ASSERT_RETINVALID(game && unit);
	ASSERT_RETINVALID(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);
	ASSERT_RETINVALID(fpHandler != NULL);
	
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT)		// don't assert on live servers to reduce server spam
	IF_NOT_RETINVALID(!UnitTestFlag(unit, UNITFLAG_FREED));
#else
	ASSERT_RETINVALID(!UnitTestFlag(unit, UNITFLAG_FREED));
#endif

	// allocate & setup event handler
	EVENT_HANDLER* handler = (EVENT_HANDLER *)GMALLOCZ(game, sizeof(EVENT_HANDLER));
	handler->m_eEvent = eEvent;
	handler->m_fpHandler = fpHandler;
	handler->m_dwId = game->m_idEventHandler++;
	if (data)
	{
		handler->m_Data = *data;
	}
	if (handler->m_dwId == INVALID_ID)
	{
		handler->m_dwId = game->m_idEventHandler++;
	}

	// if we're not processing a triggered event, add the new handler directly to the appropriate list
	if (unit->m_nEventHandlerIterRef <= 0)
	{
		// yes, this is N, but we want to save memory because of the number of events and the size of the unit struct, and N should always be small
		EVENT_HANDLER * prev = NULL;
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			prev = curr;
			curr = curr->m_pNextHandler;
		}
		if (!prev)
		{
			unit->m_pEventHandlers[eEvent] = handler;
		}
		else
		{
			prev->m_pNextHandler = handler;
		}
	}
	else	// put it on the pending list to add when iterref hits 0
	{
		handler->m_pNextPending = unit->m_pEventHandlerPending;
		unit->m_pEventHandlerPending = handler;
	}

#if ISVERSION(DEVELOPMENT)
	handler->file = file;
	handler->line = line;
	unit->m_nEventHandlerCount++;
	if (unit->m_nEventHandlerCount > ABSURD_EVENT_HANDLER_COUNT)
	{
		HandleAbsurdEventHandlerCount(game, unit);
	}
#endif
#if ISVERSION(SERVER_VERSION)
	GameServerContext * server_context = game->pServerContext;
	if (server_context) 
	{
		PERF_ADD64(server_context->m_pPerfInstance, GameServer, GameServerGameEventHandlers, 1);			
	}
#endif
#if GLOBAL_EVENT_HANDLER_TRACKING
	handler->track_name = szGlobalTrackingName;
	handler->track_index = idGlobalTrackingIndex;
	GlobalEventHandlerTrackerAdd(idGlobalTrackingIndex, szGlobalTrackingName);
#endif

	return handler->m_dwId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD UnitRegisterEventHandlerFunc(
	GAME * game,
#if GLOBAL_EVENT_HANDLER_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if ISVERSION(DEVELOPMENT)
	const char * file,
	unsigned int line,
#endif
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler,
	const EVENT_DATA * data)
{
	return sUnitRegisterEventHandler(game, 
#if GLOBAL_EVENT_HANDLER_TRACKING
		idGlobalTrackingIndex, szGlobalTrackingName,
#endif
#if ISVERSION(DEVELOPMENT)
		file, line,
#endif
		unit, eEvent, fpHandler, data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD UnitRegisterEventHandlerFunc(
	GAME * game,
#if GLOBAL_EVENT_HANDLER_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if ISVERSION(DEVELOPMENT)
	const char * file,
	unsigned int line,
#endif
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler,
	const EVENT_DATA & data)
{
	return sUnitRegisterEventHandler(game, 
#if GLOBAL_EVENT_HANDLER_TRACKING
		idGlobalTrackingIndex, szGlobalTrackingName,
#endif
#if ISVERSION(DEVELOPMENT)
		file, line,
#endif
		unit, eEvent, fpHandler, &data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUnregisterEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	DWORD id)
{
	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);
	ASSERT_RETFALSE(id != INVALID_ID);
 
	EVENT_HANDLER * prev = NULL;
	EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
	while (curr)
	{
		if (curr->m_dwId == id)
		{
			ASSERT(curr->m_eEvent == eEvent);
			// if we're not processing a triggered event, take it off the list immediately
			if (unit->m_nEventHandlerIterRef <= 0)
			{
				if (!prev)
				{
					unit->m_pEventHandlers[eEvent] = curr->m_pNextHandler;
				}
				else
				{
					prev->m_pNextHandler = curr->m_pNextHandler;
				}
				sTrackingRemoveEventHandler(game, unit, curr);
				GFREE(game, curr);
			}
			else	// put it on the pending list for deletion when iter ref hits 0
			{
				curr->m_fpHandler = NULL;
				curr->m_dwId = INVALID_ID;
				curr->m_pNextPending = unit->m_pEventHandlerPending;
				unit->m_pEventHandlerPending = curr;
			}
			return TRUE;
		}
		prev = curr;
		curr = curr->m_pNextHandler;
	}

	// need to search the pending list too!
	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * prev = NULL;
		EVENT_HANDLER * curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_dwId == id)
			{
				ASSERT(curr->m_eEvent == eEvent);
				curr->m_fpHandler = NULL;
				curr->m_dwId = INVALID_ID;
				return TRUE;
			}
			prev = curr;
			curr = curr->m_pNextPending;
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUnregisterEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler)
{
	BOOL found = FALSE;

	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);
	ASSERT_RETFALSE(fpHandler);

	EVENT_HANDLER * prev = NULL;
	EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
	while (curr)
	{
		if (curr->m_fpHandler == fpHandler)
		{
			ASSERT(curr->m_eEvent == eEvent);
			if (unit->m_nEventHandlerIterRef <= 0)
			{
				if (!prev)
				{
					unit->m_pEventHandlers[eEvent] = curr->m_pNextHandler;
				}
				else
				{
					prev->m_pNextHandler = curr->m_pNextHandler;
				}

				EVENT_HANDLER * next = curr->m_pNextHandler;
				sTrackingRemoveEventHandler(game, unit, curr);
				GFREE(game, curr);
				curr = next;
			}
			else	// put it on the pending list for deletion when iter ref hits 0
			{
				curr->m_fpHandler = NULL;
				curr->m_dwId = INVALID_ID;
				curr->m_pNextPending = unit->m_pEventHandlerPending;
				unit->m_pEventHandlerPending = curr;
				prev = curr;
				curr = curr->m_pNextHandler;
			}
			found = TRUE;
		} 
		else 
		{
			prev = curr;
			curr = curr->m_pNextHandler;
		}
	}

	// need to search the pending list too!
	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * prev = NULL;
		EVENT_HANDLER * curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler)
			{
				ASSERT(curr->m_eEvent == eEvent);
				curr->m_fpHandler = NULL;
				curr->m_dwId = INVALID_ID;
				found = TRUE;
			}
			prev = curr;
			curr = curr->m_pNextPending;
		}
	}

	return found;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent)
{
	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);

	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler != NULL)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}

		curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_eEvent == eEvent)
			{
				return TRUE;
			}
			curr = curr->m_pNextPending;
		}
	}
	else
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler != NULL)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	DWORD id)
{
	if (id == INVALID_ID)
	{
		return UnitHasEventHandler(game, unit, eEvent);
	}
	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);

	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_dwId == id && curr->m_fpHandler != NULL)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}

		curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_dwId == id && curr->m_eEvent == eEvent)
			{
				return TRUE;
			}
			curr = curr->m_pNextPending;
		}
	}
	else
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_dwId == id && curr->m_fpHandler != NULL)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler)
{
	if (fpHandler == NULL)
	{
		return UnitHasEventHandler(game, unit, eEvent);
	}
	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);

	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}

		curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler && curr->m_eEvent == eEvent)
			{
				return TRUE;
			}
			curr = curr->m_pNextPending;
		}
	}
	else
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler)
			{
				return TRUE;
			}
			curr = curr->m_pNextHandler;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitEventFind(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler,
	EVENT_DATA * data,
	DWORD dwEventDataCompare)
{
	ASSERT_RETFALSE(game && unit);
	ASSERT_RETFALSE(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);

	if (unit->m_nEventHandlerIterRef > 0)
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler &&
				EventDataIsEqual(data, &curr->m_Data, dwEventDataCompare))
			{
				return curr->m_dwId;
			}
			curr = curr->m_pNextHandler;
		}

		curr = unit->m_pEventHandlerPending;
		while (curr)
		{
			if (curr->m_eEvent == eEvent &&
				curr->m_fpHandler == fpHandler && 
				EventDataIsEqual(data, &curr->m_Data, dwEventDataCompare))
			{
				return curr->m_dwId;
			}
			curr = curr->m_pNextPending;
		}
	}
	else
	{
		EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
		while (curr)
		{
			if (curr->m_fpHandler == fpHandler &&
				EventDataIsEqual(data, &curr->m_Data, dwEventDataCompare))
			{
				return curr->m_dwId;
			}
			curr = curr->m_pNextHandler;
		}
	}
	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameCheckNestedTriggeredEvents(
	GAME * game,
	UNIT_EVENT eEvent)
{
	static const unsigned int MAX_NESTED_EVENTS_OF_EACH_TYPE = 32;

	return (game->m_NestedEventsCounter[eEvent] < MAX_NESTED_EVENTS_OF_EACH_TYPE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameIncrementNestedTriggeredEvents(
	GAME * game,
	UNIT_EVENT eEvent)
{
	game->m_NestedEventsCounter[eEvent]++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameDecrementNestedTriggeredEvents(
	GAME * game,
	UNIT_EVENT eEvent)
{
	ASSERT_RETURN(game->m_NestedEventsCounter[eEvent] > 0);
	game->m_NestedEventsCounter[eEvent]--;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitTriggerEvent(
	GAME * game,
	UNIT_EVENT eEvent,
	UNIT * unit,
	UNIT * otherunit,
	void * data)
{
	ASSERT_RETURN(game && unit);
	ASSERT_RETURN(eEvent >= 0 && eEvent < NUM_UNIT_EVENTS);

	ASSERT_RETURN(GameCheckNestedTriggeredEvents(game, eEvent));

	GameIncrementNestedTriggeredEvents(game, eEvent);

	unit->m_nEventHandlerIterRef++;

	EVENT_HANDLER * curr = unit->m_pEventHandlers[eEvent];
	while (curr)
	{
		if (curr->m_fpHandler)
		{
			ASSERT(curr->m_eEvent == eEvent);
			curr->m_fpHandler(game, unit, otherunit, &curr->m_Data, data, curr->m_dwId);
		}
		curr = curr->m_pNextHandler;
	}

	unit->m_nEventHandlerIterRef--;
	ASSERT(unit->m_nEventHandlerIterRef >= 0);

	GameDecrementNestedTriggeredEvents(game, eEvent);

	// handle pending operations
	if (unit->m_nEventHandlerIterRef <= 0)
	{
		EVENT_HANDLER * handler = unit->m_pEventHandlerPending;
		while (handler)
		{
			EVENT_HANDLER * next = handler->m_pNextPending;
			if (handler->m_fpHandler)
			{
				// put it on the proper event list
				EVENT_HANDLER * prev = NULL;
				EVENT_HANDLER * iter = unit->m_pEventHandlers[handler->m_eEvent];
				while (iter)
				{
					prev = iter;
					iter = iter->m_pNextHandler;
				}
				if (!prev)
				{
					unit->m_pEventHandlers[handler->m_eEvent] = handler;
				}
				else
				{
					prev->m_pNextHandler = handler;
				}
				handler->m_pNextPending = NULL;
			}
			else
			{
				// delete the handler
				BOOL fFound = FALSE;
				EVENT_HANDLER * prev = NULL;
				EVENT_HANDLER * iter = unit->m_pEventHandlers[handler->m_eEvent];
				while (iter)
				{
					if (iter == handler)
					{
						if (!prev)
						{
							unit->m_pEventHandlers[handler->m_eEvent] = iter->m_pNextHandler;
						}
						else
						{
							prev->m_pNextHandler = iter->m_pNextHandler;
						}
						fFound = TRUE;
						break;
					}
					prev = iter;
					iter = iter->m_pNextHandler;
				}
				sTrackingRemoveEventHandler(game, unit, handler);
				GFREE(game, handler);
			}
			handler = next;
		}
		unit->m_pEventHandlerPending =  NULL;
	}
}



