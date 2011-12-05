//----------------------------------------------------------------------------
// FILE: c_trade.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_input.h"
#include "c_message.h"
#include "c_trade.h"
#include "c_chatNetwork.h"
#include "console.h"
#include "globalindex.h"
#include "inventory.h"
#include "items.h"
#include "npc.h"
#include "player.h"
#include "quests.h"
#include "s_message.h" // also includes prime.h
#include "stringtable.h"
#include "trade.h"
#include "treasure.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_chat.h"
#include "unit_priv.h" // also includes units.h, which includes games.h
#include "uix_scheduler.h"
#include "uix_menus.h"
#include "Currency.h"
#include "stringreplacement.h"

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
static const int MONEY_INVALID = -1;
static const int REPEAT_DELAY_IN_MSECS = 10;
static const int MONEY_NOTIFY_DELAY_IN_MS = 1500;

//----------------------------------------------------------------------------
enum REPEAT_MONEY
{
	REPEAT_MONEY_INVALID = -1,
	REPEAT_MONEY_INCREASE,
	REPEAT_MONEY_DECREASE,
};

//----------------------------------------------------------------------------
struct CTRADE_GLOBALS
{
	UNITID idTradingWith;
	TRADE_STATUS eMyStatus;
	TRADE_STATUS eTheirStatus;
	
	TIME tiRepeatMoneyStartTime;
	DWORD dwRepeatMoneyEvent;
	REPEAT_MONEY eRepeatType;
	int nRepeatMoney;

	DWORD dwNotifyServerMoneyEvent;
		
	int nTradeOfferVersion;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static CTRADE_GLOBALS* sgpCTradeGlobals = NULL;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientTradeGlobalsInit(
	CTRADE_GLOBALS* pCTradeGlobals)
{
	pCTradeGlobals->idTradingWith = INVALID_ID;
	pCTradeGlobals->eMyStatus = TRADE_STATUS_NONE;
	pCTradeGlobals->eTheirStatus = TRADE_STATUS_NONE;
	
	pCTradeGlobals->tiRepeatMoneyStartTime = 0;
	pCTradeGlobals->dwRepeatMoneyEvent = 0;
	pCTradeGlobals->eRepeatType = REPEAT_MONEY_INVALID;
	pCTradeGlobals->nRepeatMoney = 0;
	
	pCTradeGlobals->dwNotifyServerMoneyEvent = 0;
	pCTradeGlobals->nTradeOfferVersion = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeInit(
	GAME* pGame)
{
	sgpCTradeGlobals = (CTRADE_GLOBALS*)GMALLOCZ( pGame, sizeof( CTRADE_GLOBALS ) );
	sClientTradeGlobalsInit( sgpCTradeGlobals );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeFree(
	GAME* pGame)
{
	GFREE( pGame, sgpCTradeGlobals );
	sgpCTradeGlobals = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CTRADE_GLOBALS* c_sCTradeGlobalsGet(
	void)
{
	ASSERTX_RETNULL( IS_CLIENT( AppGetCltGame() ), "Client only" );
	ASSERTX_RETNULL( sgpCTradeGlobals != NULL, "CTradeGlobals are NULL!" );
	return sgpCTradeGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeStart(
	UNIT* pTradingWith,
	const TRADE_SPEC &tTradeSpec)
{
	ASSERTX_RETURN( pTradingWith, "Expected trading with" );
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();	
	ASSERTX_RETURN( pCTradeGlobals, "Expected trade globals" );	
	UNITID idTradingWith = UnitGetId( pTradingWith );

	// hide any asking/request dialog
	UIHideGenericDialog();

	// if we're already trading, stop it and do what the server is now telling us
	if (pCTradeGlobals->idTradingWith != INVALID_ID)
	{
		ASSERTX( 0, "Already trading with a player, cancelling old trade session to begin new one" );
		c_TradeForceCancel();
	}
		
	// record who we're trading with	
	ASSERTX_RETURN( pCTradeGlobals->idTradingWith == INVALID_ID, "Expected trading with nobody" );
	pCTradeGlobals->idTradingWith = idTradingWith;
	pCTradeGlobals->eMyStatus = TRADE_STATUS_NONE;
	pCTradeGlobals->eTheirStatus = TRADE_STATUS_NONE;
	pCTradeGlobals->nTradeOfferVersion = 0;

	// init status buttons
	c_TradeSetStatus( idTradingWith, pCTradeGlobals->eTheirStatus );
	c_TradeSetStatus( LocalPlayerGetID(), pCTradeGlobals->eMyStatus );
		
	// show the trade interface		
	UIShowTradeScreen( idTradingWith, FALSE, &tTradeSpec );

	// init the money controls
	c_TradeMoneyModified( 0, 0, 0, 0 );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TradeCancel(
	BOOL bCloseUI)
{
	GAME* pGame = AppGetCltGame();
	if (!pGame)
	{
		return FALSE;
	}

	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	ASSERTX_RETFALSE( pCTradeGlobals, "Expected trade globals" );		
	UNITID idTradingWith = pCTradeGlobals->idTradingWith;

	if (idTradingWith != INVALID_ID)
	{
		
		// hide our UI	
		if (!bCloseUI || c_TradeForceCancel())
		{
		
			// tell the server we cancelled the trade ourselves
			MSG_CCMD_TRADE_CANCEL tMessage;
			c_SendMessage( CCMD_TRADE_CANCEL, &tMessage );

			return TRUE;
		}
						
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TradeForceCancel(
	void)
{
	GAME* pGame = AppGetCltGame();
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	ASSERTX_RETFALSE( pCTradeGlobals, "Expected trade globals" );		
	UNITID idTradingWith = pCTradeGlobals->idTradingWith;
	UNIT* pTradingWith = UnitGetById( pGame, idTradingWith );

	// record that we're no longer trading, it's important to do this first to prevent recursive
	// calls, which frequently happens with the ui and deactivate events that try to
	// cancel trade as ui pieces go away
	pCTradeGlobals->idTradingWith = INVALID_ID;
	
	if (pTradingWith)
	{
		if( AppIsTugboat() )
		{
			UIHidePaneMenu( KPaneMenuStash );
		}
		
		if (UnitIsMerchant( pTradingWith ))
		{
		
			// done tradign with merchant		
			if (UIIsMerchantScreenUp())
			{
				UIHideMerchantScreen();
			}
		}
		else
		{
			// close the trade UI
			UIShowTradeScreen( INVALID_ID, TRUE, NULL );
		}

		return TRUE;  // have cancelled
							
	}

	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TradeIsTrading(
	void)
{
	
	// it's OK for this question to be asked when the trade system 
	// has not been initialized, happens because some UI screen logic
	if (sgpCTradeGlobals == NULL)
	{
		return FALSE;
	}

	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	ASSERTX_RETFALSE( pCTradeGlobals, "Expected trade globals" );
	return pCTradeGlobals->idTradingWith != INVALID_ID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEventEnableTradeButton(
	GAME* pGame,
	const CEVENT_DATA& data,
	DWORD )
{
	UI_COMPONENT* pButton = (UI_COMPONENT*)data.m_Data1;
	ASSERT_RETURN(pButton);

	UIComponentSetActive(pButton, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeSetStatus(
	UNITID idTrader,
	TRADE_STATUS eStatus,
	int nTradeOfferVersion)
{
	CTRADE_GLOBALS *pCTradeGlobals = c_sCTradeGlobalsGet();	
	ASSERTX_RETURN( pCTradeGlobals, "Expected trade globals" );		
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	UNITID idPlayer = UnitGetId( pPlayer );
	
	// if this is our status
	UI_COMPONENT_ENUM eLabel = UICOMP_INVALID;
	if (idTrader == idPlayer)
	{
		if (AppIsHellgate() && eStatus == TRADE_STATUS_NONE /*&& pCTradeGlobals->eMyStatus != eStatus*/)
		{
			UI_COMPONENT *pButton = UIComponentGetByEnum(UICOMP_TRADE_BUTTON_MY_ACCEPT);
			if (pButton)
			{
				UIComponentSetActive(pButton, FALSE);
				CSchedulerRegisterEvent(AppCommonGetCurTime() + (MSECS_PER_SEC*3), sEventEnableTradeButton, CEVENT_DATA((DWORD_PTR)pButton, NULL, NULL, NULL));
			}			
		}

		pCTradeGlobals->eMyStatus = eStatus;
		if(nTradeOfferVersion != NONE) pCTradeGlobals->nTradeOfferVersion = nTradeOfferVersion;
		eLabel = UICOMP_TRADE_BUTTON_LABEL_MY_ACCEPT;
	}
	else
	{
		pCTradeGlobals->eTheirStatus = eStatus;
		eLabel = UICOMP_TRADE_BUTTON_LABEL_THEIR_ACCEPT;	
	}

	// what is the string to use
	GLOBAL_STRING eString = GS_INVALID; 
	if (eStatus == TRADE_STATUS_ACCEPT)
	{
		eString = GS_TRADE_STATUS_ACCEPT;
	}
	else
	{
		eString = GS_TRADE_STATUS_NONE;	
	}
		
	// set the button text
	if( AppIsHellgate() || idTrader == idPlayer )
	{
		UILabelSetTextKeyByEnum( eLabel, GlobalStringGetKey( eString ) );
	}
	if( AppIsTugboat() )
	{
		if (idTrader == idPlayer)
		{
			UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_TRADE_HIGHLIGHT_MY_ACCEPT);
			if (eStatus == TRADE_STATUS_ACCEPT)
			{
				UIComponentSetActive(pComponent, TRUE);
			}
			else
			{
				UIComponentSetActive(pComponent, FALSE);
				UIComponentSetVisible(pComponent, FALSE);
			}
		}
		else
		{
			UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_TRADE_HIGHLIGHT_THEIR_ACCEPT);
			if (eStatus == TRADE_STATUS_ACCEPT)
			{
				UIComponentSetActive(pComponent, TRUE);
			}
			else
			{
				UIComponentSetVisible(pComponent, FALSE);
				UIComponentSetActive(pComponent, FALSE);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChangeTradeAcceptStatus(
	TRADE_STATUS eStatus)
{
	
	// tell our client about the change
	c_TradeSetStatus( LocalPlayerGetID(), eStatus );
	
	// tell the server about our new status
	MSG_CCMD_TRADE_STATUS tMessage;
	tMessage.nStatus = eStatus;
	CTRADE_GLOBALS *pCTradeGlobals = c_sCTradeGlobalsGet();	
	if(pCTradeGlobals) tMessage.nTradeOfferVersion =
		pCTradeGlobals->nTradeOfferVersion;
	c_SendMessage( CCMD_TRADE_STATUS, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_GetMoneyValueInComponent(
	UI_COMPONENT *pComponent)
{
	ASSERTX_RETVAL( pComponent, MONEY_INVALID, "Expected compoent" );
	
	// get text from control
	UI_EDITCTRL *pEditControl = UICastToEditCtrl( pComponent );
	if (!pEditControl || !pEditControl->m_Line.HasText())
	{
		return MONEY_INVALID;
	}

	const WCHAR *puszText = pEditControl->m_Line.GetText();
	
	// get new money value
	int nMoney;
	if (PStrToInt( puszText, nMoney ) == FALSE)
	{
		return MONEY_INVALID;
	}

	return nMoney;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoMoneyNotify(
	GAME *pGame,
	const CEVENT_DATA &tCEventData,
	DWORD)
{

	// clear event
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	pCTradeGlobals->dwNotifyServerMoneyEvent = 0;

	// get money
	UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );				
	int nMoneyInComp = c_GetMoneyValueInComponent( pCompMoneyYours );
	int nRealWorldComp = 0;
	// send
	MSG_CCMD_TRADE_MODIFY_MONEY tMessage;
	tMessage.nMoney = nMoneyInComp;
	tMessage.nRealWorldMoney = nRealWorldComp;
	c_SendMessage( CCMD_TRADE_MODIFY_MONEY, &tMessage );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeToggleAccept(
	void)
{
	CTRADE_GLOBALS *pCTradeGlobals = c_sCTradeGlobalsGet();	
	ASSERTX_RETURN( pCTradeGlobals, "Expected trade globals" );		
	TRADE_STATUS eStatus = pCTradeGlobals->eMyStatus;
	
	// switch status
	if (eStatus == TRADE_STATUS_NONE)
	{
		eStatus = TRADE_STATUS_ACCEPT;
	}
	else
	{
		eStatus = TRADE_STATUS_NONE;
	}

	// if we have a money notify event, do it now instead of waiting for the timer
	if (pCTradeGlobals->dwNotifyServerMoneyEvent != 0)
	{
		CSchedulerCancelEvent(pCTradeGlobals->dwNotifyServerMoneyEvent );
		sDoMoneyNotify( AppGetCltGame(), NULL, 0 );
	}
	
	// change trade status
	sChangeTradeAcceptStatus( eStatus );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeCannotAccept(
	void)
{

	// clear our accept mode
	c_TradeSetStatus( LocalPlayerGetID(), TRADE_STATUS_NONE );
	
	// display a message window or string or something
	ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TRADE_CANNOT_ACCEPT ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackTradeRequestAccept(
	PGUID guidOtherPlayer, 
	void *)
{
	// tell the server
	MSG_CCMD_TRADE_REQUEST_ACCEPT tMessage;
	tMessage.idToTradeWith = (UNITID)guidOtherPlayer;	
	c_SendMessage( CCMD_TRADE_REQUEST_ACCEPT, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackTradeRequestReject(
	PGUID guidOtherPlayer, 
	void *)
{
	// tell the server
	MSG_CCMD_TRADE_REQUEST_REJECT tMessage;
	tMessage.idToTradeWith = (UNITID)guidOtherPlayer;	
	c_SendMessage( CCMD_TRADE_REQUEST_REJECT, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackTradeRequestIgnore(
	PGUID guidOtherPlayer, 
	void *pData)
{
	sCallbackTradeRequestReject(guidOtherPlayer, pData);

	// add the player to the chat ignore list
	UNITID idUnit = (UNITID)guidOtherPlayer;
	GAME *pGame = AppGetCltGame();
	ASSERT_RETURN(pGame);
	UNIT *pUnit = UnitGetById(pGame, idUnit);
	ASSERT_RETURN(pUnit);

	PGUID guidRealGuid = UnitGetGuid(pUnit);
	c_ChatNetIgnoreMember(guidRealGuid);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackCancelAskToTrade(
	void *pUserData, 
	DWORD dwCallbackData)
{
	
	// tell the server to forget it
	MSG_CCMD_TRADE_REQUEST_NEW_CANCEL tMessage;
	c_SendMessage( CCMD_TRADE_REQUEST_NEW_CANCEL, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeAskOtherToTrade(
	UNITID idToTradeWith)
{
	GAME *pGame = AppGetCltGame();
	
	// get unit to trade with
	UNIT *pUnitToTradeWith = UnitGetById( pGame, idToTradeWith );	
	if (pUnitToTradeWith == NULL)
	{
		return;
	}

	// must be within interact distance
	UNIT *pLocalPlayer = LocalPlayerGet();
	float flDistSq = UnitsGetDistanceSquared( pLocalPlayer, pUnitToTradeWith );
	float flTradeDistanceSq = TRADE_DISTANCE_SQ - 1.0f;  // minus a little fudge
	if (flDistSq >= flTradeDistanceSq)
	{
					
		// start the conversation dialog
		const WCHAR *puszText = GlobalStringGet( GS_TRADE_TOO_FAR );
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_TRADE ), puszText, FALSE );
		
	}
	else
	{
		const WCHAR *puszText = GlobalStringGet( GS_TRADE_WAITING_FOR_RESPONSE );

		// setup callback
		DIALOG_CALLBACK tCallback;
		DialogCallbackInit( tCallback );
		tCallback.pfnCallback = sCallbackCancelAskToTrade;
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_TRADE ), puszText, FALSE, &tCallback );
		
		// tell the server we're waiting
		c_ChatNetRequestSocialInteraction(pUnitToTradeWith, SOCIAL_INTERACTION_TRADE_REQUEST);

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeBeingAskedToTrade(
	UNITID idToTradeWith)
{
	GAME *pGame = AppGetCltGame();
	CTRADE_GLOBALS *pCTradeGlobals = c_sCTradeGlobalsGet();	
	ASSERTX_RETURN( pCTradeGlobals, "Expected trade globals" );		
	
	// get unit to trade with
	UNIT *pUnitToTradeWith = UnitGetById( pGame, idToTradeWith );	
	if (pUnitToTradeWith == NULL)
	{
		return;
	}
	
	// get the conversation text
	const WCHAR *puszRequestText = GlobalStringGet( GS_TRADE_REQUEST_NEW );	

	// get player name
	WCHAR uszName[ MAX_CHARACTER_NAME ];
	UnitGetName( pUnitToTradeWith, uszName, arrsize(uszName), 0 );
	
	// replace the player name into the text
	const WCHAR *puszToken = StringReplacementTokensGet( SR_PLAYER );
	WCHAR uszText[ MAX_STRING_ENTRY_LENGTH ];
	PStrCopy( uszText, puszRequestText, MAX_STRING_ENTRY_LENGTH );
	PStrReplaceToken( uszText, arrsize(uszText), puszToken, uszName );
																														
	UIShowPlayerRequestDialog((PGUID)idToTradeWith, uszText, sCallbackTradeRequestAccept, sCallbackTradeRequestReject, sCallbackTradeRequestIgnore );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeRequestCancelledByAskingPlayer(
	UNITID idInitialRequester)
{

	// the other player who was asking us cancelled their request to trade
	UICancelPlayerRequestDialog((PGUID)idInitialRequester, sCallbackTradeRequestAccept, sCallbackTradeRequestReject, sCallbackTradeRequestIgnore );
	UIAddServerChatLine( GlobalStringGet( GS_TRADE_TRADER_CANCELLED ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeRequestOtherBusy(
	void)
{

	// tell em the other player is busy
	UIHideGenericDialog();
	if( AppIsHellgate() )
	{
		UIAddServerChatLine( GlobalStringGet( GS_TRADE_OTHER_BUSY ) );
	}
	else
	{
		// let us know they're unable to trade with a dialog in Mythos.
		const WCHAR *puszText = GlobalStringGet( GS_TRADE_OTHER_BUSY );
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_TRADE ), puszText, FALSE );

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeRequestOtherRejected(
	void)
{

	// other player rejected trade offer
	UIHideGenericDialog();
	UIAddServerChatLine( GlobalStringGet( GS_TRADE_REJECTED ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeRequestOtherIsTrialUser(void)
{

	// other player rejected trade offer
	UIHideGenericDialog();
	UIAddServerChatLine( GlobalStringGet( GS_OTHER_PLAYER_IS_TRIAL_ACCOUNT ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeRequestOtherNotFound(
	void)
{
	UIHideGenericDialog();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeMoneyModified(
	int nMoneyYours,
	int nMoneyTheirs,
	int nMoneyRealWorldYours,
	int nMoneyRealWorldTheirs )
{
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "Expected game" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// get the money values in text form
	const int MAX_STRING = 64;
	WCHAR uszMoneyYours[ MAX_STRING ];
	WCHAR uszMoneyTheirs[ MAX_STRING ];
	PStrPrintf( uszMoneyYours, MAX_STRING, L"%d", nMoneyYours );
	PStrPrintf( uszMoneyTheirs, MAX_STRING, L"%d", nMoneyTheirs );
	
	// set your money
	UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );
	ASSERTX_RETURN( pCompMoneyYours, "Expected comp" );
	UILabelSetText( pCompMoneyYours, uszMoneyYours );

	// set their money
	UI_COMPONENT *pCompMoneyTheirs = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_THEIRS );
	ASSERTX_RETURN( pCompMoneyTheirs, "Expected comp" );
	UILabelSetText( pCompMoneyTheirs, uszMoneyTheirs );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendNewMoney(
	void)
{	
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();

	// if we have a notify event for the server, cancel it in favor of this new one
	if (pCTradeGlobals->dwNotifyServerMoneyEvent != 0)
	{
		CSchedulerCancelEvent( pCTradeGlobals->dwNotifyServerMoneyEvent );
		pCTradeGlobals->dwNotifyServerMoneyEvent = 0;
	}

	// if we are accepted, change our status to unaccepted so that the other player
	// sees at least that change (not the money) immediately
	if (pCTradeGlobals->eMyStatus == TRADE_STATUS_ACCEPT)
	{
		sChangeTradeAcceptStatus( TRADE_STATUS_NONE );
	}
		
	// create new event in a few seconds
	pCTradeGlobals->dwNotifyServerMoneyEvent = 
		CSchedulerRegisterEvent( AppCommonGetCurTime() + MONEY_NOTIFY_DELAY_IN_MS, sDoMoneyNotify, NULL );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SetMoneyValueInComponent(
	UI_COMPONENT *pComp,
	int nMoney)
{		
	ASSERTX_RETURN( pComp, "Expected component" );
	
	// get money value as string
	const int MAX_MONEY_STRING = 64;
	WCHAR uszMoney[ MAX_MONEY_STRING ];
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney );

	// set new value into your money control
	UILabelSetText( pComp, uszMoney );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSetTradeMoney(
	int nMoney,
	BOOL bForceSendToServer)
{
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );

	UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );				
	int nMoneyInComp = c_GetMoneyValueInComponent( pCompMoneyYours );
	
	// cap the money value 
	cCurrency playerCurrency = UnitGetCurrency( pPlayer );
	int nMoneyMax = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );
	
	nMoney = PIN( nMoney, 0, nMoneyMax );
	BOOL bChangedMoney = FALSE;
	if (nMoney != nMoneyInComp || bForceSendToServer == TRUE)
	{
	
		// set money value in comp
		c_SetMoneyValueInComponent( pCompMoneyYours, nMoney );
		
		// tell server about the new money value
		sSendNewMoney();

		// we've changed money
		bChangedMoney = TRUE;
		
	}
				
	return bChangedMoney;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeMoneyOnChar( 
	UI_COMPONENT *pComponent,
	WCHAR ucCharacter)
{
	ASSERTX_RETURN( pComponent, "Expected compoent" );
	
	if (ucCharacter == VK_RETURN)
	{
		UI_EDITCTRL *pEditControl = UICastToEditCtrl( pComponent );
		pEditControl->m_bHasFocus = FALSE;
	}
	else
	{
	
		// clean up money string (leading zeros for instance)
		int nMoney = c_GetMoneyValueInComponent( pComponent );
		c_SetMoneyValueInComponent( pComponent, nMoney );
		sSetTradeMoney( nMoney, TRUE );		
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCloseRepeatMoneyEvent(
	void)
{
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	
	// cancel the event
	CSchedulerCancelEvent( pCTradeGlobals->dwRepeatMoneyEvent );
	pCTradeGlobals->dwRepeatMoneyEvent = 0;
	
	// clear out other event data
	pCTradeGlobals->tiRepeatMoneyStartTime = 0;
	pCTradeGlobals->eRepeatType = REPEAT_MONEY_INVALID;
	pCTradeGlobals->nRepeatMoney = 0;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sButtonIsPressed(
	UI_COMPONENT *pCompButton)
{
	ASSERTX_RETFALSE( pCompButton, "Expected comp" );
	UI_BUTTONEX *pButton = UICastToButton( pCompButton );
	if (pButton)
	{
		return	UIComponentGetActive( pButton ) && 
				UIComponentCheckBounds( pButton ) &&
				UIButtonGetDown( pButton );
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoRepeatMoney(
	GAME *pGame,
	const CEVENT_DATA &tCEventData,
	DWORD)
{
	CTRADE_GLOBALS *pCTradeGlobals = c_sCTradeGlobalsGet();
	BOOL bContinueRepeat = FALSE;
		
	UI_COMPONENT *pCompForRepeat = NULL;
	if (pCTradeGlobals->eRepeatType == REPEAT_MONEY_INCREASE)
	{
		pCompForRepeat = UIComponentGetByEnum( UICOMP_TRADE_BUTTON_MONEY_INCREASE );
	}
	else
	{
		pCompForRepeat = UIComponentGetByEnum( UICOMP_TRADE_BUTTON_MONEY_DECREASE );
	}
	if (pCompForRepeat)
	{
	
		// mouse must be inside button and must still be pressed
		if (sButtonIsPressed( pCompForRepeat ))
		{
			
			// how long have we had the button down
			TIME tiPressedTimeInMS = AppCommonGetCurTime() - pCTradeGlobals->tiRepeatMoneyStartTime;
			
			// how much money will we increase by
			if (tiPressedTimeInMS > 300)
			{
				UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );				
				int nMoney = c_GetMoneyValueInComponent( pCompMoneyYours );
				int nChange = pCTradeGlobals->nRepeatMoney;
				if (pCTradeGlobals->eRepeatType == REPEAT_MONEY_DECREASE)
				{
					nChange = -nChange;
				}
				nMoney += nChange;
				sSetTradeMoney( nMoney, FALSE );

				// we've done another repeat event now
				pCTradeGlobals->nRepeatMoney += 2;

			}
			
			// do this again			
			bContinueRepeat = TRUE;
						
		}
				
	}

	// schedule another event if we want to keep going
	if (bContinueRepeat)
	{
		pCTradeGlobals->dwRepeatMoneyEvent = 
			CSchedulerRegisterEvent( AppCommonGetCurTime() + REPEAT_DELAY_IN_MSECS, sDoRepeatMoney, NULL);		
	}
	else
	{
		sCloseRepeatMoneyEvent();
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartRepeatMoney( 
	enum REPEAT_MONEY eType)
{
	CTRADE_GLOBALS* pCTradeGlobals = c_sCTradeGlobalsGet();
	
	// ignore this if we already have an event for the same type
	if (eType == pCTradeGlobals->eRepeatType)
	{
		return;
	}

	// close any event that was there before
	sCloseRepeatMoneyEvent();
	
	// setup a new event
	pCTradeGlobals->tiRepeatMoneyStartTime = AppCommonGetCurTime();
	pCTradeGlobals->dwRepeatMoneyEvent = 
		CSchedulerRegisterEvent( AppCommonGetCurTime() + REPEAT_DELAY_IN_MSECS, sDoRepeatMoney, NULL);	
	pCTradeGlobals->eRepeatType = eType;
	UNIT *pUnit = UIGetControlUnit();
	int moneyValue = UnitGetCurrency( pUnit ).GetValue( KCURRENCY_VALUE_INGAME );
	pCTradeGlobals->nRepeatMoney = MAX( 1, moneyValue / 100000);
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeMoneyOnIncrease( 
	UI_COMPONENT *pComponent)
{
	ASSERTX_RETURN( pComponent, "Expected compoent" );
	UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );	
	int nMoney = c_GetMoneyValueInComponent( pCompMoneyYours ) + 1;
	if (sSetTradeMoney( nMoney, FALSE ) == TRUE)
	{
		
		// start event for repeat money increase
		sStartRepeatMoney( REPEAT_MONEY_INCREASE );

	}
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeMoneyOnDecrease( 
	UI_COMPONENT *pComponent)
{
	ASSERTX_RETURN( pComponent, "Expected compoent" );
	UI_COMPONENT *pCompMoneyYours = UIComponentGetByEnum( UICOMP_TRADE_EDIT_MONEY_YOURS );	
	int nMoney = c_GetMoneyValueInComponent( pCompMoneyYours ) - 1;
	if (sSetTradeMoney( nMoney, FALSE ) == TRUE)
	{
		
		// start event for repeat money decrease
		sStartRepeatMoney( REPEAT_MONEY_DECREASE );

	}
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TradeError(
	UNITID idTradingWith,
	enum TRADE_ERROR eError)
{	
	// KCK: Simple errors handled first
	if (eError == TE_PLAYER_IS_TRIAL_ONLY)
		c_TradeRequestOtherIsTrialUser();
	else if (eError == TE_PLAYER_REJECTED_OFFER)
		c_TradeRequestOtherRejected();
	else if (eError == TE_PLAYER_BUSY)
		c_TradeRequestOtherBusy();

	ASSERTX_RETURN( idTradingWith != INVALID_ID, "Expected trading with unit id" );
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "Expected game" );
	
	// get who we were trading with
	WCHAR uszTradingWith[ MAX_CHARACTER_NAME ];
	UNIT *pTradingWith = UnitGetById( pGame, idTradingWith );
	if (pTradingWith)
	{
		UnitGetName( pTradingWith, uszTradingWith, MAX_CHARACTER_NAME, 0 );
	}
	else
	{
		PStrCopy( uszTradingWith, GlobalStringGet( GS_UNKNOWN ), MAX_CHARACTER_NAME );	
	}

	// get the reason message
	const WCHAR *puszReason = NULL;
	switch (eError)
	{
		//----------------------------------------------------------------------------
		case TE_UNABLE_TO_RECEIVE_ITEMS:
		{
			puszReason = GlobalStringGet( GS_TRADE_ERROR_UNABLE_TO_RECEIVE_ITEMS );
			break;
		}
		//----------------------------------------------------------------------------
		default:
		{
			puszReason = GlobalStringGet( GS_UNKNOWN );
			break;
		}
	}
	
	// construct final message
	WCHAR uszMessage[ MAX_STRING_ENTRY_LENGTH ];
	PStrCopy( uszMessage, GlobalStringGet( GS_TRADE_ERROR ), MAX_STRING_ENTRY_LENGTH );
	
	// replace the portions of the error message
	const WCHAR *puszTokenReason = StringReplacementTokensGet( SR_REASON );
	const WCHAR *puszTokenPlayerName = StringReplacementTokensGet( SR_PLAYER_NAME );
	PStrReplaceToken( uszMessage, MAX_STRING_ENTRY_LENGTH, puszTokenReason, puszReason );
	PStrReplaceToken( uszMessage, MAX_STRING_ENTRY_LENGTH, puszTokenPlayerName, uszTradingWith );
	
	// display message to user
	UIShowGenericDialog( GlobalStringGet( GS_ERROR ), uszMessage );
		
}
	
#endif // !ISVERSION(SERVER_VERSION)
