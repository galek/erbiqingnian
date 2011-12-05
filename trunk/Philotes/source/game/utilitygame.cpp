//----------------------------------------------------------------------------
// FILE: utilitygame.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "asyncrequest.h"
#include "email.h"
#include "itemrestore.h"
#include "items.h"
#include "utilitygame.h"
#include "prime.h"
#include "game.h"
#include "s_network.h"
#include "s_playerEmail.h"

#if ISVERSION(SERVER_VERSION)
#include "ServerRunnerAPI.h"
#include "DatabaseManager.h"
#include "database.h"
#include "utilitygame.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

#if ISVERSION(SERVER_VERSION)
struct AuctionOwnerRetrieveInfo
{
	typedef struct
	{
		UNIQUE_ACCOUNT_ID* pAccountID;
		PGUID* pCharID;
	} InitData, DataType;

	static BOOL InitRequestData(
		DataType* pData,
		InitData* pInit)
	{
		ASSERT_RETFALSE(pInit != NULL);
		ASSERT_RETFALSE(pData != NULL);
		ASSERT_RETFALSE(pInit->pAccountID != NULL);
		ASSERT_RETFALSE(pInit->pCharID != NULL);
		pData->pAccountID = pInit->pAccountID;
		pData->pCharID = pInit->pCharID;
		return TRUE;
	}

	static void FreeRequestData(DataType*) { }

	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC hDBC,
		DataType* pData)
	{
		ASSERT_RETX(hDBC != NULL, DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(pData != NULL, DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(pData->pAccountID != NULL, DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(pData->pCharID != NULL, DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLRETURN hRet;

		SQLLEN targetAccountIDLen = 0;
		SQLLEN targetCharIDLen = 0;

		LPCSTR szStatement = 
			"SELECT tbl_characters.account_id, tbl_characters.unit_id "
			"FROM tbl_characters "
			"WHERE tbl_characters.name = '" AUCTION_OWNER_NAME_A "'";
		CScopedSQLStmt hStmt(hDBC, szStatement);
		ASSERT_GOTO(hStmt, _err);

		hRet = SQLBindCol(hStmt, 1, SQL_C_SBIGINT, pData->pAccountID, sizeof(*pData->pAccountID), &targetAccountIDLen);
		ASSERTV_GOTO(SQL_OK(hRet), _err, "Unable to bind account id.");

		hRet = SQLBindCol(hStmt, 2, SQL_C_SBIGINT, pData->pCharID, sizeof(*pData->pCharID), &targetCharIDLen);
		ASSERTV_GOTO(SQL_OK(hRet), _err, "Unable to bind character id.");

		hRet = SQLExecute(hStmt);
		ASSERTV_GOTO(SQL_OK(hRet), _err, "Unable to execute statement.");

		hRet = SQLFetch(hStmt);
		ASSERT_GOTO(SQL_OK(hRet), _err);

		if (targetAccountIDLen == SQL_NULL_DATA ||
			targetCharIDLen == SQL_NULL_DATA) {
			goto _err;
		}

		toRet = DB_REQUEST_RESULT_SUCCESS;

_err:
		if (toRet != DB_REQUEST_RESULT_SUCCESS) {
			TraceError("AuctionOwnerRetrieveInfo DB Error. Statement: %s, SQL Error: %s",
				szStatement, GetSQLErrorStr(hStmt).c_str());
			*pData->pAccountID = INVALID_UNIQUE_ACCOUNT_ID;
			*pData->pCharID = INVALID_GUID;
		}
		return toRet;
	}

	NULL_BATCH_MEMBERS();
};

DB_REQUEST_BATCHER<AuctionOwnerRetrieveInfo>::_BATCHER_DATA DB_REQUEST_BATCHER<AuctionOwnerRetrieveInfo>::s_dat;

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UTILITY_GAME_CSR_EMAIL::UTILITY_GAME_CSR_EMAIL( void )
	:	dwCSRPipeID( 0 ),
		eRecipientType( CERT_INVALID ),
		idRecipient( (ULONGLONG)INVALID_ID )
{	
	uszRecipient[ 0 ] = 0;
	uszSubject[ 0 ] = 0;
	uszBody[ 0 ] = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UTILITY_GAME_CSR_ITEM_RESTORE::UTILITY_GAME_CSR_ITEM_RESTORE( void )
	:	dwCSRPipeID( 0 ),
		idAccount( INVALID_UNIQUE_ACCOUNT_ID ),
		guidCharacter( INVALID_GUID ),
		guidItem( INVALID_GUID ),
		idTempRestoreItem( INVALID_ID )
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *UtilityGameActionToString(
	UTILITY_GAME_ACTION eAction)
{
	switch (eAction)
	{
		case UGA_CSR_EMAIL:				return "csr email";
		case UGA_CSR_ITEM_RESTORE:		return "csr item restore";
		case UGA_AH_EMAIL_SEND_ITEM_TO_PLAYER: return "ah email send item to player";
		default:						return "unknown";
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID UtilityGameGetGameId(
	GameServerContext *pServerContext)
{
	GAMEID idGame = INVALID_ID;
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(pServerContext != NULL, INVALID_ID);

	CSAutoLock autoLock(&pServerContext->m_csUtilGameInstance);
	if (pServerContext->m_idUtilGameInstance == INVALID_ID) 
	{
		GAME_SETUP gameSetup;
		SETBIT(gameSetup.tGameVariant.dwGameVariantFlags, GV_UTILGAME);
		pServerContext->m_idUtilGameInstance = SrvNewGame(GAME_SUBTYPE_CUSTOM, &gameSetup, TRUE, 0);
		ASSERT(pServerContext->m_idUtilGameInstance != INVALID_ID);
	}
	idGame = pServerContext->m_idUtilGameInstance;
#endif
	return idGame;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilityGameProcessRequest(
	GAME *pGame,
	const MSG_APPCMD_UTILITY_GAME_REQUEST *pMessage)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pMessage, "Expected message" );
	
#if ISVERSION(SERVER_VERSION)
	UTILITY_GAME_ACTION eAction = (UTILITY_GAME_ACTION)pMessage->dwUtilityGameAction;	

	switch( eAction )
	{
		
		//----------------------------------------------------------------------------
		case UGA_CSR_EMAIL:
		{
			ASSERTX_RETFALSE( pMessage->dwMessageSize == sizeof( UTILITY_GAME_CSR_EMAIL ), "Invalid utility game request message data" );
			const UTILITY_GAME_CSR_EMAIL *pEmailRequest = (const UTILITY_GAME_CSR_EMAIL *)pMessage->byMessageData;
			
			// we don't need to save this request data because we store everything
			// we need in the email subsystems
			
			// start the email send
			return s_PlayerEmailCSRSend( pGame, pEmailRequest );
			
		}

		//----------------------------------------------------------------------------
		case UGA_CSR_ITEM_RESTORE:
		{
			ASSERTX_RETFALSE( pMessage->dwMessageSize == sizeof( UTILITY_GAME_CSR_ITEM_RESTORE ), "Invalid utility game request message data" );
			const UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest = (const UTILITY_GAME_CSR_ITEM_RESTORE *)pMessage->byMessageData;
			
			// save the request data so we can refer it later after we've loaded
			// item data from the database
			ASYNC_REQUEST_ID idRequest = AsyncRequestNew( pGame, (void *)pRestoreRequest, sizeof( *pRestoreRequest ) );
			if (idRequest == INVALID_ID)
			{
				s_ItemRestoreSendResult( pGame, pRestoreRequest, FALSE, "Unable to allocate async request" );
				return FALSE;
			}
			else
			{
				// start the request
				return s_ItemRestore( pGame, pRestoreRequest, idRequest );
			}
			break;
						
		}

		//----------------------------------------------------------------------------
		case UGA_AH_EMAIL_SEND_ITEM_TO_PLAYER:
		{
			ASSERTX_RETFALSE( pMessage->dwMessageSize == sizeof( UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER ), "Invalid utility game request message data" );
			const UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER* pEmailRequest = (UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER*)pMessage->byMessageData;
			return s_PlayerEmailAHSendItemToPlayer(pGame, pEmailRequest);
		}

		//----------------------------------------------------------------------------
		default:
		{
			break;
		}
		
	}
#endif

	return FALSE;
		
}

#if ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AuctionOwnerRetrieveAccountInfo(
	UNIQUE_ACCOUNT_ID* pAccountID,
	PGUID* pCharID)
{
	ASSERT_RETFALSE(pAccountID != NULL);
	ASSERT_RETFALSE(pCharID != NULL);

	AuctionOwnerRetrieveInfo::InitData initData;
	initData.pAccountID = pAccountID;
	initData.pCharID = pCharID;

	DB_REQUEST_TICKET nullTicket;
	return DatabaseManagerQueueRequest<AuctionOwnerRetrieveInfo>(
		SvrGetGameDatabaseManager(),
		TRUE,
		nullTicket,
		NULL,
		NULL,
		FALSE,
		&initData);
}
#endif
