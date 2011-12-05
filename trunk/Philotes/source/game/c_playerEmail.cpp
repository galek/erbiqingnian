//----------------------------------------------------------------------------
// c_playerEmail.cpp
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_playerEmail.h"
#include "c_message.h"
#include "s_playerEmail.h"
#include "uix_email.h"
#include "c_chatNetwork.h"
#include "hash.h"
#include "fasttraversehash.h"
#include "array.h"
#include "globalindex.h"
#include "uix_auction.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct EMAIL_WRAPPER
{
	ULONGLONG id;
	EMAIL_WRAPPER * next;
	EMAIL_WRAPPER * traverse[2];

	PLAYER_EMAIL_MESSAGE message;
};

typedef CFastTraverseHash<EMAIL_WRAPPER, ULONGLONG, 1024> EMAIL_LOOKUP;


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static EMAIL_LOOKUP s_Lookup;
static BOOL s_LookupIsInitialized = FALSE;
static DWORD s_MessageCount = 0;
static DWORD s_UnreadMessageCount = 0;


//----------------------------------------------------------------------------
// PRIVATE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
static EMAIL_WRAPPER * sFindOrAddEntry(
	ULONGLONG messageId,
	BOOL bIsMarkedRead )
{
	EMAIL_WRAPPER * entry = s_Lookup.Get(messageId);
	if (!entry)
	{
		entry = s_Lookup.Add(NULL, messageId);
		ASSERT_RETNULL(entry);

		++s_MessageCount;
		if (!bIsMarkedRead)
			++s_UnreadMessageCount;
	}
	return entry;
}

//----------------------------------------------------------------------------
static EMAIL_WRAPPER * sFindEntry(
	ULONGLONG messageId )
{
	return s_Lookup.Get(messageId);
}

//----------------------------------------------------------------------------
int PLAYER_EMAIL_MESSAGE::CalculateDaysUntilDeletion() const
{
	static const int SEC_PER_DAY = (24 * 60 * 60);
	return (this->CalculateSecondsUntilDeletion() + (SEC_PER_DAY/2)) / SEC_PER_DAY;
}

//----------------------------------------------------------------------------
int PLAYER_EMAIL_MESSAGE::CalculateSecondsUntilDeletion() const
{
	time_t now = time(NULL);

	if (now >= this->TimeOfManditoryDeletionUTC)
		return 0;

	return (int)(this->TimeOfManditoryDeletionUTC - now);
}


//----------------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void c_PlayerEmailInit(
	void )
{
	if (s_LookupIsInitialized)
		s_Lookup.Destroy(NULL);
	s_Lookup.Init();
	s_MessageCount = 0;
	s_UnreadMessageCount = 0;
}

//----------------------------------------------------------------------------
void c_PlayerEmailHandleResult(
	DWORD actionType,
	DWORD actionResult,
	EMAIL_SPEC_SOURCE actionContext)
{
	if (actionContext == ESS_PLAYER && actionType == CCMD_EMAIL_SEND_COMMIT)
	{
		UIEmailOnSendResult((PLAYER_EMAIL_RESULT)actionResult);
	}
	else if (actionContext == ESS_PLAYER_ITEMSALE)
	{
		MSG_SCMD_AH_ERROR errorMsg;
		errorMsg.ErrorCode = AH_ERROR_SELL_ITEM_INTERNAL_ERROR;
		errorMsg.ItemGUID = INVALID_GUID;
		UIAuctionError(&errorMsg);
	}
	else if (actionContext == ESS_PLAYER_ITEMBUY)
	{
		MSG_SCMD_AH_ERROR errorMsg;
		errorMsg.ErrorCode = AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
		errorMsg.ItemGUID = INVALID_GUID;
		UIAuctionError(&errorMsg);
	}
}

//----------------------------------------------------------------------------
BOOL c_PlayerEmailSendGuildMessage(
	const WCHAR * szSubject,
	const WCHAR * szBody )
{
	ASSERT_RETFALSE(szSubject);
	ASSERT_RETFALSE(szBody);

	{
		MSG_CCMD_EMAIL_SEND_START startMsg;
		PStrCopy(startMsg.wszEmailSubject, szSubject, MAX_EMAIL_SUBJECT);
		PStrCopy(startMsg.wszEmailBody, szBody, MAX_EMAIL_BODY);
		c_SendMessage(CCMD_EMAIL_SEND_START, &startMsg);
	}

	{
		MSG_CCMD_EMAIL_ADD_GUILD_MEMBERS_AS_RECIPIENTS guildMsg;
		c_SendMessage(CCMD_EMAIL_ADD_GUILD_MEMBERS_AS_RECIPIENTS, &guildMsg);
	}

	{
		MSG_CCMD_EMAIL_SEND_COMMIT commitMsg;
		c_SendMessage(CCMD_EMAIL_SEND_COMMIT, &commitMsg);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
BOOL c_PlayerEmailSendCharacterMessage(
	const WCHAR * szTargetCharacterName,
	const WCHAR * szSubject,
	const WCHAR * szBody,
	PGUID idAttachedItem,
	DWORD dwAttachedMoney )
{
	ASSERT_RETFALSE(szTargetCharacterName);
	ASSERT_RETFALSE(szSubject);
	ASSERT_RETFALSE(szBody);

	{
		MSG_CCMD_EMAIL_SEND_START startMsg;
		PStrCopy(startMsg.wszEmailSubject, szSubject, MAX_EMAIL_SUBJECT);
		PStrCopy(startMsg.wszEmailBody, szBody, MAX_EMAIL_BODY);
		c_SendMessage(CCMD_EMAIL_SEND_START, &startMsg);
	}

	{
		MSG_CCMD_EMAIL_ADD_RECIPIENT_BY_CHARACTER_NAME targetMsg;
		PStrCopy(targetMsg.wszTargetCharacterName, szTargetCharacterName, MAX_CHARACTER_NAME);
		c_SendMessage(CCMD_EMAIL_ADD_RECIPIENT_BY_CHARACTER_NAME, &targetMsg);
	}

	if (idAttachedItem != INVALID_GUID || dwAttachedMoney != 0)
	{
		MSG_CCMD_EMAIL_SET_ATTACHMENTS attachmentsMsg;
		attachmentsMsg.idAttachedItemId = idAttachedItem;
		attachmentsMsg.dwAttachedMoney = dwAttachedMoney;
		c_SendMessage(CCMD_EMAIL_SET_ATTACHMENTS, &attachmentsMsg);
	}

	{
		MSG_CCMD_EMAIL_SEND_COMMIT commitMsg;
		c_SendMessage(CCMD_EMAIL_SEND_COMMIT, &commitMsg);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
DWORD c_PlayerEmailMessageCount(
	void )
{
	return s_MessageCount;
}

//----------------------------------------------------------------------------
DWORD c_PlayerEmailUnreadMessageCount(
	void )
{
	return s_UnreadMessageCount;
}

//----------------------------------------------------------------------------
DWORD c_PlayerEmailGetMessageIds(
	ULONGLONG * dest,
	DWORD count,
	BOOL ignoreRead )
{
	ASSERT_RETZERO(dest);
	DWORD ii = 0;
	EMAIL_WRAPPER * itr = s_Lookup.GetFirst();
	while (itr && ii < count)
	{
		if (!ignoreRead || !itr->message.bIsMarkedRead)
		{
			dest[ii] = itr->id;
			++ii;
		}
		itr = s_Lookup.GetNext(itr);
	}
	return ii;
}

//----------------------------------------------------------------------------
struct SPECIAL_SENDER
{
	PLAYER_EMAIL_TYPES eMailType;
	GLOBAL_STRING eGlobalString;
};
static const SPECIAL_SENDER sgtSpecialSenderLookup[] = 
{
	{ PLAYER_EMAIL_TYPE_CSR,					GS_SENDER_CSR },
	{ PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE,		GS_SENDER_CSR },
	{ PLAYER_EMAIL_TYPE_SYSTEM,					GS_SENDER_SYSTEM },
	{ PLAYER_EMAIL_TYPE_AUCTION,				GS_SENDER_AUCTION },
	{ PLAYER_EMAIL_TYPE_CONSIGNMENT,			GS_SENDER_CONSIGNMENT },
};

//----------------------------------------------------------------------------
static void sSetWellKnownSenderName(
	PLAYER_EMAIL_MESSAGE &tMessage)
{

	const int nNumEntries = arrsize( sgtSpecialSenderLookup );
	for (int i = 0; i < nNumEntries; ++i)
	{
		const SPECIAL_SENDER *pSpecialSender = &sgtSpecialSenderLookup[ i ];
		if (pSpecialSender->eMailType == tMessage.eMessageType)
		{
			PStrCopy( 
				tMessage.wszSenderCharacterName, 
				GlobalStringGet( pSpecialSender->eGlobalString ), 
				MAX_CHARACTER_NAME);		
			break;
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetWellKnownSubjectAndBody( 
	PLAYER_EMAIL_MESSAGE &message)
{

	// csr item restorations (for now we assume that an email from CSR that
	// has an item attachment is an item restore ... we can change this as
	// we add more item sends to the CSR portal)
	if (message.eMessageType == PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE)
	{
	
		// set subject
		PStrCopy( 
			message.wszEmailSubject,
			GlobalStringGet( GS_EMAIL_SUBJECT_CSR_ITEM_ATTACHED ),
			MAX_EMAIL_SUBJECT);
			
		// set body
		PStrCopy( 
			message.wszEmailBody,
			GlobalStringGet( GS_EMAIL_BODY_CSR_ITEM_ATTACHED ),
			MAX_EMAIL_SUBJECT);
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetWellKnownEmailContents(
	PLAYER_EMAIL_MESSAGE & message)
{

	// set special sender name
	sSetWellKnownSenderName( message );

	// set special subject and body
	sSetWellKnownSubjectAndBody( message );
	
}

//----------------------------------------------------------------------------
BOOL c_PlayerEmailGetMessage(
	ULONGLONG messageId,
	PLAYER_EMAIL_MESSAGE & message )
{
	EMAIL_WRAPPER * entry = s_Lookup.Get(messageId);

	if (!entry)
		return FALSE;

	message = entry->message;
	
	// set special names/subject/body for client side display
	sSetWellKnownEmailContents( message );
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
void c_PlayerEmailRestoreOutboxAttachments(
	void )
{
	MSG_CCMD_EMAIL_RESTORE_OUTBOX_ITEM_LOC msg;
	c_SendMessage(CCMD_EMAIL_RESTORE_OUTBOX_ITEM_LOC, &msg);
}

//----------------------------------------------------------------------------
void c_PlayerEmailMarkMessageRead(
	ULONGLONG messageId )
{
	EMAIL_WRAPPER * entry = sFindEntry(messageId);
	if (!entry)
		return;

	entry->message.bIsMarkedRead = TRUE;

	MSG_CCMD_EMAIL_MARK_MESSAGE_READ msg;
	msg.idEmailMessageId = messageId;
	c_SendMessage(CCMD_EMAIL_MARK_MESSAGE_READ, &msg);

	UIEmailOnUpdate();
}

//----------------------------------------------------------------------------
void c_PlayerEmailRemoveAttachedMoney(
	ULONGLONG messageId )
{
	EMAIL_WRAPPER * entry = sFindEntry(messageId);
	if (!entry)
		return;

	MSG_CCMD_EMAIL_REMOVE_ATTACHED_MONEY msg;
	msg.idEmailMessageId = messageId;
	c_SendMessage(CCMD_EMAIL_REMOVE_ATTACHED_MONEY, &msg);
}

//----------------------------------------------------------------------------
void c_PlayerEmailClearItemAttachment(
	ULONGLONG messageId )
{
	EMAIL_WRAPPER * entry = sFindEntry(messageId);
	if (!entry)
		return;

	MSG_CCMD_EMAIL_CLEAR_ATTACHED_ITEM msg;
	msg.idEmailMessageId = messageId;
	c_SendMessage(CCMD_EMAIL_CLEAR_ATTACHED_ITEM, &msg);
}

//----------------------------------------------------------------------------
void c_PlayerEmailDeleteMessage(
	ULONGLONG messageId )
{
	EMAIL_WRAPPER * entry = sFindEntry(messageId);
	if (!entry)
		return;
	s_Lookup.Remove(messageId);

	MSG_CCMD_EMAIL_DELETE_MESSAGE msg;
	msg.idEmailMessageId = messageId;
	c_SendMessage(CCMD_EMAIL_DELETE_MESSAGE, &msg);

	UIEmailOnUpdate();
}

//----------------------------------------------------------------------------
void c_PlayerEmailHandleMetadata(
	MSG_SCMD_EMAIL_METADATA * msg )
{
	ASSERT_RETURN(msg);

	EMAIL_WRAPPER * newEntry = sFindOrAddEntry(msg->idEmailMessageId, msg->bIsMarkedRead);
	ASSERT_RETURN(newEntry);

	newEntry->message.EmailMessageId = msg->idEmailMessageId;
	newEntry->message.SenderCharacterId = msg->idSenderCharacterId;
	PStrCopy(newEntry->message.wszSenderCharacterName, msg->wszSenderCharacterName, MAX_CHARACTER_NAME);
	newEntry->message.eMessageStatus = (EMAIL_STATUS)msg->eMessageStatus;
	newEntry->message.eMessageType = (PLAYER_EMAIL_TYPES)msg->eMessageType;
	newEntry->message.TimeSentUTC = msg->timeSentUTC;
	newEntry->message.TimeOfManditoryDeletionUTC = msg->timeOfManditoryDeletionUTC;
	newEntry->message.bIsMarkedRead = msg->bIsMarkedRead;
	newEntry->message.AttachedItemId = msg->idAttachedItemId;
	newEntry->message.AttachedMoney = msg->dwAttachedMoney;
}

//----------------------------------------------------------------------------
void c_PlayerEmailHandleData(
	MSG_SCMD_EMAIL_DATA * msg )
{
	ASSERT_RETURN(msg);

	EMAIL_WRAPPER * entry = sFindEntry(msg->idEmailMessageId);
	ASSERT_RETURN(entry);

	PStrCopy(entry->message.wszEmailSubject, msg->wszEmailSubject, MAX_EMAIL_SUBJECT);
	PStrCopy(entry->message.wszEmailBody, msg->wszEmailBody, MAX_EMAIL_BODY);

	if (!entry->message.bIsMarkedRead)
	{
		UIEmailNewMailNotification();
	}

	UIEmailOnUpdate();
}

//----------------------------------------------------------------------------
void c_PlayerEmailHandleDataUpdate(
	MSG_SCMD_EMAIL_UPDATE * msg )
{
	ASSERT_RETURN(msg);

	EMAIL_WRAPPER * entry = sFindEntry(msg->idEmailMessageId);
	ASSERT_RETURN(entry);

	entry->message.TimeOfManditoryDeletionUTC = msg->timeOfManditoryDeletionUTC;
	entry->message.bIsMarkedRead = msg->bIsMarkedRead;
	entry->message.AttachedItemId = msg->idAttachedItemId;
	entry->message.AttachedMoney = msg->dwAttachedMoney;

	UIEmailOnUpdate();
}


#endif	//	ISVERSION(SERVER_VERSION)
