//----------------------------------------------------------------------------
// FILE: s_store.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __S_STORE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __S_STORE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;
struct TRADE_SPEC;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )

void s_StoreEnter(
	UNIT *pPlayer,
	UNIT *pMerchant,
	const TRADE_SPEC &tTradeSpec);

void s_StoreExit(
	UNIT *pPlayer,
	UNIT *pMerchant);
	
void s_StoreInit(
	UNIT *pUnit);

void s_StoreTryExpireOldItems( 
	UNIT *pPlayer,
	BOOL bForce);

BOOL s_StoreBuyItem(	
	UNIT *pPlayer,
	UNIT *&pItem,
	UNIT *pMerchant,
	const INVENTORY_LOCATION &tInvLocSuggested);

BOOL s_StoreSellItem(
	UNIT *pPlayer,
	UNIT *pItem,
	UNIT *pMerchant);

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

BOOL StoreMerchantWillBuyItem(	
	UNIT *pSellToUnit,
	UNIT *pItem,
	int *pnLocation = NULL);

enum STAT_VERSION_RESULT s_VersionStatMerchantLastBrowsedTick(
	struct STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit);
	
#endif  // end !ISVERSION( CLIENT_ONLY_VERSION )