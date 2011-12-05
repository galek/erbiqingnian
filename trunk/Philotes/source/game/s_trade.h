//----------------------------------------------------------------------------
// FILE: s_trade.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_TRADE_H_
#define __S_TRADE_H_

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
struct TRADE_SPEC;
enum INVENTORY_STYLE;
struct STRADE_SESSION;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct STRADE_SESSION
{
	cCurrency nCurrency;
	int nTradeOfferVersion;
};

//----------------------------------------------------------------------------
// FUNCTIONS PROTOTYPES
//----------------------------------------------------------------------------
		
void s_TradeInit(
	GAME *pGame);

void s_TradeFree(
	GAME *pGame);

void s_TradeStart(
	UNIT* pPlayer,
	UNIT* pTradingWith,
	const TRADE_SPEC &tTradeSpec);

void s_TradeCancel(
	UNIT* pPlayer,
	BOOL bForce);
		
void s_TradeSetStatus(
	UNIT *pTrader,
	TRADE_STATUS eStatus,
	int nTradeOfferVersion);

void s_TradeOfferModified( 
	UNIT *pUnit);

void s_TradeRequestNew( 
	UNIT *pUnit, 
	UNIT *pUnitToTradeWith);

void s_TradeRequestNewCancel( 
	UNIT *pUnit);
	
void s_TradeRequestAccept(
	UNIT *pUnit,
	UNIT *pAskedToTradeBy);
	
void s_TradeRequestReject(
	UNIT *pUnit,
	UNIT *pAskedToTradeBy);

BOOL s_TradeIsAnyPlayerTradingWith(
	UNIT *pUnit);
	
void s_TradeModifyMoney( 
	UNIT *pUnit, 
	cCurrency &currencyToTrade );

void s_TradeSessionInit(
	STRADE_SESSION *pSession);
		
#endif  // end __S_TRADE_H_
