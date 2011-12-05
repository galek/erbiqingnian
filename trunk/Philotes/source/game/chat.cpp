//----------------------------------------------------------------------------
// chat.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "clients.h"
#include "s_message.h"
#include "console.h"
#include "chat.h"
#include "level.h"
#include "uix.h"
#include "uix_chat.h"
#include "stringhashkey.h"
#include "stringtable.h"
#include "hash.h"
#include "ChatServer.h"
#include "c_message.h"
#include "player.h"
#include "language.h"
#include "uix_graphic.h"
#include "uix_components.h"

#if !ISVERSION(SERVER_VERSION)
	#include "svrstd.h"
	#include "c_channelmap.h"
	#include "c_chatNetwork.h"
	#include "UserChatCommunication.h"
	#include "wordfilter.h"
	#include "stringreplacement.h"
	#include "e_ui.h"
	#include "uix_hypertext.h"
	#include "uix_graphic.h"
#endif

#include "c_LevelAreasClient.h"
#include "GameChatCommunication.h"
#include "GameServer.h"
#include "stlallocator.h"
#include "achievements.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define IGNORE_HASH_SIZE			16
#define MAX_IGNORED_CHAT_CHANNELS	32

typedef CStringHashKey<WCHAR,MAX_CHAT_CNL_NAME> CHANNEL_KEY;

struct CHANNEL_ENTRY
{
	CHANNEL_KEY id;
	CHANNEL_ENTRY * next;
};

struct HYPERTEXT_MESSAGE_WRAPPER
{
	VALIDATED_HYPERTEXT_MESSAGE Message;
	DWORD DataReceivedSoFar;

	bool operator < (const HYPERTEXT_MESSAGE_WRAPPER & other) const
	{
		return (this->Message.messageId < other.Message.messageId);
	}
};

typedef set_mp<HYPERTEXT_MESSAGE_WRAPPER>::type HYPERTEXT_MESSAGE_LOOKUP;


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static HASH<
		CHANNEL_ENTRY,
		CHANNEL_KEY,
		IGNORE_HASH_SIZE> s_ignoredChatChannels;

static BOOL s_ignoredChatChannelsInitialized = FALSE;

static HYPERTEXT_MESSAGE_LOOKUP * s_hypertextMessageLookup = NULL;


//----------------------------------------------------------------------------
// UNIQUE CHANNEL NAMES
//----------------------------------------------------------------------------
const WCHAR * CHAT_CHANNEL_NAME_ANNOUNCEMENTS	= L"_ANNOUNCEMENTS";
const WCHAR * CHAT_CHANNEL_NAME_ADMINISTRATORS	= L"_ADMINS";


//----------------------------------------------------------------------------
// CHAT FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sChatGetTypeColorEnum(
	CHAT_TYPE eChatType )
{
	if (AppIsHellgate())
	{
		switch (eChatType)
		{
			case CHAT_TYPE_WHISPER:		return FONTCOLOR_CHAT_WHISPER;
			case CHAT_TYPE_PARTY:		return FONTCOLOR_CHAT_PARTY;
			case CHAT_TYPE_GAME:		return FONTCOLOR_CHAT_GENERAL;
			case CHAT_TYPE_SERVER:		return FONTCOLOR_CONSOLE_SYSTEM;
			case CHAT_TYPE_QUEST:		return FONTCOLOR_QUEST_LOG_TYPE_HEADER;
			case CHAT_TYPE_ANNOUNCEMENT:return FONTCOLOR_CHAT_ANNOUNCEMENT;
			case CHAT_TYPE_GUILD:		return FONTCOLOR_CHAT_GUILD;
			case CHAT_TYPE_CSR:			return FONTCOLOR_CHAT_CSR;
			case CHAT_TYPE_CLIENT_ERROR:return FONTCOLOR_CONSOLE_SYSTEM;
			case CHAT_TYPE_SHOUT:		return FONTCOLOR_CHAT_GLOBAL;
			case CHAT_TYPE_EMOTE:		return FONTCOLOR_CHAT_EMOTE;
			default:
				return FONTCOLOR_CHAT_GENERAL;
		};
	}
	else
	{
		switch (eChatType)
		{
			case CHAT_TYPE_WHISPER:		return FONTCOLOR_CHAT_WHISPER_NAME;
			case CHAT_TYPE_PARTY:		return FONTCOLOR_CHAT_PARTY_NAME;
			case CHAT_TYPE_GAME:		return FONTCOLOR_CHAT_GENERAL_NAME;
			case CHAT_TYPE_SERVER:		return FONTCOLOR_CONSOLE_SYSTEM;
			case CHAT_TYPE_QUEST:		return FONTCOLOR_QUEST_LOG_TYPE_HEADER;
			case CHAT_TYPE_ANNOUNCEMENT:return FONTCOLOR_CHAT_ANNOUNCEMENT_NAME;
			case CHAT_TYPE_GUILD:		return FONTCOLOR_CHAT_GUILD_NAME;
			case CHAT_TYPE_CSR:			return FONTCOLOR_CHAT_CSR;
			case CHAT_TYPE_SHOUT:		return FONTCOLOR_CHAT_GLOBAL;
			case CHAT_TYPE_EMOTE:		return FONTCOLOR_CHAT_EMOTE_NAME;
			default:
				return FONTCOLOR_CHAT_GENERAL_NAME;
		};
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetChatGetTypeNameColorEnum(
	CHAT_TYPE eChatType)
{
	switch (eChatType)
	{
		case CHAT_TYPE_WHISPER:		return FONTCOLOR_CHAT_WHISPER_NAME;
		case CHAT_TYPE_PARTY:		return FONTCOLOR_CHAT_PARTY_NAME;
		case CHAT_TYPE_GAME:		return FONTCOLOR_CHAT_GENERAL_NAME;
		case CHAT_TYPE_SERVER:		return FONTCOLOR_CONSOLE_SYSTEM;
		case CHAT_TYPE_QUEST:		return FONTCOLOR_QUEST_LOG_TYPE_HEADER;
		case CHAT_TYPE_ANNOUNCEMENT:return FONTCOLOR_CHAT_ANNOUNCEMENT_NAME;
		case CHAT_TYPE_GUILD:		return FONTCOLOR_CHAT_GUILD_NAME;
		case CHAT_TYPE_CSR:			return FONTCOLOR_CHAT_CSR;
		case CHAT_TYPE_SHOUT:		return FONTCOLOR_CHAT_GLOBAL;
		case CHAT_TYPE_EMOTE:		return FONTCOLOR_CHAT_EMOTE;
		default:
			return FONTCOLOR_CHAT_GENERAL_NAME;
	};
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ChatGetTypeColor(
	CHAT_TYPE eChatType )
{
	return GetFontColor(sChatGetTypeColorEnum(eChatType));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ChatGetTypeNameColor(
	CHAT_TYPE eChatType )
{
	return GetFontColor(sGetChatGetTypeNameColorEnum(eChatType));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetChatTypeLabel(
	CHAT_TYPE eChatType,
	WCHAR * dest,
	DWORD destLen,
	const WCHAR * suppliedCnlName )
{
#if !ISVERSION(SERVER_VERSION)
	const WCHAR *puszText = NULL;
	switch(eChatType)
	{
		case CHAT_TYPE_PARTY:
			puszText = GlobalStringGet( GS_CHAT_CONTEXT_PARTY );
			break;
		case CHAT_TYPE_GUILD:
			puszText = c_PlayerGetGuildName();
			break;
		case CHAT_TYPE_ANNOUNCEMENT:
			puszText = GlobalStringGet( GS_CHAT_CONTEXT_ANNOUNCEMENT );		
			break;
		case CHAT_TYPE_SHOUT:
			if (suppliedCnlName && suppliedCnlName[0])
			{
				puszText = suppliedCnlName;
				break;
			}
			// no break
		case CHAT_TYPE_GAME:
			if(gApp.pClientGame) 
			{
				LEVEL * level = LevelGetByID(gApp.pClientGame, gApp.m_idCurrentLevel);
				if( AppIsHellgate() )
				{
					puszText = LevelGetDisplayName(level);
				}
				else
				{
					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( LevelGetLevelAreaID( level ),
											 LevelGetDepth( level ),
											 dest,
											 destLen );
					return;

				}
				break;
			}
			// no break
		default:
			if(suppliedCnlName && suppliedCnlName[0])
			{
				puszText = suppliedCnlName;
			}
			else
			{
				static const WCHAR *puszUnknown = L"?";
				puszText = puszUnknown;
			}
			break;
	}
	
	// copy to dest
	ASSERTX_RETURN( puszText, "Expected chat type text" );
	PStrCopy(dest, puszText, destLen);
			
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL sIsValidHypertextDisplayString(
	const WCHAR * str,
	DWORD dwStrLen)
{
	if (!str)
		return FALSE;
	for (DWORD ii = 0; ii < dwStrLen && str[ii] != L'\0'; ++ii)
	{
		if (str[ii] == CTRLCHAR_COLOR || str[ii] == CTRLCHAR_LINECOLOR || str[ii] == CTRLCHAR_HYPERTEXT)
			ii++;
		else if (str[ii] == CTRLCHAR_COLOR_RETURN || str[ii] == CTRLCHAR_HYPERTEXT_END)
			continue;
		else if (!PStrIsPrintable(str[ii]))
			return FALSE;
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DisplayChat(
	CHAT_TYPE eChatType,
	const WCHAR * sender,
	const WCHAR * guild,
	const WCHAR * message,
	const WCHAR * channelName /* = NULL */,
	PGUID guidSender /* = INVALID_GUID*/,
	UI_LINE * pLine /* = NULL */)
{
#if !ISVERSION(SERVER_VERSION)	
	ASSERT_RETURN(sender);
	ASSERT_RETURN(message);

	WCHAR msg[CONSOLE_MAX_STR];
	PStrCopy(msg, message, CONSOLE_MAX_STR);

	if ( (pLine && !sIsValidHypertextDisplayString(msg, CONSOLE_MAX_STR)) ||
		(!pLine && !PStrIsPrintable(msg, CONSOLE_MAX_STR)))	//	if not validated, all data must be assumed to be dangerous
	{
		return;
	}

	if(c_ChatChannelIsIgnored(eChatType, channelName))
	{
		return;
	}

	// send message through the word filters
	ChatFilterStringInPlace(msg);
	
	WCHAR chatTypeLabel[MAX_CHEAT_LEN];
	sGetChatTypeLabel(eChatType,chatTypeLabel,MAX_CHEAT_LEN,channelName);

	WCHAR uszLine[CONSOLE_MAX_STR];
	if (UIChatAbbreviateNames())
	{
		const LANGUAGE_DATA *pLanguageData = LanguageGetData( AppGameGet(), LanguageGetCurrent() );
		ASSERT_RETURN(pLanguageData);
				
		WCHAR uszSender[256];
		ASSERT_RETURN(pLanguageData->nAbbrevChatNameLen < arrsize(uszSender));
		PStrCopy( uszSender, sender, pLanguageData->nAbbrevChatNameLen );
		PStrCopy( uszLine, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_ABBREVIATED), CONSOLE_MAX_STR );
		PStrReplaceToken( uszLine, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_PLAYER), uszSender );
	}
	else
	{
		PStrCopy( uszLine, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_GENERAL), CONSOLE_MAX_STR );
		PStrReplaceToken( uszLine, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_PLAYER), sender );
	}
	PStrReplaceToken( uszLine, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), chatTypeLabel );
	// KCK, Since guild is often empty, I didn't want to bracket it in the format string. So check here if it has a value
	// and bracket it if so.
	WCHAR	szGuild[CONSOLE_MAX_STR] = { 0 };
	if (guild && guild[0])
	{
		// KCK: Temporary fix; Make sure guild names are no longer than 4 chars. Replace with acronyms in the future.
//		PStrPrintf(szGuild, CONSOLE_MAX_STR, L"<%s> ", guild);
		PStrPrintf(szGuild, CONSOLE_MAX_STR, L"<%1.4s> ", guild);
	}
	PStrReplaceToken( uszLine, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_GUILD), szGuild );
	PStrReplaceToken( uszLine, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_MESSAGE), msg );

	UIAddChatLine(
		eChatType,
		ChatGetTypeColor(eChatType),
		uszLine,
		pLine );	

	if (guidSender != INVALID_GUID &&
		UIChatBubblesEnabled())
	{
		GAME *pGame = AppGetCltGame();
		if (pGame)
		{
			UNIT *pSenderUnit = UnitGetByGuid(pGame, guidSender);
			if (pSenderUnit)
			{
				UIAddChatBubble( msg, ChatGetTypeColor(eChatType), pSenderUnit );
			}
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_IgnoreChatChannel(
	const WCHAR * channelName )
{
	if(!s_ignoredChatChannelsInitialized)
	{
		s_ignoredChatChannels.Init();
		s_ignoredChatChannelsInitialized = TRUE;
	}

	if(!channelName || !channelName[0])
		return;

	CStringHashKey<WCHAR,MAX_CHAT_CNL_NAME> key = channelName;
	if(!s_ignoredChatChannels.Get(key))
		s_ignoredChatChannels.Add(NULL, key);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnignoreChatChannel(
	const WCHAR * channelName )
{
	if(!s_ignoredChatChannelsInitialized)
		return;

	if(!channelName || !channelName[0])
		return;

	CStringHashKey<WCHAR,MAX_CHAT_CNL_NAME> key = channelName;
	s_ignoredChatChannels.Remove(key);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ChatChannelIsIgnored(
	CHAT_TYPE eChatType,
	const WCHAR * channelName)
{
	if(!s_ignoredChatChannelsInitialized)
		return FALSE;

	//	can only ignore global, game, and guild messages for now
	if( eChatType != CHAT_TYPE_GAME &&
		eChatType != CHAT_TYPE_GUILD )
		return FALSE;

	if(!channelName)
		return FALSE;

	CStringHashKey<WCHAR,MAX_CHAT_CNL_NAME> key = channelName;
	return (s_ignoredChatChannels.Get(key) != NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ClearIgnoredChatChannels(
	void )
{
	if(s_ignoredChatChannelsInitialized)
	{
		s_ignoredChatChannels.Destroy( NULL );
		s_ignoredChatChannelsInitialized = FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_SendSysText(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game, 
	GAMECLIENT * client,
	MSG_SCMD_SYS_TEXT * msg)
{
	msg->bConsoleOnly = (BYTE)bConsoleOnly;
	switch (eChatType)
	{
	case CHAT_TYPE_WHISPER:
		{
			NETCLIENTID64 idNetClient = ClientGetNetId(client);
			ASSERT_RETFALSE(idNetClient != INVALID_NETCLIENTID64);
			s_SendMessageNetClient(idNetClient, SCMD_SYS_TEXT, msg);
		}
		break;
	case CHAT_TYPE_PARTY:
		ASSERT(0);
		break;
	case CHAT_TYPE_GAME:
	case CHAT_TYPE_SHOUT:
		{
			if (client)
			{
				s_SendMessageToOthers(game, ClientGetId(client), SCMD_SYS_TEXT, msg);
			}
			else
			{
				s_SendMessageToAll(game, SCMD_SYS_TEXT, msg);
			}
		}
		break;
	case CHAT_TYPE_SERVER:
		ASSERT(0);
		break;
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysText(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game, 
	GAMECLIENT * client,
	const WCHAR * text,
	DWORD color)
{
	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;
	PStrCopy(msg.str, text, MAX_CHEAT_LEN);
	
	return s_SendSysText(eChatType, bConsoleOnly, game, client, &msg);
}


//----------------------------------------------------------------------------
// watch the locking on this 'cuz calling ClientGetNetId invokes an app lock
//----------------------------------------------------------------------------
static BOOL s_SendSysTextApp(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	APPCLIENTID idAppClient,
	MSG_SCMD_SYS_TEXT * msg)
{
	msg->bConsoleOnly = (BYTE)bConsoleOnly;
	switch (eChatType)
	{
	case CHAT_TYPE_WHISPER:
		{
			NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
			ASSERT_RETFALSE(idNetClient != INVALID_NETCLIENTID64);
			s_SendMessageNetClient(idNetClient, SCMD_SYS_TEXT, msg);
		}
		break;
	case CHAT_TYPE_PARTY:
		ASSERT(0);
		break;
	case CHAT_TYPE_GAME:
	case CHAT_TYPE_SHOUT:
		ASSERT(0);
		break;
	case CHAT_TYPE_SERVER:
		{
			NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
			if (idAppClient != INVALID_CLIENTID)
			{
				idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
			}
			ClientSystemSendMessageToAll(AppGetClientSystem(), idNetClient, SCMD_SYS_TEXT, msg);
		}
		break;
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
// watch the locking on this 'cuz calling ClientGetNetId invokes an app lock
//----------------------------------------------------------------------------
BOOL s_SendSysTextApp(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	APPCLIENTID idAppClient,
	const WCHAR * text,
	DWORD color)
{
	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;
	PStrCopy(msg.str, text, MAX_CHEAT_LEN);

	return s_SendSysTextApp(eChatType, bConsoleOnly, idAppClient, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_SendSysTextNet(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	NETCLIENTID64 idNetClient,
	MSG_SCMD_SYS_TEXT * msg)
{
	switch (eChatType)
	{
	case CHAT_TYPE_WHISPER:
		{
			msg->bConsoleOnly = (BYTE)bConsoleOnly;
			s_SendMessageNetClient(idNetClient, SCMD_SYS_TEXT, msg);
		}
		break;
	case CHAT_TYPE_PARTY:
		ASSERT(0);
		break;
	case CHAT_TYPE_GAME:
	case CHAT_TYPE_SHOUT:
		ASSERT(0);
		break;
	case CHAT_TYPE_SERVER:
		ASSERT(0);
		break;
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysTextNet(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	NETCLIENTID64 idNetClient,
	const WCHAR * text,
	DWORD color)
{
	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;
	PStrCopy(msg.str, text, MAX_CHEAT_LEN);
	
	return s_SendSysTextNet(eChatType, bConsoleOnly, idNetClient, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysTextFmt1(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	const WCHAR * str)
{
	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;
	
	PStrCopy(msg.str, str, MAX_CHEAT_LEN);

	return s_SendSysText(eChatType, bConsoleOnly, game, client, &msg);
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysTextFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;
	
	PStrVprintf(msg.str, MAX_CHEAT_LEN, format, args);

	return s_SendSysText(eChatType, bConsoleOnly, game, client, &msg);
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysTextAppFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	APPCLIENTID idAppClient,
	DWORD color,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;

	PStrVprintf(msg.str, MAX_CHEAT_LEN, format, args);

	return s_SendSysTextApp(eChatType, bConsoleOnly, idAppClient, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendSysTextNetFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	NETCLIENTID64 idNetClient,
	DWORD color,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

	MSG_SCMD_SYS_TEXT msg;
	msg.dwColor = color;

	PStrVprintf(msg.str, MAX_CHEAT_LEN, format, args);

	return s_SendSysTextNet(eChatType, bConsoleOnly, idNetClient, &msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_Emote(
	const WCHAR *uszMessage,
	int nUnitMode /*=INVALID_ID*/)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( uszMessage, "Expected message" );

	MSG_CCMD_EMOTE tMessage;
	PStrCopy( tMessage.uszMessage, uszMessage, MAX_EMOTE_LEN );
	tMessage.mode = nUnitMode;
	c_SendMessage( CCMD_EMOTE, &tMessage );

	WCHAR message[MAX_CHEAT_LEN];
	PStrPrintf(message, MAX_CHEAT_LEN, L"%s %s", gApp.m_wszPlayerName, uszMessage);

	c_ShowEmoteText(message, MAX_CHEAT_LEN);
#endif
}

void c_Emote(
	int nEmoteId)
{
#if !ISVERSION(SERVER_VERSION)
	const EMOTE_DATA *pEmote = (EMOTE_DATA *) ExcelGetData(NULL, DATATABLE_EMOTES, nEmoteId);
	ASSERTX_RETURN(pEmote, "Invalid emote ID");

	const WCHAR *szText = StringTableGetStringByIndex(pEmote->nTextString);

	c_Emote(szText, pEmote->nUnitMode);
#endif
}

BOOL x_EmoteCanPerform(
	UNIT *pPlayer,
	const EMOTE_DATA *pEmote)
{
	ASSERT_RETFALSE(pPlayer && UnitIsPlayer(pPlayer));
	ASSERT_RETFALSE(pEmote);

	if (pEmote->nRequiresAchievement != INVALID_LINK)
	{
		const ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(UnitGetGame(pPlayer), DATATABLE_ACHIEVEMENTS, pEmote->nRequiresAchievement);
		ASSERT_RETFALSE(pAchievement);

		return x_AchievementIsComplete(pPlayer, pAchievement);
	}

	return TRUE;
}

BOOL x_EmoteCanPerform(
	UNIT *pPlayer,
	int nEmoteId)
{
	ASSERT_RETFALSE(pPlayer && UnitIsPlayer(pPlayer));

	const EMOTE_DATA *pEmote = (EMOTE_DATA *) ExcelGetData(UnitGetGame(pPlayer), DATATABLE_EMOTES, nEmoteId);
	ASSERTX_RETFALSE(pEmote, "Invalid emote ID");

	return x_EmoteCanPerform(pPlayer, pEmote);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ShowEmoteText(
	const WCHAR * wszText,
	DWORD textLen )
{
	WCHAR message[MAX_CHEAT_LEN];
	PStrCopy(message, MAX_CHEAT_LEN, wszText, textLen);

	ChatFilterStringInPlace(message);

	// do this in local AND global chat channels 
	UIAddChatLine(CHAT_TYPE_GAME, ChatGetTypeColor(CHAT_TYPE_EMOTE), message);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_Emote(
	GAME *pGame,
	UNIT *pUnit,
	const WCHAR *uszMessage)
{
#if ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pUnit ), "Expected player" );
	ASSERTX_RETURN( uszMessage, "Expected message" );
	ASSERTX_RETURN( uszMessage[ 0 ], "Invalid message" );

	LEVEL *pLevel = UnitGetLevel(pUnit);
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pLevel->m_idChatChannel != INVALID_CHANNELID, "Expected local chat channel for level" );
	
	WCHAR uszName[ MAX_CHARACTER_NAME ];
	UnitGetName( pUnit, uszName, MAX_CHARACTER_NAME, 0 );

	WCHAR message[ MAX_CHAT_MESSAGE/sizeof(WCHAR) ];
	PStrPrintf(
		message,
		MAX_CHAT_MESSAGE/sizeof(WCHAR),
		L"%s %s", 
		uszName, 
		uszMessage);

	CHAT_REQUEST_MSG_SEND_SYSTEM_MESSAGE_TO_CHANNEL cnlMsg;
	cnlMsg.ullSendingCharacterGuid = UnitGetGuid( pUnit );
	cnlMsg.idChannel = pLevel->m_idChatChannel;
	cnlMsg.eMessageType = CHAT_TYPE_EMOTE;
	cnlMsg.Message.SetWStrMsg(message);

	GameServerToChatServerSendMessage(
		&cnlMsg,
		GAME_REQUEST_SEND_SYSTEM_MESSAGE_TO_CHANNEL);
#else
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pUnit);
	UNREFERENCED_PARAMETER(uszMessage);
#endif
}

#if !ISVERSION(SERVER_VERSION)
//	vvvvvvvvvvvvvvvvvv client only methods vvvvvvvvvvvvvvvvvvvv

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendItemChatDo(
	ITEM_CHAT_TYPE type,
	const WCHAR * wszTargetName,
	CHANNELID targetChannel,
	const WCHAR * wszMessage,
	const UI_LINE * pLine)
{
	MSG_CCMD_ITEM_MESSAGE toSend;

	toSend.eItemChatType = (BYTE)type;
	toSend.wszTargetName[0] = 0;
	toSend.targetChannelId = targetChannel;
	PStrCopy(toSend.wszMessageText, wszMessage, (MAX_CHAT_MESSAGE/sizeof(WCHAR)));
	if (wszTargetName)
		PStrCopy(toSend.wszTargetName, wszTargetName, MAX_CHARACTER_NAME);

	MSG_SET_BLOB_LEN(
		toSend,
		HypertextData,
		c_UIEncodeHypertextForGameServer(
			wszMessage,
			pLine,
			toSend.HypertextData,
			MAX_CLIENT_TO_GAME_HYPERTEXT_DATA));

	c_SendMessage(CCMD_ITEM_MESSAGE, &toSend);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendItemChat(
	ITEM_CHAT_TYPE type,
	const WCHAR * wszMessage,
	const UI_LINE * pLine)
{
	sSendItemChatDo(type, L"", INVALID_CHANNELID, wszMessage, pLine);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendItemChat(
	ITEM_CHAT_TYPE type,
	CHANNELID targetChannel,
	const WCHAR * wszMessage,
	const UI_LINE * pLine)
{
	sSendItemChatDo(type, L"", targetChannel, wszMessage, pLine);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendItemChat(
	ITEM_CHAT_TYPE type,
	const WCHAR * wszTargetName,
	const WCHAR * wszMessage,
	const UI_LINE * pLine)
{
	sSendItemChatDo(type, wszTargetName, INVALID_CHANNELID, wszMessage, pLine);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendInstancingChannelChat(
	const WCHAR *szInstancingChannel,
	const WCHAR *szChatMessage,
	UI_LINE *pLine)
{
	WCHAR szInstancedChannel[MAX_CHAT_CNL_NAME];

	szChatMessage = PStrSkipWhitespace(szChatMessage);
	if (!szChatMessage[0])
		return;

	CHANNELID idChannel = GetInstancedChannel(szInstancingChannel, szInstancedChannel);

	if (idChannel != INVALID_CHANNELID)
	{
		BOOL bHasHypertext = UICurrentConsoleLineIsHypertext(pLine);
		if (bHasHypertext)
		{
			sSendItemChat(ITEM_CHAT_INSTANCING_CHANNEL_CHAT, idChannel, szChatMessage, pLine);
		}
		else
		{
			CHAT_REQUEST_MSG_MESSAGE_CHANNEL chatMsg;
			chatMsg.TagChannel = idChannel;
			chatMsg.Message.SetWStrMsg(szChatMessage);
			c_ChatNetSendMessage(&chatMsg, USER_REQUEST_MESSAGE_CHANNEL);
		}

		c_DisplayChat(
			CHAT_TYPE_SHOUT,
			gApp.m_wszPlayerName,
			c_PlayerGetGuildName(),
			szChatMessage,
			szInstancedChannel,
			INVALID_GUID,
			(bHasHypertext)? pLine: NULL);

		ConsoleSetChatContext(CHAT_TYPE_SHOUT);

		ConsoleSetLastInstancingChannelName(szInstancingChannel);

		if (!UIChatTabIsVisible(CHAT_TAB_GLOBAL) && !UIChatTabIsVisible(CHAT_TAB_LOCAL))
		{
			UIChatTabActivate(CHAT_TAB_LOCAL);
		}
	}
	else
		ASSERT_MSG("Chatting on a channel you are not in");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendPartyChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine)
{
	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
		return;

	BOOL bHasHypertext = UICurrentConsoleLineIsHypertext(pLine);
	if (bHasHypertext)
	{
		sSendItemChat(ITEM_CHAT_PARTY_CHAT, szChatMessage, pLine);
	}
	else
	{
		CHAT_REQUEST_MSG_MESSAGE_PARTY_MEMBERS msg;
		msg.Message.SetWStrMsg(szChatMessage);
		c_ChatNetSendMessage(&msg, USER_REQUEST_MESSAGE_PARTY_MEMBERS);
	}

	c_DisplayChat(
		CHAT_TYPE_PARTY,
		gApp.m_wszPlayerName,
		L"",
		szChatMessage,
		NULL,
		INVALID_GUID,
		(bHasHypertext)? pLine: NULL);

	ConsoleSetChatContext(CHAT_TYPE_PARTY);

	if (!UIChatTabIsVisible(CHAT_TAB_GLOBAL) && !UIChatTabIsVisible(CHAT_TAB_PARTY))
	{
		UIChatTabActivate(CHAT_TAB_PARTY);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendTownChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine)
{
	if (gApp.m_idLevelChatChannel == INVALID_CHANNELID)
		return;

	BOOL bHasHypertext = UICurrentConsoleLineIsHypertext(pLine);
	if (bHasHypertext)
	{
		sSendItemChat(ITEM_CHAT_TOWN_CHAT, gApp.m_idLevelChatChannel, szChatMessage, pLine);
	}
	else
	{
		CHAT_REQUEST_MSG_MESSAGE_CHANNEL chatMsg;
		chatMsg.TagChannel = gApp.m_idLevelChatChannel;
		chatMsg.Message.SetWStrMsg(szChatMessage);
		c_ChatNetSendMessage(&chatMsg, USER_REQUEST_MESSAGE_CHANNEL);
	}

	c_DisplayChat(
		CHAT_TYPE_GAME,
		gApp.m_wszPlayerName,
		c_PlayerGetGuildName(),
		szChatMessage,
		NULL,
		INVALID_GUID,
		(bHasHypertext)? pLine: NULL);

	ConsoleSetChatContext(CHAT_TYPE_GAME);

	if (!UIChatTabIsVisible(CHAT_TAB_GLOBAL) && !UIChatTabIsVisible(CHAT_TAB_LOCAL))
	{
		UIChatTabActivate(CHAT_TAB_LOCAL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendWhisperChat(
	const WCHAR * szTargetPlayerName,
	const WCHAR * szChatMessage,
	UI_LINE * pLine)
{
	if (UICurrentConsoleLineIsHypertext(pLine))
	{
		sSendItemChat(ITEM_CHAT_WHISPER_CHAT, szTargetPlayerName, szChatMessage, pLine);

		//	TODO: maybe send success/failure like with normal whisper chat...
		c_DisplayChat(
			CHAT_TYPE_WHISPER,
			gApp.m_wszPlayerName,
			L"",
			szChatMessage,
			szTargetPlayerName,
			INVALID_GUID,
			pLine);
	}
	else
	{
		CHAT_REQUEST_MSG_DIRECT_MESSAGE_MEMBER chatMessage;
		PStrCopy(chatMessage.wszMemberName, szTargetPlayerName, MAX_CHARACTER_NAME);
		chatMessage.Message.SetWStrMsg(szChatMessage);
		c_ChatNetSendMessage(&chatMessage, USER_REQUEST_DIRECT_MESSAGE_MEMBER);
	}

	ConsoleSetChatContext(CHAT_TYPE_WHISPER);

	ConsoleSetLastTalkedToPlayerName(szTargetPlayerName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendAdminChat(
	const WCHAR * szChatMessage)
{
	CHAT_REQUEST_MSG_MESSAGE_CHANNEL chatMsg;
	chatMsg.TagChannel = CHAT_CHANNEL_NAME_ADMINISTRATORS;
	chatMsg.Message.SetWStrMsg(szChatMessage);
	c_ChatNetSendMessage(&chatMsg, USER_REQUEST_MESSAGE_CHANNEL);

	c_DisplayChat(
		CHAT_TYPE_GUILD,
		gApp.m_wszPlayerName,
		c_PlayerGetGuildName(),
		szChatMessage,
		CHAT_CHANNEL_NAME_ADMINISTRATORS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendGuildChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine)
{
	BOOL bHasHypertext = UICurrentConsoleLineIsHypertext(pLine);
	if (bHasHypertext)
	{
		sSendItemChat(ITEM_CHAT_GUILD_CHAT, szChatMessage, pLine);
	}
	else
	{
		CHAT_REQUEST_MSG_MESSAGE_GUILD_MEMBERS msg;
		msg.Message.SetWStrMsg(szChatMessage);
		c_ChatNetSendMessage(&msg, USER_REQUEST_MESSAGE_GUILD_MEMBERS);
	}

	c_DisplayChat(
		CHAT_TYPE_GUILD,
		gApp.m_wszPlayerName,
		L"",
		szChatMessage,
		NULL,
		INVALID_GUID,
		(bHasHypertext)? pLine: NULL);

	ConsoleSetChatContext(CHAT_TYPE_GUILD);

	if (!UIChatTabIsVisible(CHAT_TAB_GLOBAL) && !UIChatTabIsVisible(CHAT_TAB_GUILD))
	{
		UIChatTabActivate(CHAT_TAB_GUILD);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static HYPERTEXT_MESSAGE_LOOKUP * sGetHypertextMessageLooup()
{
	if (s_hypertextMessageLookup == NULL)
	{
		s_hypertextMessageLookup = (HYPERTEXT_MESSAGE_LOOKUP*)MALLOC(NULL, sizeof(HYPERTEXT_MESSAGE_LOOKUP));
		ASSERT_RETNULL(s_hypertextMessageLookup);
		new (s_hypertextMessageLookup) HYPERTEXT_MESSAGE_LOOKUP(HYPERTEXT_MESSAGE_LOOKUP::key_compare(), NULL);
	}
	return s_hypertextMessageLookup;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static HYPERTEXT_MESSAGE_WRAPPER * sGetHypertextMessage(
	ULONGLONG messageId )
{
	HYPERTEXT_MESSAGE_LOOKUP * lookup = sGetHypertextMessageLooup();
	ASSERT_RETNULL(lookup);

	HYPERTEXT_MESSAGE_WRAPPER key;
	key.Message.messageId = messageId;

	HYPERTEXT_MESSAGE_LOOKUP::iterator itr = lookup->find(key);
	if (itr == lookup->end())
		return NULL;

	return &(*itr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_HypertextDataStartReceived(
	ULONGLONG		messageId,
	PGUID			senderCharacterId,
	WCHAR *			wszSenderCharacterName,
	CHANNELID		channelId,
	WCHAR *			wszChannelName,
	ITEM_CHAT_TYPE	eItemChatType,
	DWORD			hypertextDataLength )
{
	ASSERT_RETURN(wszSenderCharacterName);
	ASSERT_RETURN(wszChannelName);

	HYPERTEXT_MESSAGE_LOOKUP * lookup = sGetHypertextMessageLooup();
	ASSERT_RETURN(lookup);

	HYPERTEXT_MESSAGE_WRAPPER newEntry;
	newEntry.DataReceivedSoFar = 0;
	newEntry.Message.messageId = messageId;
	newEntry.Message.senderCharacterId = senderCharacterId;
	PStrCopy(newEntry.Message.wszSenderCharacterName, wszSenderCharacterName, MAX_CHARACTER_NAME);
	newEntry.Message.channelId = channelId;
	PStrCopy(newEntry.Message.wszChannelName, wszChannelName, MAX_CHAT_CNL_NAME);
	newEntry.Message.eItemChatType = eItemChatType;
	newEntry.Message.wszMessageText[0] = 0;
	newEntry.Message.hypertextDataLength = hypertextDataLength;
	newEntry.Message.hypertextData = NULL;
	if (hypertextDataLength > 0)
	{
		newEntry.Message.hypertextData = (BYTE*)MALLOCZ(NULL, hypertextDataLength);
		ASSERT_RETURN(newEntry.Message.hypertextData);
	}

	ASSERT_DO(lookup->insert(newEntry).second)
	{
		FREE(NULL, newEntry.Message.hypertextData);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_HypertextDataTextReceived(
	ULONGLONG		messageId,
	WCHAR *			wszMessageText )
{
	HYPERTEXT_MESSAGE_WRAPPER * message = sGetHypertextMessage(messageId);
	ASSERT_RETURN(message);

	PStrCopy(message->Message.wszMessageText, wszMessageText, (MAX_CHAT_MESSAGE/sizeof(WCHAR)));

	if (message->Message.hypertextDataLength == 0)
	{
		c_HypertextDataPacketReceived(messageId, NULL, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleCompleteHypertextMessage(
	__notnull VALIDATED_HYPERTEXT_MESSAGE * message )
{
	UI_LINE tLine;

	c_UIDecodeHypertextMessageFromChatServer(
		message->wszMessageText,
		&tLine,
		message->hypertextData,
		message->hypertextDataLength );

	switch (message->eItemChatType)
	{
	case ITEM_CHAT_INSTANCING_CHANNEL_CHAT:
		{
			c_DisplayChat(
				CHAT_TYPE_SHOUT,
				message->wszSenderCharacterName,
				L"",
				message->wszMessageText,
				message->wszChannelName,
				message->senderCharacterId,
				&tLine );
		}
		break;
	case ITEM_CHAT_PARTY_CHAT:
		{
			c_DisplayChat(
				CHAT_TYPE_PARTY,
				message->wszSenderCharacterName,
				L"",
				message->wszMessageText,
				NULL,
				message->senderCharacterId,
				&tLine );
		}
		break;
	case ITEM_CHAT_TOWN_CHAT:
		{
			c_DisplayChat(
				CHAT_TYPE_GAME,
				message->wszSenderCharacterName,
				L"",
				message->wszMessageText,
				message->wszChannelName,
				message->senderCharacterId,
				&tLine );
		}
		break;
	case ITEM_CHAT_WHISPER_CHAT:
		{
			PStrCopy(gApp.m_wszLastWhisperSender, MAX_CHARACTER_NAME, message->wszSenderCharacterName, MAX_CHARACTER_NAME);

			c_DisplayChat(
				CHAT_TYPE_WHISPER,
				message->wszSenderCharacterName,
				L"",
				message->wszMessageText,
				gApp.m_wszPlayerName,
				INVALID_GUID,
				&tLine );
		}
		break;
	case ITEM_CHAT_GUILD_CHAT:
		{
			c_DisplayChat(
				CHAT_TYPE_GUILD,
				message->wszSenderCharacterName,
				L"",
				message->wszMessageText,
				NULL,
				message->senderCharacterId,
				&tLine );
		}
		break;
	default: return;
	};
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_HypertextDataPacketReceived(
	ULONGLONG		messageId,
	BYTE *			hypertextData,
	DWORD			hyertextDataLen )
{
	HYPERTEXT_MESSAGE_WRAPPER * message = sGetHypertextMessage(messageId);
	ASSERT_RETURN(message);

	if (hypertextData && hyertextDataLen > 0)
	{
		ASSERT_RETURN(
			MemoryCopy(
				message->Message.hypertextData + message->DataReceivedSoFar,
				message->Message.hypertextDataLength - message->DataReceivedSoFar,
				hypertextData,
				hyertextDataLen ));

		message->DataReceivedSoFar += hyertextDataLen;
	}

	if (message->DataReceivedSoFar == message->Message.hypertextDataLength)
	{
		sHandleCompleteHypertextMessage(&message->Message);
		
		FREE(NULL, message->Message.hypertextData);

		HYPERTEXT_MESSAGE_LOOKUP * lookup = sGetHypertextMessageLooup();
		ASSERT_RETURN(lookup);

		HYPERTEXT_MESSAGE_WRAPPER key;
		key.Message.messageId = messageId;

		HYPERTEXT_MESSAGE_LOOKUP::iterator itr = lookup->find(key);
		ASSERT_RETURN(itr != lookup->end());

		lookup->erase(itr);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ClearHypertextChatMessages(
	void )
{
	if (s_hypertextMessageLookup != NULL)
	{
		for (HYPERTEXT_MESSAGE_LOOKUP::const_iterator itr = s_hypertextMessageLookup->begin();
			itr != s_hypertextMessageLookup->end(); ++itr)
		{
			FREE(NULL, itr->Message.hypertextData);
		}

		s_hypertextMessageLookup->~HYPERTEXT_MESSAGE_LOOKUP();
		FREE(NULL, s_hypertextMessageLookup);
		s_hypertextMessageLookup = NULL;
	}
}

//	^^^^^^^^^^^^^^^^^^^^^^	CLIENT ONLY METHODS		^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif	//	!ISVERSION(SERVER_VERSION)
