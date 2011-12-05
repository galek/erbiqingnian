//----------------------------------------------------------------------------
// FILE: recipe.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

/*
Tradesman will exchange completed goods for recipe components.
*/

#ifndef __RECIPE_H_
#define __RECIPE_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __DIALOG_H_
#include "dialog.h"
#endif

#include "../data_common/excel/recipe_properties_hdr.h"


//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
struct DATA_TABLE;

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

#define MAX_RECIPE_INGREDIENTS		6			// number of ingredients a recipe can have
#define MAX_RECIPE_RESULTS			6				// number of recipe results allowed	
#define MAX_RECIPES					10				// number of recipes in a list
#define MAX_MIN_MAX_RANGE			2				// range for MIN max
#define MAX_RECIPE_CATETORIES_TO_CRAFT	3				// max number of skills necessary to craft recipe
#define MAX_RECIPE_CATEGORY_SKILLS_TO_EXECUTE 20	// max number of skills that will execute on the resulting crafted item
#define MAX_RECIPE_PROPERTIES_TO_ADJUST 10			// max number of skills that will execute on the resulting crafted item
#define MAX_RECIPE_UNITS_DROP_FROM		10			// max unit types to drop recipe from

//----------------------------------------------------------------------------
// Enums
//----------------------------------------------------------------------------

enum RECIPE_FLAGS
{	
	RECIPE_FLAG_CAN_BE_LEARNED = 0,
	RECIPE_FLAG_CAN_SPAWN,
	RECIPE_FLAG_CAN_SPAWN_AT_MERCHENT,
	NUM_RECIPE_FLAGS,
};

enum RECIPE_PROPERTIES_FLAGS
{	
	RECIPE_PROPERTIES_FLAG_COMPUTE_AS_RANGE = 0,
	RECIPE_PROPERTIES_FLAG_IGNORE_DEFAULT_PERCENT_INVESTED
};


enum RECIPE_CATEGORY_SCRIPTS
{
	RECIPE_CATEGORY_SCRIPT_LEVEL,		//tells what level the category is
	RECIPE_CATEGORY_SCRIPT_COUNT
};
//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Recipes in Tugboat require certain levels of a category to actually craft the item
//----------------------------------------------------------------------------
struct RECIPE_CATEGORY
{
	char			szName[DEFAULT_INDEX_SIZE];						// unique task name for category
	int				nString;										// category name string
	PCODE			codeScripts[RECIPE_CATEGORY_SCRIPT_COUNT];		// scripts for recipe categories	
	int				nSkillsToExecuteOnItem[ MAX_RECIPE_CATEGORY_SKILLS_TO_EXECUTE ];	//number of skills to execute on the item created for a recipe
	int				nRecipePane;									// recipe pane the recipe appears on - used for merchants
};

//----------------------------------------------------------------------------
//Each property of a recipe has the following properties....
//such as Min and Max values of a property
//such as min and max chance for a property to occur
//----------------------------------------------------------------------------
const enum RECIPE_VALUES_PER_PROP
{
	RECIPE_VALUE_PER_PROP_MIN_PERCENT,
	RECIPE_VALUE_PER_PROP_MAX_PERCENT,
	RECIPE_VALUE_PER_PROP_MIN_VALUE,
	RECIPE_VALUE_PER_PROP_MAX_VALUE,
	RECIPE_VALUE_PER_PROP_MAX_VALUE_PERCENT,
	RECIPE_VALUE_COUNT_PER_PROPERTY
};



struct RECIPE_PROPERTY_VALUES
{
	int				nLastRecipeID;
	int				nValues[ NUM_RECIPE_PROP ][ RECIPE_VALUE_COUNT_PER_PROPERTY ];
};


//----------------------------------------------------------------------------
//Recipes can hold certain properties for tugboat
//----------------------------------------------------------------------------
struct RECIPE_PROPERTIES
{
	WORD			wCode;											// unique recipe property id	
	char			szName[DEFAULT_INDEX_SIZE];						// unique task name for category
	PCODE			codeValueManipulator[ RECIPE_VALUE_COUNT_PER_PROPERTY ];	//each value will be manipulated by the script
	int				nString;										 // string describing the property
	int				nDefaultRange;									 // default range for the player to adjust values
	int				nDefaultValues[RECIPE_VALUE_COUNT_PER_PROPERTY]; // default values
	DWORD			dwFlags;
};



//----------------------------------------------------------------------------
struct RECIPE_INGREDIENT_DEFINITION
{
	int nItemClass;
	int nUnitType;
	int nItemQuality;
	int nQuantityMin;
	int nQuantityMax;
};

//----------------------------------------------------------------------------
struct RECIPE_INGREDIENT
{
	int nItemClass;
	int nUnitType;
	int nItemQuality;
	int nQuantity;
};

//----------------------------------------------------------------------------
struct RECIPE_INSTANCE_INGREDIENTS
{
	RECIPE_INGREDIENT tIngredient[ MAX_RECIPE_INGREDIENTS ];
};

//----------------------------------------------------------------------------
struct RECIPE_INSTANCE_RESULTS
{
	UNITID idResult[ MAX_RECIPE_RESULTS ];
	int nNumResults;
};

//----------------------------------------------------------------------------
struct RECIPE_DEFINITION
{
	char szName[DEFAULT_INDEX_SIZE];	// unique task name for recipe
	
	WORD wCode;							// unique recipe id	
	BOOL bCube;
	BOOL bAlwaysKnown;
	BOOL bDontRequireExactIngredients;	// this cube recipe allows other ingredients to be present besides the required ones
	BOOL bAllowInRandomSingleUse;		// this recipe can be used when creating random recipes
	BOOL bRemoveOnLoad;					// old recipe that we want to remove when loaded
	int nWeight;						// weight for random recipes
	int	nExperience;					// xp reward
	int	nGold;							// gold reward
	
	//int	nItemsClassToCollect[MAX_RECIPE_INGREDIENTS];		// List of item types to collect
	//int	nItemsNumberToCollect[MAX_RECIPE_INGREDIENTS];		// number of each item to collect	
	//int	nItemsNumberToCollectMax[MAX_RECIPE_INGREDIENTS];	// some randomness for recipes that support instances
	//int	nItemsQualityToCollect[MAX_RECIPE_INGREDIENTS];		// required qualities of each item if any
	BOOL bResultQualityModifiesIngredientQuantity;			// the quality of the result item will modify the quantity of the ingredients needed
	RECIPE_INGREDIENT_DEFINITION tIngredientDef[ MAX_RECIPE_INGREDIENTS ];				// convenient structure to look at for an ingredient instead of individual arrays
	
	int nRewardItemClass[MAX_RECIPE_RESULTS];				// reward item types
	int nTreasureClassResult[MAX_RECIPE_RESULTS];			// result item(s)	
	
	int	nInvLocIngredients;				// ingredients must be put in this slot to create the recipe
	
	//flags for the recipe
	DWORD					pdwFlags[DWORD_FLAG_SIZE(NUM_RECIPE_FLAGS)];				// please help move bool values into here. Save some memory
	int						nSpawnLevels[ MAX_MIN_MAX_RANGE ];							// Spawn levels for the recipe
	int						nRecipeCategoriesNecessaryToCraft[ MAX_RECIPE_CATETORIES_TO_CRAFT ];		//skills necessary to craft the recipe
	int						nRecipeCategoriesLevelsNecessaryToCraft[ MAX_RECIPE_CATETORIES_TO_CRAFT ];	//skill levels necessary to craft the recipe

	int						nRecipeProperties[ NUM_RECIPE_PROP ][ RECIPE_VALUE_COUNT_PER_PROPERTY ];	//every recipe can have properties.
	BOOL					nRecipePropertyEnabled[ NUM_RECIPE_PROP ];

	//price of recipe
	int						nBaseCost;

	PCODE					codeProperties;

	//recipes spawn for
	int						nSpawnForUnittypes[ MAX_RECIPE_UNITS_DROP_FROM ];
	int						nSpawnForMonsterClass[ MAX_RECIPE_UNITS_DROP_FROM ];
};

//----------------------------------------------------------------------------
struct RECIPELIST_DEFINITION
{
	char							szName[DEFAULT_INDEX_SIZE];		// unique recipelist name
	WORD							wCode;							// unique recipe list id	
	int								nRecipes[MAX_RECIPES];			// List of recipes
	BOOL							bRandomlySelectable;			// can be randomly selected
	int								nRandomSelectWeight;			// weight for random selection
};

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

const RECIPE_PROPERTIES *RecipeGetPropertyDefinition(
	int nRecipePropertyID );

const RECIPE_DEFINITION *RecipeGetDefinition(
	int nRecipe);

const RECIPELIST_DEFINITION *RecipeListGetDefinition(
	int nRecipeList);

const RECIPE_CATEGORY *RecipeGetCategoryDefinition(
	int nRecipeCategoryID );

BOOL RecipeDefinitionPostProcess(
		struct EXCEL_TABLE * table );

BOOL RecipeDefinitionPostProcessRow(
	struct EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata);
		
BOOL RecipeIsComplete(
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeList,
	UNITID idRecipeGiver,
	BOOL bAlwaysCheckOnPerson);

int RecipeGetAvailableIngredientCount(
	UNIT *pPlayer,
	int nRecipe,
	const RECIPE_INGREDIENT *pIngredient,
	BOOL bAlwaysCheckOnPerson);

BOOL RecipeUnitMatchesIngredient(	
	UNIT *pUnit,
	const RECIPE_INGREDIENT *pIngredient);

BOOL RecipeUnitMatchesIngredientTugboat(	
	 UNIT *pUnit,
	 const RECIPE_INGREDIENT_DEFINITION *pIngredient);

const BOOL RecipeFindIngredientMatchingItem(
	UNIT *pItem,
	UNIT *pPlayer,
	int nRecipe,
	int nIndexInRecipeLilst,
	UNIT *pRecipeGiver,
	RECIPE_INGREDIENT *pIngredientToReturn);
	
BOOL RecipeItemIsComponent(
	UNIT *pItem,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pRecipeGiver);

int RecipeGetIngredientInvLoc(
	UNIT *pUnit);

int RecipeGetResultsInvLoc(
	UNIT *pUnit);

UNIT *RecipeGetComponentUnit(
	UNIT *pUnit,
	UNIT *pRecipeGiver);
	
UNIT *RecipeSingleUseGetFirstResult(
	UNIT *pRecipe);
	
void RecipeInstanceGetIngredients(
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNIT *pGiver,
	RECIPE_INSTANCE_INGREDIENTS *pIngredients);

void RecipeInstanceGetResults(
	UNIT *pRecipe,
	RECIPE_INSTANCE_RESULTS *pResults);

BOOL RecipeGiverInstancesComponents(
	UNIT *pRecipeGiver);

BOOL RecipePlayerKnowsNecessaryCategories( int nRecipe,
									       UNIT *pPlayer );

BOOL RecipePlayerKnowsNecessaryCategories( const RECIPE_DEFINITION *pRecipe,
										   UNIT *pPlayer );

BOOL RecipePlayerCanCreateRecipe( UNIT *pPlayer,
								 int nRecipe,
								 BOOL bIgnoreIngredients );

BOOL RecipePlayerCanCreateRecipe( UNIT *pPlayer,
								 const RECIPE_DEFINITION *pRecipe,
								 BOOL bIgnoreIngredients );	

int RecipeGetCategoryLevel( UNIT *pPlayer,
						    int nRecipeCategory,
						    int nParam1 = 0,
						    int nParam2 = 0,
						    int nSkill = 0,
						    int nSkillLevel = 0 );



float RecipeGetFailureChance( UNIT *pPlayer,
							  int nRecipeID );


BOOL RecipeHasProperty( int nRecipeID,
					   RECIPE_PROPERTIES_ENUM nProperty );

BOOL RecipeHasProperty( const RECIPE_DEFINITION *pRecipeDefinition,
					    RECIPE_PROPERTIES_ENUM nProperty );


int RecipeGetPropertyChanceOfSuccess( UNIT *pPlayer,
						    int nRecipeID,
							RECIPE_PROPERTIES_ENUM nRecipePropertyID );
int RecipeGetPropertyChanceOfSuccess( UNIT *pPlayer,
						   const RECIPE_DEFINITION *pRecipeDefinition,
						   RECIPE_PROPERTIES_ENUM nRecipePropertyID );
	
int RecipeGetPropertyValue( UNIT *pPlayer,
							int nRecipeID,
							RECIPE_PROPERTIES_ENUM nRecipePropertyID,
							float fPercent = 0.0f );
int RecipeGetPropertyValue( UNIT *pPlayer,
							const RECIPE_DEFINITION *pRecipeDefinition,
							RECIPE_PROPERTIES_ENUM nRecipePropertyID,
							float fPercent = 0.0f);

int RecipeGetMinPropertyValue( UNIT *pPlayer,
						       const RECIPE_DEFINITION *pRecipeDefinition,
						       RECIPE_PROPERTIES_ENUM nRecipePropertyID );

int RecipeGetMaxPropertyValue( UNIT *pPlayer,
							  const RECIPE_DEFINITION *pRecipeDefinition,
							  RECIPE_PROPERTIES_ENUM nRecipePropertyID );


BOOL RecipeCheatingEnabled();


int RecipeGetPropertyStringID( int nRecipePropertyID );

void RecipeFillOutRecipePropertyValues( UNIT *pPlayer,
									    const RECIPE_DEFINITION *pRecipeDefinition );

void RecipeInitUnit( GAME *pGame, UNIT *pUnit );
void RecipeFreeUnit( GAME *pGame, UNIT *pUnit );

BOOL RecipeCanSpawnAtMerchant( int nRecipeID );
BOOL RecipeCanSpawnAtMerchant( const RECIPE_DEFINITION *pRecipeDefinition );

void RecipeModifyCreatedItemByCraftingTreeInvestment( UNIT *pPlayer,
													  UNIT *pItem,
													  int nRecipeID );

int RecipeGetPlayerInputForRecipeProp( UNIT *pPlayer,
									   int nRecipeID,
									   RECIPE_PROPERTIES_ENUM nRecipeProp );

int RecipeGetPlayerInputForRecipeProp( UNIT *pPlayer,
									   const RECIPE_DEFINITION *pRecipeDefinition,
									   RECIPE_PROPERTIES_ENUM nRecipeProp );


#if ISVERSION(DEVELOPMENT)
BOOL RecipeToggleCheating();
#endif
#endif
