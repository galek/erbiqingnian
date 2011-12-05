//----------------------------------------------------------------------------
// FILE: c_trade.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __C_TRADE_H_
#define __C_TRADE_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __TRADE_H_
#include "trade.h"
#endif

#ifndef __CURRENCY_H_
#include "Currency.h"
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UNIT;
struct GAME;
struct UI_COMPONENT;
enum INVENTORY_STYLE;
struct TRADE_SPEC;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS PROTOTYPES
//----------------------------------------------------------------------------

void c_TradeInit(
	GAME *pGame);

void c_TradeFree(
	GAME *pGame);

void c_TradeStart(
	UNIT* pTradingWith,
	const TRADE_SPEC &tTradeSpec);

BOOL c_TradeCancel(
	BOOL bCloseUI = TRUE);

BOOL c_TradeForceCancel(
	void);

BOOL c_TradeIsTrading(
	void);

void c_TradeSetStatus(
	UNITID idTrader,
	TRADE_STATUS eStatus,
	int nTradeOfferVersion = NONE);

void c_TradeToggleAccept(
	void);

void c_TradeCannotAccept(
	void);

void c_TradeAskOtherToTrade(
	UNITID idToTradeWith);
	
void c_TradeBeingAskedToTrade(
	UNITID idToTradeWith);

void c_TradeRequestCancelledByAskingPlayer(
	UNITID idInitialRequester);
	
void c_TradeRequestOtherBusy(
	void);

void c_TradeRequestOtherRejected(
	void);

void c_TradeRequestOtherIsTrialUser(
	void);

void c_TradeRequestOtherNotFound(
	void);

void c_TradeMoneyModified(
	int nMoneyYours,
	int nMoneyTheirs,
	int nMoneyRealWorldYours,
	int nMoneyRealWorldTheirs );
	
void c_TradeMoneyOnChar( 
	UI_COMPONENT *pComponent,
	WCHAR ucCharacter);

void c_TradeMoneyOnIncrease( 
	UI_COMPONENT *pComponent);

void c_TradeMoneyOnDecrease( 
	UI_COMPONENT *pComponent);

void c_TradeError(
	UNITID idTradingWith,
	enum TRADE_ERROR eError);
	
int c_GetMoneyValueInComponent(
	UI_COMPONENT *pComponent);

void c_SetMoneyValueInComponent(
	UI_COMPONENT *pComp,
	int nMoney);
	
#endif  // end __C_TRADE_H_
