//----------------------------------------------------------------------------
// c_chatNetwork.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "console.h"
#include "chat.h"
#include "gag.h"
#include "player.h"
#include "uix.h"
#include "uix_components_complex.h"
#include "perfhier.h"
#include "c_voicechat.h"
#include "duel.h"
#include "c_trade.h"

#if !ISVERSION(SERVER_VERSION)
#include "c_message.h"
#include "c_connmgr.h"
#include "EmbeddedServerRunner.h"
#include "UserChatCommunication.h"
#include "c_chatNetwork.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "c_channelmap.h"
#include "player.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"

#define ALERT_FADEOUT_TIME			12.0f

//----------------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NET_COMMAND_TABLE * c_ChatNetGetClientRequestCmdTable(
	void )
{
	return UserChatGetRequestTbl();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NET_COMMAND_TABLE * c_ChatNetGetServerResponseCmdTable(
	void )
{
	ASSERT_RETNULL(c_ChatNetValidateCommandTable());
	return UserChatGetResponseTbl();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ChatMessageRerouteToServer(
	MSG_STRUCT * msg,
	CHAT_MESSAGE * chatMsg,
	NET_CMD command)
{
/* Not yet ready for prime time
	int len = MSG_GET_BLOB_LEN(chatMsg, HypertextData);
	if (len > 0)
	{
		// TODO: Reroute this message to the server for verification and forwarding
		return TRUE;
	}
*/
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetSendMessage(
	MSG_STRUCT * msg,
	NET_CMD command )
{
	BYTE buff[SMALL_MSG_BUF_SIZE];
	BYTE_BUF_NET byteBuff(buff,SMALL_MSG_BUF_SIZE);

	if(!ConnectionManagerSend(AppGetChatChannel(),msg,command))
	{
		goto _SEND_ERROR;
	}

	return;

_SEND_ERROR:
	GLOBAL_STRING eGSTitle = GS_BLANK;
	GLOBAL_STRING eGSMessage = GS_BLANK;
	if( AppGetType() == APP_TYPE_OPEN_CLIENT ||
		AppGetType() == APP_TYPE_CLOSED_CLIENT )
	{
		eGSTitle = GS_NETWORK_ERROR_TITLE;
		eGSMessage = GS_NETWORK_ERROR;
	}
	AppSetErrorState(
		eGSTitle,
		eGSMessage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetSendKeepAlive(
	void )
{
	CHAT_REQUEST_MSG_KEEP_ALIVE keepAlive;
	c_ChatNetSendMessage(&keepAlive, USER_REQUEST_KEEP_ALIVE);
}

//----------------------------------------------------------------------------
// COMMAND HANDLERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdResult(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_OPERATION_RESULT * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_OPERATION_RESULT*)data;
	WCHAR text[2048];

	if (myMsg->eResult == CHAT_ERROR_SOURCE_MEMBER_GAGGED)
	{
		c_GagDisplayGaggedMessage();
		return;
	}
	
	switch(myMsg->eRequest)
	{
	case USER_REQUEST_ADD_MEMBER_AS_BUDDY:
	case USER_REQUEST_REMOVE_MEMBER_AS_BUDDY:
	case USER_REQUEST_CHANGE_BUDDY_NICKNAME:
		c_BuddyOnOpResult(myMsg);
		break;
	case USER_REQUEST_INVITE_PARTY_MEMBER:
		if (myMsg->eResult == CHAT_ERROR_MEMBER_ALREADY_IN_A_PARTY)
		{
			PStrPrintf(text, sizeof(text)/sizeof(WCHAR), GlobalStringGet(GS_PARTY_INVITE_FAILED_ALREADY_IN_A_PARTY), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER), text);
		}
		else if (myMsg->eResult == CHAT_ERROR_PARTY_MODE_MISMATCH)
		{
			//	NOTE: myMsg->ullRequestCode actually has the mode of play for the target player if we want to someday tell the user what mode they are in.
			PStrPrintf(text, sizeof(text)/sizeof(WCHAR), GlobalStringGet(GS_PARTY_INVITE_FAILED_PARTY_MODE_MISMATCH), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER), text);
		}
		else if (myMsg->eResult == CHAT_ERROR_PARTY_ACCEPT_NOT_ALLOWED)
		{
			if (myMsg->ullRequestCode == PARTY_JOIN_RESTRICTION_DEAD_HARDCORE_PLAYER) {
				PStrPrintf(text, sizeof(text)/sizeof(WCHAR), GlobalStringGet(GS_PARTY_INVITE_FAILED_DEAD_HARDCORE_PLAYER), myMsg->wszResultData);
				UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER), text);
			}
		}
		else if (myMsg->eResult == CHAT_ERROR_PARTY_INVITE_NOT_ALLOWED)
		{
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),GlobalStringGet(GS_PARTY_INVITE_FAILED_INVITER_CANNOT_JOIN_PARTIES));
		}
		else if (myMsg->eResult == CHAT_ERROR_MEMBER_NOT_LOGGED_IN)
		{
			PStrPrintf(text, sizeof(text)/sizeof(WCHAR), GlobalStringGet(GS_PARTY_INVITE_FAILED_PLAYER_NOT_FOUND), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),text);
		}
		else if(myMsg->eResult == CHAT_ERROR_DIFFICULTY_MISMATCH)
		{
			PStrPrintf(text, arrsize(text), GlobalStringGet(GS_PARTY_INVITE_FAILED_PARTY_DIFFICULTY_MISMATCH), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),text);
		}
		else if(myMsg->eResult == CHAT_ERROR_HARDCORE_MISMATCH)
		{
			PStrPrintf(text, arrsize(text), GlobalStringGet(GS_PARTY_INVITE_FAILED_PARTY_HARDCORE_MISMATCH), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),text);
		}
		else if(myMsg->eResult == CHAT_ERROR_ELITE_MISMATCH)
		{
			PStrPrintf(text, arrsize(text), GlobalStringGet(GS_PARTY_INVITE_FAILED_PARTY_ELITE_MISMATCH), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),text);
		}
		else if(myMsg->eResult == CHAT_ERROR_LEAGUE_MISMATCH)
		{
			PStrPrintf(text, arrsize(text), GlobalStringGet(GS_PARTY_INVITE_FAILED_PARTY_LEAGUE_MISMATCH), myMsg->wszResultData);
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_INVITE_FAILED_HEADER),text);
		}
		
		break;
	case USER_REQUEST_ACCEPT_PARTY_INVITE:
	case USER_REQUEST_JOIN_PARTY:
		if (myMsg->eResult == CHAT_ERROR_PARTY_ACCEPT_NOT_ALLOWED)
		{
			switch (myMsg->ullRequestCode)
			{
			case PARTY_JOIN_RESTRICTION_NOT_IN_TOWN:
				UIShowGenericDialog(GlobalStringGet(GS_PARTY_ACCEPT_FAILED_HEADER), GlobalStringGet(GS_PARTY_ACCEPT_FAILED_NOT_IN_TOWN));
				break;
			case PARTY_JOIN_RESTRICTION_DEAD_HARDCORE_PLAYER:
				UIShowGenericDialog(GlobalStringGet(GS_PARTY_ACCEPT_FAILED_HEADER), GlobalStringGet(GS_PARTY_ACCEPT_FAILED_DIED_IN_HARDCORE));
				break;
			default:
				break;
			}
		}
		else if (myMsg->eResult == CHAT_ERROR_PARTY_MODE_MISMATCH)
		{
			UIShowGenericDialog(GlobalStringGet(GS_PARTY_ACCEPT_FAILED_HEADER), GlobalStringGet(GS_PARTY_ACCEPT_FAILED_MODE_MISMATCH));
		}
		break;
	case USER_REQUEST_REQUEST_SOCIAL_INTERACTION:
		if (myMsg->ullRequestCode == SOCIAL_INTERACTION_DUEL_REQUEST)
		{
			c_DuelDisplayInviteError(
				DUEL_ILLEGAL_REASON_NONE,
				DUEL_ILLEGAL_REASON_PLAYER_NOT_FOUND);
		}
		else if (myMsg->ullRequestCode == SOCIAL_INTERACTION_TRADE_REQUEST)
		{
			c_TradeRequestOtherNotFound();
		}
		break;

	default:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdYourChatMemberInfo(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_YOUR_CHAT_MEMBER_INFO * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_YOUR_CHAT_MEMBER_INFO*)data;

	DBG_ASSERT( gApp.m_characterGuid == INVALID_GUID );

	gApp.m_characterGuid = myMsg->YourGuid;
	PStrCopy(gApp.m_wszPlayerName,myMsg->wszYourName,MAX_CHARACTER_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdMemberInfo(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent when a client requests info about a specific member...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdDirectMessageReceiver(
	GAME * game,
	__notnull BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_DIRECT_MESSAGE_RECEIVER * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_DIRECT_MESSAGE_RECEIVER*)data;

	PStrCopy(gApp.m_wszLastWhisperSender, MAX_CHARACTER_NAME, myMsg->wszSenderName, MAX_CHARACTER_NAME);

	c_DisplayChat(
		CHAT_TYPE_WHISPER,
		myMsg->wszSenderName,
		myMsg->wszSenderGuildName,
		(WCHAR*)myMsg->Message.Data,
		gApp.m_wszPlayerName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdDirectMessageSender(
	GAME * game,
	__notnull BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_DIRECT_MESSAGE_SENDER * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_DIRECT_MESSAGE_SENDER*)data;

	if (myMsg->bReceived) {
		c_DisplayChat(
			CHAT_TYPE_WHISPER,
			gApp.m_wszPlayerName,
			c_PlayerGetGuildName(),
			(WCHAR*)myMsg->Message.Data,
			myMsg->wszReceiverName);
	} else {
		c_DisplayChat(
			CHAT_TYPE_SERVER,
			gApp.m_wszPlayerName,
			c_PlayerGetGuildName(),
			GlobalStringGet( GS_CHAT_WHISPEREE_NOT_FOUND ),
			myMsg->wszReceiverName);
	}
}

//----------------------------------------------------------------------------
// Displays total member count information from the chat server, then
// signals the game server for the player list.
//----------------------------------------------------------------------------
static void sChatCmdTotalMemberCount(
	GAME * game,
	__notnull BYTE * data)
{
	UNREFERENCED_PARAMETER(game);

	CHAT_RESPONSE_MSG_TOTAL_MEMBER_COUNT * msg = (CHAT_RESPONSE_MSG_TOTAL_MEMBER_COUNT*)data;

	if (msg->dwPlayerCount != (DWORD)-1)
	{
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_TOTAL_PLAYERS_CURRENT_ONLINE ),
			msg->dwPlayerCount);
	}

	MSG_CCMD_WHO whoMsg;
	c_SendMessage(CCMD_WHO, &whoMsg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPlayerIgnoreStatus(
	GAME * game,
	__notnull BYTE * data)
{
	UNREFERENCED_PARAMETER(game);

	CHAT_RESPONSE_MSG_PLAYER_IGNORE_STATUS * msg = (CHAT_RESPONSE_MSG_PLAYER_IGNORE_STATUS*)data;

	if (msg->idPlayerGuid != INVALID_CLUSTERGUID)
	{
		int index = c_PlayerFindPartyMember(msg->idPlayerGuid);
		if (index != INVALID_INDEX)
		{
			DWORD dwXfireUserId = c_PlayerGetPartyMemberClientData(index);
			c_VoiceChatIgnoreUser(dwXfireUserId, msg->bPlayerIsIgnored ? true : false);
		}
	}
}

//----------------------------------------------------------------------------
// Set our instanced channel map to this channel.
//----------------------------------------------------------------------------
static void sChatCmdInstancedChannelJoin(
	GAME * game,
	__notnull BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_INSTANCED_CHANNEL_JOIN * myMsg = NULL;
	myMsg = (CHAT_RESPONSE_MSG_INSTANCED_CHANNEL_JOIN*)data;

	AddInstancedChannel(myMsg->wszInstancingChannelName,
		myMsg->wszChannelName,
		myMsg->IdChannel);

	if(AppGetType() != APP_TYPE_SINGLE_PLAYER)
	{	//They don't really care about chat channels if there's no one to talk to.
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet(GS_CHAT_CHANNEL_ADDED),
			myMsg->wszChannelName );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdChannelMessage(
	GAME * game,
	__notnull BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_CHANNEL_MESSAGE * chatMsg = (CHAT_RESPONSE_MSG_CHANNEL_MESSAGE*)data;

	if (PStrCmp(chatMsg->wszChannelName, CHAT_CHANNEL_NAME_ANNOUNCEMENTS ) == 0 )
	{
		c_DisplayChat(
			CHAT_TYPE_ANNOUNCEMENT,
			chatMsg->wszSenderName,
			chatMsg->wszSenderGuildName,
			(WCHAR*)chatMsg->Message.Data,
			chatMsg->wszChannelName );
	}
	else if (PStrCmp(chatMsg->wszChannelName, CHAT_CHANNEL_NAME_ADMINISTRATORS ) == 0 )
	{
		c_DisplayChat(
			CHAT_TYPE_SERVER,
			chatMsg->wszSenderName,
			chatMsg->wszSenderGuildName,
			(WCHAR*)chatMsg->Message.Data,
			chatMsg->wszChannelName );
	}
	else if (chatMsg->IdChannel == gApp.m_idLevelChatChannel)
	{
		c_DisplayChat(
			CHAT_TYPE_GAME,
			chatMsg->wszSenderName,
			chatMsg->wszSenderGuildName,
			(WCHAR*)chatMsg->Message.Data,
			chatMsg->wszChannelName,
			chatMsg->SenderGuid);
	}
	else
	{
		c_DisplayChat(
			CHAT_TYPE_SHOUT,
			chatMsg->wszSenderName,
			chatMsg->wszSenderGuildName,
			(WCHAR*)chatMsg->Message.Data,
			chatMsg->wszChannelName,
			chatMsg->SenderGuid );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdAlertMessage(
	GAME * game,
	__notnull BYTE * data )
{
	sChatCmdChannelMessage(game, data);

	CHAT_RESPONSE_MSG_CHANNEL_MESSAGE * chatMsg = (CHAT_RESPONSE_MSG_CHANNEL_MESSAGE*)data;
	if (chatMsg)
	{
		UIShowQuickMessage((WCHAR*)chatMsg->Message.Data, ALERT_FADEOUT_TIME);
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdSystemMessage(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_SYSTEM_MESSAGE * sysMsg = NULL;
	sysMsg = (CHAT_RESPONSE_MSG_SYSTEM_MESSAGE*)data;

	if (sysMsg->eMessageType == CHAT_TYPE_EMOTE)
	{
		c_ShowEmoteText((WCHAR*)sysMsg->Message.Data,MAX_CHAT_MESSAGE/sizeof(WCHAR));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdChannelInfo(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent when a client requests info about a specific channel...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdChannelMember(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent when a client requests info about the members of a channel...
}

static short ssPartyMembersToIgnore = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyCreated(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	CHAT_RESPONSE_MSG_PARTY_CREATED * createdPartyMsg = NULL;
	createdPartyMsg = (CHAT_RESPONSE_MSG_PARTY_CREATED*)data;

	c_PlayerClearPartyInvites();

	if( c_PlayerGetPartyId() != createdPartyMsg->PartyInfo.IdPartyChannel &&
		c_PlayerGetPartyId() != INVALID_CHANNELID )
	{
		c_VoiceChatLeaveRoom();
		if (!c_PlayerClearParty())
		{
			UIAddChatLine(
				CHAT_TYPE_SERVER,
				ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_PARTY_LEAVE_YOU ) );
		}
	}
	
	c_PlayerSetParty(&createdPartyMsg->PartyInfo);
	if(ConsoleGetChatContext() == CHAT_TYPE_GAME)
		ConsoleSetChatContext(CHAT_TYPE_PARTY);

	if( createdPartyMsg->PartyInfo.wszLeaderName[0] )
	{
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_JOIN_YOU ),
			createdPartyMsg->PartyInfo.wszLeaderName );
	}
		
	ssPartyMembersToIgnore = createdPartyMsg->PartyInfo.wCurrentMembers - 1;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyDisbanded(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	CHAT_RESPONSE_MSG_PARTY_DISBANDED * disbandedMsg = NULL;
	disbandedMsg = (CHAT_RESPONSE_MSG_PARTY_DISBANDED*)data;

	if( c_PlayerGetPartyId() == disbandedMsg->IdPartyChannel )
	{
		c_VoiceChatLeaveRoom();
		if (!c_PlayerClearParty())
		{
			UIAddChatLine(
				CHAT_TYPE_SERVER,
				ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_PARTY_LEAVE_YOU ) );
		}

		if(ConsoleGetChatContext() == CHAT_TYPE_PARTY)
			ConsoleSetChatContext(CHAT_TYPE_GAME);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyMemberAdded(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);

	CHAT_RESPONSE_MSG_CHANNEL_MEMBER * partyMemberMsg = NULL;
	partyMemberMsg = (CHAT_RESPONSE_MSG_CHANNEL_MEMBER*)data;

	if( c_PlayerGetPartyId() == partyMemberMsg->MemberInfo.IdPartyChannel &&
		partyMemberMsg->MemberInfo.MemberGuid != gApp.m_characterGuid )
	{
		c_PlayerAddPartyMember(
			&partyMemberMsg->MemberInfo );

		if (ssPartyMembersToIgnore <= 0)
		{
			UIAddChatLineArgs(
				CHAT_TYPE_SERVER,
				ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_PARTY_JOIN ),
				partyMemberMsg->MemberInfo.wszMemberName );
		}
		else
		{
			--ssPartyMembersToIgnore;
		}

		if (AppGetType() == APP_TYPE_CLOSED_CLIENT)
		{
			//	query if we are ignoring this player so we can silence their xfire chatter...
			CHAT_REQUEST_MSG_GET_MEMBER_INFO mbrMsg;
			mbrMsg.TagMember = partyMemberMsg->MemberInfo.MemberGuid;
			c_ChatNetSendMessage(
				&mbrMsg,
				USER_REQUEST_QUERY_IS_IGNORING_MEMBER );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyMemberRemoved(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);

	CHAT_RESPONSE_MSG_MEMBER_ID * memberRemovedMsg = NULL;
	memberRemovedMsg = (CHAT_RESPONSE_MSG_MEMBER_ID*)data;

	if( c_PlayerGetPartyId() == memberRemovedMsg->IdChannel )
	{
		int index = c_PlayerFindPartyMember( memberRemovedMsg->MemberGuid );
		if( index != -1 )
		{
			const WCHAR * memberName = c_PlayerGetPartyMemberName(index);
			if( memberName )
			{
				UIAddChatLineArgs(
					CHAT_TYPE_SERVER,
					ChatGetTypeColor(CHAT_TYPE_SERVER),
					GlobalStringGet( GS_PARTY_LEAVE ),
					memberName );
			}
		}
		c_PlayerRemovePartyMember(
			memberRemovedMsg->MemberGuid );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyInfoUpdated(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_PARTY_INFO * partyInfoUpdatedMsg = NULL;
	partyInfoUpdatedMsg = (CHAT_RESPONSE_MSG_PARTY_INFO*)data;

	if(c_PlayerGetPartyId() == partyInfoUpdatedMsg->PartyInfo.IdPartyChannel )
	{
		c_PlayerUpdatePartyInfo(
			&partyInfoUpdatedMsg->PartyInfo );
	}
}

//----------------------------------------------------------------------------
//	NOTE: for now this handles both the initial data retrieval as well as
//			subsiquent data update messages.
//----------------------------------------------------------------------------
static void sChatCmdPartyClientData(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_PARTY_CLIENT_DATA * partyClientData = NULL;
	partyClientData = (CHAT_RESPONSE_MSG_PARTY_CLIENT_DATA*)data;

	if(c_PlayerGetPartyId() == INVALID_CHANNELID)
		return;

	c_PlayerSetPartyClientData(
		&partyClientData->PartyClientData );

	if (partyClientData->PartyClientData.XFireRoomId[0] != 0)
		c_VoiceChatJoinRoom(partyClientData->PartyClientData.XFireRoomId);
	if (c_VoiceChatIsChatting())
		c_VoiceChatStart();

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyLeaderChanged(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_PARTY_LEADER_CHANGED * leaderChangedMsg = NULL;
	leaderChangedMsg = (CHAT_RESPONSE_MSG_PARTY_LEADER_CHANGED*)data;

	if(c_PlayerGetPartyId() == leaderChangedMsg->IdPartyChannel )
	{
		c_PlayerUpdatePartyLeader(
			leaderChangedMsg->NewLeaderGuid,
			leaderChangedMsg->wszNewLeaderName );

		if( PStrICmp( leaderChangedMsg->wszNewLeaderName, gApp.m_wszPlayerName, MAX_CHARACTER_NAME ) == 0 )
		{
			UIAddChatLine(
				CHAT_TYPE_SERVER,
				ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_PARTY_LEADER_YOU ) );

			if (c_VoiceChatIsInRoom())
				c_VoiceChatStartHost();
			else
				c_VoiceChatCreateRoom();
		}
		else
		{
			UIAddChatLineArgs(
				CHAT_TYPE_SERVER,
				ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_PARTY_LEADER ),
				leaderChangedMsg->wszNewLeaderName );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyMemberUpdated(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	
	CHAT_RESPONSE_MSG_CHANNEL_MEMBER * updateMsg = NULL;
	updateMsg = (CHAT_RESPONSE_MSG_CHANNEL_MEMBER*)data;

	c_PlayerUpdatePartyMember(
		&updateMsg->MemberInfo );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyMessage(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_CHANNEL_MESSAGE * chatMsg = (CHAT_RESPONSE_MSG_CHANNEL_MESSAGE*)data;

	c_DisplayChat(
		CHAT_TYPE_PARTY,
		chatMsg->wszSenderName,
		chatMsg->wszSenderGuildName,
		(WCHAR*)chatMsg->Message.Data,
		NULL,
		chatMsg->SenderGuid );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyJoinAuthNeeded(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent when someone requests to join the current party, the client is the leader, and auth. joins is enabled...
	ASSERT_MSG("Party join authentication is not yet implemented on the client.");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyInviteSent(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);

	CHAT_RESPONSE_MSG_PARTY_INVITE_SENT * inviteMsg = NULL;
	inviteMsg = (CHAT_RESPONSE_MSG_PARTY_INVITE_SENT*)data;

	c_PlayerAddPartyInvite( inviteMsg->InviterGuid );

	UIShowPartyInviteDialog(inviteMsg->InviterGuid, inviteMsg->wszInviterName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyInviteDeclined(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	CHAT_RESPONSE_MSG_PARTY_INVITE_DECLINED * declinedMsg = NULL;
	declinedMsg = (CHAT_RESPONSE_MSG_PARTY_INVITE_DECLINED*)data;

	UIAddChatLineArgs(
		CHAT_TYPE_SERVER,
		ChatGetTypeColor(CHAT_TYPE_SERVER),
		GlobalStringGet( GS_PARTY_INVITE_DECLINE ),
		declinedMsg->wszDecliningMemberName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyInviteCanceled(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	CHAT_RESPONSE_MSG_PARTY_INVITE_CANCELED * canceledMsg = NULL;
	canceledMsg = (CHAT_RESPONSE_MSG_PARTY_INVITE_CANCELED*)data;

	if( c_PlayerGetPartyInviter() == canceledMsg->CancellingMemberGuid )
	{
		c_PlayerAddPartyInvite( INVALID_CLUSTERGUID );
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_INVITE_CANCEL ),
			canceledMsg->wszCancellingMemberName );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdImChannelCreated(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_IM_CHANNEL_CREATED * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_IM_CHANNEL_CREATED*)data;

	// TODO: hook this message to ui here...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdImChannelMemberAdded(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_CHANNEL_MEMBER * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_CHANNEL_MEMBER*)data;

	// TODO: hook this message to ui here...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdImChannelMemberRemoved(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_MEMBER_ID * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_MEMBER_ID*)data;

	// TODO: hook this message to ui here...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdAdvertisePartyResult(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_ADVERTISE_PARTY_RESULT * resultMsg = NULL;
	resultMsg = (CHAT_RESPONSE_MSG_ADVERTISE_PARTY_RESULT*)data;

	UIPartyCreateOnServerResult( (UI_PARTY_CREATE_RESULT)c_ChatNetGetPartyAdResult(resultMsg->ChatErrorCode) );
}

static SIMPLE_DYNAMIC_ARRAY<CHAT_RESPONSE_MSG_PARTY_INFO> sPartyAdList;
static BOOL sbPartyListInitialized = FALSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdStartAdvertisedPartyList(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	sPartyAdList.Init(MEMORY_FUNC_FILELINE(NULL, 0));
	sbPartyListInitialized = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdAdvertisedPartyInfo(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);

	CHAT_RESPONSE_MSG_PARTY_INFO * advertisedParty = NULL;
	advertisedParty = (CHAT_RESPONSE_MSG_PARTY_INFO*)data;

	ASSERT_RETURN(sbPartyListInitialized);

	sPartyAdList.Add(MEMORY_FUNC_FILELINE(advertisedParty[0]));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdEndAdvertisedPartyList(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);

	if (sPartyAdList.Count() > 0)
		UIPartyListAddPartyDesc(&sPartyAdList[0], sPartyAdList.Count());
	else
		UIPartyListAddPartyDesc(NULL, 0);

	sbPartyListInitialized = FALSE;
	sPartyAdList.Destroy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyInfo(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent for parties that the client requests info about...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdPartyMember(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(data);
	//	sent for parties who's info is requested, this includes advertised
	//		parties. for now we assume this message is for advertised parties...

	CHAT_RESPONSE_MSG_CHANNEL_MEMBER * partyMemberMsg = NULL;
	partyMemberMsg = (CHAT_RESPONSE_MSG_CHANNEL_MEMBER*)data;

	UIPartyListAddPartyMember(
		partyMemberMsg->MemberInfo.IdPartyChannel,
		partyMemberMsg->MemberInfo.wszMemberName,
		partyMemberMsg->MemberInfo.PlayerData.nCharacterExpLevel,
		partyMemberMsg->MemberInfo.PlayerData.nCharacterExpRank,
		partyMemberMsg->MemberInfo.PlayerData.nPlayerUnitClass,
		partyMemberMsg->MemberInfo.MemberGuid);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdHypertextStart(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_HYPERTEXT_START * message = (CHAT_RESPONSE_MSG_HYPERTEXT_START*)data;
	c_HypertextDataStartReceived(
		message->messageId,
		message->senderCharacterId,
		message->wszSenderCharacterName,
		message->channelId,
		message->wszChannelName,
		(ITEM_CHAT_TYPE)message->eItemChatType,
		message->hypertextDataLength );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdHypertextMessage(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_HYPERTEXT_MESSAGE * message = (CHAT_RESPONSE_MSG_HYPERTEXT_MESSAGE*)data;
	c_HypertextDataTextReceived(
		message->messageId,
		message->wszMessageText );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdHypertextData(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_HYPERTEXT_DATA * message = (CHAT_RESPONSE_MSG_HYPERTEXT_DATA*)data;
	c_HypertextDataPacketReceived(
		message->messageId,
		message->HypertextData,
		MSG_GET_BLOB_LEN(message, HypertextData) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdYourGuildInfo(
	GAME * game,
	BYTE * data )
{
	ASSERT_RETURN(data);
	CHAT_RESPONSE_MSG_YOUR_GUILD_INFO * guildInfoMsg = NULL;
	guildInfoMsg = (CHAT_RESPONSE_MSG_YOUR_GUILD_INFO*)data;

	c_PlayerSetGuildInfo(
		guildInfoMsg->dwGuildChatChannelId,
		guildInfoMsg->wszGuildName,
		(GUILD_RANK)guildInfoMsg->eGuildRank );

	if (guildInfoMsg->wszGuildName[0] == 0)
	{
		c_PlayerClearGuildMemberList();
		c_PlayerClearGuildLeaderInfo();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdYourGuildLeaderInfo(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_YOUR_GUILD_LEADER_INFO * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_YOUR_GUILD_LEADER_INFO*)data;

	c_PlayerSetGuildLeaderInfo(
		msg->idLeaderCharacterId,
		msg->wszLeaderName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdYourGuildSettingsInfo(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_YOUR_GUILD_SETTINGS_INFO * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_YOUR_GUILD_SETTINGS_INFO*)data;

	c_PlayerSetGuildSettingsInfo(
		msg->wszLeaderRankName,
		msg->wszOfficerRankName,
		msg->wszMemberRankName,
		msg->wszRecruitRankName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdYourGuildPermissionsInfo(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_YOUR_GUILD_PERMISSIONS_INFO * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_YOUR_GUILD_PERMISSIONS_INFO*)data;

	c_PlayerSetGuildPermissionsInfo(
		(GUILD_RANK)msg->eMinPromoteRank,
		(GUILD_RANK)msg->eMinDemoteRank,
		(GUILD_RANK)msg->eMinEmailRank,
		(GUILD_RANK)msg->eMinInviteRank,
		(GUILD_RANK)msg->eMinBootRank );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildListMemberStatus(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_STATUS * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_STATUS*)data;

	c_PlayerGuildMemberListAddOrUpdateMember(
		msg->idMemberCharacterId,
		msg->wszMemberName,
		msg->bIsOnline,
		(GUILD_RANK)msg->eGuildRank);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildListMemberStatusEx(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_STATUS_EX * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_STATUS_EX*)data;

	c_PlayerGuildMemberListAddOrUpdateMemberEx(
		msg->idMemberCharacterId,
		msg->wszMemberName,
		msg->bIsOnline,
		(GUILD_RANK)msg->eGuildRank,
		msg->PlayerData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildListMemberRemoved(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_REMOVED * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_GUILD_LIST_MEMBER_REMOVED*)data;

	c_PlayerGuildMemberListRemoveMember(
		msg->idMemberCharacterId,
		msg->wszMemberName );

	UIAddChatLineArgs(
		CHAT_TYPE_SERVER,
		ChatGetTypeColor(CHAT_TYPE_GUILD),
		GlobalStringGet(GS_GUILD_MEMBER_LEFT_TEXT),
		msg->wszMemberName,
		c_PlayerGetGuildName());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildMessage(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_CHANNEL_MESSAGE * chatMsg = (CHAT_RESPONSE_MSG_CHANNEL_MESSAGE*)data;

	c_DisplayChat(
		CHAT_TYPE_GUILD,
		chatMsg->wszSenderName,
		chatMsg->wszSenderGuildName,
		(WCHAR*)chatMsg->Message.Data,
		NULL,
		chatMsg->SenderGuid );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildInviteReceived(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_GUILD_INVITE_RECEIVED * chatMsg = NULL;
	chatMsg = (CHAT_RESPONSE_MSG_GUILD_INVITE_RECEIVED*)data;

	// HACK: We don't actually know their guid here. But we don't use it either, so just setting it to 0 (anything other than INVALID_GUID) for now.
	UIShowGuildInviteDialog((PGUID)0, chatMsg->wszInviterName, chatMsg->wszGuildName);

	//	TEMP: temp command line text
	UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_INVITE_RECEIVED), chatMsg->wszInviterName, chatMsg->wszGuildName);

	//	TEMP: super temp accept/decline instructions
	UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_INVITE_RECEIVED_TYPE_TO_ACCEPT));
	UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_INVITE_RECEIVED_TYPE_TO_DECLINE));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildInviteDeclined(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_GUILD_INVITE_INFO * chatMsg = NULL;
	chatMsg = (CHAT_RESPONSE_MSG_GUILD_INVITE_INFO*)data;

	WCHAR wszMessageText[2048];
	PStrPrintf(wszMessageText, 2048, GlobalStringGet(GS_GUILD_INVITE_DECLINED_TEXT), chatMsg->wszInvitedPlayerName, chatMsg->wszGuildName);
	UIShowGenericDialog(GlobalStringGet(GS_GUILD_INVITE_DECLINED_HEADER), wszMessageText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildInviteAccepted(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);
	CHAT_RESPONSE_MSG_GUILD_INVITE_INFO * chatMsg = NULL;
	chatMsg = (CHAT_RESPONSE_MSG_GUILD_INVITE_INFO*)data;

	WCHAR wszMessageText[2048];
	PStrPrintf(wszMessageText, 2048, GlobalStringGet(GS_GUILD_INVITE_ACCEPTED_TEXT), chatMsg->wszInvitedPlayerName, chatMsg->wszGuildName);
	UIShowGenericDialog(GlobalStringGet(GS_GUILD_INVITE_ACCEPTED_HEADER), wszMessageText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildRemovalNotice(
	GAME * game,
	BYTE * data )
{
	c_PlayerClearGuildInfo();
	c_PlayerClearGuildLeaderInfo();
	c_PlayerClearGuildMemberList();

	CHAT_RESPONSE_MSG_GUILD_REMOVAL_NOTICE * chatMsg = NULL;
	chatMsg = (CHAT_RESPONSE_MSG_GUILD_REMOVAL_NOTICE*)data;
	WCHAR wszMessageText[2048];
	PStrPrintf(wszMessageText, 2048, GlobalStringGet(GS_GUILD_REMOVED_FROM_GUILD_TEXT), chatMsg->wszGuildName);
	UIShowGenericDialog(GlobalStringGet(GS_GUILD_REMOVED_FROM_GUILD_HEADER), wszMessageText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdGuildLeadershipGained(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_GUILD_LEADERSHIP_GAINED * noticeMsg = NULL;
	noticeMsg = (CHAT_RESPONSE_MSG_GUILD_LEADERSHIP_GAINED*)data;

	UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_LEADERSHIP_GAINED), noticeMsg->wszGuildName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdCsrSessionStart(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_CSRSESSION_START * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_CSRSESSION_START*)data;

	UICSRPanelOpen();
	ConsoleSetCSRPlayerName(msg->wszCsrRepName);
	UIAddChatLineArgs(CHAT_TYPE_CSR, ChatGetTypeColor(CHAT_TYPE_CSR), GlobalStringGet(GS_CSR_CHAT_STARTED), msg->wszCsrRepName);

	PlayerCSRChatEnter();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdCsrSessionMessage(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_CSRSESSION_MESSAGE * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_CSRSESSION_MESSAGE*)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(msg->wszCsrRepName);
	ASSERT_RETURN(msg->wszChatMessage);

	UIAddChatLineArgs(
		CHAT_TYPE_CSR,
		ChatGetTypeColor(CHAT_TYPE_CSR),
		L"%s %s: %s",
		GlobalStringGet(GS_CSR_CHAT_PREFIX),
		msg->wszCsrRepName,
		msg->wszChatMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatCmdCsrSessionEnd(
	GAME * game,
	BYTE * data )
{
	CHAT_RESPONSE_MSG_CSRSESSION_END * msg = NULL;
	msg = (CHAT_RESPONSE_MSG_CSRSESSION_END*)data;

	UIAddChatLineArgs(CHAT_TYPE_CSR, ChatGetTypeColor(CHAT_TYPE_CSR), GlobalStringGet(GS_CSR_CHAT_ENDED), msg->wszCsrRepName);
	UICSRPanelClose();
	ConsoleSetCSRPlayerName(L"");

	PlayerCSRChatExit();
}


//----------------------------------------------------------------------------
// HANDLER ARRAY
//----------------------------------------------------------------------------

typedef void FP_PROCESS_MESSAGE(GAME * game, BYTE * data);

struct PROCESS_MESSAGE_STRUCT
{
	NET_CMD					cmd;
	FP_PROCESS_MESSAGE *	fp;
	BOOL					bNeedGame;
	const char *			szFuncName;	
};

#if ISVERSION(DEVELOPMENT)
#define SRV_MESSAGE_PROC(c, f, g)		{ c, f, g, #f },
#else
#define SRV_MESSAGE_PROC(c, f, g)		{ c, f, g, "" },
#endif

PROCESS_MESSAGE_STRUCT gfpChatServerMessageHandlers[] = 
{
	//				  command										handler							needs game
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_RESULT,					sChatCmdResult,					FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_YOUR_CHAT_MEMBER_INFO,		sChatCmdYourChatMemberInfo,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_ALERT_MESSAGE,				sChatCmdAlertMessage,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_SYSTEM_MESSAGE,			sChatCmdSystemMessage,		FALSE )
	//	members messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_MEMBER_INFO,				sChatCmdMemberInfo,				FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_DIRECT_MESSAGE_RECEIVER,	sChatCmdDirectMessageReceiver,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_DIRECT_MESSAGE_SENDER,		sChatCmdDirectMessageSender,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_TOTAL_MEMBER_COUNT,		sChatCmdTotalMemberCount,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PLAYER_IGNORE_STATUS,		sChatCmdPlayerIgnoreStatus,		FALSE )
	//  instanced channel messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_INSTANCED_CHANNEL_JOIN,	sChatCmdInstancedChannelJoin,	FALSE )
	//	chat channel messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CHANNEL_MESSAGE,			sChatCmdChannelMessage,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CHANNEL_INFO,				sChatCmdChannelInfo,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CHANNEL_MEMBER,			sChatCmdChannelMember,			FALSE )
	//	party messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_CREATED,				sChatCmdPartyCreated,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_DISBANDED,			sChatCmdPartyDisbanded,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_MESSAGE,				sChatCmdPartyMessage,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_INFO,				sChatCmdPartyInfo,				FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_INFO_UPDATED,		sChatCmdPartyInfoUpdated,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_CLIENT_DATA,			sChatCmdPartyClientData,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_CLIENT_DATA_UPDATED,	sChatCmdPartyClientData,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_LEADER_CHANGED,		sChatCmdPartyLeaderChanged,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_MEMBER,				sChatCmdPartyMember,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_MEMBER_UPDATED,		sChatCmdPartyMemberUpdated,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_MEMBER_ADDED,		sChatCmdPartyMemberAdded,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_MEMBER_REMOVED,		sChatCmdPartyMemberRemoved,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_JOIN_AUTH_NEEDED,	sChatCmdPartyJoinAuthNeeded,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_INVITE_SENT,			sChatCmdPartyInviteSent,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_INVITE_DECLINED,		sChatCmdPartyInviteDeclined,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PARTY_INVITE_CANCELED,		sChatCmdPartyInviteCanceled,	FALSE )
	//	buddy list messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_BUDDY_INFO,				sChatCmdBuddyInfo,				FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_BUDDY_INFO_UPDATE,			sChatCmdBuddyInfoUpdate,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_BUDDY_REMOVED,				sChatCmdBuddyRemoved,			FALSE )
	//	IM channel messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_IM_CHANNEL_CREATED,		sChatCmdImChannelCreated,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_IM_MEMBER_ADDED,			sChatCmdImChannelMemberAdded,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_IM_MEMBER_REMOVED,			sChatCmdImChannelMemberRemoved,	FALSE )
	//	party advertisement messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_ADVERTISE_PARTY_RESULT,	sChatCmdAdvertisePartyResult,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_ADVERTISED_PARTY_LIST_START, sChatCmdStartAdvertisedPartyList, FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_ADVERTISED_PARTY_INFO,		sChatCmdAdvertisedPartyInfo,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_ADVERTISED_PARTY_LIST_END,	sChatCmdEndAdvertisedPartyList,	FALSE )
	// ignore list messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PLAYER_IGNORED,			sChatCmdPlayerIgnored,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_PLAYER_UNIGNORED,			sChatCmdPlayerUnIgnored,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_IGNORE_ERROR,				sChatCmdIgnoreError,			FALSE )
	//	hypertext messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_HYPERTEXT_START,			sChatCmdHypertextStart,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_HYPERTEXT_MESSAGE,			sChatCmdHypertextMessage,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_HYPERTEXT_DATA,			sChatCmdHypertextData,			FALSE )
	//	guild messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_YOUR_GUILD_INFO,			sChatCmdYourGuildInfo,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_YOUR_GUILD_LEADER_INFO,	sChatCmdYourGuildLeaderInfo,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_YOUR_GUILD_SETTINGS_INFO,	sChatCmdYourGuildSettingsInfo,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_YOUR_GUILD_PERMISSIONS_INFO,sChatCmdYourGuildPermissionsInfo,FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_LIST_MEMBER_STATUS,	sChatCmdGuildListMemberStatus,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_LIST_MEMBER_STATUS_EX,sChatCmdGuildListMemberStatusEx,FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_LIST_MEMBER_REMOVED,	sChatCmdGuildListMemberRemoved,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_MESSAGE,				sChatCmdGuildMessage,			FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_INVITE_RECEIVED,		sChatCmdGuildInviteReceived,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_INVITE_DECLINED,		sChatCmdGuildInviteDeclined,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_INVITE_ACCEPTED,		sChatCmdGuildInviteAccepted,	FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_REMOVAL_NOTICE,		sChatCmdGuildRemovalNotice,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_GUILD_LEADERSHIP_GAINED,	sChatCmdGuildLeadershipGained,	FALSE )
	//	csr chat session messages
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CSRSESSION_START,			sChatCmdCsrSessionStart,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CSRSESSION_MESSAGE,		sChatCmdCsrSessionMessage,		FALSE )
	SRV_MESSAGE_PROC( CHAT_USER_RESPONSE_CSRSESSION_END,			sChatCmdCsrSessionEnd,			FALSE )
};

//----------------------------------------------------------------------------
// HANDLER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD c_ChatNetGetPartyAdResult(
	DWORD errorCode )
{
	switch (errorCode)
	{
	case CHAT_ERROR_NONE:						return PCR_SUCCESS;
	case CHAT_ERROR_INVALID_PARTY_NAME:			return PCR_INVALID_PARTY_NAME;
	case CHAT_ERROR_INVALID_PARTY_LEVEL_RANGE:	return PCR_INVALID_PARTY_LEVEL_RANGE;
	case CHAT_ERROR_MEMBER_ALREADY_IN_A_PARTY:	return PCR_ALREADY_IN_A_PARTY;
	case CHAT_ERROR_PARTY_NAME_TAKEN:			return PCR_PARTY_NAME_TAKEN;
	default:									return PCR_ERROR;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChatNetProcessChatMessage(
	MSG_STRUCT * msg )
{
	ASSERT_RETURN(msg);
	NET_CMD command = msg->hdr.cmd;

	ASSERT_RETURN(command >= 0 && command < CHAT_USER_RESPONSE_COUNT);
	ASSERT_RETURN(gfpChatServerMessageHandlers[command].fp);

	NetTraceRecvMessage(gApp.m_ChatServerCommandTable, msg, "CLTRCV: ");

	GAME * game = AppGetCltGame();
	ASSERT_RETURN(game || !gfpChatServerMessageHandlers[command].bNeedGame);

	gfpChatServerMessageHandlers[command].fp(game, (BYTE*)msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetProcessChatMessages(
	void )
{
	PERF(CLT_MSG);

	// remove all the messages and process them
	MSG_BUF * head, * tail;
	MailSlotGetMessages(AppGetChatMailSlot(), head, tail);

	MSG_BUF * msg = NULL;
	MSG_BUF * next = head;
	while( (msg = next) != NULL )
	{
		next = msg->next;

		BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];
		if (NetTranslateMessageForRecv(gApp.m_ChatServerCommandTable, msg->msg, msg->size, (MSG_STRUCT *)message))
		{
			sChatNetProcessChatMessage((MSG_STRUCT *)message);
		}

		GAME * game = AppGetCltGame();
		if (game && GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN)
		{
			// put rest of messages back on front of mailbox & set up head, tail for recycling (tail is current message)
			MSG_BUF * unprocessed_head = msg->next;
			MSG_BUF * unprocessed_tail = tail;
			tail = msg;
			msg->next = NULL;
			if (unprocessed_head)
			{
				MailSlotPushbackMessages(AppGetChatMailSlot(), unprocessed_head, unprocessed_tail);
			}
			break;
		}
	}
	MailSlotRecycleMessages(AppGetChatMailSlot(), head, tail);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ChatNetValidateCommandTable(
	void )
{
	HALTX(arrsize(gfpChatServerMessageHandlers) == CHAT_USER_RESPONSE_COUNT, "Chat server command table failed validation. Sizes do not match.");
	for (unsigned int ii = 0; ii < arrsize(gfpChatServerMessageHandlers); ++ii)
	{
		HALTX(gfpChatServerMessageHandlers[ii].cmd == ii, "Chat server command table failed validation. A message handler is out of order somewhere.");
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetGetAdvertisedParties(
	void )
{
	//	TODO: some day this will use a WCHAR string search pattern...
	CHAT_REQUEST_MSG_GET_ADVERTISED_PARTIES getPartiesMsg;
	c_ChatNetSendMessage(
		&getPartiesMsg,
		USER_REQUEST_GET_ADVERTISED_PARTIES );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void
	c_ChatNetIgnoreMember(
		PGUID pGuid)
{
	CHAT_REQUEST_MSG_MEMBER_IGNORE_MEMBER ignoreMsg;
	ignoreMsg.TagMemberToIgnore = pGuid;

	if (pGuid != INVALID_CLUSTERGUID)
	{
		int index = c_PlayerFindPartyMember(pGuid);
		if (index != INVALID_INDEX)
		{
			DWORD dwXfireUserId = c_PlayerGetPartyMemberClientData(index);
			c_VoiceChatIgnoreUser(dwXfireUserId, true);
		}
	}

	c_ChatNetSendMessage( &ignoreMsg, USER_REQUEST_MEMBER_IGNORE_MEMBER );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void
	c_ChatNetIgnoreMemberByName(
		const WCHAR * pName)
{
	CHAT_REQUEST_MSG_MEMBER_IGNORE_MEMBER ignoreMsg;
	ignoreMsg.TagMemberToIgnore = pName;

	if (pName != NULL)
	{
		int count = c_PlayerGetPartyMemberCount();
		for (int i = 0; i < count; i++)
		{
			if (PStrICmp(c_PlayerGetPartyMemberName(i), pName) == 0)
			{
				DWORD dwXfireUserId = c_PlayerGetPartyMemberClientData(i);
				c_VoiceChatIgnoreUser(dwXfireUserId, true);
				break;
			}

		}
	}

	c_ChatNetSendMessage( &ignoreMsg, USER_REQUEST_MEMBER_IGNORE_MEMBER );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetUnignoreMember(
		const WCHAR * strPlayer )
{
	CHAT_REQUEST_MSG_MEMBER_UNIGNORE_MEMBER unignoreMsg;
	PStrCopy(unignoreMsg.wszMemberName, strPlayer, MAX_CHARACTER_NAME);

	if (strPlayer != NULL)
	{
		int count = c_PlayerGetPartyMemberCount();
		for (int i = 0; i < count; i++)
		{
			if (PStrICmp(c_PlayerGetPartyMemberName(i), strPlayer) == 0)
			{
				DWORD dwXfireUserId = c_PlayerGetPartyMemberClientData(i);
				c_VoiceChatIgnoreUser(dwXfireUserId, false);
				break;
			}

		}
	}

	c_ChatNetSendMessage( &unignoreMsg, USER_REQUEST_MEMBER_UNIGNORE_MEMBER );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetRequestSocialInteraction(
	UNIT * pTarget,
	SOCIAL_INTERACTION_TYPE interactionType )
{
	ASSERT_RETURN(pTarget);

	CHAT_REQUEST_MSG_REQUEST_SOCIAL_INTERACTION requestMsg;
	requestMsg.TagTargetMember = UnitGetGuid(pTarget);
	requestMsg.dwInteractionType = interactionType;

	c_ChatNetSendMessage(&requestMsg, USER_REQUEST_REQUEST_SOCIAL_INTERACTION);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetRefreshBuddyListInfo(
	void )
{
	CHAT_REQUEST_MSG_REFRESH_ONLINE_BUDDY_INFO msg;
	c_ChatNetSendMessage(&msg, USER_REQUEST_REFRESH_ONLINE_BUDDY_INFO);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ChatNetRefreshGuildListInfo(
	void )
{
	CHAT_REQUEST_MSG_REFRESH_ONLINE_GUILD_MBRS_INFO msg;
	c_ChatNetSendMessage(&msg, USER_REQUEST_REFRESH_ONLINE_GUILD_MBRS_INFO);
}


#endif	//	!ISVERSION(SERVER_VERSION)
