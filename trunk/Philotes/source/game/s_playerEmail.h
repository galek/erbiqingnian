//----------------------------------------------------------------------------
// s_playerEmail.h
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _S_PLAYER_EMAIL_H_
#define _S_PLAYER_EMAIL_H_


#ifndef _STLALLOCATOR_H_
#include "stlallocator.h"
#endif

#ifndef _C_MESSAGE_H_
#include "c_message.h"
#endif

#include "email.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_INDIVIDUAL_TARGET_CHARACTERS	50


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum PLAYER_EMAIL_RESULT
{
	PLAYER_EMAIL_SUCCESS = 0,

	PLAYER_EMAIL_UNKNOWN_ERROR,			//	an error occurred that stopped the email from ever being created, show something to the user
	PLAYER_EMAIL_UNKNOWN_ERROR_SILENT,	//	an error occurred that will be handled later by an auto bounce behind the scenes, show nothing to the user now
	PLAYER_EMAIL_NO_RECIPIENTS_SPECIFIED,
	PLAYER_EMAIL_INVALID_ITEM,
	PLAYER_EMAIL_INVALID_ATTACHMENTS,
	PLAYER_EMAIL_NO_MULTSEND_ATTACHEMENTS,
	PLAYER_EMAIL_INVALID_MONEY,
	PLAYER_EMAIL_NOT_A_GUILD_MEMBER,
	PLAYER_EMAIL_TARGET_IGNORING_SENDER,
	PLAYER_EMAIL_TARGET_NOT_FOUND,
	PLAYER_EMAIL_GUILD_RANK_TOO_LOW,
	PLAYER_EMAIL_TARGET_IS_TRIAL_USER,
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum PLAYER_EMAIL_TYPES
{
	PLAYER_EMAIL_TYPE_INVALID = 0,

	PLAYER_EMAIL_TYPE_PLAYER			= 1,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_GUILD				= 2,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_CSR				= 3,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_SYSTEM			= 4,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_AUCTION			= 5,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_CONSIGNMENT		= 6,	// do not change value, written to database!!!	
	PLAYER_EMAIL_TYPE_ITEMSALE			= 7,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_ITEMBUY			= 8,	// do not change value, written to database!!!
	PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE	= 9,	// do not change value, written to database!!!	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum EMAIL_RECIPIENT_TYPE
{
	ERT_INVALID = -1,
	
	ERT_CHARACTER = 0,	// do not change value, used in database stored procedure sp_email_add_recipient_by_id
	ERT_ACCOUNT = 1,	// do not change value, used in database stored procedure sp_email_add_recipient_by_id
	
	ERT_NUM_TYPES		// keep this last please
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)

void
	s_PlayerEmailFreeEmail(
		struct GAME * game,
		struct UNIT * unit );

PGUID s_PlayerEmailCreateNewEmail(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	const WCHAR * emailSubject,
	const WCHAR * emailBody,
	EMAIL_SPEC_SOURCE emailSpecSource = ESS_PLAYER);

void
	s_PlayerEmailAddEmailRecipientByCharacterName(
		struct UNIT * unit,
		const WCHAR * playerName );

void
	s_PlayerEmailAddGuildMembersAsRecipients(
		struct UNIT * unit );

void
	s_PlayerEmailAddEmailAttachments(
		struct UNIT * unit,
		PGUID idItemToAttach,
		DWORD dwMoneyToAttach );

BOOL
	s_PlayerEmailSend(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		DWORD dwFee = (DWORD)-1);

BOOL
	s_PlayerEmailCSRSend(
		struct GAME * game,
		const struct UTILITY_GAME_CSR_EMAIL * csrEmail,
		ASYNC_REQUEST_ID idRequest = INVALID_ID,
		struct UNIT * item = NULL);

BOOL
	s_PlayerEmailCSRSendItemRestore(
		struct GAME * game,
		struct UNIT * item,
		const struct UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreData,
		ASYNC_REQUEST_ID idRequest);

BOOL
	s_PlayerEmailAHSendItemToPlayer(
		struct GAME * game,
		const struct UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER* pEmailInfo);


void 
	s_PlayerEmailCSRSendComplete(
		GAME *pGame,	
		ULONGLONG idEmail,
		BOOL bSuccess,
		const char *pszMessage,
		BOOL bRemoveSpec);		
	
void
	s_PlayerEmailHandleCreationResult(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG idEmailMessageId,
		DWORD emailCreationResultFlags,
		enum EMAIL_SPEC_SOURCE eEmailSpecSource,
		DWORD dwUserContext,
		const vector_mp<wstring_mp>::type & ignoringPlayersList );

void
	s_SystemEmailHandleItemMoved(
		GAME * game,
		EMAIL_SPEC_SOURCE eSpecSource,
		GAMECLIENT * client,
		UNIT * unit,
		ULONGLONG moveContext,
		PGUID itemGuid,
		BOOL bSuccess,
		ULONGLONG messageId );

void 
	s_AuctionOwnerEmailHandleItemMoved(
		GAME * game,
		ULONGLONG moveContext,
		PGUID itemId,
		BOOL bSuccess,
		ULONGLONG messageId);
		
void
	s_PlayerEmailHandleMoneyDeducted(
		GAME * game,
		GAMECLIENT * client,
		UNIT * unit,
		BOOL bSuccess );

void
	s_PlayerEmailHandleDeliveryResult(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG idEmailMessage,
		enum EMAIL_SPEC_SOURCE eSpecSource,
		BOOL bSuccess );

void
	s_PlayerEmailRetrieveMessages(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId = (ULONGLONG)-1 );

void
	s_PlayerEmailHandleEmailMetadata(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		MSG_APPCMD_EMAIL_METADATA * message );

void
	s_PlayerEmailHandleEmailData(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		MSG_APPCMD_ACCEPTED_EMAIL_DATA * message );

void
	s_PlayerEmailHandleEmailDataUpdate(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		MSG_APPCMD_EMAIL_UPDATE * message );

void
	s_PlayerEmailRestoreOutboxItemLocation(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit );

void
	s_PlayerEmailMarkMessageAsRead(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId );

void
	s_PlayerEmailRemoveAttachedMoney(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId );

void
	s_PlayerEmailHandleAttachedMoneyInfoForRemoval(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId,
		DWORD dwMoneyAmmount,
		PGUID idMoneyUnitId );

void
	s_PlayerEmailHandleAttachedMoneyRemovalResult(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId,
		DWORD dwMoneyAmmount,
		BOOL bSuccess );

void
	s_PlayerEmailClearAttachedItem(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId );

void 
	s_PlayerEmailItemAttachmenTaken(
		struct UNIT *pPlayer,
		struct UNIT *pItem);

void
	s_PlayerEmailDeleteMessage(
		struct GAME * game,
		struct GAMECLIENT * client,
		struct UNIT * unit,
		ULONGLONG messageId );

#endif	//	ISVERSION(SERVER_VERSION)

int
	x_PlayerEmailGetCostToSend(
		UNIT * pSourcePlayerUnit,
		PGUID attachedItem,
		UINT32 attachedMoney );

#endif	//	_S_PLAYER_EMAIL_H_
