//----------------------------------------------------------------------------
// playertrace.h
// 
// Modified     : $DateTime$
// by           : $Author$
//
// Functions and data structures for player transaction logging.
//
// (C)Copyright 2007, Ping0. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _PLAYERTRACE_H_
#define _PLAYERTRACE_H_

#ifndef WPP_PLAYER_ACTION_GUID_DEFINED
#define WPP_PLAYER_ACTION_GUID_DEFINED
const GUID PlayerAction_WPPControlGuid = {
	// {0DE1BE20-8FC3-4f64-9C94-D78A966AF2A5}
	0xde1be20,
	0x8fc3,
	0x4f64,
	{0x9c,0x94,0xd7,0x8a,0x96,0x6a,0xf2,0xa5}
};
#else
extern const GUID PlayerAction_WPPControlGuid;
#endif //!WPP_PLAYER_ACTION_GUID_DEFINED

#define PLAYER_LOG_VERSION					3

#define MAX_ITEM_PLAYER_LOG_STRING_LENGTH	1024

static const char * sgszPlayerActionLogName = "PlayerTrace";

enum PLAYER_LOG_TYPE
{
	PLAYER_LOG_ACCOUNT_LOGIN		= 1,	// saved in database, do not change value
	PLAYER_LOG_CHAR_LOGIN			= 2,	// saved in database, do not change value
	PLAYER_LOG_CHAR_CREATE			= 3,	// saved in database, do not change value
	PLAYER_LOG_CHAR_DELETE			= 4,	// saved in database, do not change value
	
	PLAYER_LOG_ITEM_GAIN_BUY		= 11,	// saved in database, do not change value
	PLAYER_LOG_ITEM_GAIN_TRADE		= 12,	// saved in database, do not change value
	PLAYER_LOG_ITEM_GAIN_PICKUP		= 13,	// saved in database, do not change value
	PLAYER_LOG_ITEM_GAIN_MAIL		= 14,	// saved in database, do not change value
	PLAYER_LOG_ITEM_GAIN_AUCTION	= 15,	// saved in database, do not change value
	PLAYER_LOG_ITEM_GAIN_MAILBOUNCE = 16,	// saved in database, do not change value

	PLAYER_LOG_ITEM_LOST_SELL		= 21,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_TRADE		= 22,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_DROP		= 23,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_MAIL		= 24,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_AUCTION	= 25,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_DISMANTLE	= 26,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_DESTROY	= 27,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_UPGRADE	= 28,	// saved in database, do not change value
	PLAYER_LOG_ITEM_LOST_UNKNOWN	= 29,	// saved in database, do not change value
};

struct ITEM_PLAYER_LOG_DATA
{
	PGUID guidUnit;
	int nLevel;
	const char* pszGenus;
	const char* pszDevName;
	const char* pszQuality;
	WCHAR wszTitle[MAX_ITEM_NAME_STRING_LENGTH];				// display name
	WCHAR wszDescription[MAX_ITEM_PLAYER_LOG_STRING_LENGTH];	// human-readable item info

	ITEM_PLAYER_LOG_DATA::ITEM_PLAYER_LOG_DATA()
		: guidUnit(INVALID_GUID), nLevel(-1), pszGenus(NULL), pszDevName(NULL), pszQuality(NULL)
	{
		wszTitle[0] = L'\0';
		wszDescription[0] = L'\0';
	}
};

void PlayerActionLoggingEnable(BOOL bEnable);
BOOL PlayerActionLoggingIsEnabled();

void TracePlayerAccountLogin(
	UNIQUE_ACCOUNT_ID idAccount);

void TracePlayerCharacterAction(
	PLAYER_LOG_TYPE actionType,			// see PLAYER_LOG_TYPE
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName);

void TracePlayerRankGain(
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName,
	int nRank);

void TracePlayerItemAction(
	PLAYER_LOG_TYPE actionType,			// see PLAYER_LOG_TYPE
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName,
	ITEM_PLAYER_LOG_DATA* pItemData);

void TracePlayerItemActionWithBlobData(
	PLAYER_LOG_TYPE actionType,			// see PLAYER_LOG_TYPE
	UNIQUE_ACCOUNT_ID idAccount,
	PGUID guidCharacter,
	const WCHAR* wszCharacterName,
	ITEM_PLAYER_LOG_DATA* pItemData,
	const struct DATABASE_UNIT* pDBUnit,
	const BYTE* pDataBuffer,
	const WCHAR* wszTraderCharacterName = NULL);

#endif
