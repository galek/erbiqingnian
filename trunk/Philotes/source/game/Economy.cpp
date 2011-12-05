//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "economy.h"
#include "excel.h"
#include "items.h"
#include "unit_priv.h"
#include "GameMaps.h"
#include "Script.h"
#include "skills.h"
#include "recipe.h"
#include "gameglobals.h"
#include "gamevariant.h"

#define MAX_BUY_PRICE				50000000	// amt vendor will sell item for
#define MAX_SELL_PRICE				1000000		// amt vendor will give you for item

//----------------------------------------------------------------------------
// HELPER EQUATIONS FUNCTIONS - I would love to move these to scripts
//----------------------------------------------------------------------------


inline float EconomyItemSellMult( UNIT *pItem, int nLevel )
{
	return (float)nLevel * .25f;
}

inline float EconomyItemSellMultRealWorld( UNIT *pItem )
{
	return 0.0f;
}


inline float EconomyItemSellMultPVP( UNIT *pItem )
{
	return 0.0f;
}


inline float EconomyMapPrice( UNIT *pItem, float defaultPrice )
{
	//get base price
	float defaultValue = defaultPrice;
	//this needs to be put inside of an excel table
	int nMin = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( MYTHOS_MAPS::GetLevelAreaID( pItem ) );
	int nMax = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( MYTHOS_MAPS::GetLevelAreaID( pItem ) );
	int nMaxPartySize = MYTHOS_LEVELAREAS::LevelAreaGetMinPartySizeRecommended( MYTHOS_MAPS::GetLevelAreaID( pItem ) );
	int nSpread = nMax - nMin;
	defaultValue += nMin * (MAP_PRICE_MULTIPLIER + (nMin + nMin));
	if( nSpread > 0 )
	{
		defaultValue += (nSpread - 1) * ( MAP_PRICE_MULTIPLIER + (nMax + nMax) );
	}
	if( nMaxPartySize > 1 ) //epic
	{
		defaultValue *= MAP_PRICE_MULTIPLIER_EPIC;
	}
	return defaultValue;
}

inline float EconomyItemBasePrice( UNIT *pItem )
{
	const UNIT_DATA *pData = UnitGetData(pItem);
	ASSERT_RETVAL( pData, 1.0f );
	float returnValue( 1.0f );
	if (pData)
	{
		returnValue = (float)pData->nBaseCost;
	}
	if( AppIsTugboat() )
	{
		if( MYTHOS_MAPS::IsMapSpawner( pItem ) )
		{
			return (float)EconomyMapPrice( pItem, returnValue );
		}	
		if( UnitIsA( pItem, UNITTYPE_RECIPE ) )
		{
			int nRecipeID = ItemGetRecipeID( pItem );
			if( nRecipeID >= 0 )
			{
				const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeID );
				ASSERT_RETVAL( pRecipe, returnValue );
				return (float)pRecipe->nBaseCost;
			}
		}	
	}
	return returnValue;
}

inline float EconomyItemPVPPriceBase( UNIT *pItem )
{
	const UNIT_DATA *pData = UnitGetData(pItem);
	ASSERT_RETVAL( pData, .0f );
	if (pData)
	{
		return (float)pData->nPVPPointCost;
	}
	return .0f;

}

inline float EconomyItemBasePriceRealWorld( UNIT *pItem )
{
	const UNIT_DATA *pData = UnitGetData(pItem);
	ASSERT_RETVAL( pData, .0f );
	if (pData)
	{
		return (float)pData->nRealWorldCost;
	}
	return .0f;
}

inline float EconomyMultByEconomy( UNIT *pItem )
{
	return 1.0f;
}
inline float EconomySocketPriceMult( UNIT *pItem )
{
	// sockets are expensive!
	return ( ItemGetModSlots( pItem ) * .5f ) + 1; 
}
inline float EconomyQualityPriceMult( UNIT *pItem )
{
	int nQuality = ItemGetQuality( pItem );
	BOOL bIdentified = ItemIsIdentified( pItem );
	if (nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData( nQuality );
		if (quality_data && quality_data->fPriceMultiplier > 0.0f && bIdentified)
		{
			return quality_data->fPriceMultiplier;
		}
	}
	return 1.0f;
}


inline int EconomyAffixPriceMultAndPriceAdd( GAME *pGame, UNIT *pItem, int item_level, float &priceMult, float &priceAdd )
{
	float basePrice = EconomyItemBasePrice( pItem );
	int AffixCount = 0;
	priceMult = 100.0f;
	if( ItemIsIdentified( pItem ) )
	{
		STATS * modlist = NULL;
		STATS * nextlist = StatsGetModList(pItem, SELECTOR_AFFIX);
		while ((modlist = nextlist) != NULL)
		{
			AffixCount++;
			nextlist = StatsGetModNext(modlist);

			int affix = StatsGetStat(modlist, STATS_AFFIX_ID);
			const AFFIX_DATA * affixdata = AffixGetData(affix);
			ASSERT_CONTINUE(affixdata);

			if (affixdata->codeBuyPriceMult != NULL_CODE)
			{
				int codelen = 0;
				BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceMult, &codelen);
				if (code)
				{
					priceMult += (float)VMExecI(pGame, pItem, item_level, (int)basePrice, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
				}
			}
			if (affixdata->codeBuyPriceAdd != NULL_CODE)
			{
				int codelen = 0;
				BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceAdd, &codelen);
				if (code)
				{
					priceAdd += (float)VMExecI(pGame, pItem, item_level, (int)basePrice, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
				}
			}
		}
	}
	else
	{
		// get all of our attached affixes
		STATS_ENTRY tStatsListAffixes[ MAX_AFFIX_ARRAY_PER_UNIT ];
		AffixCount = UnitGetStatsRange(
			pItem, 
			STATS_ATTACHED_AFFIX_HIDDEN, 
			0, 
			tStatsListAffixes, 
			AffixGetMaxAffixCount());
		
		// apply all the affixes
		for (int i = 0; i < AffixCount; ++i)
		{
			
			int affix = tStatsListAffixes[ i ].value;
			if( affix != INVALID_ID )
			{
				const AFFIX_DATA * affixdata = AffixGetData(affix);

				ASSERT_CONTINUE(affixdata);

				if (affixdata->codeBuyPriceMult != NULL_CODE)
				{
					int codelen = 0;
					BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceMult, &codelen);
					if (code)
					{
						priceMult += (float)VMExecI(pGame, pItem, item_level, (int)basePrice, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
					}
				}
				if (affixdata->codeBuyPriceAdd != NULL_CODE)
				{
					int codelen = 0;
					BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceAdd, &codelen);
					if (code)
					{
						priceAdd += (float)VMExecI(pGame, pItem, item_level, (int)basePrice, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
					}
				}
			}
		}
	}

	priceMult /= 100.0f; //this needs to be devided by 100.0f
	return AffixCount;
}


//----------------------------------------------------------------------------
// END HELPER FUNCTIONS
//----------------------------------------------------------------------------


inline float EconomyItemsValueInGame( GAME *pGame,
								  UNIT *pItem,
								  int nItemLevel,
								  BOOL bForceIdentified =FALSE)
{	
	
	float fPrice = EconomyItemBasePrice( pItem );					
	if( !ItemIsUnidentified( pItem ) ||
		bForceIdentified )
	{
		
		fPrice *= EconomySocketPriceMult( pItem );		
		fPrice *= EconomyQualityPriceMult( pItem );
		float priceAdd( 0 );
		float priceMult( 0.0f );
		EconomyAffixPriceMultAndPriceAdd( pGame, pItem, nItemLevel, priceMult, priceAdd );
		fPrice *= priceMult;
		fPrice += priceAdd;			
	}
	if ( fPrice < 1.0f ) 
		fPrice = 1.0f;
	fPrice *= ItemGetQuantity(pItem);
	fPrice *= EconomyMultByEconomy( pItem );
	return fPrice;
}

inline float EconomyItemValueRealWorld( UNIT *pItem )
{
	float fPrice = EconomyItemBasePriceRealWorld( pItem );
	fPrice *= ItemGetQuantity(pItem);
	fPrice *= EconomyMultByEconomy( pItem );
	return fPrice;
}

inline float EconomyItemValuePVPPrice( UNIT *pItem )
{
	float fPrice = EconomyItemPVPPriceBase( pItem );
	fPrice *= ItemGetQuantity(pItem);
	fPrice *= EconomyMultByEconomy( pItem );
	return fPrice;

}


inline cCurrency EconomyItemBuyPriceTugboat( GAME *pGame,
										 UNIT *pItem,
										 BOOL bForceIdentified = FALSE )
{
	float priceInGame = EconomyItemsValueInGame( pGame, pItem, 1, bForceIdentified);
	float priceInWorld = EconomyItemValueRealWorld( pItem );
	float priceInPVPPoints = EconomyItemValuePVPPrice( pItem );
	return cCurrency( (int)priceInGame, (int)priceInWorld, (int)priceInPVPPoints );
}

inline cCurrency EconomyItemSellPriceTugboat( GAME *pGame,
										 UNIT *pItem )
{
	float priceInGame = EconomyItemsValueInGame( pGame, pItem, 1 );
	float priceInWorld = EconomyItemValueRealWorld( pItem );
	float priceInPVP = EconomyItemValuePVPPrice( pItem );
	priceInGame *= EconomyItemSellMult( pItem, 1 );
	UNIT* pOwner = UnitGetContainer( pItem );
	if ( pOwner && UnitIsA( pOwner, UNITTYPE_PLAYER ) && PlayerIsElite( pOwner ) )
	{
		if( AppIsHellgate() )
		{
			priceInGame += PCT( (int)priceInGame, ELITE_ITEM_SELL_PRICE_PERCENT );
		}
		else
		{
			priceInGame += PCT( (int)priceInGame, MYTHOS_ELITE_ITEM_SELL_PRICE_PERCENT );
		}
	}
	priceInWorld *= EconomyItemSellMultRealWorld( pItem );
	priceInPVP *= EconomyItemSellMultPVP( pItem );
	//lets modify the price if it's crafted!	
	if( ItemIsACraftedItem( pItem ) )
	{
		
		BOOL atLeastOne( priceInGame > 0 );
		priceInGame *= MYTHOS_CRAFTED_ITEM_SELL_MULT;
		if( atLeastOne && priceInGame <= 0 )
			priceInGame = 1;
		priceInWorld *= MYTHOS_CRAFTED_ITEM_SELL_MULT;
		priceInPVP *= MYTHOS_CRAFTED_ITEM_SELL_MULT;

	}
	return cCurrency( (int)priceInGame, (int)priceInWorld, (int)priceInPVP );
}


inline cCurrency EconomyItemBuyPriceHellgate( GAME *pGame,
										  UNIT *pItem )
{	
	int item_level = UnitGetExperienceLevel(pItem);	
	// get base price
	int base_price = 0;
	const ITEM_LEVEL * item_lvl_data = ItemLevelGetData(pGame, item_level);
	if (item_lvl_data && item_lvl_data->codeBuyPriceBase != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEM_LEVELS, item_lvl_data->codeBuyPriceBase, &codelen);
		if (code)
		{
			base_price = VMExecI(pGame, pItem, item_level, 0, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	// get multiplier from item class
	int multiplier = 100;

	const UNIT_DATA * item_data = UnitGetData(pItem);
	if (item_data && item_data->codeBuyPriceMult != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEMS, item_data->codeBuyPriceMult, &codelen);
		if (code)
		{
			multiplier = VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	// get adder from item class
	int adder = 0;

	if (item_data && item_data->codeBuyPriceAdd != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEMS, item_data->codeBuyPriceAdd, &codelen);
		if (code)
		{
			adder = VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	// add in multipliers and adders from affixes
	int affix_mult = 0;
	int affix_add = 0;

	STATS * modlist = NULL;
	STATS * nextlist = StatsGetModList(pItem, SELECTOR_AFFIX);
	while ((modlist = nextlist) != NULL)
	{
		nextlist = StatsGetModNext(modlist);

		int affix = StatsGetStat(modlist, STATS_AFFIX_ID);
		const AFFIX_DATA * affixdata = AffixGetData(affix);
		ASSERT_CONTINUE(affixdata);

		if (affixdata->codeBuyPriceMult != NULL_CODE)
		{
			int codelen = 0;
			BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceMult, &codelen);
			if (code)
			{
				affix_mult += VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
			}
		}
		if (affixdata->codeBuyPriceAdd != NULL_CODE)
		{
			int codelen = 0;
			BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeBuyPriceAdd, &codelen);
			if (code)
			{
				affix_add += VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
			}
		}
	}
	multiplier += affix_mult;
	adder += affix_add;

	int price = PCT(base_price, multiplier) + adder;
	price *= ItemGetQuantity(pItem);
	price = PIN(price, 0, MAX_BUY_PRICE);

	cCurrency addToPrice;
	UNIT *pContainedItem = NULL;
	while ((pContainedItem = UnitInventoryIterate(pItem, pContainedItem)) != NULL)
	{
		addToPrice += EconomyItemBuyPrice( pContainedItem);
	}	
	price += addToPrice.GetValue( KCURRENCY_VALUE_INGAME );
		
	return cCurrency( price, 0 );
}

cCurrency EconomyItemSellPriceHellgate( GAME *pGame, UNIT *pItem )
{
	int base_price = 0;
	int multiplier = 100;
	int adder = 0;

	int item_level = UnitGetExperienceLevel(pItem);

	// get base price
	const ITEM_LEVEL * item_lvl_data = ItemLevelGetData( pGame, item_level);
	if (item_lvl_data && item_lvl_data->codeSellPriceBase != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEM_LEVELS, item_lvl_data->codeSellPriceBase, &codelen);
		if (code)
		{
			base_price = VMExecI(pGame, pItem, item_level, 0, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	const UNIT_DATA * item_data = UnitGetData(pItem);

	// get multiplier from item class
	if (item_data && item_data->codeSellPriceMult != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEMS, item_data->codeSellPriceMult, &codelen);
		if (code)
		{
			multiplier = VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	// get adder from item class
	if (item_data && item_data->codeSellPriceAdd != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEMS, item_data->codeSellPriceAdd, &codelen);
		if (code)
		{
			adder = VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
		}
	}

	// add in multipliers and adders from affixes
	int affix_mult = 0;
	int affix_add = 0;

	STATS * modlist = NULL;
	STATS * nextlist = StatsGetModList(pItem, SELECTOR_AFFIX);
	while ((modlist = nextlist) != NULL)
	{
		nextlist = StatsGetModNext(modlist);

		int affix = StatsGetStat(modlist, STATS_AFFIX_ID);
		const AFFIX_DATA * affixdata = AffixGetData(affix);
		if (affixdata == NULL) {
			continue;
		}

		if (affixdata->codeSellPriceMult != NULL_CODE)
		{
			int codelen = 0;
			BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeSellPriceMult, &codelen);
			if (code)
			{
				affix_mult += VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
			}
		}
		if (affixdata->codeSellPriceAdd != NULL_CODE)
		{
			int codelen = 0;
			BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_AFFIXES, affixdata->codeSellPriceAdd, &codelen);
			if (code)
			{
				affix_add += VMExecI(pGame, pItem, item_level, base_price, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);	
			}
		}
	}

	multiplier += affix_mult;
	adder += affix_add;

	int price = PCT(base_price, multiplier) + adder;

	// special case for recipes
	if (UnitIsA( pItem, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		// get recipe results
		RECIPE_INSTANCE_RESULTS tResults;
		RecipeInstanceGetResults( pItem, &tResults );
					
		// get price of each result
		cCurrency resultsCurrency;
		for (int i = 0; i < tResults.nNumResults; ++i)
		{
			UNIT *pResult = UnitGetById( pGame, tResults.idResult[ i ] );
			if (pResult)
			{
				resultsCurrency += EconomyItemBuyPrice( pResult );
			}
		}
		int nResultsPrice = resultsCurrency.GetValue( KCURRENCY_VALUE_INGAME );

		// modify the total price by the sell price mult of the recipe
		
		price = PCT(nResultsPrice, multiplier) + adder;
	}
	else
	{
		cCurrency resultsCurrency;
		UNIT *pContainedItem = NULL;
		while ((pContainedItem = UnitInventoryIterate(pItem, pContainedItem)) != NULL)
		{
			resultsCurrency += EconomyItemBuyPrice(pContainedItem);
		}
		price += resultsCurrency.GetValue( KCURRENCY_VALUE_INGAME );
	}

	price *= ItemGetQuantity(pItem);
	UNIT* pOwner = UnitGetContainer( pItem );
	if ( pOwner && UnitIsA( pOwner, UNITTYPE_PLAYER ) && PlayerIsElite( pOwner ) )
	{
		if( AppIsHellgate() )
		{
			price += PCT( price, ELITE_ITEM_SELL_PRICE_PERCENT );
		}
		else
		{
			price += PCT( price, MYTHOS_ELITE_ITEM_SELL_PRICE_PERCENT );
		}
	}
	price = PIN(price, 0, MAX_SELL_PRICE);
	return cCurrency( price, 0 );
}



cCurrency EconomyItemBuyPrice( UNIT *pItem,
								UNIT *pMerchant,
								BOOL bForceIdentified /*=FALSE*/,
								BOOL bIgnoreMerchantCheck /*=FALSE*/)
{

	
	if (!pItem)
	{
		return 0;
	}
	GAME * game = UnitGetGame(pItem);
	ASSERT_RETZERO(game);

	if (!pMerchant && ItemBelongsToAnyMerchant(pItem) == FALSE && !bIgnoreMerchantCheck)
	{
		return 0;
	}		
	
	int nLocation = INVLOC_NONE;
	if (UnitGetInventoryLocation(pItem, &nLocation) && nLocation == INVLOC_MERCHANT_BUYBACK)
	{
		return EconomyItemSellPrice(pItem);
	}

	// check for explicit buy price set by server ( for gambling prices, as an example )
	int nPriceFromServer = UnitGetStat( pItem, STATS_BUY_PRICE_OVERRIDE );
	if (nPriceFromServer != NONE)
	{
		return cCurrency( nPriceFromServer, 0 );
	}

	if( AppIsHellgate() )
	{
		return EconomyItemBuyPriceHellgate( game, pItem );
	}
	else
	{
		return EconomyItemBuyPriceTugboat( game, pItem, bForceIdentified );
	}		
}

cCurrency EconomyItemSellPrice( UNIT *pItem )
{

	if (!pItem)
	{
		return cCurrency( 0, 0 ); 
	}
	
	GAME * game = UnitGetGame(pItem);
	ASSERT_RETZERO(game);
	
	// check for explicit sell price set by server
	int nPriceFromServer = UnitGetStat( pItem, STATS_SELL_PRICE_OVERRIDE );
	if (nPriceFromServer != NONE)
	{
		return cCurrency( nPriceFromServer, 0 );
	}
	
	if( AppIsHellgate() )
	{
		return EconomyItemSellPriceHellgate( game, pItem );
	}
	else
	{
		return EconomyItemSellPriceTugboat( game, pItem );
	}		
	
}
