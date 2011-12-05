///----------------------------------------------------------------------------
// FILE: skill_strings.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "colors.h"
#include "fontcolor.h"
#include "globalindex.h"
#include "units.h"
#include "skills.h"
#include "skill_strings.h"
#include "language.h"
#include "stringtable.h"
#include "script.h"
#include "inventory.h"
#include "gameglobals.h"
#include "uix_priv.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define STRING_TOKEN_DURATION	 -2
#define STRING_TOKEN_MIN_DAM	-3
#define STRING_TOKEN_MAX_DAM	-4
#define STRING_TOKEN_SFX_ATTACK -5
#define STRING_TOKEN_RANGE		-6
#define STRING_TOKEN_FIELD_DURATION -7
#define STRING_TOKEN_FIELD_RADIUS -8
#define STRING_TOKEN_FIELD_POWERCOST -9
#define STRING_TOKEN_DAMAGE_RADIUS -10
#define STRING_TOKEN_SPECIFIC_STAT -11
#define SKILL_STRING_TOKEN		L"string"

static BOOL sgbDebugStatStrings = FALSE;

//----------------------------------------------------------------------------
struct SKILL_STRING_FUNCTION_INPUT
{
	int nSkill;
	const SKILL_DATA *pSkillData;
	UNIT *pUnit;
	UNIT *pPlayer;
	int nSkillLevel;
	int nSkillLevelUnmodified;
	int nStringIndex;
	SKILL_STRING_TYPES eStringType;
	const struct SKILL_STRING_FUNCTION_LOOKUP * pLookup;
	
	SKILL_STRING_FUNCTION_INPUT::SKILL_STRING_FUNCTION_INPUT( void )
		:	nSkill( INVALID_LINK ),
			pSkillData( NULL ),
			pUnit( NULL ),
			pPlayer( NULL ),
			nSkillLevel( 0 ),
			pLookup(NULL),
			eStringType( SKILL_STRING_DESCRIPTION ),
			nStringIndex( INVALID_ID )
		{ }
};

//----------------------------------------------------------------------------
struct SKILL_STRING_FUNCTION_OUTPUT
{
	WCHAR *puszBuffer;
	int nBufferSize;
		
	SKILL_STRING_FUNCTION_OUTPUT::SKILL_STRING_FUNCTION_OUTPUT( void )
		:	puszBuffer( NULL ),
			nBufferSize( 0 )
		{ }
};

//----------------------------------------------------------------------------
typedef void (*PFN_SKILL_INFO)( const SKILL_STRING_FUNCTION_INPUT &tIn, SKILL_STRING_FUNCTION_OUTPUT &tOut );

//----------------------------------------------------------------------------
#define MAX_VALUES_PER_SKILL_INFO 10
#define MAX_STATS_PER_SKILL_CODE 10
struct SKILL_STRING_FUNCTION_LOOKUP
{
	const char *szName;
	PFN_SKILL_INFO pfnInfo;
	int pnIndices[ MAX_VALUES_PER_SKILL_INFO ];
	int pnParams[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL bPercent[ MAX_VALUES_PER_SKILL_INFO ];
};

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSimple(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	// just get the static tooltip
	if (tIn.nStringIndex != INVALID_LINK)
	{
		const WCHAR *puszString = StringTableGetStringByIndex( tIn.nStringIndex );
		PStrCopy( tOut.puszBuffer, puszString, tOut.nBufferSize );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMultiBullets(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
//	int nNumBulletsPerLevel = SkillMultiShot
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDuration(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	sSimple(tIn, tOut);
	int nSkillDuration = SkillDataGetStateDuration(UnitGetGame(tIn.pUnit), tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, FALSE);
	
	const WCHAR * pszFormat = GlobalStringGet(GS_SKILL_DURATION);
	ASSERT_RETURN( pszFormat );

	WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
	PStrPrintf(uszTemp, MAX_SKILL_TOOLTIP, pszFormat, (int)(nSkillDuration / GAME_TICKS_PER_SECOND));

	PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s %s", tOut.puszBuffer, uszTemp);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDurationf(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	if(AppIsTugboat())
	{ // looked at adding the same strings for mythos, but they look like they're set up different... - bmanegold
		sDuration(tIn, tOut);
		return;
	}

	sSimple(tIn, tOut);
	int nSkillDuration = SkillDataGetStateDuration(UnitGetGame(tIn.pUnit), tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, FALSE);

	const WCHAR * pszFormat = GlobalStringGet(GS_SKILL_DURATIONF);
	ASSERT_RETURN( pszFormat );

	WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
	PStrPrintf(uszTemp, MAX_SKILL_TOOLTIP, pszFormat, ((float)nSkillDuration / (float)GAME_TICKS_PER_SECOND));

	PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s %s", tOut.puszBuffer, uszTemp);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRange(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	sSimple(tIn, tOut);
	float fRange = SkillGetRange( tIn.pUnit, tIn.nSkill, NULL, FALSE, tIn.nSkillLevel );
	
	const WCHAR * pszFormat = GlobalStringGet(GS_SKILL_RANGE);
	ASSERT_RETURN( pszFormat );

	// get number as string
	const int MAX_NUMBER = 32;
	WCHAR uszNumber[ MAX_NUMBER ];					
	LanguageFormatFloatString( uszNumber, MAX_NUMBER, fRange, 1 );

	// plug into string
	const WCHAR *puszToken = L"[string1]";
	WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
	PStrCopy( uszTemp, pszFormat, MAX_SKILL_TOOLTIP );
	PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );
	
	PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s %s", tOut.puszBuffer, uszTemp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCooldownAddToEnd(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	int nCooldownTicks = SkillGetCooldown( UnitGetGame( tIn.pUnit ), tIn.pUnit, NULL, tIn.nSkill, tIn.pSkillData, 1, tIn.nSkillLevel );

	float fCoodownSeconds = (float)nCooldownTicks / GAME_TICKS_PER_SECOND_FLOAT;

	const WCHAR * pszFormat = GlobalStringGet(GS_SKILL_COOLDOWN);
	ASSERT_RETURN( pszFormat );

	// get number as string
	const int MAX_NUMBER = 32;
	WCHAR uszNumber[ MAX_NUMBER ];					
	LanguageFormatFloatString( uszNumber, MAX_NUMBER, fCoodownSeconds, 1 );

	// plug into string
	const WCHAR *puszToken = L"[string1]";
	WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
	PStrCopy(uszTemp, pszFormat, MAX_SKILL_TOOLTIP );
	PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );

	PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s\n%s", tOut.puszBuffer, uszTemp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCooldown(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	sSimple(tIn, tOut);

	sCooldownAddToEnd( tIn, tOut );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringWithStatValue(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut )
{
	int pnStatsEntryToOutput[ MAX_VALUES_PER_SKILL_INFO ];
	int pnOutputToStat[ MAX_VALUES_PER_SKILL_INFO ];
	int pnScriptParams[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL pbNegateValue[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL pbDivideByGameTicksPerSecond[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL pbDivideBy10[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL pbDivideByMSPerSecond[ MAX_VALUES_PER_SKILL_INFO ];
	BOOL bUseParamsForScript = FALSE;
	BOOL bDisplayCooldown = FALSE;
	BOOL bRequiresStatsEntries = FALSE;
	BOOL bRegenInSeconds = FALSE;


	BOOL pbFloat[MAX_STATS_PER_SKILL_CODE];
	//float pfValues[MAX_STATS_PER_SKILL_CODE];

	{

		ZeroMemory( pbDivideByGameTicksPerSecond, MAX_VALUES_PER_SKILL_INFO * sizeof(BOOL) );
		ZeroMemory( pbDivideByMSPerSecond, MAX_VALUES_PER_SKILL_INFO * sizeof(BOOL) );
		ZeroMemory( pbFloat, MAX_VALUES_PER_SKILL_INFO * sizeof(BOOL) );
		ZeroMemory( pbDivideBy10, MAX_VALUES_PER_SKILL_INFO * sizeof(BOOL) );
		for ( int i = 0; i < MAX_VALUES_PER_SKILL_INFO; i++ )
		{
			pnOutputToStat[ i ] = INVALID_ID;
			pnScriptParams[ i ] = INVALID_ID;
		}

		const int cnMaxBufLen = MAX_SKILL_STRING_CODE_LENGTH;
		char token[ cnMaxBufLen ];
		const char* buf = tIn.pSkillData->pszStringFunctions[ tIn.eStringType ];
		int nCurrBufLen = MAX_SKILL_STRING_CODE_LENGTH;
		int len;
		buf = PStrTok(token, buf, " ", cnMaxBufLen, nCurrBufLen, &len);
		// skip the function name
		if ( token[ 0 ] )
			buf = PStrTok(token, buf, " ", cnMaxBufLen, nCurrBufLen, &len);

		int nTokenIndex = 0;
		while ( token[ 0 ] )
		{
			BOOL bIncrementTokenIndex = TRUE;
			int nValue = atoi( token );
			if ( token[ 1 ] == 'P' )
			{
				pnScriptParams[ 0 ] = pnScriptParams[ 1 ] = pnScriptParams[ 2 ] = nValue - 1;
				pnStatsEntryToOutput[ 0 ] = 0;
				pnStatsEntryToOutput[ 1 ] = 1;
				pnStatsEntryToOutput[ 2 ] = 2;
				pbNegateValue[ 0 ] = pbNegateValue[ 1 ] = pbNegateValue[ 2 ] = FALSE;
				nTokenIndex = 2;
				bUseParamsForScript = TRUE;
				bRequiresStatsEntries = TRUE;
			}
			else if ( ! nValue )
			{
				if ( PStrICmp( token, "Dur", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_DURATION;
					pbFloat [nTokenIndex] = TRUE;
				}
				else if ( PStrICmp( token, "Range", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_RANGE;
				}
				else if ( PStrICmp( token, "Damages", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ 0 ] = STRING_TOKEN_MIN_DAM;
					pnStatsEntryToOutput[ 1 ] = STRING_TOKEN_MAX_DAM;
					pnStatsEntryToOutput[ 2 ] = STRING_TOKEN_SFX_ATTACK;
					pbNegateValue[ 0 ] = pbNegateValue[ 1 ] = pbNegateValue[ 2 ] = FALSE;
					nTokenIndex = 3;
					break;
				}
				else if ( PStrICmp( token, "FieldDur", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_FIELD_DURATION;
					pbDivideByGameTicksPerSecond[ nTokenIndex ] = TRUE;
				}
				else if ( PStrICmp( token, "FieldRadius", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_FIELD_RADIUS;
				}
				else if ( PStrICmp( token, "DmgRadius", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_DAMAGE_RADIUS;
				}
				else if ( PStrICmp( token, "Cooldown", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					bDisplayCooldown = TRUE;
					bIncrementTokenIndex = FALSE;
				}
				else if ( PStrICmp( token, "Power", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_FIELD_POWERCOST;
				}
				else
				{
					char szSpecificStatToken[ MAX_SKILL_STRING_CODE_LENGTH ];
					const char* pBuffCurr = token;
					int nSpecificStatCurrBufLen = MAX_SKILL_STRING_CODE_LENGTH;
					int nSpecificStatTokenLen;
					pBuffCurr = PStrTok(szSpecificStatToken, pBuffCurr, ",", MAX_SKILL_STRING_CODE_LENGTH, nSpecificStatCurrBufLen, &nSpecificStatTokenLen);
					while ( szSpecificStatToken[ 0 ] )
					{
						int nIndex = ExcelGetLineByStringIndex( EXCEL_CONTEXT(), DATATABLE_STATS, szSpecificStatToken );
						if ( nIndex != INVALID_ID )
							pnOutputToStat[ nTokenIndex ] = nIndex;
						else 
						{
							int nStatTokenCurrentChar = 0;
							if ( szSpecificStatToken[ nStatTokenCurrentChar ] == '-' )
							{
								pbNegateValue[ nTokenIndex ] = TRUE;
								nStatTokenCurrentChar += 1;
							}
							if ( szSpecificStatToken[ nStatTokenCurrentChar ] == 'd' )
							{
								pbDivideByGameTicksPerSecond[ nTokenIndex ] = TRUE;
							}
							else if ( PStrICmp( &szSpecificStatToken[ nStatTokenCurrentChar ], "sec", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
							{
								bRegenInSeconds = TRUE;
							}
							else if ( PStrICmp( &szSpecificStatToken[ nStatTokenCurrentChar ], "ms", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
							{
								pbDivideByMSPerSecond[ nTokenIndex ] = TRUE;
							}
						}
						pBuffCurr = PStrTok(szSpecificStatToken, pBuffCurr, ",", MAX_SKILL_STRING_CODE_LENGTH, nSpecificStatCurrBufLen, &nSpecificStatTokenLen);
					}
					pnStatsEntryToOutput[ nTokenIndex ] = STRING_TOKEN_SPECIFIC_STAT;
				}
				pbNegateValue[ nTokenIndex ] = FALSE;
			} 
			else if ( nValue > 0 )
			{
				pbNegateValue[ nTokenIndex ] = FALSE;
				pnStatsEntryToOutput[ nTokenIndex ] = nValue - 1;
				if ( token[ 1 ] == 'd' )
				{
					pbDivideByGameTicksPerSecond[ nTokenIndex ] = TRUE;
					pbFloat[nTokenIndex] = TRUE;
				}
				else if ( token[ 1 ] == 't' )
				{
					pbDivideBy10[nTokenIndex] = TRUE;
					if ( PStrICmp( &token[ 1 ], "tf", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
					{
						pbFloat[nTokenIndex] = TRUE;
					}
				}
				else if ( token[ 1 ] == 'f' )
				{
					pbFloat[nTokenIndex] = TRUE;
				}
				else if ( PStrICmp( &token[ 1 ], "sec", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					bRegenInSeconds = TRUE;
					pbFloat[nTokenIndex] = TRUE;
				}
				else if ( PStrICmp( &token[ 1 ], "ms", MAX_SKILL_STRING_CODE_LENGTH ) == 0 )
				{
					pbDivideByMSPerSecond[ nTokenIndex ] = TRUE;
					pbFloat[nTokenIndex] = TRUE;
				}
				bRequiresStatsEntries = TRUE;
			}
			else
			{
				pbNegateValue[ nTokenIndex ] = TRUE;
				pnStatsEntryToOutput[ nTokenIndex ] = -nValue - 1;
				bRequiresStatsEntries = TRUE;
			}

			if ( bIncrementTokenIndex )
				nTokenIndex++;
			buf = PStrTok(token, buf, " ", cnMaxBufLen, nCurrBufLen, &len);
		}

		for ( int i = nTokenIndex; i < MAX_VALUES_PER_SKILL_INFO; i++ )
		{
			pnStatsEntryToOutput[ i ] = -1;
		}

	}

	GAME * pGame = UnitGetGame( tIn.pUnit );

	struct STATS * pStatsList = NULL;
	STATS_ENTRY	pStatsEntries[ MAX_STATS_PER_SKILL_CODE ];

	int code_len = 0;
	BOOL bIncludeValue = FALSE;
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeInfoScript, &code_len );	
	if ( code_ptr )
		bIncludeValue = TRUE;

	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsPostLaunch, &code_len );
	if( !code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnStateSetServerOnly, &code_len );
	if( !code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnStateSet, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsSkillEvent, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsSkillEventServer, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnPulseServerOnly, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnPulse, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnSkillStart, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsServerPostLaunch, &code_len );
	if ( ! code_ptr )
		code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeStatsOnLevelChange, &code_len );

	ASSERT_RETURN( code_ptr );

	int nStatCount = 0;
	if ( ! bUseParamsForScript )
	{
		int nValue = 0;
		if ( code_ptr )
		{
			pStatsList = StatsListInit(pGame);

			UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
			UnitGetWeapons( tIn.pPlayer, tIn.nSkill, pWeapons, FALSE );
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				if ( pWeapons[ i ] && StatsGetParent(pWeapons[ i ]) != tIn.pPlayer)
				{
					pWeapons[ i ] = NULL;
					if ( i < MAX_WEAPONS_PER_UNIT - 1 )
					{
						pWeapons[ i ] = pWeapons[ i + 1 ];
						i--;
					}
				}
			}
			UnitInventorySetEquippedWeapons(tIn.pPlayer, pWeapons);
			int nParam1 = 1; // some skills require a param to distinguish between states that need stats and those that don't
			nValue = VMExecI(pGame, tIn.pPlayer, tIn.pPlayer, pStatsList, nParam1, tIn.nSkillLevel, tIn.nSkill, tIn.nSkillLevel, INVALID_ID, code_ptr, code_len);

			UnitInventorySetEquippedWeapons( tIn.pPlayer, NULL );

			nStatCount = StatsListGetEntries( pStatsList, pStatsEntries, MAX_STATS_PER_SKILL_CODE );
		}

		if ( ! nStatCount && nValue && bIncludeValue )
		{
			pStatsEntries[ 0 ].value = nValue;
			pStatsEntries[ 0 ].stat = INVALID_ID;
			nStatCount = 1;
		}

	} 
	else if ( code_ptr ) 
	{
		UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
		UnitGetWeapons( tIn.pPlayer, tIn.nSkill, pWeapons, FALSE );
		UnitInventorySetEquippedWeapons(tIn.pPlayer, pWeapons);

		for ( int i = 0; i < MAX_VALUES_PER_SKILL_INFO; i++ )
		{
			if ( pnScriptParams[ i ] == INVALID_ID )
				continue;
			

			pStatsList = StatsListInit(pGame);
			
			int nValue = 0;

			nValue = VMExecI(pGame, tIn.pPlayer, tIn.pPlayer, pStatsList, i, tIn.nSkillLevel, tIn.nSkill, tIn.nSkillLevel, INVALID_ID, code_ptr, code_len);

			int nStatCountCurr = StatsListGetEntries( pStatsList, &pStatsEntries[ i ], MAX_STATS_PER_SKILL_CODE - i );

			if ( ! nStatCountCurr && nValue && bIncludeValue )
			{
				pStatsEntries[ nStatCount ].value = nValue;
				pStatsEntries[ nStatCount ].stat  = INVALID_ID;
			} else {
				pStatsEntries[ nStatCount ].value = pStatsEntries[ i + pnScriptParams[ i ] ].value;
				pStatsEntries[ nStatCount ].stat  = pStatsEntries[ i + pnScriptParams[ i ] ].stat;
			}
			if ( pStatsEntries[ nStatCount ].value )
				nStatCount++;

			StatsListFree( pGame, pStatsList );
			pStatsList = NULL;
		}

		UnitInventorySetEquippedWeapons( tIn.pPlayer, NULL );
	}

	for ( int i = 0; i < nStatCount; i++ )
	{
		if ( pStatsEntries[ i ].stat == INVALID_ID )
			continue;
		const STATS_DATA * pStatsData = StatsGetData( pGame, STAT_GET_STAT( pStatsEntries[ i ].stat ) );
		if ( !bRegenInSeconds && ( pStatsData->m_nRegenIntervalInMS || pStatsData->m_nSpecialFunction == STATSSPEC_LINKED_REGEN ) ) // we like to represent these as changes per minute so that the numbers look larger 
			pStatsEntries[ i ].value *= 60;
		if ( pStatsData->m_nShift )
			pStatsEntries[ i ].value >>= pStatsData->m_nShift;
	}

	ASSERT_GOTO (tIn.nStringIndex != INVALID_LINK, cleanup);

	if ( nStatCount == 0 && bRequiresStatsEntries )
		goto cleanup;

	int nStatsToPrint = 0;	
	float pnValues[ MAX_VALUES_PER_SKILL_INFO ];

	for ( ; nStatsToPrint < MAX_VALUES_PER_SKILL_INFO; nStatsToPrint++ )
	{
		if ( pnStatsEntryToOutput[ nStatsToPrint ] == -1 )
			break;
		switch ( pnStatsEntryToOutput[ nStatsToPrint ] )
		{
		case STRING_TOKEN_DURATION:
			pnValues[ nStatsToPrint ] = (float)(SkillDataGetStateDuration(UnitGetGame(tIn.pUnit), tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, FALSE) / GAME_TICKS_PER_SECOND);
			break;

		case STRING_TOKEN_MIN_DAM:
			pnValues[ nStatsToPrint ] = (float)(PCT( StatsGetStatAny( pStatsList, STATS_BASE_DMG_MIN, NULL ), StatsGetStatAny( pStatsList, STATS_DAMAGE_MIN, NULL ) ) >> StatsGetShift( NULL, STATS_BASE_DMG_MIN ));
			break;

		case STRING_TOKEN_MAX_DAM:
			pnValues[ nStatsToPrint ] = (float)(PCT( StatsGetStatAny( pStatsList, STATS_BASE_DMG_MAX, NULL ), StatsGetStatAny( pStatsList, STATS_DAMAGE_MAX, NULL ) )>> StatsGetShift( NULL, STATS_BASE_DMG_MAX ));
			break;

		case STRING_TOKEN_SFX_ATTACK:
			pnValues[ nStatsToPrint ] = (float)StatsGetStatAny( pStatsList, STATS_SFX_ATTACK, NULL );
			break;

		case STRING_TOKEN_RANGE:
			pnValues[ nStatsToPrint ] = (float) SkillGetRange( tIn.pUnit, tIn.nSkill, NULL, FALSE, tIn.nSkillLevel );
			break;

		case STRING_TOKEN_FIELD_POWERCOST:
			pnValues[ nStatsToPrint ] = (float) (SkillGetPowerCost( tIn.pUnit, tIn.pSkillData, tIn.nSkillLevel, FALSE ) >> StatsGetShift( NULL, STATS_POWER_CUR ));
			break;

		case STRING_TOKEN_DAMAGE_RADIUS:
			{
				STATS * pRider = StatsGetRider( pStatsList, NULL );
				int nValue = 0;
				while ( pRider )
				{
					nValue = StatsGetStat( pRider, STATS_DAMAGE_RADIUS );
					if ( nValue != 0 )
						break;
					pRider = StatsGetRider( pStatsList, pRider );
				}
				 nValue /= (int)RADIUS_DIVISOR;
				 pnValues[ nStatsToPrint ] = (float)nValue;
			}
			break;

		case STRING_TOKEN_FIELD_DURATION:
			{
				STATS * pRider = StatsGetRider( pStatsList, NULL );
				pnValues[ nStatsToPrint ] = 0;
				while ( pRider )
				{
					pnValues[ nStatsToPrint ] = (float)StatsGetStat( pRider, STATS_DAMAGE_FIELD );
					if ( pnValues[ nStatsToPrint ] != 0 )
						break;
					pRider = StatsGetRider( pStatsList, pRider );
				}
			}
			break;

		case STRING_TOKEN_FIELD_RADIUS:
			{
				STATS * pRider = StatsGetRider( pStatsList, NULL );
				int nValue = 0;
				while ( pRider )
				{
					if ( StatsGetStat( pRider, STATS_DAMAGE_FIELD ) )
					{
						nValue = StatsGetStat( pRider, STATS_DAMAGE_RADIUS );
						if ( nValue != 0 )
							break;
					}
					pRider = StatsGetRider( pStatsList, pRider );
				}
				 nValue/= (int)RADIUS_DIVISOR;
				 pnValues[ nStatsToPrint ] = (float)nValue;
			}
			break;

		case STRING_TOKEN_SPECIFIC_STAT:
			{
				ASSERT_CONTINUE( pnOutputToStat[ nStatsToPrint ] != INVALID_ID );
				pnValues[ nStatsToPrint ] = 0;
				for ( int i = 0; i < nStatCount; i++ )
				{
					if ( pStatsEntries[ i ].GetStat() == pnOutputToStat[ nStatsToPrint ] )
					{
						pnValues[ nStatsToPrint ] = (float)pStatsEntries[ i ].value;
						break;
					}
				}
			}
			break;

		default:
			{
				ASSERT_CONTINUE( nStatCount >= pnStatsEntryToOutput[ nStatsToPrint ] );
				ASSERT_CONTINUE( pnStatsEntryToOutput[ nStatsToPrint ] < MAX_STATS_PER_SKILL_CODE );
				ASSERT_CONTINUE( pnStatsEntryToOutput[ nStatsToPrint ] >= 0 );
				if ( pnStatsEntryToOutput[ nStatsToPrint ] >= nStatCount )
					goto cleanup;
				pnValues[ nStatsToPrint ] = (float)pStatsEntries[ pnStatsEntryToOutput[ nStatsToPrint ] ].value;
			}
			break;
		}
		if ( pbNegateValue[ nStatsToPrint ] )
			pnValues[ nStatsToPrint ] = -pnValues[ nStatsToPrint ];
		if ( pbDivideByGameTicksPerSecond[ nStatsToPrint ] )
			pnValues[ nStatsToPrint ] /= GAME_TICKS_PER_SECOND;
		if ( pbDivideByMSPerSecond[ nStatsToPrint ] )
			pnValues[ nStatsToPrint ] /= MSECS_PER_SEC;
		if(pbDivideBy10[nStatsToPrint])
			pnValues[nStatsToPrint] /= 10;
	}

	const WCHAR *puszString = NULL;
	if ( nStatsToPrint > 0 )
	{
		GRAMMAR_NUMBER eNumber = pnValues[ 0 ] > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		puszString = StringTableGetStringByIndexVerbose( tIn.nStringIndex, dwAttributes, 0, NULL, NULL);
		if ( puszString[ 0 ] == 0 && eNumber == PLURAL )
		{
			dwAttributes = StringTableGetGrammarNumberAttribute( SINGULAR );
			puszString = StringTableGetStringByIndexVerbose( tIn.nStringIndex, dwAttributes, 0, NULL, NULL);
		}
	}
	//This wasn't here before but Putting this in now seems like it could break a lot of things.
	//ASSERTX( puszString, "Description of skill not found." );
	if( puszString != NULL )
	{

		PStrPrintfTokenNumberValues( 
			tOut.puszBuffer, 
			tOut.nBufferSize, 
			puszString, 
			SKILL_STRING_TOKEN, 
			pnValues, 
			pbFloat,
			arrsize( pnValues ));
	}

	if ( bDisplayCooldown )
	{
		sCooldownAddToEnd( tIn, tOut );
	}

	if ( sgbDebugStatStrings )
	{
#define DEBUG_STATS_PER_ROW 3

		for ( int i = 0; i < nStatCount && i + DEBUG_STATS_PER_ROW - 1 < MAX_STATS_PER_SKILL_CODE; i += DEBUG_STATS_PER_ROW )
		{
			WCHAR pszFormatString[ 256 ];
			PStrPrintf( pszFormatString, 256, L"%d:[string1] %d:[string2] %d:[string3]\n", i + 1, i + 2, i + 3 ); // assumes DEBUG_STATS_PER_ROW == 3

			int pnDebugValues[ DEBUG_STATS_PER_ROW ];
			for ( int k = 0; k < DEBUG_STATS_PER_ROW; k++ )
			{
				pnDebugValues[ k ] = (i + k) < nStatCount ? pStatsEntries[ i + k ].value : 0;
			}

			WCHAR pszDebugString[ 256 ];
			PStrPrintfTokenIntValues( pszDebugString, 256, pszFormatString, SKILL_STRING_TOKEN, pnDebugValues, DEBUG_STATS_PER_ROW );
			
			if ( i == 0 )
			{
				PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s\n", tOut.puszBuffer);
			}
			PStrPrintf(tOut.puszBuffer, tOut.nBufferSize, L"%s%s", tOut.puszBuffer, pszDebugString);
		}

	}

cleanup:
	if ( pStatsList )
		StatsListFree( pGame, pStatsList );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillStringsToggleDebug()
{
	sgbDebugStatStrings = !sgbDebugStatStrings;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSkillBonus(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut )
{	
	ASSERT( tIn.pSkillData );
	ASSERT( tOut.puszBuffer );
	const SKILL_DATA *pSkillData = tIn.pSkillData;	
	GAME *game = UnitGetGame( tIn.pPlayer );
	//look for variables first
	const WCHAR *puszString = StringTableGetStringByIndex( tIn.nStringIndex ); //this does not need verbose 
	ASSERT( puszString );
	int count( 0 );	
	const WCHAR *uszStringValues[ MAX_SKILL_BONUS_COUNT ];
	const int MAX_INT_VALUE_LEN = 32;
	WCHAR uszIntValuesAsString[ MAX_SKILL_BONUS_COUNT ][ MAX_INT_VALUE_LEN ];
	for( int i = 0; i < MAX_SKILL_BONUS_COUNT; i++ )
	{		
		if( pSkillData->tSkillBonus.nSkillBonusBySkills[ i ] == INVALID_ID )
			break;
		const SKILL_DATA *pSD = SkillGetData( game, pSkillData->tSkillBonus.nSkillBonusBySkills[ i ] );
		if( pSD )
		{
			uszStringValues[i] = StringTableGetStringByIndex( pSD->nDisplayName );
			int nIntValue = SkillGetBonusSkillValue( tIn.pPlayer, pSkillData, i ); 	//pSkillData->tSkillBonus.nSkillBonusByValue[ i ];
			LanguageFormatIntString( uszIntValuesAsString[ i ], MAX_INT_VALUE_LEN, nIntValue );
		}
		count++;
	}
	
	// setup array of string values
	const WCHAR *uszValues[ MAX_SKILL_BONUS_COUNT * 2 ];
	int nFinalValueIndex = 0;
	for (int i = 0; i < MAX_SKILL_BONUS_COUNT; ++i)
	{
		uszValues[ nFinalValueIndex++ ] = uszStringValues[ i ];
		uszValues[ nFinalValueIndex++ ] = uszIntValuesAsString[ i ];
	}

	// print	
	PStrPrintfTokenStringValues(
		tOut.puszBuffer, 
		tOut.nBufferSize, 
		puszString,
		SKILL_STRING_TOKEN,
		uszValues,
		count * 2,
		FALSE);
		
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sStringSkillVar(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut )
{
	int nStatsToPrint = 0;	
	int	pnValues[ MAX_VALUES_PER_SKILL_INFO ];
	//look for variables first
	for( int i = 0; i < MAX_VALUES_PER_SKILL_INFO; i++ )
	{
		if( tIn.pLookup->pnIndices[ i ] >= 0 &&
			tIn.pLookup->pnIndices[ i ] < SKILL_VARIABLE_COUNT )
		{
			SKILL_VARIABLES index( (SKILL_VARIABLES)( tIn.pLookup->pnIndices[ i ] ) );
			if( tIn.pSkillData->codeVariables[ index ] )
			{
				//get the skill varaible
				pnValues[ nStatsToPrint++ ] = SkillGetVariable( tIn.pUnit, NULL, tIn.pSkillData, index, tIn.nSkillLevel );
				if( nStatsToPrint >= MAX_VALUES_PER_SKILL_INFO )
					break;	//break. Asking for to many stats.
			}
		}
	}

	const WCHAR *puszString = NULL;
	if ( nStatsToPrint > 0 )
	{
		GRAMMAR_NUMBER eNumber = pnValues[ 0 ] > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		puszString = StringTableGetStringByIndexVerbose( tIn.nStringIndex, dwAttributes, 0, NULL, NULL);
	}
	if( puszString )
	{
		PStrPrintfTokenIntValues(
			tOut.puszBuffer, 
			tOut.nBufferSize, 
			puszString, 
			SKILL_STRING_TOKEN,
			pnValues,
			arrsize( pnValues ));
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInfoScript(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	ASSERT_RETURN (tIn.nStringIndex != INVALID_LINK);

	GAME * pGame = UnitGetGame( tIn.pUnit );
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeInfoScript, &code_len );
	if ( ! code_ptr )
		return;

	ASSERT_RETURN( code_ptr );

	int nValue = VMExecI(pGame, tIn.pPlayer, tIn.nSkill, tIn.nSkillLevel, tIn.nSkill, tIn.nSkillLevel, INVALID_ID, code_ptr, code_len);
	if ( ! nValue )
		return;

	const WCHAR *puszString = StringTableGetStringByIndex( tIn.nStringIndex );

	PStrPrintfTokenIntValues(
		tOut.puszBuffer, 
		tOut.nBufferSize, 
		puszString, 
		SKILL_STRING_TOKEN,
		&nValue,
		1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEventParams(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	ASSERT_RETURN (tIn.nStringIndex != INVALID_LINK);


	const SKILL_EVENT * pSkillEvent = NULL;
	{ // find the event that has the params that we need
		SKILL_EVENTS_DEFINITION * pEvents = ( tIn.pSkillData->nEventsId != INVALID_ID ) ? (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, tIn.pSkillData->nEventsId ) : NULL;
		if ( ! pEvents )
			return;

		for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
		{
			const SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
			for ( int j = 0; j < pHolder->nEventCount; j++ )
			{
				SKILL_EVENT * pCurr = &pHolder->pEvents[ j ];
				if ( pCurr->nType == INVALID_ID )
					continue;
				const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = (SKILL_EVENT_TYPE_DATA *) ExcelGetData( NULL, DATATABLE_SKILLEVENTTYPES, pCurr->nType );
				if ( pSkillEventTypeData->bParamsUsedInSkillString )
				{
					pSkillEvent = pCurr;
					break;
				}
			}
		}
	}

	ASSERT_RETURN( pSkillEvent );

	int nStatsToPrint = 0;
	int	pnValues[ MAX_VALUES_PER_SKILL_INFO ];
	for ( ; nStatsToPrint < MAX_VALUES_PER_SKILL_INFO; nStatsToPrint++ )
	{
		if ( tIn.pLookup->pnIndices[ nStatsToPrint ] == -1 )
			break;
		if ( tIn.pLookup->pnIndices[ nStatsToPrint ] == STRING_TOKEN_DURATION )
		{
			pnValues[ nStatsToPrint ] = SkillDataGetStateDuration(UnitGetGame(tIn.pUnit), tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, FALSE);
			pnValues[ nStatsToPrint ] /= GAME_TICKS_PER_SECOND;
		} else {
			ASSERT_CONTINUE( tIn.pLookup->pnIndices[ nStatsToPrint ] >= 0 );
			ASSERT_CONTINUE( tIn.pLookup->pnIndices[ nStatsToPrint ] < MAX_SKILL_EVENT_PARAM );

			float fValue = SkillEventGetParamFloat(tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, pSkillEvent, tIn.pLookup->pnIndices[ nStatsToPrint ]);

			if ( tIn.pLookup->pnParams[ nStatsToPrint ] != INVALID_ID )
			{
				ASSERT_CONTINUE( tIn.pLookup->pnParams[ nStatsToPrint ] >= 0 );
				ASSERT_CONTINUE( tIn.pLookup->pnParams[ nStatsToPrint ] < MAX_SKILL_EVENT_PARAM );

				float fPerLevel = SkillEventGetParamFloat(tIn.pUnit, tIn.nSkill, tIn.nSkillLevel, pSkillEvent, tIn.pLookup->pnParams[ nStatsToPrint ]);
				fValue += fPerLevel * tIn.nSkillLevel;
			}
			if ( tIn.pLookup->bPercent[ nStatsToPrint ] )
				fValue *= 100.0f;
			pnValues[ nStatsToPrint ] = (int)fValue;
		}
	}

	const WCHAR *puszString = NULL;
	if ( nStatsToPrint > 0 )
	{
		GRAMMAR_NUMBER eNumber = pnValues[ 0 ] > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		puszString = StringTableGetStringByIndexVerbose( tIn.nStringIndex, dwAttributes, 0, NULL, NULL);
		if ( puszString[ 0 ] == 0 && eNumber == PLURAL )
		{
			dwAttributes = StringTableGetGrammarNumberAttribute( SINGULAR );
			puszString = StringTableGetStringByIndexVerbose( tIn.nStringIndex, dwAttributes, 0, NULL, NULL);
		}
	}

	PStrPrintfTokenIntValues(
		tOut.puszBuffer, 
		tOut.nBufferSize, 
		puszString, 
		SKILL_STRING_TOKEN,
		pnValues,
		arrsize( pnValues ));
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_SKILLS_IN_LIST 10
static void sListSkills(
	const SKILL_STRING_FUNCTION_INPUT &tIn,
	SKILL_STRING_FUNCTION_OUTPUT &tOut)
{
	GAME * pGame = UnitGetGame( tIn.pUnit );

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, tIn.pSkillData->codeInfoScript, &code_len );	
	if ( ! code_ptr )
		return;

	STATS * pStats = StatsListInit( pGame );
	int nValue = VMExecI(pGame, tIn.pPlayer, pStats, tIn.nSkill, tIn.nSkillLevel, tIn.nSkill, tIn.nSkillLevel, INVALID_ID, code_ptr, code_len);
	if ( ! nValue )
		return;

	STATS_ENTRY pStatsEntries[ MAX_SKILLS_IN_LIST ];
	int nSkillCount = StatsGetRange( pStats, STATS_SKILL_LEVEL, 0, pStatsEntries, MAX_SKILLS_IN_LIST );
	ASSERT( nSkillCount < MAX_SKILLS_IN_LIST );

	for ( int i = 0; i < nSkillCount; i++ )
	{
		if ( pStatsEntries[ i ].value > tIn.nSkillLevel )
			continue; // only display skills that are the proper level

		int nSkillToList = StatGetParam(STATS_SKILL_LEVEL, pStatsEntries[i].GetParam(), 0);
		if ( nSkillToList == INVALID_ID )
			continue;

		const SKILL_DATA * pSkillToList = SkillGetData( pGame, nSkillToList );

		const WCHAR *puszSkillName = StringTableGetStringByIndex( pSkillToList->nDisplayName );
		const WCHAR *puszFormatString = StringTableGetStringByIndex( tIn.nStringIndex );

		WCHAR puszStringToAppend[ 512 ];
		PStrPrintfTokenStringValues( 
			puszStringToAppend, 
			512, 
			puszFormatString, 
			SKILL_STRING_TOKEN,
			&puszSkillName,
			1,
			FALSE);

		PStrCat( tOut.puszBuffer, puszStringToAppend, tOut.nBufferSize );
	}
}

//----------------------------------------------------------------------------
static SKILL_STRING_FUNCTION_LOOKUP sgtSkillInfoTable[] = 
{
	{ "Simple",			sSimple,		},
	{ "MultiBullets",	sMultiBullets,	},
	{ "DurationOnlyf",	sDurationf,		},
	{ "DurationOnly",	sDuration,		},
	{ "RangeOnly",		sRange,			},
	{ "CooldownOnly",	sCooldown,		},
	{ "List Skills",	sListSkills,	},
	{ "Stats",			sStringWithStatValue,	},
	{ "Events 1P",		sEventParams,	{ 0, -1, -1, -1, -1},	{ -1, -1, -1, -1, -1},	{ TRUE, FALSE, FALSE, FALSE, FALSE},	},
	// Must have this one here, so that it matches 1P over 1
	{ "Events 1",		sEventParams,	{ 0, -1, -1, -1, -1},	{ -1, -1, -1, -1, -1},	{ FALSE, FALSE, FALSE, FALSE, FALSE},	},
	{ "Events 2+3P",	sEventParams,	{ 1, -1, -1, -1, -1},	{ 2, -1, -1, -1, -1},	{ TRUE, FALSE, FALSE, FALSE, FALSE},	},
	{ "Skills 0123",	sStringSkillVar,	{ 0, 1, 2, 3, 4},	},
	{ "Skills 5678",	sStringSkillVar,	{ 5, 6, 7, 8, 9},	},
	{ "Skills 3012",	sStringSkillVar,	{ 3, 0, 1, 2, 0},	},
	{ "Skills 4203",	sStringSkillVar,	{ 4, 2, 0, 3, 0},	},
	{ "Skills 3201",	sStringSkillVar,	{ 3, 2, 0, 1, 0},	},
	{ "Skills 1023",	sStringSkillVar,	{ 1, 0, 2, 3, 4},	},
	{ "Skills 4213",	sStringSkillVar,	{ 4, 2, 1, 3, 0},	},
	{ "Skills 3210",	sStringSkillVar,	{ 3, 2, 1, 0, 0},	},
	{ "Skills 4013",	sStringSkillVar,	{ 4, 0, 1, 3, 0},	},
	{ "Skills 1234",	sStringSkillVar,	{ 1, 2, 3, 4, 5},	},
	{ "Skills 2345",	sStringSkillVar,	{ 2, 3, 4, 5, 6},	},
	{ "SkillBonus",		sSkillBonus,	},
};
static int sgnNumSkillInfoTable = arrsize( sgtSkillInfoTable );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const SKILL_STRING_FUNCTION_LOOKUP *sFindSkillInfoLookup(
	int nSkill,
	SKILL_STRING_TYPES eType )
{
	// We can't call this because it returns NULL - if we have not yet cached the
	// skill info function index, then we need to write it, so we must needs get
	// the skill data the old-fashioned way
	//const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	SKILL_DATA *pSkillData = (SKILL_DATA *)ExcelGetData(NULL, DATATABLE_SKILLS, nSkill);

	if(pSkillData->pnStringFunctions[ eType ] >= 0)
	{
		return &sgtSkillInfoTable[pSkillData->pnStringFunctions[ eType ]];
	}

	// loop through table
	for (int i = 0; i < sgnNumSkillInfoTable; ++i)
	{
		const SKILL_STRING_FUNCTION_LOOKUP *pLookup = &sgtSkillInfoTable[ i ];
		if (PStrICmp( pLookup->szName, pSkillData->pszStringFunctions[ eType ], PStrLen( pLookup->szName ) ) == 0)
		{
			pSkillData->pnStringFunctions[ eType ] = i;
			return pLookup;
		}
	}
	
	if(pSkillData->pszStringFunctions[ eType ][0] != '\0')
	{
		ShowDataWarning("Skill String function '%s' not found at skill %s", pSkillData->pszStringFunctions[ eType ], pSkillData->szName);
	}

	// not found
	return NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillGetString(
	int nSkill,
	UNIT *pUnit,
	SKILL_STRING_TYPES eStringType,	
	SKILL_TOOLTIP_TYPE eTooltipType,
	WCHAR *puszBuffer,
	int nBufferSize,
	DWORD dwFlags,
	BOOL bWithFormatting, /* = TRUE */
	int nSkillLevel	) /* = 0 */
{
	ASSERTX_RETFALSE( nSkill != INVALID_LINK, "Invalid skill" );
	ASSERTX_RETFALSE( pUnit, "Expected Unit" );
	ASSERTX_RETFALSE( puszBuffer, "Expected string buffer" );
	ASSERTX_RETFALSE( nBufferSize > 0, "Invalid buffer size" );
	ASSERT_RETFALSE( eStringType >= 0 );
	ASSERT_RETFALSE( eStringType < NUM_SKILL_STRINGS );
					
	// initialize buffer
	puszBuffer[ 0 ] = 0;
	GAME * pGame = UnitGetGame( pUnit );
		
	// fill out the input struct 
	SKILL_STRING_FUNCTION_INPUT tIn;
	tIn.nSkill = nSkill;
	tIn.pSkillData = SkillGetData( NULL, nSkill );
	ASSERT_RETFALSE( tIn.pSkillData );
	tIn.nStringIndex = tIn.pSkillData->pnStrings[ eStringType ];
	tIn.eStringType = eStringType;
	tIn.pUnit = pUnit;
	tIn.pPlayer = GameGetControlUnit( UnitGetGame( pUnit ) );
	ASSERTX_RETFALSE( tIn.pPlayer, "Expected to have a control unit" );
	tIn.nSkillLevel = (nSkillLevel > 0) ? nSkillLevel : SkillGetLevel( pUnit, nSkill );
	tIn.nSkillLevelUnmodified = tIn.nSkillLevel;
	if ( eTooltipType == STT_NEXT_LEVEL )
		tIn.nSkillLevel++;
	if ( tIn.nSkillLevel == 0 )
		tIn.nSkillLevel = 1;

	// find the lookup entry
	tIn.pLookup = sFindSkillInfoLookup( nSkill, eStringType );
	if (tIn.pLookup )
	{		
		BOOL bHasString( FALSE );
		{
			// fill out output struct		
			WCHAR uszEffect[ MAX_SKILL_TOOLTIP ];
			uszEffect[ 0 ] = 0;		
			SKILL_STRING_FUNCTION_OUTPUT tOut;
			tOut.puszBuffer = uszEffect;
			tOut.nBufferSize = MAX_SKILL_TOOLTIP;
					
			// do callback
			tIn.pLookup->pfnInfo( tIn, tOut );

			// put into buffer
			bHasString = PStrLen( tOut.puszBuffer ) > 0;
			if ( bHasString && bWithFormatting )
			{			
				GLOBAL_STRING stringIndex( GS_SKILL_EFFECT );
				switch( eStringType )
				{
				case SKILL_STRING_EFFECT_ACCUMULATION:
					stringIndex = GS_SKILL_EFFECT_ACCUMULATION;
					break;
				case SKILL_STRING_EFFECT:		
					stringIndex = (dwFlags & SKILL_STRING_FLAG_NO_TITLE) != 0 ? GS_SKILL_EFFECT_NO_TITLE : GS_SKILL_EFFECT;
					break;
				case SKILL_STRING_DESCRIPTION:
					stringIndex = GS_SKILL_DESCRIPTION;
					break;
				case SKILL_STRING_SKILL_BONUSES:
					stringIndex = GS_SKILL_SKILLBONUS;
					break;
				}			
				PStrPrintf(
					puszBuffer,
					nBufferSize,
					GlobalStringGet( stringIndex ),
					tOut.puszBuffer);
			}
			else if( bHasString )
			{
				PStrCopy(
					puszBuffer,
					tOut.puszBuffer,
					PStrLen( tOut.puszBuffer ) + 1 );
			}
		}

		{
			// append power cost
			WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
			if( (AppIsHellgate() || eStringType != SKILL_STRING_DESCRIPTION ) &&
				( tIn.pSkillData->nPowerCost || tIn.pSkillData->eUsage == USAGE_PASSIVE ||
				  tIn.pSkillData->eUsage == USAGE_TOGGLE ) )
			{
				int nPowerCost;
				int nNextCost;
				GLOBAL_STRING ePowerCost;
				if ( tIn.pSkillData->nPowerCost) 
				{
					nPowerCost = SkillGetPowerCost( tIn.pPlayer, tIn.pSkillData, tIn.nSkillLevel, FALSE );
					nNextCost = SkillGetPowerCost( tIn.pPlayer, tIn.pSkillData, tIn.nSkillLevel + 1, FALSE );
					int nMaxPower = UnitGetStat( tIn.pPlayer, STATS_POWER_MAX );
					ePowerCost = nPowerCost > nMaxPower ? GS_SKILL_POWER_COST_TOO_HIGH : GS_SKILL_POWER_COST;
				}
				else
				{
					ePowerCost = GS_SKILL_UPKEEP_COST;
					nPowerCost = SkillGetSelectCost( pUnit, nSkill, tIn.pSkillData, tIn.nSkillLevel );
					nNextCost = SkillGetPowerCost( tIn.pPlayer, tIn.pSkillData, tIn.nSkillLevel < MAX_LEVELS_PER_SKILL ? ( tIn.nSkillLevel + 1 ) : tIn.nSkillLevel, FALSE );
				}
				BOOL bPrint = nPowerCost != 0;
				if( AppIsHellgate() )
				{
					if ( ( dwFlags & SKILL_STRING_FLAG_NO_POWER_COST_IF_SAME ) != 0 && nNextCost == nPowerCost )
						bPrint = FALSE;
					if ( ( dwFlags & SKILL_STRING_FLAG_NO_POWER_COST_IF_SAME) == 0 && nNextCost != nPowerCost )
						bPrint = FALSE;
				}
				if ( bPrint )	
				{
				
					//if( (dwFlags & SKILL_STRING_FLAG_NO_POWER_COST_IF_SAME ) )
					//	nPowerCost = nNextCost;
					float fPowerCost = StatsGetAsFloat( STATS_POWER_CUR, nPowerCost );					
					
					// get value as language formatted string
					const int MAX_NUMBER = 32;
					WCHAR uszNumber[ MAX_NUMBER ];					
					LanguageFormatFloatString( uszNumber, MAX_NUMBER, fPowerCost, 1 );
					
					// plug number into string	
					PStrCopy( uszTemp, GlobalStringGet( ePowerCost ), MAX_SKILL_TOOLTIP );
					const WCHAR *puszToken = L"[string1]";
					PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );
					
					if ( bHasString )
					{			
						PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
					}
					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
					bHasString = TRUE;
				}
			}
		}

		if ( SkillDataTestFlag( tIn.pSkillData, SKILL_FLAG_DISPLAY_RANGE ) )
		{
			// append range
			WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];

			float fRange = SkillGetRange( tIn.pUnit, tIn.nSkill, NULL, FALSE, tIn.nSkillLevel );
			if( fRange != 0.0f )
			{
				float fRangeNext = SkillGetRange( tIn.pUnit, tIn.nSkill, NULL, FALSE, tIn.nSkillLevel+1 );

				BOOL bPrint = TRUE;
				if ( ( dwFlags & SKILL_STRING_FLAG_NO_RANGE_IF_SAME ) != 0 && fRange == fRangeNext )
					bPrint = FALSE;
				if ( ( dwFlags & SKILL_STRING_FLAG_NO_RANGE_IF_SAME) == 0 && fRange != fRangeNext )
					bPrint = FALSE;
				if ( bPrint )	
				{

					// get number as string
					const int MAX_NUMBER = 32;
					WCHAR uszNumber[ MAX_NUMBER ];					
					LanguageFormatFloatString( uszNumber, MAX_NUMBER, fRange, 1 );

					// plug into string
					const WCHAR *puszToken = L"[string1]";					
					PStrCopy( uszTemp, GlobalStringGet( GS_SKILL_RANGE ), MAX_SKILL_TOOLTIP );
					PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );

					if ( bHasString )
					{			
						PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
					}
					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
					bHasString = TRUE;
				}
			}
		}

		if ( SkillDataTestFlag( tIn.pSkillData, SKILL_FLAG_DISPLAY_COOLDOWN ) )
		{
			// append range
			WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];

			int nTicks = SkillGetCooldown( pGame, tIn.pUnit, NULL, tIn.nSkill, tIn.pSkillData, 0, tIn.nSkillLevel );
			if( nTicks )
			{
				int nTicksNext = SkillGetCooldown( pGame, tIn.pUnit, NULL, tIn.nSkill, tIn.pSkillData, 0, tIn.nSkillLevel+1 );

				BOOL bPrint = TRUE;
				if ( ( dwFlags & SKILL_STRING_FLAG_NO_COOLDOWN_IF_SAME ) != 0 && nTicks == nTicksNext )
					bPrint = FALSE;
				if( AppIsHellgate() )
				{
					//tugboat always wants to see the cool down...
					if ( ( dwFlags & SKILL_STRING_FLAG_NO_COOLDOWN_IF_SAME) == 0 && nTicks != nTicksNext )
						bPrint = FALSE;
				}
				if ( bPrint )	
				{
					float flValue = (float)nTicks / GAME_TICKS_PER_SECOND_FLOAT;
					
					// get number as string
					const int MAX_NUMBER = 32;
					WCHAR uszNumber[ MAX_NUMBER ];					
					LanguageFormatFloatString( uszNumber, MAX_NUMBER, flValue, 1 );

					// plug into string
					const WCHAR *puszToken = L"[string1]";
					PStrCopy(uszTemp, GlobalStringGet( GS_SKILL_COOLDOWN ), MAX_SKILL_TOOLTIP );
					PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );
				
					if ( bHasString )
					{			
						PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
					}
					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
					bHasString = TRUE;
				}
			}
		}	

		{
			// append perk point cost
			WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];
			if( SkillUsesPerkPoints(tIn.pSkillData) && tIn.nSkillLevel >= 0 && eStringType == SKILL_STRING_DESCRIPTION )
			{
				int nPerkPointsRequired = SkillGetPerkPointsRequiredForNextLevel(tIn.pSkillData, tIn.nSkillLevelUnmodified);
				int nPlayerPerkPoints = UnitGetStat(tIn.pUnit, STATS_PERK_POINTS);
				GLOBAL_STRING ePerkPointCost = nPlayerPerkPoints >= nPerkPointsRequired ? GS_SKILL_PERK_POINT_COST : GS_SKILL_PERK_POINT_COST_TOO_HIGH;

				BOOL bPrint = nPerkPointsRequired > 0;
				if ( bPrint )	
				{
					// get value as language formatted string
					const int MAX_NUMBER = 32;
					WCHAR uszNumber[ MAX_NUMBER ];					
					LanguageFormatIntString( uszNumber, MAX_NUMBER, nPerkPointsRequired);

					// plug number into string	
					PStrCopy( uszTemp, GlobalStringGet( ePerkPointCost ), MAX_SKILL_TOOLTIP );
					const WCHAR *puszToken = L"[string1]";
					PStrReplaceToken( uszTemp, MAX_SKILL_TOOLTIP, puszToken, uszNumber );

					if ( bHasString )
					{			
						PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
					}
					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
					bHasString = TRUE;
				}
			}
		}

		// info was found
		return bHasString;
		
	}
	
	return FALSE;  // no lookup found
	
}
