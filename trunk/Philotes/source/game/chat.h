//----------------------------------------------------------------------------
// chat.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_CHAT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _CHAT_H_

#include "console.h"

#ifndef _CHATSERVER_H_
#include "..\source\ServerSuite\Chat\ChatServer.h"
#endif

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum CHAT_TYPE
{
	CHAT_TYPE_INVALID			= 0,

	CHAT_TYPE_WHISPER			= MAKE_MASK(0),
	CHAT_TYPE_PARTY				= MAKE_MASK(1),
	CHAT_TYPE_GAME				= MAKE_MASK(2),
	CHAT_TYPE_SERVER			= MAKE_MASK(3),
	CHAT_TYPE_QUEST				= MAKE_MASK(4),
	CHAT_TYPE_GUILD				= MAKE_MASK(5),
	CHAT_TYPE_ANNOUNCEMENT		= MAKE_MASK(6),
	CHAT_TYPE_CLIENT_ERROR		= MAKE_MASK(7),
	CHAT_TYPE_CSR				= MAKE_MASK(8),
	CHAT_TYPE_SHOUT				= MAKE_MASK(9),
	CHAT_TYPE_EMOTE				= MAKE_MASK(10),
	CHAT_TYPE_MAIL				= MAKE_MASK(11),
	CHAT_TYPE_REPLY				= MAKE_MASK(12),
};
#define CHAT_TYPE_LOCALIZED_CMD CHAT_TYPE_CLIENT_ERROR

enum SOCIAL_INTERACTION_TYPE
{
	SOCIAL_INTERACTION_INVALID = -1,

	SOCIAL_INTERACTION_TRADE_REQUEST,
	SOCIAL_INTERACTION_DUEL_REQUEST,
};


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
struct VALIDATED_HYPERTEXT_MESSAGE
{
	ULONGLONG		messageId;
	PGUID			senderCharacterId;
	WCHAR			wszSenderCharacterName[MAX_CHARACTER_NAME];
	CHANNELID		channelId;
	WCHAR			wszChannelName[MAX_CHAT_CNL_NAME];
	ITEM_CHAT_TYPE	eItemChatType;
	WCHAR			wszMessageText[(MAX_CHAT_MESSAGE/sizeof(WCHAR))];
	DWORD			hypertextDataLength;
	BYTE *			hypertextData;
};

struct EMOTE_DATA
{
	char			szName[DEFAULT_INDEX_SIZE];
	int				nCommandString;
	int				nDescriptionString;
	int				nTextString;
	int				nCommandStringEnglish;
	int				nDescriptionStringEnglish;
	int				nTextStringEnglish;
	int				nUnitMode;
	int				nRequiresAchievement;
};

//----------------------------------------------------------------------------
// UNIQUE CHANNEL NAMES
//----------------------------------------------------------------------------
extern const WCHAR * CHAT_CHANNEL_NAME_ANNOUNCEMENTS;
extern const WCHAR * CHAT_CHANNEL_NAME_ADMINISTRATORS;


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

void c_DisplayChat(
	CHAT_TYPE eChatType,
	const WCHAR * sender,
	const WCHAR * guild,
	const WCHAR * message,
	const WCHAR * channelName = NULL,
	PGUID guidSender = INVALID_GUID,
	UI_LINE * pLine = NULL );

void c_IgnoreChatChannel(
	const WCHAR * channelName );

void c_UnignoreChatChannel(
	const WCHAR * channelName );

BOOL c_ChatChannelIsIgnored(
	CHAT_TYPE eChatType,
	const WCHAR * channelName);

void c_ClearIgnoredChatChannels(
	void );

BOOL s_SendSysText(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game, 
	struct GAMECLIENT * client,
	const WCHAR * text,
	DWORD color = CONSOLE_SYSTEM_COLOR);

BOOL s_SendSysTextApp(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	APPCLIENTID idAppClient,
	const WCHAR * text,
	DWORD color = CONSOLE_SYSTEM_COLOR);

BOOL s_SendSysTextNet(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	NETCLIENTID64 idNetClient,
	const WCHAR * text,
	DWORD color = CONSOLE_SYSTEM_COLOR);

BOOL s_SendSysTextFmt1(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game,
	struct GAMECLIENT * client,
	DWORD color,
	const WCHAR * str);

BOOL s_SendSysTextFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	GAME * game,
	struct GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...);	

BOOL s_SendSysTextAppFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	APPCLIENTID idAppClient,
	DWORD color,
	const WCHAR * format,
	...);
	
BOOL s_SendSysTextNetFmt(
	CHAT_TYPE eChatType,
	BOOL bConsoleOnly,
	NETCLIENTID64 idNetClient,
	DWORD color,
	const WCHAR * format,
	...);	

DWORD ChatGetTypeColor(
	CHAT_TYPE eChatType );

DWORD ChatGetTypeNameColor(
	CHAT_TYPE eChatType );

void c_Emote(
	const WCHAR *uszMessage,
	int nUnitMode = INVALID_ID);

void c_Emote(
	int nEmoteId);

void c_ShowEmoteText(
	const WCHAR * wszText,
	DWORD textLen );

void s_Emote(
	GAME *pGame,
	UNIT *pUnit,
	const WCHAR *uszMessage);

//----------------------------------------------------------------------------

void c_SendInstancingChannelChat(
	const WCHAR *szInstancingChannel,
	const WCHAR *szChatMessage,
	UI_LINE *pLine);

void c_SendPartyChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine);

void c_SendTownChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine);

void c_SendWhisperChat(
	const WCHAR * szTargetPlayerName,
	const WCHAR * szChatMessage,
	UI_LINE * pLine);

void c_SendAdminChat(
	const WCHAR * szChatMessage);

void c_SendGuildChannelChat(
	const WCHAR * szChatMessage,
	UI_LINE * pLine);

void c_HypertextDataStartReceived(
	ULONGLONG		messageId,
	PGUID			senderCharacterId,
	WCHAR *			wszSenderCharacterName,
	CHANNELID		channelId,
	WCHAR *			wszChannelName,
	ITEM_CHAT_TYPE	eItemChatType,
	DWORD			hypertextDataLength );

void c_HypertextDataTextReceived(
	ULONGLONG		messageId,
	WCHAR *			wszMessageText );

void c_HypertextDataPacketReceived(
	ULONGLONG		messageId,
	BYTE *			hypertextData,
	DWORD			hyertextDataLen );

void c_ClearHypertextChatMessages(
	void );

BOOL x_EmoteCanPerform(
	UNIT *pPlayer,
	int nEmoteId);

BOOL x_EmoteCanPerform(
	UNIT *pPlayer,
	const struct EMOTE_DATA *pEmote);

#endif // _CHAT_H_
