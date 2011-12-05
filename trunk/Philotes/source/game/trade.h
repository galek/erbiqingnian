//----------------------------------------------------------------------------
// FILE: trade.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __TRADE_H_
#define __TRADE_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UNIT;
struct GAME;
struct INTERACT_MENU_CHOICE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
#define TRADE_DISTANCE (8.0f)
#define TRADE_DISTANCE_SQ (TRADE_DISTANCE * TRADE_DISTANCE)

//----------------------------------------------------------------------------
enum TRADE_ERROR
{
	TE_INVALID = -1,
	
	TE_NONE,
	TE_UNABLE_TO_RECEIVE_ITEMS,
	TE_PLAYER_BUSY,
	TE_PLAYER_IS_TRIAL_ONLY,
	TE_PLAYER_REJECTED_OFFER,
	
	TE_NUM_ERRORS
	
};

//----------------------------------------------------------------------------
enum TRADE_STATUS
{
	TRADE_STATUS_NONE = -1,
	TRADE_STATUS_ACCEPT
};

//----------------------------------------------------------------------------
// FUNCTIONS PROTOTYPES
//----------------------------------------------------------------------------

void TradeInit(
	GAME *pGame);

void TradeFree(
	GAME *pGame);

BOOL TradeCanTrade(
	UNIT *pUnit);

BOOL TradeCanTradeWith(
	UNIT *pUnit,
	UNIT *pUnitToTradeWith,
	int *nOverrideTooltipId = NULL );
	
BOOL TradeIsTrading(
	UNIT *pPlayer);

BOOL TradeIsTradingWith(
	UNIT *pPlayer,
	UNIT *pTradingWith);

UNITID TradeGetTradingWithID(
	UNIT *pUnit);

UNIT *TradeGetTradingWith(
	UNIT *pUnit);

BOOL TradeCanTradeItem(
	UNIT *pPlayer,
	UNIT *pItem);
			
#endif  // end __TRADE_H_
