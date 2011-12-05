//----------------------------------------------------------------------------
// unitevent.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_UNITEVENT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _UNITEVENT_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _UNITEVENTS_HDR_
#define _UNITEVENTS_HDR_
#include "..\data_common\excel\unitevents_hdr.h"
#endif


//----------------------------------------------------------------------------
// DEBUG DEFINE
//----------------------------------------------------------------------------
#define GLOBAL_EVENT_HANDLER_TRACKING			(ISVERSION(DEVELOPMENT) || ISVERSION(SERVER_VERSION))


//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef void (*FP_UNIT_EVENT_HANDLER)(GAME * game, UNIT * unit, UNIT * otherunit, struct EVENT_DATA * event_data, void * data, DWORD dwId);


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void UnitFreeEventHandlers(
	GAME * game,
	UNIT * unit);

void UnitUnregisterAllEventHandlers(
	GAME * game,
	UNIT * unit);

#if ISVERSION(DEVELOPMENT)
 #if GLOBAL_EVENT_HANDLER_TRACKING
   #define UnitRegisterEventHandler(g, u, e, fp, d)				{ static unsigned int UIDEN(GET__) = GlobalEventHandlerTrackerRegisterEvent(#fp); UnitRegisterEventHandlerFunc(g, UIDEN(GET__), #fp, __FILE__, __LINE__, u, e, fp, d); }
   #define UnitRegisterEventHandlerRet(r, g, u, e, fp, d)		{ static unsigned int UIDEN(GET__) = GlobalEventHandlerTrackerRegisterEvent(#fp); r = UnitRegisterEventHandlerFunc(g, UIDEN(GET__), #fp, __FILE__, __LINE__, u, e, fp, d); }
 #else
   #define UnitRegisterEventHandler(g, u, e, fp, d)				{ UnitRegisterEventHandlerFunc(g, __FILE__, __LINE__, u, e, fp, d); }
   #define UnitRegisterEventHandlerRet(r, g, u, e, fp, d)		{ r = UnitRegisterEventHandlerFunc(g, __FILE__, __LINE__, u, e, fp, d); }
 #endif
#else
 #if GLOBAL_EVENT_HANDLER_TRACKING
   #define UnitRegisterEventHandler(g, u, e, fp, d)				{ static unsigned int UIDEN(GET__) = GlobalEventHandlerTrackerRegisterEvent(#fp); UnitRegisterEventHandlerFunc(g, UIDEN(GET__), #fp, u, e, fp, d); }
   #define UnitRegisterEventHandlerRet(r, g, u, e, fp, d)		{ static unsigned int UIDEN(GET__) = GlobalEventHandlerTrackerRegisterEvent(#fp); r = UnitRegisterEventHandlerFunc(g, UIDEN(GET__), #fp, u, e, fp, d); }
 #else
   #define UnitRegisterEventHandler(g, u, e, fp, d)				{ UnitRegisterEventHandlerFunc(g, u, e, fp, d); }
   #define UnitRegisterEventHandlerRet(r, g, u, e, fp, d)		{ r = UnitRegisterEventHandlerFunc(g, u, e, fp, d); }
 #endif
#endif

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
	const EVENT_DATA * event_data);

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
	const EVENT_DATA & event_data);

BOOL UnitUnregisterEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	DWORD id);

BOOL UnitUnregisterEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler);

BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent);

BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	DWORD id);

BOOL UnitHasEventHandler(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler);

int UnitEventFind(
	GAME * game,
	UNIT * unit,
	UNIT_EVENT eEvent,
	FP_UNIT_EVENT_HANDLER fpHandler,
	EVENT_DATA * data,
	DWORD dwEventDataCompare);	// see EVENT_DATA_COMPARE enum
	
void UnitTriggerEvent(
	GAME * game,
	UNIT_EVENT eEvent,
	UNIT * unit,
	UNIT * otherunit,
	void * data);

void UnitsAllTriggerEvent(
	GAME * game,
	UNIT_EVENT eEvent,
	UNIT * otherunit,
	void * data);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_EVENT_HANDLER_TRACKING
void GlobalEventHandlerTrackerUpdate(
	void);

void GlobalEventHandlerTrackerTrace(
	void);

unsigned int GlobalEventHandlerTrackerRegisterEvent(
	const char * name);
#else
#define GlobalEventHandlerTrackerUpdate()
#define GlobalEventHandlerTrackerTrace()
#endif


#endif // _UNITEVENT_H_