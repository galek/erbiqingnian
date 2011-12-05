//----------------------------------------------------------------------------
// unit_metrics.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "unit_priv.h"
#include "unit_metrics.h"
#include "unit_events.h"
#include "definition_priv.h"
#include "level.h"
#include "skills.h"
#include "version.h"
#include "markup.h"
#include "filepaths.h"
#include "player.h"
#include "inventory.h"
#include "combat.h"
#include "items.h"
#include "monsters.h"

#if ISVERSION(SERVER_VERSION)
#include "GameEventClass_PETSdef.h"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_RECORD_DEFINITION * sGetRecordDef(
	GAME * pGame )
{
	return pGame ? pGame->m_UnitRecording : NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCleanStringForXML(
	char * pszString )
{
	PStrReplaceChars( pszString, '\'', ' ' );
	PStrReplaceChars( pszString, '\"', ' ' );
}

//----------------------------------------------------------------------------
// if this turns out to be slow, we should cache the index of the event in the event handler's EVENT_DATA after we find it the first time.
// (Unregister the event and re-register it with the index that we find here.  That way we only need to find it once per event entry.)
// Also, we should flush/publish the events more frequently if the list is getting too long.
//----------------------------------------------------------------------------
#if !ISVERSION(RETAIL_VERSION)
static int sFindEventRecord( 
	UNIT_RECORD_DEFINITION * pRecordDef, 
	UNIT_EVENT eEvent, 
	UNIT * pActor, 
	UNIT * pActorOwner, 
	UNIT * pTarget,
	LEVEL * pLevel,
	const char * pszParam )
{
	int pnLevels[ 3 ];
	pnLevels[ 0 ] = UnitGetExperienceLevel( pActor );
	pnLevels[ 1 ] = pActorOwner ? UnitGetExperienceLevel( pActorOwner ) : 0;
	pnLevels[ 2 ] = pTarget ? UnitGetExperienceLevel( pTarget ) : 0;

	WORD pwCodes[ 3 ];
	pwCodes[ 0 ] = UnitGetClassCode( pActor );
	pwCodes[ 1 ] = pActorOwner	? UnitGetClassCode( pActorOwner ) : 0;
	pwCodes[ 2 ] = pTarget		? UnitGetClassCode( pTarget ) : 0;

	unsigned __int64 pnGuids[ 3 ];
	pnGuids[ 0 ] = UnitGetGuid( pActor );
	pnGuids[ 1 ] = pActorOwner	? UnitGetGuid( pActorOwner ) : 0;
	pnGuids[ 2 ] = pTarget		? UnitGetGuid( pTarget ) : 0;

	for ( int i = pRecordDef->nEventRecordCount - 1; i >= 0; i-- ) // it is more likely to be at the end
	{
		UNIT_EVENT_RECORD & tRecord = pRecordDef->pEventRecords[ i ];
		if ( tRecord.eEvent == eEvent &&
			tRecord.tActor.nGenus		== UnitGetGenus( pActor ) &&
			tRecord.tActorOwner.nGenus	== (pActorOwner ? UnitGetGenus( pActorOwner ) : GENUS_NONE) &&
			tRecord.tTarget.nGenus		== (pTarget ? UnitGetGenus( pTarget ) : GENUS_NONE) &&
			tRecord.tActor.nLevel		== pnLevels[ 0 ] &&
			tRecord.tActorOwner.nLevel	== pnLevels[ 1 ] &&
			tRecord.tTarget.nLevel		== pnLevels[ 2 ] &&
			tRecord.tActor.Guid		== pnGuids[ 0 ] &&
			tRecord.tActorOwner.Guid	== pnGuids[ 1 ] &&
			tRecord.tTarget.Guid		== pnGuids[ 2 ] &&
			tRecord.tActor.wCode		== pwCodes[ 0 ] &&
			tRecord.tActorOwner.wCode	== pwCodes[ 1 ] &&
			tRecord.tTarget.wCode		== pwCodes[ 2 ] &&
			(!pLevel || PStrCmp( tRecord.szLevel, pLevel->m_szLevelName, MAX_XML_STRING_LENGTH ) == 0) && // we shouldn't need to check the drlg if the level matches in a game
			(!pszParam || PStrCmp( tRecord.szParam, pszParam, MAX_XML_STRING_LENGTH ) == 0) // we shouldn't need to check the drlg if the level matches in a game
			)
			{
				return i;
			}
	}
	return INVALID_ID;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddUnitInfoToRecord(
	GAME * pGame, 
	UNIT_EVENT_RECORD::UNIT_INFO & tRecord,
	UNIT * pUnit)
{
	if ( pUnit )
	{
		tRecord.nGenus = UnitGetGenus( pUnit );
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		PStrCopy(tRecord.szName, pUnitData->szName, MAX_XML_STRING_LENGTH);
		sCleanStringForXML( tRecord.szName );
		tRecord.wCode = UnitGetClassCode( pUnit );
		tRecord.nLevel = UnitGetExperienceLevel( pUnit );
		tRecord.Guid = UnitGetGuid( pUnit );
		tRecord.dwSecondsPlayed = UnitGetPlayedTime( pUnit );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_EVENT_RECORD * sAddEventRecord(
	GAME * pGame, 
	UNIT_EVENT eEvent,
	const char * pszEventName,
	UNIT * pActor,
	UNIT * pActorOwner, 
	UNIT * pTarget,
	LEVEL * pLevel,
	const char * pszParam,
	int nCount )
{
	UNIT_RECORD_DEFINITION * pRecordDef = sGetRecordDef( pGame );
	
	pRecordDef->nEventRecordCount++;
	ArrayResize(pRecordDef->pEventRecords, pRecordDef->nEventRecordCount);

	UNIT_EVENT_RECORD * pEventRecord = &pRecordDef->pEventRecords[ pRecordDef->nEventRecordCount - 1 ];
	ZeroMemory( pEventRecord, sizeof( UNIT_EVENT_RECORD ) );

	pEventRecord->eEvent = eEvent;

	pEventRecord->nEventCount = nCount;

	sAddUnitInfoToRecord( pGame, pEventRecord->tActor, pActor );
	sAddUnitInfoToRecord( pGame, pEventRecord->tActorOwner, pActorOwner );
	sAddUnitInfoToRecord( pGame, pEventRecord->tTarget, pTarget );

	if ( pszParam )
	{
		PStrCopy( pEventRecord->szParam, pszParam, MAX_XML_STRING_LENGTH );
		sCleanStringForXML( pEventRecord->szParam );
	}

	PStrCopy( pEventRecord->szEventName, pszEventName, MAX_XML_STRING_LENGTH );
	sCleanStringForXML( pEventRecord->szEventName );
	if ( pLevel )
	{
		PStrCopy( pEventRecord->szLevel, pLevel->m_szLevelName, MAX_XML_STRING_LENGTH );
		sCleanStringForXML( pEventRecord->szLevel );
	}

	{
		pEventRecord->bHardcore = FALSE;
		if ( !pEventRecord->bHardcore && pActor && UnitIsPlayer( pActor ) && PlayerIsHardcore( pActor ) )
			pEventRecord->bHardcore = TRUE;
		if ( !pEventRecord->bHardcore && pTarget && UnitIsPlayer( pTarget ) && PlayerIsHardcore( pTarget ) )
			pEventRecord->bHardcore = TRUE;
		if ( !pEventRecord->bHardcore && pActorOwner && UnitIsPlayer( pActorOwner ) && PlayerIsHardcore( pActorOwner ) )
			pEventRecord->bHardcore = TRUE;
	}

	{
		pEventRecord->bElite = FALSE;
		if ( !pEventRecord->bElite && pActor && UnitIsPlayer( pActor ) && PlayerIsElite( pActor ) )
			pEventRecord->bElite = TRUE;
		if ( !pEventRecord->bElite && pTarget && UnitIsPlayer( pTarget ) && PlayerIsElite( pTarget ) )
			pEventRecord->bElite = TRUE;
		if ( !pEventRecord->bElite && pActorOwner && UnitIsPlayer( pActorOwner ) && PlayerIsElite( pActorOwner ) )
			pEventRecord->bElite = TRUE;
	}

	return pEventRecord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sAddSummaryEvents( 
	GAME * pGame,
	UNIT * pActor,
	LEVEL * pLevel )
{
	ASSERT_RETURN( UnitIsPlayer( pActor ) );
	sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "Gold Total",	pActor, NULL, NULL, pLevel, "", UnitGetStat( pActor, STATS_GOLD ) );
	sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "AP Total",		pActor, NULL, NULL, pLevel, "", UnitGetStat( pActor, STATS_ACHIEVEMENT_POINTS_TOTAL ) );
	sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "AP Current",	pActor, NULL, NULL, pLevel, "", UnitGetStat( pActor, STATS_ACHIEVEMENT_POINTS_CUR ) );
	sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "Armor",		pActor, NULL, NULL, pLevel, "", UnitGetStatShift( pActor, STATS_ARMOR_BUFFER_MAX ) );
	sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "Shields",		pActor, NULL, NULL, pLevel, "", UnitGetStatShift( pActor, STATS_SHIELD_BUFFER_MAX ) );

	int nNumElementalTypes = ExcelGetNumRows( EXCEL_CONTEXT( pGame ), DATATABLE_DAMAGETYPES );
	for ( int i = 0; i < nNumElementalTypes; i++ )
	{
		int nDefense = UnitGetStat( pActor, STATS_SFX_DEFENSE, i );
		if ( ! nDefense )
			continue;

		const DAMAGE_TYPE_DATA * pDamageTypeData = (const DAMAGE_TYPE_DATA *)ExcelGetData(EXCEL_CONTEXT(pGame), DATATABLE_DAMAGETYPES, i);
		sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "SFX Defense",	pActor, NULL, NULL, pLevel, pDamageTypeData->szDamageType, nDefense );
	}
	
	UNIT * pItemCurr = UnitInventoryIterate( pActor, NULL);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pActor, pItemCurr );

		if ( ItemIsEquipped(pItemCurr, pActor) )
		{
			int nQuality = ItemGetQuality( pItemCurr );
			const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nQuality );
			sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "Item Worn", pActor, pItemCurr, NULL, pLevel, pItemQualityData->szName, nQuality );
		}
		pItemCurr = pItemNext;
	}

	UNIT_ITERATE_STATS_RANGE( pActor, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = StatGetParam( STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0 );
		if (nSkill == INVALID_ID)
		{
			continue;
		}

		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE ) )
			continue;

		int nLevel = pStatsEntry[ jj ].value;
		sAddEventRecord( pGame, (UNIT_EVENT)INVALID_ID, "Skill Known", pActor, NULL, NULL, pLevel, pSkillData->szName, nLevel );
	}
	UNIT_ITERATE_STATS_END;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sMetricsAreEnabled ( 
	GAME * pGame )
{
	return ( 
		( AppTestFlag( AF_UNITMETRICS ) && GameGetGameFlag( pGame, GAMEFLAG_UNITMETRICS_ENABLED ) ) ||
		( AppTestFlag( AF_UNITMETRICS_HCE_ONLY ) && GameGetGameFlag( pGame, GAMEFLAG_UNITMETRICS_ENABLED ) &&
			( GameIsVariant( pGame, GV_ELITE ) || GameIsVariant( pGame, GV_HARDCORE ) ) ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(RETAIL_VERSION)
static void sUnitRecordEvent(
	GAME* pGame, 
	UNIT* pActor, 
	UNIT* pTarget, 
	struct EVENT_DATA* pEventData, 
	void* data, 
	DWORD dwId )
{
	UNIT_EVENT eEvent = (UNIT_EVENT) pEventData->m_Data1;

	if ( ! sMetricsAreEnabled( pGame ) )
	{
		UnitUnregisterEventHandler( pGame, pActor, eEvent, dwId );
		return;
	}

	UNIT_EVENT_RECORDING_METHOD eMethod = (UNIT_EVENT_RECORDING_METHOD) pEventData->m_Data2;
	int nEventIndex = (int)pEventData->m_Data3;
	int nSaveToken = (int)pEventData->m_Data4;

	ASSERT_RETURN( pActor );
	UNIT * pActorOwner = UnitGetUltimateOwner( pActor );

	UNIT_RECORD_DEFINITION * pRecordDef = sGetRecordDef( pGame );
	if ( ! pRecordDef )
		return;

	if ( nSaveToken != pRecordDef->nSaveToken )
		nEventIndex = INVALID_ID;

	UNIT_EVENT_RECORD * pEventRecord = NULL;
	LEVEL * pLevel = UnitGetLevel( pActor );
	if ( ! pLevel && pActorOwner )
		pLevel = UnitGetLevel( pActorOwner );
	if ( ! pLevel || ! pLevel->m_pLevelDefinition )
		return;

	switch ( eEvent )
	{
	case EVENT_DIDKILL:
	case EVENT_GOTKILLED:
		if ( pActor == pTarget )
			return;
		if ( pTarget && ! UnitIsA( pTarget, UNITTYPE_CREATURE ) )
			return; // no crates or barrels
		break;
	}

	int nSkillLevel = 0;
	const char * pszParam = NULL;
	if ( eMethod == UNIT_EVENT_RECORD_SKILL_COUNT && data )
	{
		EVENT_DATA * pSkillEventData = (EVENT_DATA *) data;
		int nSkill = (int)pSkillEventData->m_Data1;
		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
		if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_TRACK_METRICS ) )  // change this to a new flag when you can get a hold of skills.xls
			return;
		pszParam = pSkillData ? pSkillData->szName : NULL;
		nSkillLevel = UnitGetBaseStat( pActor, STATS_SKILL_LEVEL, nSkill );
	}
	if ( eMethod == UNIT_EVENT_RECORD_STAT && data )
	{
		EVENT_DATA * pSkillEventData = (EVENT_DATA *) data;
		int nStat = (int)pSkillEventData->m_Data1;
		const STATS_DATA * pStatData = StatsGetData( pGame, nStat );
		pszParam = pStatData ? pStatData->m_szName : NULL;
	}

	UNIT_EVENT_TYPE_DATA * pEventType = (UNIT_EVENT_TYPE_DATA *) ExcelGetData( pGame, DATATABLE_UNIT_EVENT_TYPES, eEvent );

	if ( eMethod == UNIT_EVENT_RECORD_NEW_ENTRY ||
		 eMethod == UNIT_EVENT_RECORD_SKILL_COUNT )
	{
		pEventRecord = sAddEventRecord( pGame, eEvent, pEventType->szName, pActor, pActorOwner, pTarget, pLevel, pszParam, 1 );
	}
	else 
	{
		// use the index that we had before - most of the time it is the right record
		int pnLevels[ 3 ];
		pnLevels[ 0 ] = UnitGetExperienceLevel( pActor );
		pnLevels[ 1 ] = pActorOwner ? UnitGetExperienceLevel( pActorOwner ) : 0;
		pnLevels[ 2 ] = pTarget ? UnitGetExperienceLevel( pTarget ) : 0;

		WORD pwCodes[ 3 ];
		pwCodes[ 0 ] = UnitGetClassCode( pActor );
		pwCodes[ 1 ] = pActorOwner	? UnitGetClassCode( pActorOwner ) : 0;
		pwCodes[ 2 ] = pTarget		? UnitGetClassCode( pTarget ) : 0;

		if ( nEventIndex >= 0 && nEventIndex < pRecordDef->nEventRecordCount )
		{
			pEventRecord = &pRecordDef->pEventRecords[ nEventIndex ];
			// see if we have the right record
			if ( pEventRecord->eEvent == eEvent &&
				pEventRecord->tActor.nGenus		== UnitGetGenus( pActor ) &&
				pEventRecord->tActorOwner.nGenus	== (pActorOwner ? UnitGetGenus( pActorOwner ) : GENUS_NONE) &&
				pEventRecord->tTarget.nGenus		== (pTarget ? UnitGetGenus( pTarget ) : GENUS_NONE) &&

				pEventRecord->tActor.Guid		== UnitGetGuid( pActor ) &&
				pEventRecord->tActorOwner.Guid	== (pActorOwner ? UnitGetGuid( pActorOwner ) : GENUS_NONE) &&
				pEventRecord->tTarget.Guid		== (pTarget ? UnitGetGuid( pTarget ) : GENUS_NONE) &&

				pEventRecord->tActor.nLevel		== pnLevels[ 0 ] &&
				pEventRecord->tActorOwner.nLevel	== pnLevels[ 1 ] &&
				pEventRecord->tTarget.nLevel		== pnLevels[ 2 ] &&

				pEventRecord->tActor.wCode		== pwCodes[ 0 ] &&
				pEventRecord->tActorOwner.wCode	== pwCodes[ 1 ] &&
				pEventRecord->tTarget.wCode		== pwCodes[ 2 ] &&
				(!pLevel || PStrCmp( pEventRecord->szLevel, pLevel->m_szLevelName, MAX_XML_STRING_LENGTH ) == 0) && // we shouldn't need to check the drlg if the level matches in a game
				(!pszParam || PStrCmp( pEventRecord->szParam, pszParam, MAX_XML_STRING_LENGTH ) == 0) 
				)
			{
			}
			else
			{// I did this backwards so that the test matches the other one in this file
				pEventRecord = NULL;
			}

		}
		if ( ! pEventRecord )
		{
			int nFoundIndex = sFindEventRecord( pRecordDef, eEvent, pActor, pActorOwner, pTarget, pLevel, pszParam );
			pEventRecord = nFoundIndex != INVALID_ID ? &pRecordDef->pEventRecords[ nFoundIndex ] : NULL;

			if ( ! pEventRecord )
			{
				pEventRecord = sAddEventRecord( pGame, eEvent, pEventType->szName, pActor, pActorOwner, pTarget, pLevel, pszParam, 1 );
			}
		}

		if ( pEventRecord )
		{
			UnitUnregisterEventHandler( pGame, pActor, eEvent, dwId );
			UnitRegisterEventHandler( pGame, pActor, eEvent, sUnitRecordEvent, EVENT_DATA( eEvent, eMethod, pRecordDef->nEventRecordCount - 1, pRecordDef->nSaveToken ) );

			ASSERT_RETURN( pEventRecord->eEvent == eEvent ); // just double check that have what we want
		}

	}

	if ( pEventRecord )
	{
		switch ( eMethod )
		{
		case UNIT_EVENT_RECORD_SUM_DAMAGE:
			if ( data )
			{
				STATS * pStats = (STATS *) data;
				int nTotalDamage = StatsGetStat( pStats, STATS_DAMAGE_DONE );
				pEventRecord->nDamageTotal += nTotalDamage;
			}// the sum damage method also updates the count
		case UNIT_EVENT_RECORD_COUNT_EVENTS:
		case UNIT_EVENT_RECORD_STAT:
			pEventRecord->nEventCount++;
			break;
		case UNIT_EVENT_RECORD_SKILL_COUNT:
			pEventRecord->nEventCount = nSkillLevel;
			break;
		case UNIT_EVENT_RECORD_ADD_SUMMARY_EVENTS:
			sAddSummaryEvents( pGame, pActor, pLevel );
			break;
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitMetricsInit( 
	UNIT * pUnit )
{
#if !ISVERSION(RETAIL_VERSION)
	GAME * pGame = UnitGetGame( pUnit );

	if ( IS_CLIENT( pGame ) )
		return;// right now the only important stuff is on the server
	if ( ! sMetricsAreEnabled( pGame ) )
		return;

	int nGenus = UnitGetGenus( pUnit );

	ASSERTX_RETURN( NUM_UNIT_EVENTS == ExcelGetNumRows( NULL, DATATABLE_UNIT_EVENT_TYPES ), "Data doesn't match code - Unit Events" );
	for ( int i = 0; i < NUM_UNIT_EVENTS; i++ )
	{
		UNIT_EVENT_TYPE_DATA * pEventTypeData = (UNIT_EVENT_TYPE_DATA *) ExcelGetData( NULL, DATATABLE_UNIT_EVENT_TYPES, i );
		ASSERT_CONTINUE( pEventTypeData );
		if ( pEventTypeData->pRecordMethodByGenus[ nGenus ] != UNIT_EVENT_RECORD_IGNORE )
		{
			UnitRegisterEventHandler( pGame, pUnit, (UNIT_EVENT)i, sUnitRecordEvent, 
				EVENT_DATA( i, pEventTypeData->pRecordMethodByGenus[ nGenus ], INVALID_ID, INVALID_ID ) );
		}
	}
#else
	REF(pUnit);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define TICKS_BETWEEN_SAVES (4*60*GAME_TICKS_PER_SECOND)
static BOOL sSaveMetrics(
	 GAME * game, 
	 UNIT * unit, const 
	 EVENT_DATA & event_data)
{
#if !ISVERSION(RETAIL_VERSION)
	UnitMetricsSystemSave( game );

	GameEventRegister( game, sSaveMetrics, NULL, NULL, EVENT_DATA(), TICKS_BETWEEN_SAVES );
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitMetricsSystemInit(
	GAME * pGame )
{
#if !ISVERSION(RETAIL_VERSION)
	if ( IS_CLIENT( pGame ) )
		return;
	if ( ! AppTestFlag( AF_UNITMETRICS ) )
		return;
	GameSetGameFlag( pGame, GAMEFLAG_UNITMETRICS_ENABLED, TRUE );
	ASSERT_RETURN( ! pGame->m_UnitRecording );
	pGame->m_UnitRecording = (UNIT_RECORD_DEFINITION *) GMALLOCZ( pGame, sizeof( UNIT_RECORD_DEFINITION ) );
	if ( pGame->m_UnitRecording )
	{
		PStrPrintf( pGame->m_UnitRecording->szBuildVersion, MAX_XML_STRING_LENGTH, "%d.%d.%d.%d", 
			TITLE_VERSION,PRODUCTION_BRANCH_VERSION,RC_BRANCH_VERSION,MAIN_LINE_VERSION );
		pGame->m_UnitRecording->pEventRecords.Init(pGame->m_MemPool);
	}
	GameEventRegister( pGame, sSaveMetrics, NULL, NULL, EVENT_DATA(), TICKS_BETWEEN_SAVES );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitMetricsSystemClose(
	GAME * pGame )
{
#if !ISVERSION(RETAIL_VERSION)
	UNIT_RECORD_DEFINITION * pDef = sGetRecordDef( pGame );
	if ( ! pDef )
		return;

	UnitMetricsSystemSave( pGame );

	pGame->m_UnitRecording->pEventRecords.Free();

	GFREE( pGame, pDef );	

	pGame->m_UnitRecording = NULL;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static 
void sSaveToMarkup( 
	UNIT_RECORD_DEFINITION * pDef, 
	CMarkup & tMarkup )
{
	tMarkup.AddElem("UNIT_RECORD_DEFINITION");

	tMarkup.AddChildElem( "szBuildVersion", pDef->szBuildVersion );
	tMarkup.AddChildElem( "szSaveTime", pDef->szSaveTime );

	CString sValue;
	sValue.Format( "%d", pDef->nEventRecordCount ); tMarkup.AddChildElem( "pEventRecordsCount", sValue );

	for ( int i = 0; i < pDef->nEventRecordCount; i++ )
	{
		tMarkup.AddChildElem( "pEventRecords" );

		tMarkup.IntoElem();

		tMarkup.AddElem("UNIT_EVENT_RECORD");

		UNIT_EVENT_RECORD & tRecord = pDef->pEventRecords[ i ];
		tMarkup.AddChildElem( "szEventName", tRecord.szEventName );
		sValue.Format( "%d", tRecord.nEventCount );						tMarkup.AddChildElem( "nEventCount", sValue );
		sValue.Format( "%d", tRecord.nDamageTotal );					tMarkup.AddChildElem( "nDamageTotal", sValue );
		sValue.Format( "%d", tRecord.tActor.nGenus );					tMarkup.AddChildElem( "tActor.nGenus", sValue );
		tMarkup.AddChildElem( "tActor.szName", tRecord.tActor.szName );
		sValue.Format( "%d", tRecord.tActor.wCode );					tMarkup.AddChildElem( "tActor.wCode", sValue );
		sValue.Format( "%d", tRecord.tActor.nLevel );					tMarkup.AddChildElem( "tActor.nLevel", sValue );
		sValue.Format( "%I64d", tRecord.tActor.Guid );					tMarkup.AddChildElem( "tActor.Guid", sValue );
		sValue.Format( "%d", tRecord.tActor.dwSecondsPlayed );			tMarkup.AddChildElem( "tActor.dwSecondsPlayed", sValue );
		sValue.Format( "%d", tRecord.tActorOwner.nGenus );				tMarkup.AddChildElem( "tActorOwner.nGenus", sValue );
		tMarkup.AddChildElem( "tActorOwner.szName", tRecord.tActorOwner.szName );
		sValue.Format( "%d", tRecord.tActorOwner.wCode );				tMarkup.AddChildElem( "tActorOwner.wCode", sValue );
		sValue.Format( "%d", tRecord.tActorOwner.nLevel );				tMarkup.AddChildElem( "tActorOwner.nLevel", sValue );
		sValue.Format( "%I64d", tRecord.tActorOwner.Guid );				tMarkup.AddChildElem( "tActorOwner.Guid", sValue );
		sValue.Format( "%d", tRecord.tActorOwner.dwSecondsPlayed );		tMarkup.AddChildElem( "tActorOwner.dwSecondsPlayed", sValue );
		sValue.Format( "%d", tRecord.tTarget.nGenus );					tMarkup.AddChildElem( "tTarget.nGenus", sValue );
		tMarkup.AddChildElem( "tTarget.szName", tRecord.tTarget.szName );
		sValue.Format( "%d", tRecord.tTarget.wCode );					tMarkup.AddChildElem( "tTarget.wCode", sValue );
		sValue.Format( "%d", tRecord.tTarget.nLevel );					tMarkup.AddChildElem( "tTarget.nLevel", sValue );
		sValue.Format( "%I64d", tRecord.tTarget.Guid );					tMarkup.AddChildElem( "tTarget.Guid", sValue );
		sValue.Format( "%d", tRecord.tTarget.dwSecondsPlayed );			tMarkup.AddChildElem( "tTarget.dwSecondsPlayed", sValue );

		tMarkup.AddChildElem( "szParam", tRecord.szParam );
		tMarkup.AddChildElem( "szLevel", tRecord.szLevel );

		tMarkup.AddChildElem( "bHardcore", tRecord.bHardcore ? "1" : "0" );
		tMarkup.AddChildElem( "bElite", tRecord.bElite ? "1" : "0" );

		tMarkup.OutOfElem();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitMetricsSystemSave(
	GAME * pGame )
{
#if !ISVERSION(RETAIL_VERSION)
	UNIT_RECORD_DEFINITION * pDef = sGetRecordDef( pGame );
	if ( ! pDef )
		return;

	if ( pDef->nEventRecordCount && sMetricsAreEnabled( pGame ) )
	{

		const OS_PATH_CHAR * pszPath = FilePath_GetSystemPath(FILE_PATH_UNIT_RECORD);
		ASSERT_RETURN( pszPath );

		FileCreateDirectory( pszPath );

		for ( int i = 0; i < pDef->nEventRecordCount; i++ )
		{
			UNIT_EVENT_RECORD * pEventRecord = & pDef->pEventRecords[ i ];
			if ( pEventRecord->nDamageTotal )
				pEventRecord->nDamageTotal = pEventRecord->nDamageTotal >> StatsGetShift( pGame, STATS_HP_CUR );
		}

		time_t curtime;
		time(&curtime);

		struct tm curtimeex;
		localtime_s(&curtimeex, &curtime);

		PStrPrintf( pDef->szSaveTime, MAX_XML_STRING_LENGTH, "%d %d %d %2d:%2d", (int)curtimeex.tm_year + 1900, (int)curtimeex.tm_mon + 1, (int)curtimeex.tm_mday, curtimeex.tm_hour, curtimeex.tm_min);

		// open and write headers for this session's log file
		PStrPrintf(pDef->tHeader.pszName, MAX_XML_STRING_LENGTH, "Recording%04d%02d%02d_%d%d_%d.xml", 
			(int)curtimeex.tm_year + 1900, (int)curtimeex.tm_mon + 1, (int)curtimeex.tm_mday, curtimeex.tm_hour, curtimeex.tm_min, pGame->m_idGame);

		CMarkup tMarkup;
		sSaveToMarkup( pDef, tMarkup );

		WCHAR szHeaderName[_countof(pDef->tHeader.pszName)];
		PStrCvt(szHeaderName, pDef->tHeader.pszName, _countof(szHeaderName));

		WCHAR pszFileName[MAX_XML_STRING_LENGTH];
		PStrPrintf(pszFileName, _countof(pszFileName), L"%s%s", pszPath, szHeaderName);

		PStrReplaceExtension(pszFileName, _countof(pszFileName), pszFileName, L"xml" );

		tMarkup.Save(pszFileName);

	}
	pDef->nEventRecordCount = 0;
	pDef->nSaveToken++;
#endif
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitTrackLevelUp(
	UNIT * pUnit )
{
#if ISVERSION(SERVER_VERSION)

	int nPlayerLevel = UnitGetExperienceLevel( pUnit );
	int nPlayerRank = UnitGetPlayerRank( pUnit );

	TrackEvent_LevelUpEvent( UnitGetGuid(pUnit), nPlayerLevel, nPlayerRank );

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = StatGetParam( STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0 );
		if (nSkill == INVALID_ID)
		{
			continue;
		}

		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		if ( !pSkillData || ! SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE ) )
			continue;

		int nSkillLevel = pStatsEntry[ jj ].value;
		TrackEvent_SkillKnownEvent( UnitGetGuid( pUnit ), pSkillData->dwCode, pSkillData->szName, nPlayerLevel, nPlayerRank, nSkillLevel );
	}
	UNIT_ITERATE_STATS_END;

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitTrackPlayerDeath(
	UNIT * pPlayer,
	UNIT * pKiller )
{
#if ISVERSION(SERVER_VERSION)

	ASSERT_RETURN( pPlayer );

	if ( pKiller && UnitIsA( pKiller, UNITTYPE_PLAYER ) )
	{
		int nDeathContext = 0; // I need some help from other people to set this - Tyler

		TrackEvent_KilledByPlayerEvent( UnitGetGuid( pPlayer ), UnitGetExperienceLevel( pPlayer ), UnitGetPlayerRank( pPlayer ),
			UnitGetGuid( pKiller ), UnitGetExperienceLevel( pKiller ), UnitGetPlayerRank( pKiller ), nDeathContext );

	} else {
		const UNIT_DATA * pKillerData = pKiller ? UnitGetData( pKiller ) : NULL;

		TrackEvent_KilledByMonsterEvent( UnitGetGuid( pPlayer ), UnitGetExperienceLevel( pPlayer ), UnitGetPlayerRank( pPlayer ), 
			pKillerData ? pKillerData->wCode : 0, pKillerData ? pKillerData->szName : "", 
			pKiller ? UnitGetExperienceLevel( pKiller ) : 0, pKiller ? MonsterGetQuality( pKiller ) : 0 );
	}

#endif
}

