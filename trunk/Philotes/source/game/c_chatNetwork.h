//----------------------------------------------------------------------------
// c_chatNetwork.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------
#ifndef _C_CHATNETWORK_H_
#define _C_CHATNETWORK_H_


#ifndef _CHAT_H_
#include "chat.h"
#endif

struct CHAT_MESSAGE;

struct NET_COMMAND_TABLE *
	c_ChatNetGetClientRequestCmdTable(
		void );

struct NET_COMMAND_TABLE *
	c_ChatNetGetServerResponseCmdTable(
		void );

BOOL 
	c_ChatMessageRerouteToServer(
		MSG_STRUCT *,
		CHAT_MESSAGE *,
		NET_CMD );

void
	c_ChatNetSendMessage(
		MSG_STRUCT *,
		NET_CMD );

void
	c_ChatNetSendKeepAlive(
		void );

void
	c_ChatNetProcessChatMessages(
		void );

BOOL
	c_ChatNetValidateCommandTable(
		void );

void
	c_ChatNetGetAdvertisedParties(
		void );

void
	c_ChatNetIgnoreMember(
		PGUID );

void
	c_ChatNetIgnoreMemberByName(
		const WCHAR * );

void
	c_ChatNetUnignoreMember(
		const WCHAR * );

DWORD
	c_ChatNetGetPartyAdResult(
		DWORD chatErrorCode );

void
	c_ChatNetRequestSocialInteraction(
		UNIT * pTarget,
		SOCIAL_INTERACTION_TYPE interactionType );

void
	c_ChatNetRefreshBuddyListInfo(
		void );

void
	c_ChatNetRefreshGuildListInfo(
		void );

#endif	//	_C_CHATNETWORK_H_
