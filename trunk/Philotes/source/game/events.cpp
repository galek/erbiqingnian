//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "events.h"
#include "dbg_file_line_table.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "game.h"
#include "prime.h"
#include "stats.h"
#include "units.h"
#include "player.h"
#include "room.h"
#if ISVERSION(SERVER_VERSION)
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#include "events.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// BUILD OPTIONS
//----------------------------------------------------------------------------
#define VALIDATE_EVENT_LISTS						0
#define MAX_NEW_ZERO_TICK_EVENTS					5000


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define	EVENT_SCHEDULE_SIZE							256
#define SCHEDULE_HASH(tick)							((tick) & (EVENT_SCHEDULE_SIZE - 1))


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct EVENT_LIST
{
	DWORD					m_tick;								// tick on which event list gets run
	EVENT_LIST *			m_next;								// doubly linked list of event lists for a tick hash (all ticks with the same hash)
	EVENT_LIST *			m_prev;
	GAME_EVENT *			m_head;								// doubly linked list of events to be run on the tick
	GAME_EVENT *			m_tail;
	unsigned int			m_count;							// number of events on the list
};

struct EVENT_SYSTEM
{
	GAME *					m_Game;
	EVENT_LIST *			m_Schedule[EVENT_SCHEDULE_SIZE];	// hash of event lists by tick
	unsigned int			m_count;							// global number of events in event system

	BOOL					m_bProcessing;						// set this flag when we're processing events
	GAME_TICK				m_tiLastProcessedTick;				// last tick we processed

	TIME					m_epsTimer;
	int						m_epsCounter;
	int						m_eps;

	EVENT_LIST *			m_GarbageEventList;					// garbage list of event lists

#if GAME_EVENTS_DEBUG		
	FP_UNIT_EVENT_TIMED *	m_pfProcessingCallback;				// current callback function we're processing
	unsigned int			m_countZeroTickEvents;				// count of zero tick events registered this frame
	DbgFileLineTable		m_DbgFlTbl;
#endif
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_EVENTS_TRACKING
#define GLOBAL_EVENTS_TRACKER_MAX_EVENTS		1024
struct GLOBAL_EVENTS_TRACKER
{
	static const unsigned int			INTERVAL = 1000 * 60;	// reset counters every minute
	struct EVENT_NAME_NODE	
	{
		const char *					m_szName;
		unsigned int					m_Index;
	};
	friend int GlobalEventNameCompare(const void *, const void *);

	volatile long						m_Count[2][GLOBAL_EVENTS_TRACKER_MAX_EVENTS];
	volatile long						m_Total[GLOBAL_EVENTS_TRACKER_MAX_EVENTS];
	const char *						m_EventName[GLOBAL_EVENTS_TRACKER_MAX_EVENTS];
	CCritSectLite						m_CS;
	EVENT_NAME_NODE						m_Events[GLOBAL_EVENTS_TRACKER_MAX_EVENTS];
	volatile long						m_NumEvents;
	DWORD								m_LastFlipTime;
	
	//------------------------------------------------------------------------
	GLOBAL_EVENTS_TRACKER(
		void)
	{
		m_CS.Init();
		m_NumEvents = 0;
		m_LastFlipTime = 0;
		for (unsigned int ii = 0; ii < GLOBAL_EVENTS_TRACKER_MAX_EVENTS; ++ii)
		{
			m_Count[0][ii] = 0;
			m_Count[1][ii] = 0;
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
		ASSERT_RETINVALID(m_NumEvents < GLOBAL_EVENTS_TRACKER_MAX_EVENTS);
		EVENT_NAME_NODE key;
		key.m_szName = name;
		EVENT_NAME_NODE * node = (EVENT_NAME_NODE *)bsearch(&key, m_Events, m_NumEvents, sizeof(EVENT_NAME_NODE), GlobalEventNameCompare);
		if (node)
		{
			return node->m_Index;
		}
		unsigned int index = m_NumEvents;
		m_Events[index].m_szName = name;
		m_Events[index].m_Index = m_NumEvents;
		m_EventName[index] = name;
		InterlockedIncrement(&m_NumEvents);
		qsort(m_Events, m_NumEvents, sizeof(EVENT_NAME_NODE), GlobalEventNameCompare);
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
	}

	//------------------------------------------------------------------------
	void Do(
		unsigned int index,
		const char * name)
	{
		ASSERT_RETURN(index < (unsigned int)m_NumEvents);
		ASSERT_RETURN(name == m_EventName[index]);
		InterlockedIncrement(&m_Count[1][index]);
	}

	//------------------------------------------------------------------------
	void Flip(
		void)
	{
		unsigned int numevents = m_NumEvents;
		for (unsigned int ii = 0; ii < numevents; ++ii)
		{
			m_Count[0][ii] = m_Count[1][ii];
			m_Count[1][ii] = 0;
		}
	}

	//------------------------------------------------------------------------
	void Update(
		void)
	{
		DWORD curtick = AppCommonGetRealTime();
		if (curtick - m_LastFlipTime < INTERVAL)
		{
			return;
		}
		m_LastFlipTime = curtick;
		Flip();
	}

	//------------------------------------------------------------------------
	void TraceAll(
		void)
	{
		TraceGameInfo("--= GLOBAL EVENT TRACKER =--");
		TraceGameInfo("%40s  %20s  %20s", "EVENT", "COUNT", "TOTAL");
		unsigned int numevents = m_NumEvents;
		for (unsigned int ii = 0; ii < numevents; ++ii)
		{
			unsigned int total = m_Total[ii];
			unsigned int count = (unsigned int)m_Count[0][ii];
			if (total > 0)
			{
				const char * name = m_EventName[ii];
				TraceGameInfo("%40s  %20d  %20d", name, count, total);
			}
		}
	}
};

//----------------------------------------------------------------------------
int GlobalEventNameCompare(
	const void * a, 
	const void * b)
{
	GLOBAL_EVENTS_TRACKER::EVENT_NAME_NODE * A = (GLOBAL_EVENTS_TRACKER::EVENT_NAME_NODE *)a;
	GLOBAL_EVENTS_TRACKER::EVENT_NAME_NODE * B = (GLOBAL_EVENTS_TRACKER::EVENT_NAME_NODE *)b;
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

GLOBAL_EVENTS_TRACKER	g_GlobalEventsTracker;

//----------------------------------------------------------------------------
void GlobalEventTrackerUpdate(
	void)
{
	g_GlobalEventsTracker.Update();
}

//----------------------------------------------------------------------------
void GlobalEventTrackerTrace(
	void)
{
	g_GlobalEventsTracker.TraceAll();
}

//----------------------------------------------------------------------------
unsigned int GlobalEventTrackerRegisterEvent(
	const char * name)
{
	return g_GlobalEventsTracker.RegisterEventName(name);
}

//----------------------------------------------------------------------------
void GlobalEventTrackerAdd(
	unsigned int index,
	const char * name)
{
	g_GlobalEventsTracker.Add(index, name);
}

//----------------------------------------------------------------------------
void GlobalEventTrackerDo(
	unsigned int index,
	const char * name)
{
	g_GlobalEventsTracker.Do(index, name);
}
#endif


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitListValidate(
	GAME_EVENT * list)
{
#if VALIDATE_EVENT_LISTS		
	if (!list)
	{
		return;
	}
	if (!list->unitnext && !list->unitprev)
	{
		return;
	}
	ASSERT_RETURN(list->unitnext && list->unitprev);

	UNIT * unit = list->unit;

	GAME_EVENT * iter = list->unitnext;
	while (iter != list)
	{
		ASSERT(iter->unit == unit);
		ASSERT(iter->unitnext != NULL); 
		ASSERT(iter->unitnext->unitprev == iter);
		ASSERT(iter->unitprev != NULL);
		ASSERT(iter->unitprev->unitnext == iter);
		iter = iter->unitnext;
	}
#else
	REF(list);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if VALIDATE_EVENT_LISTS
#define EventSystemValidate(es)			EventSystemValidateDbg(es, __FILE__, __LINE__)
static void EventSystemValidateDbg(
	EVENT_SYSTEM * es,
	const char * file,
	unsigned int line)
{
#pragma message("HINT: Compiling EventSystemValidate")
	if (!es)
	{
		return;
	}
	for (int ii = 0; ii < EVENT_SCHEDULE_SIZE; ii++)
	{
		EVENT_LIST * list = es->schedule[ii];
		while (list)
		{
			FL_ASSERT(list->tick >= GameGetTick(es->m_Game), file, line);
			if (list->next)
			{
				FL_ASSERT(list->next->prev == list, file, line);
			}

			GAME_EVENT * cur_event = list->head;
			if (!cur_event)
			{
				FL_ASSERT(list->tail == NULL, file, line);
			}
			else
			{
				FL_ASSERT(cur_event->prev == NULL, file, line);
			}

			unsigned int count = 0;
			while (cur_event)
			{
				FL_ASSERT(cur_event->list == list, file, line);
				if (cur_event->next)
				{
					FL_ASSERT(cur_event->next->prev == cur_event, file, line);
				}
				else
				{
					FL_ASSERT(list->tail == cur_event, file, line);
				}
				if (cur_event->unit)
				{
					sUnitListValidate(cur_event);
				}
				cur_event = cur_event->next;
				count++;
			}
			FL_ASSERT(count == list->count, file, line);

			list = list->next;
		}
	}
}
#else

#define EventSystemValidate(es)

#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EventSystemInit(
	GAME * game)
{
	game->pEventSystem = (EVENT_SYSTEM *)GMALLOCZ(game, sizeof(EVENT_SYSTEM));
	game->pEventSystem->m_Game = game;
	game->pEventSystem->m_tiLastProcessedTick = (DWORD)(-1);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline EVENT_LIST * GetEventList(
	GAME * game,
	DWORD tick)
{
	ASSERT_RETNULL(game);
	EVENT_SYSTEM * es = game->pEventSystem;
	ASSERT_RETNULL(es);

	EventSystemValidate(es);

	int hash = SCHEDULE_HASH(tick);

	// look for an event_list for my tick
	EVENT_LIST * list = es->m_Schedule[hash];
	EVENT_LIST * prev = NULL;
	while (list)
	{
		ASSERT(list->m_tick >= GameGetTick(game));
		if (list->m_tick >= tick)
		{
			break;
		}
		prev = list;
		list = list->m_next;
	}

	// none found so create and link in a new event list
	if (!list || list->m_tick != tick)
	{
		EVENT_LIST * newlist = NULL;
		if (es->m_GarbageEventList)
		{
			newlist = es->m_GarbageEventList;
			es->m_GarbageEventList = newlist->m_next;
		}
		else
		{
			newlist = (EVENT_LIST *)GMALLOCZ(game, sizeof(EVENT_LIST));
		}
		newlist->m_tick = tick;
		newlist->m_prev = prev;
		newlist->m_next = list;
		if (!prev)
		{
			es->m_Schedule[hash] = newlist;
		}
		else
		{
			prev->m_next = newlist;
		}
		if (list)
		{
			list->m_prev = newlist;
		}
		list = newlist;
	}
	return list;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGameEventListValidate(
	const EVENT_LIST * list)
{
#if VALIDATE_EVENT_LISTS
	unsigned int count = 0;
	ASSERT((list->m_head == NULL && list->m_tail == NULL) || (list->m_head != NULL && list->m_tail != NULL));
	GAME_EVENT * prev = NULL;
	GAME_EVENT * curr = list->m_head;
	while (curr)
	{
		prev = curr;
		curr = curr->next;
		++count;
	}
	ASSERT(list->m_tail == prev);
	ASSERT(list->m_count == count);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void AddToEventList(
	EVENT_LIST * list,
	GAME_EVENT * event)
{
	sGameEventListValidate(list);

	// add event to end of event list
	ASSERT(event->next == NULL);
	ASSERT(event->prev == NULL);
	event->prev = list->m_tail;
	if (list->m_tail)
	{
		list->m_tail->next = event;
	}
	else
	{
		list->m_head = event;
	}
	list->m_tail = event;
	list->m_count++;

	event->list = list;

	sGameEventListValidate(list);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void AddToUnitList(
	GAME_EVENT * list,
	GAME_EVENT * event)
{
	// add event to end of event list (since it's circular, that's the prev of list
	if (!list)
	{
		return;
	}
	ASSERT_RETURN(list->unitprev && list->unitprev->unitnext == list);
	ASSERT_RETURN(list->unitnext && list->unitnext->unitprev == list);
	ASSERT_RETURN(list->unit == event->unit);
	sUnitListValidate(list);

	// doubly linked circular list
	event->unitnext = list;
	event->unitprev = list->unitprev;
	list->unitprev->unitnext = event;
	list->unitprev = event;

	ASSERT(list->unitprev->unit == event->unit && list->unitnext->unit == event->unit);
	sUnitListValidate(list);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGameEventRegister(
	GAME * game,
	EVENT_SYSTEM * es,
	GAME_EVENT * event,
	GAME_TICK tick)
{
	// point back to the list i'm on
	EVENT_LIST * list = GetEventList(game, tick);
	ASSERT_RETURN(list);
	EventSystemValidate(es);
	AddToEventList(list, event);
	EventSystemValidate(es);
	es->m_count++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME_EVENT * GameEventRegisterFunc(
	GAME * game,
#if GLOBAL_EVENTS_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
	const char * file,
	int line,
#endif
	FP_UNIT_EVENT_TIMED * pfCallback,
	UNIT * unit,
	GAME_EVENT * event_list,
	const EVENT_DATA * event_data,
	int nTick)
{
	ASSERT_RETNULL(game);
	if (GameGetState(game) >= GAMESTATE_REQUEST_SHUTDOWN)
	{
		return NULL;
	}
	EVENT_SYSTEM * es = game->pEventSystem;
	ASSERT_RETNULL(es);

#if GAME_EVENTS_DEBUG
	// it's illegal to register an event from itself if the tick is 0
	FL_ASSERT_RETNULL(nTick > 0 || es->m_pfProcessingCallback != pfCallback, file, line);

	// don't allow too many events
	FL_ASSERT_RETNULL(nTick > 0 || es->m_countZeroTickEvents < MAX_NEW_ZERO_TICK_EVENTS, file, line);

	FL_ASSERT_RETNULL(nTick >= 0, file, line);
#else
	ASSERT_RETNULL(nTick >= 0);
#endif

#if ISVERSION(DEVELOPMENT) && GLOBAL_EVENTS_TRACKING
	if (unit && UnitGetRoom(unit) != NULL && RoomIsActive(UnitGetRoom(unit)) == FALSE)
	{
		TRACE_ACTIVE("ROOM [%d: %s]  UNIT [%d: %s]  CALLBACK [%s]  Register Event In Inactive Room\n", 
			RoomGetId(UnitGetRoom(unit)), RoomGetDevName(UnitGetRoom(unit)), UnitGetId(unit), UnitGetDevName(unit), szGlobalTrackingName);
	}
#endif

	EventSystemValidate(es);

	GAME_TICK game_tick = GameGetTick(game);
	GAME_TICK tick = game_tick + nTick;
	if (tick <= es->m_tiLastProcessedTick)
	{
		tick = game_tick + 1;
	}

	GAME_EVENT * event = (GAME_EVENT *)GMALLOCZ(game, sizeof(GAME_EVENT));
	ASSERT_RETNULL(event);

	event->pfCallback = pfCallback;
	event->unit = unit;
	event->m_dwGameTick = tick;
	if (event_data)
	{
		event->m_Data = *event_data;
	}

#if GAME_EVENTS_DEBUG
	event->file = file;
	event->line = line;
	es->m_DbgFlTbl.Add(file, line);
#endif
#if GLOBAL_EVENTS_TRACKING
	event->track_name = szGlobalTrackingName;
	event->track_index = idGlobalTrackingIndex;
	GlobalEventTrackerAdd(idGlobalTrackingIndex, szGlobalTrackingName);
#endif

	if (!unit || !UnitTestFlag(unit, UNITFLAG_IN_LIMBO))
	{
		sGameEventRegister(game, es, event, tick);
	}

	// link into unit event list
	AddToUnitList(event_list, event);

	EventSystemValidate(es);

	return event;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME_EVENT * GameEventRegisterFunc(
	GAME * game,
#if GLOBAL_EVENTS_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
	const char * file,
	int line,
#endif
	FP_UNIT_EVENT_TIMED * pfCallback,
	UNIT * unit,
	GAME_EVENT * event_list,
	const EVENT_DATA & event_data,
	int nTick)
{
	return GameEventRegisterFunc(game,
#if GLOBAL_EVENTS_TRACKING
		idGlobalTrackingIndex, szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
		file, line,
#endif
		pfCallback, unit, event_list, &event_data, nTick);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void RemoveFromUnitList(
	GAME_EVENT * event)
{
	ASSERT_RETURN(event);
	sUnitListValidate(event);

	ASSERT(event->unit);
	ASSERT_RETURN(event->unitnext && event->unitprev);

	GAME_EVENT * list = event->unitnext;

	ASSERT_RETURN(event->unitnext->unit == event->unit && event->unitprev->unit == event->unit);
	event->unitnext->unitprev = event->unitprev;
	event->unitprev->unitnext = event->unitnext;
	event->unitprev = NULL;
	event->unitnext = NULL;
	DBG_ASSERT(!event->unitnext && !event->unitprev);

	sUnitListValidate(list);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sRemoveFromList(
	EVENT_SYSTEM * es,
	GAME_EVENT * event)
{
	ASSERT_RETURN(es);
	ASSERT_RETURN(event);

	EVENT_LIST * list = event->list;

	sGameEventListValidate(list);

	// remove the event from the list
	if (event->prev)
	{
		event->prev->next = event->next;
	}
	else
	{
		list->m_head = event->next;
	}
	if (event->next)
	{
		event->next->prev = event->prev;
	}
	else
	{
		list->m_tail = event->prev;
	}
	event->prev = NULL;
	event->next = NULL;
	event->list = NULL;
	ASSERT(list->m_count > 0);
	list->m_count--;
	ASSERT(es->m_count > 0);
	es->m_count--;

	sGameEventListValidate(list);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void RemoveFromList(
	EVENT_SYSTEM * es,
	GAME_EVENT * event)
{
	ASSERT_RETURN(event);

	if (event->unit)
	{
		RemoveFromUnitList(event);
	}

	if (event->list)		// the only time an event has no list if is if the unit is in limbo
	{
		sRemoveFromList(es, event);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void RemoveEmptyList(
	EVENT_SYSTEM * es,
	EVENT_LIST * list)
{
	EventSystemValidate(es);

	// if the list is now empty, remove the list
	if (!list->m_head)
	{
		if (list->m_prev)
		{
			list->m_prev->m_next = list->m_next;
		}
		else
		{
			es->m_Schedule[SCHEDULE_HASH(list->m_tick)] = list->m_next;
		}
		if (list->m_next)
		{
			list->m_next->m_prev = list->m_prev;
		}

		memclear(list, sizeof(EVENT_LIST));
		list->m_next = es->m_GarbageEventList;
		es->m_GarbageEventList = list;
	}

	EventSystemValidate(es);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sGameEventUnregister(
	GAME * game,
	GAME_EVENT * event,
	BOOL bDeleteUnit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(event);

	EVENT_SYSTEM * es = game->pEventSystem;

	ASSERT_RETURN(es);

	EventSystemValidate(es);

	EVENT_LIST * list = event->list;

	// if the event is in the curtick, null the callback function to indicate a "do nothing"
	if (es->m_bProcessing && list && list->m_tick == GameGetTick(game))
	{
		event->pfCallback = NULL;	// just mark for deletion if we're processing this list
	}
	else
	{
		RemoveFromList(es, event);  // this frees both the event and the list
#if GAME_EVENTS_DEBUG
		es->m_DbgFlTbl.Sub(event->file, event->line);
#endif
		GFREE(es->m_Game, event);
		if (list && !list->m_head)
		{
			RemoveEmptyList(es, list);
		}
	}

	EventSystemValidate(es);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameEventUnregister(
	GAME * game,
	GAME_EVENT * event)
{
	ASSERT_RETURN(game && event);
	sGameEventUnregister(game, event, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameEventUnregisterAll(
	GAME * game,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const EVENT_DATA * check_data)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unitlist);
	sUnitListValidate(unitlist);
	GAME_EVENT * event = unitlist->unitnext;
	while (event != unitlist)
	{
		GAME_EVENT * next = event->unitnext;

		ASSERT(event->unitnext && event->unitnext->unit == event->unit);
		ASSERT(event->unitprev && event->unitprev->unit == event->unit);
		if (!pfCallback || event->pfCallback == pfCallback)
		{
			if (!pfUnregisterCheck || pfUnregisterCheck(game, event->unit, event->m_Data, check_data))
			{
				sGameEventUnregister(game, event, !pfCallback && !pfUnregisterCheck);
			}
		}

		event = next;
	}
	sUnitListValidate(unitlist);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameEventUnregisterAll(
	GAME * game,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const EVENT_DATA & check_data)
{
	GameEventUnregisterAll(game, pfCallback, unitlist, pfUnregisterCheck, &check_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameEventExists(
	GAME * game,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const EVENT_DATA * check_data)
{
	ASSERT_RETFALSE(game);
	GAME_EVENT * event = unitlist->unitnext;
	while (event != unitlist)
	{
		GAME_EVENT * next = event->unitnext;

		if (!pfCallback || event->pfCallback == pfCallback)
		{
			if (!pfUnregisterCheck || pfUnregisterCheck(game, event->unit, event->m_Data, check_data))
			{
				return TRUE;
			}
		}

		event = next;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameEventsProcess(
	GAME * game)
{
	ASSERT_RETURN(game);
	EVENT_SYSTEM * es = game->pEventSystem;
	ASSERT_RETURN(es);

	EventSystemValidate(es);

	int hash = SCHEDULE_HASH(GameGetTick(game));

	// look for the list for the given tick in the event system
	EVENT_LIST * list = es->m_Schedule[hash];
	while (list)
	{
		if (list->m_tick == GameGetTick(game))
		{
			break;
		}
		list = list->m_next;
	}
	if (!list)
	{
		es->m_tiLastProcessedTick = GameGetTick(game);
		return;		// done, no events registered for this tick
	}

	unsigned int count = 0;
	unsigned int initial_count = list->m_count;

#if GAME_EVENTS_DEBUG
	es->m_countZeroTickEvents = 0;
#endif

	es->m_bProcessing = TRUE;
	while (list->m_head)
	{
		sGameEventListValidate(list);

		GAME_EVENT * event = list->m_head;
		RemoveFromList(es, event);
		DBG_ASSERT(event->list == NULL && event->next == NULL && event->prev == NULL && event->unitnext == NULL && event->unitprev == NULL);

#if GAME_EVENTS_DEBUG
		es->m_DbgFlTbl.Sub(event->file, event->line);
#endif

		if (event->pfCallback)
		{
#if GAME_EVENTS_DEBUG
			es->m_pfProcessingCallback = event->pfCallback;
#endif
#if GLOBAL_EVENTS_TRACKING
			GlobalEventTrackerDo(event->track_index, event->track_name);
#if ISVERSION(DEVELOPMENT)
			if (event->unit && UnitGetRoom(event->unit) != NULL && RoomIsActive(UnitGetRoom(event->unit)) == FALSE)
			{
				TRACE_ACTIVE("ROOM [%d: %s]  UNIT [%d: %s]  CALLBACK [%s]  Event callback in Inactive Room\n", 
					RoomGetId(UnitGetRoom(event->unit)), RoomGetDevName(UnitGetRoom(event->unit)), UnitGetId(event->unit), UnitGetDevName(event->unit), event->track_name);
			}
#endif
#endif
			event->pfCallback(game, event->unit, event->m_Data);
		}
#if GAME_EVENTS_DEBUG
		else
		{
			es->m_pfProcessingCallback = NULL;
		}
#endif
		++count;
#if ISVERSION(SERVER_VERSION)
		GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
		if (pContext) PERF_ADD64(pContext->m_pPerfInstance, GameServer, GameServerGameEventRate, 1);
#endif

		GFREE(game, event);
	}
	es->m_bProcessing = FALSE;
#if GAME_EVENTS_DEBUG
	es->m_pfProcessingCallback = NULL;
	es->m_countZeroTickEvents = 0;
#endif

	ASSERT(list->m_count == 0 && count >= initial_count);
	ASSERT(list->m_head == NULL && list->m_tail == NULL);

	RemoveEmptyList(es, list);

	es->m_tiLastProcessedTick = GameGetTick(game);

	TIME time = AppCommonGetCurTime();
	if (time - es->m_epsCounter > MSECS_PER_SEC)
	{
		es->m_epsTimer = time;
		es->m_eps = es->m_epsCounter;
		es->m_epsCounter = 0;
	}
	es->m_epsCounter += count;

	EventSystemValidate(es);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EventSystemFree(
	GAME * game)
{
	EVENT_SYSTEM * es = game->pEventSystem;
	if (!es)
	{
		return 0;
	}

	int freecount = 0;

	// go through and delete all lists
	for (int ii = 0; ii < EVENT_SCHEDULE_SIZE; ii++)
	{
		EVENT_LIST* list = es->m_Schedule[ii];
		while (list)
		{
			while (list->m_head)
			{
				GAME_EVENT * event = list->m_head;
				RemoveFromList(es, event);
				GFREE(game, event);
				freecount++;
			}	
			RemoveEmptyList(es, list);
			list = es->m_Schedule[ii];
		}
	}

	EVENT_LIST * list = es->m_GarbageEventList;
	while (list)
	{
		EVENT_LIST * next = list->m_next;
		GFREE(game, list);
		list = next;
	}

#if GAME_EVENTS_DEBUG
	es->m_DbgFlTbl.CleanUp();
#endif

	GFREE(game, es);
	game->pEventSystem = NULL;
	
	return freecount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EventSystemGetStats(
	GAME * game,
	int * pnTotalEvents,
	int * pnEventsPerSecond)
{
	EVENT_SYSTEM * es = game->pEventSystem;
	if (!es)
	{
		return;
	}
	*pnTotalEvents = es->m_count;
	*pnEventsPerSecond = es->m_eps;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GAME_EVENTS_DEBUG		
void EventSystemGetDebugData(
	GAME * game,
	EVENT_SYSTEM_DEBUG * debug_data)
{
	ASSERT_RETURN(game);
	EVENT_SYSTEM * es = game->pEventSystem;
	ASSERT_RETURN(es);
	debug_data->num_events = es->m_count;
	debug_data->fltbl = &(es->m_DbgFlTbl);
}
#endif


//----------------------------------------------------------------------------
// Store the time
// Iterate the unit list, and remove every event from the event list
//----------------------------------------------------------------------------
void UnitEventEnterLimbo(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(unit);

	EVENT_SYSTEM * es = game->pEventSystem;
	EventSystemValidate(es);

	UnitSetStat(unit, STATS_LIMBO_TIME, GameGetTick(game));

	GAME_EVENT * list = &unit->events;
	sUnitListValidate(list);

	GAME_EVENT * event = list->unitnext;
	while (event != list)
	{
		if (event->list)
		{
			sRemoveFromList(es, event);
		}
		event->list = NULL;
		event->prev = NULL;
		event->next = NULL;
		event = event->unitnext;
	}

	EventSystemValidate(es);
}


//----------------------------------------------------------------------------
// Get the time
// Iterate the unit list, and add every event back to the event list, fixing
//    the time
//----------------------------------------------------------------------------
void UnitEventExitLimbo(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	if (UnitTestFlag(unit, UNITFLAG_FREED))
	{
		return;
	}
	EVENT_SYSTEM * es = game->pEventSystem;
	ASSERT_RETURN(es);
	EventSystemValidate(es);

#if ISVERSION(DEVELOPMENT)
	if (UnitGetRoom(unit) != NULL)
	{
		ASSERT(RoomIsActive(UnitGetRoom(unit)));
	}
#endif

	GAME_TICK tick = UnitGetStat(unit, STATS_LIMBO_TIME);
	GAME_TICK offset = GameGetTick(game) - tick;

	GAME_EVENT * list = &unit->events;
	GAME_EVENT * event = list->unitnext;
	while (event != list)
	{
		event->m_dwGameTick += offset;
		// for now, remove the event from the event list before adding
		ASSERT(event->prev == NULL);
		ASSERT(event->next == NULL);
		sGameEventRegister(game, es, event, event->m_dwGameTick);
		event = event->unitnext;
	}
	EventSystemValidate(es);
}


//----------------------------------------------------------------------------
// test code
//----------------------------------------------------------------------------
struct test_struct
{
	DWORD index;
	DWORD tick;
};

BOOL test_callBack(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA & event_data)
{
//	test_struct * testdata = (test_struct *)(event_data.m_Data1);

	return TRUE;
}

void test_EventSystem(
	GAME * game)
{
	enum
	{
		test_size = 100000,
		test_period = 4096,
	};
	test_struct testdata[test_size];

	srand(0);

	EventSystemInit(game);

	// register a bunch of events
	for (int ii = 0; ii < test_size; ii++)
	{
		testdata[ii].index = ii;
		testdata[ii].tick = RandGetNum(game->rand, test_period);
		EVENT_DATA event_data((DWORD_PTR)(testdata + ii));
		GameEventRegister(game, test_callBack, NULL, NULL, &event_data, testdata[ii].tick);
	}

	// process
	GAME_TICK oldtick = GameGetTick(game);
	game->tiGameTick = 0;
	for (int jj = 0; jj < test_period; jj++)
	{
		GameEventsProcess(game);
		game->tiGameTick++;
	}

	int freecount = EventSystemFree(game);
	trace("freed: %d\n", freecount);

	game->tiGameTick = oldtick;
	srand((unsigned)time(NULL));
}
