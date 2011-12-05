//----------------------------------------------------------------------------
// FILE: s_recipe.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "s_recipe.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "picker.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "recipe.h"
#include "gameunits.h"
#include "trade.h"
#include "treasure.h"
#include "gameglobals.h"
#include "Currency.h"
#include "s_quests.h"
#include "achievements.h"
#include "skills.h"
#include "recipe.h"
#include "s_crafting.h"
//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum RECIPE_COMPOENT_TYPE
{
	RCT_INGREDIENT,
	RCT_RESULT
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddRecipeComponent(
	UNIT *pUnit,
	int nInvLoc,
	UNIT *pComponent,
	int nRecipe,
	int nIndexInRecipeList,
	RECIPE_COMPOENT_TYPE eType,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nInvLoc != INVLOC_NONE, "Expected inventory location" );
	ASSERTX_RETFALSE( pComponent, "Expected component unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver unit" );
	UNITID idRecipeGiver = UnitGetId( pRecipeGiver );
	
	// set stat linking to this recipe
	UnitSetStat( pComponent, STATS_RECIPE_COMPONENT, nRecipe );
	UnitSetStat( pComponent, STATS_RECIPE_COMPONENT_INDEX, nIndexInRecipeList );
	UnitSetStat( pComponent, STATS_RECIPE_GIVER_UNIT_ID, idRecipeGiver );

	// for ingredients, mark them as such
	if (eType == RCT_INGREDIENT)
	{
		UnitSetStat( pComponent, STATS_RECIPE_INGREDIENT, nRecipe );
	}
	else if (eType == RCT_RESULT)
	{
		UnitSetStat( pComponent, STATS_RECIPE_RESULT, nRecipe );
	}
							
	// put in inventory
	if (UnitInventoryAdd(INV_CMD_PICKUP, pUnit, pComponent, nInvLoc) == FALSE)
	{
		UnitFree( pComponent );
	}

	// this item is now ready to do network communication if the unit it's a part of is ready
	if (UnitCanSendMessages( pUnit ))
	{
	
		// set can send messages
		UnitSetCanSendMessages( pComponent, TRUE );
	
		// tell clients about the item
		GAME *pGame = UnitGetGame( pUnit );
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			if (ClientCanKnowUnit( pClient, pComponent ))
			{
				s_SendUnitToClient( pComponent, pClient );
			}
			
		}		

	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCreateRecipeIngredients(
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver,
	int nHighestResultItemQuality)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)		//
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	const RECIPE_DEFINITION *ptRecipeDef = RecipeGetDefinition( nRecipe );
	ASSERTX_RETFALSE( ptRecipeDef, "Expected recipe definition" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	
	// go through all ingredients	
	int nAllRecipesIngredientsLoc = RecipeGetIngredientInvLoc( pUnit );
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		const RECIPE_INGREDIENT_DEFINITION *pIngredientDef = &ptRecipeDef->tIngredientDef[ i ];
		ASSERT_CONTINUE(pIngredientDef);
		
		int nItemClass = pIngredientDef->nItemClass;
		if (nItemClass != INVALID_LINK)
		{
		
			// figure out the quantity needed
			int nQuantityNeeded = pIngredientDef->nQuantityMin;
			
			// add some randomness
			int nAdditionalCount = pIngredientDef->nQuantityMax - nQuantityNeeded;
			nQuantityNeeded += RandGetNum( pGame, 0, nAdditionalCount );
			
			// multiply the quantity by any quality modifier of the result
			if (nHighestResultItemQuality != INVALID_LINK &&
				ptRecipeDef->bResultQualityModifiesIngredientQuantity == TRUE)
			{
				const ITEM_QUALITY_DATA *pQualityData = ItemQualityGetData( nHighestResultItemQuality );
				if (pQualityData && pQualityData->fRecipeQuantityMultiplier != 1.0f)
				{
					nQuantityNeeded = (int)(nQuantityNeeded * pQualityData->fRecipeQuantityMultiplier);
				}
			}
			
			// create item
			if (nQuantityNeeded > 0)
			{
					
				ITEM_SPEC tItemSpec;
				tItemSpec.nItemClass = nItemClass;
				tItemSpec.nQuality = pIngredientDef->nItemQuality;
				SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );  // always identify item
				UNIT *pItem = s_ItemSpawn( pGame, tItemSpec, NULL );
				ASSERTX_RETFALSE( pItem, "Could not create recipe ingredient" );
							
				// set the quantity of the item needed
				ItemSetQuantity( pItem, nQuantityNeeded );
				
				// add to components
				if (sAddRecipeComponent( 
						pUnit, 
						nAllRecipesIngredientsLoc, 
						pItem, 
						nRecipe, 
						nIndexInRecipeList,
						RCT_INGREDIENT, 
						pRecipeGiver) == FALSE)
				{
					return FALSE;
				}

			}
						
		}		
		
	}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)	
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sCreateCraftResult(
	UNIT *pReceiver,
	const RECIPE_DEFINITION *pRecipeDef,
	int nResultIndex,
	UNIT *pRecipeGiver)
{				
	ASSERTX_RETNULL( pReceiver, "Expected receiver unit" );
	ASSERTX_RETNULL( pRecipeDef, "Expected recipe definition" );
	ASSERTX_RETNULL( nResultIndex >= 0 && nResultIndex < MAX_RECIPE_RESULTS, "Invalid result index" );
	ASSERTX_RETNULL( pRecipeGiver, "Expected recipe giver" );	
		
	// try explict items classes
	UNIT *pReward = NULL;
#if !ISVERSION(CLIENT_ONLY_VERSION)		//
	GAME *pGame = UnitGetGame( pRecipeGiver );	// CHB 2007.03.16 - Moved to silence compiler warning.
	int nItemClass = pRecipeDef->nRewardItemClass[ nResultIndex ];
	if (nItemClass != INVALID_LINK)
	{
	
		// init spawn spec
		ITEM_SPEC tSpec;
		SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
		if( AppIsTugboat() )
		{
			SETBIT( tSpec.dwFlags, ISF_AFFIX_SPEC_ONLY_BIT );			//Tugboat wants crafted items not to have any random affixs
			SETBIT( tSpec.dwFlags, ISF_USE_SPEC_LEVEL_ONLY_BIT );		//Tugboat wants the items to spaw at the correct level
			SETBIT( tSpec.dwFlags, ISF_SKIP_QUALITY_AFFIXES_BIT );		//Tugboat wants NO random affixs based off of quality
			SETBIT( tSpec.dwFlags, ISF_ZERO_OUT_SOCKETS );				//Tugboat wants NO sockets on crafted items
			SETBIT( tSpec.dwFlags, ISF_IS_CRAFTED_ITEM );				//Tugboat wants NO sockets on crafted items
		}
		tSpec.nItemClass = pRecipeDef->nRewardItemClass[ nResultIndex ];
		tSpec.pSpawner = pRecipeGiver;
		if (AppIsTugboat())
		{
			SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
			tSpec.guidRestrictedTo = UnitGetGuid( pReceiver );
		}
		pReward = s_ItemSpawn( pGame, tSpec, NULL );			

		const UNIT_DATA *pItemData = ItemGetData( nItemClass );
		REF( pItemData );
		ASSERTV_RETNULL( 
			pReward,
			"Recipe '%s' unable to create specific result item '%s'",
			pRecipeDef->szName,
			pItemData->szName);
	}
	else
	{
		
		// try treasure
		int nTreasure = pRecipeDef->nTreasureClassResult[ nResultIndex ];
		if (nTreasure != INVALID_LINK)
		{
		
			// setup spawn spec
			ITEM_SPEC tSpec;
			tSpec.unitOperator = pReceiver;
			if (AppIsTugboat())
			{
				SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
				tSpec.guidRestrictedTo = UnitGetGuid( pReceiver );
			}			
			SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );

			// setup result data
			const int MAX_RESULT_ITEMS = 1;
			UNIT *tItems[ MAX_RESULT_ITEMS ];
			ITEMSPAWN_RESULT tResult;
			tResult.pTreasureBuffer = tItems;
			tResult.nTreasureBufferSize = arrsize( tItems );			

			// create item
			//s_TreasureSpawnClass( pRecipeGiver, nTreasure, &tSpec, &tResult);
			s_TreasureSpawnClass( pRecipeGiver, nTreasure, &tSpec, &tResult, UnitGetStat(pReceiver, STATS_DIFFICULTY_CURRENT)/*, UnitGetStat(pRecipeGiver, STATS_LEVEL)*/ );
			if (tResult.nTreasureBufferCount == 1)
			{
				pReward = tResult.pTreasureBuffer[ 0 ];
			}
			else
			{
				const TREASURE_DATA *pTreasureData = TreasureGetData( pGame, nTreasure );
				REF( pTreasureData );
				ASSERTV( 
					tResult.nTreasureBufferCount == 0, 
					"Recipe '%s' creates multiple results using treasure '%s', which is not currently supported",
					pRecipeDef->szName,
					pTreasureData->szTreasure);
							
				// we don't support being able to craft multiple things, but maybe one day we will
				for (int i = 0; i < tResult.nTreasureBufferCount; ++i)
				{
					UnitFree( tResult.pTreasureBuffer[ i ] );
				}

			}
		
		}
		
	}
#endif
	return pReward;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCreateRecipeResults(
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver,
	int *pnItemQualityHighest)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)		//
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pnItemQualityHighest, "Expected quality storage" );
	const RECIPE_DEFINITION *ptRecipeDef = RecipeGetDefinition( nRecipe );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	
	// init quality
	*pnItemQualityHighest = INVALID_LINK;
	
	// go through all results
	UNIT *pUnitWithResults = RecipeGetComponentUnit( pUnit, pRecipeGiver );
	int nAllRecipesResltsLoc = RecipeGetResultsInvLoc( pUnitWithResults );
	BOOL bCreatedResults = FALSE;
	for (int i = 0; i < MAX_RECIPE_RESULTS; ++i)
	{
		UNIT *pItem = sCreateCraftResult( pUnit, ptRecipeDef, i, pRecipeGiver );
		if (pItem)
		{
					
			// add to components
			if (sAddRecipeComponent( 
					pUnit, 
					nAllRecipesResltsLoc, 
					pItem, 
					nRecipe, 
					nIndexInRecipeList,
					RCT_RESULT, 
					pRecipeGiver) == FALSE)
			{
				return FALSE;
			}
		
			// we've created something	
			bCreatedResults = TRUE;
			
			// keep track of the highest item quality
			int nItemQuality = ItemGetQuality( pItem );
			if (ItemQualityIsBetterThan( nItemQuality, *pnItemQualityHighest ))
			{
				*pnItemQualityHighest = nItemQuality;
			}
			
		}	
			
	}
	return bCreatedResults;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDestroyRecipeComponents( 
	UNIT *pUnit, 
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver unit" );
	
	// destroy all items in inventory linked to this recipe
	UNIT *pItemNext = NULL;
	UNIT *pItem = UnitInventoryIterate( pUnit, NULL );
	while (pItem)
	{
		
		// get next item
		pItemNext = UnitInventoryIterate( pUnit, pItem );
		
		// free item if party of this recipe
		if (RecipeItemIsComponent( pItem, nRecipe, nIndexInRecipeList, pRecipeGiver ))
		{
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );
		}
		
		// goto next item
		pItem = pItemNext;
		
	}

	return TRUE;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGiveResult( 
	UNIT *pPlayer, 
	UNIT *pItem,
	int nRecipe)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pItem, "Expected item unit" );
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe" );
	const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( nRecipe );
	ASSERTX_RETURN( pRecipeDef, "Expected recipe definition" );
	
	// to make it easy, just clear all recipe stats we could have used
	UnitClearStat( pItem, STATS_RECIPE_COMPONENT, 0 );
	UnitClearStat( pItem, STATS_RECIPE_COMPONENT_INDEX, 0 );	
	UnitClearStat( pItem, STATS_RECIPE_INGREDIENT, 0 );
	UnitClearStat( pItem, STATS_RECIPE_RESULT, 0 );
	UnitClearStat( pItem, STATS_RECIPE_GIVER_UNIT_ID, 0 );

	// where will we put the result
	int nInvSlotResult = pRecipeDef->nInvLocIngredients;
	ASSERTX_RETURN( nInvSlotResult != INVALID_LINK, "Expected slot to put instanced recipe results in" );

	// since we use the item crafting in order to give some quest recipe items, we have to 
	// let the server be able to override the check that doesn't allow players to put no drop
	// items into the crafting inventory grids

	// find a location for the item	
	BOOL bGiven = FALSE;
	int nX, nY;			
	if (UnitInventoryFindSpace( pPlayer, pItem, nInvSlotResult, &nX, &nY, CLF_IGNORE_NO_DROP_GRID_BIT ))
	{
		bGiven = InventoryChangeLocation( pPlayer, pItem, nInvSlotResult, nX, nY, CLF_IGNORE_NO_DROP_GRID_BIT );
	}

	if (bGiven)
	{
		s_QuestEventItemCrafted( pPlayer, pItem );
	}
	else	
	{

		// restrict for this player
		UnitSetRestrictedToGUID( pItem, UnitGetGuid( pPlayer ) );
	
		// can't find spot in inventory, drop to ground and lock for this player
		// drop item to ground, but lock to this player so nobody can take it
		DROP_RESULT eResult = s_ItemDrop( pPlayer, pItem );
		if( eResult != DR_OK_DESTROYED )  // this never should be but just in case
		{
			// lock the item to this player
			ItemLockForPlayer( pItem, pPlayer );
		}
		
	}

	// inform the achievements system
	s_AchievementsSendItemCraft(pItem, pPlayer);

	GAME * pGame = UnitGetGame( pItem );
	ASSERTX_RETURN( pGame, "Expected game" );
	UnitTriggerEvent( pGame, EVENT_ITEM_CRAFT, pItem, pPlayer, NULL );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasAnyRecipeComponents( 
	UNIT *pPlayer, 
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );

	// iterate the trade offer 
	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryIterate( pPlayer, pItem )) != NULL)
	{
		if (RecipeItemIsComponent( pItem, nRecipe, nIndexInRecipeList, pRecipeGiver ))
		{
			return TRUE;
		}		
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCreateRecipeComponents( 
	UNIT *pUnit, 
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver" );
	
	// if we have any items for this recipe already, we've already done this so don't do it again
	if (sHasAnyRecipeComponents( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver ) == TRUE)
	{
		return TRUE;
	}

	// create results
	int nItemQualityHighest = INVALID_LINK;
	if (sCreateRecipeResults( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver, &nItemQualityHighest ) == FALSE)
	{
		return FALSE;
	}
		
	// create ingredients
	if (sCreateRecipeIngredients( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver, nItemQualityHighest ) == FALSE)
	{
		return FALSE;
	}
	
	return TRUE;
#else
	return TRUE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}	

//----------------------------------------------------------------------------
enum RECIPE_RESULTS
{
	RECIPE_RESULTS_GIVEN,
	RECIPE_RESULTS_NOT_FOUND
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static RECIPE_RESULTS sGiveInstancedResults(
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETVAL( pPlayer, RECIPE_RESULTS_NOT_FOUND, "Expected player unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), RECIPE_RESULTS_NOT_FOUND, "Expected player unit" );
	ASSERTX_RETVAL( nRecipe != INVALID_LINK, RECIPE_RESULTS_NOT_FOUND, "Expected recipe link" );
	ASSERTX_RETVAL( pRecipeGiver, RECIPE_RESULTS_NOT_FOUND, "Expected recipe giver" );

	// what is the focus unit to take the result from
	UNIT *pUnitWithResults = RecipeGetComponentUnit( pPlayer, pRecipeGiver );
	
	// get inventory slot for recipe results
	int nAllRecipesResltsLoc = RecipeGetResultsInvLoc( pUnitWithResults );
	
	// go through inventory
	BOOL bGaveResults = FALSE;
	UNIT* pItem = NULL;
	pItem = UnitInventoryLocationIterate( pUnitWithResults, nAllRecipesResltsLoc, pItem, 0, FALSE );
	while ( pItem != NULL )
	{
	
		// get next item, cause we will move the result
		UNIT *pNext = UnitInventoryLocationIterate( pUnitWithResults, nAllRecipesResltsLoc, pItem, 0, FALSE );
		
		// is this recipe result for the recipe in question
		if (RecipeItemIsComponent( pItem, nRecipe, nIndexInRecipeList, pRecipeGiver ))
		{
		
			// give to player
			sGiveResult( pPlayer, pItem, nRecipe );
			bGaveResults = TRUE;
		}
		
		// next item in this inv slot
		pItem = pNext;
		
	}		

	// see if we gave anything	
	if (bGaveResults)
	{

		// this recipe has been "used up", recreate results for next time around
		if (UnitIsA( pRecipeGiver, UNITTYPE_SINGLE_USE_RECIPE ) == FALSE)
		{
		
			// destroy this recipes components
			sDestroyRecipeComponents( pUnitWithResults, nRecipe, nIndexInRecipeList, pRecipeGiver );
			
			// create a new recipe to take it's place
			if (sCreateRecipeComponents( pUnitWithResults, nRecipe, nIndexInRecipeList, pRecipeGiver ) == FALSE)
			{
				// nope, couldn't do it, forget it
				sDestroyRecipeComponents( pUnitWithResults, nRecipe, nIndexInRecipeList, pRecipeGiver );
			}
			
		}

		// inform client 
		MSG_SCMD_RECIPE_CREATED tMessage;
		tMessage.nRecipe = nRecipe;
		GAME *pGame = UnitGetGame( pPlayer );
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		s_SendMessage( pGame, idClient, SCMD_RECIPE_CREATED, &tMessage );
	
		return RECIPE_RESULTS_GIVEN;
		
	}
	else
	{
		return RECIPE_RESULTS_NOT_FOUND;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRemoveIngredient(
	UNIT *pItem,
	int nQuantityLeftToRemove)
{		
	ASSERTX_RETZERO( pItem, "Expected item" );
	ASSERTX_RETZERO( nQuantityLeftToRemove > 0, "Invalid quantity to remove" );
					
	// how many of the item do we have
	int nQuantity = ItemGetQuantity( pItem );
	ASSERTX_RETZERO( nQuantity > 0, "Item has invalid quantity" );

	// how much will we actually remove this time
	int nQuantityToRemove = MIN( nQuantityLeftToRemove, nQuantity );
	nQuantityLeftToRemove -= nQuantityToRemove;
	
	// either set new quantity or destroy item
	if (nQuantity == nQuantityToRemove)
	{
		UnitFree( pItem, UFF_SEND_TO_CLIENTS );
	}
	else
	{
		ASSERTX( nQuantity > nQuantityToRemove, "Item quantity should be larger than the quantity we're removing" );
		ItemSetQuantity( pItem, nQuantity - nQuantityToRemove );
	}
	
	return nQuantityLeftToRemove;
	
}

//----------------------------------------------------------------------------
//remove items we are asking for
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static BOOL sRecipeRemoveItemsForTugboat( UNIT *pUnit,
										  int nRecipe )
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	
	const RECIPE_DEFINITION *pRecipeDef =  RecipeGetDefinition( nRecipe );
	ASSERTX_RETFALSE( pRecipeDef, "Expected Recipe" );
	BOOL bRemovedItems( FALSE );
	// go through all the collect items
	for ( int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i )
	{
		const RECIPE_INGREDIENT_DEFINITION nIngredient = pRecipeDef->tIngredientDef[ i ];

		if (nIngredient.nItemClass != INVALID_LINK)
		{			
			int numberOfItemsToRemove = nIngredient.nQuantityMin;
			if( numberOfItemsToRemove > 0)
			{

				// find one of these items in the player inventory ... we will only check the
				// on person inventory of the player, but we will also ignore equipable slots
				DWORD dwInvIterateFlags = 0;
				SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
				SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

				UNIT* pItem = NULL;
				UNIT* pNextItem = NULL;
				pItem = UnitInventoryIterate( pUnit, NULL, dwInvIterateFlags );
				while (pItem != NULL)
				{
					pNextItem = UnitInventoryIterate( pUnit, pItem, dwInvIterateFlags );
					if (RecipeUnitMatchesIngredientTugboat( pItem, &nIngredient ))
					{
						numberOfItemsToRemove = sRemoveIngredient( pItem, numberOfItemsToRemove );
						bRemovedItems = TRUE;
						if (numberOfItemsToRemove == 0)
						{
							break;
						}							
					}	
					pItem = pNextItem;
				}		
			}
		}
	}
	return bRemovedItems;
}

static void sRecipeRemoveItems( 
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	//const RECIPE_DEFINITION *pRecipeDef =  RecipeGetDefinition( nRecipe );

	// get ingredients necessary for this recipe instance
	RECIPE_INSTANCE_INGREDIENTS tInstanceIngredients;
	RecipeInstanceGetIngredients( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver, &tInstanceIngredients );
	
	// go through all the collect items
	for ( int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i )
	{
		const RECIPE_INGREDIENT *pIngredient = &tInstanceIngredients.tIngredient[ i ];

		if (pIngredient->nItemClass != INVALID_LINK)
		{			
			int numberOfItemsToRemove = pIngredient->nQuantity;
			if( numberOfItemsToRemove > 0)
			{

				// find one of these items in the player inventory ... we will only check the
				// on person inventory of the player, but we will also ignore equipable slots
				DWORD dwInvIterateFlags = 0;
				SETBIT( dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
				SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

				UNIT* pItem = NULL;
				UNIT* pNextItem = NULL;
				pItem = UnitInventoryIterate( pUnit, NULL, dwInvIterateFlags );
				while (pItem != NULL)
				{
					pNextItem = UnitInventoryIterate( pUnit, pItem, dwInvIterateFlags );
					if (RecipeUnitMatchesIngredient( pItem, pIngredient ))
					{
						numberOfItemsToRemove = sRemoveIngredient( pItem, numberOfItemsToRemove );
						if (numberOfItemsToRemove == 0)
						{
							break;
						}							
					}	
					pItem = pNextItem;
				}		
			}
		}
	}

	//// for recipes that we needed to put items in a special slot to craft, return any leftovers
	//// to the regular inventory
	//if (pRecipeDef->nInvLocIngredients != INVALID_LINK)
	//{
	//	int nInvLocIngredients = pRecipeDef->nInvLocIngredients;
	//	s_ItemsRestoreToStandardLocations( pUnit, nInvLocIngredients );
	//}
		
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sClearCurrentRecipeStats(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	
	UnitClearStat( pPlayer, STATS_RECIPE_CURRENT, 0 );
	UnitClearStat( pPlayer, STATS_RECIPE_CURRENT_INDEX, 0 );
	UnitClearStat( pPlayer, STATS_RECIPE_CURRENT_GIVER_UNIT_ID, 0 );
	
}
#endif

//----------------------------------------------------------------------------
//this completes a recipe and gives player the resulting reward
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
inline UNIT * sRecipeCreateItemFromRecipe( UNIT *pUnit,
										 UNIT *pRecipeGiver,
										 const RECIPE_DEFINITION *pRecipeDef )
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	ASSERTX_RETNULL( pRecipeDef, "Expected recipe" );
	UNIT *pCreatedItem( NULL );
	// no pre-created instanced rewards to give, create all new ones
	for ( int i = 0; i < MAX_RECIPE_RESULTS; ++i ) 
	{
		//make sure it's valid, if not we aren't awarding any more
		if ( pRecipeDef->nRewardItemClass[ i ] != INVALID_ID )		
		{
			UNIT *pReward = sCreateCraftResult( pUnit, pRecipeDef, i, pRecipeGiver );
			if( pReward )
			{
				pCreatedItem = pReward;
				// this item is now ready to do network communication
				UnitSetCanSendMessages( pReward, TRUE );					

				// put in world (yeah, kinda lame, but it keeps the pickup code path clean),
				// the unit was restricted to this pOwner GUID, so at least no other players
				// are getting new unit messages for it
				UnitWarp( 
					pReward,
					UnitGetRoom( pUnit ), 
					UnitGetPosition( pUnit ),
					cgvYAxis,
					cgvZAxis,
					0);

				// pick it up
				s_ItemPickup( pUnit, pReward );

				// event for crafted item
				s_QuestEventItemCrafted( pUnit, pReward );

			}			
			else
			{
				break;
			}
		}			
	}
	return pCreatedItem;
}
static void sRecipeComplete( 
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver unit" );
	const RECIPE_DEFINITION *pRecipeDef =  RecipeGetDefinition( nRecipe );
	ASSERTX_RETURN( pRecipeDef, "No recipe def");

	// first take all the stuff away for the recipe
	sRecipeRemoveItems( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver );
					
	int xp = pRecipeDef->nExperience;
	s_UnitGiveExperience( pUnit, xp );

	int gold = pRecipeDef->nGold;
	cCurrency unitCurrency = UnitGetCurrency( pUnit );	
	unitCurrency += cCurrency( gold,  0 );
	//gold += UnitGetStat( pUnit, STATS_GOLD );
	//UnitSetStat( pUnit, STATS_GOLD, gold ); //award gold

	// see if we already have items pre-created to give
	if (sGiveInstancedResults( pUnit, nRecipe, nIndexInRecipeList, pRecipeGiver ) != RECIPE_RESULTS_GIVEN)
	{
		//actualy create the items
		sRecipeCreateItemFromRecipe( pUnit, pRecipeGiver, pRecipeDef );
		
	}
	
	// clear current recipe stats
	sClearCurrentRecipeStats( pUnit );
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCreateRecipeListComponents( 
	UNIT *pPlayer, 
	int nRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nRecipeList != INVALID_LINK, "Expected recipe list link" );
	
	// go through all recipes
	int nRecipeCount = 0;
	const RECIPELIST_DEFINITION *ptRecipeListDef = RecipeListGetDefinition( nRecipeList );	
	for (int i = 0; i < MAX_RECIPES; ++i)
	{
		int nRecipe = ptRecipeListDef->nRecipes[ i ];
		if (nRecipe != INVALID_LINK)
		{
			if (sCreateRecipeComponents( pPlayer, nRecipe, i, pRecipeGiver ) == FALSE)
			{
				sDestroyRecipeComponents( pPlayer, nRecipe, i, pRecipeGiver );
			}
			else
			{
				nRecipeCount++;
			}
		}		
	}
	return nRecipeCount;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDestroyRecipeListComponents( 
	UNIT *pPlayer, 
	int nRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nRecipeList != INVALID_LINK, "Expected recipe list link" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver unit" );
	
	// go through all recipes
	const RECIPELIST_DEFINITION *ptRecipeListDef = RecipeListGetDefinition( nRecipeList );	
	for (int i = 0; i < MAX_RECIPES; ++i)
	{
		int nRecipe = ptRecipeListDef->nRecipes[ i ];
		if (nRecipe != INVALID_LINK)
		{
			sDestroyRecipeComponents( pPlayer, nRecipe, i, pRecipeGiver );
		}		
	}
	return TRUE;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSelectRandomRecipeList(
	GAME *pGame)
{

	// start picker
	PickerStart( pGame, tPicker );
		
	// go through all rows
	int nNumChoices = 0;
	int nNumRecipeLists = ExcelGetNumRows( pGame, DATATABLE_RECIPELISTS );
	for (int i = 0; i < nNumRecipeLists; ++i)
	{
		const RECIPELIST_DEFINITION *pRecipeListDef = RecipeListGetDefinition( i );
		if (pRecipeListDef)
		{
			if (pRecipeListDef->bRandomlySelectable == TRUE)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pRecipeListDef->nRandomSelectWeight));
				nNumChoices++;		
			}
		}
	}

	// select one
	if (nNumChoices)
	{
		return PickerChoose( pGame );
	}

	return INVALID_LINK;
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_RecipeTradesmanGetCurrentRecipeList(
	UNIT *pPlayer,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( pRecipeGiver, "Expected NPC" );
	
	// get recipe currently assigned via stat
	int nRecipeGiverClass = UnitGetClass( pRecipeGiver );
	int nRecipeList = UnitGetStat( pPlayer, STATS_CRAFTSMAN_RECIPE_LIST_CURRENT, nRecipeGiverClass );
	
	// if no list, use the one in unit data
	if (nRecipeList == INVALID_LINK)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pRecipeGiver );
		nRecipeList = pUnitData->nRecipeList;
	}
	
	// if still no list, select one that can be randomly selected 
	if (nRecipeList == INVALID_LINK)
	{
		GAME *pGame = UnitGetGame( pPlayer );
		nRecipeList = sSelectRandomRecipeList( pGame );
	}

	// save the current recipe list selected
	if (nRecipeList != INVALID_LINK)
	{
		UnitSetStat( pPlayer, STATS_CRAFTSMAN_RECIPE_LIST_CURRENT, nRecipeGiverClass, nRecipeList );
	}
	
	return nRecipeList;
		
}

//----------------------------------------------------------------------------
#define RECIPE_TIME_MASK (0x0000FFFF)  // we only need this amount of precision for refresh time, plus it needs to be small to store the tick in a stat

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTryExpireRecipes(
	UNIT *pPlayer,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );
	DWORD dwNow = GameGetTick( pGame );
	DWORD dwNowLowBits = dwNow & RECIPE_TIME_MASK;
	int nRecipeGiverClass = UnitGetClass( pRecipeGiver );
			
	// when did this player last browse any personal craftsman
	DWORD dwLastBrowsedTick = UnitGetStat( pPlayer, STATS_PERSONAL_CRAFTSMAN_LAST_BROWSED_TICK, nRecipeGiverClass );

	// has it been long enough
	DWORD dwRefreshTimeInTicks = GAME_TICKS_PER_SECOND * SECONDS_PER_MINUTE * CRAFTSMAN_REFRESH_TIME_IN_MINUTES;
	if (dwNowLowBits - dwLastBrowsedTick >= dwRefreshTimeInTicks || dwLastBrowsedTick == 0)
	{
	
		// destroy existing recipe list
		int nRecipeList = s_RecipeTradesmanGetCurrentRecipeList( pPlayer, pRecipeGiver );
		sDestroyRecipeListComponents( pPlayer, nRecipeList, pRecipeGiver );
		
		// clear out the recipe list that was in use so that another one will be selected when we ask for it
		UnitClearStat( pPlayer, STATS_CRAFTSMAN_RECIPE_LIST_CURRENT, nRecipeGiverClass );
		
	}

	// return the recipe list now in use
	return s_RecipeTradesmanGetCurrentRecipeList( pPlayer, pRecipeGiver );
	
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT_INTERACT s_RecipeTradesmanInteract(
	UNIT *pPlayer,
	UNIT *pNPC)
{
	ASSERTX_RETVAL( pPlayer, UNIT_INTERACT_NONE, "Invalid player" );
	ASSERTX_RETVAL( pNPC, UNIT_INTERACT_NONE, "Invalid npc" );
	GAME *pGame = UnitGetGame( pPlayer );

	// get the current recipe of this tradesman
	int nRecipeList = s_RecipeTradesmanGetCurrentRecipeList( pPlayer, pNPC );				
	if (nRecipeList != INVALID_LINK)
	{	
		
		// set flag that players are trading
		UnitSetStat( pPlayer, STATS_TRADING, UnitGetId( pNPC ) );

		if (RecipeGiverInstancesComponents( pNPC ))
		{
		
			// see if it's time to refresh the content of personal craftsman
			nRecipeList = sTryExpireRecipes( pPlayer, pNPC );

			// create components for all recipes in this recipe list			
			if (sCreateRecipeListComponents( pPlayer, nRecipeList, pNPC ) == 0)
			{
			
				// whoops, something is broken, destroy all items we've created so far
				sDestroyRecipeListComponents( pPlayer, nRecipeList, pNPC );
				
				// forget it
				return UNIT_INTERACT_NONE;
				
			}

			// has now browsed this craftsman
			DWORD dwNow = GameGetTick( pGame );
			DWORD dwNowLowBits = dwNow & RECIPE_TIME_MASK;			
			int nRecipeGiverClass = UnitGetClass( pNPC );
			UnitSetStat( pPlayer, STATS_PERSONAL_CRAFTSMAN_LAST_BROWSED_TICK, nRecipeGiverClass, dwNowLowBits );
		
		}
		else
		{
		
			// send the contents of the store to the client trading
			DWORD dwFlags = 0;
			GAMECLIENT *pClient = UnitGetClient( pPlayer );
			ASSERTX( pClient, "Player entering store has no client!" );
			s_InventorySendToClient( pNPC, pClient, dwFlags );		
			
		}

		// open recipe list
		s_RecipeListOpen( pPlayer, nRecipeList, pNPC );
		return UNIT_INTERACT_TALK;
		
	}

	return UNIT_INTERACT_NONE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRecipeIsInUnitRecipeList(
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe,
	int nIndexInRecipeList)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( nIndexInRecipeList >= 0 && nIndexInRecipeList < MAX_RECIPES, "Invalid recipe list index" );
	
	const UNIT_DATA *pUnitData = UnitGetData( pRecipeGiver );
	ASSERTX_RETFALSE( pUnitData, "Expected unitdata" );
	int nRecipeList = s_RecipeTradesmanGetCurrentRecipeList( pPlayer, pRecipeGiver );
	const RECIPELIST_DEFINITION *pRecipeListDef = RecipeListGetDefinition( nRecipeList );
	ASSERTX_RETFALSE(pRecipeListDef, "No recipelist definition" );
	if (pRecipeListDef->nRecipes[ nIndexInRecipeList ] == nRecipe)
	{
		return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRecipeIsAvailable(	
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe,
	int nIndexInRecipeList = INVALID_INDEX)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );

	// if giver is a one time recipe, must match that recipe
	if (UnitIsA( pRecipeGiver, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		
		// must be owned by player
		if (UnitGetUltimateOwner( pRecipeGiver ) != pPlayer)
		{
			return FALSE;
		}
		
		// recipe must be in a location that the player can take from, this is a generally 
		// good thing to check, but specifically fixes the exploit of people trying 
		// to create recipes from blueprints that are in their personal merchant stores
		if (ItemIsInCanTakeLocation( pRecipeGiver ) == FALSE)
		{
			return FALSE;
		}
		
		// check that this single use recipe matches the type we think it should
		int nRecipeSingleUse = UnitGetStat( pRecipeGiver, STATS_RECIPE_SINGLE_USE );
		return nRecipeSingleUse == nRecipe;
		
	}
	
	// check recipe list
	if (sRecipeIsInUnitRecipeList( pPlayer, pRecipeGiver, nRecipe, nIndexInRecipeList ))
	{
		return TRUE;
	}	

	return FALSE;
	
}


//----------------------------------------------------------------------------
//returns TRUE if the recipe failed
//----------------------------------------------------------------------------
BOOL sRecipeShouldFailDueToPlayerAbility( UNIT *pPlayer,
									      int nRecipe )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nRecipe != INVALID_ID );
	GAME *pGame = UnitGetGame( pPlayer );
	
	float fPercentChanceOfFailure = RecipeGetFailureChance( pPlayer, nRecipe );
	return (fPercentChanceOfFailure > RandGetFloat( pGame, 0, 1.0f ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void s_sRecipeStampCreatorsName( UNIT *pItem,
									    UNIT *pPlayer )
{
	ASSERT_RETURN( pItem );
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );

	// get player name
	WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
	PStrCopy( uszPlayerName, L"", arrsize(uszPlayerName) );
	UnitGetName( pPlayer, uszPlayerName, arrsize(uszPlayerName), 0 );
	int nPlayerNameSize = PStrLen( uszPlayerName );
	for( int t = 0; t < nPlayerNameSize; t++ )
	{
		UnitSetStat( pItem, STATS_CRAFTED_ITEM_CREATOR, t, (int)uszPlayerName[ t ] );
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeCreate( 
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe,
	int nIndexInRecipeList)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	if( AppIsHellgate() )
	{
		ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	}
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );


	if( AppIsHellgate() )
	{
		// recipe must be available
		if (sRecipeIsAvailable( pPlayer, pRecipeGiver, nRecipe, nIndexInRecipeList ))
		{
			UNITID idRecipeGiver = UnitGetId( pRecipeGiver );
			if (RecipeIsComplete( pPlayer, nRecipe, nIndexInRecipeList, idRecipeGiver, FALSE ))
			{
			
	#if !ISVERSION( CLIENT_ONLY_VERSION )			
				// try to take your stuff and complete the recipe
				sRecipeComplete( pPlayer, nRecipe, nIndexInRecipeList, pRecipeGiver );
				
				// destroy the recipe if it's a single use recipe
				if (UnitIsA( pRecipeGiver, UNITTYPE_SINGLE_USE_RECIPE ))
				{
					int nQuantity = ItemGetQuantity( pRecipeGiver );
					if (nQuantity <= 1)
					{
						UnitFree( pRecipeGiver, UFF_SEND_TO_CLIENTS );
					}
					else
					{
						ItemSetQuantity( pRecipeGiver, nQuantity - 1 );
					}
					
				}
	#endif	

			}

		}
	}
	else if( AppIsTugboat() )  //create recipe, create item
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )
		//we have to be able to create the recipe
		ASSERT_RETURN( RecipePlayerCanCreateRecipe( pPlayer, nRecipe, FALSE ) );
		//gets all the presets for the recipe and saves it on the player.
		RecipeFillOutRecipePropertyValues( pPlayer, RecipeGetDefinition( nRecipe ) );

		UNIT *pUnitCreated( NULL );
		//now lets check to see if the player has chanced it to much and the recipe fails to create anything
		if( RecipeCheatingEnabled() ||
			!sRecipeShouldFailDueToPlayerAbility( pPlayer, nRecipe ) )
		{			
			//pass in the player here because he is the creator of the item being made
			pUnitCreated = sRecipeCreateItemFromRecipe( pPlayer, pPlayer, RecipeGetDefinition( nRecipe ));

		}
		// try to take your stuff and complete the recipe		
		if( !RecipeCheatingEnabled() )
		{
			if( !sRecipeRemoveItemsForTugboat( pPlayer, nRecipe ) )
			{
				UnitFree( pUnitCreated, UFF_SEND_TO_CLIENTS );
			}		
		}
		//ok Item is created, lets add all the magical crafting stats we need
		if( pUnitCreated )
		{
			RecipeModifyCreatedItemByCraftingTreeInvestment( pPlayer, pUnitCreated, nRecipe );
		}
		//Send attempt to achievement system
		

		//send message to client it's working great!
		// setup message
		// inform client 
		MSG_SCMD_RECIPE_CREATED tMessage;
		tMessage.nRecipe = nRecipe;
		if( pUnitCreated != NULL )
		{
			//stamp the users name on the item
			if( CRAFTING::CraftingItemCanHaveCreatorsName( pUnitCreated ) )
			{
				s_sRecipeStampCreatorsName( pUnitCreated, pPlayer );
			}

			tMessage.bSuccess = 1;
			s_ItemResend( pUnitCreated );			//resend the item	
			CRAFTING::s_CraftingPlayerRegisterNewMod( pPlayer, pUnitCreated );	//register new unit
		}
		else
		{
			tMessage.bSuccess = 0;
		}
		s_AchievementsSendItemCraft(pUnitCreated,pPlayer,tMessage.bSuccess);

		GAME *pGame = UnitGetGame( pPlayer );
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		s_SendMessage( pGame, idClient, SCMD_RECIPE_CREATED, &tMessage );

		
#endif
	}
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeBuy( 
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe )
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );


#if !ISVERSION( CLIENT_ONLY_VERSION )
	const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( nRecipe );
	ASSERT_RETURN(pRecipeDef);
	cCurrency cost( pRecipeDef->nBaseCost, 0 );

	if ( UnitGetCurrency( pPlayer ) < cost ||
		PlayerHasRecipe( pPlayer, nRecipe ))
	{
		return;
	}
	PlayerLearnRecipe( pPlayer, nRecipe );	

	UnitRemoveCurrencyValidated( pPlayer, cost );


#endif

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeIngredientInventoryReturn(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// move all inventory items in the recipe ingredients panel back to the player inventory
	int nRecipeLoc = GlobalIndexGet( GI_INVENTORY_LOCATION_RECIPE_INGREDIENTS );		
	s_ItemsRestoreToStandardLocations( pUnit, nRecipeLoc );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeSendForceClosed( 
	UNIT *pPlayer,
	UNIT *pRecipe)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pRecipe, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pRecipe, UNITTYPE_SINGLE_USE_RECIPE ), "Expected single use recipe" );

	// setup message
	MSG_SCMD_RECIPE_FORCE_CLOSED tMessage;
	tMessage.idRecipeGiver = UnitGetId( pRecipe );
	
	// send to client
	s_SendUnitMessage( pPlayer, SCMD_RECIPE_FORCE_CLOSED, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeForceClose(
	UNIT *pPlayer,
	UNIT *pRecipe)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pRecipe, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pRecipe, UNITTYPE_SINGLE_USE_RECIPE ), "Expected single use recipe" );
	
	// cancel any recipe
	s_RecipeClose( pPlayer );

	// tell player to close UI	
	sRecipeSendForceClosed( pPlayer, pRecipe );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeClose(
	UNIT* pPlayer)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// who is this player trading with
	UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
	if (pTradingWith && pTradingWith != pPlayer && UnitIsTradesman( pTradingWith ))
	{
		UnitClearStat( pPlayer, STATS_TRADING, 0 );		
	}
										
	//// move all inventory items in the recipe ingredients panel back to the player inventory
	//sRecipeIngredientInventoryReturn( pPlayer );
	//
	// clear the current recipe we're looking at
	sClearCurrentRecipeStats( pPlayer );

	//// return any items to standard locations that are still in the recipe interface	
	//s_ItemsReturnToStandardLocations( pPlayer, FALSE );
	
	
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RecipeSplitIngredientToLocation( 
	UNIT *pPlayer, 
	UNIT *pItem, 
	const INVENTORY_LOCATION &tInvLocation)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pItem, "Expected item" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// get the current recipe the player is looking at
	int nRecipe = UnitGetStat( pPlayer, STATS_RECIPE_CURRENT );
	int nIndexInRecipeList = UnitGetStat( pPlayer, STATS_RECIPE_CURRENT_INDEX );
	UNITID idGiver = UnitGetStat( pPlayer, STATS_RECIPE_CURRENT_GIVER_UNIT_ID );
	UNIT *pRecipeGiver = UnitGetById( pGame, idGiver );
	if (nRecipe != INVALID_LINK && pRecipeGiver != NULL)
	{
	
		// how many is this item stack
		int nQuantity = ItemGetQuantity( pItem );

		// get ingredient instance
		RECIPE_INSTANCE_INGREDIENTS tInstanceIngredients;
		RecipeInstanceGetIngredients( pPlayer, nRecipe, nIndexInRecipeList, pRecipeGiver, &tInstanceIngredients );
		
		// go through ingredients
		for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
		{
			const RECIPE_INGREDIENT *pIngredient = &tInstanceIngredients.tIngredient[ i ];
			if (pIngredient->nItemClass != INVALID_LINK)
			{
				if (RecipeUnitMatchesIngredient( pItem, pIngredient ))
				{
					
					// how many of these are already available to for cooking
					int nCurrentCount = RecipeGetAvailableIngredientCount( pPlayer, nRecipe, pIngredient, FALSE );
					
					// how many more are needed
					int nNumNeeded = pIngredient->nQuantity - nCurrentCount;
					if (nNumNeeded > 0)
					{
						
						// see if we can split exactly what we need
						int nNumToSplit = MIN( nQuantity, nNumNeeded );
						if (nNumToSplit != nQuantity)
						{
						
							// split the item
							UNIT *pItemSplit = UnitInventoryRemove( pItem, ITEM_SINGLE );
																										
							// put in inventory
							if (UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pItemSplit, tInvLocation.nInvLocation, tInvLocation.nX, tInvLocation.nY))
							{
							
								// set the new quantity on both items
								ItemSetQuantity( pItemSplit, nNumToSplit );
								ItemSetQuantity( pItem, nQuantity - nNumToSplit );
														
								// this item is now ready to do network communication
								UnitSetCanSendMessages( pItemSplit, TRUE );
							
								// tell clients about the item
								GAME *pGame = UnitGetGame( pPlayer );
								for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
									 pClient;
									 pClient = ClientGetNextInGame( pClient ))
								{
									if (ClientCanKnowUnit( pClient, pItemSplit ))
									{
										s_SendUnitToClient( pItemSplit, pClient );
									}
									
								}		
							
								// find a spot for the leftovers
								int nInvLoc;
								int nX;
								int nY;
								if (ItemGetOpenLocation( pPlayer, pItem, TRUE, &nInvLoc, &nX, &nY ))
								{
									InventoryChangeLocation( pPlayer, pItem, nInvLoc, nX, nY, 0 );
								}
								
								// we've done a split now
								return TRUE;
							
							}
							else
							{
								UnitFree( pItem );							
							}
																					
						}
						
					}
					
				}
				
			}
			
		}
		
	}
	
	return FALSE;  // did not split
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeSelect( 
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nIndexInRecipeList >= 0 && nIndexInRecipeList < MAX_RECIPES, "Invalid recipe index" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	
	// keep a stat of the current recipe the player is looking at
	UnitSetStat( pPlayer, STATS_RECIPE_CURRENT, nRecipe );
	UnitSetStat( pPlayer, STATS_RECIPE_CURRENT_INDEX, nIndexInRecipeList );
	UnitSetStat( pPlayer, STATS_RECIPE_CURRENT_GIVER_UNIT_ID, UnitGetId( pRecipeGiver ) );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeOpen(
	UNIT *pPlayer,
	int nRecipe,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver unit" );

	// you cannot open a recipe if you're at a craftsman or trading (unless you're at a merchant, cause
	// we support that UI crazyness)
	UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
	if (pTradingWith && UnitIsMerchant( pTradingWith ) == FALSE)
	{
		return;
	}
		
	// we need to make sure the client knows about their stash so they know how many ingredients they have there
	s_sSendStashToClient( pPlayer );

	// setup message
	MSG_SCMD_RECIPE_OPEN tMessage;			
	tMessage.idRecipeGiver = UnitGetId( pRecipeGiver );
	tMessage.nRecipe = nRecipe;
			
	// send it
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_RECIPE_OPEN, &tMessage );	
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeListOpen(
	UNIT *pPlayer,
	int nRecipeList,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nRecipeList != INVALID_LINK, "Expected recipe list link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver unit" );

	// we need to make sure the client knows about their stash so they know how many ingredients they have there
	s_sSendStashToClient( pPlayer );

	// set giver
	MSG_SCMD_RECIPE_LIST_OPEN tMessage;			
	tMessage.idRecipeGiver = UnitGetId( pRecipeGiver );
	tMessage.nRecipeList = nRecipeList;
			
	// send it
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_RECIPE_LIST_OPEN, &tMessage );	

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetRandomRecipe(
	GAME *pGame)
{

	// start picker
	PickerStart( pGame, tPicker );

	// add all random choices to picker
	int nNumRecipes = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	int nNumChoices = 0;
	for (int i = 0; i < nNumRecipes; ++i)
	{
		const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( i );
		if (pRecipeDef)
		{
			if (pRecipeDef->bAllowInRandomSingleUse == TRUE)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pRecipeDef->nWeight));
				nNumChoices++;
			}		
		}
	}

	// select one
	if (nNumChoices)
	{
		return PickerChoose( pGame );
	}
	
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RecipeSingleUseInit(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	ASSERTX_RETFALSE( UnitIsA( pItem, UNITTYPE_SINGLE_USE_RECIPE ), "Expected recipe item" );

	// only setup if we don't have one yet	
	int nRecipe = UnitGetStat( pItem, STATS_RECIPE_SINGLE_USE );
	if (nRecipe == INVALID_LINK)
	{
		GAME *pGame = UnitGetGame( pItem );
		
		// select a recipe at random or use the specific one provided
		int nRecipe = INVALID_LINK;
		const UNIT_DATA *pUnitData = UnitGetData( pItem );
		if (pUnitData->nRecipeSingleUse != INVALID_LINK)
		{
			nRecipe = pUnitData->nRecipeSingleUse;
		}
		else
		{
			nRecipe = sGetRandomRecipe( pGame );
		}

		// no valid recipe
		if (nRecipe == INVALID_LINK)
		{
			return FALSE;
		}

		// save recipe in stat				
		UnitSetStat( pItem, STATS_RECIPE_SINGLE_USE, nRecipe );
		
		// create the recipe components as self contained in the recipe
		if (sCreateRecipeComponents( pItem, nRecipe, 0, pItem ) == FALSE)
		{
			sDestroyRecipeComponents( pItem, nRecipe, INVALID_INDEX, pItem );
			return FALSE;
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RecipeVersion(
	UNIT *pItem)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_SINGLE_USE_RECIPE ), "Expected single use recipe" );
	BOOL bDestroy = FALSE;
	
	// check for recipe that no longer exists
	int nRecipe = UnitGetStat( pItem, STATS_RECIPE_SINGLE_USE );
	if (nRecipe == INVALID_LINK)
	{
		bDestroy = TRUE;
	}

	// check for old recipes that we want to remove
	const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( nRecipe );
	if (pRecipeDef)
	{
		if (pRecipeDef->bRemoveOnLoad == TRUE)
		{
			bDestroy = TRUE;
		}
	}
	else
	{
		// recipe definition is no longer around for some reason (maybe a special event item)
		// so we're going to destroy it
		bDestroy = TRUE;
	}
	
	// destroy it if desired
	if (bDestroy == TRUE)
	{
		UnitFree( pItem );
	}
		
}	

//Assigns a specific recipe to a recipe item
BOOL s_RecipeAssignSpecificRecipeID( UNIT *pRecipeItem,
								     int nRecipeID )
{
	ASSERT_RETFALSE( pRecipeItem );
	int nRecipeCount = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_RECIPES );
	ASSERT_RETFALSE( nRecipeID >= 0 && nRecipeID < nRecipeCount );
	const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( nRecipeID );
	ASSERT_RETFALSE( pRecipeDefinition );
	//ASSERT_RETFALSE( TESTBIT( pRecipeDefinition->pdwFlags, RECIPE_FLAG_CAN_SPAWN ) );	
	UnitSetStat( pRecipeItem, STATS_RECIPE_ASSIGNED, nRecipeID );
	return TRUE;
}

//Assigns a random recipe based off the players level to the recipe item
BOOL s_RecipeAssignRandomRecipeID( UNIT *pUnit,
								   UNIT *pRecipeItem )
{
	ASSERT_RETFALSE( pRecipeItem && pUnit );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE( pGame );
	int nRecipeCount = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_RECIPES );
	PickerStart( pGame, picker );	
	int nUnitLevel = UnitGetStat( pUnit, STATS_LEVEL );
	for( int t =0; t < nRecipeCount; t++ )
	{
		const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( t );
		if( pRecipeDefinition &&
			TESTBIT( pRecipeDefinition->pdwFlags, RECIPE_FLAG_CAN_SPAWN ) &&
			( nUnitLevel >= pRecipeDefinition->nSpawnLevels[ 0 ]  &&
			  ( pRecipeDefinition->nSpawnLevels[ 1 ] == -1 ||
			    nUnitLevel <= pRecipeDefinition->nSpawnLevels[ 1 ] ) ) )
		{
			BOOL addToPicker( TRUE );
			if( pUnit )
			{
				for( int i = 0; i < MAX_RECIPE_UNITS_DROP_FROM; i++ )
				{
					if( pRecipeDefinition->nSpawnForUnittypes[ i ] != INVALID_ID &&
						!UnitIsA( pUnit, pRecipeDefinition->nSpawnForUnittypes[ i ] ) )
					{
						addToPicker = FALSE;
						break;
					}
					if( pRecipeDefinition->nSpawnForMonsterClass[ i ] != INVALID_ID &&
						!UnitGetClass( pUnit ) == pRecipeDefinition->nSpawnForMonsterClass[ i ] )
					{
						addToPicker = FALSE;
						break;
					}
				}
			}
			else
			{
				addToPicker = TRUE;
			}			
			if( addToPicker )
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, t, pRecipeDefinition->nWeight));
			}
		}
	}

	
	int nRecipeID = PickerChoose( pGame );
	ASSERT( nRecipeID != -1 );
	if( nRecipeID < 0 )
		nRecipeID = 100;	//hack
	if( nRecipeID != INVALID_ID )
	{
		s_RecipeAssignSpecificRecipeID( pRecipeItem, nRecipeID );
		return TRUE;
	}
	return FALSE;
}	

//Clears all players modified stats
void s_RecipeClearPlayerModifiedRecipeProperties( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	for( int t = 0; t < NUM_RECIPE_PROP; t++ )
	{
		s_RecipeSetPlayerModifiedRecipeProperty( pPlayer, t, 0 );
	}

}

//WHen the player adjusts his recipes chances this saves the values per property
void s_RecipeSetPlayerModifiedRecipeProperty( UNIT *pPlayer,
											 int nProperty,
											 int nValue )
{
	ASSERT_RETURN( pPlayer );
	UnitSetStat( pPlayer, STATS_RECIPE_PROP_PLAYER_INPUT, nProperty, nValue );
}