//----------------------------------------------------------------------------
// c_playerEmail.h
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_PLAYER_EMAIL_H_
#define _C_PLAYER_EMAIL_H_

#ifndef _S_MESSAGE_H_
#include "s_message.h"
#endif

#ifndef _CHATSERVER_H_
#include "..\source\ServerSuite\Chat\ChatServer.h"
#endif

#ifndef _S_PLAYER_EMAIL_H_
#include "s_playerEmail.h"
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PLAYER_EMAIL_MESSAGE
{
	ULONGLONG			EmailMessageId;
	PGUID				SenderCharacterId;
	WCHAR				wszSenderCharacterName[MAX_CHARACTER_NAME];
	EMAIL_STATUS		eMessageStatus;
	PLAYER_EMAIL_TYPES	eMessageType;
	time_t				TimeSentUTC;
	time_t				TimeOfManditoryDeletionUTC;
	BOOL				bIsMarkedRead;
	WCHAR				wszEmailSubject[MAX_EMAIL_SUBJECT];
	WCHAR				wszEmailBody[MAX_EMAIL_BODY];
	PGUID				AttachedItemId;
	DWORD				AttachedMoney;

	int CalculateDaysUntilDeletion() const;
	int CalculateSecondsUntilDeletion() const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void
	c_PlayerEmailInit(
		void );

void
	c_PlayerEmailHandleResult(
		DWORD actionType,
		DWORD actionResult,
		EMAIL_SPEC_SOURCE actionContext );

BOOL
	c_PlayerEmailSendGuildMessage(
		const WCHAR * szSubject,
		const WCHAR * szBody );

BOOL
	c_PlayerEmailSendCharacterMessage(
		const WCHAR * szTargetCharacterName,
		const WCHAR * szSubject,
		const WCHAR * szBody,
		PGUID idAttachedItem,
		DWORD dwAttachedMoney );

DWORD
	c_PlayerEmailMessageCount(
		void );

DWORD
	c_PlayerEmailUnreadMessageCount(
		void );

DWORD
	c_PlayerEmailGetMessageIds(
		ULONGLONG * dest,
		DWORD count,
		BOOL ignoreRead );

BOOL
	c_PlayerEmailGetMessage(
		ULONGLONG messageId,
		PLAYER_EMAIL_MESSAGE & message );

void
	c_PlayerEmailMarkMessageRead(
		ULONGLONG messageId );

void
	c_PlayerEmailRemoveAttachedMoney(
		ULONGLONG messageId );

void
	c_PlayerEmailClearItemAttachment(
		ULONGLONG messageId );

void
	c_PlayerEmailDeleteMessage(
		ULONGLONG messageId );

void
	c_PlayerEmailRestoreOutboxAttachments(
		void );

void
	c_PlayerEmailHandleMetadata(
		MSG_SCMD_EMAIL_METADATA * msg );

void
	c_PlayerEmailHandleData(
		MSG_SCMD_EMAIL_DATA * msg );

void
	c_PlayerEmailHandleDataUpdate(
		MSG_SCMD_EMAIL_UPDATE * msg );


#endif	//	_C_PLAYER_EMAIL_H_
