#include "runnerstd.h"
#include "playertrace.h"
#include "xfer.h"    // required by dbunit.h
#include "dbunit.h"
#include "playertrace.cpp.tmh"

// this value doesn't matter since it is overwritten upon runner init
static volatile LONG sbEnablePlayerActionLogging = TRUE;

void PlayerActionLoggingEnable(BOOL bEnable)
{
	InterlockedExchange(&sbEnablePlayerActionLogging, bEnable);
}

BOOL PlayerActionLoggingIsEnabled()
{
	return (sbEnablePlayerActionLogging == TRUE);
}

void TracePlayerAccountLogin(
	UNIQUE_ACCOUNT_ID idAccount)
{
	SYSTEMTIME tTime;	
	GetSystemTime( &tTime );

	// 1 param: account ID
	DoTraceMessage(PLAYER_ACTION_FLAG,
		"LogVersion=%d|"		\
		"ActionType=%d|"		\
		"UtcTime=%04i-%02i-%02iT%02i:%02i:%02i.%03iZ|" \
		"AccountId=0x%I64d",
		PLAYER_LOG_VERSION,
		PLAYER_LOG_ACCOUNT_LOGIN,
		tTime.wYear, tTime.wMonth, tTime.wDay, tTime.wHour, tTime.wMinute, tTime.wSecond, tTime.wMilliseconds,
		idAccount );
}

void TracePlayerCharacterAction(
	PLAYER_LOG_TYPE actionType,
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName)
{
	ASSERT_RETURN(wszCharacterName);
	SYSTEMTIME tTime;	
	GetSystemTime( &tTime );

	// 3 params: account ID, character GUID, character name
	DoTraceMessage(PLAYER_ACTION_FLAG,
		"LogVersion=%d|"			\
		"ActionType=%d|"			\
		"UtcTime=%04i-%02i-%02iT%02i:%02i:%02i.%03iZ|" \
		"AccountId=0x%I64d|"		\
		"CharacterGuid=0x%I64d|"	\
		"CharacterName=%S",
		PLAYER_LOG_VERSION,
		actionType,
		tTime.wYear, tTime.wMonth, tTime.wDay, tTime.wHour, tTime.wMinute, tTime.wSecond, tTime.wMilliseconds,
		idAccount,
		guidCharacter,
		wszCharacterName );
}

void TracePlayerItemAction(
	PLAYER_LOG_TYPE actionType,			// see PLAYER_LOG_TYPE
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName,
	ITEM_PLAYER_LOG_DATA* pItemData)
{
	ASSERT_RETURN(pItemData);
	ASSERT_RETURN(wszCharacterName);
	SYSTEMTIME tTime;
	GetSystemTime( &tTime );

	// item-related action
	DoTraceMessage(PLAYER_ACTION_FLAG,
		"LogVersion=%d|"			\
		"ActionType=%d|"			\
		"UtcTime=%04i-%02i-%02iT%02i:%02i:%02i.%03iZ|" \
		"AccountId=0x%I64d|"		\
		"CharacterGuid=0x%I64d|"	\
		"CharacterName=%S|"			\
		"UnitGuid=0x%I64d|"			\
		"UnitDevName=%s|"			\
		"UnitDisplayName=%S|"		\
		"UnitLevel=%d|"				\
		"UnitQuality=%s|"			\
		"UnitDescription=%S",
		PLAYER_LOG_VERSION,
		actionType,
		tTime.wYear, tTime.wMonth, tTime.wDay, tTime.wHour, tTime.wMinute, tTime.wSecond, tTime.wMilliseconds,
		idAccount,
		guidCharacter,
		wszCharacterName,
		pItemData->guidUnit,
		pItemData->pszDevName ? pItemData->pszDevName : "NULL",
		pItemData->wszTitle[0] ? pItemData->wszTitle : L"NULL",
		pItemData->nLevel,
		pItemData->pszQuality ? pItemData->pszQuality : "NULL",
		pItemData->wszDescription[0] ? pItemData->wszDescription : L"NULL" );
}

static BOOL sByteArrayToHexString(
	char * dest,
	DWORD destlen,
	const BYTE * src,
	DWORD srclen)
{
	ASSERT_RETFALSE( src );
	ASSERT_RETFALSE( dest );
	ASSERT_RETFALSE( destlen >= 2 * srclen );

	static const char ssHex[16] =
	{
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};

	for (DWORD ii = 0; ii < srclen; ++ii)
	{
		dest[2*ii] = ssHex[((src[ii] >> 4) & 0xF)]; // high nibble
		dest[2*ii+1] = ssHex[(src[ii]) & 0xF];      // low nibble
	}

	dest[destlen - 1] = '\0';

	return TRUE;
}

void TracePlayerItemActionWithBlobData(
	PLAYER_LOG_TYPE actionType,			// see PLAYER_LOG_TYPE
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName,
	ITEM_PLAYER_LOG_DATA* pItemData,
	const DATABASE_UNIT* pDBUnit,
	const BYTE* pDataBuffer,
	const WCHAR* wszTraderCharacterName/* = NULL*/)
{
	ASSERT_RETURN(pItemData);
	ASSERT_RETURN(wszCharacterName);
	ASSERT_RETURN(pDBUnit);
	ASSERT_RETURN(pDataBuffer);
	SYSTEMTIME tTime;	
	GetSystemTime( &tTime );

	char szHexData[2 * UNITFILE_MAX_SIZE_SINGLE_UNIT + 1];
	ASSERTX_RETURN(
		sByteArrayToHexString(
			szHexData,
			2 * pDBUnit->dwDataSize + 1,
			pDataBuffer,
			pDBUnit->dwDataSize ),
		"Error converting unit data to hex string; transaction not logged!" );

	// item-related action w/ binary blob and DB unit data
	DoTraceMessage(PLAYER_ACTION_FLAG,
		"LogVersion=%d|"			\
		"ActionType=%d|"			\
		"UtcTime=%04i-%02i-%02iT%02i:%02i:%02i.%03iZ|" \
		"AccountId=0x%I64d|"		\
		"CharacterGuid=0x%I64d|"	\
		"CharacterName=%S|"			\
		"TraderName=%S|"				\
		"UnitGuid=0x%I64d|"			\
		"UnitDevName=%s|"			\
		"UnitDisplayName=%S|"		\
		"UnitLevel=%d|"				\
		"UnitQuality=%s|"			\
		"UnitDescription=%S|"		\
		"UnitGenusEnum=%d|"			\
		"UnitClassCode=%d|"			\
		"UnitSecondsPlayed=%d|"		\
		"UnitExperience=%d|"		\
		"UnitMoney=%d|"				\
		"UnitQuantity=%d|"			\
		"UnitGuidUltimateOwner=0x%I64d|" \
		"UnitGuidContainer=0x%I64d|"	\
		"UnitInvLocCode=%d|"		\
		"UnitInvLocX=%d|"			\
		"UnitInvLocY=%d|"			\
		"UnitBlobData=0x%s",
		PLAYER_LOG_VERSION,
		actionType,
		tTime.wYear, tTime.wMonth, tTime.wDay, tTime.wHour, tTime.wMinute, tTime.wSecond, tTime.wMilliseconds,
		idAccount,
		guidCharacter,
		wszCharacterName,
		wszTraderCharacterName ? wszTraderCharacterName : L"NULL",
		pItemData->guidUnit,
		pItemData->pszDevName ? pItemData->pszDevName : "NULL",
		pItemData->wszTitle[0] ? pItemData->wszTitle : L"NULL",
		pItemData->nLevel,
		pItemData->pszQuality ? pItemData->pszQuality : "NULL",
		pItemData->wszDescription[0] ? pItemData->wszDescription : L"NULL",
		pDBUnit->eGenus,
		pDBUnit->dwClassCode,
		pDBUnit->dwSecondsPlayed,
		pDBUnit->dwExperience,
		pDBUnit->dwMoney,
		pDBUnit->dwQuantity,
		pDBUnit->guidUltimateOwner,
		pDBUnit->guidContainer,
		pDBUnit->tDBUnitInvLoc.dwInvLocationCode,
		pDBUnit->tDBUnitInvLoc.tInvLoc.nX,
		pDBUnit->tDBUnitInvLoc.tInvLoc.nY,
		szHexData );
}
