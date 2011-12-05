//----------------------------------------------------------------------------
// FILE: s_recipe.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __S_RECIPE_H_
#pragma message ("    HINT: " __S_RECIPE_H_ " included multiple times in this translation unit")
#else
#define __S_RECIPE_H_

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
enum UNIT_INTERACT;
struct INVENTORY_LOCATION;

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------



UNIT_INTERACT s_RecipeTradesmanInteract( 
	UNIT *pPlayer,
	UNIT *pNPC);

void s_RecipeCreate( 
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe,
	int nIndexInRecipeList);

void s_RecipeBuy( 
	UNIT *pPlayer,
	UNIT *pRecipeGiver,
	int nRecipe);


void s_RecipeForceClose(
	UNIT *pPlayer,
	UNIT *pRecipe);
	
void s_RecipeClose( 
	UNIT* pPlayer);

BOOL s_RecipeSplitIngredientToLocation( 
	UNIT *pUnit, 
	UNIT *pItem, 
	const INVENTORY_LOCATION &tInvLocation);

void s_RecipeSelect( 
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver);

void s_RecipeOpen(
	UNIT *pPlayer,
	int nRecipe,
	UNIT *pRecipeGiver);

void s_RecipeListOpen(
	UNIT *pPlayer,
	int nRecipeList,
	UNIT *pRecipeGiver);

BOOL s_RecipeSingleUseInit(
	UNIT *pItem);

void s_RecipeVersion(
	UNIT *pUnit);

int s_RecipeTradesmanGetCurrentRecipeList(
	UNIT *pPlayer,
	UNIT *pRecipeGiver);



//Assigns a specific recipe to a recipe item
BOOL s_RecipeAssignSpecificRecipeID( UNIT *pRecipeItem,
								     int nRecipeID );

//Assigns a random recipe based off the players level to the recipe item
BOOL s_RecipeAssignRandomRecipeID( UNIT *pPlayer,
								   UNIT *pRecipeItem );


//Clears all players modified stats
void s_RecipeClearPlayerModifiedRecipeProperties( UNIT *pPlayer );

//WHen the player adjusts his recipes chances this saves the values per property
void s_RecipeSetPlayerModifiedRecipeProperty( UNIT *pPlayer,
											  int nProperty,
											  int nValue );

#endif