//----------------------------------------------------------------------------
// s_chatNetwork.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "units.h"
#include "gameunits.h"
#include "c_network.h"
#include "c_ui.h"
#include "console.h"
#include "effect.h"
#include "unit_priv.h"
#include "unitmodes.h"
#include "missiles.h"
#include "objects.h"
#include "s_message.h"
#include "combat.h"
#include "items.h"
#include "player.h"
#include "inventory.h"
#include "weapons.h"
#include "movement.h"
#include "c_sound.h"
#include "skills.h"
#include "s_network.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "c_camera.h"
#include "states.h"
#include "uix.h"
#include "uix_components_complex.h"
#include "unittag.h"
#include "clients.h"
#include "c_message.h"
#include "pets.h"
#include "performance.h"
#include "npc.h"
#include "windowsmessages.h"
#include "c_tasks.h"
#include "stringtable.h"
#include "quests.h"
#include "monsters.h"
#include "c_trade.h"
#include "uix_money.h"
#include "filepaths.h"
#include "waypoint.h"
#include "e_main.h"
#include "e_settings.h"
#include "e_environment.h"
#include "e_irradiance.h"
#include "teams.h"
#include "c_townportal.h"
#include "perfhier.h"
#include "c_reward.h"
#include "jobs.h"
#include "svrstd.h"
#include "script.h"
#if !ISVERSION(SERVER_VERSION)
#include "EmbeddedServerRunner.h"
#else
#include "s_chatNetwork.cpp.tmh"
#include "s_townInstanceList.h"
#endif
#include "GameChatCommunication.h"
#include "s_chatNetwork.h"
#include "GameServer.h"
#include "gamelist.h"
#include "s_partyCache.h"
#include "openlevel.h"
#include "chat.h"
#include "gameglobals.h"
#include "email.h"

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Sends a request to the chat server to create a specified channel
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sCreateInstancingChannel(
	CHANNEL_DEFINITION * defInstancingChannel)
{
	CHAT_REQUEST_MSG_CREATE_INSTANCING_CHAT_CHANNEL globalCnlMsg;
	STATIC_CHECK(sizeof(defInstancingChannel->szName)/sizeof(defInstancingChannel->szName[0])
		== sizeof(globalCnlMsg.wszChannelName)/sizeof(globalCnlMsg.wszChannelName[0]), sizecheck);
	PStrCvt( globalCnlMsg.wszChannelName, defInstancingChannel->szName, 
		sizeof(defInstancingChannel->szName)/sizeof(defInstancingChannel->szName[0]));
	globalCnlMsg.wMaxMembers = (WORD)defInstancingChannel->nMaxMembers;
	globalCnlMsg.ullRequestCode = (ULONGLONG)-1;
	GameServerToChatServerSendMessage(
		&globalCnlMsg,
		GAME_REQUEST_CREATE_INSTANCING_CHAT_CHANNEL );
}
//----------------------------------------------------------------------------
// Looks at the chat spreadsheet for a list of instancing channels.
// Sends a request to the chat server to create each channel on the list.
//----------------------------------------------------------------------------
void CreateInstancingChannels()
{
	int nChannels = ExcelGetCount(EXCEL_CONTEXT(),
					DATATABLE_CHAT_INSTANCED_CHANNELS);
	for(int i = 0; i < nChannels; i++)
	{
		CHANNEL_DEFINITION *pChannelDef = (CHANNEL_DEFINITION *)
			ExcelGetData(NULL, DATATABLE_CHAT_INSTANCED_CHANNELS, i);
		if(pChannelDef) sCreateInstancingChannel(pChannelDef);
		else TraceError("No definition for channel %d in !FUNC!", i);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sGetCodeForInstancingChannel(
	GAME *pGame,
	UNIT *pUnit, 
	CHANNEL_DEFINITION * pChannelDef)
{
	ASSERT_RETZERO(pChannelDef);
	ASSERT_RETZERO(pUnit);
	ASSERT_RETZERO(pGame);
	if( pChannelDef->codeScriptChannelCode != NULL_CODE )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_CHAT_INSTANCED_CHANNELS,
			pChannelDef->codeScriptChannelCode, &code_len);
		if (code_ptr)
		{
			return VMExecI(pGame, pUnit, code_ptr, code_len);
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
// Get the channel def by matching the excel and call above function
//----------------------------------------------------------------------------
static DWORD sGetCodeForInstancingChannel(
	GAME *pGame,
	UNIT *pUnit, 
	WCHAR *szInstancingChannel)
{
	ASSERT_RETZERO(szInstancingChannel);
	int nChannels = ExcelGetCount(EXCEL_CONTEXT(pGame),
		DATATABLE_CHAT_INSTANCED_CHANNELS);
	for(int i = 0; i < nChannels; i++)
	{
		CHANNEL_DEFINITION *pChannelDef = (CHANNEL_DEFINITION *)
			ExcelGetData(pGame, DATATABLE_CHAT_INSTANCED_CHANNELS, i);
		if(pChannelDef)
		{
			WCHAR szName[MAX_INSTANCING_CHANNEL_NAME];
			PStrCvt(szName, pChannelDef->szName, MAX_INSTANCING_CHANNEL_NAME);
			if(PStrICmp(szName, szInstancingChannel))
				return sGetCodeForInstancingChannel(pGame, pUnit, pChannelDef);
		}
	}
	ASSERT_RETZERO(FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ChatNetAddPlayerToInstancingChannel(
	PGUID idPlayer,
	GAME *pGame,
	UNIT *pUnit, 
	WCHAR *szInstancingChannel,
	CHANNEL_DEFINITION * pChannelDef /* = NULL */)
{
	UNREFERENCED_PARAMETER(pUnit);
	CHAT_REQUEST_MSG_ADD_INSTANCING_CHANNEL_MEMBER addMbrMsg;
	PStrCopy( addMbrMsg.wszChannelName, szInstancingChannel, 
		MAX_INSTANCING_CHANNEL_NAME );
	addMbrMsg.TagMember  = idPlayer;
	
	if(pChannelDef)
	{
		addMbrMsg.dwCode = sGetCodeForInstancingChannel(pGame, pUnit, pChannelDef);
	}
	else
	{
		addMbrMsg.dwCode = sGetCodeForInstancingChannel(pGame, pUnit, szInstancingChannel);
	}
	GameServerToChatServerSendMessage(
		&addMbrMsg,
		GAME_REQUEST_ADD_INSTANCING_CHANNEL_MEMBER );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetRetrieveGuildInvitesSentOffline(
	UNIT *pUnit )
{
	ASSERT_RETURN(pUnit);

	CHAT_REQUEST_MSG_RETRIEVE_GUILD_INVITES reqMsg;
	reqMsg.idPlayerGuid = UnitGetGuid(pUnit);
	UnitGetName(pUnit, reqMsg.wszPlayerName, MAX_CHARACTER_NAME, 0);

	GameServerToChatServerSendMessage(
		&reqMsg,
		GAME_REQUEST_RETRIEVE_GUILD_INVITES );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetRetrieveGuildAssociation(
	UNIT * pUnit)
{
	ASSERT_RETURN(pUnit);

	if (!UnitIsA(pUnit, UNITTYPE_PLAYER))
		return;

	PGUID guidPlayer = UnitGetGuid(pUnit);
	WCHAR wszPlayerName[MAX_CHARACTER_NAME];
	UnitGetName(pUnit, wszPlayerName, MAX_CHARACTER_NAME, 0);

	s_ChatNetRetrieveGuildAssociation(guidPlayer, wszPlayerName);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetRetrieveGuildAssociation(
	PGUID guidPlayer,
	const WCHAR * wszPlayerName)
{
	ASSERT_RETURN(guidPlayer != INVALID_GUID);
	ASSERT_RETURN(wszPlayerName && wszPlayerName[0]);

	CHAT_REQUEST_MSG_RETRIEVE_GUILD_ASSOCIATION reqMsg;
	reqMsg.idPlayerGuid = guidPlayer;
	PStrCopy(reqMsg.wszPlayerName, wszPlayerName, MAX_CHARACTER_NAME);

	GameServerToChatServerSendMessage(
		&reqMsg,
		GAME_REQUEST_RETRIEVE_GUILD_ASSOCIATION );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ChatNetSendUnitGuildAssociationToClient(
	GAME * pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit )
{
	ASSERT_RETURN(pGame && pClient && pUnit);
#if ISVERSION(SERVER_VERSION)
	if (UnitIsA(pUnit, UNITTYPE_PLAYER))
	{
		GAMECLIENT * guildMbrClient = UnitGetClient( pUnit );
		if (!guildMbrClient)
			return;

		MSG_SCMD_UNIT_GUILD_ASSOCIATION guildMsg;
		guildMsg.id = UnitGetId( pUnit );
		s_ClientGetGuildAssociation( guildMbrClient, guildMsg.wszGuildName, MAX_GUILD_NAME, guildMsg.eGuildRank, guildMsg.wszRankName, MAX_CHARACTER_NAME );

		if (!guildMsg.wszGuildName[0])
			return;

		SendMessageToClient( pGame, pClient, SCMD_UNIT_GUILD_ASSOCIATION, &guildMsg );
	}
#endif
}

//----------------------------------------------------------------------------
// Currently, we ignore the chat server ID and assume that there is only
// instance 0, as it's currently a singleton.
//----------------------------------------------------------------------------
void s_ChatNetInitForApp(
	GameServerContext * serverContext,
	SVRID idChatServer)
{
	UNREFERENCED_PARAMETER(idChatServer);

	{
		CHAT_REQUEST_MSG_SET_CHAT_SERVER_DEFAULTS defaults;
		defaults.wDefMaxPartyMembers = (WORD)MAX_PARTY_MEMBERS;
		GameServerToChatServerSendMessage(
			&defaults,
			GAME_REQUEST_SET_CHAT_SERVER_DEFAULTS);
	}

	CreateInstancingChannels();

	//We're willing to re-initialize chat, just in case the chat server has been restarted.
	serverContext->m_chatInitializedForApp = TRUE;

}

//----------------------------------------------------------------------------
// Adds player to all opt-out chat channels.  Currently only "chat"
//----------------------------------------------------------------------------
void s_ChatNetAddPlayerToDefaultInstancingChannels(
	PGUID idPlayer, 
	GAME *pGame,
	UNIT *pUnit)
{
	int nChannels = ExcelGetCount(EXCEL_CONTEXT(pGame),
		DATATABLE_CHAT_INSTANCED_CHANNELS);
	for(int i = 0; i < nChannels; i++)
	{
		CHANNEL_DEFINITION *pChannelDef = (CHANNEL_DEFINITION *)
			ExcelGetData(pGame, DATATABLE_CHAT_INSTANCED_CHANNELS, i);
		if(pChannelDef && pChannelDef->bOptOut) 
		{
			WCHAR szName[MAX_INSTANCING_CHANNEL_NAME];
			PStrCvt(szName, pChannelDef->szName, MAX_INSTANCING_CHANNEL_NAME);
			s_ChatNetAddPlayerToInstancingChannel(
				idPlayer, pGame, pUnit, szName, pChannelDef);
		}
	}
}
#endif

//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetSendSystemMessageToMember(
	PGUID idCharacter,
	CHAT_TYPE eMessageType,
	const WCHAR* szMessage,
	PGUID MessageGuid )
{
	ASSERT_RETURN(idCharacter != INVALID_GUID);
	ASSERT_RETURN(eMessageType != CHAT_TYPE_INVALID);

	CHAT_REQUEST_MSG_SEND_SYSTEM_MESSAGE_TO_MEMBER reqMsg;
	reqMsg.TagMember = idCharacter;
	reqMsg.eMessageType = eMessageType;
	reqMsg.Message.SetWStrMsg(szMessage);
	reqMsg.MessageGuid = MessageGuid;

	GameServerToChatServerSendMessage(
		&reqMsg,
		GAME_REQUEST_SEND_SYSTEM_MESSAGE_TO_MEMBER );
}
#endif

//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetRequestPlayerDataForWarpToPlayer(
	GAME * pGame,
	UNIT * pUnit,
	PGUID targetUnitGuid,
	ULONGLONG ullRequestCode /*= 0*/ )
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN(targetUnitGuid != INVALID_GUID);

	CHAT_REQUEST_MSG_REQUEST_MEMBER_GAMESVR_INFO requestMsg;
	requestMsg.TagMember = targetUnitGuid;
	requestMsg.ullRequestCode = ullRequestCode;
	requestMsg.idRequestingGame = GameGetId(pGame);
	requestMsg.ullRequestingAccountId = UnitGetAccountId(pUnit);

	GameServerToChatServerSendMessage(
		&requestMsg,
		GAME_REQUEST_REQUEST_MEMBER_GAMESVR_INFO);
}
#endif

//----------------------------------------------------------------------------
void s_ChatNetRegisterNewInstance(
	GAMEID idGame,
	int nLevelDefinition,
	int nInstanceType,
	int nPVPWorld )
{
#if ISVERSION(SERVER_VERSION)
	CHAT_REQUEST_MSG_REGISTERED_AREA_INSTANCE msg;
	msg.idAreaType = nLevelDefinition;
	msg.idGameInstance = idGame;
	msg.nInstanceType = nInstanceType;
	msg.nPVPWorld = nPVPWorld;
	GameServerToChatServerSendMessage(&msg, GAME_REQUEST_NEW_AREA_INSTANCE);
#else
	UNREFERENCED_PARAMETER(idGame);
	UNREFERENCED_PARAMETER(nLevelDefinition);
#endif
}

//----------------------------------------------------------------------------
void s_ChatNetInstanceRemoved(
	GAMEID idGame,
	int nLevelDefinition,
	int nPVPWorld )
{
#if ISVERSION(SERVER_VERSION)
	CHAT_REQUEST_MSG_REGISTERED_AREA_INSTANCE msg;
	msg.idAreaType = nLevelDefinition;
	msg.idGameInstance = idGame;
	msg.nPVPWorld = nPVPWorld;
	GameServerToChatServerSendMessage(&msg, GAME_REQUEST_DELETED_AREA_INSTANCE);
#else
	UNREFERENCED_PARAMETER(idGame);
	UNREFERENCED_PARAMETER(nLevelDefinition);
#endif
}

//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void s_ChatNetSendHypertextChatMessage(
	GAMECLIENT * pClient,
	UNIT * pUnit,
	ITEM_CHAT_TYPE eItemChatType,
	const WCHAR * wszTargetPlayerName,
	CHANNELID targetChannelId,
	const WCHAR * wszMessageText,
	BYTE * hypertextData,
	DWORD hypertextDataLength )
{
	ASSERT_RETURN(pClient);
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN(wszTargetPlayerName);
	ASSERT_RETURN(wszMessageText);
	ASSERT_RETURN(hypertextData);

	ULONGLONG messageId = GameServerGenerateGUID();
	UNIQUE_ACCOUNT_ID accountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(pClient));
	PGUID characterId = UnitGetGuid(pUnit);

	{
		CHAT_REQUEST_MSG_HYPERTEXT_CHAT_MESSAGE_START startMsg;
		startMsg.messageId = messageId;
		startMsg.senderAccountId = accountId;
		startMsg.senderCharacterId = characterId;
		startMsg.eItemChatType = (BYTE)eItemChatType;
		PStrCopy(startMsg.wszTargetName, wszTargetPlayerName, MAX_CHARACTER_NAME);
		startMsg.targetChannelId = targetChannelId;
		PStrCopy(startMsg.wszMessageText, wszMessageText, (MAX_CHAT_MESSAGE/sizeof(WCHAR)));
		startMsg.hypertextDataLength = hypertextDataLength;

		ASSERT_RETURN(GameServerToChatServerSendMessage(&startMsg, GAME_REQUEST_HYPERTEXT_CHAT_MESSAGE_START));
	}

	DWORD bytesSent = 0;
	while (bytesSent < hypertextDataLength)
	{
		CHAT_REQUEST_MSG_HYPERTEXT_CHAT_MESSAGE_DATA dataMsg;
		dataMsg.messageId = messageId;
		dataMsg.senderAccountId = accountId;
		dataMsg.senderCharacterId = characterId;
		dataMsg.eItemChatType = (BYTE)eItemChatType;
		PStrCopy(dataMsg.wszTargetName, wszTargetPlayerName, MAX_CHARACTER_NAME);
		dataMsg.targetChannelId = targetChannelId;

		DWORD bytesToSend = MIN((DWORD)MAX_HYPERTEXT_DATA_PER_MSG, hypertextDataLength - bytesSent);

		ASSERT_RETURN(
			MemoryCopy(dataMsg.HypertextData, MAX_HYPERTEXT_DATA_PER_MSG, hypertextData + bytesSent, bytesToSend));

		MSG_SET_BLOB_LEN(dataMsg, HypertextData, bytesToSend);
		bytesSent += bytesToSend;

		ASSERT_RETURN(GameServerToChatServerSendMessage(&dataMsg, GAME_REQUEST_HYPERTEXT_CHAT_MESSAGE_DATA));
	}
}
#endif

//----------------------------------------------------------------------------
// MESSAGE HANDLERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPostToPlayerGameMailbox(
	GameServerContext * serverContext,
	GAMEID playerGameId,
	UNIQUE_ACCOUNT_ID playerAccountId,
	__notnull MSG_STRUCT * msg,
	NET_CMD cmd)
{
#if ISVERSION(SERVER_VERSION)
	SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, playerGameId);
	NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(playerAccountId);
	if( pMailbox && clientConnectionId != INVALID_NETCLIENTID64 )
	{
		return SvrNetPostFakeClientRequestToMailbox(pMailbox, clientConnectionId, msg, cmd);
	}
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPostToGameMailbox(
	GameServerContext * serverContext,
	GAMEID idGame,
	__notnull MSG_STRUCT * msg,
	NET_CMD cmd)
{
#if ISVERSION(SERVER_VERSION)
	SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, idGame);
	if( pMailbox )
	{
		return SvrNetPostFakeClientRequestToMailbox(pMailbox, ServiceUserId( USER_SERVER, 0, 0 ), msg, cmd);
	}
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_OPERATION_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_GET_GAME_STARTUP_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	//If/when we have multiple chat servers, we will need to pass in the sender here.
	s_ChatNetInitForApp(serverContext, sender);

	//	TODO: if/when we have multiple chat servers, only queue this message if it was "our" chat server that's coming back online...

	PartyCacheClearCache(serverContext->m_PartyCache);

	TownInstanceListClearList(serverContext->m_pInstanceList);

	//	queue an internal message to re-establish all level channels in case this is a chat server coming back on-line
	MSG_APPCMD_RECOVER_LEVEL_CHAT_CHANNELS recoverMessage;
	AppPostMessageToMailbox(
		ServiceUserId(USER_SERVER,0,INVALID_SVRCONNECTIONID),
		APPCMD_RECOVER_LEVEL_CHAT_CHANNELS,
		&recoverMessage );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_ADD_AREA_INSTANCE_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	CHAT_RESPONSE_MSG_REGISTERED_AREA_INSTANCE * areaMsg = (CHAT_RESPONSE_MSG_REGISTERED_AREA_INSTANCE*)msg;
	ASSERT(
		TownInstanceListAddInstance(
			serverContext->m_pInstanceList,
			areaMsg->idAreaType,
			areaMsg->idGame,
			areaMsg->areaInstanceNumber,
			areaMsg->nInstanceType,
			areaMsg->nPVPWorld));
#else
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_REMOVE_AREA_INSTANCE_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	CHAT_RESPONSE_MSG_REGISTERED_AREA_INSTANCE * areaMsg = (CHAT_RESPONSE_MSG_REGISTERED_AREA_INSTANCE*)msg;

	ASSERT(
		TownInstanceListRemoveInstance(
			serverContext->m_pInstanceList,
			areaMsg->idAreaType,
			areaMsg->idGame,
			areaMsg->areaInstanceNumber,
			areaMsg->nPVPWorld));
#else
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_MEMBER_SIGNED_IN_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	UNREFERENCED_PARAMETER(sender);
	CHAT_RESPONSE_MSG_MEMBER_SIGNED_IN * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_MEMBER_SIGNED_IN*)msg;
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	//	add them to their level chat channel
	APPCLIENTID idAppClient = ClientSystemFindByPguid(
									serverContext->m_ClientSystem,
									myMsg->MemberGuid );

	if( idAppClient != INVALID_CLIENTID )
	{
		GAMEID				idGame =	ClientSystemGetClientGame(
											serverContext->m_ClientSystem,
											idAppClient );
		SERVER_MAILBOX *	pMailbox =	GameListGetPlayerToGameMailbox(
											serverContext->m_GameList,
											idGame );

		if( pMailbox )
		{
			MSG_APPCMD_PLAYER_IN_CHAT_SERVER loggedInMsg;
			ASSERT(
				SvrNetPostFakeClientRequestToMailbox(
					pMailbox,
					ClientGetNetId(
						serverContext->m_ClientSystem,
						idAppClient ),
					&loggedInMsg,
					APPCMD_PLAYER_IN_CHAT_SERVER ) );
		}
		else
		{
			TraceError("Error processing player logged into chat server command. Character PGUID: %#018I64x", myMsg->MemberGuid);
		}

		SVR_VERSION_ONLY(s_ChatNetRetrieveGuildAssociation(myMsg->MemberGuid, myMsg->MemberName));
	}

#if !ISVERSION(SERVER_VERSION)
	if(!serverContext->m_chatInitializedForApp) s_ChatNetInitForApp(serverContext, sender);
#endif

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_SOCIAL_INTERACTION_REQUEST_HANDLER(
	__notnull LPVOID svrContext,
	SVRID,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	CHAT_RESPONSE_MSG_SOCIAL_INTERACTION_REQUEST * myMsg = (CHAT_RESPONSE_MSG_SOCIAL_INTERACTION_REQUEST*)msg;
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	switch (myMsg->dwRequestType)
	{
	case SOCIAL_INTERACTION_TRADE_REQUEST:
		{
			MSG_CCMD_TRADE_REQUEST_NEW tradeMsg;
			tradeMsg.guidToTradeWith = myMsg->TargetGuid;
			ASSERT(sPostToPlayerGameMailbox(
					serverContext,
					myMsg->RequesterGameId,
					myMsg->RequesterAccountId,
					&tradeMsg,
					CCMD_TRADE_REQUEST_NEW));
		}
		break;
	case SOCIAL_INTERACTION_DUEL_REQUEST:
		{
			MSG_CCMD_DUEL_INVITE duelMsg;
			duelMsg.guidOpponent = myMsg->TargetGuid;
			ASSERT(sPostToPlayerGameMailbox(
				serverContext,
				myMsg->RequesterGameId,
				myMsg->RequesterAccountId,
				&duelMsg,
				CCMD_DUEL_INVITE));
		}
		break;
	default:
		goto _ERROR;
	}
_ERROR:
	TraceError("Error processing social interaction request from chat server. Requester account id: %#018I64x, Requester game id: %#018I64x, Request type: %x",
		myMsg->RequesterAccountId, myMsg->RequesterGameId, myMsg->dwRequestType );
#else
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_MEMBER_GAMESVR_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	CHAT_RESPONSE_MSG_MEMBER_GAMESVR_INFO * myMsg = (CHAT_RESPONSE_MSG_MEMBER_GAMESVR_INFO*)msg;
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	MSG_APPCMD_WARP_TO_PLAYER_RESULT playerInfoMsg;
	playerInfoMsg.ullRequestCode = myMsg->ullRequestCode;
	playerInfoMsg.bFoundTargetMember = myMsg->bFoundTargetMember;
	PStrCopy(playerInfoMsg.TargetMemberName, myMsg->TargetMemberName, MAX_CHARACTER_NAME);
	playerInfoMsg.TargetMemberGuid = myMsg->TargetMemberGuid;
	playerInfoMsg.TargetMemberGameData.Set(&myMsg->TargetMemberGameData);

	if (!sPostToPlayerGameMailbox(serverContext, myMsg->idRequestingGame, myMsg->ullRequestingAccountId, &playerInfoMsg, APPCMD_WARP_TO_PLAYER_RESULT))
	{
		TraceError("Error processing member gamesvr info from chat server. Requester account id: %#018I64x, Requester game id: %#018I64x, Request type: %x",
			myMsg->ullRequestingAccountId, myMsg->idRequestingGame, myMsg->ullRequestCode );
	}
#else
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_CHANNEL_CREATED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	CHAT_RESPONSE_MSG_CHANNEL_CREATED * myMsg = (CHAT_RESPONSE_MSG_CHANNEL_CREATED*)msg;
	GameServerContext * serverContext = (GameServerContext*)svrContext;

	GAMEID idGame = INVALID_ID;
	LEVELID idLevel = INVALID_ID;

	if(s_ChatNetDisectRequestCode(myMsg->ullRequestCode,idGame,idLevel))
	{
		SERVER_MAILBOX * pMailbox = GameListGetPlayerToGameMailbox(
										serverContext->m_GameList,
										idGame );
		if( pMailbox )
		{
			MSG_APPCMD_CHAT_CHANNEL_CREATED channelMsg;
			channelMsg.idLevel = idLevel;
			channelMsg.idChatChannel = myMsg->IdChannel;
			ASSERT(
				SvrNetPostFakeClientRequestToMailbox(
					pMailbox,
					ServiceUserId(
						USER_SERVER,
						INVALID_SVRINSTANCE,
						INVALID_SVRCONNECTIONID),
					&channelMsg,
					APPCMD_CHAT_CHANNEL_CREATED ) );
		}
		else
		{
			trace("Got a chat channel for a closed game.");
			s_ChatNetReleaseChatChannel(
				myMsg->IdChannel );
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_CREATED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_CREATED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_CREATED*)msg;

	PartyCacheAddParty(
		serverContext->m_PartyCache,
		myMsg->IdParty,
		&myMsg->PartyData );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_DISBANDED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_DISBANDED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_DISBANDED*)msg;

	PartyCacheRemoveParty(
		serverContext->m_PartyCache,
		myMsg->IdParty );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_MEMBER_ADDED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_ADDED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_ADDED*)msg;

	PartyCacheAddPartyMember(
		serverContext->m_PartyCache,
		myMsg->IdParty,
		myMsg->MemberGuid,
		ClientSystemFindByPguid(
			serverContext->m_ClientSystem,
			myMsg->MemberGuid ),
		&myMsg->MemberGameData );

	MSG_APPCMD_PARTY_MEMBER_INFO_APP partyInfoMsg;
	partyInfoMsg.IdGame = myMsg->IdGame;
	partyInfoMsg.ullPlayerAccountId = myMsg->ullPlayerAccountId;
	partyInfoMsg.IdPlayerCharacterGuid = myMsg->MemberGuid;
	partyInfoMsg.IdNewPartyChannel = myMsg->IdParty;
	partyInfoMsg.LeaveReason = (BYTE)PARTY_LEAVE_REASON_INVALID;

	ASSERT(
		AppPostMessageToMailbox(
			ServiceUserId(
				USER_SERVER,
				INVALID_SVRINSTANCE,
				INVALID_SVRCONNECTIONID),
			APPCMD_PARTY_MEMBER_INFO_APP,
			&partyInfoMsg));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_MEMBER_REMOVED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_REMOVED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_REMOVED*)msg;

	PartyCacheRemovePartyMember(
		serverContext->m_PartyCache,
		myMsg->IdParty,
		myMsg->MemberGuid );

	MSG_APPCMD_PARTY_MEMBER_INFO_APP partyInfoMsg;
	partyInfoMsg.IdGame = myMsg->IdGame;
	partyInfoMsg.ullPlayerAccountId = myMsg->ullPlayerAccountId;
	partyInfoMsg.IdPlayerCharacterGuid = myMsg->MemberGuid;
	partyInfoMsg.IdNewPartyChannel = INVALID_CHANNELID;
	partyInfoMsg.LeaveReason = myMsg->LeavingReasonCode;

	ASSERT(
		AppPostMessageToMailbox(
			ServiceUserId(
				USER_SERVER,
				INVALID_SVRINSTANCE,
				INVALID_SVRCONNECTIONID),
			APPCMD_PARTY_MEMBER_INFO_APP,
			&partyInfoMsg));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_DATA_UPDATED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_DATA_UPDATED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_DATA_UPDATED*)msg;

	CachedPartyPtr party(serverContext->m_PartyCache);
	party = PartyCacheLockParty(serverContext->m_PartyCache,myMsg->IdParty);
	if( party )
	{
		party->PartyData.Set( &myMsg->PartyData );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_MEMBER_UPDATED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_UPDATED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_MEMBER_UPDATED*)msg;

	CachedPartyMemberPtr member(serverContext->m_PartyCache);
	member = PartyCacheLockPartyMember(
		serverContext->m_PartyCache,
		myMsg->IdParty,
		myMsg->MemberGuid );
	if( member )
	{
		member->MemberData.Set( &myMsg->MemberGameData );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PARTY_LEADER_CHANGED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	REF(svrContext);
	REF(sender);
	REF(msg);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	CHAT_RESPONSE_MSG_GAME_PARTY_LEADER_CHANGED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_GAME_PARTY_LEADER_CHANGED*)msg;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_REQUESTED_PARTY_CREATED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	UNREFERENCED_PARAMETER(sender);
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_REQUESTED_PARTY_CREATED * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_REQUESTED_PARTY_CREATED*)msg;

	PARTYID idParty = (PARTYID)myMsg->IdParty;
	GAMEID idGame = myMsg->idGame;	

	// get mailbox for the game instance that created the party	... it's entirely
	// possible that the mailbox does not exist because the game has been closed
	SERVER_MAILBOX * pMailbox = GameListGetPlayerToGameMailbox( serverContext->m_GameList, idGame );
	if( pMailbox )
	{
		MSG_APPCMD_AUTO_PARTY_CREATED tMessage;
		tMessage.idParty = idParty;
		tMessage.ullRequestCode = myMsg->ullRequestCode;
		ASSERT(
			SvrNetPostFakeClientRequestToMailbox(
				pMailbox,
				ServiceUserId(
					USER_SERVER,
					INVALID_SVRINSTANCE,
					INVALID_SVRCONNECTIONID),
				&tMessage,
				APPCMD_AUTO_PARTY_CREATED) );
	}
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PLAYER_GUILD_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNREFERENCED_PARAMETER(sender);
	CHAT_RESPONSE_MSG_PLAYER_GUILD_INFO * infoMsg = NULL;
	infoMsg = (CHAT_RESPONSE_MSG_PLAYER_GUILD_INFO*)msg;

	MSG_APPCMD_PLAYER_GUILD_DATA_APP selfMsgInfoMsg;
	PStrCopy(selfMsgInfoMsg.wszPlayerName, MAX_CHARACTER_NAME, infoMsg->wszPlayerName, MAX_CHARACTER_NAME);
	selfMsgInfoMsg.ullPlayerGuid = infoMsg->ullPlayerGuid;
	selfMsgInfoMsg.ullPlayerAccountId = infoMsg->ullPlayerAccountId;
	PStrCopy(selfMsgInfoMsg.wszGuildName, MAX_CHAT_CNL_NAME, infoMsg->wszGuildName, MAX_CHAT_CNL_NAME);
	selfMsgInfoMsg.eGuildRank = infoMsg->eGuildRank;
	PStrCopy(selfMsgInfoMsg.wszRankName, infoMsg->wszRankName, MAX_CHARACTER_NAME);

	ASSERT(
		AppPostMessageToMailbox(
			ServiceUserId(
				USER_SERVER,
				INVALID_SVRINSTANCE,
				INVALID_SVRCONNECTIONID),
			APPCMD_PLAYER_GUILD_DATA_APP,
			&selfMsgInfoMsg));

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_PLAYER_GUILD_ACTION_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	CHAT_RESPONSE_MSG_PLAYER_GUILD_ACTION_RESULT * resMessage = NULL;
	resMessage = (CHAT_RESPONSE_MSG_PLAYER_GUILD_ACTION_RESULT*)msg;

	MSG_APPCMD_PLAYER_GUILD_ACTION_RESULT actionMessage;
	PStrCopy(actionMessage.wszPlayerName, MAX_CHARACTER_NAME, resMessage->wszPlayerName, MAX_CHARACTER_NAME);
	actionMessage.ullPlayerGuid = resMessage->ullPlayerGuid;
	actionMessage.ullPlayerAccountId = resMessage->ullPlayerAccountId;
	actionMessage.wPlayerRequest = resMessage->wPlayerRequest;
	actionMessage.wActionResult = resMessage->wActionResult;

	AppPostMessageToMailbox(
		ServiceUserId(
			USER_SERVER,
			INVALID_SVRINSTANCE,
			INVALID_SVRCONNECTIONID),
		APPCMD_PLAYER_GUILD_ACTION_RESULT,
		&actionMessage);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_CREATION_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_CREATION_RESULT * resMsg = (CHAT_RESPONSE_MSG_EMAIL_CREATION_RESULT*)msg;

	MSG_APPCMD_EMAIL_CREATION_RESULT createResult;
	createResult.idEmailMessageId = resMsg->idEmailMessageId;
	createResult.dwfEmailCreationResult = resMsg->dweEmailCreationResult;
	createResult.eEmailSpecSource = resMsg->eEmailSpecSource;
	createResult.dwUserContext = resMsg->dwUserContext;
	PStrCopy(createResult.wszIgnoringTarget, resMsg->wszIgnoringTarget1, MAX_CHARACTER_NAME);

	// post to the game that originated the message
	if (resMsg->eEmailSpecSource == ESS_PLAYER ||
		resMsg->eEmailSpecSource == ESS_PLAYER_ITEMSALE ||
		resMsg->eEmailSpecSource == ESS_PLAYER_ITEMBUY)
	{
	
		// for players, we will send to whatever game they are in ... in theory, this would
		// account for players that switched games after sending an email and it could still work
		ASSERT(
			sPostToPlayerGameMailbox(
				serverContext,
				resMsg->idSenderGameId,
				resMsg->idSenderAccountId,
				&createResult,
				APPCMD_EMAIL_CREATION_RESULT));
	}
	else
	{
		GAMEID idGame = resMsg->idSenderGameId;
		if (idGame != INVALID_GAMEID)
		{
		
			MSG_APPCMD_EMAIL_CREATION_RESULT_UTIL_GAME tMessage;
			tMessage.tResult = createResult;
			
			ASSERT(
				sPostToGameMailbox(
					serverContext,
					idGame,
					&tMessage,
					APPCMD_EMAIL_CREATION_RESULT_UTIL_GAME));		
		}		
	}
	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_DELIVERY_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_DELIVERY_RESULT * resMsg = (CHAT_RESPONSE_MSG_EMAIL_DELIVERY_RESULT*)msg;

	// setup the result message
	MSG_APPCMD_EMAIL_DELIVERY_RESULT deliveryResult;
	deliveryResult.bEmailDeliveryResult = resMsg->bEmailDeliveryResult;
	deliveryResult.idEmailMessageId	= resMsg->idEmailMessageId;
	deliveryResult.eEmailSpecSource = resMsg->eEmailSpecSource;

	// send the response to the game that generated the email ... note that we're
	// considering "system" emails from ESS_CSR, ESS_AUCTION etc as going to the well
	// known utility game
	
	EMAIL_SPEC_SOURCE eEmailSpecSource = (EMAIL_SPEC_SOURCE)resMsg->eEmailSpecSource;
	if (eEmailSpecSource == ESS_PLAYER ||
		eEmailSpecSource == ESS_PLAYER_ITEMSALE ||
		eEmailSpecSource == ESS_PLAYER_ITEMBUY)
	{
		ASSERT(
			sPostToPlayerGameMailbox(
				serverContext,
				resMsg->idSenderGameId,
				resMsg->idSenderAccountId,
				&deliveryResult,
				APPCMD_EMAIL_DELIVERY_RESULT));
	}
	else
	{
		GAMEID idGame = resMsg->idSenderGameId;
		if (idGame != INVALID_GAMEID)
		{

			// have to wrap this in another message so that we can 
			// properly route it to the utility game ... man this is a pain in the ass!		
			MSG_APPCMD_EMAIL_DELIVERY_RESULT_UTIL_GAME tMessage;
			tMessage.tResult = deliveryResult;
			
			ASSERT(
				sPostToGameMailbox(
					serverContext,
					idGame,
					&tMessage,
					APPCMD_EMAIL_DELIVERY_RESULT_UTIL_GAME));		
		}		
	
	}
	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_RECEIVED_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_RECEIVED * notifyMsg = (CHAT_RESPONSE_MSG_EMAIL_RECEIVED*)msg;

	MSG_APPCMD_EMAIL_NOTIFICATION gameNotifyMsg;
	gameNotifyMsg.idEmailMessageId = notifyMsg->idEmailMessageId;

	ASSERT(
		sPostToPlayerGameMailbox(
			serverContext,
			notifyMsg->idTargetGameId,
			notifyMsg->idTargetAccountId,
			&gameNotifyMsg,
			APPCMD_EMAIL_NOTIFICATION));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_METADATA_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_METADATA * chatMsg = (CHAT_RESPONSE_MSG_EMAIL_METADATA*)msg;

	MSG_APPCMD_EMAIL_METADATA metadataMsg;
	metadataMsg.idEmailMessageId = chatMsg->idEmailMessageId;
	metadataMsg.idSenderCharacterId = chatMsg->idSenderCharacterId;
	PStrCopy(metadataMsg.wszSenderCharacterName, chatMsg->wszSenderCharacterName, MAX_CHARACTER_NAME);
	metadataMsg.eMessageStatus = chatMsg->eMessageStatus;
	metadataMsg.eMessageType = chatMsg->eMessageType;
	ASSERT(MemoryCopy(metadataMsg.MessageContextData, sizeof(metadataMsg.MessageContextData), chatMsg->MessageContextData, MSG_GET_BLOB_LEN(chatMsg, MessageContextData)));
	MSG_SET_BLOB_LEN(metadataMsg, MessageContextData, MSG_GET_BLOB_LEN(chatMsg, MessageContextData));
	metadataMsg.timeSentUTC = chatMsg->timeSentUTC;
	metadataMsg.timeOfManditoryDeletionUTC = chatMsg->timeOfManditoryDeletionUTC;
	metadataMsg.bIsMarkedRead = chatMsg->bIsMarkedRead;
	metadataMsg.idAttachedItemId = chatMsg->idAttachedItemId;
	metadataMsg.dwAttachedMoney = chatMsg->dwAttachedMoney;
	metadataMsg.idAttachedMoneyUnitId = chatMsg->idAttachedMoneyUnitId;

	UNIQUE_ACCOUNT_ID idAccountAuctionChar = GameServerGetAuctionHouseAccountID(serverContext);
	if (idAccountAuctionChar != chatMsg->idTargetAccountId) {
		ASSERT(
			sPostToPlayerGameMailbox(
			serverContext,
			chatMsg->idTargetGameId,
			chatMsg->idTargetAccountId,
			&metadataMsg,
			APPCMD_EMAIL_METADATA));
	} else {
		SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, chatMsg->idTargetGameId);
		if (pMailbox) {
			ASSERT(SvrNetPostFakeClientRequestToMailbox(pMailbox, ServiceUserId(USER_SERVER, 0, 0),
				&metadataMsg, APPCMD_UTIL_GAME_EMAIL_METADATA));
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_ACCEPTED_EMAIL_DATA_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_ACCEPTED_EMAIL_DATA * chatMsg = (CHAT_RESPONSE_MSG_ACCEPTED_EMAIL_DATA*)msg;

	MSG_APPCMD_ACCEPTED_EMAIL_DATA dataMsg;
	dataMsg.idEmailMessageId = chatMsg->idEmailMessageId;
	PStrCopy(dataMsg.wszEmailSubject, chatMsg->wszEmailSubject, MAX_EMAIL_SUBJECT);
	PStrCopy(dataMsg.wszEmailBody, chatMsg->wszEmailBody, MAX_EMAIL_BODY);

	UNIQUE_ACCOUNT_ID idAccountAuctionChar = GameServerGetAuctionHouseAccountID(serverContext);
	if (idAccountAuctionChar != chatMsg->idTargetAccountId) {
		ASSERT(
			sPostToPlayerGameMailbox(
				serverContext,
				chatMsg->idTargetGameId,
				chatMsg->idTargetAccountId,
				&dataMsg,
				APPCMD_ACCEPTED_EMAIL_DATA));
	} else {
		SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, chatMsg->idTargetGameId);
		if (pMailbox) {
			ASSERT(SvrNetPostFakeClientRequestToMailbox(pMailbox, ServiceUserId(USER_SERVER, 0, 0),
				&dataMsg, APPCMD_UTIL_GAME_EMAIL_DATA));
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_UPDATE_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_UPDATE * chatMsg = (CHAT_RESPONSE_MSG_EMAIL_UPDATE*)msg;

	MSG_APPCMD_EMAIL_UPDATE updateMsg;
	updateMsg.idEmailMessageId = chatMsg->idEmailMessageId;
	updateMsg.timeOfManditoryDeletionUTC = chatMsg->timeOfManditoryDeletionUTC;
	updateMsg.bIsMarkedRead = chatMsg->bIsMarkedRead;
	updateMsg.idAttachedItemId = chatMsg->idAttachedItemId;
	updateMsg.dwAttachedMoney = chatMsg->dwAttachedMoney;
	updateMsg.idAttachedMoneyUnitId = chatMsg->idAttachedMoneyUnitId;

	ASSERT(
		sPostToPlayerGameMailbox(
			serverContext,
			chatMsg->idTargetGameId,
			chatMsg->idTargetAccountId,
			&updateMsg,
			APPCMD_EMAIL_UPDATE));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CHAT_GAME_RESPONSE_EMAIL_ATTACHED_MONEY_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	CHAT_RESPONSE_MSG_EMAIL_ATTACHED_MONEY_INFO * chatMsg = (CHAT_RESPONSE_MSG_EMAIL_ATTACHED_MONEY_INFO*)msg;

	MSG_APPCMD_EMAIL_ATTACHED_MONEY_INFO moneyInfo;
	moneyInfo.idEmailMessageId = chatMsg->idEmailMessageId;
	moneyInfo.dwMoneyAmmount = chatMsg->dwAttachedMoneyAmmount;
	moneyInfo.idMoneyUnitId = chatMsg->idAttachedMoneyItemId;

	ASSERT(
		sPostToPlayerGameMailbox(
			serverContext,
			chatMsg->idTargetGameId,
			chatMsg->idTargetAccountId,
			&moneyInfo,
			APPCMD_EMAIL_ATTACHED_MONEY_INFO));
#endif
}


//----------------------------------------------------------------------------
// RESPONSE HANDLERS ARRAY
//----------------------------------------------------------------------------
#undef   _GAME_CHAT_RESPONSE_TABLE_
#undef   NET_MSG_TABLE_BEGIN
#undef   NET_MSG_TABLE_DEF
#undef   NET_MSG_TABLE_END
#define  NET_MSG_TABLE_BEGIN(x)			FP_NET_RESPONSE_COMMAND_HANDLER sChatToGameResponseHandlers[] = {
#define  NET_MSG_TABLE_DEF(cmd,msg,s,r)	cmd##_HANDLER,
#define  NET_MSG_TABLE_END(x)			};
#include "GameChatCommunication.h"


//----------------------------------------------------------------------------
// HANDLER METHODS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL SrvValidateChatMessageTable(
	void )
{
	ASSERT_RETFALSE(ARRAYSIZE(sChatToGameResponseHandlers) == CHAT_GAME_RESPONSE_COUNT);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// MESSAGING METHODS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ChatNetAddPlayerToChannel(
	PGUID playerGuid,
	CHANNELID channelId )
{
	CHAT_REQUEST_MSG_ADD_CHANNEL_MEMBER addRequest;
	addRequest.TagChannel = channelId;
	addRequest.TagMember = playerGuid;
	GameServerToChatServerSendMessage(
		&addRequest,
		GAME_REQUEST_ADD_CHANNEL_MEMBER );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ChatNetRemovePlayerFromChannel(
	PGUID playerGuid,
	CHANNELID channelId )
{
	CHAT_REQUEST_MSG_REMOVE_CHANNEL_MEMBER rmvRequest;
	rmvRequest.TagChannel = channelId;
	rmvRequest.TagMember = playerGuid;
	GameServerToChatServerSendMessage(
		&rmvRequest,
		GAME_REQUEST_REMOVE_CHANNEL_MEMBER );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ChatNetRequestChatChannel(
	LEVEL * requestingLevel )
{
	ASSERT_RETURN(requestingLevel);

	CHAT_REQUEST_MSG_CREATE_CHAT_CHANNEL createMessage;
	createMessage.wszChannelName[0] = 0;
	createMessage.wMaxMembers = MAX_CHAT_CNL_MEMBERS;
	createMessage.ullRequestCode = s_ChatNetCreateRequestCode(requestingLevel);
	GameServerToChatServerSendMessage(
		&createMessage,
		GAME_REQUEST_CREATE_CHAT_CHANNEL );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ChatNetReleaseChatChannel(
	CHANNELID channel )
{
	CHAT_REQUEST_MSG_CLOSE_CHAT_CHANNEL closeMessage;
	closeMessage.TagChannel = channel;
	GameServerToChatServerSendMessage(
		&closeMessage,
		GAME_REQUEST_CLOSE_CHAT_CHANNEL );
}
#endif


//----------------------------------------------------------------------------
//		|    reserverd    |    game index   |    game itr     |    level id     |
//		|xxxxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx|
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
ULONGLONG s_ChatNetCreateRequestCode(
	struct LEVEL * pLevel )
{
	if( !pLevel )
		return (ULONGLONG)-1;

	GAME * pGame = LevelGetGame(pLevel);
	ASSERT_RETX(pGame,(ULONGLONG)-1);

	GAMEID idGame = GameGetId(pGame);
	return  ( (ULONGLONG)( GameIdGetIndex(idGame) & 0xFFFF ) << 32 ) |
			( (ULONGLONG)( GameIdGetIter( idGame) & 0xFFFF ) << 16 ) |
			  (ULONGLONG)( LevelGetID(    pLevel) & 0xFFFF );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ChatNetDisectRequestCode(
	ULONGLONG requestCode,
	GAMEID & idGame,
	LEVELID & idLevel )
{
	idLevel = (LEVELID)(requestCode & 0xFFFF);
	idGame = GameIdNew(
				(UINT)((requestCode >> 32) & 0xFFFF),
				(UINT)((requestCode >> 16) & 0xFFFF) );
	return (idLevel != 0xFFFF);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ChatNetLoginMember(UNIT * unit, UNIQUE_ACCOUNT_ID idAccount)
{
	ASSERT_RETFALSE(unit);
	//	log them into the chat server
	CHAT_REQUEST_MSG_LOGIN_MEMBER loginChatMember;
	loginChatMember.MemberAccountId = idAccount;
	loginChatMember.MemberGuid = UnitGetGuid(unit);
	UnitGetName(unit, loginChatMember.wszMemberName,MAX_CHARACTER_NAME,0);
	loginChatMember.PlayerData.idUnit			= UnitGetId(unit);
	loginChatMember.PlayerData.idGame			= UnitGetGameId(unit);
	loginChatMember.PlayerData.nPlayerUnitClass = UnitGetClass(unit);
	loginChatMember.PlayerData.nCharacterExpLevel=UnitGetExperienceLevel(unit);
	loginChatMember.PlayerData.nCharacterExpRank= UnitGetExperienceRank(unit);
	ASSERT_RETFALSE(GameServerToChatServerSendMessage(
		&loginChatMember,
		GAME_REQUEST_LOGIN_MEMBER ) );

	return TRUE;
}
#endif
