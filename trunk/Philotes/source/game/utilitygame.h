//----------------------------------------------------------------------------
// FILE: utilitygame.h
//----------------------------------------------------------------------------

#ifndef __UTILITY_GAME_H_
#define __UTILITY_GAME_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "../Chat/ChatServer.h"
#include "c_message.h"

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum CSR_EMAIL_RECIPIENT_TYPE;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

// !NOTE! Please keep these limits in sync with the CSR Portal tool
// edit box controls tha will limit the # of characters that can be
// typed in the UI
#define MAX_CSR_EMAIL_SUBJECT	MAX_EMAIL_SUBJECT
#define MAX_CSR_EMAIL_BODY		800		// MAX_EMAIL_BODY, note that it's note the same as player in game email because we have some message size limits we have to deal with

//----------------------------------------------------------------------------
enum UTILITY_GAME_ACTION
{
	UGA_INVALID = -1,
	
	UGA_CSR_EMAIL,				// in game email from a CSR rep
	UGA_CSR_ITEM_RESTORE,		// restore a "lost" item from the warehouse db and send to user
	UGA_AH_EMAIL_SEND_ITEM_TO_PLAYER,
	
	UGA_NUM_ACTIONS				// keep this last please
};

//----------------------------------------------------------------------------
struct UTILITY_GAME_CSR_EMAIL
{

	DWORD dwCSRPipeID;							// CSR Portal pipe id this request came through
	CSR_EMAIL_RECIPIENT_TYPE eRecipientType;	// type of id and name of recipient
	ULONGLONG idRecipient;						// id of recipient (account id, character id, guild id)
	WCHAR uszRecipient[ MAX_CHARACTER_NAME ];	// the name of the recipient (character, account, guild)
	WCHAR uszSubject[ MAX_CSR_EMAIL_SUBJECT ];	// the subject
	WCHAR uszBody[ MAX_CSR_EMAIL_BODY ];		// the email body

	// constructor	
	UTILITY_GAME_CSR_EMAIL( void );
	
};

//----------------------------------------------------------------------------
struct UTILITY_GAME_CSR_ITEM_RESTORE
{

	DWORD dwCSRPipeID;				// CSR Portal pipe id this request came through
	UNIQUE_ACCOUNT_ID idAccount;	// account to send the restored item to
	PGUID guidCharacter;			// the character that we are restoring the item for (we got this character from the item log database, it might not exist today as the character could be deleted)
	PGUID guidItem;					// the GUID of the item we want restore, this came from the item log warehouse db

	UNITID idTempRestoreItem;		// temporary item instance that is being restored
		
	// constructor	
	UTILITY_GAME_CSR_ITEM_RESTORE( void );
	
};

//----------------------------------------------------------------------------
struct UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER
{
	PGUID	idPlayer;
	PGUID	idItem;
	BOOL	bIsWithdrawn;
	WCHAR	szPlayerName[MAX_CHARACTER_NAME];
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
	
const char *UtilityGameActionToString(
	UTILITY_GAME_ACTION eAction);
	
GAMEID UtilityGameGetGameId(
	struct GameServerContext* pSvrCtx);

BOOL UtilityGameProcessRequest( 
	GAME *pGame,
	const MSG_APPCMD_UTILITY_GAME_REQUEST *pMessage);
	
void *UtilityGameGetRequestData(
	ASYNC_REQUEST_ID idRequest);
		
#if ISVERSION(SERVER_VERSION)
BOOL AuctionOwnerRetrieveAccountInfo(
	UNIQUE_ACCOUNT_ID* pAccountID,
	PGUID* pCharID);
#endif

#endif
