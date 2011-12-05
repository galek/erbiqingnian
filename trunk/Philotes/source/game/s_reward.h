//----------------------------------------------------------------------------
// FILE: s_reward.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_REWARD_H_
#define __S_REWARD_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __S_TRADE_H_
#include "s_trade.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UNIT;
enum INVENTORY_STYLE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum REWARD_CONSTANTS
{
	REWARD_TAKE_ALL = -1,
};

//----------------------------------------------------------------------------
struct TAKE_ITEM
{
	UNIT *pTaker;			// who took the item
	UNIT *pItem;			// the item	
	void *pUserData;		// callback param
	int nItemsLeftInSlot;	// # of items left in slot item was taken from
};

//----------------------------------------------------------------------------
typedef void (* PFN_TAKE_ITEM_CALLBACK)( TAKE_ITEM &tTakeItem );
typedef void (* PFN_ALL_ITEM_TAKEN_CALLBACK)( void *pUserData );

//----------------------------------------------------------------------------
void OfferActionsInit( 
	struct OFFER_ACTIONS &tActions);

//----------------------------------------------------------------------------
struct OFFER_ACTIONS
{
	OFFER_ACTIONS::OFFER_ACTIONS( void ) { OfferActionsInit( *this ); }
		
	PFN_TAKE_ITEM_CALLBACK pfnItemTaken;	// callback when taking single item
	void *pItemTakenUserData;				// param for callback

	PFN_ALL_ITEM_TAKEN_CALLBACK pfnAllTaken;// called when last item is taken
	void *pAllTakenUserData;				// param for callback
	
};

//----------------------------------------------------------------------------
struct SREWARD_SESSION 
{
	OFFER_ACTIONS tActions;
	TAKE_ITEM tTakeItem;
	int nNumTaken;				// number of rewards the player has taken so far
	int nMaxTakes;				// how many rewards the player can take
	int nMoneyOffer;			// money offered for player/player trades
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void s_RewardSessionInit(
	SREWARD_SESSION *pRewardSession);

SREWARD_SESSION *s_RewardSessionGet(
	UNIT *pUnit);

void s_RewardOpen(
	INVENTORY_STYLE eStyle,
	UNIT *pUnit,
	UNIT *pOfferer,
	UNIT **pRewards,
	int nNumRewards,
	int nOriginalMaxTakes,
	int nNumTaken,
	int nOfferDefinition,
	const OFFER_ACTIONS &tActions);

void s_RewardCancel(
	UNIT *pUnit);
	
void s_RewardClose(
	UNIT *pUnit);

void s_RewardTakeAll(
	UNIT *pUnit);

BOOL s_RewardCanTakeAll(
	UNIT *pUnit);

void s_RewardItemTaken(
	UNIT *pUnit,
	UNIT *pItem);

#endif