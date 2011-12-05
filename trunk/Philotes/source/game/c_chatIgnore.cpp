#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include "Chat.h"
#include "ChatServer.h"
#include "EmbeddedServerRunner.h"
#include "UserChatCommunication.h"
#include "uix.h"
#include "uix_chat.h"
#include "globalindex.h"

static WCHAR				IgnoredPlayers[MAX_IGNORED_CHAT_MEMBERS][MAX_CHARACTER_NAME];
static CCriticalSection		csIgnoreList(TRUE);

void c_IgnoreListInit()
{
	CSAutoLock autoLock(&csIgnoreList);

	for (UINT32 i = 0; i < MAX_IGNORED_CHAT_MEMBERS; i++) {
		IgnoredPlayers[i][0] = L'\0';
	}
}

void c_ShowIgnoreList()
{
	CSAutoLock autoLock(&csIgnoreList);

	UINT32 count = 0;
	for (UINT32 i = 0; i < MAX_IGNORED_CHAT_MEMBERS; i++) {
		if (IgnoredPlayers[i][0]) {
			count++;
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_LIST), IgnoredPlayers[i]);
		}
	}

	UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
		GlobalStringGet(GS_IGNORE_LIST_TOTAL), count);
}

void sChatCmdPlayerIgnored(
	GAME* game,
	__notnull BYTE* data)
{
	CHAT_RESPONSE_MSG_PLAYER_IGNORED* pMsg = (CHAT_RESPONSE_MSG_PLAYER_IGNORED*)data;
	ASSERT_RETURN(pMsg != NULL);

	CSAutoLock autoLock(&csIgnoreList);
	for (UINT32 i = 0; i < MAX_IGNORED_CHAT_MEMBERS; i++) {
		if (IgnoredPlayers[i][0] == L'\0') {
			PStrCopy(IgnoredPlayers[i], pMsg->wszPlayerName, MAX_CHARACTER_NAME);
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ADD), pMsg->wszPlayerName);
			return;
		}
	}
}

void sChatCmdPlayerUnIgnored(
	GAME* game,
	__notnull BYTE* data)
{
	CHAT_RESPONSE_MSG_PLAYER_UNIGNORED* pMsg = (CHAT_RESPONSE_MSG_PLAYER_UNIGNORED*)data;
	ASSERT_RETURN(pMsg != NULL);

	CSAutoLock autoLock(&csIgnoreList);
	for (UINT32 i = 0; i < MAX_IGNORED_CHAT_MEMBERS; i++) {
		if (PStrICmp(IgnoredPlayers[i], pMsg->wszPlayerName) == 0) {
			IgnoredPlayers[i][0] = L'\0';
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_REMOVE), pMsg->wszPlayerName);
			return;
		}
	}
}

void sChatCmdIgnoreError(
		GAME* game,
		__notnull BYTE* data)
{
	CHAT_RESPONSE_MSG_IGNORE_ERROR* pMsg = (CHAT_RESPONSE_MSG_IGNORE_ERROR*)data;
	ASSERT_RETURN(pMsg != NULL);

	switch (pMsg->dwError) {
		case CHAT_ERROR_IGNORE_LIST_NOT_INITIALIZED:
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ERROR_NOT_INIT));
			break;
		case CHAT_ERROR_MAX_IGNORED_MEMBERS:
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ERROR_MAX));
			break;
		case CHAT_ERROR_MEMBER_ALREADY_IGNORED:
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ERROR_ALREADY_IGNORED));
			break;
		case CHAT_ERROR_MEMBER_NOT_IGNORED:
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ERROR_NOT_IGNORED));
			break;
		default:
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_IGNORE_ERROR_UNKNOWN));
			break;
	}
}

#endif