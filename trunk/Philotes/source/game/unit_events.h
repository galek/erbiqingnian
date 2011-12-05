//----------------------------------------------------------------------------
// unit_events.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNIT_EVENTS_H_
#define _UNIT_EVENTS_H_

enum UNIT_EVENT_RECORDING_METHOD
{
	UNIT_EVENT_RECORD_IGNORE = -1,
	UNIT_EVENT_RECORD_NEW_ENTRY,
	UNIT_EVENT_RECORD_COUNT_EVENTS,
	UNIT_EVENT_RECORD_SUM_DAMAGE,
	UNIT_EVENT_RECORD_SKILL_COUNT,
	UNIT_EVENT_RECORD_STAT,
	UNIT_EVENT_RECORD_ADD_SUMMARY_EVENTS,
};

struct UNIT_EVENT_TYPE_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	UNIT_EVENT_RECORDING_METHOD pRecordMethodByGenus[ NUM_GENUS ];	
};

#endif