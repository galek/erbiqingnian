//----------------------------------------------------------------------------
// s_chatNetwork.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _S_CHATNETWORK_H_
#define _S_CHATNETWORK_H_
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CHANNEL_DEFINITION
{
	char	szName[27];
	BOOL	bOptOut;
	int		nMaxMembers;
	PCODE	codeScriptChannelCode;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL SrvValidateChatMessageTable(
	void);

void s_ChatNetRequestChatChannel(
	struct LEVEL *);

void s_ChatNetReleaseChatChannel(
	CHANNELID);

void s_ChatNetAddPlayerToChannel(
	PGUID,
	CHANNELID);

void s_ChatNetRemovePlayerFromChannel(
	PGUID,
	CHANNELID);

ULONGLONG s_ChatNetCreateRequestCode(
	struct LEVEL *);

BOOL s_ChatNetDisectRequestCode(
	ULONGLONG,
	GAMEID &,
	LEVELID &);

BOOL s_ChatNetLoginMember(UNIT * unit, UNIQUE_ACCOUNT_ID idAccount);

void s_ChatNetAddPlayerToInstancingChannel(
	PGUID idPlayer,
	GAME *pGame,
	UNIT *pUnit, 
	WCHAR *szInstanicngChannel,
	CHANNEL_DEFINITION * pChannelDef = NULL);

void s_ChatNetRetrieveGuildInvitesSentOffline(
	UNIT *pUnit );

void s_ChatNetRetrieveGuildAssociation(
	UNIT * pUnit );

void s_ChatNetRetrieveGuildAssociation(
	PGUID guidPlayer,
	const WCHAR * wszPlayerName);

void s_ChatNetSendUnitGuildAssociationToClient(
	GAME * pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit );

void s_ChatNetAddPlayerToDefaultInstancingChannels(
	PGUID idPlayer, 
	GAME *pGame,
	UNIT *pUnit );

void s_ChatNetSendSystemMessageToMember(
	PGUID idCharacter,
	CHAT_TYPE eMessageType,
	const WCHAR* szMessage,
	PGUID MessageGuid );

void s_ChatNetRequestPlayerDataForWarpToPlayer(
	GAME * pGame,
	UNIT * pUnit,
	PGUID targetCharacterGuid,
	ULONGLONG ullRequestCode = 0 );

void s_ChatNetRegisterNewInstance(
	GAMEID idGame,
	int nLevelDefinition,
	int nInstanceType,
	int nPVPWorld );

void s_ChatNetInstanceRemoved(
	GAMEID idGame,
	int nLevelDefinition,
	int nPVPWorld );

void s_ChatNetSendHypertextChatMessage(
	GAMECLIENT * pClient,
	UNIT * pUnit,
	ITEM_CHAT_TYPE eItemChatType,
	const WCHAR * wszTargetPlayerName,
	CHANNELID targetChannelId,
	const WCHAR * wszMessageText,
	BYTE * hypertextData,
	DWORD hypertextDataLength );


#endif //!ISVERSION(CLIENT_ONLY_VERSION)

#endif //_S_CHATNETWORK_H_