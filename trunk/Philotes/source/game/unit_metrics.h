//----------------------------------------------------------------------------
// unit_metrics.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNIT_METRICS_H_
#define _UNIT_METRICS_H_

struct UNIT_EVENT_RECORD
{
	struct UNIT_INFO
	{
		int nGenus;
		char szName[ MAX_XML_STRING_LENGTH ];
		WORD wCode;
		DWORD dwSecondsPlayed;
		int nLevel;
		PGUID Guid;
	};
	UNIT_INFO tActor;
	UNIT_INFO tActorOwner;
	UNIT_INFO tTarget;

	UNIT_EVENT eEvent; // this is handy for searching but is not saved
	char szEventName[ MAX_XML_STRING_LENGTH ]; // this is saved
	int nEventCount; // sometimes we are counting how many times this event occured
	int nDamageTotal;

	char szParam[ MAX_XML_STRING_LENGTH ];

	char szLevel[ MAX_XML_STRING_LENGTH ];

	BOOL bHardcore;
	BOOL bElite;
};

struct UNIT_RECORD_DEFINITION
{
	DEFINITION_HEADER tHeader;

	BUCKET_ARRAY<UNIT_EVENT_RECORD,24> pEventRecords;
	int nEventRecordCount;
	int nEventRecordAllocated;
	int nSaveToken;
	char szBuildVersion[ MAX_XML_STRING_LENGTH ];
	char szSaveTime[ MAX_XML_STRING_LENGTH ];
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UnitMetricsInit( 
	UNIT * pUnit );

void UnitMetricsSystemInit(
	GAME * pGame );
void UnitMetricsSystemClose(
	GAME * pGame );
void UnitMetricsSystemSave(
	GAME * pGame );

void s_UnitTrackLevelUp(
	UNIT * pUnit );
void s_UnitTrackPlayerDeath(
	UNIT * pPlayer,
	UNIT * pKiller );

#endif