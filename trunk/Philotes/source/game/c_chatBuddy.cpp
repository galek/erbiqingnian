//----------------------------------------------------------------------------
// c_chatBuddy.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_chatBuddy.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "c_connmgr.h"
#include "EmbeddedServerRunner.h"
#include "UserChatCommunication.h"
#include "BuddySystem.h"
#include "uix.h"
#include "console.h"
#include "chat.h"
#include "c_chatNetwork.h"
#include "uix_components.h"
#include "uix_social.h"
#include "globalindex.h"
#include "units.h"


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
struct BUDDY_CLIENT_CONTEXT
{
	CLIENT_BUDDY_ID				BuddyLinkId;
	PGUID						BuddyCharGuid;
	WCHAR						BuddyCharName[MAX_CHARACTER_NAME];
	WCHAR						BuddyNickName[MAX_CHARACTER_NAME];
	INT32						BuddyCharacterClass;
	INT32						BuddyCharacterLevel;
	INT32						BuddyCharacterRank;
	INT32						BuddyLevelDefinitionId;
	INT32						BuddyAreaId;
	INT32						BuddyAreaDepth;
};

static BUDDY_CLIENT_CONTEXT		BuddyList[MAX_CHAT_BUDDIES];


//----------------------------------------------------------------------------
// PRIVATE HELPER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BUDDY_CLIENT_CONTEXT * sFindBuddyByCharName(
	const WCHAR * wszName)
{
	for (UINT ii = 0; ii < MAX_CHAT_BUDDIES; ++ii)
	{
		if (PStrICmp(wszName, BuddyList[ii].BuddyCharName) == 0)
		{
			return &BuddyList[ii];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BUDDY_CLIENT_CONTEXT * sFindBuddyByNickName(
	const WCHAR * wszName)
{
	for (UINT ii = 0; ii < MAX_CHAT_BUDDIES; ++ii)
	{
		if (PStrICmp(wszName, BuddyList[ii].BuddyNickName) == 0)
		{
			return &BuddyList[ii];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BUDDY_CLIENT_CONTEXT * sFindBuddyByLinkId(
	CLIENT_BUDDY_ID buddyId)
{
	for (UINT ii = 0; ii < MAX_CHAT_BUDDIES; ++ii)
	{
		if (BuddyList[ii].BuddyLinkId == buddyId)
		{
			return &BuddyList[ii];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BUDDY_CLIENT_CONTEXT * sFindAvailableBuddyEntry(
	void )
{
	for (UINT ii = 0; ii < MAX_CHAT_BUDDIES; ++ii)
	{
		if (BuddyList[ii].BuddyLinkId == INVALID_BUDDY_ID)
		{
			return &BuddyList[ii];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBuddyError(
	LPCWSTR szHeaderString,
	LPCWSTR szErrorString,
	LPCWSTR szTargetName = NULL)
{
	if (szHeaderString && szErrorString)
	{
		WCHAR szBuddyError[256];
		PStrPrintf(szBuddyError, arrsize(szBuddyError), szErrorString, WSTR_ARG(szTargetName));
		UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), szBuddyError);
		UIShowGenericDialog(szHeaderString, szBuddyError);
	}
}

//----------------------------------------------------------------------------
// CLIENT BUDDY SYSTEM METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_BuddyListInit()
{
	for (UINT32 i = 0; i < MAX_CHAT_BUDDIES; i++)
	{
		BuddyList[i].BuddyLinkId = INVALID_BUDDY_ID;
		BuddyList[i].BuddyCharGuid = INVALID_GUID;
		BuddyList[i].BuddyCharName[0] = L'\0';
		BuddyList[i].BuddyNickName[0] = L'\0';
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_BuddyOnOpResult(
	void * data )
{
	CHAT_RESPONSE_MSG_OPERATION_RESULT * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_OPERATION_RESULT*)data;

	switch (myMsg->eRequest)
	{
	case USER_REQUEST_ADD_MEMBER_AS_BUDDY:
		{
			LPCWSTR szHeaderString = StringTableGetStringByKey("InteractPlayerBuddyAdd");

			switch (myMsg->eResult)
			{
			case CHAT_ERROR_NONE:
				break;	//	success

			case CHAT_ERROR_MAX_BUDDIED_MEMBERS:
				sBuddyError(szHeaderString, GlobalStringGet(GS_BUDDY_ACCEPT_ERROR_LIMIT));
				break;
			case CHAT_ERROR_MEMBER_NOT_FOUND:
				sBuddyError(szHeaderString, GlobalStringGet(GS_CHAT_WHISPEREE_NOT_FOUND));
				break;
			case CHAT_ERROR_MEMBER_ALREADY_A_BUDDY:
				sBuddyError(szHeaderString, GlobalStringGet(GS_BUDDY_EXISTING), myMsg->wszResultData);
				break;
			default:
				sBuddyError(szHeaderString, GlobalStringGet(GS_BUDDY_LIST_ERROR));
				break;
			}
		}
		break;
	case USER_REQUEST_REMOVE_MEMBER_AS_BUDDY:
		{
			LPCWSTR szHeaderString = StringTableGetStringByKey("InteractPlayerBuddyRemove");

			switch (myMsg->eResult)
			{
			case CHAT_ERROR_NONE:
				break;	//	success

			case CHAT_ERROR_MEMBER_NOT_A_BUDDY:
			default:
				sBuddyError(szHeaderString, GlobalStringGet(GS_BUDDY_REMOVE_ERROR));
				break;
			}
		}
		break;
	case USER_REQUEST_CHANGE_BUDDY_NICKNAME:
		{
			switch (myMsg->eResult)
			{
			case CHAT_ERROR_NONE:
				break;	//	success

			case CHAT_ERROR_MEMBER_NOT_A_BUDDY:
			default:
				sBuddyError(GlobalStringGet(GS_BUDDY_LIST_ERROR), GlobalStringGet(GS_BUDDY_LIST_ERROR));
				break;
			}
		}
		break;
	default:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_BuddyAdd(
	LPCWSTR TargetCharName)
{
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByCharName(TargetCharName);
	if (entry)
	{
		sBuddyError(StringTableGetStringByKey("InteractPlayerBuddyAdd"), GlobalStringGet(GS_BUDDY_EXISTING), TargetCharName);
		return FALSE;
	}

	CHAT_REQUEST_MSG_ADD_MEMBER_AS_BUDDY msgAdd;
	PStrCopy(msgAdd.wszMemberName, TargetCharName, MAX_CHARACTER_NAME);
	c_ChatNetSendMessage(&msgAdd, USER_REQUEST_ADD_MEMBER_AS_BUDDY);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_BuddyRemove(
	LPCWSTR TargetNickName)
{
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByNickName(TargetNickName);

	if (!entry)
	{
		sBuddyError(StringTableGetStringByKey("InteractPlayerBuddyRemove"), GlobalStringGet(GS_BUDDY_BUDDY_ERROR), TargetNickName);
		return FALSE;
	}

	CHAT_REQUEST_MSG_REMOVE_MEMBER_AS_BUDDY msgRemove;
	msgRemove.ullBuddyId = entry->BuddyLinkId;
	c_ChatNetSendMessage(&msgRemove, USER_REQUEST_REMOVE_MEMBER_AS_BUDDY);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_BuddyRemove(
	CLIENT_BUDDY_ID idTarget)
{
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(idTarget);

	if (!entry)
	{
		return FALSE;
	}

	CHAT_REQUEST_MSG_REMOVE_MEMBER_AS_BUDDY msgRemove;
	msgRemove.ullBuddyId = idTarget;
	c_ChatNetSendMessage(&msgRemove, USER_REQUEST_REMOVE_MEMBER_AS_BUDDY);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_BuddySetNick(
	LPCWSTR NickNameOld,
	LPCWSTR NickNameNew)
{
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByNickName(NickNameOld);

	if (!entry)
		return FALSE;

	CHAT_REQUEST_MSG_CHANGE_BUDDY_NICKNAME renameMsg;
	renameMsg.ullBuddyId = entry->BuddyLinkId;
	PStrCopy(renameMsg.wszNewBuddyNickname, NickNameNew, MAX_CHARACTER_NAME);

	c_ChatNetSendMessage(&renameMsg, USER_REQUEST_CHANGE_BUDDY_NICKNAME);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_BuddyShowList()
{
	UINT32 count = 0;

	for (UINT32 i = 0; i < MAX_CHAT_BUDDIES; i++) {
		if (BuddyList[i].BuddyLinkId != INVALID_BUDDY_ID) {
			if (BuddyList[i].BuddyCharGuid != INVALID_GUID) {
				UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
					GlobalStringGet(GS_BUDDY_ONLINE), BuddyList[i].BuddyNickName, BuddyList[i].BuddyCharName);
			} else {
				UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
					GlobalStringGet(GS_BUDDY_OFFLINE), BuddyList[i].BuddyNickName);
			}
			count++;
		}
	}
	UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
		GlobalStringGet(GS_BUDDY_COUNT), count);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_BuddyGetList(BUDDY_UI_ENTRY *pEntries, int nNumEntries)
{
	int nCount = 0;

	for (UINT32 i = 0; i < MAX_CHAT_BUDDIES && nCount < nNumEntries; i++) 
	{
		if (BuddyList[i].BuddyLinkId != INVALID_BUDDY_ID) 
		{
			PStrCopy(pEntries[nCount].szCharName, AppIsHellgate() ? BuddyList[i].BuddyNickName : BuddyList[i].BuddyCharName, MAX_CHARACTER_NAME);
			PStrCopy(pEntries[nCount].szAcctNick, BuddyList[i].BuddyNickName, MAX_CHARACTER_NAME);
			pEntries[nCount].bOnline = (BuddyList[i].BuddyCharGuid != INVALID_GUID);
			pEntries[nCount].nCharacterClass = BuddyList[i].BuddyCharacterClass;
			pEntries[nCount].nCharacterLevel = BuddyList[i].BuddyCharacterLevel;
			pEntries[nCount].nCharacterRank = BuddyList[i].BuddyCharacterRank;
			pEntries[nCount].idLink = BuddyList[i].BuddyLinkId;
			pEntries[nCount].nLevelDefinitionId = BuddyList[i].BuddyLevelDefinitionId;
			pEntries[nCount].nAreaId = BuddyList[i].BuddyAreaId;
			pEntries[nCount].nAreaDepth = BuddyList[i].BuddyAreaDepth;
			nCount++;
		}
	}

	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPCWSTR c_BuddyGetCharNameByID(
	CLIENT_BUDDY_ID linkId)
{
	ASSERT_RETNULL(linkId != INVALID_BUDDY_ID);
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(linkId);
	return (entry) ? entry->BuddyCharName : NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPCWSTR c_BuddyGetNickNameByID(
	CLIENT_BUDDY_ID linkID)
{
	ASSERT_RETNULL(linkID != INVALID_BUDDY_ID);
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(linkID);
	return (entry) ? entry->BuddyNickName : NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPCWSTR c_BuddyGetCharNameByGuid(PGUID guid)
{
	ASSERT_RETNULL(guid != INVALID_GUID);

	for (UINT32 i = 0; i < MAX_CHAT_BUDDIES; i++) {
		if (BuddyList[i].BuddyCharGuid == guid) {
			return BuddyList[i].BuddyCharName;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_BuddyGetGuidByID(
	CLIENT_BUDDY_ID linkID)
{
	ASSERT_RETX(linkID != INVALID_BUDDY_ID, INVALID_GUID);
	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(linkID);
	return (entry) ? entry->BuddyCharGuid : INVALID_GUID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_BuddyGetLevelId(
	PGUID guid)
{
	ASSERT_RETNULL(guid != INVALID_GUID);

	for (UINT32 i = 0; i < MAX_CHAT_BUDDIES; i++) {
		if (BuddyList[i].BuddyCharGuid == guid) {
			return BuddyList[i].BuddyLevelDefinitionId;
		}
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
// CLIENT BUDDY SYSTEM MESSAGE HANDLERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sChatCmdBuddyInfo(
	GAME * game,
	__notnull BYTE * data)
{
	CHAT_RESPONSE_MSG_BUDDY_INFO* msgInfo = (CHAT_RESPONSE_MSG_BUDDY_INFO*)data;
	UNREFERENCED_PARAMETER(game);

	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(msgInfo->BuddyId);

	if (entry)
	{
		if (entry->BuddyCharGuid == INVALID_GUID && msgInfo->CurrentCharId != INVALID_GUID)
		{
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_BUDDY_LOGIN), entry->BuddyNickName, msgInfo->wszCurrentCharName);
		}
		else if (entry->BuddyCharGuid != INVALID_GUID && msgInfo->CurrentCharId == INVALID_GUID)
		{
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_BUDDY_LOGOFF), entry->BuddyNickName, entry->BuddyCharName);
		}
	}
	else
	{
		entry = sFindAvailableBuddyEntry();
		ASSERTV_RETURN(entry, "No available buddy entries for new buddy info.");
	}

	entry->BuddyLinkId = msgInfo->BuddyId;
	entry->BuddyCharGuid = msgInfo->CurrentCharId;
	PStrCopy(entry->BuddyCharName, msgInfo->wszCurrentCharName, MAX_CHARACTER_NAME);
	PStrCopy(entry->BuddyNickName, msgInfo->wszBuddyNickname, MAX_CHARACTER_NAME);

	entry->BuddyCharacterClass = msgInfo->CurrentCharData.nPlayerUnitClass;
	entry->BuddyCharacterLevel = msgInfo->CurrentCharData.nCharacterExpLevel;
	entry->BuddyCharacterRank = msgInfo->CurrentCharData.nCharacterExpRank;
	entry->BuddyLevelDefinitionId = msgInfo->CurrentCharData.nLevelDefId;
	entry->BuddyAreaId = msgInfo->CurrentCharData.nArea;
	entry->BuddyAreaDepth = msgInfo->CurrentCharData.nDepth;

	UIBuddyListUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sChatCmdBuddyInfoUpdate(
	GAME * game,
	__notnull BYTE * data )
{
	CHAT_RESPONSE_MSG_BUDDY_INFO_UPDATE* msgInfo = (CHAT_RESPONSE_MSG_BUDDY_INFO_UPDATE*)data;
	UNREFERENCED_PARAMETER(game);

	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(msgInfo->BuddyId);
	if (!entry)
		return;

	entry->BuddyCharacterClass = msgInfo->CurrentCharData.nPlayerUnitClass;
	entry->BuddyCharacterLevel = msgInfo->CurrentCharData.nCharacterExpLevel;
	entry->BuddyCharacterRank = msgInfo->CurrentCharData.nCharacterExpRank;
	entry->BuddyLevelDefinitionId = msgInfo->CurrentCharData.nLevelDefId;
	entry->BuddyAreaId = msgInfo->CurrentCharData.nArea;
	entry->BuddyAreaDepth = msgInfo->CurrentCharData.nDepth;

	UIBuddyListUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sChatCmdBuddyRemoved(
	GAME * game,
	__notnull BYTE * data )
{
	CHAT_RESPONSE_MSG_BUDDY_REMOVED * removedMsg = (CHAT_RESPONSE_MSG_BUDDY_REMOVED*)data;
	UNREFERENCED_PARAMETER(game);

	BUDDY_CLIENT_CONTEXT * entry = sFindBuddyByLinkId(removedMsg->BuddyId);
	if (!entry)
		return;

	entry->BuddyLinkId = INVALID_BUDDY_ID;
	entry->BuddyCharGuid = INVALID_GUID;
	entry->BuddyCharName[0] = 0;
	entry->BuddyNickName[0] = 0;
	entry->BuddyCharacterClass = INVALID_ID;
	entry->BuddyCharacterLevel = INVALID_ID;
	entry->BuddyCharacterRank = INVALID_ID;
	entry->BuddyLevelDefinitionId = INVALID_ID;
	entry->BuddyAreaId = INVALID_ID;
	entry->BuddyAreaDepth = INVALID_ID;

	UIBuddyListUpdate();
}


#endif	//	!ISVERSION(SERVER_VERSION)
