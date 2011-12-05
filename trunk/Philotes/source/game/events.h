//----------------------------------------------------------------------------
//
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
#ifndef _EVENTS_H_
#define _EVENTS_H_


//----------------------------------------------------------------------------
// DEBUG DEFINES
//----------------------------------------------------------------------------
#define GAME_EVENTS_DEBUG				ISVERSION(DEVELOPMENT)
#define GLOBAL_EVENTS_TRACKING			(ISVERSION(DEVELOPMENT) || ISVERSION(SERVER_VERSION))


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef BOOL FP_UNIT_EVENT_TIMED(struct GAME * game, struct UNIT * unit, const struct EVENT_DATA & event_data);
typedef BOOL FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK(struct GAME * game, struct UNIT * unit, const struct EVENT_DATA & event_data, const struct EVENT_DATA * check_data);

struct EVENT_LIST;


//----------------------------------------------------------------------------
// TEST CODE
//----------------------------------------------------------------------------
void test_EventSystem(
	GAME * game);


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
#include "event_data.h"

struct GAME_EVENT
{
	GAME_EVENT *			next;
	GAME_EVENT *			prev;

	UNIT *					unit;
	GAME_EVENT *			unitprev;
	GAME_EVENT *			unitnext;

	FP_UNIT_EVENT_TIMED *	pfCallback;
	EVENT_LIST *			list;

#if GAME_EVENTS_DEBUG
	const char *			file;
#endif
#if GLOBAL_EVENTS_TRACKING
	const char *			track_name;
#endif

	EVENT_DATA				m_Data;
	DWORD					m_dwGameTick;

#if GAME_EVENTS_DEBUG
	int						line;
#endif
#if GLOBAL_EVENTS_TRACKING
	unsigned int			track_index;
#endif
};


//----------------------------------------------------------------------------
// EVENT SYSTEM FUNCTIONS
//----------------------------------------------------------------------------
void EventSystemInit(
	GAME * game);

int EventSystemFree(
	GAME * game);

#if GAME_EVENTS_DEBUG
 #if GLOBAL_EVENTS_TRACKING
  #define GameEventRegister(game, callback, unit, event_list, event_data, tick)				{ static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#callback); GameEventRegisterFunc(game, UIDEN(GET__), #callback, __FILE__, __LINE__, callback, unit, event_list, event_data, tick); }
  #define GameEventRegisterRet(ret, game, callback, unit, event_list, event_data, tick)		{ static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#callback); ret = GameEventRegisterFunc(game, UIDEN(GET__), #callback, __FILE__, __LINE__, callback, unit, event_list, event_data, tick); }
 #else
  #define GameEventRegister(game, callback, unit, event_list, event_data, tick)				{ GameEventRegisterFunc(game, __FILE__, __LINE__, callback, unit, event_list, event_data, tick); }
  #define GameEventRegisterRet(ret, game, callback, unit, event_list, event_data, tick)		{ ret = GameEventRegisterFunc(game, __FILE__, __LINE__, callback, unit, event_list, event_data, tick); }
 #endif
#else
 #if GLOBAL_EVENTS_TRACKING
  #define GameEventRegister(game, callback, unit, event_list, event_data, tick)				{ static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#callback); GameEventRegisterFunc(game, UIDEN(GET__), #callback, callback, unit, event_list, event_data, tick); }
  #define GameEventRegisterRet(ret, game, callback, unit, event_list, event_data, tick)		{ static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#callback); ret = GameEventRegisterFunc(game, UIDEN(GET__), #callback, callback, unit, event_list, event_data, tick); }
 #else
   #define GameEventRegister(game, callback, unit, event_list, event_data, tick)			{ GameEventRegisterFunc(game, callback, unit, event_list, event_data, tick); }
   #define GameEventRegisterRet(ret, game, callback, unit, event_list, event_data, tick)	{ ret = GameEventRegisterFunc(game, callback, unit, event_list, event_data, tick); }
 #endif
#endif

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
	int nTick);

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
	int nTick);

void GameEventUnregister(
	GAME * game,
	GAME_EVENT * event);

void GameEventUnregisterAll(
	GAME * game,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck = NULL,
	const EVENT_DATA * check_data = NULL);

void GameEventUnregisterAll(
	GAME * pGame,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const EVENT_DATA & check_data);

BOOL GameEventExists(
	GAME * game,
	FP_UNIT_EVENT_TIMED * pfCallback,
	GAME_EVENT * unitlist,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck = NULL,
	const EVENT_DATA * check_data = NULL);

void GameEventsProcess(
	GAME * game);

void EventSystemGetStats(
	GAME * game,
	int * pnTotalEvents,
	int * pnEventsPerSecond);

#if GAME_EVENTS_DEBUG
void EventSystemGetDebugData(
	GAME * game,
	struct EVENT_SYSTEM_DEBUG * debug_data);
#endif

void UnitEventEnterLimbo(
	GAME * game,
	UNIT * unit);

void UnitEventExitLimbo(
	GAME * game,
	UNIT * unit);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_EVENTS_TRACKING
void GlobalEventTrackerUpdate(
	void);

void GlobalEventTrackerTrace(
	void);

unsigned int GlobalEventTrackerRegisterEvent(
	const char * name);
#else
#define GlobalEventTrackerUpdate()
#define GlobalEventTrackerTrace()
#endif


#endif // _EVENTS_H_