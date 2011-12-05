//----------------------------------------------------------------------------
// FILE: recipe.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "excel_private.h"
#include "recipe.h"
#include "inventory.h"
#include "items.h"
#include "skills.h"
#include "script.h"
#include "excel_common.h"
#include "uix.h"
#include "c_recipe.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

static BOOL bCheatingEnabled( FALSE );
BOOL RecipeToggleCheating()
{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	bCheatingEnabled = !bCheatingEnabled;
	if(bCheatingEnabled )
	{
		UIShowTaskCompleteMessage( L"Crafting Cheat Enabled" );
	}
	else
	{
		UIShowTaskCompleteMessage( L"Crafting Cheat Disabled" );		
	}
	return bCheatingEnabled;
#else
	return FALSE;
#endif
}

inline BOOL RecipeCheatingEnabled()
{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	return bCheatingEnabled;
#else
	return FALSE;
#endif
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const RECIPE_DEFINITION *RecipeGetDefinition(
	int nRecipe)
{
	return (const RECIPE_DEFINITION *)ExcelGetData( NULL, DATATABLE_RECIPES, nRecipe );
}

const RECIPE_PROPERTIES *RecipeGetPropertyDefinition(
	int nRecipePropertyID )
{
	return (const RECIPE_PROPERTIES *)ExcelGetData( NULL, DATATABLE_RECIPEPROPERTIES, nRecipePropertyID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const RECIPELIST_DEFINITION *RecipeListGetDefinition(
	int nRecipeList)
{
	return (const RECIPELIST_DEFINITION *)ExcelGetData( NULL, DATATABLE_RECIPELISTS, nRecipeList );
}

//----------------------------------------------------------------------------
//Returns the recipe category struct
//----------------------------------------------------------------------------
const RECIPE_CATEGORY *RecipeGetCategoryDefinition(
	int nRecipeCategoryID )
{
	return (const RECIPE_CATEGORY *)ExcelGetData( NULL, DATATABLE_RECIPECATEGORIES, nRecipeCategoryID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeDefinitionPostProcess( struct EXCEL_TABLE * table)
{	
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Index == DATATABLE_RECIPES);
	unsigned int nNumOfRecipes = ExcelGetCountPrivate(table);
	ASSERTX_RETFALSE(nNumOfRecipes != 0, "Number of recipes was zero");
	for( unsigned int t = 0; t < nNumOfRecipes; t++ )
	{

		RECIPE_DEFINITION * recipe = (RECIPE_DEFINITION *)ExcelGetDataPrivate(table, t);
		if( !recipe )
			return FALSE;
		for( int i = 0; i < NUM_RECIPE_PROP; i++ )
		{
			const RECIPE_PROPERTIES *pRecipeProp = RecipeGetPropertyDefinition( i );
			ASSERT_CONTINUE( pRecipeProp );
			recipe->nRecipePropertyEnabled[ i ] = FALSE;
			for( int c = 0; c < RECIPE_VALUE_COUNT_PER_PROPERTY; c++ )
			{
				recipe->nRecipeProperties[ i ][ c ] = pRecipeProp->nDefaultValues[c];
			}		
		}

		//lets set some properties on the excel data.
		if( recipe->codeProperties != NULL_CODE )
		{			
			int code_len( 0 );
			BYTE * code_ptr = ExcelGetScriptCode( EXCEL_CONTEXT(), DATATABLE_RECIPES, recipe->codeProperties, &code_len);
			ASSERTX_RETTRUE( code_ptr, "NO stats in skill attack script" );	
			VMExecI( NULL, (int)t, INVALID_ID, code_ptr, code_len, NULL );			
		}	
	}
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeDefinitionPostProcessRow(
	EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata)
{


	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeUnitMatchesIngredient(	
	UNIT *pUnit,
	const RECIPE_INGREDIENT *pIngredient)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pIngredient, "Expected ingredient" );

	// check for invalid ingredient
	if (pIngredient->nItemClass == INVALID_LINK)
	{
		return FALSE;
	}	
	
	// must be item	
	if (UnitGetGenus( pUnit ) != GENUS_ITEM)
	{
		return FALSE;
	}

	// must match class	
	if (UnitGetClass( pUnit ) != pIngredient->nItemClass)
	{
		return FALSE;
	}

	// must match quality
	if (pIngredient->nItemQuality != INVALID_LINK)
	{
		if (ItemGetQuality( pUnit ) != pIngredient->nItemQuality)
		{
			return FALSE;
		}
	}

	// note that we do *not* match quantity here, we're only concerned with "type"
	return TRUE;			
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BOOL RecipeFindIngredientMatchingItem(
	UNIT *pItem,
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeLilst,
	UNIT *pRecipeGiver,
	RECIPE_INGREDIENT *pIngredientToReturn)
{
	ASSERTX_RETNULL( pItem, "Expected item" );
	ASSERTX_RETNULL( pPlayer, "Expected player" );
	ASSERTX_RETNULL( nRecipe != INVALID_LINK, "Invalid recipe" );
	ASSERTX_RETNULL( pRecipeGiver, "Expected recipe giver" );
	ASSERTX_RETNULL( pIngredientToReturn, "Expected ingredient to return" );

	// get ingredients	
	RECIPE_INSTANCE_INGREDIENTS tInstanceIngredients;
	RecipeInstanceGetIngredients( pPlayer, nRecipe, nIndexInRecipeLilst, pRecipeGiver, &tInstanceIngredients );

	// check each one	
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		const RECIPE_INGREDIENT *pIngredient = &tInstanceIngredients.tIngredient[ i ];
		if (RecipeUnitMatchesIngredient( pItem, pIngredient ))
		{
			*pIngredientToReturn = *pIngredient;
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RecipeGetAvailableIngredientCount(
	UNIT *pPlayer,
	int nRecipe,
	const RECIPE_INGREDIENT *pIngredient,
	BOOL bAlwaysCheckOnPerson)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pIngredient, "Expected ingredient" );
	const RECIPE_DEFINITION *ptRecipeDef = RecipeGetDefinition( nRecipe );
	ASSERTX_RETFALSE(ptRecipeDef, "No recipe definition");

	int nNumIngredients = 0;

	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags)) != NULL)
	{
	
		// check if it matches ingredient
		if (RecipeUnitMatchesIngredient( pItem, pIngredient ) == TRUE)
		{
			// add to our count
			nNumIngredients += ItemGetQuantity( pItem );
		}
							
	}
		
	return nNumIngredients;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRecipeUnitHasEnoughOfIngredient(
	UNIT *pPlayer,
	int nRecipe,
	const RECIPE_INGREDIENT *pIngredient,
	BOOL bAlwaysCheckOnPerson)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pIngredient, "Expected ingredient" );

	// get the count of ingredients that this player has ready to "cook"
	int nCount = RecipeGetAvailableIngredientCount( pPlayer, nRecipe, pIngredient, bAlwaysCheckOnPerson );
	
	// must have enough of it
	return nCount >= pIngredient->nQuantity;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeIsComplete(
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeList,
	UNITID idRecipeGiver,
	BOOL bAlwaysCheckOnPerson)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( nIndexInRecipeList >= 0 && nIndexInRecipeList < MAX_RECIPES, "Invalid recipe index" );
	ASSERTX_RETFALSE( idRecipeGiver != INVALID_ID, "Expected recipe giver id" );
	GAME *pGame = UnitGetGame( pPlayer );
	UNIT *pRecipeGiver = UnitGetById( pGame, idRecipeGiver );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver" );
	
	// get recipe ingredient instance
	RECIPE_INSTANCE_INGREDIENTS tInstanceIngredients;
	RecipeInstanceGetIngredients( pPlayer, nRecipe, nIndexInRecipeList, pRecipeGiver, &tInstanceIngredients );
				
	// go check all ingredients
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		const RECIPE_INGREDIENT *pIngredient = &tInstanceIngredients.tIngredient[ i ];

		// must have item class
		if (pIngredient->nItemClass != INVALID_LINK)
		{
			if (sRecipeUnitHasEnoughOfIngredient( pPlayer, nRecipe, pIngredient, bAlwaysCheckOnPerson ) == FALSE)
			{
				return FALSE;
			}
						
		}
				
	}
	
	// all ingredients found
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeItemIsComponent(
	UNIT *pItem,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver)
{	
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver unit" );
	UNITID idRecipeGiver = UnitGetId( pRecipeGiver );
	
	if (UnitGetStat( pItem, STATS_RECIPE_COMPONENT ) == nRecipe)
	{
		if (nIndexInRecipeList == INVALID_INDEX || UnitGetStat( pItem, STATS_RECIPE_COMPONENT_INDEX ) == nIndexInRecipeList)
		{
			UNITID idRecipeGiverInStat = UnitGetStat( pItem, STATS_RECIPE_GIVER_UNIT_ID );
			if (idRecipeGiverInStat == idRecipeGiver || 
				UnitIsA( UnitGetContainer( pItem ), UNITTYPE_SINGLE_USE_RECIPE ))
			{
				return TRUE;
			}
		}
	}
		
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RecipeGetIngredientInvLoc(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	ASSERTX_RETINVALID( pUnitData, "Expected unitdata" );
	return pUnitData->nInvLocRecipeIngredients;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RecipeGetResultsInvLoc(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	ASSERTX_RETINVALID( pUnitData, "Expected unitdata" );
	return pUnitData->nInvLocRecipeResults;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *RecipeGetComponentUnit(
	UNIT *pUnit,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	
	if (pRecipeGiver != NULL && UnitIsA( pRecipeGiver, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		return pRecipeGiver;
	}
	else
	{
		return pUnit;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *RecipeSingleUseGetFirstResult(
	UNIT *pRecipe)
{
	ASSERTX_RETNULL( pRecipe, "Expected recipe unit" );
	ASSERTX_RETNULL( UnitIsA( pRecipe, UNITTYPE_SINGLE_USE_RECIPE ), "Expected single use recipe unit" );
	int nInvLocResult = RecipeGetResultsInvLoc( pRecipe );
	
	// go through inventory
	UNIT *pItem = UnitInventoryLocationIterate( pRecipe, nInvLocResult, NULL );
	return pItem;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeIngredientInit(
	RECIPE_INGREDIENT *pIngredient)
{
	ASSERTX_RETURN( pIngredient, "Expected ingredient" );
	pIngredient->nItemClass = INVALID_LINK;
	pIngredient->nItemQuality = INVALID_LINK;
	pIngredient->nQuantity = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeIngredientInitFromUnit( 
	RECIPE_INGREDIENT *pIngredient, 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pIngredient, "Expected ingredient instance" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	pIngredient->nItemClass = UnitGetClass( pUnit );
	pIngredient->nQuantity = ItemGetQuantity( pUnit );
	pIngredient->nItemQuality = ItemGetQuality( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RecipeInstanceGetIngredients(
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver,
	RECIPE_INSTANCE_INGREDIENTS *pInstanceIngredients)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Invalid recipe" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	ASSERTX_RETURN( pInstanceIngredients, "Expected instance ingredients" );
	const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( nRecipe );
		
	// init ingredients
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		RECIPE_INGREDIENT *pIngredient = &pInstanceIngredients->tIngredient[ i ];
		sRecipeIngredientInit( pIngredient );
	}
	
	// see if there are real recipe instances
	int nNumIngredients = 0;
	if (RecipeGiverInstancesComponents( pRecipeGiver ))
	{		
			
		// which unit has the ingredients
		UNIT *pUnitWithInIngredients = RecipeGetComponentUnit( pUnit, pRecipeGiver );

		// get inventory slot for recipe ingredients
		int nInvLocIngredients = RecipeGetIngredientInvLoc( pUnitWithInIngredients );
		
		// go through inventory
		UNIT* pItem = NULL;
		while ((pItem = UnitInventoryLocationIterate( pUnitWithInIngredients, nInvLocIngredients, pItem )) != NULL)
		{
		
			// is this recipe result for the recipe in question
			if (RecipeItemIsComponent( pItem, nRecipe, nIndexInRecipeList, pRecipeGiver ))
			{
				ASSERTX_BREAK( nNumIngredients < MAX_RECIPE_INGREDIENTS, "Too many recipe ingredients in instance" );
				RECIPE_INGREDIENT *pIngredient = &pInstanceIngredients->tIngredient[ nNumIngredients++ ];
				sRecipeIngredientInitFromUnit( pIngredient, pItem );
			}
								
		}
			
	}
	else
	{	
		// no real instances of recipes (Mythos), just use the definition as is
		ASSERTX_RETURN(pRecipeDef, "No recipe definition");
		// just use the definition as is
		for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
		{
			const RECIPE_INGREDIENT_DEFINITION *pIngredientDef = &pRecipeDef->tIngredientDef[ i ];
			RECIPE_INGREDIENT *pIngredient = &pInstanceIngredients->tIngredient[ i ];
			pIngredient->nItemClass = pIngredientDef->nItemClass;
			pIngredient->nUnitType = pIngredientDef->nUnitType;
			pIngredient->nItemQuality = pIngredientDef->nItemQuality;
			pIngredient->nQuantity = pIngredientDef->nQuantityMin;  // always use min I guess
		}
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeResultsInit(
	RECIPE_INSTANCE_RESULTS *pResults)
{
	ASSERTX_RETURN( pResults, "Expected instance results" );
	pResults->nNumResults = 0;
	for (int i = 0; i < MAX_RECIPE_RESULTS; ++i)
	{
		pResults->idResult[ i ] = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RecipeInstanceGetResults(
	UNIT *pRecipe,
	RECIPE_INSTANCE_RESULTS *pResults)
{
	ASSERTX_RETURN( pRecipe, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pRecipe, UNITTYPE_SINGLE_USE_RECIPE ), "Expected single use recipe" );
	ASSERTX_RETURN( pResults, "Expected instance results" );

	// init results
	sRecipeResultsInit( pResults );
		
	// see if there are real recipe instances
	int nNumIngredients = 0;
	if (RecipeGiverInstancesComponents( pRecipe ))
	{		

		// get the recipe this unit creates
		int nRecipe = UnitGetStat( pRecipe, STATS_RECIPE_SINGLE_USE );
					
		// get inventory slot for recipe ingredients
		int nInvLocResults = RecipeGetResultsInvLoc( pRecipe );
		
		// go through inventory
		UNIT* pItem = NULL;
		while ((pItem = UnitInventoryLocationIterate( pRecipe, nInvLocResults, pItem )) != NULL)
		{
		
			// is this recipe result for the recipe in question
			if (RecipeItemIsComponent( pItem, nRecipe, INVALID_INDEX, pRecipe ))
			{
				ASSERTX_BREAK( nNumIngredients < MAX_RECIPE_RESULTS, "Too many recipe results in instance" );
				pResults->idResult[ pResults->nNumResults++ ] = UnitGetId( pItem );
			}
								
		}
			
	}


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeGiverInstancesComponents(
	UNIT *pRecipeGiver)
{
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver" );
	
	if (UnitIsTradesman( pRecipeGiver ) && UnitTradesmanSharesInventory( pRecipeGiver ))
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipePlayerKnowsNecessaryCategories( int nRecipe,
									  UNIT *pPlayer )
{
	return RecipePlayerKnowsNecessaryCategories( RecipeGetDefinition( nRecipe ), pPlayer );
}

//----------------------------------------------------------------------------
//returns if the player knows the necasary catetories to crate the recipe( and level of category )
//----------------------------------------------------------------------------
BOOL RecipePlayerKnowsNecessaryCategories( const RECIPE_DEFINITION *pRecipe,
									       UNIT *pPlayer )
{
	ASSERT_RETFALSE( pRecipe );
	ASSERT_RETFALSE( pPlayer );	

	for( int t = 0; t < MAX_RECIPE_CATETORIES_TO_CRAFT; t++ )
	{
		if( pRecipe->nRecipeCategoriesNecessaryToCraft[ t ] == INVALID_ID )
			break;
		int nLevel = RecipeGetCategoryLevel( pPlayer, pRecipe->nRecipeCategoriesNecessaryToCraft[ t ] );
		if( nLevel < pRecipe->nRecipeCategoriesLevelsNecessaryToCraft[ t ] )
			return FALSE;

	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipeUnitMatchesIngredientTugboat(	
	UNIT *pUnit,
	const RECIPE_INGREDIENT_DEFINITION *pIngredient)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pIngredient, "Expected ingredient" );

	// check for invalid ingredient
	if (pIngredient->nItemClass == INVALID_LINK)
	{
		return FALSE;
	}	

	// must be item	
	if (UnitGetGenus( pUnit ) != GENUS_ITEM)
	{
		return FALSE;
	}

	// must match class	
	if (UnitGetClass( pUnit ) != pIngredient->nItemClass)
	{
		return FALSE;
	}

	// must match quality
	if (pIngredient->nItemQuality != INVALID_LINK)
	{
		if (ItemGetQuality( pUnit ) != pIngredient->nItemQuality)
		{
			return FALSE;
		}
	}

	// note that we do *not* match quantity here, we're only concerned with "type"
	return TRUE;			

}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipePlayerCanCreateRecipe( UNIT *pPlayer,
								 int nRecipe,
								 BOOL bIgnoreIngredients )
{
	return RecipePlayerCanCreateRecipe( pPlayer, RecipeGetDefinition( nRecipe ), bIgnoreIngredients );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RecipePlayerCanCreateRecipe( UNIT *pPlayer,
								 const RECIPE_DEFINITION *pRecipe,
								 BOOL bIgnoreIngredients )
{
	ASSERT_RETFALSE( AppIsTugboat() ); //tugboat only
	ASSERT_RETFALSE( pRecipe );
	ASSERT_RETFALSE( pPlayer );
	if( RecipeCheatingEnabled())
		return TRUE;	//for cheating
	//first check necessary skills to craft
	if( !RecipePlayerKnowsNecessaryCategories( pRecipe, pPlayer ) )
		return FALSE;

	if( bIgnoreIngredients )
		return TRUE;

	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );


	// go check all ingredients
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		const RECIPE_INGREDIENT_DEFINITION nRecipeIngredient = pRecipe->tIngredientDef[ i ];				
		// must have item class
		if (nRecipeIngredient.nItemClass != INVALID_LINK)
		{
			int nNumIngredients( 0 );
			UNIT* pItem = NULL;
			while ((pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags)) != NULL)
			{
				// check if it matches ingredient
				if (RecipeUnitMatchesIngredientTugboat( pItem, &nRecipeIngredient ) == TRUE)
				{
					// add to our count
					nNumIngredients += ItemGetQuantity( pItem );
				}
			}
			if( nNumIngredients < nRecipeIngredient.nQuantityMin )
				return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//returns the value of a recipe property
//----------------------------------------------------------------------------
inline int sRecipeGetProperty( UNIT *pPlayer,
							   const RECIPE_DEFINITION *pRecipe,
							   RECIPE_PROPERTIES_ENUM nProperty,
							   RECIPE_VALUES_PER_PROP nRecipePropertValue )
{	
	ASSERT_RETZERO( nRecipePropertValue >= 0 && nRecipePropertValue < RECIPE_VALUE_COUNT_PER_PROPERTY );
	ASSERT_RETZERO( nProperty >= 0 && nProperty < NUM_RECIPE_PROP );	 
	ASSERT_RETZERO( pRecipe );
	ASSERT_RETZERO( pRecipe->nRewardItemClass[ 0 ] != INVALID_ID );
	ASSERT_RETZERO( pPlayer );
	//this will return 
	RecipeFillOutRecipePropertyValues( pPlayer, pRecipe );
	ASSERT_RETZERO( pPlayer->pRecipePropertyValues );      
	return pPlayer->pRecipePropertyValues->nValues[ nProperty ][nRecipePropertValue];
}

//----------------------------------------------------------------------------
//returns the value of a recipe property
//----------------------------------------------------------------------------
inline int sRecipeGetProperty( UNIT *pPlayer,
							   int nRecipeID,
							   RECIPE_PROPERTIES_ENUM nProperty,
							   RECIPE_VALUES_PER_PROP nRecipePropertValue )
{	
	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeID );
	return sRecipeGetProperty( pPlayer, pRecipe, nProperty, nRecipePropertValue );
}

//----------------------------------------------------------------------------
//returns TRUE if a given recipe has a the recipe property passed in
//----------------------------------------------------------------------------
BOOL RecipeHasProperty( int nRecipeID,
					   RECIPE_PROPERTIES_ENUM nProperty )
{
	return RecipeHasProperty( RecipeGetDefinition( nRecipeID ), nProperty );
}


//----------------------------------------------------------------------------
//returns TRUE if a given recipe has a the recipe property passed in
//----------------------------------------------------------------------------
BOOL RecipeHasProperty( const RECIPE_DEFINITION *pRecipeDefinition,
					    RECIPE_PROPERTIES_ENUM nProperty )
{
	ASSERT_RETFALSE( pRecipeDefinition );
	return pRecipeDefinition->nRecipePropertyEnabled[ nProperty ];
}
//----------------------------------------------------------------------------
//returns the recipe's value for a property
//----------------------------------------------------------------------------
int RecipeGetPropertyValue( UNIT *pPlayer,
						   int nRecipeID,
						   RECIPE_PROPERTIES_ENUM nRecipePropertyID,
						   float fPercent /*=0*/)
{
	return RecipeGetPropertyValue( pPlayer, RecipeGetDefinition( nRecipeID ), nRecipePropertyID, fPercent );
}
//----------------------------------------------------------------------------
//returns the recipe's value for a property
//----------------------------------------------------------------------------
int RecipeGetPropertyValue( UNIT *pPlayer,
						   const RECIPE_DEFINITION *pRecipeDefinition,
						   RECIPE_PROPERTIES_ENUM nRecipePropertyID,
						   float fPercent /*=0*/)
{
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pRecipeDefinition );
	ASSERT_RETZERO( (int)fPercent <= 100 );
	fPercent = MIN( 100.0f, fPercent );
	fPercent = MAX( 0.0f, fPercent );
	//get the base value of the recipe's property
	int nMinOrgValue = sRecipeGetProperty( pPlayer, pRecipeDefinition, nRecipePropertyID, RECIPE_VALUE_PER_PROP_MIN_VALUE );	
	int nMaxValueInitially = sRecipeGetProperty( pPlayer, pRecipeDefinition, nRecipePropertyID, RECIPE_VALUE_PER_PROP_MAX_VALUE );
	if( nMinOrgValue == 0 &&
		nMaxValueInitially == 0 )
	{
		return 0;
	}
	int nMinValue = nMinOrgValue;
	int nValueBetween = nMaxValueInitially - nMinValue;
	int nMaxValue = nMaxValueInitially;

	//get percent values now
	float fMaxPercentValue = (float)sRecipeGetProperty( pPlayer, pRecipeDefinition, nRecipePropertyID, RECIPE_VALUE_PER_PROP_MAX_VALUE_PERCENT );
	if( fMaxPercentValue == 0 &&
		fPercent != 0.0f )
	{
		fMaxPercentValue = (1.0f/(MAX( (float)nMinOrgValue, (float)nMaxValueInitially ) - MIN( (float)nMinOrgValue, (float)nMaxValueInitially ) )) * 100.0f;
	}
	int nValueToRemove = -1;
	if( nMaxValue > nMinValue )
	{
		nValueToRemove = (int)CEIL( (float)nValueBetween * ( 1.0f - (fMaxPercentValue/100.0f)) );
	}
	else
	{
		nValueToRemove = (int)FLOOR( (float)nValueBetween * ( 1.0f - (fMaxPercentValue/100.0f)) );
	}


	const RECIPE_PROPERTIES *pRecipeProperty = RecipeGetPropertyDefinition( nRecipePropertyID );
	ASSERT_RETVAL( pRecipeProperty, 0 );
	if( TESTBIT( pRecipeProperty->dwFlags, RECIPE_PROPERTIES_FLAG_COMPUTE_AS_RANGE ) )
	{
		nValueBetween = (int)((float)nValueBetween * fMaxPercentValue/100.0f);		
	}
	else
	{
		nValueToRemove = abs( nValueToRemove );
		nMinValue = nMinOrgValue - nValueToRemove;	
		nMaxValue = nMaxValueInitially - nValueToRemove;
		if( nMinValue <= 0 )
		{
			nMinValue = nMinOrgValue;
		}
		nValueBetween = nMaxValue - nMinValue;	
	}

	//now get the value
	int nValue = ((int)CEIL((float)nValueBetween * (fPercent/100.0f))) + nMinValue;

	//so they can never be the same - this only happens on very low level weapons where the
	//range isn't great enough. - this isn't pretty.
	if( nMaxValue == nMinValue ) //nvalue will be the same
	{			
		if( fPercent < 100.0f )
		{
			nValue += ( nMinOrgValue > 1 )?-1:0;
		}
		else
		{
			nValue++;
		}
	}
	return nValue;	
}

//----------------------------------------------------------------------------
//returns the min recipe's value for a property
//----------------------------------------------------------------------------
int RecipeGetMinPropertyValue( UNIT *pPlayer,
							  const RECIPE_DEFINITION *pRecipeDefinition,
							  RECIPE_PROPERTIES_ENUM nRecipePropertyID )
{
	return RecipeGetPropertyValue( pPlayer, pRecipeDefinition, nRecipePropertyID, .0f );
}
//----------------------------------------------------------------------------
//returns the max recipe's value for a property
//----------------------------------------------------------------------------
int RecipeGetMaxPropertyValue( UNIT *pPlayer,
							  const RECIPE_DEFINITION *pRecipeDefinition,
							  RECIPE_PROPERTIES_ENUM nRecipePropertyID )
{
	return RecipeGetPropertyValue( pPlayer, pRecipeDefinition, nRecipePropertyID, 100.0f );
}


//----------------------------------------------------------------------------
//returns the recipe's string ID
//----------------------------------------------------------------------------
int RecipeGetPropertyStringID( int nRecipePropertyID )
{
	ASSERT_RETINVALID( nRecipePropertyID >= 0 && nRecipePropertyID < NUM_RECIPE_PROP );
	const RECIPE_PROPERTIES *pProperty = RecipeGetPropertyDefinition( nRecipePropertyID );
	ASSERT_RETINVALID( pProperty );
	return pProperty->nString;
}

//----------------------------------------------------------------------------
//returns the recipe's chance
//----------------------------------------------------------------------------
int RecipeGetPropertyChanceOfSuccess( UNIT *pPlayer,
						   int nRecipeID,
						   RECIPE_PROPERTIES_ENUM nRecipePropertyID )
{
	return RecipeGetPropertyChanceOfSuccess( pPlayer, RecipeGetDefinition( nRecipeID ), nRecipePropertyID );
}

//----------------------------------------------------------------------------
//returns the recipe's chance
//----------------------------------------------------------------------------
int RecipeGetPropertyChanceOfSuccess( UNIT *pPlayer,
						   const RECIPE_DEFINITION *pRecipeDefinition,
						   RECIPE_PROPERTIES_ENUM nRecipePropertyID )
{
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pRecipeDefinition );
	//get the base value of the recipe's property
	int nMaxValue = sRecipeGetProperty( pPlayer, pRecipeDefinition, nRecipePropertyID, RECIPE_VALUE_PER_PROP_MAX_PERCENT );	
	return MAX( 0, nMaxValue ); //can't go negative
}

//----------------------------------------------------------------------------
//returns the categories level (points invested in )
//----------------------------------------------------------------------------
int RecipeGetCategoryLevel( UNIT *pPlayer,
						   int nRecipeCategory,
						   int nParam1,
						   int nParam2,
						   int nSkill,
						   int nSkillLevel )
{
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( pGame );	
	const RECIPE_CATEGORY *pRecipeCategory = RecipeGetCategoryDefinition( nRecipeCategory ); 
	ASSERT_RETFALSE( pRecipeCategory );	
	PCODE codeExecute = pRecipeCategory->codeScripts[RECIPE_CATEGORY_SCRIPT_LEVEL];
	int nLevel( 0 );
	if( codeExecute  )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_RECIPECATEGORIES, codeExecute, &code_len);		
		nLevel = VMExecI( pGame, pPlayer, pPlayer, NULL, nParam1, nParam2, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
	}
	return nLevel;
}

inline void sRecipeClearOutPropertyValues( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( pPlayer->pRecipePropertyValues );
	ZeroMemory( pPlayer->pRecipePropertyValues, sizeof( RECIPE_PROPERTY_VALUES ) );
	pPlayer->pRecipePropertyValues->nLastRecipeID = -1;
}

//----------------------------------------------------------------------------
//Since we are using tree's in the crafting system and we have all our skills
//copied, we can't set a generic stat per skill to raise individual tree stat values.
//For example, we having forging in both the Sword and Mace tree. But if we 
//set a stat that gave forging a plus 10, then it would work for both Swords and Axes.
//which we don't want. Soooooo we'll keep our memory foot print down by computing the stats
//from the skills on the item creation and then remove those values after the item is created.
//----------------------------------------------------------------------------
void RecipeFillOutRecipePropertyValues( UNIT *pPlayer,
									    const RECIPE_DEFINITION *pRecipeDefinition )
{
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( pRecipeDefinition );
	ASSERT_RETURN( pPlayer->pRecipePropertyValues );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );	
	int nRecipeID = ExcelGetLineByCode( EXCEL_CONTEXT( pGame ),DATATABLE_RECIPES, pRecipeDefinition->wCode );
	RECIPE_PROPERTY_VALUES &nPropertyValues = *pPlayer->pRecipePropertyValues;		
	// If we do this, we'll never see properties update based on skill investment unless
	// you flip back and forth between two recipes
	//if( nPropertyValues.nLastRecipeID == nRecipeID )
	//	return;	//already filled out with current recipe
	sRecipeClearOutPropertyValues( pPlayer );	
	nPropertyValues.nLastRecipeID = nRecipeID;	
	STATS *pStats = StatsListInit( pGame );
	ASSERT_RETURN( pStats );
	//we now have everything we need to know


	//first we get all the base values of the recipe
	for( int nRecipePropertyID = 0;
		 nRecipePropertyID < NUM_RECIPE_PROP; 
		 nRecipePropertyID++ )
	{
		for( int nRecipePropValueID = 0;
			 nRecipePropValueID < RECIPE_VALUE_COUNT_PER_PROPERTY;
			 nRecipePropValueID++ )
		{
			nPropertyValues.nValues[ nRecipePropertyID ][ nRecipePropValueID ] = pRecipeDefinition->nRecipeProperties[ nRecipePropertyID ][ nRecipePropValueID ];
		}
	}
	//second we get all the skills that the category tells us to check, which adds on more base values
	for( int i = 0; i < MAX_RECIPE_CATETORIES_TO_CRAFT; i++ )
	{
		if( pRecipeDefinition->nRecipeCategoriesNecessaryToCraft[ i ] == INVALID_ID )
			continue;
		int nRecipeCategoryID = pRecipeDefinition->nRecipeCategoriesNecessaryToCraft[ i ];
		const RECIPE_CATEGORY *pCategory = RecipeGetCategoryDefinition( nRecipeCategoryID );
		ASSERT_RETURN( pCategory );
		//next add on the skill stats
		for( int t = 0; t < MAX_RECIPE_CATEGORY_SKILLS_TO_EXECUTE; t++ )
		{
			if( pCategory->nSkillsToExecuteOnItem[ t ] == INVALID_ID )
				continue;
			SkillExecuteScript( SKILL_SCRIPT_CRAFTING_PROPERTIES,
								pPlayer,
								pCategory->nSkillsToExecuteOnItem[ t ], //skill
								INVALID_SKILL_LEVEL,	//skill level
								NULL,					//object
								pStats );				//stats to add the values to
		}
	}

	//before the last step we add any bonuses we get for crafting ( usually crafting zones )
	int nBonus = UnitGetStat( pPlayer, STATS_CRAFTING_BONUS );
	if( nBonus > 0 )
	{
		for( int nRecipePropertyID = 0;
			nRecipePropertyID < NUM_RECIPE_PROP; 
			nRecipePropertyID++ )
		{
			if( nPropertyValues.nValues[ nRecipePropertyID ][ RECIPE_VALUE_PER_PROP_MAX_PERCENT ] != 0 )
			{
				//we have to have at least 1% to show up!
				nPropertyValues.nValues[ nRecipePropertyID ][ RECIPE_VALUE_PER_PROP_MAX_PERCENT ] = MAX( 1, nPropertyValues.nValues[ nRecipePropertyID ][ RECIPE_VALUE_PER_PROP_MAX_PERCENT ] - nBonus );				
			}
		}
	}

	//last we execute the values on the recipe properties to tell use what the values should be computed out as
	for( int nRecipePropertyID = 0;
		nRecipePropertyID < NUM_RECIPE_PROP; 
		nRecipePropertyID++ )
	{
		for( int nRecipePropValueID = 0;
			nRecipePropValueID < RECIPE_VALUE_COUNT_PER_PROPERTY;
			nRecipePropValueID++ )
		{
			const RECIPE_PROPERTIES *pRecipeProps = RecipeGetPropertyDefinition( nRecipePropertyID );
			int nValue = nPropertyValues.nValues[ nRecipePropertyID ][ nRecipePropValueID ];
			if( pRecipeProps->codeValueManipulator[ nRecipePropValueID ] != NULL_CODE )
			{
				int code_len( 0 );
				BYTE * code_ptr = ExcelGetScriptCode( EXCEL_CONTEXT(), DATATABLE_RECIPEPROPERTIES, pRecipeProps->codeValueManipulator[ nRecipePropValueID ], &code_len);		
				nPropertyValues.nValues[ nRecipePropertyID ][ nRecipePropValueID ] = VMExecI( pGame, pPlayer, NULL, nValue, pRecipeDefinition->nRewardItemClass[ 0 ], code_ptr, code_len, NULL );			
			}			 
		}
	}

	//clear those stats out
	StatsListFree(pGame, pStats);

}


//----------------------------------------------------------------------------
//returns a number between 0-1. 0 - no chance for failure, 1 - Will Fail
//----------------------------------------------------------------------------
float RecipeGetFailureChance( UNIT *pPlayer,
							  int nRecipeID )
{
	ASSERT_RETFALSE( pPlayer );	
	ASSERT_RETFALSE( nRecipeID >= 0 );
	const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( nRecipeID );
	ASSERT_RETFALSE( pRecipeDefinition );	
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( pGame );	
	
	//chances for failure
//#define CHANCE_MULT
#ifdef CHANCE_MULT 
	float fChanceOfSuccess( 1.0f );	
#else
	float fChanceOfFailure( 0.0f );
#endif

	for( int t = 0; t < NUM_RECIPE_PROP; t++ )
	{		
		if( t == RECIPE_PROP_CHANCEFORSUCCESS )
			continue;		//we ignore this		
		//if( !RecipeHasProperty( pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)t ) )
		//	continue;		
		float fPlayerChanceMod = (float)RecipeGetPlayerInputForRecipeProp( pPlayer, nRecipeID, (RECIPE_PROPERTIES_ENUM)t );		
		float fMaxPercentChance = (float)sRecipeGetProperty( pPlayer, pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)t, RECIPE_VALUE_PER_PROP_MAX_PERCENT );
		fPlayerChanceMod = fMaxPercentChance * ( fPlayerChanceMod/100.f );
		//these stats should be adjusted from the UI( client )		
		if( ((int)fPlayerChanceMod) != 0 )
		{			
#ifdef CHANCE_MULT
			float fChance = (float)fPlayerChanceMod/fMaxPercentChance;
			fChance *= MIN( 1.0f, fMaxPercentChance/100.0f);
			fChance = 1.0f - fChance;
			fChanceOfSuccess *= fChance;
#else
			fChanceOfFailure += fPlayerChanceMod;
#endif
		}
	}
#ifdef CHANCE_MULT
	fChanceOfSuccess = MIN( 1.0f, fChanceOfSuccess );
	return 1.0f - fChanceOfSuccess;
#else
	return MIN( 1.0f, fChanceOfFailure/100.0f );
#endif
}

void RecipeInitUnit( GAME *pGame, 
					 UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pGame );
	RecipeFreeUnit( pGame, pUnit );
	if( UnitGetGenus( pUnit ) == GENUS_PLAYER)
	{		
		pUnit->pRecipePropertyValues = (RECIPE_PROPERTY_VALUES *)GMALLOCZ( pGame, sizeof( RECIPE_PROPERTY_VALUES ) );
		sRecipeClearOutPropertyValues( pUnit );
	}
}

void RecipeFreeUnit( GAME *pGame, 
					 UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pGame );
	if( pUnit->pRecipePropertyValues )
	{
		GFREE( pGame, pUnit->pRecipePropertyValues );
		pUnit->pRecipePropertyValues = NULL;
	}
}

//----------------------------------------------------------------------------
//Returns if the recipe can be sold at a merchant
//----------------------------------------------------------------------------
BOOL RecipeCanSpawnAtMerchant( int nRecipeID )
{
	return RecipeCanSpawnAtMerchant( RecipeGetDefinition( nRecipeID ) );
}
//----------------------------------------------------------------------------
//Returns if the recipe can be sold at a merchant
//----------------------------------------------------------------------------
BOOL RecipeCanSpawnAtMerchant( const RECIPE_DEFINITION *pRecipeDefinition )
{
	ASSERT_RETFALSE( pRecipeDefinition );
	return TESTBIT( pRecipeDefinition->pdwFlags, RECIPE_FLAG_CAN_SPAWN_AT_MERCHENT );
}

//----------------------------------------------------------------------------
//Returns if the recipe can be sold at a merchant
//----------------------------------------------------------------------------
void RecipeModifyCreatedItemByCraftingTreeInvestment( UNIT *pPlayer,
													  UNIT *pItem,
													  int nRecipeID )
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pItem, "Expected item" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( pGame, "Expected game" );
	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeID );
	ASSERTX_RETURN( pRecipe, "Expected item" );
	for( int t = 0; t < MAX_RECIPE_CATETORIES_TO_CRAFT; t++ )
	{
		if( pRecipe->nRecipeCategoriesNecessaryToCraft[ t ] == INVALID_ID )
			continue;
		const RECIPE_CATEGORY *pRecipeCategory = RecipeGetCategoryDefinition( pRecipe->nRecipeCategoriesNecessaryToCraft[ t ] );
		ASSERT_CONTINUE( pRecipeCategory );
		for( int i = 0; i < MAX_RECIPE_CATEGORY_SKILLS_TO_EXECUTE; i++ )
		{
			if( pRecipeCategory->nSkillsToExecuteOnItem[ i ] == INVALID_ID )
				continue;
			//execute skill on item
			SkillExecuteScript( SKILL_SCRIPT_CRAFTING, 
								pPlayer,  
								pRecipeCategory->nSkillsToExecuteOnItem[ i ],
								INVALID_SKILL_LEVEL,	//skill level
								pItem,					//object
								NULL,					//Stats
								nRecipeID,				//param1
								pRecipe->nRecipeCategoriesNecessaryToCraft[ t ] );	//param2

		}
	}	
	PCODE pCode = pItem->pUnitData->codeRecipeProperty;
	if( pCode != NULL_CODE )
	{
		int code_len( 0 );
		BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_ITEMS, pCode, &code_len);		
		VMExecI( pGame, pItem, pPlayer, NULL, nRecipeID, pRecipe->nRecipeCategoriesNecessaryToCraft[ 0 ], INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code_ptr, code_len);		
	}

	
}

int RecipeGetPlayerInputForRecipeProp( UNIT *pPlayer,
									   int nRecipeID,
									   RECIPE_PROPERTIES_ENUM nRecipeProp  )
{
	ASSERT_RETZERO( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );
	if( IS_SERVER( pGame ) )
	{

		return UnitGetStat( pPlayer, STATS_RECIPE_PROP_PLAYER_INPUT, nRecipeProp );	

	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		return UIGetRecipePCTOfPropertyModifiedByPlayer( nRecipeID, nRecipeProp );
#else
		return 0;
#endif
	}
	
}

int RecipeGetPlayerInputForRecipeProp( UNIT *pPlayer,
									  const RECIPE_DEFINITION *pRecipeDefinition,
									  RECIPE_PROPERTIES_ENUM nRecipeProp  )
{
	return RecipeGetPlayerInputForRecipeProp( pPlayer,
											  ExcelGetLineByCode( EXCEL_CONTEXT( NULL ), DATATABLE_RECIPES, pRecipeDefinition->wCode ),
											  nRecipeProp );
}